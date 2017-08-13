#include "config.h"
#include <razorback/debug.h>
#include <razorback/submission.h>
#include <razorback/lock.h>
#include <razorback/log.h>
#include <razorback/thread.h>
#include <razorback/api.h>
#include <razorback/block.h>
#include <razorback/block_id.h>
#include <razorback/queue.h>
#include <razorback/block_id.h>
#include <razorback/thread_pool.h>
#include <razorback/response_queue.h>

#include "block_pool_private.h"
#include "submission_private.h"
#include "local_cache.h"
#include "connected_entity_private.h"
#include "transfer/core.h"
#include "runtime_config.h"
#include <string.h>

struct ThreadPool *requestThreadPool= NULL;
struct ThreadPool *responseThreadPool= NULL;
struct ThreadPool *submissionThreadPool = NULL;

void Submission_GlobalCache_RequestThread(struct Thread *p_pThread);
void Submission_GlobalCache_ResponseThread(struct Thread *p_pThread);
int Submission_GlobalCache_ResponseHandler(struct BlockPoolItem *p_pItem, void *);
void Submission_SubmitThread(struct Thread *p_pThread);

static struct List *requestQueue = NULL;
static struct List *submitQueue = NULL;

static struct RazorbackContext *sg_pContext = NULL;
static bool sg_bInitDone=false;

struct CacheResult
{
    uint32_t iSfFlags;
    uint32_t iEntFlags;
    struct BlockId *pId;
};

bool
Submission_Init(struct RazorbackContext *p_pContext)
{
    
    if (sg_bInitDone) 
        return true;

    sg_pContext = p_pContext;
    if (!BlockPool_Init(p_pContext))
    {
        rzb_log(LOG_ERR, "%s: Failed initialize block pool", __func__);
        return false;
    }

    requestQueue = List_Create(LIST_MODE_QUEUE,
    		NULL, // Cmp
    		NULL, // KeyCmp
    		NULL, // Destroy
    		NULL, // Clone
    		NULL, // lock
    		NULL); // Unlock
    submitQueue = List_Create(LIST_MODE_QUEUE,
			NULL, // Cmp
    		NULL, // KeyCmp
    		NULL, // Destroy
    		NULL, // Clone
    		NULL, // lock
    		NULL); // Unlock

    requestThreadPool = ThreadPool_Create(
    		Config_getSubGcReqThreadsInit(),
    		Config_getSubGcReqThreadsMax(),
    		p_pContext,
    		"GC Request Thread (%d)",
    		Submission_GlobalCache_RequestThread);

    responseThreadPool = ThreadPool_Create(
    		Config_getSubGcRespThreadsInit(),
    		Config_getSubGcRespThreadsMax(),
    		p_pContext,
    		"GC Response Thread (%d)",
    		Submission_GlobalCache_ResponseThread);

    submissionThreadPool = ThreadPool_Create(
			Config_getSubTransferThreadsInit(),
    		Config_getSubTransferThreadsMax(),
    		p_pContext,
    		"Submission Transfer Thread (%d)",
    		Submission_SubmitThread);

    sg_bInitDone = true;

    return true;
}

