#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/json_buffer.h>

#include "dbus/messages.h"
#include "dispatcher.h"
#include "routing_list.h"

static bool RT_Announce_Deserialize_Json(struct Message *message);
static bool RT_Announce_Deserialize(struct Message *message, int mode);
static bool RT_Announce_Serialize_Json(struct Message *message);
static bool RT_Announce_Serialize(struct Message *message, int mode);

struct Message *
RT_Announce_Initialize (void)
{
    struct Message *msg;
    struct MessageRT_Announce *announce;

    if ((msg = Message_Create_Broadcast(MESSAGE_TYPE_RT_ANNOUNCE, MESSAGE_VERSION_1, sizeof(struct MessageRT_Announce), sg_rbContext.uuidNuggetId)) == NULL)
        return NULL;
    announce = msg->message;
    announce->serial = RoutingList_GetSerial();
    
    msg->destroy=Message_Destroy;
    msg->deserialize = RT_Announce_Deserialize;
    msg->serialize = RT_Announce_Serialize;

    return msg;
}

static struct MessageHandler handler = {
    MESSAGE_TYPE_RT_ANNOUNCE, 
    RT_Announce_Serialize,
    RT_Announce_Deserialize,
    Message_Destroy
};

// messages.h
void 
DBus_RT_Announce_Init(void)
{
    Message_Register_Handler(&handler);
}


static bool
RT_Announce_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageRT_Announce))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return RT_Announce_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}

static bool
RT_Announce_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return RT_Announce_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}


static bool
RT_Announce_Serialize_Json(struct Message *message)
{
    struct MessageRT_Announce *announce;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    announce = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_uint64_t(msg, "Serial", announce->serial))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_uint64", __func__);
        return false;
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
RT_Announce_Deserialize_Json(struct Message *message)
{
    struct MessageRT_Announce *announce;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
 
    announce = message->message;
    if (!JsonBuffer_Get_uint64_t(msg, "Serial", &announce->serial))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_uint64", __func__);
        return false;
    }

    json_object_put(msg);

    return true;
}


