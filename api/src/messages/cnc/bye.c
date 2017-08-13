#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>

#include "messages/core.h"
#include "messages/cnc/core.h"

SO_PUBLIC struct Message *
MessageBye_Initialize (
                       const uuid_t p_uuidSourceNugget)
{
    struct Message *msg;
    if ((msg = Message_Create_Broadcast(MESSAGE_TYPE_BYE, MESSAGE_VERSION_1, 0, p_uuidSourceNugget)) == NULL)
        return NULL;

    msg->destroy = Message_Destroy;
    msg->deserialize = Message_Deserialize_Empty;
    msg->serialize = Message_Serialize_Empty;
    return msg;
}

static struct MessageHandler handler = {
    MESSAGE_TYPE_BYE,
    Message_Serialize_Empty,
    Message_Deserialize_Empty,
    Message_Destroy
};

// core.h
void 
Message_CnC_Bye_Init(void)
{
    Message_Register_Handler(&handler);
}
