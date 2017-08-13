#include "config.h"
#include <razorback/debug.h>
#include <razorback/hash.h>
#include <razorback/list.h>
#include <razorback/uuids.h>
#include <razorback/block_id.h>
#include <razorback/block.h>
#include <razorback/event.h>
#include <razorback/judgment.h>
#include <razorback/string_list.h>

#include <razorback/json_buffer.h>
#ifdef _MSC_VER
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "bobins.h"
#include <stdio.h>
#define NUM_FMT "%I64u"
#else //_MSC_VER
#include <arpa/inet.h>
#include <sys/socket.h>
#define NUM_FMT "%ju"
#endif //_MSC_VER


#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <string.h>

SO_PUBLIC bool JsonBuffer_Put_uint8_t (json_object * parent,
                                    const char *name, uint8_t p_iValue)
{
    json_object *new;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;

    if ((new = json_object_new_int(p_iValue)) == NULL)
        return false;

    json_object_object_add(parent, name, new);
    return true;
}
SO_PUBLIC bool JsonBuffer_Put_uint16_t (json_object * parent, const char * name, uint16_t p_iValue)
{
    json_object *new;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;

    if ((new = json_object_new_int(p_iValue)) == NULL)
        return false;

    json_object_object_add(parent, name, new);

    return true;
}

SO_PUBLIC bool JsonBuffer_Put_uint32_t (json_object * parent, const char * name, uint32_t p_iValue)
{
    json_object *new;
    char *str;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Larger numbers dont fit in the API so send them as strings
    if (asprintf(&str, "%u", p_iValue) == -1)
        return false;

    if ((new = json_object_new_string(str)) == NULL)
        return false;

    json_object_object_add(parent, name, new);
    free(str);
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_uint64_t (json_object * parent, const char * name, uint64_t p_iValue)
{
    json_object *new;
    char *str;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;

    // Larger numbers dont fit in the API so send them as strings
    if (asprintf(&str, NUM_FMT, p_iValue) == -1)
        return false;

    if ((new = json_object_new_string(str)) == NULL)
        return false;

    json_object_object_add(parent, name, new);
    free(str);
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_ByteArray (json_object * parent, const char *name, 
                                      uint32_t p_iSize,
                                      const uint8_t * p_pByteArray)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;
    char *buff;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bmem = BIO_new(BIO_s_mem());
    BIO_push(b64, bmem);
    BIO_write(b64, p_pByteArray, p_iSize);
    //BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    buff = (char *)malloc(bptr->length+1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = '\0';

    BIO_free_all(b64);

    if (!JsonBuffer_Put_String(parent, name, buff))
        return false;
    free(buff);
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_String (json_object * parent, const char * name,
                                   const char * p_sString)
{
    json_object *new;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;

    if ((new = json_object_new_string((const char *)p_sString)) == NULL)
        return false;

    json_object_object_add(parent, name, new);
    
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_uint8_t (json_object * parent, const char * name, uint8_t * p_pValue)
{
    int tmp;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name))  == NULL)
        return false;
    if (json_object_get_type(object) != json_type_int)
        return false;

    tmp = json_object_get_int(object);
    *p_pValue = (uint8_t) tmp;
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_uint16_t (json_object * parent, const char * name,
                                     uint16_t * p_pValue)
{
    int tmp;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name))  == NULL)
        return false;
    if (json_object_get_type(object) != json_type_int)
        return false;

    tmp = json_object_get_int(object);
    *p_pValue = (uint16_t) tmp;

    return true;
}

SO_PUBLIC bool JsonBuffer_Get_uint32_t (json_object * parent, const char *name,
                                     uint32_t * p_pValue)
{
    const char *tmp;
    uint32_t val;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name))  == NULL)
        return false;
    if (json_object_get_type(object) != json_type_string)
        return false;
    tmp = json_object_get_string(object);
    if (sscanf(tmp, "%u", &val) != 1)
        return false;
    *p_pValue = val;
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_uint64_t (json_object * parent, const char *name,
                                     uint64_t * p_pValue)
{
    const char *tmp;
    uint64_t val;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name))  == NULL)
        return false;
    if (json_object_get_type(object) != json_type_string)
        return false;
    tmp = json_object_get_string(object);
    if (sscanf(tmp, NUM_FMT, &val) != 1)
        return false;
    *p_pValue = val;

    return true;
}

