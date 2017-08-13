#include "config.h"

#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/inspector_queue.h>
#include <razorback/nugget.h>
#include <razorback/log.h>
#include <razorback/uuids.h>

#include "global_cache.h"
#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "routing_list.h"
#include "configuration.h"

#include <errno.h>
#include <sys/time.h>

/** Globals
 */

void 
SubmissionProcessor_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);

    // for each message
    struct Message *message;
    struct Message *outputMessage;
    struct MessageBlockSubmission *l_mbsInput;
    struct Message *l_misInspection;
    struct Nugget *nugget = NULL;
    char *dest = NULL;
    char dataType[UUID_STRING_LENGTH];
    char appType[UUID_STRING_LENGTH];
    char nuggetId[UUID_STRING_LENGTH];
    uint32_t l_iDisposition = 0;
    uint64_t metabookId = 0;
    struct Queue *queue, *outputQueue;
    struct DatabaseConnection *pDbCon;

    // for routing failures
    uint32_t sfFlags, entFlags, result;

    // initialize the database
    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to intialiaze database thread info", __func__);
        return;
    }
    if ((pDbCon = Database_Connect
        (g_sDatabaseHost, g_iDatabasePort, g_sDatabaseUser,
         g_sDatabasePassword, g_sDatabaseSchema)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to failure of Database_Connect", __func__);
        return;
    }

    // intialize the input queue
    if ((queue =Queue_Create (INPUT_QUEUE, QUEUE_FLAG_RECV, Razorback_Get_Message_Mode())) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to failure of InputQueue_Initialize", __func__);
        Database_Disconnect(pDbCon);
        Database_Thread_End();
        return;
    }
    // intialize the input queue
    if ((outputQueue =Queue_Create (INPUT_QUEUE, QUEUE_FLAG_SEND, MESSAGE_MODE_JSON)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to failure of InputQueue_Initialize", __func__);
        Database_Disconnect(pDbCon);
        Database_Thread_End();
        return;
    }

    // run
    while (!Thread_IsStopped(p_pThread))
    {
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
                     "%s: Dropped block judgment due to failure of InputQueue_Get", __func__);
            // drop message
            continue;
        }
        l_mbsInput = message->message;
        char * name;
        // Unknown data type, drop the message
        if (Datastore_Get_UUID_Name_By_UUID(pDbCon, l_mbsInput->pEvent->pBlock->pId->uuidDataType, UUID_TYPE_DATA_TYPE, &name) != R_FOUND)
        {
            // destroy the input message
            message->destroy(message);
            free(name);
            continue;

        }
        free(name);

        // Create an Event
        // TODO: Check if we have the block stored already else this will fail silently.
        if (Datastore_Insert_Event (pDbCon, l_mbsInput->pEvent) != R_SUCCESS)
        {
            rzb_log(LOG_ERR, "%s: Failed to insert event record", __func__);
        }
        // Store the block metadata
        if (Datastore_Get_Block_MetabookId (pDbCon, l_mbsInput->pEvent->pBlock->pId, &metabookId) != R_FOUND)
        {
            rzb_log(LOG_ERR, "%s: Failed to lookup block metabook id", __func__);
        } 
        else
        {
           if (Datastore_Insert_Metadata_List(pDbCon, l_mbsInput->pEvent->pBlock->pMetaDataList, metabookId) != R_SUCCESS)
           {
                rzb_log(LOG_ERR, "%s: Failed to insert block metadata", __func__);
           }
        }
         
        // XXX: This is not so efficient work out how to do this better.
        //
        if (l_mbsInput->pEvent->pBlock->pParentId != NULL)
        {
            Datastore_Insert_Block_Link(pDbCon, 
                    l_mbsInput->pEvent->pBlock->pParentId,
                    l_mbsInput->pEvent->pBlock->pId);
        }

        //TODO: Look up storage localities
        uint32_t localityCount =0;
        uint8_t *localities = NULL;
        if (Datastore_Insert_Block_Locality_Link(pDbCon, l_mbsInput->pEvent->pBlock->pId, l_mbsInput->storedLocality) != R_SUCCESS) {
			rzb_log(LOG_ERR, "%s: Failed to insert block locality", __func__);
		}
		
		
		// TODO: Does this break taint/dirty?
        if (l_mbsInput->iReason == SUBMISSION_REASON_REQUESTED)
        {
            // Record storage locality.
            
            // create the inspection message
            if ((l_misInspection = MessageInspectionSubmission_Initialize
                (l_mbsInput->pEvent, l_iDisposition, localityCount, localities)) == NULL)
            {
                // destroy the input message    
                message->destroy(message);
                // error
                rzb_log (LOG_ERR,
                         "%s: Dropped block routing due to failure of MessageInspectionSubmission_Initialize", __func__);
                continue;
            }

            if(RoutingList_RouteMessage (pDbCon, l_misInspection, l_mbsInput->pEvent->pBlock->pId->uuidDataType) == false)
            {
                GlobalCache_Lock();

                result = Datastore_Get_BlockDisposition(l_mbsInput->pEvent->pBlock->pId, pDbCon, &sfFlags, &entFlags, false);
                if (result != R_FOUND)
                {
                    rzb_log(LOG_ERR, "%s, Failed to get current block disposition", __func__);
                } else if ((sfFlags & SF_FLAG_BAD) != SF_FLAG_BAD) {
                    sfFlags |= SF_FLAG_GOOD;
                    if (Datastore_Set_BlockDisposition(l_mbsInput->pEvent->pBlock->pId, pDbCon, sfFlags, entFlags) != R_SUCCESS)
                        rzb_log(LOG_ERR, "%s: Failed to set block flags", __func__);
                }
                GlobalCache_Unlock();
            }
            l_misInspection->destroy (l_misInspection);
        }

        if (g_outputEventEnabled)
        {
            if (Datastore_Get_NuggetByUUID(pDbCon, l_mbsInput->pEvent->pId->uuidNuggetId, &nugget) != R_FOUND)
            {
                rzb_log(LOG_ERR, "%s: Failed to lookup nugget", __func__);
                message->destroy(message);
                continue;
            }

            if ((outputMessage = MessageOutputEvent_Initialize(l_mbsInput->pEvent, nugget)) == NULL)
            {
                rzb_log(LOG_ERR, "%s: Failed to create output", __func__);
                Nugget_Destroy(nugget);
                message->destroy(message);
                continue;
            }
            uuid_unparse(nugget->uuidApplicationType, appType);
            uuid_unparse(nugget->uuidNuggetId, nuggetId);
            uuid_unparse(l_mbsInput->pEvent->pBlock->pId->uuidDataType, dataType);
            if (asprintf(&dest, "/topic/Event.%s.%s.%s", dataType, appType, nuggetId) == -1)
            {
                ((struct MessageOutputEvent *)outputMessage->message)->event = NULL;
                message->destroy(message);
                outputMessage->destroy(outputMessage);
                rzb_log(LOG_ERR, "%s: Failed to create dest", __func__);
                continue;
            }

            if (!Queue_Put_Dest(outputQueue, outputMessage, dest))
                rzb_log(LOG_ERR, "%s: Failed to send output message", __func__);

            // destroy the input message
            // Set this to null so its not free'd twice
            ((struct MessageOutputEvent *)outputMessage->message)->event = NULL;
            message->destroy(message);
            outputMessage->destroy(outputMessage);
            free(dest);
        }
    }


    // terminate queues
    Queue_Terminate (queue);
    Queue_Terminate (outputQueue);

    Database_Disconnect(pDbCon);
    Database_Thread_End();
}
