#include "config.h"
#include <razorback/debug.h>
#include <razorback/block.h>
#include <razorback/block_id.h>
#include <razorback/ntlv.h>
#include <razorback/log.h>
#include <razorback/uuids.h>

#include <string.h>
#include <stdio.h>


SO_PUBLIC struct Block *
Block_Create (void)
{
    struct Block * l_pBlock;
    if ((l_pBlock = calloc (1, sizeof (struct Block))) == NULL )
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate memory for new Block", __func__);
        return NULL;
    }
    if ((l_pBlock->pId = BlockId_Create ()) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to create metadata list", __func__);
        free(l_pBlock);
        return NULL;
    }
    
    if ((l_pBlock->pMetaDataList = NTLVList_Create ()) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to create metadata list", __func__);
        free(l_pBlock);
        return NULL;
    }

    return l_pBlock;
}

SO_PUBLIC void
Block_Destroy (struct Block *p_pBlock)
{

    BlockId_Destroy (p_pBlock->pId);

    if (p_pBlock->pParentId != NULL)
    {
        BlockId_Destroy (p_pBlock->pParentId);
    }

    if (p_pBlock->pMetaDataList != NULL)
        List_Destroy (p_pBlock->pMetaDataList);

    free(p_pBlock);
}

SO_PUBLIC struct Block *
Block_Clone (const struct Block *p_pSource)
{
    struct Block *l_pDestination;

	ASSERT (p_pSource != NULL);

    if ((l_pDestination = calloc(1, sizeof (struct Block))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new block", __func__);
        Block_Destroy (l_pDestination);
        return NULL;
    }
    if ((l_pDestination->pId = BlockId_Clone (p_pSource->pId)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to clone block ID", __func__);
        Block_Destroy (l_pDestination);
        return NULL;
    }

    if (p_pSource->pParentId == NULL)
        l_pDestination->pParentId = NULL;
    else
    {
        if ((l_pDestination->pParentId = BlockId_Clone (p_pSource->pParentId)) == NULL)
        {
            rzb_log (LOG_ERR, "%s: failed due to lack of memory", __func__);
            Block_Destroy (l_pDestination);
            return NULL;
        }
    }

    l_pDestination->pMetaDataList = List_Clone(p_pSource->pMetaDataList);
    if (l_pDestination->pMetaDataList == NULL )
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of List_Clone", __func__);
        Block_Destroy (l_pDestination);
        return NULL;
    }

    return l_pDestination;
}

SO_PUBLIC uint32_t
Block_BinaryLength (struct Block * p_pBlock)
{
    uint32_t l_iSize = 0;
    l_iSize += BlockId_BinaryLength (p_pBlock->pId);
    l_iSize += sizeof (uint8_t);    // 0 or 1: 1 if parent ID present
    if (p_pBlock->pParentId != NULL)
        l_iSize += BlockId_BinaryLength (p_pBlock->pParentId);
    l_iSize += NTLVList_Size (p_pBlock->pMetaDataList);
	l_iSize += sizeof (uint16_t);

    return l_iSize;
}

SO_PUBLIC bool
Block_MetaData_Add(struct Block *block, uuid_t uuidName, uuid_t uuidType, uint8_t *data, uint32_t size)
{
    return NTLVList_Add(block->pMetaDataList, uuidName, uuidType, size, data);
}

SO_PUBLIC bool
Block_MetaData_Add_FileName(struct Block *block, const char * fileName)
{
    uuid_t uuidName;
    uuid_t uuidType;
    UUID_Get_UUID(NTLV_NAME_FILENAME, UUID_TYPE_NTLV_NAME, uuidName);
    UUID_Get_UUID(NTLV_TYPE_STRING, UUID_TYPE_NTLV_TYPE, uuidType);
    if (uuidName == NULL || uuidType == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to lookup uuids", __func__);
        return false;
    }

    return Block_MetaData_Add(block, uuidName, uuidType, (uint8_t *)fileName, strlen(fileName));
}

