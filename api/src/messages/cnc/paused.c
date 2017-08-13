#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>

#include "messages/core.h"
#include "messages/cnc/core.h"

SO_PUBLIC struct Message *
MessagePaused_Initialize (
                          const uuid_t p_uuidSourceNugget,
                          const uuid_t p_uuidDestNugget)
{
    struct Message *msg;
    if ((msg = Message_Create_Directed(MESSAGE_TYPE_PAUSED, MESSAGE_VERSION_1, 0, p_uuidSourceNugget, p_uuidDestNugget)) == NULL)
        return NULL;

    msg->destroy = Message_Destroy;
    msg->deserialize = Message_Deserialize_Empty;
    msg->serialize = Message_Serialize_Empty;
    return msg;
}

static struct MessageHandler handler = {
    MESSAGE_TYPE_PAUSED,
    Message_Serialize_Empty,
    Message_Deserialize_Empty,
    Message_Destroy
};

// core.h
void 
Message_CnC_Paused_Init(void)
{
    Message_Register_Handler(&handler);
}