SO_PUBLIC char * JsonBuffer_Get_String (json_object * parent, const char * name)
{
    const char *tmp;
    char * ret;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return NULL;
    if (name == NULL)
        return NULL;
    if ((object = json_object_object_get(parent, name))  == NULL)
        return false;
    if (json_object_get_type(object) != json_type_string)
        return false;
    tmp = json_object_get_string(object);
    if (asprintf(&ret, "%s", tmp) == -1)
        return NULL;
    return ret;
}

SO_PUBLIC bool JsonBuffer_Get_ByteArray (json_object * parent, const char * name,
                                      uint32_t *p_iSize,
                                      uint8_t **p_pByteArray)
{
    json_object *object;
    char *input;
    uint8_t *output;
    BIO *bmem, *b64;
    size_t length;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_string)
        return false;
    input = (char *)json_object_get_string(object);
    length = strlen(input);
    if ((output = calloc(length, sizeof(char))) == NULL)
        return false;
    
    b64 = BIO_new (BIO_f_base64());
    BIO_set_flags (b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf (input, strlen(input));
    bmem = BIO_push (b64, bmem);
    *p_iSize = BIO_read (bmem, output, length);
    BIO_free_all (bmem);
    *p_pByteArray = output;
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_UUID (json_object * parent, const char *name, uuid_t p_uuid)
{
    json_object *object;
    char *tmp;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // get the container
    if ((object = json_object_object_get(parent, name))  == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    if ((tmp =(char *) JsonBuffer_Get_String(object, "id")) == NULL)
        return false;

    uuid_parse(tmp, p_uuid);
    free(tmp);
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_UUID (json_object * parent, const char * name, uuid_t p_uuid)
{
    char uuid[UUID_STRING_LENGTH];
    json_object *new;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((new = json_object_new_object()) == NULL)
        return false;

    json_object_object_add(parent, name, new);
    uuid_unparse(p_uuid, uuid);
    parent = new;
    if (!JsonBuffer_Put_String(new, "id", uuid))
        return false;
    // TODO: Add other UUID attributes

    return true;
}

static bool
JsonBuffer_Get_NTLVItem (struct List *list, json_object *parent )
{
    char *str = NULL;
    uint8_t *byteData = NULL;
    uint32_t size = 0;
    uuid_t name, type, uuid;
    uint16_t port =0;
    uint8_t proto =0;

    if (!JsonBuffer_Get_UUID(parent, "Name", name))
        return false;

    if (!JsonBuffer_Get_UUID(parent, "Type", type))
        return false;

    if (json_object_object_get(parent, "String_Value") != NULL)
        str = JsonBuffer_Get_String(parent, "String_Value");
    
    if (json_object_object_get(parent, "Bin_Value") != NULL)
        JsonBuffer_Get_ByteArray(parent, "Bin_Value", &size, &byteData);

    if (( str == NULL) && (byteData == NULL))
        return false;

    UUID_Get_UUID(NTLV_TYPE_STRING, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(type, uuid) == 0)
        NTLVList_Add(list, name, type, strlen(str)+1, (uint8_t*)str);
    
    UUID_Get_UUID(NTLV_TYPE_PORT, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(type, uuid) == 0)
    {
        sscanf(str, "%hu", &port);
        NTLVList_Add(list, name, type, 2, (uint8_t*)&port);
    }
    UUID_Get_UUID(NTLV_TYPE_IPPROTO, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(type, uuid) == 0)
    {
        sscanf(str, "%hhu", &proto);
        NTLVList_Add(list, name, type, 1, (uint8_t*)&proto);
    }

    UUID_Get_UUID(NTLV_TYPE_IPv4_ADDR, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(type, uuid) == 0)
    {
        if ((byteData = calloc(4, sizeof(char))) == NULL)
            return false;
        inet_pton(AF_INET, str, byteData);
        NTLVList_Add(list, name, type, 4, byteData);

    }

    UUID_Get_UUID(NTLV_TYPE_IPv6_ADDR, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(type, uuid) == 0)
    {
        if ((str = calloc(16, sizeof(char))) == NULL)
            return false;
        inet_pton(AF_INET6, str, byteData);
        NTLVList_Add(list, name, type, 16, byteData);

    }


    if (str != NULL)
        free(str);
    if (byteData != NULL)
        free(byteData);

    return true;
}

static int
JsonBuffer_Put_NTLVItem (struct NTLVItem *p_pItem, json_object *parent )
{
    json_object *object;
    char *str = NULL;
    bool doFree = false;
    uuid_t uuid;
    uint16_t port =0;
    uint8_t proto =0;

    if ((object = json_object_new_object()) == NULL)
        return false;

    json_object_array_add(parent, object);

    if (!JsonBuffer_Put_UUID(object, "Name", p_pItem->uuidName))
        return LIST_EACH_ERROR;

    if (!JsonBuffer_Put_UUID(object, "Type", p_pItem->uuidType))
        return LIST_EACH_ERROR;

    UUID_Get_UUID(NTLV_TYPE_STRING, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(uuid, p_pItem->uuidType) == 0)
        str = (char *)p_pItem->pData;
    
    UUID_Get_UUID(NTLV_TYPE_PORT, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(uuid, p_pItem->uuidType) == 0)
    {
        memcpy(&port, p_pItem->pData, 2);

        if (asprintf(&str, "%hu", port) == -1)
            return LIST_EACH_ERROR;

        doFree = true;
    }
    UUID_Get_UUID(NTLV_TYPE_IPPROTO, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(uuid, p_pItem->uuidType) == 0)
    {
        memcpy(&proto, p_pItem->pData, 1);

        if (asprintf(&str, "%hhu", port) == -1)
            return LIST_EACH_ERROR;

        doFree = true;
    }
    UUID_Get_UUID(NTLV_TYPE_IPv4_ADDR, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(uuid, p_pItem->uuidType) == 0)
    {
        if ((str = calloc(1, INET_ADDRSTRLEN)) == NULL)
            return LIST_EACH_ERROR;
        doFree=true;
        inet_ntop(AF_INET, p_pItem->pData, str, INET_ADDRSTRLEN);
    }

    UUID_Get_UUID(NTLV_TYPE_IPv6_ADDR, UUID_TYPE_NTLV_TYPE, uuid);
    if (uuid_compare(uuid, p_pItem->uuidType) == 0)
    {
        if ((str = calloc(1, INET_ADDRSTRLEN)) == NULL)
            return LIST_EACH_ERROR;
        doFree=true;
        inet_ntop(AF_INET6, p_pItem->pData, str, INET6_ADDRSTRLEN);
    }


    if (str == NULL)
    {
        if (!JsonBuffer_Put_ByteArray(object, "Bin_Value", p_pItem->iLength, p_pItem->pData))
            return LIST_EACH_ERROR;
    }
    else
        if (!JsonBuffer_Put_String(object, "String_Value", str))
            return LIST_EACH_ERROR;

    if (doFree)
        free(str);

    return LIST_EACH_OK;
}

SO_PUBLIC bool JsonBuffer_Put_NTLVList (json_object * parent, const char * name,
                                     struct List *p_pList)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_array()) == NULL)
        return false;

    json_object_object_add(parent, name, object);

    if (!List_ForEach(p_pList, (int (*)(void*,void*))JsonBuffer_Put_NTLVItem, object))
        return false;

    return true;
}

