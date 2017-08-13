#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/block.h>
#include <razorback/event.h>
#include <razorback/json_buffer.h>


#include "messages/core.h"
#include "binary_buffer.h"

#include <string.h>

static void InspectionSubmission_Destroy (struct Message *message);
static bool InspectionSubmission_Deserialize_Binary(struct Message *message);
static bool InspectionSubmission_Deserialize_Json(struct Message *message);
static bool InspectionSubmission_Deserialize(struct Message *message, int mode);
static bool InspectionSubmission_Serialize_Binary(struct Message *message);
static bool InspectionSubmission_Serialize_Json(struct Message *message);
static bool InspectionSubmission_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_INSPECTION,
    InspectionSubmission_Serialize,
    InspectionSubmission_Deserialize,
    InspectionSubmission_Destroy
};

//core.h
void
MessageInspectionSubmission_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageInspectionSubmission_Initialize (
                                        const struct Event *p_pEvent,
                                        uint32_t p_iReason,
                                        uint32_t localityCount,
                                        uint8_t *localities)
{
    struct Message * msg;
    struct MessageInspectionSubmission *message;

    ASSERT (p_pEvent != NULL);
    if (p_pEvent == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_INSPECTION, MESSAGE_VERSION_1, sizeof(struct MessageInspectionSubmission))) == NULL)
        return NULL;
    message = msg->message;
	
    if ((message->pBlock = Block_Clone(p_pEvent->pBlock)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to clone block", __func__);
        InspectionSubmission_Destroy(msg);
        return NULL;
    }

    message->iReason = p_iReason;
    if ((message->eventId = EventId_Clone(p_pEvent->pId)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to clone event id", __func__);
        InspectionSubmission_Destroy(msg);
        return NULL;
    }
    message->pEventMetadata = List_Clone (p_pEvent->pMetaDataList);
    if (message->pEventMetadata == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to clone metadata list", __func__);
        InspectionSubmission_Destroy(msg);
        return NULL;
    }
    message->localityCount = localityCount;
    if (localityCount > 0)
    {
        if ((message->localityList = calloc(localityCount, sizeof(uint8_t))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to clone locality list", __func__);
            InspectionSubmission_Destroy(msg);
            return NULL;
        }
        memcpy(message->localityList, localities, localityCount);
    }
    msg->destroy=InspectionSubmission_Destroy;
    msg->deserialize=InspectionSubmission_Deserialize;
    msg->serialize=InspectionSubmission_Serialize;

    return msg;
}

static void
InspectionSubmission_Destroy (struct Message
                                     *message)
{
    struct MessageInspectionSubmission *msg;
    ASSERT (message != NULL);
    if (message == NULL)
        return;

    msg = (struct MessageInspectionSubmission *)message->message;

    // destroy any malloc'd components
    if (msg->pBlock != NULL)
        Block_Destroy (msg->pBlock);
    if (msg->eventId != NULL)
        EventId_Destroy(msg->eventId);
    if (msg->pEventMetadata != NULL)
        List_Destroy(msg->pEventMetadata);
    if (msg->localityList != NULL)
        free(msg->localityList);

    Message_Destroy(message);
}

static bool
InspectionSubmission_Deserialize_Binary(struct Message *message)
{
    struct BinaryBuffer *buffer;
    struct MessageInspectionSubmission *submit;
	uint32_t i;
    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((buffer = BinaryBuffer_CreateFromMessage(message)) == NULL)
        return false;
    
    submit = (struct MessageInspectionSubmission *)message->message;

    if (!BinaryBuffer_Get_uint32_t (buffer, &submit->iReason))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Get_EventId (buffer, &submit->eventId))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Get_NTLVList (buffer, &submit->pEventMetadata))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    
    if (!BinaryBuffer_Get_Block (buffer, &submit->pBlock))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Get_uint32_t (buffer, &submit->localityCount))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (submit->localityCount > 0)
        if ((submit->localityList = calloc(submit->localityCount, sizeof(uint8_t))) == NULL)
        {
            BinaryBuffer_Destroy (buffer);
            return false;
        }

    for (i = 0; i < submit->localityCount; i++)
    {
        if (!BinaryBuffer_Get_uint8_t (buffer, &(submit->localityList[i])))
        {
            BinaryBuffer_Destroy (buffer);
            return false;
        }
    }
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy (buffer);

    return true;
}

static bool
InspectionSubmission_Deserialize_Json(struct Message *message)
{
    struct MessageInspectionSubmission *submit;
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
    if (!JsonBuffer_Get_EventId (msg, "Event_ID", &submit->eventId))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_NTLVList (msg, "Event_Metadata", &submit->pEventMetadata))
    {
        json_object_put(msg);
        return false;
    }
    
    if (!JsonBuffer_Get_Block (msg, "Block", &submit->pBlock))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint8List (msg, "Avaliable_Localities", &submit->localityList, &submit->localityCount))
    {
        json_object_put(msg);
        return false;
    }
    json_object_put(msg);
    return true;
}

static bool
InspectionSubmission_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageInspectionSubmission))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return InspectionSubmission_Deserialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return InspectionSubmission_Deserialize_Json(message);
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
InspectionSubmission_Serialize_Binary(struct Message *message)
{
    struct MessageInspectionSubmission *submit;
    struct BinaryBuffer *buffer;
	uint32_t i;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    submit = (struct MessageInspectionSubmission *)message->message;

    message->length = Block_BinaryLength (submit->pBlock) +
            NTLVList_Size (submit->pEventMetadata) +
            sizeof(struct EventId) + //Source nugget ID
            (uint32_t) sizeof (submit->iReason) +
            sizeof(uint32_t) + submit->localityCount;


    if ((buffer = BinaryBuffer_Create(message->length)) == NULL)
        return false;

    if (!BinaryBuffer_Put_uint32_t (buffer, submit->iReason))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Put_EventId (buffer, submit->eventId))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Put_NTLVList (buffer, submit->pEventMetadata))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }

    if (!BinaryBuffer_Put_Block (buffer, submit->pBlock))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    if (!BinaryBuffer_Put_uint32_t (buffer, submit->localityCount))
    {
        BinaryBuffer_Destroy (buffer);
        return false;
    }
    for (i = 0; i < submit->localityCount; i++)
    {
        if (!BinaryBuffer_Put_uint8_t (buffer, submit->localityList[i]))
        {
            BinaryBuffer_Destroy (buffer);
            return false;
        }
    }
    message->serialized = buffer->pBuffer;
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy(buffer);
    return true;
}

static bool
InspectionSubmission_Serialize_Json(struct Message *message)
{
    struct MessageInspectionSubmission *submit;
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
    if (!JsonBuffer_Put_EventId (msg, "Event_ID", submit->eventId))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_NTLVList (msg, "Event_Metadata", submit->pEventMetadata))
    {
        json_object_put(msg);
        return false;
    }

    if (!JsonBuffer_Put_Block (msg, "Block", submit->pBlock))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint8List (msg, "Avaliable_Localities", submit->localityList, submit->localityCount))
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
InspectionSubmission_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return InspectionSubmission_Serialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return InspectionSubmission_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
