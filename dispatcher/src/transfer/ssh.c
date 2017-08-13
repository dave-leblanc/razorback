#include "config.h"
#include <razorback/types.h>
#include <razorback/nugget.h>


#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>
#include <libssh/messages.h>
#define WITH_SERVER
#include <libssh/sftp.h>
#include <libssh/string.h>


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>

#include "dispatcher.h"
#include "configuration.h"
#include "database.h"
#include "datastore.h"
#include "transfer/core.h"

#define KEYS_FOLDER ETC_DIR "/"

static bool Transfer_Server_SSH_Start(void);
static struct DatabaseConnection *dbCon = NULL;

static struct TransferServerDescriptor descriptor = {
    1,
    "SSH",
    "SSH scp based transfer server",
    Transfer_Server_SSH_Start
};

struct SFTP_Session
{
    ssh_session ssh;
    ssh_channel chan;
    struct Nugget *nugget;
};

bool
Transfer_Server_SSH_Init(void)
{
    return TransferServer_Register(&descriptor);
}

static int 
auth_password(struct SFTP_Session *session, const char *user, const char *password)
{
    uuid_t nuggetId;
    uuid_parse(user, nuggetId);

    if (Datastore_Get_NuggetByUUID(dbCon, nuggetId, &session->nugget) != R_FOUND)
    {
        rzb_log(LOG_ERR, "%s: Invalid nugget ID", __func__);
        return 0;
    }


    if (strcasecmp (password, Razorback_Get_Transfer_Password()) != 0)
        return 0;

    return 1; // authenticated
}

static int authenticate(struct SFTP_Session *session) {
    ssh_message message;

    do {
        message=ssh_message_get(session->ssh);
        if(!message)
            break;
        switch(ssh_message_type(message)){
            case SSH_REQUEST_AUTH:
                switch(ssh_message_subtype(message)){
                    case SSH_AUTH_METHOD_PASSWORD:
                        if(auth_password(session, ssh_message_auth_user(message),
                                ssh_message_auth_password(message)))
                        {
                               ssh_message_auth_reply_success(message,0);
                               ssh_message_free(message);
                               return 1;
                        }
                        ssh_message_auth_set_methods(message,
                                                SSH_AUTH_METHOD_PASSWORD |
                                                SSH_AUTH_METHOD_INTERACTIVE);
                        // not authenticated, send default message
                        ssh_message_reply_default(message);
                        break;

                    case SSH_AUTH_METHOD_NONE:
                    default:
                        rzb_log(LOG_ERR, "%s: User %s wants to auth with unknown auth %d", __func__,
                               ssh_message_auth_user(message),
                               ssh_message_subtype(message));
                        ssh_message_auth_set_methods(message,
                                                SSH_AUTH_METHOD_PASSWORD |
                                                SSH_AUTH_METHOD_INTERACTIVE);
                        ssh_message_reply_default(message);
                        break;
                }
                break;
            default:
                ssh_message_auth_set_methods(message,
                                                SSH_AUTH_METHOD_PASSWORD |
                                                SSH_AUTH_METHOD_INTERACTIVE);
                ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    } while (1);
    return 0;
}

#define TYPE_DIR 1
#define TYPE_FILE 2
struct sftp_handle {
    int type;
    size_t offset;
    char *name;
    int eof;
    DIR *dir;
    FILE *file;
};
int reply_status(sftp_client_message msg){
    switch(errno){
        case EACCES:
            return sftp_reply_status(msg,SSH_FX_PERMISSION_DENIED,
                                     "permission denied");
        case ENOENT:
            return sftp_reply_status(msg,SSH_FX_NO_SUCH_FILE,
                                     "no such file or directory");
        case ENOTDIR:
            return sftp_reply_status(msg,SSH_FX_FAILURE,
                                     "not a directory");
        default:
            return sftp_reply_status(msg,SSH_FX_FAILURE,NULL);
    }
}

void handle_opendir(sftp_client_message msg){
    DIR *dir=opendir(msg->filename);
    struct sftp_handle *hdl;
    ssh_string handle;
    if(!dir){
        reply_status(msg);
        return;
    }
    hdl=malloc(sizeof(struct sftp_handle));
    memset(hdl,0,sizeof(struct sftp_handle));
    hdl->type=TYPE_DIR;
    hdl->offset=0;
    hdl->dir=dir;
    hdl->name=strdup(msg->filename);
    handle=sftp_handle_alloc(msg->sftp,hdl);
    sftp_reply_handle(msg,handle);
    free(handle);
}

sftp_attributes attr_from_stat(struct stat *statbuf){
    sftp_attributes attr=malloc(sizeof(struct sftp_attributes_struct));
    memset(attr,0,sizeof(*attr));
    attr->size=statbuf->st_size;
    attr->uid=statbuf->st_uid;
    attr->gid=statbuf->st_gid;
    attr->permissions=statbuf->st_mode;
    attr->atime=statbuf->st_atime;
    attr->mtime=statbuf->st_mtime;
    attr->flags=SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_UIDGID
            | SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACMODTIME;
    return attr;
}

int handle_stat(sftp_client_message msg,int follow){
    struct stat statbuf;
    sftp_attributes attr;
    int ret = 0;
//    if (fnmatch("/var/lib/razorback/blocks*", msg->filename, 0) == 0)
//    {
        if(follow)
            ret=stat(msg->filename,&statbuf);
        else 
            ret=lstat(msg->filename,&statbuf);
        if(ret<0){
            reply_status(msg);
            return 0;
        }
#if 0
    }
    else
    {
        errno = EACCES;
        reply_status(msg);
        return 0;
    }
#endif
    attr=attr_from_stat(&statbuf);
    sftp_reply_attr(msg, attr);
    sftp_attributes_free(attr);
    return 0;
}

