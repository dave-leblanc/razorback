/** @file dispatcher.c
 * the dispatcher
 */

#include <razorback/api.h>
#include <razorback/block_id.h>
#include <razorback/queue.h>
#include <razorback/connected_entity.h>
#include <razorback/thread_pool.h>
#include <razorback/messages.h>
#include <razorback/nugget.h>
#include <razorback/string_list.h>
#include <razorback/log.h>

#include "database.h"
#include "datastore.h"
#include "global_cache.h"
#include "routing_list.h"
#include "judgment_processor.h"
#include "submission_processor.h"
#include "request_processor.h"
#include "log_processor.h"
#include "dispatcher.h"
#include "statistics.h"
#include "flag_copier.h"
#include "console.h"
#include "configuration.h"
#include "dbus/processor.h"
#include "transfer/core.h"
#include "requeue.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>


//XXX: HUGE UGLY HIDDEN DISPATCHER ONLY SYMBOL
//
extern bool CommandAndControl_ArmHelloTimer (void);


static pthread_mutex_t sg_pmTerminationLock;
static bool sg_bTerminate;
uuid_t g_uuidDispatcherId;
struct Queue *g_ccWrite = NULL;

struct ThreadPool *g_cacheThreadPool = NULL;
struct ThreadPool *g_judgmentThreadPool = NULL;
struct ThreadPool *g_logThreadPool = NULL;
struct ThreadPool *g_submissionThreadPool = NULL;


static struct Thread *sg_pFlagThread = NULL;
static struct Thread *sg_pRequeueThread = NULL;

static struct DatabaseConnection *sg_pCnC_Db_Connection = NULL;

// GLobal statistics structure
struct StatTree *g_stats;

static void entityRemoved(struct ConnectedEntity *entity, uint32_t remainingCount);

static bool ProcessRegReqMessage (struct Message *message);
static bool ProcessRegRespMessage (struct Message *message);
static bool ProcessRegErrMessage (struct Message *message);
static bool ProcessConfUpdateMessage (struct Message *message);
static bool ProcessConfAckMessage (struct Message *message);
static bool ProcessConfErrMessage (struct Message *message);
static bool ProcessPauseMessage (struct Message *message);
static bool ProcessPausedMessage (struct Message *message);
static bool ProcessGoMessage (struct Message *message);
static bool ProcessRunningMessage (struct Message *message);
static bool ProcessTermMessage (struct Message *message);
static bool ProcessByeMessage (struct Message *message);
static bool ProcessHelloMessage (struct Message *message);
static bool ProcessReRegMessage (struct Message *message);

static struct RazorbackCommandAndControlHooks sg_rcchCandCHooks = {
    ProcessRegReqMessage,
    ProcessRegRespMessage,
    ProcessRegErrMessage,
    ProcessConfUpdateMessage,
    ProcessConfAckMessage,
    ProcessConfErrMessage,
    ProcessPauseMessage,
    ProcessPausedMessage,
    ProcessGoMessage,
    ProcessRunningMessage,
    ProcessTermMessage,
    ProcessByeMessage,
    ProcessHelloMessage,
    ProcessReRegMessage
};

struct RazorbackContext sg_rbContext;
pthread_mutex_t contextLock;

static const struct option sg_options[] =
{
    {"debug",no_argument, NULL, 'd'},
    {NULL, 0, NULL, 0}
};


bool
Dispatcher_Terminate (void)
{
    bool l_bTerminate;

    pthread_mutex_lock (&sg_pmTerminationLock);
    l_bTerminate = sg_bTerminate;
    pthread_mutex_unlock (&sg_pmTerminationLock);
    return l_bTerminate;
}