SO_PUBLIC bool JsonBuffer_Get_NTLVList (json_object * parent, const char * name,
                                     struct List **p_pList)
{
    struct List *list;
    json_object *object;
	int i;

    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_array)
        return false;
    parent = object;
    if ((list = NTLVList_Create()) == NULL)
        return false;

    for (i = 0; i < json_object_array_length(parent); i++)
    {
        if (((object = json_object_array_get_idx(parent, i)) == NULL) ||
                (json_object_get_type(object) != json_type_object) )
        {
            List_Destroy(list);
            return false;
        }
        if (!JsonBuffer_Get_NTLVItem(list, object))
        {
            List_Destroy(list);
            return false;
        }
    }
    *p_pList = list;
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_Hash (json_object * parent, const char * name,
                                 const struct Hash *p_pHash)
{
    json_object *object;
    const char *str = NULL;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    switch (p_pHash->iType)
    {
    case HASH_TYPE_MD5:
        str= "MD5";
        break;
    case HASH_TYPE_SHA1:
        str= "SHA1";
        break;
    case HASH_TYPE_SHA224:
        str= "SHA224";
        break;
    case HASH_TYPE_SHA256:
        str= "SHA256";
        break;
    case HASH_TYPE_SHA512:
        str= "SHA512";
        break;
    default:
        return false;
    }
    if (!JsonBuffer_Put_String(parent, "Type", str))
        return false;
    if ((str = Hash_ToText(p_pHash)) == NULL)
        return false;
    if (!JsonBuffer_Put_String(parent, "Value", str))
        return false;
    free((void *)str);
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_Hash (json_object * parent, const char *name, struct Hash **p_pHash)
{
    struct Hash *hash;
    json_object *object, *object2;
    const char *type, *data, *pos;
	size_t i;
#ifdef _MSC_VER
	char tmp[3] = { '\0', '\0','\0' };
	unsigned long b;
#endif
    ASSERT( parent != NULL);
    ASSERT(name != NULL);

    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;

    if ((object = json_object_object_get(parent, "Type")) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_string)
        return false;
    type = json_object_get_string(object);
    if ((object2 = json_object_object_get(parent, "Value")) == NULL)
        return false;
    if (json_object_get_type(object2) != json_type_string)
        return false;
    data = json_object_get_string(object2);


    if ((hash = calloc(1, sizeof (struct Hash))) == NULL)
        return false;

    if (strcmp(type, "MD5") == 0)
        hash->iType = HASH_TYPE_MD5;
    else if (strcmp(type, "SHA1") == 0)
        hash->iType = HASH_TYPE_SHA1;
    else if (strcmp(type, "SHA224") == 0)
        hash->iType = HASH_TYPE_SHA224;
    else if (strcmp(type, "SHA256") == 0)
        hash->iType = HASH_TYPE_SHA256;
    else if (strcmp(type, "SHA512") == 0)
        hash->iType = HASH_TYPE_SHA512;

    hash->iSize = strlen(data)/2;
    if ((hash->pData = calloc(hash->iSize, sizeof(uint8_t))) == NULL)
    {
        Hash_Destroy(hash);
        return false;
    }
    pos = data;
    for(i = 0; i < hash->iSize; i++) {
#ifdef _MSC_VER
		tmp[0] = *pos;
		tmp[1] = *(pos+1);
		b = strtoul(tmp,NULL, 16);
		hash->pData[i] = (uint8_t) b;
#else
        sscanf(pos, "%2hhx", &hash->pData[i]);
#endif
        pos += 2;
    }
    hash->iFlags = HASH_FLAG_FINAL;
    *p_pHash = hash;
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_BlockId (json_object * parent, const char *name,
                                    struct BlockId *p_pId)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    if (!JsonBuffer_Put_Hash(parent, "Hash", p_pId->pHash))
        return false;
    if (!JsonBuffer_Put_uint64_t(parent, "Size", p_pId->iLength))
        return false;
    if (!JsonBuffer_Put_UUID(parent, "Data_Type", p_pId->uuidDataType))
        return false;
    return true;
}


SO_PUBLIC bool JsonBuffer_Get_BlockId (json_object * parent, const char * name,
                                    struct BlockId **p_pId)
{
    struct BlockId *id;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;
    if ((id = calloc(1, sizeof(struct BlockId))) == NULL)
        return false;

    if (!JsonBuffer_Get_Hash(parent, "Hash", &id->pHash))
    {
        BlockId_Destroy(id);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(parent, "Size", &id->iLength))
    {
        BlockId_Destroy(id);
        return false;
    }
    if (!JsonBuffer_Get_UUID(parent, "Data_Type", id->uuidDataType))
    {
        BlockId_Destroy(id);
        return false;
    }
    *p_pId = id;
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_Block (json_object * parent, const char * name, 
                                  struct Block *p_pBlock)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;

    if (!JsonBuffer_Put_BlockId(parent, "Id", p_pBlock->pId))
        return false;

    if (p_pBlock->pParentId != NULL)
        if (!JsonBuffer_Put_BlockId(parent, "Parent_Id", p_pBlock->pParentId))
            return false;

    if (p_pBlock->pParentBlock != NULL)
        if (!JsonBuffer_Put_Block(parent, "Parent", p_pBlock->pParentBlock))
            return false;

    if (p_pBlock->pMetaDataList != NULL)
        if (!JsonBuffer_Put_NTLVList(parent, "Metadata", p_pBlock->pMetaDataList))
            return false;


    return true;
}

SO_PUBLIC bool JsonBuffer_Get_Block (json_object * parent, const char * name,
                                  struct Block **p_pBlock)
{
    struct Block *block;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;
    if ((block = calloc(1, sizeof(struct Block))) == NULL)
        return false;

    if (!JsonBuffer_Get_BlockId(parent, "Id", &block->pId))
    {
        Block_Destroy(block);
        return false;
    }

    if (json_object_object_get(parent, "Parent_Id") != NULL)
    {
        if (!JsonBuffer_Get_BlockId(parent, "Parent_Id", &block->pParentId))
        {
            Block_Destroy(block);
            return false;
        }
    }
    if (json_object_object_get(parent, "Parent") != NULL)
    {
        if (!JsonBuffer_Get_Block(parent, "Parent", &block->pParentBlock))
        {
            Block_Destroy(block);
            return false;
        }
    }
    if (json_object_object_get(parent, "Metadata") != NULL)
    {
        if (!JsonBuffer_Get_NTLVList(parent, "Metadata", &block->pMetaDataList))
        {
            Block_Destroy(block);
            return false;
        }
    }

    *p_pBlock = block;
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_Event (json_object * parent, const char * name, 
                                  struct Event *p_pEvent)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    if (!JsonBuffer_Put_EventId(parent, "Id", p_pEvent->pId))
        return false;

    if (p_pEvent->pParentId != NULL)
        if (!JsonBuffer_Put_EventId(parent, "Parent_Id", p_pEvent->pParentId))
            return false;

    if (p_pEvent->pParent != NULL)
        if (!JsonBuffer_Put_Event(parent, "Parent", p_pEvent->pParent))
            return false;

    if (p_pEvent->pMetaDataList != NULL)
        if (!JsonBuffer_Put_NTLVList(parent, "Metadata", p_pEvent->pMetaDataList))
            return false;

    if (p_pEvent->pBlock != NULL)
        if (!JsonBuffer_Put_Block(parent, "Block", p_pEvent->pBlock))
            return false;
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_Event (json_object * parent, const char * name,
                                  struct Event **p_pEvent)
{
    struct Event *event;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;
    if ((event = calloc(1, sizeof(struct Event))) == NULL)
        return false;
    if (!JsonBuffer_Get_EventId(parent, "Id", &event->pId))
        return false;

    if (json_object_object_get(parent, "Parent_Id") != NULL)
    {
        if (!JsonBuffer_Get_EventId(parent, "Parent_Id", &event->pParentId))
        {
            Event_Destroy(event);
            return false;
        }
    }

    if (json_object_object_get(parent, "Parent") != NULL)
    {
        if (!JsonBuffer_Get_Event(parent, "Parent", &event->pParent))
        {
            Event_Destroy(event);
            return false;
        }
    }
    if (json_object_object_get(parent, "Metadata") != NULL)
    {
        if (!JsonBuffer_Get_NTLVList(parent, "Metadata", &event->pMetaDataList))
        {
            Event_Destroy(event);
            return false;
        }
    } 
    else
    {
        if ((event->pMetaDataList = NTLVList_Create()) == NULL)
        {
            Event_Destroy(event);
            return false;
        }

    }
    if (json_object_object_get(parent, "Block") != NULL)
    {
        if (!JsonBuffer_Get_Block(parent, "Block", &event->pBlock))
        {
            Event_Destroy(event);
            return false;
        }
    }

    *p_pEvent = event;
    return true;
}


SO_PUBLIC bool JsonBuffer_Put_EventId (json_object * parent, const char * name,
                                    struct EventId *p_pEventId)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    // XXX: Refactor this!!
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, "Nugget", object);
    if (!JsonBuffer_Put_UUID(object, "Id", p_pEventId->uuidNuggetId))
        return false;
    // XXX: End
    
    if (!JsonBuffer_Put_uint64_t(parent, "Seconds", p_pEventId->iSeconds))
        return false;

    if (!JsonBuffer_Put_uint64_t(parent, "Nano_Seconds", p_pEventId->iNanoSecs))
        return false;

    return true;
}