SO_PUBLIC int
Submission_Submit(struct BlockPoolItem *p_pItem, int p_iFlags, uint32_t *p_pSf_Flags, uint32_t *p_pEnt_Flags)
{
	Lookup_Result result;
	uint32_t sfflags = 0;
	uint32_t entflags = 0;
	BlockPool_Item_Lock(p_pItem);

    if ( (p_pItem->pEvent->pBlock->pParentId != NULL ) &&
            BlockId_IsEqual(p_pItem->pEvent->pBlock->pId, p_pItem->pEvent->pBlock->pParentId) )
    {
        // You shall not pass!!!
        rzb_log(LOG_ERR, "%s: Block submission listing its self as parent dropped.", __func__);
        BlockPool_Item_Unlock(p_pItem);
        BlockPool_DestroyItem(p_pItem);
        return RZB_SUBMISSION_ERROR;

    }
    
	if (p_pSf_Flags == NULL || p_pEnt_Flags == NULL) {
		rzb_log(LOG_ERR, "%s: NULL pointer arguments to function", __func__);
        BlockPool_Item_Unlock(p_pItem);
        BlockPool_DestroyItem(p_pItem);
		return RZB_SUBMISSION_ERROR;
	}
	
    // Set up the data type.
    if (uuid_is_null(p_pItem->pEvent->pBlock->pId->uuidDataType) == 1)
    {
        rzb_log(LOG_INFO, "%s: Submission with null data type dropped.", __func__);
        if (p_pItem->submittedCallback != NULL)
            p_pItem->submittedCallback(p_pItem);

        BlockPool_Item_Unlock(p_pItem);
        BlockPool_DestroyItem(p_pItem);
        return RZB_SUBMISSION_NO_TYPE;
    }

    result = checkLocalEntry(p_pItem->pEvent->pBlock->pId->pHash->pData, p_pItem->pEvent->pBlock->pId->pHash->iSize,
			                 &sfflags, &entflags, BADHASH);
    if (result != R_FOUND)
    {
        result = checkLocalEntry(p_pItem->pEvent->pBlock->pId->pHash->pData, p_pItem->pEvent->pBlock->pId->pHash->iSize,
					   &sfflags, &entflags, GOODHASH);
    }
	if (result == R_FOUND) {
        rzb_log(LOG_INFO, "%s: Local Cache Hit - SF: 0x%08x, ENT: 0x%08x", __func__, sfflags, entflags); 
        BlockPool_DestroyItemDataList(p_pItem); // We don't need the data any more.
        BlockPool_SetStatus(p_pItem, BLOCK_POOL_STATUS_SUBMIT_DATA);
        BlockPool_SetFlags(p_pItem, p_iFlags | BLOCK_POOL_FLAG_EVENT_ONLY);
        List_Push(submitQueue, p_pItem);
	}
	else 
	{
        BlockPool_SetStatus(p_pItem, BLOCK_POOL_STATUS_CHECK_GLOBAL_CACHE);
        BlockPool_SetFlags(p_pItem, p_iFlags);
        List_Push(requestQueue, p_pItem);
    }

	BlockPool_Item_Unlock(p_pItem);
    *p_pSf_Flags = sfflags;
    *p_pEnt_Flags = entflags;
    return RZB_SUBMISSION_OK;
}

void 
Submission_GlobalCache_RequestThread(struct Thread *p_pThread)
{
	struct BlockPoolItem *item = NULL;
	struct Queue *queue = NULL;
	struct Message *message = NULL;
#if 0
    struct timespec l_tsTimeOut;
    l_tsTimeOut.tv_sec=1;
#endif
    if ((queue = Queue_Create(REQUEST_QUEUE, QUEUE_FLAG_SEND, Razorback_Get_Message_Mode())) == NULL)
        return;

    while (!Thread_IsStopped(p_pThread))
    {
    	item = List_Pop(requestQueue);
    	if (item == NULL)
    	{
    		rzb_log(LOG_ERR, "%s: Failed to dequeue item", __func__);
    		continue;
    	}
    	BlockPool_Item_Lock(item);
    	if (BlockPool_GetStatus(item) != BLOCK_POOL_STATUS_CHECK_GLOBAL_CACHE)
		{
    		rzb_log(LOG_ERR, "%s: Item in queue with wrong status");
    		BlockPool_Item_Unlock(item);
    		continue;
		}
		BlockPool_SetStatus(item, BLOCK_POOL_STATUS_CHECKING_GLOBAL_CACHE);

		if (( message = MessageCacheReq_Initialize(
						sg_pContext->uuidNuggetId, item->pEvent->pBlock->pId)) == NULL)
		{
			rzb_log(LOG_ERR, "%s: Failed to initialize cache request", __func__);
			BlockPool_Item_Unlock(item);
			continue;
		}
		Queue_Put(queue, message);
		BlockPool_Item_Unlock(item);
		message->destroy(message);
    }
}

