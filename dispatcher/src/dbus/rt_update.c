#include "config.h"

#include <razorback/debug.h>
#include <razorback/messages.h>
#include <razorback/log.h>
#include <razorback/json_buffer.h>

#include "dbus/messages.h"
#include "dispatcher.h"
#include "routing_list.h"

static bool RT_Update_Deserialize_Json(struct Message *message);
static bool RT_Update_Deserialize(struct Message *message, int mode);
static bool RT_Update_Serialize_Json(struct Message *message);
static bool RT_Update_Serialize(struct Message *message, int mode);
static void RT_Update_Destroy (struct Message *message);

struct Message *
RT_Update_Initialize_To (uint8_t reason, uuid_t dest)
{
    struct Message *msg;
    struct MessageRT_Update *update;

    if ((msg = Message_Create_Directed(MESSAGE_TYPE_RT_UPDATE, MESSAGE_VERSION_1, sizeof(struct MessageRT_Update), sg_rbContext.uuidNuggetId, dest)) == NULL)
        return NULL;
    update = msg->message;
    update->serial = RoutingList_GetSerial();
    update->reason = reason;
    update->list = List_Clone(g_routingList);

    
    msg->destroy=RT_Update_Destroy;
    msg->deserialize = RT_Update_Deserialize;
    msg->serialize = RT_Update_Serialize;

    return msg;
}

struct Message *
RT_Update_Initialize (uint8_t reason)
{
    struct Message *msg;
    struct MessageRT_Update *update;

    if ((msg = Message_Create_Broadcast(MESSAGE_TYPE_RT_UPDATE, MESSAGE_VERSION_1, sizeof(struct MessageRT_Update), sg_rbContext.uuidNuggetId)) == NULL)
        return NULL;
    update = msg->message;
    update->serial = RoutingList_GetSerial();
    update->reason = reason;
    update->list = List_Clone(g_routingList);

    
    msg->destroy=RT_Update_Destroy;
    msg->deserialize = RT_Update_Deserialize;
    msg->serialize = RT_Update_Serialize;

    return msg;
}

static struct MessageHandler handler = {
    MESSAGE_TYPE_RT_UPDATE, 
    RT_Update_Serialize,
    RT_Update_Deserialize,
    RT_Update_Destroy
};

// messages.h
void 
DBus_RT_Update_Init(void)
{
    Message_Register_Handler(&handler);
}


static bool
RT_Update_Deserialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    struct MessageRT_Update *update;
    if ( message == NULL )
        return false;

    if ((message->message = calloc(1,sizeof(struct MessageRT_Update))) == NULL)
        return false;
    update = message->message;
    update->list = List_Create(LIST_MODE_GENERIC,
            DataType_Cmp, // Cmp
            DataType_KeyCmp, // KeyCmp
            DataType_Destroy, // Destroy
            DataType_Clone, // Clone
            NULL, // Lock
            NULL); // Unlock

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return RT_Update_Deserialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}

static bool
RT_Update_Serialize(struct Message *message, int mode)
{
    ASSERT(message != NULL);
    if ( message == NULL )
        return false;

    switch (mode)
    {
    case MESSAGE_MODE_JSON:
        return RT_Update_Serialize_Json(message);
    default:
        rzb_log(LOG_ERR, "%s: Invalid deserialization mode", __func__);
        return false;
    }
    return false;
}

static int 
RT_Update_Serialize_AT(void *vAppType, void *vList)
{
    struct RoutingAppListEntry *appType = vAppType;
    json_object *list = vList;
    json_object *msg;
    if ((msg = json_object_new_object()) == NULL)
        return LIST_EACH_ERROR;
    if (!JsonBuffer_Put_UUID(msg, "App_Type", appType->uuidAppType))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_UUID", __func__);
        return LIST_EACH_ERROR;
    }
    json_object_array_add(list, msg);
    return LIST_EACH_OK;
}

