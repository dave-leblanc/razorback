#include "config.h"

#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/thread.h>
#include <razorback/queue.h>
#include <razorback/nugget.h>
#include <razorback/event.h>

#include "judgment_processor.h"
#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "global_cache.h"
#include "configuration.h"
#include "dbus/processor.h"

#include <errno.h>
#include <sys/time.h>

#define DISP_MASK  (SF_FLAG_GOOD | SF_FLAG_BAD)


static void
JudgmentProcessor_ProcessAlert(struct Judgment *judgment, struct DatabaseConnection *dbCon, struct Queue *queue)
{
    uint32_t sfFlags = 0, entFlags = 0;
    uint32_t oldSfFlags = 0, oldEntFlags = 0;
    uint32_t newSfFlags = 0, newEntFlags = 0;
    struct Block *block = NULL;
    struct Event *event = NULL;
    struct Nugget *nugget = NULL;
    struct Message *message = NULL;
    struct MessageAlertPrimary *alert;
    char *dest = NULL;
    char dataType[UUID_STRING_LENGTH];
    char appType[UUID_STRING_LENGTH];
    char nuggetId[UUID_STRING_LENGTH];

    Lookup_Result result;

    GlobalCache_Lock();


    result = Datastore_Get_BlockDisposition(judgment->pBlockId, dbCon, &sfFlags, &entFlags, false);
    if (result != R_FOUND)
    {
        rzb_log(LOG_ERR, "%s, Failed to get current block disposition", __func__);
        GlobalCache_Unlock();
        return;
    }
    oldSfFlags = sfFlags;
    oldEntFlags = entFlags;


    // Setup new SF Flags without the disposition bits this will be set later.
    newSfFlags = (sfFlags & ~(DISP_MASK)) | (judgment->Set_SfFlags & ~(DISP_MASK));
    // Remove bits that where unset
    newSfFlags = newSfFlags & ~(judgment->Unset_SfFlags);

    // Setup new Ent flags
    newEntFlags = entFlags | judgment->Set_EntFlags;
    // Remove bits that where unset
    newEntFlags = newEntFlags & ~(judgment->Unset_EntFlags);

    if ((sfFlags & SF_FLAG_BAD) == SF_FLAG_BAD)
    {
        // All outcomes restul in the block still being bad.
        newSfFlags = newSfFlags | SF_FLAG_BAD;
    } 
    else if ((sfFlags & SF_FLAG_GOOD) == SF_FLAG_GOOD)
    {
        if ((judgment->Set_SfFlags & SF_FLAG_BAD) == SF_FLAG_BAD)
        {
            // Remove good - Set bad
            newSfFlags = newSfFlags | SF_FLAG_BAD;
        }
        else 
        {
            // Guess we are just setting other flags, we are still good though.
            newSfFlags = newSfFlags | SF_FLAG_GOOD;
        }
    }
    else
    {
        // No current status, just set what the message has
        newSfFlags = newSfFlags | (judgment->Set_SfFlags & DISP_MASK);
    }
    if (Datastore_Set_BlockDisposition(judgment->pBlockId, dbCon, newSfFlags, newEntFlags) != R_SUCCESS)
        rzb_log(LOG_ERR, "%s: Failed to set block flags", __func__);

    GlobalCache_Unlock();
    if (Datastore_Insert_Alert(dbCon, judgment) != R_SUCCESS)
        rzb_log(LOG_ERR, "%s: Failed to insert alert", __func__);
    // Gen output
    //
    if (g_outputAlertEnabled)
    {
        if (Datastore_Get_NuggetByUUID (dbCon, judgment->uuidNuggetId, &nugget) != R_FOUND)
        {
            rzb_log(LOG_ERR, "%s: Failed to get nugget", __func__);
            return;
        }

        if (Datastore_Get_Event_Chain(dbCon, judgment->pEventId, &event, &block) != R_SUCCESS)
        {
            Nugget_Destroy(nugget);
            rzb_log(LOG_ERR, "%s: Failed to get event chain", __func__);
            return;
        }

        message = MessageAlertPrimary_Initialize(
                event,
                block,
                judgment->pMetaDataList,
                nugget,
                judgment,
                newSfFlags, newEntFlags,
                oldSfFlags, oldEntFlags);
        if (message == NULL)
        {
            Event_Destroy(event);
            Nugget_Destroy(nugget);
            rzb_log(LOG_ERR, "%s: Failed create output", __func__);
            return;
        }
        alert = message->message;
        uuid_unparse(alert->block->pId->uuidDataType, dataType);
        uuid_unparse(alert->nugget->uuidApplicationType, appType);
        uuid_unparse(alert->nugget->uuidNuggetId, nuggetId);
        if (asprintf(&dest, "/topic/Alert.%u.%s.%s.%s", alert->priority, dataType, appType, nuggetId) == -1)
        {
            rzb_log(LOG_ERR, "%s: Failed create dest", __func__);
            alert->metadata = NULL;
            alert->message = NULL;
            message->destroy(message); 
            return;
        }

        if (!Queue_Put_Dest(queue, message, dest))
        {
            rzb_log(LOG_ERR, "%s: Failed send message", __func__);
            free(dest);
            alert->metadata = NULL;
            alert->message = NULL;
            message->destroy(message); 
            return;
        }
        free(dest);
        alert->metadata = NULL;
        alert->message = NULL;
        message->destroy(message); 
    }
}