int
Submission_GlobalCache_ResponseHandler(struct BlockPoolItem *p_pItem, void * userData)
{
    struct Thread *l_pThread;
    struct CacheResult *l_pRes;

    // Pull the data out the thread.
    l_pThread = Thread_GetCurrent();
    l_pRes = (struct CacheResult *)l_pThread->pUserData;

    if (BlockPool_GetStatus(p_pItem) == BLOCK_POOL_STATUS_CHECKING_GLOBAL_CACHE)
    {
        if (BlockId_IsEqual(p_pItem->pEvent->pBlock->pId, l_pRes->pId))
        {
			if (l_pRes->iSfFlags & SF_FLAG_BAD) {
				
				if (addLocalEntry (p_pItem->pEvent->pBlock->pId->pHash->pData, p_pItem->pEvent->pBlock->pId->pHash->iSize, 
							                 l_pRes->iSfFlags & (SF_FLAG_CANHAZ ^ SF_FLAG_ALL), l_pRes->iEntFlags, BADHASH) != R_SUCCESS) 
				{
					rzb_log(LOG_ERR, "%s: Could not add to bad cache", __func__);
				}
			}
			else 
            {
				if (addLocalEntry (p_pItem->pEvent->pBlock->pId->pHash->pData, p_pItem->pEvent->pBlock->pId->pHash->iSize, 
								             l_pRes->iSfFlags & (SF_FLAG_CANHAZ ^ SF_FLAG_ALL), l_pRes->iEntFlags, GOODHASH) != R_SUCCESS)
				{
					rzb_log(LOG_ERR, "%s: Could not add to good cache", __func__);
				}
			}
            rzb_log(LOG_DEBUG, "%s: Got flags SF: 0x%08x, ENT: 0x%08x", __func__, l_pRes->iSfFlags, l_pRes->iEntFlags); 
            BlockPool_SetStatus (p_pItem, BLOCK_POOL_STATUS_SUBMIT_DATA);
            if ((l_pRes->iSfFlags & SF_FLAG_CANHAZ) != SF_FLAG_CANHAZ)
                BlockPool_SetFlags(p_pItem, p_pItem->iStatus | BLOCK_POOL_FLAG_EVENT_ONLY);

            List_Push(submitQueue, p_pItem);
        }
    }
    Thread_Destroy(l_pThread);
    return LIST_EACH_OK;
}

