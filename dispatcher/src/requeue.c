#include "config.h"

#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/thread.h>
#include <razorback/nugget.h>
#include <razorback/list.h>
#include <razorback/event.h>
#include <razorback/inspector_queue.h>
#include <razorback/log.h>

#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "global_cache.h"
#include "configuration.h"
#include "requeue.h"

#include <errno.h>
#include <sys/time.h>

static struct List *queue = NULL;

static int Request_Cmp(void *a, void *b) { return -1; }

static bool Request_ProcessBlock(struct DatabaseConnection *dbCon, struct RequeueRequest *);
static bool Request_ProcessStatus(struct DatabaseConnection *dbCon, struct RequeueRequest *, uint8_t);

bool
Requeue_Request(struct RequeueRequest *req)
{
    return List_Push(queue, req);
}

void 
Requeue_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);
    struct RequeueRequest *req;
    struct Nugget *nugget;
    char appType[UUID_STRING_LENGTH];
    char nuggetId[UUID_STRING_LENGTH];

    // a database connection
    struct DatabaseConnection *dbCon;

    queue = List_Create(LIST_MODE_QUEUE,
            Request_Cmp, // Cmp
            Request_Cmp, // KeyCmp
            NULL,        // Destroy
            NULL,        // Clone
            NULL,        // Lock
            NULL);       // Unlock

    if (queue == NULL)
        return;
    
    // initialize the database
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
    if (Datastore_Get_NuggetByUUID (dbCon, g_uuidDispatcherId, &nugget) != R_FOUND)
    {
        rzb_log(LOG_ERR, "%s: Failed to get nugget", __func__);
        return;
    }
    uuid_unparse(nugget->uuidApplicationType, appType);
    uuid_unparse(nugget->uuidNuggetId, nuggetId);

    // run
    while (!Thread_IsStopped (p_pThread))
    {
        // Wait for a requeue request
        if ((req = List_Pop(queue)) == NULL)
        {
        	rzb_log(LOG_ERR, "%s: Error dequeuing message", __func__);
            continue;
        }

        switch (req->type)
        {
        case REQUEUE_TYPE_BLOCK:
            rzb_log(LOG_DEBUG, "%s: Request Type: Block", __func__);
            if (!Request_ProcessBlock(dbCon, req))
                rzb_log(LOG_ERR, "%s: Request_ProcessBlock failed", __func__);
            goto freeRequest;
        case REQUEUE_TYPE_ERROR:
            rzb_log(LOG_DEBUG, "%s: Request Type: Error", __func__);
            if (!Request_ProcessStatus(dbCon, req, JUDGMENT_REASON_ERROR))
                rzb_log(LOG_ERR, "%s: Request_ProcessBlock failed", __func__);
            goto freeRequest;
        case REQUEUE_TYPE_PENDING:
            rzb_log(LOG_DEBUG, "%s: Request Type: Pending", __func__);
            if (!Request_ProcessStatus(dbCon, req, JUDGMENT_REASON_PENDING))
                rzb_log(LOG_ERR, "%s: Request_ProcessBlock failed", __func__);
            goto freeRequest;
        default:
            rzb_log(LOG_ERR, "%s: Bad requeue request type: %d", __func__, req->type);
            goto freeRequest;
        }
freeRequest:
        if (req->blockId != NULL)
            BlockId_Destroy(req->blockId);
        free(req);
    }

    Nugget_Destroy(nugget);
    // terminate database
    Database_Disconnect (dbCon);

    // done
    return;
}




static bool Request_ProcessBlock(struct DatabaseConnection *dbCon, struct RequeueRequest *req)
{
    unsigned char *uuids = NULL; // Yuk
    uint64_t count = 0;
    char uuid[UUID_STRING_LENGTH];
    struct Event *event = NULL;
    struct Message *message = NULL;
    struct Queue *queue = NULL;

    if (uuid_is_null(req->appType) == 1)
    {
        Datastore_Get_Block_Inspection_AppTypes (dbCon, req->blockId, &uuids, &count);
    }
    else
    {
        uuids = calloc(16,sizeof(char));
        uuid_copy(uuids, req->appType);
        count = 1;
    }
    for (uint64_t i = 0; i < count; i++)
    {
        uuid_unparse(&(uuids[i*16]), uuid);
        rzb_log(LOG_DEBUG, "%ju - %s", i, uuid);
        if (Datastore_Get_Block_Inspection_Event(dbCon, req->blockId, &(uuids[i*16]), &event) != R_FOUND)
            continue;
        // TODO: Look up block storage localities.
        // TODO: This should probably not be 0 in the second arg or the arg should not exist.
        message = MessageInspectionSubmission_Initialize (event, 0, 0, NULL);
        if (message == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create inspection message", __func__);
            Event_Destroy(event);
            continue;
        }
        queue = InspectorQueue_Initialize (&(uuids[i*16]), QUEUE_FLAG_SEND);
        if (queue == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create inspection message", __func__);
            Event_Destroy(event);
            message->destroy(message);
            continue;
        }
        if (Datastore_Update_Block_Inspection_Status (dbCon, req->blockId,
                &(uuids[i*16]), NULL, JUDGMENT_REASON_PENDING) != R_SUCCESS)
        {
            rzb_log(LOG_ERR, "%s: Failed to create inspection message", __func__);
            Event_Destroy(event);
            message->destroy(message);
            continue;
        }

        if (!Queue_Put(queue, message))
        {
            rzb_log(LOG_ERR, "%s: Failed to enqueue message", __func__);
            Event_Destroy(event);
            message->destroy(message);
            continue;
        }
        Event_Destroy(event);
        message->destroy(message);
    }
    free(uuids);
    return true;
}

static bool Request_ProcessStatus(struct DatabaseConnection *dbCon, struct RequeueRequest *req, uint8_t status)
{
    unsigned char *uuid;
    struct Event **events = NULL;
    uint64_t count = 0;
    unsigned char *appTypes = NULL;
    struct Message *message = NULL;
    struct Queue *queue = NULL;

    if (uuid_is_null(req->appType) == 1)
        uuid = NULL;
    else
        uuid = req->appType;

    Datastore_Get_Block_Inspection_Events_By_Status(dbCon, status, uuid, &events, &appTypes, &count);
    for (uint64_t i = 0; i < count; i++)
    {
        message = MessageInspectionSubmission_Initialize (events[i], 0, 0, NULL);
        if (message == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create inspection message", __func__);
            Event_Destroy(events[i]);
            continue;
        }
        queue = InspectorQueue_Initialize (&(appTypes[i*16]), QUEUE_FLAG_SEND);
        if (queue == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create inspection message", __func__);
            Event_Destroy(events[i]);
            message->destroy(message);
            continue;
        }
        if (Datastore_Update_Block_Inspection_Status (dbCon, events[i]->pBlock->pId,
                &(appTypes[i*16]), NULL, JUDGMENT_REASON_PENDING) != R_SUCCESS)
        {
            rzb_log(LOG_ERR, "%s: Failed to create inspection message", __func__);
            Event_Destroy(events[i]);
            message->destroy(message);
            continue;
        }

        if (!Queue_Put(queue, message))
        {
            rzb_log(LOG_ERR, "%s: Failed to enqueue message", __func__);
            Event_Destroy(events[i]);
            message->destroy(message);
            continue;
        }
        Event_Destroy(events[i]);
        message->destroy(message);
        
    }
    free(events);
    free(appTypes);

    return true;
}

