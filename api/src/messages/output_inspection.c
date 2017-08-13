#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/event.h>
#include <razorback/block.h>
#include <razorback/block_id.h>
#include <razorback/nugget.h>
#include <razorback/json_buffer.h>

#include "messages/core.h"

#include <string.h>

static void OutputInspection_Destroy (struct Message *message);
static bool OutputInspection_Deserialize_Json(struct Message *message);
static bool OutputInspection_Deserialize(struct Message *message, int mode);
static bool OutputInspection_Serialize_Json(struct Message *message);
static bool OutputInspection_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_OUTPUT_INSPECTION,
    OutputInspection_Serialize,
    OutputInspection_Deserialize,
    OutputInspection_Destroy
};

//core.h
void
MessageOutputInspection_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageOutputInspection_Initialize (
                                   struct Nugget *nugget,
                                   struct BlockId *blockId, uint8_t reason, bool final)
{
    struct Message *msg;
    struct MessageOutputInspection *message;
    struct timespec l_tsTime;
    ASSERT (blockId != NULL);
    ASSERT (nugget != NULL);
    if (blockId == NULL)
        return NULL;
    if (nugget == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_OUTPUT_INSPECTION, MESSAGE_VERSION_1, sizeof(struct MessageOutputInspection))) == NULL)
        return NULL;
    message = msg->message;
    message->blockId = BlockId_Clone(blockId);
    message->nugget = nugget;
    message->status = reason;
    message->final = final;

    memset(&l_tsTime, 0, sizeof(struct timespec));
    if (clock_gettime(CLOCK_REALTIME, &l_tsTime) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to get time stamp", __func__);
        return NULL;
    }
    message->seconds = l_tsTime.tv_sec;
    message->nanosecs = l_tsTime.tv_nsec;

    msg->destroy = OutputInspection_Destroy;
    msg->deserialize=OutputInspection_Deserialize;
    msg->serialize=OutputInspection_Serialize;

    return msg;
}

static void
OutputInspection_Destroy (struct Message *message)
{
    struct MessageOutputInspection *msg;
    
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg->blockId != NULL)
        BlockId_Destroy(msg->blockId);

    if (msg->nugget != NULL)
        Nugget_Destroy(msg->nugget);

    Message_Destroy(message);
}

static bool
OutputInspection_Deserialize_Json(struct Message *message)
{
    struct MessageOutputInspection *event;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
    
    event = message->message;

    if (!JsonBuffer_Get_Nugget(msg, "Nugget", &event->nugget))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_BlockId(msg, "BlockId", &event->blockId))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint8_t(msg, "Status", &event->status))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "Seconds", &event->seconds))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "NanoSecs", &event->nanosecs))
    {
        json_object_put(msg);
        return false;
    }
    uint8_t final = 0;
    if (!JsonBuffer_Get_uint8_t(msg, "Final", &final))
    {
        json_object_put(msg);
        return false;
    }
    if (final == 0)
        event->final = false;
    else 
        event->final = true;

    json_object_put(msg);
    return true;
}

static bool
OutputInspection_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageOutputInspection))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return OutputInspection_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
OutputInspection_Serialize_Json(struct Message *message)
{
    struct MessageOutputInspection *event;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    event = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_Nugget(msg, "Nugget", event->nugget))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_BlockId(msg, "BlockId", event->blockId))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint8_t(msg, "Status", event->status))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "Seconds", event->seconds))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "NanoSecs", event->nanosecs))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint8_t(msg, "Final", (event->final ? 1 : 0)))
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
OutputInspection_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return OutputInspection_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
