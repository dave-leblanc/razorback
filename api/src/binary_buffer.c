#include "config.h"
#include <razorback/api.h>
#include <razorback/block.h>
#include <razorback/debug.h>
#include <razorback/log.h>
#include <razorback/hash.h>
#include <razorback/event.h>
#include <razorback/judgment.h>
#include <razorback/thread.h>
#include <razorback/uuids.h>
#include <razorback/string_list.h>

#ifdef HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#ifdef _MSC_VER
#include <WinSock.h>
#else //_MSC_VER
#include <arpa/inet.h>
#endif //_MSC_VER

#include <stdlib.h>
#include <string.h>

#include "runtime_config.h"
#include "binary_buffer.h"

#include <stdio.h>

struct BinaryBuffer *
BinaryBuffer_CreateFromMessage (struct Message *message)
{
    struct BinaryBuffer *buf;
    ASSERT(message != NULL);
    if (message == NULL)
        return NULL;

    if ((buf = calloc (1, sizeof (struct BinaryBuffer))) == NULL)
        return NULL;

    buf->pBuffer = message->serialized;
    buf->iLength = message->length;
	
    return buf;
}

struct BinaryBuffer *
BinaryBuffer_Create (uint32_t p_iSize)
{
    struct BinaryBuffer *l_pBuffer;
    if ((l_pBuffer = calloc (1, sizeof (struct BinaryBuffer))) == NULL)
    {
        rzb_perror ("BinaryBuffer_Create: calloc failure: %s");
        return NULL;
    }


    if (p_iSize > (uint32_t) Config_getMaxBlockSize ())
        return NULL;


    l_pBuffer->iLength = p_iSize;

    // alocate the buffer
    if ((l_pBuffer->pBuffer = calloc (p_iSize, sizeof (uint8_t))) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to lack of memory", __func__);
        free (l_pBuffer);
        return NULL;
    }

    // set the offset
    l_pBuffer->iOffset = 0;

    // done
    return l_pBuffer;
}

void
BinaryBuffer_Destroy (struct BinaryBuffer *p_pBuffer)
{
    ASSERT (p_pBuffer != NULL);

    // free the heap memory
    if (p_pBuffer->pBuffer != NULL)
        free (p_pBuffer->pBuffer);

    free (p_pBuffer);
}

bool
BinaryBuffer_Put_uint8_t (struct BinaryBuffer *p_pBuffer, uint8_t p_iValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint32_t) sizeof (uint8_t) > p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }

    // place in the buffer in network order
    p_pBuffer->pBuffer[p_pBuffer->iOffset] = p_iValue;

    // update length
    p_pBuffer->iOffset += (uint32_t) sizeof (uint8_t);

    // done
    return true;
}

bool
BinaryBuffer_Put_uint16_t (struct BinaryBuffer * p_pBuffer, uint16_t p_iValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint32_t) sizeof (uint16_t) >
        p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }

    // place in the buffer in network order
    p_iValue = htobe16 (p_iValue);
    memcpy(&(p_pBuffer->pBuffer[p_pBuffer->iOffset]), &p_iValue, sizeof(uint16_t));

    // update length
    p_pBuffer->iOffset += (uint32_t) sizeof (uint16_t);

    // done
    return true;
}

bool
BinaryBuffer_Put_uint32_t (struct BinaryBuffer * p_pBuffer, uint32_t p_iValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint32_t) sizeof (uint32_t) >
        p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // place in the buffer in network order
    p_iValue = htobe32 (p_iValue);
    memcpy(&(p_pBuffer->pBuffer[p_pBuffer->iOffset]), &p_iValue, sizeof(uint32_t));

    // update length
    p_pBuffer->iOffset += (uint32_t) sizeof (uint32_t);

    // done
    return true;
}

bool
BinaryBuffer_Put_uint64_t (struct BinaryBuffer * p_pBuffer, uint64_t p_iValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint64_t) sizeof (uint64_t) >
        p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // place in the buffer in network order
    p_iValue = htobe64 (p_iValue);
    memcpy(&(p_pBuffer->pBuffer[p_pBuffer->iOffset]), &p_iValue, sizeof(uint64_t));

    // update length
    p_pBuffer->iOffset += (uint64_t) sizeof (uint64_t);

    // done
    return true;
}

