#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>

#include "dbus/messages.h"
#include "dispatcher.h"

struct Message *
RT_Request_Initialize (uuid_t destNugget)
{
    struct Message *msg;

    if ((msg = Message_Create_Directed(MESSAGE_TYPE_RT_REQUEST, MESSAGE_VERSION_1, 0, sg_rbContext.uuidNuggetId, destNugget)) == NULL)
        return NULL;

    msg->destroy=Message_Destroy;
    msg->deserialize = Message_Deserialize_Empty;
    msg->serialize = Message_Serialize_Empty;

    return msg;
}

static struct MessageHandler handler = {
    MESSAGE_TYPE_RT_REQUEST,
    Message_Serialize_Empty,
    Message_Deserialize_Empty,
    Message_Destroy
};

// core.h
void 
DBus_RT_Request_Init(void)
{
    Message_Register_Handler(&handler);
}
