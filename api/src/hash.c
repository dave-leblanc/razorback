#include "config.h"
#include <razorback/debug.h>
#include <razorback/hash.h>
#include <razorback/log.h>
#include <stdio.h>
#include <string.h>

#include <openssl/evp.h>

#include "runtime_config.h"

SO_PUBLIC bool
Hash_IsEqual (const struct Hash *p_pHashA, const struct Hash *p_pHashB)
{
    ASSERT (p_pHashA != NULL);
    ASSERT (p_pHashB != NULL);
    ASSERT (p_pHashA->pData != NULL);
    ASSERT (p_pHashB->pData != NULL);
    ASSERT (p_pHashA->iFlags & HASH_FLAG_FINAL);
    ASSERT (p_pHashB->iFlags & HASH_FLAG_FINAL);

    if (!(p_pHashA->iFlags & HASH_FLAG_FINAL))
        return false;

    if (!(p_pHashB->iFlags & HASH_FLAG_FINAL))
        return false;

    if (p_pHashA == p_pHashB)
        return true;
    if (p_pHashA->iSize != p_pHashB->iSize)
        return false;
    return (memcmp (p_pHashA->pData, p_pHashB->pData, p_pHashA->iSize) == 0);
}

SO_PUBLIC char *
Hash_ToText (const struct Hash *p_pHash)
{
    char *l_sString = NULL;
    uint32_t l_iIndex;
    char *l_sTemp;

	ASSERT (p_pHash != NULL);
    ASSERT (p_pHash->iFlags & HASH_FLAG_FINAL);

    if ((l_sString = (char *)calloc((p_pHash->iSize * 2) + 1, sizeof(char *))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new string", __func__);
        return NULL;
    }
    
    l_sTemp = l_sString;

    for(l_iIndex=0; l_iIndex< p_pHash->iSize; l_iIndex++){
        snprintf(l_sTemp, 3, "%02x", p_pHash->pData[l_iIndex]);
        l_sTemp+=(char)2;
    }
    return l_sString;
}

SO_PUBLIC struct Hash *
Hash_Create (void)
{
    return Hash_Create_Type(Config_getHashType());
}

static bool
Hash_Init_OpenSSL(struct Hash *hash)
{
	const EVP_MD *m;
	hash->pData = (uint8_t *)calloc (EVP_MAX_MD_SIZE, sizeof (uint8_t));
    if (hash->pData == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to lack of memory", __func__);
        return false;
    }
    switch (hash->iType)
    {
    case HASH_TYPE_MD5:
        m = EVP_get_digestbyname("md5");
        break;
    case HASH_TYPE_SHA1:
        m = EVP_get_digestbyname("sha1");
        break;
    case HASH_TYPE_SHA224:
        m = EVP_get_digestbyname("sha224");
        break;
    case HASH_TYPE_SHA256:
        m = EVP_get_digestbyname("sha256");
        break;
    case HASH_TYPE_SHA512:
        m = EVP_get_digestbyname("sha512");
        break;
    default:
        m = NULL;
        rzb_log (LOG_ERR, "%s: failed due to invalid type", __func__);
        return false;
    }

    EVP_DigestInit(&hash->CTX, m);
	return true;
}

SO_PUBLIC struct Hash *
Hash_Create_Type (uint32_t p_iType)
{

    struct Hash *l_pHash;
    if ((l_pHash = (struct Hash *)calloc(1, sizeof(struct Hash))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate new hash", __func__);
        return NULL;
    }

    l_pHash->iFlags=0;
    l_pHash->iType = p_iType;

	if (!Hash_Init_OpenSSL(l_pHash))
	{
		Hash_Destroy(l_pHash);
		return NULL;
	}
    return l_pHash;
}

SO_PUBLIC bool
Hash_Update (struct Hash * p_pHash, uint8_t * p_pData, uint32_t p_iLength)
{
    ASSERT (p_pHash != NULL);
    ASSERT (p_pHash->pData != NULL);
    ASSERT (p_pHash->iType > 0);
    ASSERT (!(p_pHash->iFlags & HASH_FLAG_FINAL));
    EVP_DigestUpdate(&p_pHash->CTX, p_pData, p_iLength);
    return true;
}
SO_PUBLIC bool
Hash_Update_File (struct Hash * p_pHash, FILE *file)
{
    uint8_t data[4096];
    size_t len;
    ASSERT (p_pHash != NULL);
    ASSERT (p_pHash->pData != NULL);
    ASSERT (p_pHash->iType > 0);
    ASSERT (!(p_pHash->iFlags & HASH_FLAG_FINAL));
    while((len = fread(data,1,4096, file)) > 0)
    {
        EVP_DigestUpdate(&p_pHash->CTX, data, len);
    }
    rewind(file);
    return true;
}

SO_PUBLIC bool
Hash_Finalize (struct Hash * p_pHash)
{
    ASSERT (p_pHash != NULL);
    ASSERT (p_pHash->pData != NULL);
    ASSERT (p_pHash->iType > 0);
    ASSERT (!(p_pHash->iFlags & HASH_FLAG_FINAL));
    EVP_DigestFinal(&p_pHash->CTX, p_pHash->pData, &p_pHash->iSize);
    p_pHash->iFlags = p_pHash->iFlags | HASH_FLAG_FINAL;
    return true;
}

SO_PUBLIC uint32_t
Hash_BinaryLength (struct Hash *p_pHash)
{
    return p_pHash->iSize + (sizeof (uint32_t)*2);
}
SO_PUBLIC uint32_t
Hash_DigestLength (struct Hash *p_pHash)
{
    // TODO: return the real length
    return p_pHash->iSize;
//    return EVP_MAX_MD_SIZE;
}

SO_PUBLIC uint32_t
Hash_StringLength (struct Hash *p_pHash)
{
    return (p_pHash->iSize * 2) + 1;
}

SO_PUBLIC void
Hash_Destroy (struct Hash *p_pHash)
{
    if (p_pHash->pData != NULL)
        free (p_pHash->pData);

    EVP_MD_CTX_cleanup(&p_pHash->CTX);
    free(p_pHash);
}

SO_PUBLIC struct Hash *
Hash_Clone (const struct Hash *p_pSource)
{
    struct Hash *l_pDestination;
	
	ASSERT (p_pSource != NULL);

    if ((l_pDestination = (struct Hash *)calloc(1, sizeof (struct Hash))) == NULL)
    {
        return NULL;
    }
    if ((p_pSource->iFlags & HASH_FLAG_FINAL) != HASH_FLAG_FINAL)
    {
        rzb_log(LOG_ERR, "%s: Can not copy a non finalized hash", __func__);
        return NULL;
    }
    l_pDestination->iType = p_pSource->iType;
    l_pDestination->iSize = p_pSource->iSize;
    l_pDestination->iFlags = HASH_FLAG_FINAL;

    if ((l_pDestination->pData = (uint8_t *)malloc(p_pSource->iSize)) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: Failed to allocation new data: %i bytes", __func__, p_pSource->iSize);
        return NULL;
    }

    memcpy(l_pDestination->pData, p_pSource->pData, p_pSource->iSize);

    return l_pDestination;
}



