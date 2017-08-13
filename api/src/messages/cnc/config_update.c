#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/list.h>
#include <razorback/uuids.h>
#include <razorback/json_buffer.h>


#include "messages/core.h"
#include "messages/cnc/core.h"

#include "binary_buffer.h"

#include <string.h>

static bool ConfigUpdate_Deserialize_Json(struct Message *message);
static bool ConfigUpdate_Deserialize_Binary(struct Message *message);
static bool ConfigUpdate_Deserialize(struct Message *message, int mode);
static void ConfigUpdate_Destroy (struct Message *message);
static bool ConfigUpdate_Serialize_Json(struct Message *message);
static bool ConfigUpdate_Serialize_Binary(struct Message *message);
static bool ConfigUpdate_Serialize(struct Message *message, int mode);

static struct MessageHandler handler = {
    MESSAGE_TYPE_CONFIG_UPDATE,
    ConfigUpdate_Serialize,
    ConfigUpdate_Deserialize,
    ConfigUpdate_Destroy
};

// core.h
void 
Message_CnC_ConfigUpdate_Init(void)
{
    Message_Register_Handler(&handler);
}

SO_PUBLIC struct Message *
MessageConfigurationUpdate_Initialize (
                                       const uuid_t p_uuidSourceNugget,
                                       const uuid_t p_uuidDestNugget)
{
    struct MessageConfigurationUpdate *message;
    struct Message *msg;
    struct List *list;

    if ((msg = Message_Create_Directed(MESSAGE_TYPE_CONFIG_UPDATE, MESSAGE_VERSION_1, sizeof(struct MessageConfigurationUpdate), p_uuidSourceNugget, p_uuidDestNugget)) == NULL)
        return NULL;
    message = msg->message;

    if ((list = UUID_Get_List(UUID_TYPE_NTLV_TYPE)) == NULL)
        return false;
    if ((message->ntlvTypes= List_Clone(list)) == NULL)
        return false;
    if ((list = UUID_Get_List(UUID_TYPE_NTLV_NAME)) == NULL)
        return false;
    if ((message->ntlvNames= List_Clone(list)) == NULL)
        return false;
    if ((list = UUID_Get_List(UUID_TYPE_DATA_TYPE)) == NULL)
        return false;
    if ((message->dataTypes= List_Clone(list)) == NULL)
        return false;

    
    msg->destroy = ConfigUpdate_Destroy;
    msg->deserialize = ConfigUpdate_Deserialize;
    msg->serialize = ConfigUpdate_Serialize;
    return msg;
}

static void
ConfigUpdate_Destroy (struct Message *msg)
{
    struct MessageConfigurationUpdate *message;
    ASSERT(msg != NULL);
    if (msg == NULL)
        return;
    message = msg->message;
    if (message->ntlvTypes != NULL)
        List_Destroy(message->ntlvTypes);
    if (message->ntlvNames != NULL)
        List_Destroy(message->ntlvNames);
    if (message->dataTypes != NULL)
        List_Destroy(message->dataTypes);

    Message_Destroy(msg);
}

static bool
ConfigUpdate_Deserialize_Binary(struct Message *message)
{
    struct BinaryBuffer *buffer;
    struct MessageConfigurationUpdate *configUpdate;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((buffer = BinaryBuffer_CreateFromMessage(message)) == NULL)
        return false;
    
    configUpdate = message->message;
    if (!BinaryBuffer_Get_UUIDList(buffer, &configUpdate->ntlvTypes))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy(buffer);
        return false;
    }
    if (!BinaryBuffer_Get_UUIDList(buffer, &configUpdate->ntlvNames))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy(buffer);
        return false;
    }
    if (!BinaryBuffer_Get_UUIDList(buffer, &configUpdate->dataTypes))
    {
        buffer->pBuffer = NULL;
        BinaryBuffer_Destroy(buffer);
        return false;
    }

    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy (buffer);

    return true;
}

static bool
ConfigUpdate_Deserialize_Json(struct Message *message)
{
    struct MessageConfigurationUpdate *configUpdate;
    json_object *msg;
    //uint32_t size; 
    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
    
    configUpdate = message->message;
    if (!JsonBuffer_Get_UUIDList(msg, "NTLV_Types", &configUpdate->ntlvTypes))
    {
        json_object_put(msg);
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Types", __func__);
        return false;
    }
    if (!JsonBuffer_Get_UUIDList(msg, "NTLV_Names", &configUpdate->ntlvNames))
    {
        json_object_put(msg);
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Names", __func__);
        return false;
    }
    if (!JsonBuffer_Get_UUIDList(msg, "Data_Types", &configUpdate->dataTypes))
    {
        json_object_put(msg);
        rzb_log(LOG_ERR, "%s: Failed to get Data Types", __func__);
        return false;
    }
    json_object_put(msg);
    return true;
}

static bool
ConfigUpdate_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageConfigurationUpdate))) == NULL)
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return ConfigUpdate_Deserialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return ConfigUpdate_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}

static bool
ConfigUpdate_Serialize_Binary(struct Message *message)
{
    struct MessageConfigurationUpdate *configUpdate;
    struct BinaryBuffer *buffer;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    configUpdate = message->message;

    message->length = (sizeof(uint32_t) * 6 )+
        UUIDList_BinarySize(configUpdate->ntlvTypes) +
        UUIDList_BinarySize(configUpdate->ntlvNames) +
        UUIDList_BinarySize(configUpdate->dataTypes);

    if ((buffer = BinaryBuffer_Create(message->length)) == NULL)
        return false;

    if (!BinaryBuffer_Put_UUIDList(buffer, configUpdate->ntlvTypes))
    {
        BinaryBuffer_Destroy(buffer);
        return false;
    }
    if (!BinaryBuffer_Put_UUIDList(buffer, configUpdate->ntlvNames))
    {
        BinaryBuffer_Destroy(buffer);
        return false;
    }
    if (!BinaryBuffer_Put_UUIDList(buffer, configUpdate->dataTypes))
    {
        BinaryBuffer_Destroy(buffer);
        return false;
    }

    message->serialized = buffer->pBuffer;
    buffer->pBuffer = NULL;
    BinaryBuffer_Destroy(buffer);
    return true;
}

static bool
ConfigUpdate_Serialize_Json(struct Message *message)
{
    struct MessageConfigurationUpdate *configUpdate;
    const char * wire;
    json_object *msg;
    ASSERT(message != NULL);
    if (message == NULL)
        return false;
    
    if ((msg = json_object_new_object()) == NULL)
        return false;
   
    configUpdate = message->message;

    if (!JsonBuffer_Put_UUIDList(msg, "NTLV_Types", configUpdate->ntlvTypes))
    {
        json_object_put(msg);
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Types", __func__);
        return false;
    }
    if (!JsonBuffer_Put_UUIDList(msg, "NTLV_Names", configUpdate->ntlvNames))
    {
        json_object_put(msg);
        rzb_log(LOG_ERR, "%s: Failed to get NTLV Names", __func__);
        return false;
    }
    if (!JsonBuffer_Put_UUIDList(msg, "Data_Types", configUpdate->dataTypes))
    {
        json_object_put(msg);
        rzb_log(LOG_ERR, "%s: Failed to get Data Types", __func__);
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
ConfigUpdate_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_BIN:
        return ConfigUpdate_Serialize_Binary(message);
    case MESSAGE_MODE_JSON:
        return ConfigUpdate_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}
