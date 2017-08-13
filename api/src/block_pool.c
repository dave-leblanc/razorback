#include "config.h"
#include <razorback/debug.h>
#include <razorback/block_pool.h>
#include <razorback/list.h>
#include <razorback/log.h>
#include <razorback/hash.h>
#include <razorback/uuids.h>
#include <razorback/ntlv.h>
#include <razorback/metadata.h>
#include <razorback/block_id.h>
#include <razorback/event.h>
#include <sys/stat.h>

#include "block_pool_private.h"
#include "fantasia.h"
#include "runtime_config.h"
#ifdef _MSC_VER
#else //_MSC_VER
#include <sys/mman.h>
#endif //_MSC_VER
#include <string.h>

static void BlockPool_Delete(void *a);


static struct List * sg_bpList;
static bool sg_bInitDone=false;
static size_t sg_size;
static struct Mutex *sg_sizeMutex;
bool
BlockPool_Init(struct RazorbackContext *context)
{
    if (sg_bInitDone) 
        return true;
    sg_bpList = List_Create(LIST_MODE_GENERIC,
            NULL, // Cmp
            NULL, // KeyCmp
            BlockPool_Delete, // destroy
            NULL, // clone
            BlockPool_Item_Lock,
            BlockPool_Item_Unlock);

    sg_sizeMutex = Mutex_Create(MUTEX_MODE_NORMAL);

	if (sg_bpList == NULL || sg_sizeMutex == NULL)
		return false;

    sg_bInitDone = true;
    return true;
}