bool
BinaryBuffer_Put_ByteArray (struct BinaryBuffer * p_pBuffer,
                            uint32_t p_iSize, const uint8_t * p_pByteArray)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_pByteArray != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + p_iSize > p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // place in the buffer in network order
    memcpy (((uint8_t *) & p_pBuffer->pBuffer[p_pBuffer->iOffset]),
            p_pByteArray, p_iSize);

    // update length
    p_pBuffer->iOffset += p_iSize;

    // done
    return true;
}

bool
BinaryBuffer_Put_String (struct BinaryBuffer * p_pBB,
                         const uint8_t * p_sString)
{
	uint32_t l_iSize;

    ASSERT (p_pBB != NULL);
    ASSERT (p_pBB->pBuffer != NULL);
    ASSERT (p_sString != NULL);

    // determine the size
    l_iSize = (uint32_t) (strlen ((char *) p_sString) + 1) * sizeof (uint8_t);

    return BinaryBuffer_Put_ByteArray (p_pBB, l_iSize, p_sString);
}


bool
BinaryBuffer_Get_uint8_t (struct BinaryBuffer * p_pBuffer, uint8_t * p_pValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_pValue != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint32_t) sizeof (uint8_t) > p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }

    // read the value
    *p_pValue = p_pBuffer->pBuffer[p_pBuffer->iOffset];

    // adjust the offset
    p_pBuffer->iOffset += (uint32_t) sizeof (uint8_t);

    // done
    return true;
}

bool
BinaryBuffer_Get_uint16_t (struct BinaryBuffer * p_pBuffer,
                           uint16_t * p_pValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_pValue != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint32_t) sizeof (uint16_t) >
        p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // read the value
//    *p_pValue =
//        be16toh (*((uint16_t *) & p_pBuffer->pBuffer[p_pBuffer->iOffset]));
    memcpy(p_pValue, &(p_pBuffer->pBuffer[p_pBuffer->iOffset]), sizeof(uint16_t));
    *p_pValue = be16toh(*p_pValue);
    // adjust the offset
    p_pBuffer->iOffset += (uint32_t) sizeof (uint16_t);

    // done
    return true;
}

bool
BinaryBuffer_Get_uint32_t (struct BinaryBuffer * p_pBuffer,
                           uint32_t * p_pValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_pValue != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint32_t) sizeof (uint32_t) >
        p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // read the value
    memcpy(p_pValue, &(p_pBuffer->pBuffer[p_pBuffer->iOffset]), sizeof(uint32_t));
    *p_pValue = be32toh(*p_pValue);

    // adjust the offset
    p_pBuffer->iOffset += (uint32_t) sizeof (uint32_t);

    // done
    return true;
}

bool
BinaryBuffer_Get_uint64_t (struct BinaryBuffer * p_pBuffer,
                           uint64_t * p_pValue)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_pValue != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + (uint64_t) sizeof (uint64_t) >
        p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // read the value
    memcpy(p_pValue, &(p_pBuffer->pBuffer[p_pBuffer->iOffset]), sizeof(uint64_t));
    *p_pValue = be64toh(*p_pValue);

    // adjust the offset
    p_pBuffer->iOffset += (uint64_t) sizeof (uint64_t);

    // done
    return true;
}

uint8_t *
BinaryBuffer_Get_String (struct BinaryBuffer * p_pBuffer)
{
	uint32_t l_iBytesLeft = p_pBuffer->iLength - p_pBuffer->iOffset;
    uint32_t l_iSize;
	uint8_t *pString;

    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);

	l_iSize = strnlen ((char *)&p_pBuffer->pBuffer[p_pBuffer->iOffset], 
			           l_iBytesLeft);

	if (l_iSize == 0)
	{
		rzb_log (LOG_ERR,
			     "%s: failed due to empty string", __func__);
		return NULL;
	}

	if (l_iSize == l_iBytesLeft)
	{
        rzb_log (LOG_ERR,
                 "%s: failed due to buffer overrun", __func__);
		return NULL;
	}

	if ((pString = (uint8_t *)calloc(l_iSize+1, sizeof(uint8_t))) == NULL) 
	{
		rzb_log (LOG_ERR,
				"%s: could not allocate memory", __func__);
        return NULL;
	}

	if (!BinaryBuffer_Get_ByteArray(p_pBuffer, l_iSize+1, pString))
	{
        rzb_log (LOG_ERR,
                "%s: failed due to failed of BinaryBuffer_Get_ByteArray", __func__);
		free(pString);
		return NULL;
	}

    return pString;
}

