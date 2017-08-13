#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/block_id.h>
#include <razorback/json_buffer.h>

#include "messages/core.h"
#include "binary_buffer.h"

#include <string.h>

static void CacheReq_Destroy (struct Message *message);
static bool CacheReq_Deserialize_Binary(struct Message *message);
static bool CacheReq_Deserialize_Json(struct Message *message);
static bool CacheReq_Deserialize(struct Message *message, int mode);
static bool CacheReq_Serialize_Binary(struct Message *message);
static bool CacheReq_Serialize_Json(struct Message *message);
static bool CacheReq_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_REQ,
    CacheReq_Serialize,
    CacheReq_Deserialize,
    CacheReq_Destroy
};

// core.h
void
MessageCacheReq_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageCacheReq_Initialize (const uuid_t p_uuidRequestor,
                            const struct BlockId *p_pBlockId)
{
    struct Message *msg;
    struct MessageCacheReq *message;

    ASSERT (p_pBlockId != NULL);

    if (p_pBlockId == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_REQ, MESSAGE_VERSION_1, sizeof(struct MessageCacheReq))) == NULL)
        return NULL;

    message = msg->message;

    // fill in rest of message
    uuid_copy (message->uuidRequestor, p_uuidRequestor);
    if ((message->pId = BlockId_Clone (p_pBlockId)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BlockId_Clone", __func__);
        CacheReq_Destroy(msg);
        return NULL;
    }
    msg->destroy=CacheReq_Destroy;
    msg->deserialize=CacheReq_Deserialize;
    msg->serialize=CacheReq_Serialize;

    // done
    return msg;
}

static void
CacheReq_Destroy (struct Message *message)
{
    struct MessageCacheReq *msg;
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg != NULL)
        BlockId_Destroy (msg->pId);
    
    Message_Destroy(message);
}

static bool
CacheReq_Deserialize_Binary(struct Message *message)
{
    struct BinaryBuffer *buffer;
    struct MessageCacheReq *submit;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((buffer = BinaryBuffer_CreateFromMessage(message)) == NULL)
        return false;
    
    submit = message->message;

    if (!BinaryBuffer_Get_UUID (buffer, submit->uuidRequestor))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Get_UUID", __func__);
        return false;
    }
    if (!BinaryBuffer_Get_BlockId (buffer, &submit->pId))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_GetBlockId", __func__);
        return false;
    }
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy (buffer);

    return true;
}

static bool
CacheReq_Deserialize_Json(struct Message *message)
{
    struct MessageCacheReq *submit;
    json_object *msg;


    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
 
    submit = message->message;

    if (!JsonBuffer_Get_UUID (msg, "Requestor", submit->uuidRequestor))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_UUID", __func__);
        return false;
    }
    if (!JsonBuffer_Get_BlockId (msg, "Block_ID", &submit->pId))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_GetBlockId", __func__);
        return false;
    }
    json_object_put(msg);

    return true;
}

static bool
CacheReq_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageCacheReq))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return CacheReq_Deserialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return CacheReq_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
CacheReq_Serialize_Binary(struct Message *message)
{
    struct MessageCacheReq *submit;
    struct BinaryBuffer *buffer;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    submit = message->message;

    message->length = (uint32_t) sizeof (submit->uuidRequestor) +
                  BlockId_BinaryLength (submit->pId);

    if ((buffer = BinaryBuffer_Create(message->length)) == NULL)
        return false;

    if (!BinaryBuffer_Put_UUID (buffer, submit->uuidRequestor))
    {
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_UUID", __func__);
        return false;
    }

    if (!BinaryBuffer_Put_BlockId (buffer, submit->pId))
    {
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_BlockId", __func__);
        return false;
    }

    message->serialized = buffer->pBuffer;
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy(buffer);
    return true;
}

static bool
CacheReq_Serialize_Json(struct Message *message)
{
    struct MessageCacheReq *submit;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    submit = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_UUID (msg, "Requestor", submit->uuidRequestor))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_UUID", __func__);
        return false;
    }

    if (!JsonBuffer_Put_BlockId (msg, "Block_ID", submit->pId))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_BlockId", __func__);
        return false;
    }
    wire = json_object_to_json_string(msg);
    message->length=strlen(wire);
    if ((message->serialized = calloc(message->length+1, sizeof(char))) == NULL)
    {
        json_object_put(msg);
        return false;
    }
    strcpy((char *)message->serialized, wire); 
    json_object_put(msg);

    return true;
}

static bool
CacheReq_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return CacheReq_Serialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return CacheReq_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