char *long_name(char *file, struct stat *statbuf){
    static char buf[256];
    char buf2[100];
    int mode=statbuf->st_mode;
    char *time,*ptr;
    strcpy(buf,"");
    switch(mode & S_IFMT){
        case S_IFDIR:
            strcat(buf,"d");
            break;
        default:
            strcat(buf,"-");
            break;
    }
    /* user */
    if(mode & 0400)
        strcat(buf,"r");
    else
        strcat(buf,"-");
    if(mode & 0200)
        strcat(buf,"w");
    else
        strcat(buf,"-");
    if(mode & 0100){
        if(mode & S_ISUID)
            strcat(buf,"s");
        else
            strcat(buf,"x");
    } else
        strcat(buf,"-");
        /*group*/
        if(mode & 040)
            strcat(buf,"r");
        else
            strcat(buf,"-");
        if(mode & 020)
            strcat(buf,"w");
        else
            strcat(buf,"-");
        if(mode & 010)
            strcat(buf,"x");
        else
            strcat(buf,"-");
        /* other */
        if(mode & 04)
            strcat(buf,"r");
        else
            strcat(buf,"-");
        if(mode & 02)
            strcat(buf,"w");
        else
            strcat(buf,"-");
        if(mode & 01)
            strcat(buf,"x");
        else
            strcat(buf,"-");
        strcat(buf," ");
        snprintf(buf2,sizeof(buf2),"%3d %d %d %d",(int)statbuf->st_nlink,
                 (int)statbuf->st_uid,(int)statbuf->st_gid,(int)statbuf->st_size);
        strcat(buf,buf2);
        time=ctime(&statbuf->st_mtime)+4;
        if((ptr=strchr(time,'\n')))
            *ptr=0;
        snprintf(buf2,sizeof(buf2)," %s %s",time,file);
    // +4 to remove the "WED "
        strcat(buf,buf2);
        return buf;
}