bool
BinaryBuffer_Get_ByteArray (struct BinaryBuffer * p_pBuffer,
                            uint32_t p_iSize, uint8_t * p_pByteArray)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_pByteArray != NULL);

    // check that space exists
    if (p_pBuffer->iOffset + p_iSize > p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR, "%s: failed due to overrun", __func__);
        return false;
    }
    // read the value
    memcpy (p_pByteArray, &p_pBuffer->pBuffer[p_pBuffer->iOffset], p_iSize);

    // adjust the offset
    p_pBuffer->iOffset += p_iSize;

    // done
    return true;
}

bool
BinaryBuffer_Get_UUID (struct BinaryBuffer * p_pBuffer, uuid_t p_uuid)
{
    uint32_t l_iIndex;
	
	ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_uuid != NULL);

    for (l_iIndex = 0; l_iIndex < 16; l_iIndex++)
    {
        if (!BinaryBuffer_Get_uint8_t (p_pBuffer, &p_uuid[l_iIndex]))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of BinaryBuffer_Get_uint8_t", __func__);
            return false;
        }
    }
    return true;
}

bool
BinaryBuffer_Put_UUID (struct BinaryBuffer * p_pBuffer, uuid_t p_uuid)
{
	uint32_t l_iIndex;

    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBuffer->pBuffer != NULL);
    ASSERT (p_uuid != NULL);

    for (l_iIndex = 0; l_iIndex < 16; l_iIndex++)
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, p_uuid[l_iIndex]))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of BinaryBuffer_Put_uint8_t", __func__);
            return false;
        }
    }
    return true;
}

static int
BinaryBuffer_Put_NTLVItem (struct NTLVItem *p_pItem,struct BinaryBuffer *p_pBuffer)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pItem != NULL);

    if (!BinaryBuffer_Put_UUID (p_pBuffer, p_pItem->uuidName))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_UUID", __func__);
        return LIST_EACH_ERROR;
    }
    if (!BinaryBuffer_Put_UUID (p_pBuffer, p_pItem->uuidType))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_UUID", __func__);
        return LIST_EACH_ERROR;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pItem->iLength))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_uint32_t", __func__);
        return LIST_EACH_ERROR;
    }
    if (!BinaryBuffer_Put_ByteArray
        (p_pBuffer, p_pItem->iLength, p_pItem->pData))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_ByteArray", __func__);
        return LIST_EACH_ERROR;
    }

    return LIST_EACH_OK;
}

bool
BinaryBuffer_Put_NTLVList (struct BinaryBuffer * p_pBuffer,
                           struct List * p_pList)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pList != NULL);

    if (p_pBuffer->iOffset + NTLVList_Size (p_pList) > p_pBuffer->iLength)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to lack overrun", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, List_Length (p_pList)))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_uint32_t", __func__);
        return false;
    }
    if (!List_ForEach(p_pList, (int (*)(void*,void*))BinaryBuffer_Put_NTLVItem, p_pBuffer))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_NTLVItem", __func__);
        return false;
    }
    return true;
}

bool
BinaryBuffer_Get_NTLVList (struct BinaryBuffer * p_pBuffer,
                           struct List ** p_pList)
{
    struct List * l_pList;
    uint32_t l_iCount;
    uint32_t l_iIndex;
    uuid_t l_uuiNameTemp;
    uuid_t l_uuiTypeTemp;
    uint32_t l_iSizeTemp;
    uint8_t *l_pDataTemp;

    ASSERT (p_pBuffer != NULL);