// add getting the data block if dirty
int
main (int argc, char ** argv)
{
    int optId; /* holds the option index for getopt*/
    int option_index = 0;
    int debug = 0;
    memset(&g_telnetConsole, 0, sizeof(struct ConsoleServiceConf));
    memset(&g_sshConsole, 0, sizeof(struct ConsoleServiceConf));

    while (1)
    {
        optId = getopt_long(argc, argv, "d", sg_options, &option_index);

        if (optId == -1)
            break;

        switch (optId)
        {
            case 'd':
                debug = 1;
                break;
            case '?':  /* getopt errors */
                // TODO: Print errors
                break;
            default:
                // TODO: Print Usage
                break;
        }
    }
    // 
    // make a point of noticing we have started
    rzb_log (LOG_DEBUG, "%s: Dispatcher Starting", __func__);


    if (!Config_Load())
    {
        rzb_log(LOG_ERR, "%s: Failed to load dispatcher config", __func__);
        return 1;
    }

    if (!debug) {
        if (!rzb_daemonize(NULL, g_sPidFile)) {
            rzb_log(LOG_ERR, "%s: Failed to daemonize", __func__);
            return 1;
        }
    } else {
        rzb_debug_logging();
    }


    // for errors
    uint8_t l_sDispatcherName[37];

    // setup termination globals
    sg_bTerminate = false;
    pthread_mutex_init (&sg_pmTerminationLock, NULL);
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&contextLock, &mutexAttr);

    // Initialize the database library
    //
    if (!Database_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to initialize database library", __func__);
        return 1;
    }
    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to initialize database thread info", __func__);
        return 1;
    }

    // Initialize Memcached
	if (!Global_Cache_Initialize()) 
	{
		rzb_log(LOG_ERR, "%s: Failed to initialize Memcached", __func__);
		return 1;
	}

    // Connect the Command and Control Database Connection
    if ((sg_pCnC_Db_Connection = Database_Connect(
                g_sDatabaseHost, g_iDatabasePort,
                g_sDatabaseUser, g_sDatabasePassword,
                g_sDatabaseSchema)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to connect CnC DB Connection", __func__);
        return 1;
    }
    Datastore_Load_UUID_List(sg_pCnC_Db_Connection, UUID_TYPE_NUGGET);
    Datastore_Load_UUID_List(sg_pCnC_Db_Connection, UUID_TYPE_NTLV_TYPE);
    Datastore_Load_UUID_List(sg_pCnC_Db_Connection, UUID_TYPE_NTLV_NAME);
    Datastore_Load_UUID_List(sg_pCnC_Db_Connection, UUID_TYPE_DATA_TYPE);

    if ((g_ccWrite = Queue_Create (COMMAND_QUEUE, QUEUE_FLAG_SEND, Razorback_Get_Message_Mode())) == NULL)
    {
        rzb_log (LOG_ERR, "%s: C&C Error: Failed to connect to MQ.", __func__);
        return 1;
    }
    Console_Initialize(&sg_rbContext);
    Transfer_Server_Init();

    // Initialize stats
	if(Statistics_Initialize() == false){
		rzb_log(LOG_ERR, "%s: Failed to initialize stats structure", __func__);
		return 1;
	}
    RoutingList_Initialize ();

    // initialize dispatcher name
    uuid_t dispatcherNugget;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcherNugget);
    uuid_unparse (dispatcherNugget, (char *) l_sDispatcherName);
    uuid_unparse (g_uuidDispatcherId, (char *) l_sDispatcherName);
    sg_rbContext.iFlags = CONTEXT_FLAG_STAND_ALONE;
    uuid_copy (sg_rbContext.uuidNuggetId, g_uuidDispatcherId);
    uuid_copy (sg_rbContext.uuidNuggetType, dispatcherNugget);
    uuid_clear (sg_rbContext.uuidApplicationType);
    sg_rbContext.inspector.hooks = NULL;
    sg_rbContext.pCommandHooks = &sg_rcchCandCHooks;
    sg_rbContext.dispatcher.priority = g_dispatcherPriority;
    sg_rbContext.dispatcher.port = transferBindPort;
    sg_rbContext.dispatcher.protocol = transferProtocol;
    sg_rbContext.dispatcher.addressList = StringList_Create();
    for (conf_int_t i = 0; i < transferHostnamesCount; i++)
        StringList_Add(sg_rbContext.dispatcher.addressList, transferHostnames[i]);

    
    if (!Razorback_Init_Context (&sg_rbContext))
    {
    	rzb_log(LOG_ERR, "%s: Failed to initialize context", __func__);
    	exit(1);
    }
    g_cacheThreadPool = ThreadPool_Create(0, g_cacheThreadsMax, &sg_rbContext, "Cache Request Thread (%d)", RequestProcessor_Thread);
    g_judgmentThreadPool = ThreadPool_Create(0, g_judgmentThreadsMax, &sg_rbContext, "Judgment Thread (%d)", JudgmentProcessor_Thread);
    g_logThreadPool = ThreadPool_Create(0, g_logThreadsMax, &sg_rbContext, "Log Thread (%d)", LogProcessor_Thread);
    g_submissionThreadPool = ThreadPool_Create(0, g_submissionThreadsMax, &sg_rbContext, "Submission Thread (%d)", SubmissionProcessor_Thread);
    DBus_Init();
    Transfer_Server_Start();
    
 
    // Wait for other dispatchers to show up.
    sleep(10);
    pthread_mutex_lock(&contextLock);
    sg_rbContext.regOk = true;
    if ( (sg_rbContext.dispatcher.flags &
                (DISP_HELLO_FLAG_LM | DISP_HELLO_FLAG_LS)) == 
            (DISP_HELLO_FLAG_LM | DISP_HELLO_FLAG_LS))
    {
        rzb_log(LOG_ERR, "Dispatcher exiting, no room in this locality, master and slave are already online");
        exit(1);
    }
    if (sg_rbContext.dispatcher.flags == 0)
        sg_rbContext.dispatcher.flags = DISP_HELLO_FLAG_LM;

    sg_rbContext.dispatcher.flags ^= DISP_HELLO_FLAG_DD;
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) != 0)
        Datastore_Reset_Nugget_States(sg_pCnC_Db_Connection);
    pthread_mutex_unlock(&contextLock);

    CommandAndControl_ArmHelloTimer ();
    


    // Add callback when a node goes offline
    ConnectedEntityList_AddPruneListener(entityRemoved);

    // Update State
    updateDispatcherState();
    

    // begin command loop
    while (!Dispatcher_Terminate ())
    {
        sleep (10); 
    }


    // make a point of noticing we have ended
    rzb_log (LOG_INFO, "%s: Dispatcher Terminated", __func__);

    // done
    return 0;
}