int handle_readdir(sftp_client_message msg){
    struct sftp_handle *handle=sftp_handle(msg->sftp,msg->handle);
    sftp_attributes attr;
    struct dirent *dir;
    char *longname;
    struct stat statbuf;
    char file[1024];
    int i;
    if(!handle || handle->type!=TYPE_DIR){
        sftp_reply_status(msg,SSH_FX_BAD_MESSAGE,"invalid handle");
        return 0;
    }
    for(i=0; !handle->eof && i<50;++i){
        dir=readdir(handle->dir);
        if(!dir){
            handle->eof=1;
            break;
        }
        snprintf(file,sizeof(file),"%s/%s",handle->name,
                 dir->d_name);
        if(lstat(file,&statbuf)){
            memset(&statbuf,0,sizeof(statbuf));
        }
        attr=attr_from_stat(&statbuf);
        longname=long_name(dir->d_name,&statbuf);
        sftp_reply_names_add(msg,dir->d_name,longname,attr);
        sftp_attributes_free(attr);
    }
    /* if there was at least one file, don't send the eof yet */
    if(i==0 && handle->eof){
        sftp_reply_status(msg,SSH_FX_EOF,NULL);
        return 0;
    }
    sftp_reply_names(msg);
    return 0;
}

int handle_read(sftp_client_message msg){
    struct sftp_handle *handle=sftp_handle(msg->sftp,msg->handle);
    uint32_t len=msg->len;
    void *data;
    uint32_t r;
    if(!handle || handle->type!=TYPE_FILE){
        sftp_reply_status(msg,SSH_FX_BAD_MESSAGE,"invalid handle");
        return 0;
    }
    if(len>(2<<14)){
        /* 32000 */
        len=2<<14;
    }
    data=malloc(len);
    if (fseeko(handle->file,msg->offset,SEEK_SET) != 0)
        rzb_log(LOG_ERR, "Failed to seek");
    r=fread(data,1,len,handle->file);
#ifdef SFTP_DEBUG
    rzb_log(LOG_DEBUG,"read %d bytes",r);
#endif //SFTP_DEBUG
    if(r<=0 && (len>0)){
        if(feof(handle->file)){
            sftp_reply_status(msg,SSH_FX_EOF,"End of file");
            rzb_log(LOG_ERR, "EOF");
        } else {
            rzb_log(LOG_ERR, "Not EOF");
            reply_status(msg);
        }
        return 0;
    }
    if (sftp_reply_data(msg,data,r) != 0)
        rzb_log(LOG_ERR, "Data Error: %ju", r);
    free(data);
    return 0;
}

int handle_write(sftp_client_message msg){
    struct sftp_handle *handle=sftp_handle(msg->sftp,msg->handle);
    uint32_t len=string_len(msg->data);
    int r;
    if(!handle || handle->type!=TYPE_FILE){
        sftp_reply_status(msg,SSH_FX_BAD_MESSAGE,"invalid handle");
        return 0;
    }
    fseeko(handle->file,msg->offset,SEEK_SET);
    do {
        r=fwrite(string_data(msg->data),1,len,handle->file);
#ifdef SFTP_DEBUG
        rzb_log(LOG_DEBUG,"wrote %d bytes",r);
#endif //SFTP_DEBUG
        if(r <= 0 && (msg->data->size > 0)){
            reply_status(msg);
            return 0;
        }
        len-=r;
        fflush(handle->file);
    } while (len>0);
    sftp_reply_status(msg,SSH_FX_OK,"");
    return 0;
}

int handle_close(sftp_client_message msg){
    struct sftp_handle *handle=sftp_handle(msg->sftp,msg->handle);
    if(!handle){
        sftp_reply_status(msg,SSH_FX_BAD_MESSAGE,"invalid handle");
        return 0;
    }
    sftp_handle_remove(msg->sftp,handle);
    if(handle->type==TYPE_DIR){
        closedir(handle->dir);
    } else {
        fsync(fileno(handle->file));
        fclose(handle->file);
    }
    if(handle->name)
        free(handle->name);
    free(handle);
    sftp_reply_status(msg,SSH_FX_OK,NULL);
    return 0;
}