    // clear the list
    if ((l_pList = NTLVList_Create()) == NULL)
    {
        *p_pList = NULL;
        return false;
    }
    // get the count
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_iCount))
    {
        List_Destroy (l_pList);
        *p_pList = NULL;
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_uint32_t", __func__);
        return false;
    }

    // get each entry
    for (l_iIndex = 0; l_iIndex < l_iCount; l_iIndex++)
    {
        // get each field
        if (!BinaryBuffer_Get_UUID (p_pBuffer, l_uuiNameTemp))
        {
            List_Destroy (l_pList);
            *p_pList = NULL;
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Get_UUID", __func__);
            return false;
        }
        if (!BinaryBuffer_Get_UUID (p_pBuffer, l_uuiTypeTemp))
        {
            List_Destroy (l_pList);
            *p_pList = NULL;
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Get_UUID", __func__);
            return false;
        }
        if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_iSizeTemp))
        {
            List_Destroy (l_pList);
            *p_pList = NULL;
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Get_uint32_t", __func__);
            return false;
        }
        l_pDataTemp = (uint8_t *)calloc (l_iSizeTemp, sizeof (uint8_t));
        if (l_pDataTemp == NULL)
        {
            List_Destroy (l_pList);
            *p_pList = NULL;
            rzb_log (LOG_ERR,
                     "%s: failed due to lack of memory", __func__);
            return false;
        }
        if (!BinaryBuffer_Get_ByteArray (p_pBuffer, l_iSizeTemp, l_pDataTemp))
        {
            free (l_pDataTemp);
            List_Destroy (l_pList);
            *p_pList = NULL;
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Get_ByteArray", __func__);
            return false;
        }
        if (!NTLVList_Add
            (l_pList, l_uuiNameTemp, l_uuiTypeTemp, l_iSizeTemp, l_pDataTemp))
        {
            free (l_pDataTemp);
            List_Destroy (l_pList);
            *p_pList = NULL;
            rzb_log (LOG_ERR,
                     "%s: failed due failure of List_Add", __func__);
            return false;
        }
        free (l_pDataTemp);
    }
    *p_pList = l_pList;
    return true;
}

bool
BinaryBuffer_Put_Hash (struct BinaryBuffer * p_pBuffer,
                       const struct Hash * p_pHash)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pHash != NULL);
    ASSERT (p_pHash->iFlags & HASH_FLAG_FINAL);
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pHash->iType))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_uint32_t", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pHash->iSize))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_uint32_t", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_ByteArray (p_pBuffer, p_pHash->iSize,
                                     p_pHash->pData))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_ByteArray", __func__);
        return false;
    }

    return true;
}

bool
BinaryBuffer_Get_Hash (struct BinaryBuffer * p_pBuffer, struct Hash ** p_pHash)
{
    struct Hash *l_pHash;

	ASSERT (p_pBuffer != NULL);

    if ((l_pHash = (struct Hash *)calloc(1, sizeof (struct Hash))) == NULL)
    {
        *p_pHash = NULL;
        return false;
    }

    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pHash->iType))
    {
        Hash_Destroy(l_pHash);
        *p_pHash = NULL;
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_uint32_t", __func__);
        return false;
    }

    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pHash->iSize))
    {
        Hash_Destroy(l_pHash);
        *p_pHash = NULL;
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_uint32_t", __func__);
        return false;
    }
    if ((l_pHash->pData = (uint8_t *)calloc (l_pHash->iSize, sizeof (uint8_t))) == NULL)
    {
        Hash_Destroy(l_pHash);
        *p_pHash = NULL;
        rzb_log (LOG_ERR, "%s: failed due lack of memory", __func__);
        return false;
    }

    if (!BinaryBuffer_Get_ByteArray
        (p_pBuffer, l_pHash->iSize, l_pHash->pData))
    {
        Hash_Destroy(l_pHash);
        *p_pHash = NULL;
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_ByteArray", __func__);
        return false;
    }
    l_pHash->iFlags = HASH_FLAG_FINAL;
    *p_pHash = l_pHash;

    return true;
}



bool
BinaryBuffer_Put_BlockId (struct BinaryBuffer * p_pBuffer,
                          struct BlockId * p_pId)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pId != NULL);

    if (!BinaryBuffer_Put_UUID (p_pBuffer, p_pId->uuidDataType))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_UUID", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint64_t (p_pBuffer, p_pId->iLength))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_uint64_t", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_Hash (p_pBuffer, p_pId->pHash))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_Hash", __func__);
        return false;
    }
    return true;
}

bool
BinaryBuffer_Get_BlockId (struct BinaryBuffer * p_pBuffer,
                          struct BlockId ** p_pId)
{
    struct BlockId * l_pId;

	ASSERT (p_pBuffer != NULL);

    if ((l_pId = (struct BlockId *)calloc(1, sizeof(struct BlockId))) == NULL)
    {
        return false;
    }
    if (!BinaryBuffer_Get_UUID (p_pBuffer, l_pId->uuidDataType))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_UUID", __func__);
        return false;
    }
    if (!BinaryBuffer_Get_uint64_t (p_pBuffer, &l_pId->iLength))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_uint64_t", __func__);
        return false;
    }
    if (!BinaryBuffer_Get_Hash (p_pBuffer, &l_pId->pHash))
    {
        free (l_pId->pHash);
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_Hash", __func__);
        return false;
    }
    *p_pId = l_pId;
    return true;
}