static bool
ProcessRegReqMessage (struct Message *msg)
{
    struct MessageRegistrationRequest *regReq = msg->message;
    struct Message *message;
    struct Nugget *l_pNugget;
    uuid_t source, dest;
    Message_Get_Nuggets(msg,source,dest);
    if (Datastore_Get_NuggetByUUID(sg_pCnC_Db_Connection, 
            source, &l_pNugget) != R_FOUND)
    {
        // some sort of error
        rzb_log (LOG_ERR,
                 "%s: nugget registration failed: nugget not found", __func__);
        if ((message = MessageError_Initialize(MESSAGE_TYPE_REG_ERR,
                                            "Invalid Registration",
                                            g_uuidDispatcherId, source)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create error message", __func__);
            return false;
        }
                

        if (!Queue_Put (g_ccWrite,message))
        {
            rzb_log (LOG_ERR,
                     "%s: dropped command response due to failure of CommandQueue_Put ( REG RESP )", __func__);
            message->destroy(message);
            return false;
        }
        message->destroy(message);
        return true;
    }
    if ( (uuid_compare(regReq->uuidNuggetType, l_pNugget->uuidNuggetType) != 0) ||
            (uuid_compare(regReq->uuidApplicationType, l_pNugget->uuidApplicationType) != 0) )
    {
        rzb_log(LOG_INFO, "%s: Invalid Nugget Registered: Name: %s, Location: %s, Contact: %s, Notes: %s", __func__,
            l_pNugget->sName, 
            l_pNugget->sLocation, 
            l_pNugget->sContact ,
            l_pNugget->sNotes);
        rzb_log (LOG_ERR,
                 "%s: nugget registration failed: app/nugget type mismatch", __func__);
        if ((message =MessageError_Initialize(MESSAGE_TYPE_REG_ERR,
                                            "Invalid Registration",
                                            g_uuidDispatcherId, source)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create error message", __func__);
            Nugget_Destroy(l_pNugget);
            return false;
        }
        if (!Queue_Put (g_ccWrite,message))
        {
            message->destroy(message);
            rzb_log (LOG_ERR,
                     "%s: dropped command response due to failure of CommandQueue_Put ( REG RESP )", __func__);
            Nugget_Destroy(l_pNugget);
            return false;
        }
        message->destroy(message);
        Nugget_Destroy(l_pNugget);
        return true;
    }

    rzb_log(LOG_INFO, "%s: Nugget Registered: Name: %s, Location: %s, Contact: %s, Notes: %s", __func__,
            l_pNugget->sName, 
            l_pNugget->sLocation, 
            l_pNugget->sContact ,
            l_pNugget->sNotes);
    // he is authentic, so continue his registration
    // add to list of items to worry about
    // this also sets his state to CONNECTEDENTITY_STATE_REQUESTEDREGISTER
    Nugget_Destroy(l_pNugget);

    if (Datastore_Set_Nugget_State(sg_pCnC_Db_Connection,source, NUGGET_STATE_REQUESTEDREGISTER != R_SUCCESS))
    {
        // some sort of error
        rzb_log (LOG_ERR,
                 "%s: dropped command due to failure of ConnectedEntityList_Add", __func__);
        return false;
    }

    // if he is an inspector, we need to add his info to the routing table
    if (regReq->iDataTypeCount != 0) 
    {
        for (uint32_t i = 0; i < regReq->iDataTypeCount; i++)
        {
            RoutingList_Add(regReq->pDataTypeList[i], regReq->uuidApplicationType);
        }
    }

    // create response
    if ((message = MessageRegistrationResponse_Initialize (
                                            g_uuidDispatcherId, source)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to create registration response", __func__);
        return false;
    }


    // transmit response
    if (!Queue_Put (g_ccWrite, message))
    {
        // error
        rzb_log (LOG_ERR,
                 "%s: dropped command response due to failure of CommandQueue_Put ( REG RESP )", __func__);
        message->destroy(message);
        return false;
    }

    // destroy message
    message->destroy(message);

    // change state
    if (Datastore_Set_Nugget_State(sg_pCnC_Db_Connection, source, NUGGET_STATE_REGISTERING) != R_SUCCESS)
    {
        // error
        rzb_log (LOG_ERR,
                 "%s: Dropped command response due to failure of ConnectedEntityList_ChangeState ( REGISTERING )", __func__);
        return false;
    }
    uuid_t masterNuggetUUID, collectorNuggetUUID;
    UUID_Get_UUID(NUGGET_TYPE_MASTER, UUID_TYPE_NUGGET_TYPE, masterNuggetUUID);
    UUID_Get_UUID(NUGGET_TYPE_COLLECTION, UUID_TYPE_NUGGET_TYPE, collectorNuggetUUID);
    if ( (uuid_compare(regReq->uuidNuggetType, masterNuggetUUID) == 0) ||
            (uuid_compare(regReq->uuidNuggetType, collectorNuggetUUID) == 0) )
    {
        // create configuration update
        if ((message = MessageConfigurationUpdate_Initialize
            ( g_uuidDispatcherId, source )) == NULL)
        {
            // error
            rzb_log (LOG_ERR,
                     "%s: Dropped command response due to failure of MessageConfigurationUpdate_Initialize", __func__);
            return false;
        }

        // transmit configuration
        if (!Queue_Put (g_ccWrite, message))
        {
            // error
            message->destroy(message);
            rzb_log (LOG_ERR,
                     "%s: Dropped command response due to failure of CommandQueue_Put ( CONFIG UPDATE )", __func__);
            return false;
        }
        // destroy message
        message->destroy(message);

        // change state
        if (Datastore_Set_Nugget_State( sg_pCnC_Db_Connection, source, NUGGET_STATE_CONFIGURING) != R_SUCCESS)
        {
            // error
            rzb_log (LOG_ERR,
                     "%s: Dropped command response due to failure of ConnectedEntityList_ChangeState ( CONFIGURING )", __func__);
            return false;
        }
    }
    else
    {
        if (Datastore_Set_Nugget_State( sg_pCnC_Db_Connection, source, NUGGET_STATE_PAUSED) != R_SUCCESS)
        {
            // error
            rzb_log (LOG_ERR,
                     "%s: Dropped command response due to failure of ConnectedEntityList_ChangeState ( PAUSED )", __func__);
            return false;
        }
        // create go message
        if ((message = MessageGo_Initialize (g_uuidDispatcherId, source )) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create go message", __func__);
            return false;
        }
        if (!Queue_Put (g_ccWrite, message))
        {
            // error
            message->destroy(message);
            rzb_log (LOG_ERR,
                     "%s: Dropped command response due to failure of CommandQueue_Put ( CONFIG UPDATE )", __func__);
            return false;
        }
        // destroy go message
        message->destroy(message);
    }
    return true;
}

static bool
ProcessRegRespMessage (struct Message *message)
{
    // we should never receive this message
    rzb_log (LOG_ERR, "%s: Received Registration Response Message", __func__);
    return true;
}

static bool
ProcessRegErrMessage (struct Message *message)
{
    // we should never receive this message 
    rzb_log (LOG_ERR, "%s Received Registration Error Message", __func__);
    return true;
}

static bool
ProcessConfUpdateMessage (struct Message *message)
{
    return true;
}

static bool
ProcessConfAckMessage (struct Message *msg)
{
    uint32_t l_iState;
    struct Message *message;
    uuid_t source, dest;
    Message_Get_Nuggets(msg,source,dest);

    // find the state
    if (Datastore_Get_Nugget_State( sg_pCnC_Db_Connection, source, &l_iState) != R_FOUND)
    {
        rzb_log (LOG_ERR,
                 "%s: nugget config ack: nugget not found", __func__);
        if ( (message = MessageTerminate_Initialize ( g_uuidDispatcherId, source, (const uint8_t *)"You are terminated in response to your Configuration Ack message: invalid nugget" ) ) == NULL)
        {
            rzb_log ( LOG_ERR, "%s Dropped command response due to failure of MessageTerminate_Initialize", __func__);
            return false;
        }
                
        if (!Queue_Put (g_ccWrite,message))
        {
            rzb_log (LOG_ERR,
                     "%s: dropped command response due to failure of CommandQueue_Put ( REG RESP )", __func__);
            message->destroy(message);
            return false;
        }
        message->destroy(message);
        return true;
    }

    // ensure state == CONFIGURING
    if (l_iState != NUGGET_STATE_CONFIGURING)
    {
        rzb_log (LOG_ERR,
                 "%s: Dropped command response due to incorrect state", __func__);
        return false;
    }
    // change state
    if (Datastore_Set_Nugget_State( sg_pCnC_Db_Connection, source, NUGGET_STATE_PAUSED) != R_SUCCESS)
    {
        // error
        rzb_log (LOG_ERR,
                 "%s: Dropped command response due to failure of ConnectedEntityList_ChangeState ( PAUSED )", __func__);
        return false;
    }
    // create go message
    if ((message = MessageGo_Initialize ( g_uuidDispatcherId, source)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to create go message", __func__);
        return false;
    }
    if (!Queue_Put (g_ccWrite, message))
    {
        // error
        message->destroy(message);
        rzb_log (LOG_ERR,
                 "%s: Dropped command response due to failure of CommandQueue_Put ( CONFIG UPDATE )", __func__);
        return false;
    }
    // destroy go message
    message->destroy(message);
    // done
    return true;
}

static bool
ProcessConfErrMessage (struct Message *msg)
{
    struct Message * message;
    uuid_t source, dest;
    Message_Get_Nuggets(msg,source,dest);
#if 0
    // mark as error
    if (!ConnectedEntityList_ChangeState
        (sg_celNuggetList, p_meError->ccmhHeader.uuidSourceNugget,
         CONNECTEDENTITY_STATE_ERROR))
    {
        // error
        rzb_log (LOG_ERR,
                 "Dispatcher dropped command response due to failure of ProcessConfErrMessage::ConnectedEntityList_ChangeState ( ERROR )");
        return false;
    }
#endif
    // create terminate message
    if ( (message = MessageTerminate_Initialize ( g_uuidDispatcherId, source, (const uint8_t *)"You are terminated in response to your Configuration Error message" ) ) == NULL)
    {
        rzb_log ( LOG_ERR, "%s Dropped command response due to failure of MessageTerminate_Initialize", __func__);
        return false;
    }

    // transmit terminate message
    if ( !Queue_Put ( g_ccWrite,  message ) )
    {
        // error
        rzb_log ( LOG_ERR, "%s: Dropped command response due to failure of CommandQueue_Put", __func__);
        message->destroy(message);
        return false;
    }

    // destroy the terminate message
    message->destroy(message);
#if 0
    // mark as terminated
    if (!ConnectedEntityList_ChangeState
        (sg_celNuggetList, l_mtTerminate.ccmhMessageHeader.uuidSourceNugget,
         CONNECTEDENTITY_STATE_TERMINATED))
    {
        // error
        rzb_log (LOG_ERR,
                 "Dispatcher dropped command response due to failure of ProcessConfErrMessage::ConnectedEntityList_ChangeState ( TERMINATED )");
        return false;
    }
#endif
    // done
    return true;
}

static bool
ProcessPauseMessage (struct Message *message)
{
    return true;
}

static bool
ProcessPausedMessage (struct Message *message)
{
    return true;
}

static bool
ProcessGoMessage (struct Message *message)
{
    return true;
}
static bool
ProcessReRegMessage (struct Message *message)
{
    return true;
}

static bool
ProcessRunningMessage (struct Message *msg)
{
    uuid_t source,dest;
    uint32_t l_iState;
    struct Message *message;
    Message_Get_Nuggets(msg,source,dest);

    if (Datastore_Get_Nugget_State( sg_pCnC_Db_Connection, source, &l_iState) != R_FOUND)
    {
        rzb_log (LOG_ERR,
                 "%s: nugget running: nugget not found", __func__);
        if ( (message = MessageTerminate_Initialize ( g_uuidDispatcherId, source, (const uint8_t *)"You are terminated in response to your running message: invalid nugget" ) ) == NULL)
        {
            rzb_log ( LOG_ERR, "%s Dropped command response due to failure of MessageTerminate_Initialize", __func__);
            return false;
        }
                
        if (!Queue_Put (g_ccWrite,message))
        {
            rzb_log (LOG_ERR,
                     "%s: dropped command response due to failure of CommandQueue_Put ( REG RESP )", __func__);
            message->destroy(message);
            return false;
        }
        message->destroy(message);
        return true;
    }
    if (l_iState != NUGGET_STATE_PAUSED)
    {
        rzb_log (LOG_ERR,
                 "%s: Dropped command response due to incorrect state", __func__);
        return false;
    }
    // change state
    if (Datastore_Set_Nugget_State( sg_pCnC_Db_Connection, source, NUGGET_STATE_RUNNING) != R_SUCCESS)
    {
        // error
        rzb_log (LOG_ERR,
                 "%s: Dropped command response due to failure of ConnectedEntityList_ChangeState ( RUNNING )", __func__);
        return false;
    }
    return true;
}

static bool
ProcessTermMessage (struct Message *message)
{
    rzb_log(LOG_ERR, "Terminating at another dispatchers request");
    exit(1);
    return true;
}

static bool
ProcessByeMessage (struct Message *message)
{
    uuid_t source,dest;
    char nugId[UUID_STRING_LENGTH];
    Message_Get_Nuggets(message,source,dest);
    uuid_unparse(source, nugId);
    // TODO: Look up nugget data in the DB and print it
    rzb_log(LOG_INFO, "%s: Nugget Going Offline: %s", __func__, nugId);
    if (Datastore_Set_Nugget_State( sg_pCnC_Db_Connection, source, NUGGET_STATE_TERMINATED) != R_SUCCESS)
    {
        // error
        rzb_log (LOG_ERR,
                 "%s: Failed to set entity state to terminated", __func__);
        return false;
    }
 
    return true;
}

static void
processLocalityHello(struct MessageHello *hello, uuid_t source)
{
    // Multiple promotions/demotions at the same time.
    if ( ((hello->flags & DISP_HELLO_FLAG_LF) != 0) &&
            ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LF)) != 0)
    {
        rzb_log(LOG_ERR, "%s: We are all just trying to get along here, removing promotion flag and letting resolution take its course", __func__);
        sg_rbContext.dispatcher.flags &= ~DISP_HELLO_FLAG_LF;
    }
    // We are promoting/demoting our selves.
    else if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LF) != 0)
    {
        if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0) &&
                ((hello->flags & DISP_HELLO_FLAG_LS) != 0 ))
        {
            rzb_log(LOG_WARNING, "%s: Master promotion complete", __func__);
            sg_rbContext.dispatcher.flags ^= DISP_HELLO_FLAG_LF;
            updateDispatcherState();
        }
        else if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) != 0) &&
                ((hello->flags & DISP_HELLO_FLAG_LM) != 0 ))
        {
            rzb_log(LOG_WARNING, "%s: Master promotion complete", __func__);
            sg_rbContext.dispatcher.flags ^= DISP_HELLO_FLAG_LF;
        }
    }
    // Another node is promoting/demoting its self
    else if ((hello->flags & DISP_HELLO_FLAG_LF) != 0)
    {
        if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0) &&
                ((hello->flags & DISP_HELLO_FLAG_LM) != 0 ))
        {
            rzb_log(LOG_WARNING, "%s: Slave promotion to master, demoting to slave", __func__);
            sg_rbContext.dispatcher.flags &= ~DISP_HELLO_FLAG_LM;
            sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LS;
            updateDispatcherState();
        }
        else if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) != 0) &&
                ((hello->flags & DISP_HELLO_FLAG_LS) != 0 ))
        {
            rzb_log(LOG_WARNING, "%s: Master demotion to slave, promoting to master", __func__);
            sg_rbContext.dispatcher.flags &= ~DISP_HELLO_FLAG_LS;
            sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LM;
            updateDispatcherState();
        }
    }
    // Conflict resolution - two masters
    else if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0) &&
                ((hello->flags & DISP_HELLO_FLAG_LM) != 0 ))
    {
        rzb_log(LOG_ERR, "%s: Locality master conflict resolution intiated", __func__);
        if (sg_rbContext.dispatcher.priority < hello->priority)
        {
            rzb_log(LOG_ERR, "%s: Locality master conflict resolved - becoming slave", __func__);
            sg_rbContext.dispatcher.flags &= (~DISP_HELLO_FLAG_LM);
            sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LS;
            updateDispatcherState();
        }
        else if (sg_rbContext.dispatcher.priority == hello->priority)
        {
            if (uuid_compare(sg_rbContext.uuidNuggetId, source) < 0)
            {
                rzb_log(LOG_ERR, "%s: Locality master conflict resolved - becoming slave", __func__);
                sg_rbContext.dispatcher.flags &= (~DISP_HELLO_FLAG_LM);
                sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LS;
                updateDispatcherState();
            }
            else
                rzb_log(LOG_ERR, "%s: Locality master conflict resolved - remaining master", __func__);
        }
        rzb_log(LOG_ERR, "%s: Locality master conflict resolved - remaining master", __func__);

    }
    // Conflict resolution - two slaves
    else if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) != 0) &&
            ((hello->flags & DISP_HELLO_FLAG_LS) != 0 ))
    {
        rzb_log(LOG_ERR, "%s: Locality slave conflict resolution intiated", __func__);
        if (sg_rbContext.dispatcher.priority > hello->priority)
        {
            rzb_log(LOG_ERR, "%s: Locality slave conflict resolved - becoming master", __func__);
            sg_rbContext.dispatcher.flags &= (~DISP_HELLO_FLAG_LS);
            sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LM;
            updateDispatcherState();
        }
        else if (sg_rbContext.dispatcher.priority == hello->priority)
        {
            if (uuid_compare(sg_rbContext.uuidNuggetId, source) > 0)
            {
                rzb_log(LOG_ERR, "%s: Locality slave conflict resolved - becoming master", __func__);
                sg_rbContext.dispatcher.flags &= (~DISP_HELLO_FLAG_LS);
                sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LM;
                updateDispatcherState();
            }
            else
                rzb_log(LOG_ERR, "%s: Locality slave conflict resolved - remaining slave", __func__);
        }
        else
            rzb_log(LOG_ERR, "%s: Locality slave conflict resolved - remaining slave", __func__);

    }
} 