void 
Submission_GlobalCache_ResponseThread(struct Thread *p_pThread)
{
    struct Message *message;
    struct MessageCacheResp *l_mcrMessage;
    struct CacheResult *l_pRes;
    struct Queue *queue;
    // TODO: Not what we think it is!!
    if ((queue = ResponseQueue_Initialize(sg_pContext->uuidNuggetId, QUEUE_FLAG_RECV)) == NULL)
        return;

    if ((l_pRes = calloc(1, sizeof(struct CacheResult))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate thread args", __func__);
        return;
    }
    p_pThread->pUserData = l_pRes;
    while (!Thread_IsStopped(p_pThread))
    {
        if ((message = Queue_Get(queue)) == NULL)
            continue;
        l_mcrMessage = message->message;
        // Copy the item into thread local storage.
        l_pRes->pId = l_mcrMessage->pId;
        l_pRes->iSfFlags = l_mcrMessage->iSfFlags;
        l_pRes->iEntFlags = l_mcrMessage->iEntFlags;
        BlockPool_ForEachItem(Submission_GlobalCache_ResponseHandler, NULL);
        // Destroy allocated items in thread local storage.
        message->destroy(message);
    }
    free(p_pThread->pUserData);
    p_pThread->pUserData = NULL;
    // TODO: BAD
    ResponseQueue_Terminate(sg_pContext->uuidNuggetId);
}

void
Submission_SubmitThread(struct Thread *p_pThread)
{
	struct Message *message;
    uint8_t storedLocality = 0;
    struct ConnectedEntity *dispatcher = NULL;
    bool transfered = false;
    uint32_t reason = 0;
    struct BlockPoolItem *item = NULL;
    struct Queue *queue = NULL;

#if 0
    struct timespec l_tsTimeOut;
    l_tsTimeOut.tv_sec=1;
#endif
    if ((queue = Queue_Create(INPUT_QUEUE, QUEUE_FLAG_SEND, Razorback_Get_Message_Mode())) == NULL)
        return;

    while (!Thread_IsStopped(p_pThread))
    {
    	item = List_Pop(submitQueue);
    	transfered = false;
    	if (item == NULL)
    	{
    		rzb_log(LOG_ERR, "%s: Failed to dequeue item", __func__);
    		continue;
    	}

    	BlockPool_Item_Lock(item);
		if (BlockPool_GetStatus(item) != BLOCK_POOL_STATUS_SUBMIT_DATA)
		{
			rzb_log(LOG_ERR, "%s: Dequeued item with wrong state", __func__);
			BlockPool_Item_Unlock(item);
			continue;
		}

        if ((item->iStatus & (BLOCK_POOL_FLAG_EVENT_ONLY |BLOCK_POOL_FLAG_UPDATE)) != 0)
        {
            rzb_log(LOG_INFO, "%s: Sending Event Only", __func__);
            reason = SUBMISSION_REASON_EVENT;
        }
        else
        {
            while (!transfered)
            {
                dispatcher = ConnectedEntityList_GetDispatcher();
                rzb_log(LOG_ERR, "%s: %z", __func__, dispatcher);
                if (dispatcher == NULL)
                {
                    rzb_log(LOG_ERR, "%s: Failed to find usable dispatcher", __func__);
                    transfered = false;
                    break;
                }
                transfered = Transfer_Store(item, dispatcher);
                if (!transfered)
                {
                    rzb_log(LOG_ERR, "%s: Marking dispatcher unusable", __func__);
                    ConnectedEntityList_MarkDispatcherUnusable(dispatcher->uuidNuggetId);
                }
            }
            if (!transfered)
            {
                rzb_log(LOG_ERR, "%s: Failed to transfer block giving up", __func__);
                if (item->submittedCallback != NULL)
                    item->submittedCallback(item);
                BlockPool_Item_Unlock(item);
                BlockPool_DestroyItem(item);
                continue;
            }
            storedLocality = dispatcher->locality;
            reason = SUBMISSION_REASON_REQUESTED;
			rzb_log(LOG_ERR, "%s: %z", __func__, dispatcher);
            ConnectedEntity_Destroy(dispatcher);
        }

        if ((message = MessageBlockSubmission_Initialize( item->pEvent, reason, storedLocality)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to create message", __func__);
            if (item->submittedCallback != NULL)
                item->submittedCallback(item);
            BlockPool_Item_Unlock(item);
            BlockPool_DestroyItem(item);
            continue;
        }

        if(!Queue_Put(queue, message))
            rzb_log(LOG_ERR, "%s: Failed to put message", __func__);

        // Set the event to null so we don't destroy it.
        ((struct MessageBlockSubmission*)message->message)->pEvent = NULL;

        message->destroy(message);

        if (item->submittedCallback != NULL)
            item->submittedCallback(item);

        if ((item->iStatus & BLOCK_POOL_FLAG_MAY_REUSE) == BLOCK_POOL_FLAG_MAY_REUSE)
        {
            BlockPool_SetStatus(item, BLOCK_POOL_STATUS_FINALIZED);
            BlockPool_SetFlags(item, 0);
            BlockPool_DestroyItemDataList(item);
            BlockPool_Item_Unlock(item);
            continue;
        }
        else
        {
        	BlockPool_Item_Unlock(item);
        	BlockPool_DestroyItem(item);
            continue;
        }
    }
    Queue_Terminate(queue);
}