bool
BinaryBuffer_Put_Block (struct BinaryBuffer * p_pBuffer,
                        struct Block * p_pBlock)
{
    ASSERT (p_pBuffer != NULL);
    ASSERT (p_pBlock != NULL);
    if (!BinaryBuffer_Put_BlockId (p_pBuffer, p_pBlock->pId))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_BlockId", __func__);
        return false;
    }

    if (p_pBlock->pParentId == NULL)
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, 0))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Put_uint8_t", __func__);
            return false;
        }
    }
    else
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, 1))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Put_uint8_t", __func__);
            return false;
        }
        if (!BinaryBuffer_Put_BlockId (p_pBuffer, p_pBlock->pParentId))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Put_BlockId", __func__);
            return false;
        }
    }

    if (!BinaryBuffer_Put_NTLVList (p_pBuffer, p_pBlock->pMetaDataList))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Put_NTLVList", __func__);
        return false;
    }


    return true;
}


bool
BinaryBuffer_Get_Block (struct BinaryBuffer * p_pBuffer,
                        struct Block ** p_pBlock)
{
    uint8_t l_iHas = 0;
    struct Block * l_pBlock;

	ASSERT (p_pBuffer != NULL);

    if ((l_pBlock = (struct Block *)calloc(1, sizeof(struct Block))) == NULL)
    {
        *p_pBlock = NULL;
        return false;
    }

    if (!BinaryBuffer_Get_BlockId (p_pBuffer, &l_pBlock->pId))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_BlockId", __func__);
        Block_Destroy(l_pBlock);
        *p_pBlock = NULL;
        return false;
    }

    if (!BinaryBuffer_Get_uint8_t (p_pBuffer, &l_iHas))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_uint8_t", __func__);
        Block_Destroy(l_pBlock);
        *p_pBlock = NULL;
        return false;
    }

    if (l_iHas == 1)
    {
        if (!BinaryBuffer_Get_BlockId (p_pBuffer, &l_pBlock->pParentId))
        {
            rzb_log (LOG_ERR,
                     "%s: failed due failure of BinaryBuffer_Get_BlockId", __func__);
            Block_Destroy(l_pBlock);
            *p_pBlock = NULL;
            return false;
        }
    }
    else
        l_pBlock->pParentId = NULL;


    if (!BinaryBuffer_Get_NTLVList (p_pBuffer, &l_pBlock->pMetaDataList))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due failure of BinaryBuffer_Get_NTLVList", __func__);
        Block_Destroy(l_pBlock);
        *p_pBlock = NULL;
        return false;
    }


    *p_pBlock = l_pBlock;
    return true;
}