int handle_open(sftp_client_message msg){
    FILE *file;
    const char *mode="r";
    struct sftp_handle *hdl;
    ssh_string handle;
    int flags =0;
    int fd =0;
    if(msg->flags & SSH_FXF_READ)
        flags |= O_RDONLY;
    if(msg->flags & SSH_FXF_WRITE)
        flags |= O_WRONLY;
    if(msg->flags & SSH_FXF_APPEND)
        flags |= O_APPEND;
    if(msg->flags & SSH_FXF_TRUNC)
        flags |= O_TRUNC;
    if(msg->flags & SSH_FXF_EXCL)
        flags |= O_EXCL;
    if(msg->flags & SSH_FXF_CREAT)
        flags |= O_CREAT;
    fd=open(msg->filename,flags,msg->attr->permissions);
    if(fd<0){
        reply_status(msg);
        return 0;
    }
    close(fd);
    switch(flags& (O_RDONLY | O_WRONLY | O_APPEND | O_TRUNC)){
        case O_RDONLY:
            mode="r";
            break;
        case (O_WRONLY|O_RDONLY):
            mode="r+";
            break;
        case (O_WRONLY|O_TRUNC):
            mode="w";
            break;
        case (O_WRONLY | O_RDONLY | O_APPEND):
            mode="a+";
            break;
        default:
            switch(flags & (O_RDONLY | O_WRONLY)){
                case O_RDONLY:
                    mode="r";
                    break;
                case O_WRONLY:
                    mode="w";
                    break;
            }
    }
    file=fopen(msg->filename,mode);
    hdl=malloc(sizeof(struct sftp_handle));
    memset(hdl,0,sizeof(struct sftp_handle));
    hdl->type=TYPE_FILE;
    hdl->offset=0;
    hdl->file=file;
    hdl->name=strdup(msg->filename);
    handle=sftp_handle_alloc(msg->sftp,hdl);
    sftp_reply_handle(msg,handle);
    free(handle);
    return 0;
}
void
Transfer_SSH_Server_Session (struct Thread *p_pThread)
{
    struct SFTP_Session *session = p_pThread->pUserData;
    char *a = NULL;
    sftp_session sftp;
    sftp = sftp_server_new(session->ssh, session->chan);
    if (sftp_server_init(sftp) != 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to init session: %s", __func__, ssh_get_error(session->ssh));
        sftp_free(sftp);
        ssh_channel_close(session->chan);
        ssh_channel_free(session->chan);
        ssh_disconnect(session->ssh);
        ssh_free(session->ssh);
        free(session);
        return;
    }
    sftp_client_message msg;
    char buffer[PATH_MAX];
    do {
        msg=sftp_get_client_message(sftp);
        if(!msg)
        {
            rzb_log(LOG_ERR, "%s: looks like the client quit: %s", __func__, ssh_get_error(session->ssh));
            break;
        }
        switch(msg->type)
        {
        case SFTP_REALPATH:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client realpath : %s",msg->filename);
#endif //SFTP_DEBUG


                if (strcmp(msg->filename, ".") == 0)
                    strcpy(buffer,"/var/lib/razorback/blocks");
                else
                {
                    a = realpath(msg->filename,buffer);
                    if (a == NULL)
                    {}
                    
                }
#if 0
                if (fnmatch("/var/lib/razorback/blocks", buffer, 0) != 0)
                {
                    errno = EACCES;
                    reply_status(msg);
                }
                else 
                {
#endif
#ifdef SFTP_DEBUG
                    rzb_log(LOG_DEBUG,"responding %s",buffer);
#endif // SFTP_DEBUG
                    sftp_reply_name(msg, buffer, NULL);
//                }
                break;
            case SFTP_OPENDIR:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client opendir(%s)",msg->filename);
#endif // SFTP_DEBUG
                handle_opendir(msg);
                break;
            case SFTP_LSTAT:
            case SFTP_STAT:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client stat(%s)",msg->filename);
#endif // SFTP_DEBUG
                handle_stat(msg,msg->type==SFTP_STAT);
                break;
            case SFTP_READDIR:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client readdir");
#endif // SFTP_DEBUG
                handle_readdir(msg);
                break;
            case SFTP_CLOSE:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client close");
#endif // SFTP_DEBUG
                handle_close(msg);
                break;
            case SFTP_OPEN:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client open(%s)",msg->filename);
#endif // SFTP_DEBUG
                handle_open(msg);
                break;
            case SFTP_READ:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client read(off=%lld,len=%d)",msg->offset,msg->len);
#endif // SFTP_DEBUG
                handle_read(msg);
                break;
            case SFTP_WRITE:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG,"client write(off=%lld len=%d)",msg->offset,string_len(msg->data));
#endif // SFTP_DEBUG
                handle_write(msg);
                break;
            case SFTP_SETSTAT:
            case SFTP_FSETSTAT:
            case SFTP_REMOVE:
            case SFTP_MKDIR:
#ifdef SFTP_DEBUG
                rzb_log(LOG_DEBUG, "client mkdir(%s)", msg->filename);
#endif // SFTP_DEBUG
                if (mkdir(msg->filename, msg->attr->permissions) != 0)
                    reply_status(msg);
                else
                    sftp_reply_status(msg,SSH_FX_OK,"");
                break;
            case SFTP_RMDIR:
            case SFTP_FSTAT:
            case SFTP_RENAME:
            case SFTP_READLINK:
            case SFTP_SYMLINK:
            default:
                rzb_log(LOG_DEBUG,"Unknown message %d",msg->type);
                sftp_reply_status(msg,SSH_FX_OP_UNSUPPORTED,"Unsupported message");
        }
        sftp_client_message_free(msg);
    } while (!Thread_IsStopped(p_pThread));

    sftp_free(sftp);
    ssh_channel_close(session->chan);
    ssh_channel_free(session->chan);
    ssh_disconnect(session->ssh);
    ssh_free(session->ssh);
    if (session->nugget != NULL)
        Nugget_Destroy(session->nugget);
    free(session);

    rzb_log(LOG_ERR, "%s: Thread exiting", __func__);
    return;
}    
void
Transfer_SSH_Server (struct Thread *p_pThread)
{
    ssh_bind sshbind;
    ssh_message message;
    int auth=0;
    int shell=0;
    int r;
    int port = transferBindPort;
    struct Thread *thread;

    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to intialiaze database thread info", __func__);
        return;
    }
    if ((dbCon = Database_Connect
        (g_sDatabaseHost, g_iDatabasePort, g_sDatabaseUser,
         g_sDatabasePassword, g_sDatabaseSchema)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Database_Connect", __func__);
        return;
    }

    
    ssh_init();
    ssh_threads_set_callbacks(ssh_threads_get_pthread());

    sshbind=ssh_bind_new();

    if (ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &port) != SSH_OK)
    {
        rzb_log(LOG_ERR, "%s: Failed to set bind port", __func__);
    }   
