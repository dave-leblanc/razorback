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

static void AlertChild_Destroy (struct Message *message);
static bool AlertChild_Deserialize_Json(struct Message *message);
static bool AlertChild_Deserialize(struct Message *message, int mode);
static bool AlertChild_Serialize_Json(struct Message *message);
static bool AlertChild_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_ALERT_CHILD,
    AlertChild_Serialize,
    AlertChild_Deserialize,
    AlertChild_Destroy
};

// core.h
void MessageAlertChild_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageAlertChild_Initialize (
                                   struct Block *block,
                                   struct Block *child,
                                   struct Nugget *nugget,
                                   uint64_t parentCount, uint64_t eventCount,
                                   uint32_t new_SF_Flags, uint32_t new_Ent_Flags,
                                   uint32_t old_SF_Flags, uint32_t old_Ent_Flags)
{
    struct Message *msg;
    struct MessageAlertChild *message;
    ASSERT (child != NULL);
    ASSERT (block != NULL);
    ASSERT (nugget != NULL);
    if (child == NULL)
        return NULL;
    if (block == NULL)
        return NULL;
    if (nugget == NULL)
        return NULL;

    if ((msg = Message_Create(MESSAGE_TYPE_ALERT_CHILD, MESSAGE_VERSION_1, sizeof(struct MessageAlertChild))) == NULL)
        return NULL;

    message = msg->message;

    message->child = child;
    message->block = block;
    message->nugget = nugget;
    message->SF_Flags = new_SF_Flags;
    message->Ent_Flags = new_Ent_Flags;
    message->Old_SF_Flags = old_SF_Flags;
    message->Old_Ent_Flags = old_Ent_Flags;
    message->parentCount = parentCount;
    message->eventCount = eventCount;

    msg->destroy = AlertChild_Destroy;
    msg->deserialize=AlertChild_Deserialize;
    msg->serialize=AlertChild_Serialize;

    return msg;
}

static void
AlertChild_Destroy (struct Message *message)
{
    struct MessageAlertChild *msg;
    
    ASSERT (message != NULL);
    if (message == NULL)
        return;
    msg = message->message;

    // destroy any malloc'd components
    if (msg->child != NULL)
        Block_Destroy(msg->child);
    if (msg->block != NULL)
        Block_Destroy(msg->block);

    if (msg->nugget != NULL)
        Nugget_Destroy(msg->nugget);

    Message_Destroy(message);
}

static bool
AlertChild_Deserialize_Json(struct Message *message)
{
    struct MessageAlertChild *alert;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg) )
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
    if (!JsonBuffer_Get_Block(msg, "Child_Block", &alert->child))
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
    if (!JsonBuffer_Get_uint64_t(msg, "Event_Count", &alert->eventCount))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Get_uint64_t(msg, "Parent_Count", &alert->parentCount))
    {
        json_object_put(msg);
        return false;
    }

    json_object_put(msg);
    return true;
}

static bool
AlertChild_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageAlertChild))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return AlertChild_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
AlertChild_Serialize_Json(struct Message *message)
{
    struct MessageAlertChild *alert;
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
    if (!JsonBuffer_Put_Block(msg, "Child_Block", alert->child))
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
    if (!JsonBuffer_Put_uint64_t(msg, "Event_Count", alert->eventCount))
    {
        json_object_put(msg);
        return false;
    }
    if (!JsonBuffer_Put_uint64_t(msg, "Parent_Count", alert->parentCount))
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
AlertChild_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return AlertChild_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