bool
BinaryBuffer_Put_EventId (struct BinaryBuffer *p_pBuffer,
                                    struct EventId *p_pEventId)
{
    if (!BinaryBuffer_Put_UUID (p_pBuffer, p_pEventId->uuidNuggetId))
    {
        rzb_log(LOG_ERR, "%s: Failed to put nugget id", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_uint64_t (p_pBuffer, p_pEventId->iSeconds))
    {
        rzb_log(LOG_ERR, "%s: Failed to put seconds", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint64_t (p_pBuffer, p_pEventId->iNanoSecs))
    {
        rzb_log(LOG_ERR, "%s: Failed to put nano seconds", __func__);
        return false;
    }

    return true;
}

bool
BinaryBuffer_Get_EventId (struct BinaryBuffer *p_pBuffer,
                                    struct EventId **p_pEventId)
{
    struct EventId *eventId = NULL;
    if ((eventId = EventId_Create()) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to create event", __func__);
        return false;
    }

    if (!BinaryBuffer_Get_UUID (p_pBuffer, eventId->uuidNuggetId))
    {
        rzb_log(LOG_ERR, "%s: Failed to get nugget id", __func__);
        return false;
    }

    if (!BinaryBuffer_Get_uint64_t (p_pBuffer, &eventId->iSeconds))
    {
        rzb_log(LOG_ERR, "%s: Failed to get seconds", __func__);
        return false;
    }
    if (!BinaryBuffer_Get_uint64_t (p_pBuffer, &eventId->iNanoSecs))
    {
        rzb_log(LOG_ERR, "%s: Failed to get nano seconds", __func__);
        return false;
    }
    *p_pEventId = eventId;
    return true;
}

bool 
BinaryBuffer_Put_Event (struct BinaryBuffer *p_pBuffer,
                                    struct Event *p_pEvent)
{
    if (!BinaryBuffer_Put_EventId (p_pBuffer, p_pEvent->pId))
    {
        rzb_log(LOG_ERR, "%s: Failed to put nugget id", __func__);
        return false;
    }
    if (p_pEvent->pParentId == NULL)
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, 0))
        {
            rzb_log(LOG_ERR, "%s: Failed to put event id marker", __func__);
            return false;
        }
    }
    else 
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, 1))
        {
            rzb_log(LOG_ERR, "%s: Failed to put event id marker", __func__);
            return false;
        }
        if (!BinaryBuffer_Put_EventId (p_pBuffer, p_pEvent->pParentId))
        {
            rzb_log(LOG_ERR, "%s: Failed to put parent event id", __func__);
            return false;
        }
    }

    if (!BinaryBuffer_Put_UUID (p_pBuffer, p_pEvent->uuidApplicationType))
    {
        rzb_log(LOG_ERR, "%s: Failed to put app type", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_NTLVList (p_pBuffer, p_pEvent->pMetaDataList))
    {
        rzb_log(LOG_ERR, "%s: Failed to put metadata list", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_Block (p_pBuffer, p_pEvent->pBlock))
    {
        rzb_log(LOG_ERR, "%s: Failed to put block", __func__);
        return false;
    }
    return true;
}

bool 
BinaryBuffer_Get_Event (struct BinaryBuffer *p_pBuffer,
                                    struct Event **p_pEvent)
{
    struct Event *l_pEvent;
    uint8_t l_iHas;

    if ((l_pEvent = calloc(1, sizeof (struct Event))) == NULL)
    {
        *p_pEvent = NULL;
        return false;
    }

    if (!BinaryBuffer_Get_EventId (p_pBuffer, &l_pEvent->pId))
    {
        rzb_log(LOG_ERR, "%s: Failed to get event id", __func__);
        Event_Destroy(l_pEvent);
        *p_pEvent = NULL;
        return false;
    }

    if (!BinaryBuffer_Get_uint8_t (p_pBuffer, &l_iHas))
    {
        rzb_log(LOG_ERR, "%s: Failed to get event id marker", __func__);
        Event_Destroy(l_pEvent);
        *p_pEvent = NULL;
        return false;
    }
    if (l_iHas == 1)
    {
        if (!BinaryBuffer_Get_EventId (p_pBuffer, &l_pEvent->pParentId))
        {
            rzb_log(LOG_ERR, "%s: Failed to get message", __func__);
            Event_Destroy(l_pEvent);
            *p_pEvent = NULL;
            return false;
        }
    }

    if (!BinaryBuffer_Get_UUID (p_pBuffer, l_pEvent->uuidApplicationType))
    {
        rzb_log(LOG_ERR, "%s: Failed to get app type", __func__);
        Event_Destroy(l_pEvent);
        *p_pEvent = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_NTLVList (p_pBuffer, &l_pEvent->pMetaDataList))
    {
        rzb_log(LOG_ERR, "%s: Failed to get metadata list", __func__);
        Event_Destroy(l_pEvent);
        *p_pEvent = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_Block (p_pBuffer, &l_pEvent->pBlock))
    {
        rzb_log(LOG_ERR, "%s: Failed to get block", __func__);
        Event_Destroy(l_pEvent);
        *p_pEvent = NULL;
        return false;
    }
    *p_pEvent = l_pEvent;
    return true;
}

bool 
BinaryBuffer_Put_Judgment (struct BinaryBuffer *p_pBuffer,
                                    struct Judgment *p_pJudgment)
{
    if (!BinaryBuffer_Put_UUID (p_pBuffer, p_pJudgment->uuidNuggetId))
    {
        rzb_log(LOG_ERR, "%s: Failed to put nugget id", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint64_t (p_pBuffer, p_pJudgment->iSeconds))
    {
        rzb_log(LOG_ERR, "%s: Failed to put seconds", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint64_t (p_pBuffer, p_pJudgment->iNanoSecs))
    {
        rzb_log(LOG_ERR, "%s: Failed to put nano seconds", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_EventId (p_pBuffer, p_pJudgment->pEventId))
    {
        rzb_log(LOG_ERR, "%s: Failed to put event id", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_BlockId (p_pBuffer, p_pJudgment->pBlockId))
    {
        rzb_log(LOG_ERR, "%s: Failed to put block id", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_uint8_t (p_pBuffer, p_pJudgment->iPriority))
    {
        rzb_log(LOG_ERR, "%s: Failed to put priority type", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_NTLVList (p_pBuffer, p_pJudgment->pMetaDataList))
    {
        rzb_log(LOG_ERR, "%s: Failed to put metadata list", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pJudgment->iGID))
    {
        rzb_log(LOG_ERR, "%s: Failed to put GID", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pJudgment->iSID))
    {
        rzb_log(LOG_ERR, "%s: Failed to put SID", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pJudgment->Set_SfFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to put set sf flags", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pJudgment->Set_EntFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to put set ent flags", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pJudgment->Unset_SfFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to put unset sf flags", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (p_pBuffer, p_pJudgment->Unset_EntFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to put  unset ent flags", __func__);
        return false;
    }
    if (p_pJudgment->sMessage == NULL)
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, 0))
        {
            rzb_log(LOG_ERR, "%s: Failed to put priority type", __func__);
            return false;
        }
    }
    else 
    {
        if (!BinaryBuffer_Put_uint8_t (p_pBuffer, 1))
        {
            rzb_log(LOG_ERR, "%s: Failed to put priority type", __func__);
            return false;
        }
        if (!BinaryBuffer_Put_String (p_pBuffer, p_pJudgment->sMessage))
        {
            rzb_log(LOG_ERR, "%s: Failed to put message", __func__);
            return false;
        }
    }
    return true;
}

bool 
BinaryBuffer_Get_Judgment (struct BinaryBuffer *p_pBuffer,
                                    struct Judgment **p_pJudgment)
{
    struct Judgment *l_pJudgment;
    uint8_t l_iHas = 0;
    if ((l_pJudgment = calloc(1, sizeof (struct Judgment))) == NULL)
    {
        *p_pJudgment = NULL;
        return false;
    }

    if (!BinaryBuffer_Get_UUID (p_pBuffer, l_pJudgment->uuidNuggetId))
    {
        rzb_log(LOG_ERR, "%s: Failed to get Nugget Id", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint64_t (p_pBuffer, &l_pJudgment->iSeconds))
    {
        rzb_log(LOG_ERR, "%s: Failed to get seconds", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint64_t (p_pBuffer, &l_pJudgment->iNanoSecs))
    {
        rzb_log(LOG_ERR, "%s: Failed to get nano seconds", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_EventId (p_pBuffer, &l_pJudgment->pEventId))
    {
        rzb_log(LOG_ERR, "%s: Failed to get event id", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_BlockId (p_pBuffer, &l_pJudgment->pBlockId))
    {
        rzb_log(LOG_ERR, "%s: Failed to get block id", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint8_t (p_pBuffer, &l_pJudgment->iPriority))
    {
        rzb_log(LOG_ERR, "%s: Failed to get priority", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_NTLVList (p_pBuffer, &l_pJudgment->pMetaDataList))
    {
        rzb_log(LOG_ERR, "%s: Failed to get metadata list", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pJudgment->iGID))
    {
        rzb_log(LOG_ERR, "%s: Failed to get GID", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pJudgment->iSID))
    {
        rzb_log(LOG_ERR, "%s: Failed to get SID", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pJudgment->Set_SfFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to get set sf flags", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pJudgment->Set_EntFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to get set ent flags", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pJudgment->Unset_SfFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to get unset sf flags", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (p_pBuffer, &l_pJudgment->Unset_EntFlags))
    {
        rzb_log(LOG_ERR, "%s: Failed to get unset sf flags", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (!BinaryBuffer_Get_uint8_t (p_pBuffer, &l_iHas))
    {
        rzb_log(LOG_ERR, "%s: Failed to get unset sf flags", __func__);
        Judgment_Destroy(l_pJudgment);
        *p_pJudgment = NULL;
        return false;
    }
    if (l_iHas == 1)
    {
        if ((l_pJudgment->sMessage =BinaryBuffer_Get_String (p_pBuffer)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to get message", __func__);
            Judgment_Destroy(l_pJudgment);
            *p_pJudgment = NULL;
            return false;
        }
    }
    
    *p_pJudgment = l_pJudgment;
    return true;
}

bool
BinaryBuffer_Get_UUIDList(struct BinaryBuffer *buffer, struct List **r_list)
{
    struct List *list;
    uint32_t count;
    uuid_t *uuids = NULL;
    uint32_t namesSize;
    char *names = NULL;
    char *curStr;
    uuid_t *curUuid;
    uint32_t pos = 0;
 
    if ((list = UUID_Create_List()) == NULL)
        return false;
    if (!BinaryBuffer_Get_uint32_t(buffer, &count))
    {
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Type Count", __func__);
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t(buffer, &namesSize))
    {
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Type name size", __func__);
        return false;
    }
    // Remove the UUIDs
    namesSize -= (count*16);
    if (count > 0)
    {
        uuids = calloc(count, sizeof(uuid_t));
        names = calloc(namesSize, sizeof(char));
        if ((uuids == NULL ) ||
                (names == NULL ) )
        {
            free(names);
            free(uuids);
            rzb_log(LOG_ERR, "%s: failed to allocate income message structures", __func__);
            return false;
        }
        if (!BinaryBuffer_Get_ByteArray(buffer, count * sizeof(uuid_t), (uint8_t*)uuids))
        {
            free(names);
            free(uuids);
            rzb_log(LOG_ERR, "%s: failed to read Types uuids", __func__);
            return false;
        }

        if (!BinaryBuffer_Get_ByteArray(buffer, namesSize * sizeof(char), (uint8_t*)names))
        {
            free(names);
            free(uuids);
            rzb_log(LOG_ERR, "%s: failed to read Types names", __func__);
            return false;
        }
        curStr = names;
        curUuid = uuids;
        for (pos =0; pos < count; pos++)
        {
            UUID_Add_List_Entry(list, *curUuid, curStr, "");
            curStr = curStr + strlen(curStr) +1;
            curUuid+=1;
        }
        free(names);
        free(uuids);
    }
    *r_list = list;
    return true;
}

struct UUID_ListStatus
{
    uuid_t *curUuid;
    char *curString;
};

static int
BinaryBuffer_UUIDList_AddData(void *vItem, void *vStatus)
{
    struct UUIDListNode *item = (struct UUIDListNode *)vItem;
    struct UUID_ListStatus *status = (struct UUID_ListStatus *)vStatus;

    memcpy(status->curString, item->sName, strlen(item->sName)+1);
    status->curString += strlen(item->sName)+1;
    uuid_copy(*status->curUuid, item->uuid);
    status->curUuid++;
    return LIST_EACH_OK;
}


bool
BinaryBuffer_Put_UUIDList(struct BinaryBuffer *buffer, struct List *list)
{
    uint8_t *data;
    uint32_t size = UUIDList_BinarySize(list);
    uint32_t count = List_Length(list);
    struct UUID_ListStatus listStatus;
    if ((data = calloc(size,sizeof(uint8_t))) == NULL)
        return false;
    
    if (!BinaryBuffer_Put_uint32_t(buffer, count))
    {
        rzb_log(LOG_ERR, "%s: Failed to put UUID Count", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t(buffer, size))
    {
        rzb_log(LOG_ERR, "%s: Failed to put UUID size", __func__);
        return false;
    }
    listStatus.curUuid = (uuid_t *)data;
    listStatus.curString = (char *)(data+(count*16));
    List_ForEach(list, BinaryBuffer_UUIDList_AddData, &listStatus);

    BinaryBuffer_Put_ByteArray(buffer, size, data);


    free(data);
    return true;
}

bool
BinaryBuffer_Get_StringList(struct BinaryBuffer *buffer, struct List **r_list)
{
    struct List *list;
    uint32_t count;
    uint32_t pos = 0;
    char *item;
 
    if ((list = StringList_Create()) == NULL)
        return false;
    if (!BinaryBuffer_Get_uint32_t(buffer, &count))
    {
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Type Count", __func__);
        return false;
    }
    for (pos = 0; pos < count; pos++)
    {
        item = (char *)BinaryBuffer_Get_String(buffer);
        StringList_Add(list, item);
        free(item);

    }
    return true;
}

static int 
BinaryBuffer_Put_StringListItem(void *vItem, void *vBuffer)
{
    struct BinaryBuffer *buffer = vBuffer;
    uint8_t *item = vItem;

    if (!BinaryBuffer_Put_String(buffer, item))
        return LIST_EACH_ERROR;

    return LIST_EACH_OK;
}

bool 
BinaryBuffer_Put_StringList(struct BinaryBuffer *buffer, struct List *list)
{
    uint32_t count = List_Length(list);
    
    if (!BinaryBuffer_Put_uint32_t(buffer, count))
    {
        rzb_log(LOG_ERR, "%s: Failed to put UUID Count", __func__);
        return false;
    }
    if (!List_ForEach(list, BinaryBuffer_Put_StringListItem, buffer))
        return false;

    return true;
}