SO_PUBLIC bool JsonBuffer_Get_EventId (json_object * parent, const char * name, 
                                    struct EventId **p_pEventId)
{
    struct EventId *eventId;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;

    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;
    // XXX: Refactor this!!
    if ((object = json_object_object_get(parent, "Nugget")) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    // XXX: End

    if ((eventId = calloc(1, sizeof(struct EventId))) == NULL)
        return false;

    // XXX: Refactor this!!
    if (!JsonBuffer_Get_UUID(object, "Id", eventId->uuidNuggetId))
    {
        EventId_Destroy(eventId);
        return false;
    }
    // XXX: End
    
    if (!JsonBuffer_Get_uint64_t(parent, "Seconds", &eventId->iSeconds))
    {
        EventId_Destroy(eventId);
        return false;
    }

    if (!JsonBuffer_Get_uint64_t(parent, "Nano_Seconds", &eventId->iNanoSecs))
    {
        EventId_Destroy(eventId);
        return false;
    }
    *p_pEventId = eventId;
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_Judgment (json_object * parent, const char * name,
                                     struct Judgment *p_pJudgment)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    if (!JsonBuffer_Put_UUID(parent, "Nugget_ID", p_pJudgment->uuidNuggetId))
        return false;

    if (!JsonBuffer_Put_uint64_t(parent, "Seconds", p_pJudgment->iSeconds))
        return false;

    if (!JsonBuffer_Put_uint64_t(parent, "Nano_Seconds", p_pJudgment->iNanoSecs))
        return false;

    if (!JsonBuffer_Put_EventId(parent, "Event_ID", p_pJudgment->pEventId))
        return false;

    if (!JsonBuffer_Put_BlockId(parent, "Block_ID", p_pJudgment->pBlockId))
        return false;
     
    if (!JsonBuffer_Put_uint8_t(parent, "Priority", p_pJudgment->iPriority))
        return false;

    if (!JsonBuffer_Put_NTLVList(parent, "Metadata", p_pJudgment->pMetaDataList))
        return false;

    if (!JsonBuffer_Put_uint32_t(parent, "GID", p_pJudgment->iGID))
        return false;

    if (!JsonBuffer_Put_uint32_t(parent, "SID", p_pJudgment->iSID))
        return false;

    if (!JsonBuffer_Put_uint32_t(parent, "Set_SF_Flags", p_pJudgment->Set_SfFlags))
        return false;

    if (!JsonBuffer_Put_uint32_t(parent, "Set_Ent_Flags", p_pJudgment->Set_EntFlags))
        return false;

    if (!JsonBuffer_Put_uint32_t(parent, "Unset_SF_Flags", p_pJudgment->Unset_SfFlags))
        return false;

    if (!JsonBuffer_Put_uint32_t(parent, "Unset_Ent_Flags", p_pJudgment->Unset_EntFlags))
        return false;

    if (p_pJudgment->sMessage != NULL)
        if (!JsonBuffer_Put_String(parent, "Message", (char *)p_pJudgment->sMessage)) 
            return false;

    return true;
}

SO_PUBLIC bool JsonBuffer_Get_Judgment (json_object * parent, const char * name, 
                                     struct Judgment **p_pJudgment)
{
    struct Judgment *judgment;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;
    if ((judgment = calloc(1, sizeof(struct Judgment))) == NULL)
        return false;

