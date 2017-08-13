#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/event.h>
#include <razorback/block.h>
#include <razorback/nugget.h>
#include <razorback/json_buffer.h>

#include "messages/core.h"

#include <string.h>

static void OutputEvent_Destroy (struct Message *message);
static bool OutputEvent_Deserialize_Json(struct Message *message);
static bool OutputEvent_Deserialize(struct Message *message, int mode);
static bool OutputEvent_Serialize_Json(struct Message *message);
static bool OutputEvent_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_OUTPUT_EVENT,
    OutputEvent_Serialize,
    OutputEvent_Deserialize,
    OutputEvent_Destroy
};

//core.h
void
MessageOutputEvent_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageOutputEvent_Initialize (
                                   struct Event *event,
                                   struct Nugget *nugget)
{
    struct Message *msg;
    struct MessageOutputEvent *message;
    ASSERT (event != NULL);
    ASSERT (nugget != NULL);
    if (event == NULL)
        return NULL;
    if (nugget == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_OUTPUT_EVENT, MESSAGE_VERSION_1, sizeof(struct MessageOutputEvent))) == NULL)
        return NULL;
    message = msg->message;
    message->event = event;
    message->nugget = nugget;

    msg->destroy = OutputEvent_Destroy;
    msg->deserialize=OutputEvent_Deserialize;
    msg->serialize=OutputEvent_Serialize;

    return msg;
}

static void
OutputEvent_Destroy (struct Message *message)
{
    struct MessageOutputEvent *msg;
    
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg->event != NULL)
        Event_Destroy(msg->event);

    if (msg->nugget != NULL)
        Nugget_Destroy(msg->nugget);

    Message_Destroy(message);
}

static bool
OutputEvent_Deserialize_Json(struct Message *message)
{
    struct MessageOutputEvent *event;
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
    if (!JsonBuffer_Get_Event(msg, "Event", &event->event))
    {
        json_object_put(msg);
        return false;
    }

    json_object_put(msg);
    return true;
}

static bool
OutputEvent_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageOutputEvent))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return OutputEvent_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
OutputEvent_Serialize_Json(struct Message *message)
{
    struct MessageOutputEvent *event;
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
    if (!JsonBuffer_Put_Event(msg, "Event", event->event))
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
OutputEvent_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return OutputEvent_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
