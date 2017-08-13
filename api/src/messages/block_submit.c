#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/event.h>
#include <razorback/json_buffer.h>

#include "messages/core.h"
#include "binary_buffer.h"

#include <string.h>

static void BlockSubmission_Destroy (struct Message *message);
static bool BlockSubmission_Deserialize_Binary(struct Message *message);
static bool BlockSubmission_Deserialize_Json(struct Message *message);
static bool BlockSubmission_Deserialize(struct Message *message, int mode);
static bool BlockSubmission_Serialize_Binary(struct Message *message);
static bool BlockSubmission_Serialize_Json(struct Message *message);
static bool BlockSubmission_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_BLOCK,
    BlockSubmission_Serialize,
    BlockSubmission_Deserialize,
    BlockSubmission_Destroy
};

// core.h
void
MessageBlockSubmission_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageBlockSubmission_Initialize (
                                   struct Event *p_pEvent,
                                   uint32_t p_iReason, uint8_t locality)
{
    struct Message *msg;
    struct MessageBlockSubmission *message;
    ASSERT (p_pEvent != NULL);
    if (p_pEvent == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_BLOCK, MESSAGE_VERSION_1, sizeof(struct MessageBlockSubmission))) == NULL)
        return NULL;

    message = msg->message;

    message->pEvent = p_pEvent;
    message->iReason = p_iReason;
    message->storedLocality = locality;

    msg->destroy = BlockSubmission_Destroy;
    msg->deserialize=BlockSubmission_Deserialize;
    msg->serialize=BlockSubmission_Serialize;

    return msg;
}

static void
BlockSubmission_Destroy (struct Message *message)
{
    struct MessageBlockSubmission *msg;
    
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg->pEvent != NULL)
        Event_Destroy (msg->pEvent);

    Message_Destroy(message);
}

static bool
BlockSubmission_Deserialize_Binary(struct Message *message)
{
    struct BinaryBuffer *buffer;
    struct MessageBlockSubmission *submit;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((buffer = BinaryBuffer_CreateFromMessage(message)) == NULL)
        return false;
    
    submit = message->message;

    if (!BinaryBuffer_Get_uint32_t (buffer, &submit->iReason))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Get_Event (buffer, &submit->pEvent))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Get_uint8_t (buffer, &submit->storedLocality))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }

    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy (buffer);

    return true;
}

static bool
BlockSubmission_Deserialize_Json(struct Message *message)
{
    struct MessageBlockSubmission *submit;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;

    submit = message->message;

    if (!JsonBuffer_Get_uint32_t (msg, "Reason", &submit->iReason))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_Event (msg, "Event", &submit->pEvent))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint8_t (msg, "Stored_Locality", &submit->storedLocality))
    {
        json_object_put(msg);
        return false;
    }

    json_object_put(msg);
    return true;
}

static bool
BlockSubmission_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageBlockSubmission))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return BlockSubmission_Deserialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return BlockSubmission_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
BlockSubmission_Serialize_Binary(struct Message *message)
{
    struct MessageBlockSubmission *submit;
    struct BinaryBuffer *buffer;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    submit = message->message;

    message->length = Event_BinaryLength (submit->pEvent) +
         sizeof(uint8_t) + // XXX: Whats this bad boy?
         (uint32_t) sizeof (submit->iReason) + sizeof(uint8_t);

    if ((buffer = BinaryBuffer_Create(message->length)) == NULL)
        return false;

    if (!BinaryBuffer_Put_uint32_t (buffer, submit->iReason))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }

    if (!BinaryBuffer_Put_Event (buffer, submit->pEvent))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Put_uint8_t (buffer, submit->storedLocality))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }

    message->serialized = buffer->pBuffer;
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy(buffer);
    return true;
}

static bool
BlockSubmission_Serialize_Json(struct Message *message)
{
    struct MessageBlockSubmission *submit;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    submit = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_uint32_t (msg, "Reason", submit->iReason))
    {
        json_object_put(msg);
        return false;
    }

    if (!JsonBuffer_Put_Event (msg, "Event", submit->pEvent))
    {
        json_object_put(msg);
        return false;
    }

    if (!JsonBuffer_Put_uint8_t (msg, "Stored_Locality", submit->storedLocality))
    {
        json_object_put(msg);
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
BlockSubmission_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return BlockSubmission_Serialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return BlockSubmission_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