static int 
RT_Update_Serialize_DT(void *vDataType, void *vList)
{
    struct RoutingListEntry *dataType = vDataType;
    json_object *list = vList;
    json_object *msg;
    json_object *apps;
    if ((msg = json_object_new_object()) == NULL)
        return LIST_EACH_ERROR;
    if (!JsonBuffer_Put_UUID(msg, "Data_Type", dataType->uuidDataType))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_UUID", __func__);
        return LIST_EACH_ERROR;
    }
    if ((apps = json_object_new_array()) == NULL)
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of json_object_new_array", __func__);
        return false;
    }
    json_object_object_add(msg, "Apps", apps);
    List_ForEach(dataType->appList, RT_Update_Serialize_AT,apps);

    json_object_array_add(list, msg);
    return LIST_EACH_OK;
}

static bool
RT_Update_Serialize_Json(struct Message *message)
{
    struct MessageRT_Update *update;
    json_object *msg, *list;
    const char * wire;

    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    update = message->message;

    if ((msg = json_object_new_object()) == NULL)
        return false;

    if (!JsonBuffer_Put_uint64_t(msg, "Serial", update->serial))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_uint64", __func__);
        return false;
    }

    if (!JsonBuffer_Put_uint8_t(msg, "Reason", update->reason))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Put_uint64", __func__);
        return false;
    }
    if ((list = json_object_new_array()) == NULL)
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of json_object_new_array", __func__);
        return false;
    }
    json_object_object_add(msg, "Table", list);
    List_ForEach(update->list, RT_Update_Serialize_DT, list); 

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
RT_Update_Deserialize_Json(struct Message *message)
{
    struct MessageRT_Update *update;
    json_object *msg, *object, *list, *appList;
    struct RoutingListEntry *dataType;
    struct RoutingAppListEntry *appType;
    uint32_t c,a;
    ASSERT(message != NULL);
    if (message == NULL)
        return false;

    if ((msg = json_tokener_parse((char *)message->serialized)) == NULL || is_error(msg))
        return false;
 
    update = message->message;
    if (!JsonBuffer_Get_uint64_t(msg, "Serial", &update->serial))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_uint64", __func__);
        return false;
    }
    if (!JsonBuffer_Get_uint8_t(msg, "Reason", &update->reason))
    {
        json_object_put(msg);
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of JsonBuffer_Get_uint8", __func__);
        return false;
    }
    if ((list = json_object_object_get(msg, "Table")) == NULL)
        return false;
    if (json_object_get_type(list) != json_type_array)
        return false;
    c = json_object_array_length(list);

    for (uint32_t i = 0; i < c; i++)
    {
        if ((dataType = calloc(1,sizeof(struct RoutingListEntry))) == NULL)
            continue;
        dataType->appList = List_Create(LIST_MODE_GENERIC,
                AppType_Cmp,
                AppType_KeyCmp,
                AppType_Destroy,
                AppType_Clone,
                NULL,
                NULL);
        if(dataType->appList == NULL)
            continue;
        if ((object = json_object_array_get_idx(list, i)) == NULL) 
            continue;
        JsonBuffer_Get_UUID(object, "Data_Type", dataType->uuidDataType);
        if ((appList = json_object_object_get(object, "Apps")) == NULL)
            continue;
        if (json_object_get_type(appList) != json_type_array)
            continue;
        a = json_object_array_length(appList);
        for (uint32_t j = 0; j < a; j++)
        {
            if ((appType = calloc(1,sizeof(struct RoutingAppListEntry))) == NULL)
                continue;
            if ((object = json_object_array_get_idx(appList, j)) == NULL) 
                continue;
            JsonBuffer_Get_UUID(object, "App_Type", appType->uuidAppType);
            List_Push(dataType->appList, appType);
        }
        List_Push(update->list, dataType);
        
    }

    json_object_put(msg);

    return true;
}

static void
RT_Update_Destroy (struct Message *msg)
{
    struct MessageRT_Update *message;

    ASSERT (msg != NULL);
    if (msg == NULL)
        return;
    message = msg->message;

	if(message->list != NULL)
	    List_Destroy (message->list);

    Message_Destroy(msg);
}

