#include "config.h"
#include <razorback/debug.h>
#include <razorback/block_id.h>
#include <razorback/hash.h>
#include <razorback/log.h>

#include <string.h>
#include <stdio.h>

#include "runtime_config.h"

SO_PUBLIC bool
BlockId_IsEqual (const struct BlockId * p_pA, const struct BlockId * p_pB)
{
	bool uuid, hash, length;
    ASSERT (p_pA != NULL);
    ASSERT (p_pB != NULL);

    // check if pointers equal
    if (p_pA == p_pB)
        return true;

    uuid = (uuid_compare (p_pA->uuidDataType, p_pB->uuidDataType) == 0);
    hash = Hash_IsEqual (p_pA->pHash, p_pB->pHash);
    length = (p_pA->iLength == p_pB->iLength);
    return (uuid && hash && length);
}

SO_PUBLIC void
BlockId_ToText (const struct BlockId *p_pA, uint8_t * p_sText)
{
    char l_sUUID[UUID_STRING_LENGTH];
    char *l_sHash;

	ASSERT (p_pA != NULL);
    ASSERT (p_sText != NULL);

    // create the text string
    uuid_unparse (p_pA->uuidDataType, l_sUUID);
    l_sHash = Hash_ToText (p_pA->pHash);
    sprintf ((char *) p_sText, "%s-%8.8jx-%s", l_sUUID, p_pA->iLength,
             (char *) l_sHash);
    free(l_sHash);
}

struct BlockId *
BlockId_Create_Shallow (void)
{
    return calloc(1, sizeof(struct BlockId));
}

SO_PUBLIC struct BlockId *
BlockId_Create (void)
{
    struct BlockId *id;

    if ((id = BlockId_Create_Shallow()) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate block id", __func__);
        return NULL;
    }
    // intialize the hash
    if ((id->pHash = Hash_Create()) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to lack of memory: Hash_Create", __func__);
        return NULL;
    }
    return id;
}

SO_PUBLIC uint32_t
BlockId_StringLength (struct BlockId *p_pB)
{
    // return the value
    return UUID_STRING_LENGTH + Hash_StringLength (p_pB->pHash) + 9;  // "%s-%8.8x-%s"
}

SO_PUBLIC void
BlockId_Destroy (struct BlockId *p_pBlockId)
{
    if (p_pBlockId->pHash != NULL)
        Hash_Destroy (p_pBlockId->pHash);
    free(p_pBlockId);
}

SO_PUBLIC struct BlockId *
BlockId_Clone (const struct BlockId *p_pSource)
{
    struct BlockId *dest;
	ASSERT (p_pSource != NULL);   

    if ((dest = BlockId_Create_Shallow()) == NULL)
        return NULL;

    if ((dest->pHash = Hash_Clone (p_pSource->pHash)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to failure of Hash_Clone", __func__);
        return NULL;
    }

    uuid_copy (dest->uuidDataType, p_pSource->uuidDataType);
    dest->iLength = p_pSource->iLength;

    // done
    return dest;
}

SO_PUBLIC uint32_t
BlockId_BinaryLength (const struct BlockId * p_pBlockId)
{
    ASSERT (p_pBlockId != NULL);

    return Hash_BinaryLength (p_pBlockId->pHash) +
        (uint32_t) sizeof (p_pBlockId->uuidDataType) +
        (uint32_t) sizeof (p_pBlockId->iLength);
}
