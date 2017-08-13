#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/json_buffer.h>

#include "dbus/messages.h"
#include "dispatcher.h"

static bool File_Delete_Deserialize_Json(struct Message *message);
static bool File_Delete_Deserialize(struct Message *message, int mode);
static bool File_Delete_Serialize_Json(struct Message *message);
static bool File_Delete_Serialize(struct Message *message, int mode);

struct Message *
File_Delete_Initialize (struct BlockId *bid, uint8_t locality)
{
    struct Message *msg;
    struct MessageFile_Delete *deletion;

    if ((msg = Message_Create_Broadcast(MESSAGE_TYPE_FILE_DELETE, MESSAGE_VERSION_1, sizeof(struct MessageFile_Delete), sg_rbContext.uuidNuggetId)) == NULL)
        return NULL;
    deletion = msg->message;
    deletion->localityId = locality;
	deletion->bid = bid;

    msg->destroy=Message_Destroy;
    msg->deserialize = File_Delete_Deserialize;
    msg->serialize = File_Delete_Serialize;

    return msg;
}

static struct MessageHandler handler = {
    MESSAGE_TYPE_FILE_DELETE, 
    File_Delete_Serialize,
    File_Delete_Deserialize,
    Message_Destroy
};

// messages.h
void 
DBus_File_Delete_Init(void)
{
    Message_Register_Handler(&handler);
}


static bool
File_Delete_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageFile_Delete))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return File_Delete_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}

static bool
File_Delete_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return File_Delete_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid serialization mode", __func__);
        return false;
    }
    return false;
}


static bool
File_Delete_Serialize_Json(struct Message *message)
{
    struct MessageFile_Delete *deletion;
    json_object *msg;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    deletion = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_uint8_t(msg, "Locality", deletion->localityId))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_uint8", __func__);
        return false;
    }
    if (!JsonBuffer_Put_BlockId(msg, "BlockId", deletion->bid))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_BlockId", __func__);
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
File_Delete_Deserialize_Json(struct Message *message)
{
    struct MessageFile_Delete *deletion;
    json_object *msg;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
 
    deletion = message->message;
    if (!JsonBuffer_Get_uint8_t(msg, "Locality", &deletion->localityId))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_uint8", __func__);
        return false;
    }
    if (!JsonBuffer_Get_BlockId(msg, "BlockId", &deletion->bid))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_BlockId", __func__);
        return false;
    }

    json_object_put(msg);

    return true;
}