SO_PUBLIC struct BlockPoolItem *
BlockPool_CreateItem (struct RazorbackContext *context)
{
    struct BlockPoolItem *item;

    ASSERT(context != NULL);
    if (context == NULL)
        return NULL;

    List_Lock(sg_bpList);

    // Enforce pool size limits.
    if ( (Config_getBlockPoolMaxItems() > 0 ) &&
    		(List_Length(sg_bpList) >= Config_getBlockPoolMaxItems()))
    {
    	rzb_log(LOG_ERR, "%s: Block pool item limit exceeded");
    	List_Unlock(sg_bpList);
    	return NULL;
    }

    if ((item = calloc(1, sizeof(struct BlockPoolItem))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new item", __func__);
        List_Unlock(sg_bpList);
        return NULL;
    }

    if ((item->pEvent = Event_Create()) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new event", __func__);
        free(item);
        List_Unlock(sg_bpList);
        return NULL;
    }

    if ((item->mutex = Mutex_Create(MUTEX_MODE_NORMAL)) == NULL)
    {
        Event_Destroy(item->pEvent);
        free(item);
        List_Unlock(sg_bpList);
        return NULL;
    }

    item->iStatus = BLOCK_POOL_STATUS_COLLECTING;

    uuid_copy(item->pEvent->pId->uuidNuggetId, context->uuidNuggetId);
    uuid_copy(item->pEvent->uuidApplicationType, context->uuidApplicationType);

    List_Push(sg_bpList, item);
    List_Unlock(sg_bpList);
    return item;
}

SO_PUBLIC bool 
BlockPool_SetItemDataType(struct BlockPoolItem *item, char * name)
{
    if (!UUID_Get_UUID(name, UUID_TYPE_DATA_TYPE, item->pEvent->pBlock->pId->uuidDataType))
    {
        rzb_log(LOG_ERR, "%s: Invalid data type name", __func__);
        return false;
    }
    return true;
}

SO_PUBLIC bool
BlockPool_AddData_FromFile(struct BlockPoolItem *item, char *fileName, bool tempFile)
{
	struct BlockPoolData *dataBlock;
    struct stat sb;

    ASSERT (item != NULL);
    if (item == NULL)
    	return false;

    ASSERT (fileName != NULL);
    if (fileName == NULL)
    	return false;




    if ((dataBlock = calloc(1, sizeof(struct BlockPoolData))) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: failed to allocate data time", __func__);
        return false;
    }
    dataBlock->data.fileName = strdup(fileName);
    dataBlock->data.tempFile = tempFile;
    if ((dataBlock->data.file = fopen(fileName, "r")) == NULL)
    {
		rzb_perror("WTF: %s");
        rzb_log(LOG_ERR, "%s: failed to open file: %s", __func__, fileName);
		free(dataBlock->data.fileName);
		free(dataBlock);
        return false;
    }
    if (fstat (fileno (dataBlock->data.file), &sb) == -1)
    {
        rzb_log(LOG_ERR, "%s: failed to stat file: %s", __func__, fileName);
		free(dataBlock->data.fileName);
		free(dataBlock);
        return false;
    }

	Mutex_Lock(sg_sizeMutex);
    if ((Config_getBlockPoolMaxSize() > 0) &&
    		((sg_size +sb.st_size )>=
    				Config_getBlockPoolMaxSize()))
    {
		rzb_log(LOG_ERR, "%s: Block pool global size limit exceeded", __func__);
		Mutex_Unlock(sg_sizeMutex);
        return false;
    }
	sg_size +=sb.st_size;
    Mutex_Unlock(sg_sizeMutex);

	BlockPool_Item_Lock(item);
    ASSERT (item->iStatus == BLOCK_POOL_STATUS_COLLECTING);
    if (item->iStatus != BLOCK_POOL_STATUS_COLLECTING)
    {
        rzb_log(LOG_ERR, "%s: failed: item not collecting", __func__);
        BlockPool_Item_Unlock(item);
		Mutex_Lock(sg_sizeMutex);
		sg_size -=sb.st_size;
		Mutex_Unlock(sg_sizeMutex);
        return false;
    }

    if ((Config_getBlockPoolMaxItemSize() > 0) &&
    		((item->pEvent->pBlock->pId->iLength +sb.st_size )>=
    				Config_getBlockPoolMaxItemSize()))
    {
		rzb_log(LOG_ERR, "%s: Block pool item size limit exceeded", __func__);
		free(dataBlock->data.fileName);
		free(dataBlock);
        BlockPool_Item_Unlock(item);
		Mutex_Lock(sg_sizeMutex);
		sg_size -=sb.st_size;
		Mutex_Unlock(sg_sizeMutex);
        return false;
    }
    item->pEvent->pBlock->pId->iLength += sb.st_size;
    dataBlock->iLength = sb.st_size;
    dataBlock->iFlags = BLOCK_POOL_DATA_FLAG_FILE;
    Hash_Update_File(item->pEvent->pBlock->pId->pHash, dataBlock->data.file);

    if (item->pDataHead == NULL)
    {
        item->pDataHead = dataBlock;
        item->pDataTail = dataBlock;
    }
    else
    {
        item->pDataTail->pNext = dataBlock;
        item->pDataTail = dataBlock;
    }

	BlockPool_Item_Unlock(item);

    return true; 
}

SO_PUBLIC bool 
BlockPool_AddData (struct BlockPoolItem *item, uint8_t * data,
                               uint32_t length, int flags)
{
	struct BlockPoolData *l_pData;

    ASSERT (item != NULL);
    if (item == NULL)
    	return false;

    ASSERT (data != NULL);
    if (data == NULL)
    	return false;

    ASSERT (length > 0);
    if (length <= 0)
    	return false;

	Mutex_Lock(sg_sizeMutex);
    if ((Config_getBlockPoolMaxSize() > 0) &&
    		((sg_size +length )>=
    				Config_getBlockPoolMaxSize()))
    {
		rzb_log(LOG_ERR, "%s: Block pool global size limit exceeded", __func__);
		List_Unlock(sg_bpList);
        return false;
    }
	sg_size +=length;
    Mutex_Unlock(sg_sizeMutex);

    BlockPool_Item_Lock(item);
    ASSERT (item->iStatus == BLOCK_POOL_STATUS_COLLECTING);
    if (item->iStatus != BLOCK_POOL_STATUS_COLLECTING)
    {
        rzb_log(LOG_ERR, "%s: failed: item not collecting", __func__);
        BlockPool_Item_Unlock(item);
		Mutex_Lock(sg_sizeMutex);
		sg_size -=length;
		Mutex_Unlock(sg_sizeMutex);
        return false;
    }

    if ((Config_getBlockPoolMaxItemSize() > 0) &&
    		((item->pEvent->pBlock->pId->iLength +length )>=
    				Config_getBlockPoolMaxItemSize()))
    {
    	rzb_log(LOG_ERR, "%s: Block Pool Item Size exceeded", __func__);
        BlockPool_Item_Unlock(item);
		Mutex_Lock(sg_sizeMutex);
		sg_size -=length;
		Mutex_Unlock(sg_sizeMutex);
		return false;
    }

    if ((l_pData = calloc(1, sizeof(struct BlockPoolData))) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: failed to allocate data time", __func__);
        BlockPool_Item_Unlock(item);
		Mutex_Lock(sg_sizeMutex);
		sg_size -=length;
		Mutex_Unlock(sg_sizeMutex);
		return false;
    }
    item->pEvent->pBlock->pId->iLength += length;
    l_pData->iLength = length;
    l_pData->iFlags = flags;
    l_pData->data.pointer = data;
    Hash_Update(item->pEvent->pBlock->pId->pHash, data, length);

    if (item->pDataHead == NULL)
    {
        item->pDataHead = l_pData;
        item->pDataTail = l_pData;
    }
    else
    {
        item->pDataTail->pNext = l_pData;
        item->pDataTail = l_pData;
    }

	BlockPool_Item_Unlock(item);

    return true;
}

SO_PUBLIC bool 
BlockPool_FinalizeItem (struct BlockPoolItem *p_pItem)
{
    ASSERT (p_pItem->iStatus == BLOCK_POOL_STATUS_COLLECTING);

    if (p_pItem->iStatus != BLOCK_POOL_STATUS_COLLECTING)
    {
        rzb_log(LOG_ERR, "%s: failed: item not collecting", __func__);
        return false;
    }
    if (!Hash_Finalize(p_pItem->pEvent->pBlock->pId->pHash))
    {
        rzb_log(LOG_ERR, "%s: Failed to finalize hash", __func__);
        return false;
    }

    if (uuid_is_null(p_pItem->pEvent->pBlock->pId->uuidDataType) == 1 && p_pItem->pDataHead != NULL)
    {
        Magic_process(p_pItem);
    }

    p_pItem->iStatus = BLOCK_POOL_STATUS_FINALIZED;
    return true;
}
void
BlockPool_DestroyItemDataList(struct BlockPoolItem *p_pItem) 
{
    struct BlockPoolData *l_pData;
    while (p_pItem->pDataHead != NULL)
    {
        l_pData = p_pItem->pDataHead;
        p_pItem->pDataHead = p_pItem->pDataHead->pNext;
        switch (l_pData->iFlags)
        {
        case BLOCK_POOL_DATA_FLAG_FILE:
            if (l_pData->data.file != NULL)
                fclose(l_pData->data.file);

            if (l_pData->data.tempFile && (l_pData->data.fileName != NULL))
                unlink(l_pData->data.fileName);

            if (l_pData->data.fileName != NULL)
                free(l_pData->data.fileName);

            break;
        case BLOCK_POOL_DATA_FLAG_MALLOCD:
            free(l_pData->data.pointer);
            break;
        case BLOCK_POOL_DATA_FLAG_MANAGED:
            break;
        default:
            rzb_log(LOG_ERR, "%s: Failed to free block data", __func__);
            break;
        }
        Mutex_Lock(sg_sizeMutex);
		sg_size -=l_pData->iLength;
		Mutex_Unlock(sg_sizeMutex);
        free(l_pData);
    }
}


static void
BlockPool_DestroyItemData(struct BlockPoolItem *p_pItem)
{
    if (p_pItem->pEvent != NULL)
        Event_Destroy(p_pItem->pEvent);

    BlockPool_DestroyItemDataList(p_pItem);
    Mutex_Destroy(p_pItem->mutex);
    free(p_pItem);
}

SO_PUBLIC bool 
BlockPool_DestroyItem (struct BlockPoolItem *p_pItem)
{
    ASSERT(p_pItem != NULL);
    if (p_pItem == NULL)
        return false;

    List_Remove(sg_bpList, p_pItem);

    return true;
}

void
BlockPool_ForEachItem(int (*function) (struct BlockPoolItem *, void *), void *userData)
{
    List_ForEach(sg_bpList, (int (*)(void *, void *))function, userData);
}

void 
BlockPool_SetStatus(struct BlockPoolItem *p_pItem, uint32_t p_iStatus)
{
    // Set the status bits retaining the flags bits
    p_pItem->iStatus = (p_iStatus & BLOCK_POOL_STATUS_MASK ) |
        ( p_pItem->iStatus & BLOCK_POOL_FLAG_MASK);
}
uint32_t 
BlockPool_GetStatus(struct BlockPoolItem *p_pItem)
{
    // Set the status bits retaining the flags bits
    return (p_pItem->iStatus & BLOCK_POOL_STATUS_MASK );
}
void 
BlockPool_SetFlags(struct BlockPoolItem *p_pItem, uint32_t p_iFlags)
{
    p_pItem->iStatus = ( p_iFlags & BLOCK_POOL_FLAG_MASK ) |
        ( p_pItem->iStatus & BLOCK_POOL_STATUS_MASK);
}

static void BlockPool_Delete(void *a)
{
    BlockPool_DestroyItemData(a);
}

void BlockPool_Item_Lock(void *a)
{
    Mutex_Lock(((struct BlockPoolItem *)a)->mutex);
}
void BlockPool_Item_Unlock(void *a)
{
    Mutex_Unlock(((struct BlockPoolItem *)a)->mutex);
}

