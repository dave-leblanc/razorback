#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/string_list.h>
#include <razorback/json_buffer.h>

#include "messages/core.h"
#include "messages/cnc/core.h"
#include "binary_buffer.h"

#include <string.h>

static bool Hello_Deserialize_Binary(struct Message *message);
static bool Hello_Deserialize_Json(struct Message *message);
static bool Hello_Deserialize(struct Message *message, int mode);
static bool Hello_Serialize_Binary(struct Message *message);
static bool Hello_Serialize_Json(struct Message *message);
static bool Hello_Serialize(struct Message *message, int mode);
static void Hello_Destroy (struct Message *message);

static struct MessageHandler handler = {
    MESSAGE_TYPE_HELLO,
    Hello_Serialize,
    Hello_Deserialize,
    Hello_Destroy
};

// core.h
void 
Message_CnC_Hello_Init(void)
{
    Message_Register_Handler(&handler);
}

static bool
Hello_Deserialize_Binary(struct Message *message)
{
    struct BinaryBuffer *buffer;
    struct MessageHello *hello;
    uuid_t dispatcher;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);

    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((buffer = BinaryBuffer_CreateFromMessage(message)) == NULL)
        return false;
    
    hello = message->message;

    if (!BinaryBuffer_Get_UUID
        (buffer, hello->uuidNuggetType))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Get_UUID", __func__);
        return false;
    }

    if (!BinaryBuffer_Get_UUID(buffer, hello->uuidApplicationType))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Get_UUID", __func__);
        return false;
    }
    if (!BinaryBuffer_Get_uint8_t(buffer, &hello->locality))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Get_UUID", __func__);
        return false;
    }
    if (uuid_compare(hello->uuidNuggetType, dispatcher) == 0)
    {

        if (!(BinaryBuffer_Get_uint8_t(buffer, &hello->priority) &&
                BinaryBuffer_Get_uint8_t(buffer, &hello->protocol) &&
                BinaryBuffer_Get_uint16_t(buffer, &hello->port) &&
                BinaryBuffer_Get_uint32_t(buffer, &hello->flags)))
        {
            buffer->pBuffer = NULL;
            BinaryBuffer_Destroy (buffer);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of BinaryBuffer_Get_uint8", __func__);
            return false;
        }
        // Address List
        if (!BinaryBuffer_Get_StringList(buffer, &hello->addressList))
        {
            buffer->pBuffer = NULL;
            BinaryBuffer_Destroy (buffer);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of BinaryBuffer_Get_StringList", __func__);
            return false;
        }

    }

    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy (buffer);

    return true;
}
static bool
Hello_Deserialize_Json(struct Message *message)
{
    struct MessageHello *hello;
    json_object *msg;
    uuid_t dispatcher;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
 
    hello = message->message;

    if (!JsonBuffer_Get_UUID
        (msg, "Nugget_Type", hello->uuidNuggetType))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_UUID", __func__);
        return false;
    }

    if (!JsonBuffer_Get_UUID(msg, "App_Type", hello->uuidApplicationType))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_UUID", __func__);
        return false;
    }
    if (!JsonBuffer_Get_uint8_t(msg, "Locality", &hello->locality))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_UUID", __func__);
        return false;
    }
    if (uuid_compare(hello->uuidNuggetType, dispatcher) == 0)
    {
        if (!(JsonBuffer_Get_uint8_t(msg, "Priority", &hello->priority) &&
                JsonBuffer_Get_uint8_t(msg, "Protocol", &hello->protocol) &&
                JsonBuffer_Get_uint16_t(msg, "Port", &hello->port) &&
                JsonBuffer_Get_uint32_t(msg, "Flags", &hello->flags)))
        {
            json_object_put(msg);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of JsonBuffer_Get_uint8", __func__);
            return false;
        }
        // Address List
        if (!JsonBuffer_Get_StringList(msg, "Address_List", &hello->addressList))
        {
            json_object_put(msg);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of JsonBuffer_Get_StringList", __func__);
            return false;
        }

    }
    json_object_put(msg);

    return true;
}

static bool
Hello_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageHello))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return Hello_Deserialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return Hello_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}

