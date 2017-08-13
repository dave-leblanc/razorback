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

static void OutputLog_Destroy (struct Message *message);
static bool OutputLog_Deserialize_Json(struct Message *message);
static bool OutputLog_Deserialize(struct Message *message, int mode);
static bool OutputLog_Serialize_Json(struct Message *message);
static bool OutputLog_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_OUTPUT_LOG,
    OutputLog_Serialize,
    OutputLog_Deserialize,
    OutputLog_Destroy
};

//core.h
void
MessageOutputLog_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageOutputLog_Initialize (
                                   struct MessageLogSubmission *log,
                                   struct Nugget *nugget)
{
    struct Message *msg;
    struct MessageOutputLog *message;
    ASSERT (log != NULL);
    ASSERT (nugget != NULL);
    if (log == NULL)
        return NULL;
    if (nugget == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_OUTPUT_LOG, MESSAGE_VERSION_1, sizeof(struct MessageOutputLog))) == NULL)
        return NULL;
    message = msg->message;
    if ((message->event = calloc(1,sizeof(struct Event))) == NULL)
    {
        Message_Destroy(msg);
        return NULL;
    }
    message->event->pId = log->pEventId;
    message->nugget = nugget;
    message->priority = log->iPriority;
    message->message = (char *)log->sMessage;

    msg->destroy = OutputLog_Destroy;
    msg->deserialize=OutputLog_Deserialize;
    msg->serialize=OutputLog_Serialize;

    return msg;
}

static void
OutputLog_Destroy (struct Message *message)
{
    struct MessageOutputLog *msg;
    
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg->event != NULL)
        Event_Destroy(msg->event);
    if (msg->message != NULL)
        free(msg->message);
    if (msg->nugget != NULL)
        Nugget_Destroy(msg->nugget);
    Message_Destroy(message);
}

static bool
OutputLog_Deserialize_Json(struct Message *message)
{
    struct MessageOutputLog *log;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
    
    log = message->message;

    if (!JsonBuffer_Get_Nugget(msg, "Nugget", &log->nugget))
    {
        json_object_put(msg);
        return false;
    }
    if (json_object_object_get(msg, "Event") != NULL)
    {
        if (!JsonBuffer_Get_Event(msg, "Event", &log->event))
        {
            json_object_put(msg);
            return false;
        }
    }
    if (!JsonBuffer_Get_uint8_t(msg, "Priority", &log->priority))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "Seconds", &log->seconds))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "Nano_Seconds", &log->nanosecs))
    {
        json_object_put(msg);
        return false;
    }
    if (( log->message = JsonBuffer_Get_String(msg, "Message")) == NULL)
    {
        json_object_put(msg);
        return false;
    }

    json_object_put(msg);
    return true;
}

static bool
OutputLog_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageOutputLog))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return OutputLog_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
OutputLog_Serialize_Json(struct Message *message)
{
    struct MessageOutputLog *log;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    log = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_Nugget(msg, "Nugget", log->nugget))
    {
        json_object_put(msg);
        return false;
    }
    if (log->event != NULL)
    {
        if (!JsonBuffer_Put_Event(msg, "Event", log->event))
        {
            json_object_put(msg);
            return false;
        }
    }
    if (!JsonBuffer_Put_uint8_t(msg, "Priority", log->priority))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "Seconds", log->seconds))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "Nano_Seconds", log->nanosecs))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_String(msg, "Message", log->message)) 
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
OutputLog_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return OutputLog_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
