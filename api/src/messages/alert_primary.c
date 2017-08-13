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

static void AlertPrimary_Destroy (struct Message *message);
static bool AlertPrimary_Deserialize_Json(struct Message *message);
static bool AlertPrimary_Deserialize(struct Message *message, int mode);
static bool AlertPrimary_Serialize_Json(struct Message *message);
static bool AlertPrimary_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_ALERT_PRIMARY,
    AlertPrimary_Serialize,
    AlertPrimary_Deserialize,
    AlertPrimary_Destroy
};

// core.h
void 
MessageAlertPrimary_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageAlertPrimary_Initialize (
                                   struct Event *event,
                                   struct Block *block,
                                   struct List *metadata,
                                   struct Nugget *nugget,
                                   struct Judgment *judgment,
                                   uint32_t new_SF_Flags, uint32_t new_Ent_Flags,
                                   uint32_t old_SF_Flags, uint32_t old_Ent_Flags)
{
    struct Message *msg;
    struct MessageAlertPrimary *message;
    ASSERT (event != NULL);
    ASSERT (block != NULL);
    ASSERT (metadata != NULL);
    ASSERT (nugget != NULL);
    ASSERT (judgment != NULL);
    if (event == NULL)
        return NULL;
    if (block == NULL)
        return NULL;
    if (metadata == NULL)
        return NULL;
    if (nugget == NULL)
        return NULL;
    if (judgment == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_ALERT_PRIMARY, MESSAGE_VERSION_1, sizeof(struct MessageAlertPrimary))) == NULL)
        return NULL;

    message = msg->message;

    message->event = event;
    message->block = block;
    message->metadata = metadata;
    message->nugget = nugget;
    message->gid = judgment->iGID;
    message->sid = judgment->iSID;
    message->message = (char *)judgment->sMessage;
    message->seconds = judgment->iSeconds;
    message->nanosecs = judgment->iNanoSecs;
    message->priority = judgment->iPriority;
    message->SF_Flags = new_SF_Flags;
    message->Ent_Flags = new_Ent_Flags;
    message->Old_SF_Flags = old_SF_Flags;
    message->Old_Ent_Flags = old_Ent_Flags;


    msg->destroy = AlertPrimary_Destroy;
    msg->deserialize=AlertPrimary_Deserialize;
    msg->serialize=AlertPrimary_Serialize;

    return msg;
}

static void
AlertPrimary_Destroy (struct Message *message)
{
    struct MessageAlertPrimary *msg;
    
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg->event != NULL)
        Event_Destroy(msg->event);
    if (msg->block != NULL)
        Block_Destroy(msg->block);

    if (msg->metadata != NULL)
        List_Destroy(msg->metadata);
    
    if (msg->message != NULL)
        free(msg->message);

    if (msg->nugget != NULL)
        Nugget_Destroy(msg->nugget);

    Message_Destroy(message);
}

static bool
AlertPrimary_Deserialize_Json(struct Message *message)
{
    struct MessageAlertPrimary *alert;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
    
    alert = message->message;

    if (!JsonBuffer_Get_Nugget(msg, "Nugget", &alert->nugget))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_Block(msg, "Block", &alert->block))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_Event(msg, "Event", &alert->event))
    {
        json_object_put(msg);
        return false;
    }
    if ((alert->message = JsonBuffer_Get_String(msg, "Message")) == NULL)
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint8_t(msg, "Priority", &alert->priority))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "Seconds", &alert->seconds))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "Nano_Seconds", &alert->nanosecs))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint32_t(msg, "GID", &alert->gid))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint32_t(msg, "SID", &alert->sid))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint32_t(msg, "SF_Flags", &alert->SF_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint32_t(msg, "Ent_Flags", &alert->Ent_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint32_t(msg, "Old_SF_Flags", &alert->Old_SF_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint32_t(msg, "Old_Ent_Flags", &alert->Old_Ent_Flags))
    {
        json_object_put(msg);
        return false;
    }

    if (!JsonBuffer_Get_NTLVList(msg, "Metadata", &alert->metadata))
    {
        json_object_put(msg);
        return false;
    }

    json_object_put(msg);
    return true;
}

static bool
AlertPrimary_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageAlertPrimary))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return AlertPrimary_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
AlertPrimary_Serialize_Json(struct Message *message)
{
    struct MessageAlertPrimary *alert;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    alert = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_Nugget(msg, "Nugget", alert->nugget))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_Block(msg, "Block", alert->block))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_Event(msg, "Event", alert->event))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_String(msg, "Message", alert->message))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint8_t(msg, "Priority", alert->priority))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "Seconds", alert->seconds))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "Nano_Seconds", alert->nanosecs))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint32_t(msg, "GID", alert->gid))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint32_t(msg, "SID", alert->sid))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint32_t(msg, "SF_Flags", alert->SF_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint32_t(msg, "Ent_Flags", alert->Ent_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint32_t(msg, "Old_SF_Flags", alert->Old_SF_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint32_t(msg, "Old_Ent_Flags", alert->Old_Ent_Flags))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_NTLVList(msg, "Metadata", alert->metadata))
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
AlertPrimary_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return AlertPrimary_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
