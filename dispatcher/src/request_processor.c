#include "config.h"

#include <razorback/debug.h>
#include <razorback/thread.h>
#include <razorback/log.h>
#include <razorback/queue.h>
#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/response_queue.h>

#include "request_processor.h"
#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "configuration.h"

#include <errno.h>
#include <sys/time.h>


void 
RequestProcessor_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);

    struct DatabaseConnection *l_pDbCon;
    struct Queue *queue;
    struct Queue *respQueue;

    // for each message
    struct Message *message;
    struct MessageCacheReq *l_mcrMessage;
    struct Message *l_mcrResponse;
    uint32_t l_iSfFlags, l_iEntFlags;
    Lookup_Result l_eLookupResult;
    char * name;

    // initialize the database
    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to initialize database thread info", __func__);
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

    // Initialize the input queue
    if ((queue = Queue_Create (REQUEST_QUEUE, QUEUE_FLAG_RECV, Razorback_Get_Message_Mode())) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of RequestQueue_Initialize", __func__);
        Database_Disconnect(l_pDbCon);
        Database_Thread_End();
        return;
    }

    // run
    while (!Thread_IsStopped(p_pThread))
    {
        l_iSfFlags = 0;
        l_iEntFlags = 0;
        // get next message from collectors
        if ((message = Queue_Get (queue)) == NULL)
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
        l_mcrMessage=message->message;
        if ((name = UUID_Get_NameByUUID(l_mcrMessage->pId->uuidDataType, UUID_TYPE_DATA_TYPE)) == NULL)
        {
            rzb_log(LOG_INFO, "%s: Cache request for invalid data type", __func__);
        }
        else
        {
            free(name);
            Statistics_Incr_DataType_CacheRequest(l_mcrMessage->pId->uuidDataType);
            l_eLookupResult = Datastore_Get_BlockDisposition(l_mcrMessage->pId, l_pDbCon, &l_iSfFlags, &l_iEntFlags, true);
            if (l_eLookupResult == R_NOT_FOUND)
            {
                Statistics_Incr_DataType_CacheCanHaz(l_mcrMessage->pId->uuidDataType);
                l_iSfFlags = SF_FLAG_PROCESSING | SF_FLAG_CANHAZ;
            }
            else if (l_eLookupResult == R_FOUND)
            {
                //XXX: We currently don't diffrentiate a hit between the GC and the DB.
                Statistics_Incr_DataType_CacheHit(l_mcrMessage->pId->uuidDataType);
                // Conditions to ask for data again.
                if ( (l_iSfFlags & SF_FLAG_DIRTY)  == SF_FLAG_DIRTY )
                {
                    Statistics_Incr_DataType_CacheCanHaz(l_mcrMessage->pId->uuidDataType);
                    // Remove the dirty flag, set processing and update cache
                    l_iSfFlags = (SF_FLAG_PROCESSING | l_iSfFlags) & ~(SF_FLAG_DIRTY);
                    Datastore_Set_BlockDisposition(l_mcrMessage->pId, l_pDbCon, l_iSfFlags, l_iEntFlags);
                    // Add the request flag before sending it to the requestor
                    l_iSfFlags = l_iSfFlags | SF_FLAG_CANHAZ;
                }

            }
        }
        rzb_log(LOG_DEBUG, "%s: Sending flags SF: 0x%08x, ENT: 0x%08x", __func__, l_iSfFlags, l_iEntFlags);
        if ((l_mcrResponse = MessageCacheResp_Initialize(l_mcrMessage->pId, 
                l_iSfFlags, l_iEntFlags)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create message", __func__);
            message->destroy(message);
            continue;
        }
        respQueue = ResponseQueue_Initialize(l_mcrMessage->uuidRequestor, QUEUE_FLAG_SEND);
        Queue_Put(respQueue, l_mcrResponse);
        message->destroy(message);
        l_mcrResponse->destroy(l_mcrResponse);
    }


    // terminate queues
    Queue_Terminate (queue);
    Database_Disconnect(l_pDbCon);
    Database_Thread_End();
}