    if (!JsonBuffer_Get_UUID(parent, "Nugget_ID", judgment->uuidNuggetId))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint64_t(parent, "Seconds", &judgment->iSeconds))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint64_t(parent, "Nano_Seconds", &judgment->iNanoSecs))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_EventId(parent, "Event_ID", &judgment->pEventId))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_BlockId(parent, "Block_ID", &judgment->pBlockId))
    {
        Judgment_Destroy(judgment);
        return false;
    }
     
    if (!JsonBuffer_Get_uint8_t(parent, "Priority", &judgment->iPriority))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_NTLVList(parent, "Metadata", &judgment->pMetaDataList))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint32_t(parent, "GID", &judgment->iGID))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint32_t(parent, "SID", &judgment->iSID))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint32_t(parent, "Set_SF_Flags", &judgment->Set_SfFlags))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint32_t(parent, "Set_Ent_Flags", &judgment->Set_EntFlags))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint32_t(parent, "Unset_SF_Flags", &judgment->Unset_SfFlags))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (!JsonBuffer_Get_uint32_t(parent, "Unset_Ent_Flags", &judgment->Unset_EntFlags))
    {
        Judgment_Destroy(judgment);
        return false;
    }

    if (json_object_object_get(parent, "Message") != NULL)
    {
        if ((judgment->sMessage = (uint8_t *)JsonBuffer_Get_String(parent, "Message")) == NULL)
        {
            Judgment_Destroy(judgment);
            return false;
        }
    }
    *p_pJudgment = judgment;
    return true;
}

