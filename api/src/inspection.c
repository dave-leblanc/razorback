#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/api.h>
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/thread.h>
#include <razorback/thread_pool.h>
#include <razorback/event.h>
#include <razorback/ntlv.h>
#include <razorback/block.h>
#include <razorback/queue.h>
#include <razorback/inspector_queue.h>
#include <razorback/judgment.h>
#include "judgment_private.h"
#include "command_and_control.h"
#include "transfer/core.h"
#include "connected_entity_private.h"
#include "runtime_config.h"
#include <errno.h>
static void Inspection_Thread (struct Thread *p_pThread);

bool
Inspection_Launch (struct RazorbackContext *p_pContext, uint32_t initThreads, uint32_t maxThreads)
{
    p_pContext->inspector.threadPool = ThreadPool_Create(
            ((initThreads == 0) ? Config_getInspThreadsInit() : initThreads),
            ((maxThreads == 0) ? Config_getInspThreadsMax() : maxThreads),
            p_pContext, "Inspection Thread Pool %i", Inspection_Thread);
    if (p_pContext->inspector.threadPool == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to launch thread.", __func__);
        return false;
    }
    return true;
}

static void
Inspection_Thread (struct Thread *p_pThread)
{
    struct RazorbackContext *l_pContext;
    struct Message *message;
    struct MessageInspectionSubmission *l_misMessage;
    struct Message *l_mjsMessage;
    struct Block *l_pBlock, *l_pClonedBlock;
    struct EventId *l_pEventId;
    struct Queue *l_pQueue;
    uint8_t l_iResult;
    struct Judgment *judgment;
    bool transfered = false;
    struct ConnectedEntity *dispatcher = NULL;
    void *threadData = NULL;

	l_pContext = Thread_GetContext (p_pThread);
    if ((l_pQueue =
         InspectorQueue_Initialize (l_pContext->uuidApplicationType,
                                    QUEUE_FLAG_RECV)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to connect to MQ - Inspector Queue",
                 __func__);
        return;
    }
    rzb_log(LOG_DEBUG, "%s: Inspection Thread Launched", __func__);
    p_pThread->pUserData = l_pQueue;
    if (l_pContext->inspector.hooks->initThread != NULL)
    {
    	if (!l_pContext->inspector.hooks->initThread(&threadData))
    	{
    		rzb_log(LOG_ERR, "%s: Failed to init thread", __func__);
    		return;
    	}
    }

    while (!Thread_IsStopped(p_pThread))
    {
        if ((message = Queue_Get (l_pQueue)) == NULL)
        {
            // timeout
            if (errno == EAGAIN || errno == EINTR)
                continue;
            // error
            rzb_log (LOG_ERR,
                     "%s: Dropped block due to failure of InspectorQueue_Get()",
                     __func__);
            // drop message
            continue;
        }
        l_misMessage = message->message;
        if (l_misMessage->pBlock == NULL)
        {
            rzb_log (LOG_ERR, "%s: Failed dispatch message due to NULL block",
                     __func__);
            continue;
        }
        if (l_misMessage->pBlock->pId->pHash == NULL)
        {
            rzb_log (LOG_ERR, "%s: Failed dispatch message due to NULL Hash",
                     __func__);
            continue;
        }
        l_pBlock = l_misMessage->pBlock;
        l_misMessage->pBlock = NULL;
        transfered=false;
        while (!transfered)
        {
            dispatcher = ConnectedEntityList_GetDispatcher();
            if (dispatcher == NULL)
            {
                rzb_log(LOG_ERR, "%s: Failed to find usable dispatcher", __func__);
                transfered = false;
                break;
            }
            transfered = Transfer_Fetch(l_pBlock, dispatcher);
            if (!transfered)
            {
                rzb_log(LOG_ERR, "%s: Marking dispatcher unusable", __func__);
                ConnectedEntityList_MarkDispatcherUnusable(dispatcher->uuidNuggetId);
            }
        }
        if (!transfered)
        {
            rzb_log(LOG_ERR, "%s: Failed to transfer block giving up", __func__);
            continue;
        }

        if (l_pBlock->data.pointer == NULL || l_pBlock->data.fileName == NULL)
        {
            rzb_log (LOG_ERR, "%s: No data block",__func__);
           continue;
        }
        if ((l_pEventId = EventId_Clone(l_misMessage->eventId)) == NULL)
        {
            rzb_log (LOG_ERR, "%s: Failed create new event id", __func__);
            continue;
        }
        

        // Clone the block for the inspector to use.
        if ((l_pClonedBlock = Block_Clone (l_pBlock)) == NULL)
        {
            rzb_log (LOG_ERR, "%s: Failed create new block", __func__);
            continue;
        }

        l_pClonedBlock->data.pointer = l_pBlock->data.pointer;
        l_pClonedBlock->data.file = l_pBlock->data.file;
        l_pClonedBlock->data.fileName = l_pBlock->data.fileName;
		l_pClonedBlock->data.tempFile = l_pBlock->data.tempFile;
		l_pBlock->data.pointer = NULL;
        l_pBlock->data.file = NULL;
        l_pBlock->data.fileName = NULL;

#ifdef _MSC_VER
		l_pClonedBlock->data.mfileHandle = l_pBlock->data.mfileHandle;
		l_pClonedBlock->data.mapHandle = l_pBlock->data.mapHandle;
		l_pBlock->data.mfileHandle = NULL;
		l_pBlock->data.mapHandle = NULL;
#endif
        


        l_iResult =
            l_pContext->inspector.hooks->processBlock (l_pClonedBlock,
                                                        l_misMessage->eventId,
                                                        l_misMessage->pEventMetadata, threadData);

        message->destroy(message);
        if ((l_iResult != JUDGMENT_REASON_DONE)
            && (l_iResult != JUDGMENT_REASON_ERROR)
            && (l_iResult != JUDGMENT_REASON_DEFERRED))
        {
            rzb_log (LOG_ERR, "%s: Bad return from inspection", __func__);
            continue;
        }


        // Lock the pause lock before submitting judgment
        Mutex_Lock (sg_mPauseLock);
        judgment = Judgment_Create(l_pEventId, l_pClonedBlock->pId);
        // Destroy the copy, we don't need it any more
        Transfer_Free(l_pClonedBlock, dispatcher);
        l_pClonedBlock->data.pointer = NULL;
        Block_Destroy (l_pClonedBlock);
        if ((l_mjsMessage = MessageJudgmentSubmission_Initialize (l_iResult, judgment)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create message", __func__);
        }
        else
        {
            Queue_Put (l_pContext->inspector.judgmentQueue, l_mjsMessage);
            l_mjsMessage->destroy(l_mjsMessage);
        }
        Mutex_Unlock (sg_mPauseLock);
        Block_Destroy(l_pBlock);
        EventId_Destroy(l_pEventId);
    }
    if (l_pContext->inspector.hooks->cleanupThread != NULL)
        l_pContext->inspector.hooks->cleanupThread(threadData);

    rzb_log(LOG_DEBUG, "%s: Inspection Thread Exiting", __func__);
    return;
}