SO_PUBLIC struct Message *
MessageHello_Initialize (struct RazorbackContext *context)
{
    struct Message * msg;
    struct MessageHello *message;
    uuid_t dispatcher;
    if (!UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher))
    	return NULL;

    if ((msg = Message_Create_Broadcast(MESSAGE_TYPE_HELLO, MESSAGE_VERSION_1, sizeof(struct MessageHello), context->uuidNuggetId)) == NULL)
        return NULL;

    message = msg->message;

    uuid_copy (message->uuidNuggetType, context->uuidNuggetType);
    uuid_copy (message->uuidApplicationType, context->uuidApplicationType);
    message->locality = context->locality;
    if (uuid_compare(dispatcher, context->uuidNuggetType) == 0)
    {
        message->flags = context->dispatcher.flags;
        message->priority = context->dispatcher.priority;
        message->port = context->dispatcher.port;
        message->protocol = context->dispatcher.protocol;
        if( (message->addressList = List_Clone(context->dispatcher.addressList)) == NULL)
        {
            Hello_Destroy(msg);
            return NULL;
        }
    }

    msg->destroy = Hello_Destroy;
    msg->deserialize=Hello_Deserialize;
    msg->serialize=Hello_Serialize;

    return msg;
}

static void
Hello_Destroy (struct Message *msg)
{
    struct MessageHello *message;

    ASSERT (msg != NULL);
    if (msg == NULL)
        return;
    message = msg->message;

	if(message->addressList != NULL)
	    List_Destroy (message->addressList);

    Message_Destroy(msg);
}


static bool
Hello_Serialize_Binary(struct Message *message)
{
    struct MessageHello *hello;
    struct BinaryBuffer *buffer;
    uuid_t dispatcher;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    hello = message->message;

    message->length = (sizeof(uuid_t) * 2) + sizeof(uint8_t);
    if (uuid_compare(hello->uuidApplicationType, dispatcher) == 0)
    {
        message->length += sizeof(uint8_t) +
            sizeof(uint32_t) + StringList_Size(hello->addressList);
    }

    if ((buffer = BinaryBuffer_Create(message->length)) == NULL)
        return false;

    if (!BinaryBuffer_Put_UUID
        (buffer, hello->uuidNuggetType))
    {
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_UUID", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_UUID
        (buffer, hello->uuidApplicationType))
    {
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Put_UUID", __func__);
        return false;
    }
    if (!BinaryBuffer_Put_uint8_t(buffer, hello->locality))
    {
        BinaryBuffer_Destroy (buffer);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of BinaryBuffer_Get_UUID", __func__);
        return false;
    }
    if (uuid_compare(hello->uuidNuggetType, dispatcher) == 0)
    {

        if (!(BinaryBuffer_Put_uint8_t(buffer, hello->priority) &&
                BinaryBuffer_Put_uint8_t(buffer, hello->protocol) &&
                BinaryBuffer_Put_uint16_t(buffer, hello->port) &&
                BinaryBuffer_Put_uint32_t(buffer, hello->flags)))
        {
            BinaryBuffer_Destroy (buffer);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of BinaryBuffer_Put_uint8", __func__);
            return false;
        }
        // Address List
        if (!BinaryBuffer_Put_StringList(buffer, hello->addressList))
        {
            BinaryBuffer_Destroy (buffer);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of BinaryBuffer_Put_StringList", __func__);
            return false;
        }
    }

    message->serialized = buffer->pBuffer;
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy(buffer);
    return true;
}

static bool
Hello_Serialize_Json(struct Message *message)
{
    struct MessageHello *hello;
    json_object *msg;
    const char * wire;
    uuid_t dispatcher;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    hello = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_UUID
        (msg, "Nugget_Type", hello->uuidNuggetType))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_UUID", __func__);
        return false;
    }
    if (!JsonBuffer_Put_UUID
        (msg, "App_Type", hello->uuidApplicationType))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_UUID", __func__);
        return false;
    }

    if (!JsonBuffer_Put_uint8_t(msg, "Locality", hello->locality))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_UUID", __func__);
        return false;
    }
    if (uuid_compare(hello->uuidNuggetType, dispatcher) == 0)
    {
        if (!(JsonBuffer_Put_uint8_t(msg, "Priority", hello->priority) &&
                JsonBuffer_Put_uint8_t(msg, "Protocol", hello->protocol) &&
                JsonBuffer_Put_uint16_t(msg, "Port", hello->port) &&
                JsonBuffer_Put_uint32_t(msg, "Flags", hello->flags)))
        {
            json_object_put(msg);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of JsonBuffer_Put_uint8", __func__);
            return false;
        }
        // Address List
        if (!JsonBuffer_Put_StringList(msg, "Address_List", hello->addressList))
        {
            json_object_put(msg);
            rzb_log (LOG_ERR,
                     "%s: failed due to failure of JsonBuffer_Put_StringList", __func__);
            return false;
        }

    }

    wire = json_object_to_json_string(msg);
    message->length = strlen(wire);
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
Hello_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return Hello_Serialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return Hello_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