SO_PUBLIC bool JsonBuffer_Put_Nugget (json_object * parent, const char * name, 
                                           struct Nugget * nugget)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_object()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;

    if (!JsonBuffer_Put_UUID(parent, "Nugget_ID", nugget->uuidNuggetId))
        return false;

    if (!JsonBuffer_Put_UUID(parent, "App_Type", nugget->uuidApplicationType))
        return false;

    if (!JsonBuffer_Put_UUID(parent, "Nugget_Type", nugget->uuidNuggetType))
        return false;

    if (nugget->sName != NULL)
        if (!JsonBuffer_Put_String(parent, "Name", nugget->sName))
            return false;

    if (nugget->sLocation != NULL)
        if (!JsonBuffer_Put_String(parent, "Location", nugget->sLocation))
            return false;

    if (nugget->sContact != NULL)
        if (!JsonBuffer_Put_String(parent, "Contact", nugget->sContact))
            return false;

    return true;
}

SO_PUBLIC bool JsonBuffer_Get_Nugget (json_object * parent, const char * name, 
                                           struct Nugget ** r_nugget)
{
    struct Nugget *nugget;
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    // Get the container
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_object)
        return false;
    parent = object;

    if ((nugget = calloc(1, sizeof(struct Nugget))) == NULL)
        return false;

    if (!JsonBuffer_Get_UUID(parent, "Nugget_ID", nugget->uuidNuggetId))
    {
        return false;
    }

    if (!JsonBuffer_Get_UUID(parent, "App_Type", nugget->uuidApplicationType))
    {
        return false;
    }
    if (!JsonBuffer_Get_UUID(parent, "Nugget_Type", nugget->uuidNuggetType))
    {
        return false;
    }
    if (json_object_object_get(parent, "Name") != NULL)
    {
        if ((nugget->sName = JsonBuffer_Get_String(parent, "Name")) == NULL)
        {
            return false;
        }
    }
    if (json_object_object_get(parent, "Location") != NULL)
    {
        if ((nugget->sLocation = JsonBuffer_Get_String(parent, "Location")) == NULL)
        {
            return false;
        }
    }

    if (json_object_object_get(parent, "Contact") != NULL)
    {
        if ((nugget->sContact = JsonBuffer_Get_String(parent, "Contact")) == NULL)
        {
            return false;
        }
    }
    *r_nugget=nugget;
    return true;
}

