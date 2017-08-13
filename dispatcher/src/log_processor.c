#include "config.h"

#include <razorback/debug.h>
#include <razorback/thread.h>
#include <razorback/log.h>
#include <razorback/queue.h>
#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/nugget.h>

#include "log_processor.h"
#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "configuration.h"

#include <errno.h>
#include <sys/time.h>


void 
LogProcessor_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);

    struct DatabaseConnection *l_pDbCon;
    struct Queue *queue, *outputQueue;

    // for each message
    struct Message *msg, *logMsg;
    struct MessageLogSubmission *message;
    struct MessageOutputLog *logOut;
    struct Nugget *nugget = NULL;
    char appType[UUID_STRING_LENGTH];
    char nuggetId[UUID_STRING_LENGTH];
    char *dest = NULL;

    // initialize the database
    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to intialiaze database thread info", __func__);
        return;
    }

    if ((l_pDbCon = Database_Connect
        (g_sDatabaseHost, g_iDatabasePort, g_sDatabaseUser,
         g_sDatabasePassword, g_sDatabaseSchema)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Database_Connect", __func__);
        return;
    }

    // intialize the input queue
    if ((queue = Queue_Create (LOG_QUEUE, QUEUE_FLAG_RECV, Razorback_Get_Message_Mode())) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of LogQueue_Initialize", __func__);
        Database_Disconnect(l_pDbCon);
        Database_Thread_End();
        return;
    }
    // Queue name irrelavant
    if ((outputQueue = Queue_Create (JUDGMENT_QUEUE, QUEUE_FLAG_SEND, MESSAGE_MODE_JSON)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Create Output", __func__);
        return;
    }

    // run
    while (!Thread_IsStopped(p_pThread))
    {
        // get next message from collectors
        if ((msg = Queue_Get (queue)) == NULL)
        {
            // timeout
            if (errno == EAGAIN)
            {
                continue;
            }
            // error
            rzb_log (LOG_ERR,
                     "%s: Failed to get message from queue.", __func__);
            // drop message
            continue;
        }
        message = msg->message;
        if (message->pEventId == NULL)
        {
            // No event
            Datastore_Insert_Log_Message(l_pDbCon, message->uuidNuggetId, message->iPriority, message->sMessage);
        }
        else 
        {
            Datastore_Insert_Log_Message_With_Event(l_pDbCon, message->uuidNuggetId, message->iPriority, message->sMessage, message->pEventId);
        }
        if (Datastore_Get_NuggetByUUID (l_pDbCon, message->uuidNuggetId, &nugget) != R_FOUND)
        {
            msg->destroy(msg);
            rzb_log(LOG_ERR, "%s: Failed to get nugget", __func__);
            continue;
        }

        if ((logMsg = MessageOutputLog_Initialize(message, nugget)) == NULL)
        {
            msg->destroy(msg);
            rzb_log(LOG_ERR, "%s: Failed to create output message", __func__);
            continue;
        }
        logOut = logMsg->message;
        uuid_unparse(nugget->uuidApplicationType, appType);
        uuid_unparse(nugget->uuidNuggetId, nuggetId);
        if (asprintf(&dest, "/topic/Log.%u.%s.%s", logOut->priority, appType, nuggetId) == -1)
        {
            rzb_log(LOG_ERR, "%s: Failed create dest", __func__);
            msg->destroy(msg);
            logOut->message = NULL;
            logOut->event->pId = NULL;
            logMsg->destroy(logMsg);
            continue;
        }

        if (!Queue_Put_Dest(outputQueue, logMsg, dest))
            rzb_log(LOG_ERR, "%s: Failed send output", __func__);
        
        // Only free these once
        logOut->message = NULL;
        logOut->event->pId = NULL;
        free(dest);
        logMsg->destroy(logMsg);
        msg->destroy(msg);
    }

    Queue_Terminate(queue);
    Queue_Terminate(outputQueue);

    // terminate queues
    //RequestQueue_Terminate ();
    Database_Disconnect(l_pDbCon);
    Database_Thread_End();
}
