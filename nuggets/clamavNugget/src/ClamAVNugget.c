#include "config.h"

#include <razorback/debug.h>
#include <razorback/socket.h>
#include <razorback/api.h>
#include <razorback/config_file.h>
#include <razorback/ntlv.h>
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/visibility.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>
#include <razorback/thread.h>
#include <razorback/timer.h>
#include <signal.h>
#define CLAM_NUGGET_CONFIG "clamav.conf"

#define SESSION_START "nIDSESSION\n"
#define SESSION_END "nEND\n"
#define PING "nPING\n"
#define PONG "PONG"
#define SCAN "nSCAN"
#define OK "OK"
#define ERROR "ERROR"
#define FOUND "FOUND"
UUID_DEFINE(CLAMAV_NUGGET, 0xbe, 0x31, 0x20, 0x18, 0x1b, 0x8e, 0x46, 0x2e, 0x90, 0xa8, 0x98, 0x3a, 0x03, 0x6c, 0x8e, 0x8c);

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(ClamAV_Detection);

SO_PUBLIC DECL_NUGGET_THREAD_INIT(Clam_Thread_Init);
SO_PUBLIC DECL_NUGGET_THREAD_CLEANUP(Clam_Thread_Cleanup);

static uuid_t sg_uuidNuggetId;
static char *sg_clamdHost;
static conf_int_t sg_clamdPort;
static conf_int_t sg_clamdPingInterval = 0;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

struct ClamConnection
{
    struct Socket *socket;
    struct Timer *timer;
    struct Mutex *mutex;
};

static struct RazorbackContext *sg_pContext = NULL;

static struct RazorbackInspectionHooks sg_InspectionHooks = {
    ClamAV_Detection,
    processDeferredList,
    Clam_Thread_Init,
    Clam_Thread_Cleanup
};


RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"Clamd.Host", RZB_CONF_KEY_TYPE_STRING, &sg_clamdHost, NULL },
    {"Clamd.Port", RZB_CONF_KEY_TYPE_INT, &sg_clamdPort, NULL },
    {"Clamd.PingInterval", RZB_CONF_KEY_TYPE_INT, &sg_clamdPingInterval, NULL },
    {"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

static char * 
readResponse(struct ClamConnection *con)
{
    ssize_t read;
    char *buf = NULL;
    char *cmd;
    char *ret;
    read = Socket_Rx_Until(con->socket, (uint8_t**)&buf, (uint8_t)'\n');
    if (read < 1) 
        return NULL;
    buf[read-1] = '\0';
    cmd = buf;
    while (cmd < buf+1022 && *cmd != ':')
        cmd++;

    if (*(cmd+1) != ' ')
    {
    	free(buf);
        return NULL;
    }

    cmd += 2;
    if (asprintf(&ret, "%s", cmd) == -1)
    {
    	free(buf);
        return NULL;
    }
    free(buf);
    return ret;
}

/** Scan a buffer with a ClamAV engine that has been initialized with RZB_start_clamav.
 * @param engine pointer to scan engine.
 * @param buffer pointer to the buffer to scan.
 * @param buffer_size size of the buffer.
 * @param virname Return a pointer to a point, set to the virus name or NULL if no virus found. 
 * @return true on success false on error
 */
static bool
RZB_scan_buffer (struct ClamConnection *con, const char *fileName, char **virname)
{
    bool ret = true;            // Return value for the function RZB_scan_buffer
    char * scanCmd;
    char *tok;
    if (asprintf(&scanCmd, "%s %s\n", SCAN, fileName) == -1 )
        return false;

    Mutex_Lock(con->mutex);
    Socket_Tx(con->socket, strlen(scanCmd), (uint8_t*)scanCmd);
    free(scanCmd);
    scanCmd = readResponse(con);
    Mutex_Unlock(con->mutex);
    tok = strrchr(scanCmd, ' ');
    *tok = '\0';
    tok++;
    if (strcmp(tok, FOUND) == 0)
    {
        tok = strchr(scanCmd, ':');
        tok+=2;

        rzb_log (LOG_INFO, "%s: Virus %s detected in %s", __func__, tok, "tmpfile");
        if (asprintf(virname, "%s", tok) == -1 )
            return false;


    } 
    else if (strcmp(tok, OK) == 0)
    {
        *virname = NULL;
        // If no virus was detected
        //rzb_log (LOG_INFO, "%s: No virus detected.", __func__);
    }
    else
    {
        rzb_log (LOG_ERR, "%s: Error Scanning file: %s", __func__, fileName);
        ret = false;
    }
    free(scanCmd);

    // Delete the file from tmpfs
    return ret;
}

void 
pingPong(void *vCon)
{
    struct ClamConnection *con = vCon;
    char *ret;
    Mutex_Lock(con->mutex);
    Socket_Tx(con->socket, strlen(PING), (uint8_t*)PING);
    ret = readResponse(con);
    if ( (ret == NULL) || (strncmp(ret, PONG, strlen(PONG)) != 0))
        rzb_log(LOG_ERR, "ClamAV Nugget: Failed to ping server, read: |%s|", ret);
    Mutex_Unlock(con->mutex);
    free(ret);
}

SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_pClamUuid;
    uuid_t l_pList[1];
    UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_pList[0]);
    uuid_copy(l_pClamUuid, CLAMAV_NUGGET);

    if (!readMyConfig(NULL, CLAM_NUGGET_CONFIG, sg_Config))
    {
        rzb_log(LOG_ERR,"%s: ClamAV Nugget: Failed to read config file", __func__);
        return false;
    }
    sg_pContext = Razorback_Init_Inspection_Context (
            sg_uuidNuggetId, l_pClamUuid, 1, l_pList,
            &sg_InspectionHooks, 
            sg_initThreads, sg_maxThreads);
    return true;
}