static int 
JsonBuffer_Put_UUIDList_Add(void *vnode, void *varray)
{
    struct UUIDListNode *node = vnode;
    char uuid[UUID_STRING_LENGTH];
    json_object *array = varray;
    json_object *object;
    if ((object = json_object_new_object()) == NULL)
        return LIST_EACH_ERROR;

    uuid_unparse(node->uuid, uuid);
    JsonBuffer_Put_String(object, "id", uuid);
    if (node->sName != NULL)
        JsonBuffer_Put_String(object, "name", node->sName);
    if (node->sDescription != NULL)
        JsonBuffer_Put_String(object, "description", node->sDescription);

    json_object_array_add(array, object);

    return LIST_EACH_OK;
}

SO_PUBLIC bool 
JsonBuffer_Put_UUIDList (json_object * parent, const char * name, 
                                           struct List * list)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_array()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    List_ForEach(list, JsonBuffer_Put_UUIDList_Add, object);
    return true;
}

SO_PUBLIC bool JsonBuffer_Get_UUIDList (json_object * parent, const char * name, 
                                           struct List ** r_list)
{
    struct List *list;
    json_object *object;
    uuid_t uuid;
    const char *uuidS, *nameS, *desc; 
	int i;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_array)
        return false;
    parent = object;
    if ((list = UUID_Create_List()) == NULL)
        return false;

    for (i = 0; i < json_object_array_length(parent); i++)
    {
        if (((object = json_object_array_get_idx(parent, i)) == NULL) ||
                (json_object_get_type(object) != json_type_object) )
        {
            List_Destroy(list);
            return false;
        }
        uuidS = JsonBuffer_Get_String(object, "id");
        nameS = JsonBuffer_Get_String(object, "name");
        desc = JsonBuffer_Get_String(object, "description");
        uuid_parse(uuidS, uuid);
        UUID_Add_List_Entry(list, uuid, nameS, desc);
        free((void *)nameS);
        free((void *)uuidS);
        free((void *)desc);
    }

    *r_list = list;
    return true;
}

