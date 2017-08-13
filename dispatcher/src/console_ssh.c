#include "config.h"
#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <razorback/thread.h>
#include <razorback/log.h>

#include "console.h"
#include "dispatcher.h"
#include "configuration.h"

#define KEYS_FOLDER ETC_DIR "/"
static int port;

struct SshConsoleSession
{
    ssh_channel ssh_channel;
    struct ssh_channel_callbacks_struct *chan_callbacks;
    struct Thread* ioThread;
    struct Thread* cliThread;
};

static int auth_password(const char *user, const char *password){
    if (strcasecmp (user, g_sConsoleUser) != 0)
        return 0;
    if (strcasecmp (password, g_sConsolePassword) != 0)
        return 0;
    return 1; // authenticated
}

static int authenticate(ssh_session session) {
    ssh_message message;

    do {
        message=ssh_message_get(session);
        if(!message)
            break;
        switch(ssh_message_type(message)){
            case SSH_REQUEST_AUTH:
                switch(ssh_message_subtype(message)){
                    case SSH_AUTH_METHOD_PASSWORD:
                        if(auth_password(ssh_message_auth_user(message),
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
                break;
        }
        ssh_message_free(message);
    } while (1);
    return 0;
}

static int copy_fd_to_chan(socket_t fd, int revents, void *userdata) {
    ssh_channel chan = (ssh_channel)userdata;
    char buf[2048];
    int sz = 0;

    if(!chan) {
        close(fd);
        return -1;
    }
    if(revents & POLLIN) {
        sz = read(fd, buf, 2048);
        if(sz > 0) {
            ssh_channel_write(chan, buf, sz);
        }
    }
    if(revents & POLLHUP) {
        ssh_channel_close(chan);
        sz = -1;
    }
    return sz;
}

static int copy_chan_to_fd(ssh_session session,
                                           ssh_channel channel,
                                           void *data,
                                           uint32_t len,
                                           int is_stderr,
                                           void *userdata) {
    int fd = *(int*)userdata;
    int sz;
    (void)session;
    (void)channel;
    (void)is_stderr;

    sz = write(fd, data, len);
    return sz;
}

static void chan_close(ssh_session session, ssh_channel channel, void *userdata) {
    int fd = *(int*)userdata;
    (void)session;
    (void)channel;

    close(fd);
}

void
Console_SSH_Quit(struct ConsoleSession *session)
{
    struct SshConsoleSession *sshSession = (struct SshConsoleSession *)session->userdata;
    Thread_Interrupt(sshSession->ioThread);
    Thread_Destroy(sshSession->ioThread);
    free(sshSession);
    free(session);
}

void
Console_SSH_IO_Thread(struct Thread *thread)
{
    struct SshConsoleSession *sshSession  = (struct SshConsoleSession*) thread->pUserData;
    ssh_channel chan = sshSession->ssh_channel;
    ssh_session session = ssh_channel_get_session(chan);
    socket_t fds[2];
    struct ConsoleSession *consoleSession;
    ssh_event event;
    short events;
    rzb_log(LOG_INFO, "%s: SSH IO Thread Launched", __func__);
    if ((consoleSession = calloc(1,sizeof(struct ConsoleSession))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate console session struct", __func__);
        ssh_disconnect(session);
        return;
    }

    if ((consoleSession->socket = calloc(1,sizeof(struct Socket))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate socket struct", __func__);
        ssh_disconnect(session);
        return;
    }
    if ((sshSession->chan_callbacks = calloc(1,sizeof(struct ssh_channel_callbacks_struct))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate callbacks struct", __func__);
        ssh_disconnect(session);
        return;
    }
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    consoleSession->socket->iSocket = fds[1];
    consoleSession->authenticated = true;
    consoleSession->userdata = sshSession;
    consoleSession->quitCallback = Console_SSH_Quit;
    ssh_callbacks_init(sshSession->chan_callbacks);
    sshSession->chan_callbacks->userdata = &fds[0];
    sshSession->chan_callbacks->channel_data_function = copy_chan_to_fd;
    sshSession->chan_callbacks->channel_eof_function = chan_close;
    sshSession->chan_callbacks->channel_close_function = chan_close;
    ssh_set_channel_callbacks(chan, sshSession->chan_callbacks);

    events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;

    event = ssh_event_new();
    if(event == NULL) {
        rzb_log(LOG_ERR, "%s: Couldn't get a event", __func__);
        ssh_disconnect(session);
        return;
    }
    if(ssh_event_add_fd(event, fds[0], events, copy_fd_to_chan, chan) != SSH_OK) {
        rzb_log(LOG_ERR, "%s: Couldn't add an fd to the event", __func__);
        ssh_disconnect(session);
        return;
    }
    if(ssh_event_add_session(event, session) != SSH_OK) {
        rzb_log(LOG_ERR, "%s: Couldn't add the session to the event", __func__);
        ssh_disconnect(session);
        return;
    }

    sshSession->cliThread = Thread_Launch (Console_Thread, consoleSession,(char *) "Console Session", thread->pContext);
    if (sshSession->cliThread == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to launch console thread", __func__);
        ssh_disconnect(session);
        close(fds[0]);
        close(fds[1]);
        free(sshSession);
        free(consoleSession);
        return;
    }
    do {
        ssh_event_dopoll(event, 1000);
    } while((!Thread_IsStopped(thread)) && (!ssh_channel_is_closed(chan)));
    // sshSession is invalid after this point
    sshSession = NULL;
    // consoleSession is invalid after this point
    consoleSession = NULL;

    ssh_event_remove_fd(event, fds[0]);

    ssh_event_remove_session(event, session);

    ssh_event_free(event);
    ssh_disconnect(session);
    close(fds[0]);
    rzb_log(LOG_INFO, "%s: SSH IO Thread Exitted", __func__);
    return;
}


    
void
Console_SSH_Server (struct Thread *p_pThread)
{
    struct SshConsoleSession *sshConsole;
    ssh_session session;
    ssh_bind sshbind;
    ssh_message message;
    ssh_channel chan=0;
    int auth=0;
    int shell=0;
    int r;
    port = g_sshConsole.bindPort;
    
    ssh_init();
    ssh_threads_set_callbacks(ssh_threads_get_pthread());

    sshbind=ssh_bind_new();

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &port);

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY,
                                            KEYS_FOLDER "ssh_host_v1_key");
                                            
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY,
                                            KEYS_FOLDER "ssh_host_dsa_key");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY,
                                            KEYS_FOLDER "ssh_host_rsa_key");
    rzb_log(LOG_INFO, "%s: Started ssh console server on port %d", __func__, port);

    if(ssh_bind_listen(sshbind)<0){
        rzb_log(LOG_ERR, "Error listening to socket: %s", __func__,  ssh_get_error(sshbind));
        return;
    }

    while(!Thread_IsStopped(p_pThread))
    {   
        session=ssh_new();
        r = ssh_bind_accept(sshbind, session);
        if(r==SSH_ERROR){
            rzb_log(LOG_ERR, "%s: Error accepting a connection: %s", __func__, ssh_get_error(sshbind));
            ssh_disconnect(session);
            continue;
        }
        if (ssh_handle_key_exchange(session)) {
            rzb_log(LOG_ERR, "%s: ssh_handle_key_exchange: %s", __func__, ssh_get_error(session));
            ssh_disconnect(session);
            continue;
        }

        /* proceed to authentication */
        auth = authenticate(session);
        if(!auth){
            rzb_log(LOG_ERR, "%s: Authentication error: %s", __func__, ssh_get_error(session));
            ssh_disconnect(session);
            continue;
        }


        /* wait for a channel session */
        do {
            message = ssh_message_get(session);
            if(message){
                if(ssh_message_type(message) == SSH_REQUEST_CHANNEL_OPEN &&
                        ssh_message_subtype(message) == SSH_CHANNEL_SESSION) {
                    chan = ssh_message_channel_request_open_reply_accept(message);
                    ssh_message_free(message);
                    break;
                } else {
                    ssh_message_reply_default(message);
                    ssh_message_free(message);
                }
            } else {
                break;
            }
        } while(!chan);

        if(!chan) {
            rzb_log(LOG_ERR, "%s: client did not ask for a channel session (%s)", __func__,
                                                        ssh_get_error(session));
            ssh_disconnect(session);
            continue;
        }


        /* wait for a shell */
        do {
            message = ssh_message_get(session);
            if(message != NULL) {
                if(ssh_message_type(message) == SSH_REQUEST_CHANNEL) {
                    if(ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_SHELL) {
                        shell = 1;
                        ssh_message_channel_request_reply_success(message);
                        ssh_message_free(message);
                        break;
                    } else if(ssh_message_subtype(message) == SSH_CHANNEL_REQUEST_PTY) {
                        ssh_message_channel_request_reply_success(message);
                        ssh_message_free(message);
                        continue;
                    }
                }
                ssh_message_reply_default(message);
                ssh_message_free(message);
            } else {
                break;
            }
        } while(!shell);

        if(!shell) {
            rzb_log(LOG_ERR, "%s: No shell requested (%s)", __func__,  ssh_get_error(session));
            ssh_disconnect(session);
            continue;
        }
        if ((sshConsole = calloc(1, sizeof(struct SshConsoleSession))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate session", __func__);
            ssh_disconnect(session);
            continue;
        }
        sshConsole->ssh_channel = chan;
        sshConsole->ioThread = Thread_Launch(Console_SSH_IO_Thread, sshConsole, (char *)"SSH IO Thread", p_pThread->pContext);
        if (sshConsole->ioThread == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to launch IO Thread", __func__);
            ssh_disconnect(session);
            free(sshConsole);
            continue;
        }
    }
    ssh_bind_free(sshbind);
    ssh_finalize();
}
 