SO_PUBLIC DECL_NUGGET_THREAD_INIT(Clam_Thread_Init)
{
    struct ClamConnection *con;
    if ((con =  calloc(1,sizeof(struct ClamConnection))) == NULL)
    {
        rzb_log(LOG_ERR, "Clam(%s): Failed to allocate connection", __func__);
        return false;
    }
    if((con->mutex = Mutex_Create(MUTEX_MODE_NORMAL)) == NULL)
    {
        rzb_log(LOG_ERR, "Clam(%s): Failed to create socket mutex", __func__);
        free(con);
        return false;
    }
    if ((con->socket = Socket_Connect((uint8_t *)sg_clamdHost, sg_clamdPort)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: ClamAV Nugget: Failed to connect to clamd", __func__);
        Mutex_Destroy(con->mutex);
        free(con);
        return false;
    }

    Socket_Tx(con->socket, strlen(SESSION_START), (uint8_t*)SESSION_START);
    if ((con->timer = Timer_Create(1, pingPong, con)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: ClamAV Nugget: Failed to create timer", __func__);
        Mutex_Destroy(con->mutex);
        Socket_Close(con->socket);
        free(con);
        return false;
    }
    *threadData = con;
    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    if (sg_pContext != NULL)
    {
        Razorback_Shutdown_Context(sg_pContext);
    }
}

SO_PUBLIC DECL_NUGGET_THREAD_CLEANUP(Clam_Thread_Cleanup)
{
    struct ClamConnection *con = threadData;
    Timer_Destroy(con->timer);
    Mutex_Lock(con->mutex);
    Socket_Tx(con->socket, strlen(SESSION_END), (uint8_t*)SESSION_END);
    Socket_Close(con->socket);
    Mutex_Unlock(con->mutex);
    Mutex_Destroy(con->mutex);
    free(con);
}

DECL_INSPECTION_FUNC(ClamAV_Detection)
{
    struct ClamConnection *con = threadData;
    char *virname = NULL;
    struct Judgment *judgment;
    if (!RZB_scan_buffer (con, block->data.fileName, &virname))
    {
        return JUDGMENT_REASON_ERROR;
    }

    if (virname != NULL)
    {
        if ((judgment = Judgment_Create(eventId, block->pId)) == NULL)
        {
                rzb_log(LOG_ERR, "%s: Failed to allocate alert metadata", __func__);
                return JUDGMENT_REASON_ERROR;
        }
        if (asprintf((char **)&judgment->sMessage, "ClamAV Found: %s", virname) == -1)
        {
                rzb_log(LOG_ERR, "%s: Failed to allocate alert metadata", __func__);
                return JUDGMENT_REASON_ERROR;
        }
        Metadata_Add_MalwareName(judgment->pMetaDataList, "ClamAV", virname);
        judgment->iPriority = 1;
        judgment->Set_SfFlags=SF_FLAG_BAD;
        Razorback_Render_Verdict(judgment);
        Judgment_Destroy(judgment);
        free(virname);
    }
    return JUDGMENT_REASON_DONE;
}

bool 
processDeferredList (struct DeferredList *p_pDefferedList)
{
    return true;
}