static bool
ProcessHelloMessage (struct Message *message)
{
    struct MessageHello *hello = message->message;
    //struct RazorbackContext *l_pContext = NULL;
    uint32_t l_iState =0;
    struct Message *term;
    char *l_sNugType, *l_sAppType, l_sNugId[UUID_STRING_LENGTH];
    uuid_t source,dest;
    uuid_t dispatcher;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);


    //l_pContext = Thread_GetContext(Thread_GetCurrent());
    Message_Get_Nuggets(message,source,dest);


    // find the state
    if (Datastore_Get_Nugget_State( sg_pCnC_Db_Connection, source, &l_iState) != R_FOUND)
    {
        // some sort of error
        rzb_log (LOG_ERR,
                 "%s: nugget hello: nugget not found", __func__);
        if ( (term = MessageTerminate_Initialize ( g_uuidDispatcherId, source, (const uint8_t *)"You are terminated in response to your hello message: invalid nugget" ) ) == NULL)
        {
            rzb_log ( LOG_ERR, "%s Dropped command response due to failure of MessageTerminate_Initialize", __func__);
            return false;
        }
                
        if (!Queue_Put (g_ccWrite,term))
        {
            rzb_log (LOG_ERR,
                     "%s: dropped command response due to failure of CommandQueue_Put ( REG RESP )", __func__);
            term->destroy(term);
            return false;
        }
        term->destroy(term);
        return true;
    }

    if (uuid_compare(dispatcher, hello->uuidNuggetType) != 0 ) // Not a dispatcher hello
    {
        // Only process the message and request termination if we are dedicated dispatcher
        if ( ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) != 0) && ( l_iState == NUGGET_STATE_UNKNOWN)  )
        {
            l_sNugType = UUID_Get_NameByUUID(hello->uuidNuggetType, UUID_TYPE_NUGGET_TYPE);
            l_sAppType = UUID_Get_NameByUUID(hello->uuidApplicationType, UUID_TYPE_NUGGET);
            uuid_unparse(source, l_sNugId);
            rzb_log(LOG_INFO, "%s: Hello received from unregistered nugget (termination requested): %s. %s, %s", __func__, l_sNugType, l_sAppType, l_sNugId);
            free(l_sNugType);
            free(l_sAppType);
            if ((term = MessageReReg_Initialize(sg_rbContext.uuidNuggetId,source)) == NULL)
            {
                rzb_log (LOG_ERR, "%s: C&C Hello Processor: Failed to create ReRegister Message", __func__);
                return false;
            }
            if (!Queue_Put (g_ccWrite, term))
            {
                term->destroy(term);
                rzb_log (LOG_ERR, "%s: C&C Hello Processor: Failed to send ReRegister Message", __func__);
                return false;
            }
            term->destroy(term);
            Datastore_Set_Nugget_State(sg_pCnC_Db_Connection, source, NUGGET_STATE_REREG);
            return true;
        }
    }
    else // Dispatcher hello
    {
        pthread_mutex_lock(&contextLock);
        if (!sg_rbContext.regOk)
        {
            
            if (sg_rbContext.locality == hello->locality)
            {
                if (hello->flags & DISP_HELLO_FLAG_LM)
                    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LS;
                else if (hello->flags & DISP_HELLO_FLAG_LS)
                    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LM;
            }
            if (hello->flags & DISP_HELLO_FLAG_DD)
                sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_DD;
        }
        else
        {
            // Handle messages from other dispatchers in our locality
            if (sg_rbContext.locality == hello->locality)
                processLocalityHello(hello, source);

            // Handle messages from the dedicated dispatcher -- conflict resolution
            if ( ((hello->flags & DISP_HELLO_FLAG_DD) != 0) &&
                    ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) != 0))
            {
                if (hello->priority > sg_rbContext.dispatcher.priority)
                    sg_rbContext.dispatcher.flags ^= DISP_HELLO_FLAG_DD;
                if (hello->priority == sg_rbContext.dispatcher.priority)
                {
                    if (uuid_compare(source, sg_rbContext.uuidNuggetId) > 0 )
                        sg_rbContext.dispatcher.flags ^= DISP_HELLO_FLAG_DD;
                }
            }
        }
        pthread_mutex_unlock(&contextLock);
    }
    ConnectedEntityList_Update (message);
    return true;
}