static void 
JudgmentProcessor_ProcessCompletion(uint8_t reason, struct Judgment *judgment, struct DatabaseConnection *dbCon, struct Queue *queue)
{
    struct Nugget *nugget = NULL;
    Lookup_Result result;
	uint8_t *locality = NULL;
	uint32_t i = 0;
	uint32_t numLocalities = 0;
    uint32_t sfFlags = 0, entFlags = 0;
    uint64_t count =0;
    bool final = false;
    struct Message *message;
    char *dest = NULL;
    char appType[UUID_STRING_LENGTH];
    char nuggetId[UUID_STRING_LENGTH];

    result = Datastore_Get_NuggetByUUID(dbCon, judgment->uuidNuggetId, &nugget);
    if (result != R_FOUND)
    {
        rzb_log(LOG_ERR, "%s, Failed to get current nugget", __func__);
        return;
    }
    // Remove the inspection record.
    Datastore_Update_Block_Inspection_Status(dbCon, judgment->pBlockId, nugget->uuidApplicationType, nugget->uuidNuggetId, reason);
    Datastore_Count_Outstanding_Block_Inspections(dbCon, judgment->pBlockId, &count);
    if (count == 0)
    {
        final = true;
        rzb_log(LOG_DEBUG, "%s, No outstanding inspections, updating flags", __func__);
        GlobalCache_Lock();
        result = Datastore_Get_BlockDisposition(judgment->pBlockId, dbCon, &sfFlags, &entFlags, false);
        if (result != R_FOUND)
        {
            Nugget_Destroy(nugget);
            rzb_log(LOG_ERR, "%s, Failed to get current block disposition", __func__);
            GlobalCache_Unlock();
            return;
        }
        if ((sfFlags & DISP_MASK) == 0)
        {
            sfFlags = sfFlags | SF_FLAG_GOOD;
            if (Datastore_Set_BlockDisposition(judgment->pBlockId, dbCon, sfFlags, entFlags) != R_SUCCESS)
                rzb_log(LOG_ERR, "%s: Failed to set block flags", __func__);
        }
        if ((sfFlags &  SF_FLAG_PROCESSING) == SF_FLAG_PROCESSING)
        {
            sfFlags = sfFlags & ~(SF_FLAG_PROCESSING);
            if (Datastore_Set_BlockDisposition(judgment->pBlockId, dbCon, sfFlags, entFlags) != R_SUCCESS)
                rzb_log(LOG_ERR, "%s: Failed to set block flags", __func__);
        }

		if ((sfFlags & g_sStorageDeletePolicy) != 0) {
            if (Datastore_Get_Block_Localities(dbCon, judgment->pBlockId, &locality, &numLocalities) != R_SUCCESS)
				rzb_log(LOG_ERR, "%s: Failed to get locality of origin for file delete", __func__);
			else {
				for (i = 0; i < numLocalities; i++) {
			        DBus_File_Delete(judgment->pBlockId, locality[i]);
				}
			}
		}

		GlobalCache_Unlock();

    }
    if (g_outputInspectionEventEnabled)
    {
        message = MessageOutputInspection_Initialize(nugget, judgment->pBlockId, reason, final);
        if (message == NULL)
        {
            Nugget_Destroy(nugget);
            return;
        }
        uuid_unparse(nugget->uuidApplicationType, appType);
        uuid_unparse(nugget->uuidNuggetId, nuggetId);
        if (asprintf(&dest, "/topic/InspectionEvent.%s.%s.%u", appType, nuggetId, reason) == -1)
        {
            rzb_log(LOG_ERR, "%s: Failed create dest", __func__);
            message->destroy(message); 
            return;
        }

        if (!Queue_Put_Dest(queue, message, dest))
        {
            rzb_log(LOG_ERR, "%s: Failed send message", __func__);
            free(dest);
            message->destroy(message); 
            return;
        }
        message->destroy(message);

    }
    else
        Nugget_Destroy(nugget);
}