static int 
JsonBuffer_Put_StringList_Add(void *vnode, void *varray)
{
    char *node = vnode;
    json_object *array = varray;
    json_object *object;
    if ((object = json_object_new_string(node)) == NULL)
        return LIST_EACH_ERROR;

    json_object_array_add(array, object);

    return LIST_EACH_OK;
}

SO_PUBLIC bool 
JsonBuffer_Put_StringList (json_object * parent, const char * name, 
                                           struct List * list)
{
    json_object *object;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_array()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    List_ForEach(list, JsonBuffer_Put_StringList_Add, object);
    return true;
}

SO_PUBLIC bool 
JsonBuffer_Get_StringList (json_object * parent, const char * name, 
                                           struct List ** r_list)
{
    struct List *list;
    json_object *object;
    const char *string;
	int i;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_array)
        return false;
    parent = object;
    if ((list = StringList_Create()) == NULL)
        return false;
    for (i = 0; i < json_object_array_length(parent); i++)
    {
        if (((object = json_object_array_get_idx(parent, i)) == NULL) ||
                (json_object_get_type(object) != json_type_string) ||
                ((string = json_object_get_string(object))== NULL))
        {
            List_Destroy(list);
            return false;
        }
        StringList_Add(list, string);
    }

    *r_list = list;
    return true;
}


SO_PUBLIC bool 
JsonBuffer_Put_uint8List (json_object * parent, const char * name, 
                                           uint8_t *list, uint32_t count)
{
    json_object *object;
	uint32_t i;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_new_array()) == NULL)
        return false;
    json_object_object_add(parent, name, object);
    parent = object;
    for (i = 0; i < count; i++)
    {
        if ((object = json_object_new_int(list[i])) == NULL)
            return false;

        json_object_array_add(parent, object);
    }
    return true;
}

SO_PUBLIC bool 
JsonBuffer_Get_uint8List (json_object * parent, const char * name, 
                                           uint8_t **list, uint32_t *count)
{
    json_object *object;
    uint32_t c =0;
    uint8_t *items = NULL;
	uint32_t i;
    ASSERT( parent != NULL);
    ASSERT(name != NULL);
    if (parent == NULL)
        return false;
    if (name == NULL)
        return false;
    if ((object = json_object_object_get(parent, name)) == NULL)
        return false;
    if (json_object_get_type(object) != json_type_array)
        return false;
    parent = object;
    c = json_object_array_length(parent);
    if (c > 0)
        if ((items = calloc(c, sizeof(uint8_t))) == NULL)
            return false;
    
    for ( i = 0; i < c; i++)
    {
        if (((object = json_object_array_get_idx(parent, i)) == NULL) ||
                (json_object_get_type(object) != json_type_int))
            items[i] = json_object_get_int(object);
    }

    *list = items;
    *count = c;
    return true;
}