static void 
entityRemoved(struct ConnectedEntity *entity, uint32_t remainingCount)
{
    uuid_t uuidNugInspection;
    char *nugType;
    char *appName;
    char nugId[UUID_STRING_LENGTH];
    uuid_t dispatcher;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);
    struct ConnectedEntity *high;
    int comp;

    Datastore_Get_UUID_By_Name(sg_pCnC_Db_Connection, NUGGET_TYPE_INSPECTION, UUID_TYPE_NUGGET_TYPE, uuidNugInspection);
    Datastore_Get_UUID_Name_By_UUID(sg_pCnC_Db_Connection, entity->uuidNuggetType, UUID_TYPE_NUGGET_TYPE, &nugType);
    Datastore_Get_UUID_Name_By_UUID(sg_pCnC_Db_Connection, entity->uuidApplicationType, UUID_TYPE_NUGGET_TYPE, &appName);
    uuid_unparse(entity->uuidNuggetId, nugId);
    rzb_log(LOG_INFO, "%s: Nugget going offline: %s - %s - %s Remaining Instances: %u", __func__, nugType, appName, nugId, remainingCount);
    free(nugType);
    free(appName);
    if (uuid_compare(entity->uuidNuggetType, uuidNugInspection) == 0)
    {
        if (remainingCount > 0) // Dont need to do anything still nuggets online.
            return;

        RoutingList_RemoveApplication(entity->uuidApplicationType);
        // TODO: Flush queue
    }
    if (uuid_compare(entity->uuidNuggetType, dispatcher) == 0)
    {
        pthread_mutex_lock(&contextLock);
        if ((entity->dispatcher->flags & DISP_HELLO_FLAG_DD) != 0) // DD Went offline
        {
            // Its election time
            high = ConnectedEntityList_GetDispatcher_HighestPriority();
            if (high == NULL) // We are highest as there is no one else
                sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_DD;
            else if (high->dispatcher->priority < sg_rbContext.dispatcher.priority) // We are higher
                sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_DD;
            else if (high->dispatcher->priority == sg_rbContext.dispatcher.priority) // We are the same
            {
                comp = uuid_compare(high->uuidNuggetId, sg_rbContext.uuidNuggetId);
                if (comp < 0 ) // We are greater
                    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_DD;
            }
            if (high != NULL)
                ConnectedEntity_Destroy(high);
        }
        if (entity->locality == sg_rbContext.locality)
        {
            if ((entity->dispatcher->flags & DISP_HELLO_FLAG_LM) != 0)
            {   
                // The master is dead time to unleash myself
                sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LM;
                sg_rbContext.dispatcher.flags &= (~DISP_HELLO_FLAG_LS);
                updateDispatcherState();
            }
        }
        pthread_mutex_unlock(&contextLock);
    }
    
}
bool
updateDispatcherState(void)
{
    pthread_mutex_lock(&contextLock);
    rzb_log(LOG_DEBUG, "%s: Updating dispatcher state", __func__);
    if (!sg_rbContext.regOk)
    {
        pthread_mutex_unlock(&contextLock);
        return true;
    }
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0)
    {
        sg_pFlagThread = Thread_Launch (FlagCopier_Thread, NULL, (char *)"Flag Copier Thread", &sg_rbContext);
        sg_pRequeueThread = Thread_Launch (Requeue_Thread, NULL, (char *)"Requeue Thread", &sg_rbContext);
        ThreadPool_LaunchWorkers(g_cacheThreadPool, g_cacheThreadsInitial);
        ThreadPool_LaunchWorkers(g_judgmentThreadPool, g_judgmentThreadsInitial);
        ThreadPool_LaunchWorkers(g_logThreadPool, g_logThreadsInitial);
        ThreadPool_LaunchWorkers(g_submissionThreadPool, g_submissionThreadsInitial);
    }
    else if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) != 0)
    {
        if (sg_pFlagThread != NULL)
        {
            Thread_InterruptAndJoin(sg_pFlagThread);
            sg_pFlagThread = NULL;
        }
        if (sg_pRequeueThread != NULL)
        {
            Thread_InterruptAndJoin(sg_pRequeueThread);
            sg_pRequeueThread = NULL;
        }
        ThreadPool_KillWorkers(g_cacheThreadPool);
        ThreadPool_KillWorkers(g_judgmentThreadPool);
        ThreadPool_KillWorkers(g_logThreadPool);
        ThreadPool_KillWorkers(g_submissionThreadPool);
    }
    pthread_mutex_unlock(&contextLock);
    return true;
}