static void
JudgmentProcessor_ProcessDeferred(struct Judgment *judgment, struct DatabaseConnection *dbCon)
{
    struct Nugget *nugget = NULL;
    Lookup_Result result;

    result = Datastore_Get_NuggetByUUID(dbCon, judgment->uuidNuggetId, &nugget);
    if (result != R_FOUND)
    {
        rzb_log(LOG_ERR, "%s, Failed to get current nugget", __func__);
        return;
    }
    Datastore_Update_Block_Inspection_Status(dbCon, judgment->pBlockId, nugget->uuidApplicationType, nugget->uuidNuggetId, JUDGMENT_REASON_DEFERRED);

}

void 
JudgmentProcessor_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);
    struct Queue *queue;
    struct Queue *outputQueue;

    // a database connection
    struct DatabaseConnection *l_pDbCon;

    // for each message
    struct Message *message;
    struct MessageJudgmentSubmission *l_mjsJudgment;

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

    if ((queue = Queue_Create (JUDGMENT_QUEUE, QUEUE_FLAG_RECV, Razorback_Get_Message_Mode())) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JudgmentQueue_Initialize", __func__);
        Database_Disconnect(l_pDbCon);
        return;
    }
    if ((outputQueue = Queue_Create (JUDGMENT_QUEUE, QUEUE_FLAG_SEND, MESSAGE_MODE_JSON)) == NULL)
    {
        Database_Disconnect(l_pDbCon);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Create Output", __func__);
        return;
    }

    // run
    while (!Thread_IsStopped(p_pThread))
    {

        // get next message
        if ((message = Queue_Get (queue)) == NULL)
        {
            // check for timeout
            if (errno == EAGAIN)
                continue;
            // error
            rzb_log (LOG_ERR,
                     "%s: Dropped block judgment due to failure of JudgmentQueue_Get", __func__);
            // drop the message
            continue;
        }
        l_mjsJudgment = message->message;
        switch (l_mjsJudgment->iReason)
        {
        case JUDGMENT_REASON_ERROR:
            // TODO: Record Error, possibly retry. - Requires Block Storage
        case JUDGMENT_REASON_DONE:
            rzb_log(LOG_DEBUG, "%s: Done/Error Judgment", __func__);
            JudgmentProcessor_ProcessCompletion(l_mjsJudgment->iReason, l_mjsJudgment->pJudgment, l_pDbCon, outputQueue);
            break;
        case JUDGMENT_REASON_DEFERRED:
            rzb_log(LOG_DEBUG, "%s: Deferred Judgment", __func__);
            JudgmentProcessor_ProcessDeferred(l_mjsJudgment->pJudgment, l_pDbCon);
            break;
        case JUDGMENT_REASON_ALERT:
            rzb_log(LOG_DEBUG, "%s: Alert Judgment", __func__);
            JudgmentProcessor_ProcessAlert(l_mjsJudgment->pJudgment, l_pDbCon, outputQueue);
            break;
        default:
            rzb_log(LOG_ERR, "%s: Bad reason code, how did i get here", __func__);
            break;
        }
        message->destroy(message);
    
    }

    // terminate database
    Database_Disconnect (l_pDbCon);

    // terminate queues
    Queue_Terminate (queue);
    Queue_Terminate (outputQueue);

    // done
    return;
}