#if 0
    if (ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY,
                                            KEYS_FOLDER "ssh_host_v1_key") != SSH_OK)
    {
        rzb_log(LOG_ERR, "%s: Failed to set host v1 key: %s", __func__, ssh_get_error(sshbind));
    }
#endif
    if (ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY,
                                            KEYS_FOLDER "ssh_host_dsa_key") != SSH_OK)
    {
        rzb_log(LOG_ERR, "%s: Failed to set host dsa key", __func__);
    }

    if (ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY,
                                            KEYS_FOLDER "ssh_host_rsa_key") != SSH_OK)
    {
        rzb_log(LOG_ERR, "%s: Failed to set host rsa key", __func__);
    }
    rzb_log(LOG_INFO, "%s: Started ssh console server on port %d", __func__, port);

    if(ssh_bind_listen(sshbind)<0){
        rzb_log(LOG_ERR, "Error listening to socket: %s", __func__,  ssh_get_error(sshbind));
        return;
    }

    while(!Thread_IsStopped(p_pThread))
    {   
        struct SFTP_Session *ses;
        if ((ses = calloc(1,sizeof(struct SFTP_Session))) == NULL)
        {
            continue;
        }
        shell=0;
        ses->ssh=ssh_new();
        r = ssh_bind_accept(sshbind, ses->ssh);
        if(r==SSH_ERROR){
            rzb_log(LOG_ERR, "%s: Error accepting a connection: %s", __func__, ssh_get_error(sshbind));
            ssh_disconnect(ses->ssh);
            ssh_free(ses->ssh);
            free(ses);
            continue;
        }
        if (ssh_handle_key_exchange(ses->ssh)) {
            rzb_log(LOG_ERR, "%s: ssh_handle_key_exchange: %s", __func__, ssh_get_error(ses->ssh));
            ssh_disconnect(ses->ssh);
            ssh_free(ses->ssh);
            free(ses);
            continue;
        }

        /* proceed to authentication */
        auth = authenticate(ses);
        if(!auth){
            rzb_log(LOG_ERR, "%s: Authentication error: %s", __func__, ssh_get_error(ses->ssh));
            ssh_disconnect(ses->ssh);
            ssh_free(ses->ssh);
            free(ses);
            continue;
        }


        /* wait for a channel session */
        do {
            message = ssh_message_get(ses->ssh);
            if(message){
                if(ssh_message_type(message) == SSH_REQUEST_CHANNEL_OPEN &&
                        ssh_message_subtype(message) == SSH_CHANNEL_SESSION) {
                    ses->chan = ssh_message_channel_request_open_reply_accept(message);
                    ssh_message_free(message);
                    break;
                } else {
                    rzb_log(LOG_ERR, "%s: Type: %d Sub: %s", __func__, ssh_message_type(message), ssh_message_subtype(message));
                    ssh_message_reply_default(message);
                    ssh_message_free(message);
                }
            } else {
                break;
            }
        } while(!ses->chan);

        if(!ses->chan) {
            rzb_log(LOG_ERR, "%s: client did not ask for a channel session (%s)", __func__,
                                                        ssh_get_error(ses->ssh));
            ssh_disconnect(ses->ssh);
            ssh_free(ses->ssh);
            if (ses->nugget != NULL)
                Nugget_Destroy(ses->nugget);
            free(ses);
            continue;
        }

        /* wait for a shell */
        do {
            message = ssh_message_get(ses->ssh);
            if(message != NULL) {
                if(ssh_message_type(message) == SSH_REQUEST_CHANNEL) {
                    rzb_log(LOG_ERR, "%s: Type: %u", __func__, ssh_message_subtype(message));

                    if(ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_SUBSYSTEM) {
                        rzb_log(LOG_DEBUG, "%s: Subsystem: %s", __func__, message->channel_request.subsystem);
                        if (strcmp(message->channel_request.subsystem, "sftp") == 0)
                            shell =1;
                        ssh_message_channel_request_reply_success(message);
                        ssh_message_free(message);
                        break;

                    }
                }
                ssh_message_reply_default(message);
                ssh_message_free(message);
            } else {
                break;
            }
        } while(!shell);

        if(!shell) {
            rzb_log(LOG_ERR, "%s: No shell requested (%s)", __func__,  ssh_get_error(ses->ssh));
            ssh_channel_close(ses->chan);
            ssh_channel_free(ses->chan);
            ssh_disconnect(ses->ssh);
            ssh_free(ses->ssh);
            if (ses->nugget != NULL)
                Nugget_Destroy(ses->nugget);
            free(ses);
            continue;
        }
        if ((thread = Thread_Launch (Transfer_SSH_Server_Session, ses, (char *) "Transfer SSH Server Session", &sg_rbContext)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to launch session thread", __func__);
            ssh_channel_close(ses->chan);
            ssh_channel_free(ses->chan);
            ssh_disconnect(ses->ssh);
            ssh_free(ses->ssh);
            if (ses->nugget != NULL)
                Nugget_Destroy(ses->nugget);
            free(ses);
            continue;
        }
        Thread_Destroy(thread);
        thread = NULL;
    }
    ssh_bind_free(sshbind);
    ssh_finalize();
}
 
static bool Transfer_Server_SSH_Start(void)
{
    Thread_Launch (Transfer_SSH_Server, NULL, (char *) "Transfer SSH Server", &sg_rbContext);
    return true;
}


