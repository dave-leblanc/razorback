/** @file routinglist.h
 * RoutingList structures and functions
 */

#include <razorback/inspector_queue.h>
#include "routing_list.h"
#include "statistics.h"
#include "dispatcher.h"
#include "submission_processor.h"
#include "datastore.h"
#include "dbus/processor.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


// Function declarations
bool RoutingList_RemoveAppType (struct RoutingListEntry * p_prleDataType,
                                uuid_t p_pApplicationType);
bool RoutingList_RemoveDataType (uuid_t p_pBlockType);

// Static Globals
struct List *g_routingList;    // Head of routing table
static uuid_t sg_uuidAllType;   // ALL_DATA type for routing
static uint64_t sg_serial =0;



bool
RoutingList_Initialize (void)
{
    // Setup ALL_DATA uuid for nuggets registering for all data
    UUID_Get_UUID (DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, sg_uuidAllType);
    g_routingList = List_Create(LIST_MODE_GENERIC,
            DataType_Cmp,
            DataType_KeyCmp,
            DataType_Destroy,
            DataType_Clone,
            NULL, //Lock
            NULL); // Unlock
    if (g_routingList == NULL)
        return false;


    // Setup stats, if configured
#ifdef ROUTING_STATS
    if((stats->routing = calloc(sizeof(RoutingStats),1)) == NULL)
    {
       rzb_log(LOG_ERR, "Failed to allocate space for the routing stats structure");
       return false;
    }
#endif /* ROUTING_STATS */
    return true;
}

void
RoutingList_Terminate (void)
{

    List_Destroy(g_routingList);
}

inline void
RoutingList_Lock (void)
{
    List_Lock(g_routingList);
}

inline void
RoutingList_Unlock (void)
{
    List_Unlock(g_routingList);
}

static struct RoutingListEntry *
RoutingList_FindDataType (uuid_t dataType)
{
    return List_Find(g_routingList, dataType);
}

static struct RoutingAppListEntry *
RoutingList_FindAppType (struct RoutingListEntry *dataTypeEntry,
                         uuid_t appType)
{
    return List_Find(dataTypeEntry->appList, appType);
}

bool
RoutingList_RemoveDataType (uuid_t dataType)
{
    struct RoutingListEntry *dataTypeEntry = NULL;
    char *dataTypeName;

    dataTypeEntry = List_Find(g_routingList, dataType);
    if (dataTypeEntry == NULL)
    {
        dataTypeName = UUID_Get_NameByUUID (dataType, UUID_TYPE_DATA_TYPE);
        rzb_log (LOG_INFO, "%s: Failed to remove %s, not found", __func__, dataTypeName);
        free(dataTypeName);
        return false;
    }
    List_Remove(g_routingList, dataTypeEntry);

    return true;
}

bool
RoutingList_RemoveAppType (struct RoutingListEntry * dataTypeEntry,
                           uuid_t appType)
{
    struct RoutingAppListEntry *appEntry = NULL;

    char *dataTypeName;
    char *appTypeName;

    appEntry = RoutingList_FindAppType(dataTypeEntry, appType);
    if (appEntry == NULL)
    {
        appTypeName = UUID_Get_NameByUUID (appType, UUID_TYPE_NUGGET);
        dataTypeName = UUID_Get_NameByUUID (dataTypeEntry->uuidDataType,
                                 UUID_TYPE_DATA_TYPE);
        rzb_log(LOG_ERR, "%s: Failed to remove app type (%s) from data type (%s)", __func__, appTypeName, dataTypeName);
        free(dataTypeName);
        free(appTypeName);
        return false;
    }
    List_Remove(dataTypeEntry->appList, appEntry);
    return true;
}


static struct RoutingListEntry *
RoutingList_AddDataType (const uuid_t p_pDataType)
{
    struct RoutingListEntry *new;

    // Build new data type entry
    if ((new = calloc (1,sizeof (struct RoutingListEntry))) == NULL)
    {
        rzb_log (LOG_ERR, "%s, Failed to add data type to routing table", __func__);
        return NULL;
    }
    uuid_copy (new->uuidDataType, p_pDataType);
    new->appList = List_Create(LIST_MODE_GENERIC,
            AppType_Cmp,
            AppType_KeyCmp,
            AppType_Destroy,
            AppType_Clone,
            NULL, // Lock
            NULL); // Unlock
    if (new->appList == NULL)
    {
        free(new);
        return NULL;
    }
    List_Push(g_routingList, new);
    return new;
}

static struct RoutingAppListEntry *
RoutingList_AddAppType (struct RoutingListEntry *dataTypeEntry,
                        uuid_t appType)
{
    struct RoutingAppListEntry *new;

    // Build new app type entry
    if ((new = calloc (1,sizeof (struct RoutingAppListEntry))) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to add application type to routing table", __func__);
        return NULL;
    }
    new->pQueue =
        InspectorQueue_Initialize (appType, QUEUE_FLAG_SEND);
    if (new->pQueue == NULL)
    {
        free(new);
        return NULL;
    }
    uuid_copy (new->uuidAppType, appType);

    List_Push(dataTypeEntry->appList, new);
    return new;
}

// Functions called externally (must manage thread locks)
bool
RoutingList_Add (uuid_t dataType, uuid_t appType)
{
    struct RoutingListEntry *l_pCurrent;
    struct RoutingAppListEntry *l_pCurrentApp;

    RoutingList_Lock ();

    // Find the correct data type route entry and, if necessary create it
    l_pCurrent = RoutingList_FindDataType (dataType);
    if (l_pCurrent == NULL)
    {
        l_pCurrent = RoutingList_AddDataType (dataType);
        if (l_pCurrent == NULL)
        {
            RoutingList_Unlock ();
            return false;
        }
    }

    // Find the correct app type entry and, if necessary create it
    l_pCurrentApp = RoutingList_FindAppType (l_pCurrent, appType);
    if (l_pCurrentApp == NULL)
    {
        l_pCurrentApp =
            RoutingList_AddAppType (l_pCurrent, appType);
        if (l_pCurrentApp == NULL)
        {
            RoutingList_Unlock ();
            return false;
        }
    }


    // StatCheck
    //g_stats->routing->routeEntryCount++;
    sg_serial++;
    DBus_Push_Update();
    RoutingList_Unlock ();
    return true;
}

struct RoutingData
{
    struct Message *message;
    struct MessageInspectionSubmission *insMessage;
    struct DatabaseConnection *dbCon;
    bool routed;
    bool error;
};

static int
RoutingList_RouteMessage_Callback(void *vAppType, void *vData)
{
    struct RoutingAppListEntry *appType = vAppType;
    struct RoutingData *routingData = vData;

    rzb_log (LOG_INFO, "%s: Routing to queue: \"%s\"", __func__, appType->pQueue->sName);

    Datastore_Insert_Block_Inspection(routingData->dbCon, 
            routingData->insMessage->eventId, appType->uuidAppType);

    Statistics_Incr_AppType_RoutingRequest(appType->uuidAppType);

    if (!(Queue_Put (appType->pQueue, routingData->message)))
    {
        Statistics_Incr_AppType_RoutingError(appType->uuidAppType);
        rzb_log (LOG_ERR,
                 "%s: Failed to forward message to all data types detection queue", __func__);
        Datastore_Update_Block_Inspection_Status(routingData->dbCon, 
                routingData->insMessage->pBlock->pId, appType->uuidAppType, 
                NULL,  JUDGMENT_REASON_ERROR);
        routingData->error = true;
    }
    else
    {
        Statistics_Incr_AppType_RoutingSuccess(appType->uuidAppType);
        routingData->routed = true;
    }
    return LIST_EACH_OK;
}

bool
RoutingList_RouteMessage (struct DatabaseConnection *dbCon, struct Message *message,
                          uuid_t p_pDataType)
{
    ASSERT (p_pBlockType != NULL);
    ASSERT (p_pApplicationType != NULL);
    struct RoutingData routingData;
    struct RoutingListEntry *listEntry;
    memset(&routingData, 0, sizeof(struct RoutingData));

    routingData.message = message;
    routingData.insMessage = message->message;
    routingData.dbCon = dbCon;
    routingData.routed = false;
    routingData.error = false;

    char *dataType;

    dataType = UUID_Get_NameByUUID (p_pDataType, UUID_TYPE_DATA_TYPE);
    rzb_log (LOG_INFO, "%s: Attempting to route: %s", __func__, dataType);

    RoutingList_Lock ();

    listEntry = RoutingList_FindDataType (sg_uuidAllType);

    //Statistics_Incr_DataType_RoutingRequest(sg_uuidAllType);
    Statistics_Incr_DataType_RoutingRequest(p_pDataType);

    // Route to all data types, if registered
    if (listEntry != NULL )
    {
        List_ForEach(listEntry->appList, RoutingList_RouteMessage_Callback, &routingData);
    }


    // Route to specific data types
    listEntry = RoutingList_FindDataType (p_pDataType);
    if (listEntry == NULL)
    {
        if (routingData.routed == true)  // We routed it to all data types
        {
            if (routingData.error)
                Statistics_Incr_DataType_RoutingError(p_pDataType);
            else
                Statistics_Incr_DataType_RoutingSuccess(p_pDataType);
            // If there was an error
            RoutingList_Unlock ();
            free(dataType);
            return true;
        }
        else
        {                       // We failed to route it at all
            //rzb_log (LOG_ERR, "%s: Failed to route message, no route entry", __func__);
            Statistics_Incr_DataType_RoutingError(p_pDataType);
            RoutingList_Unlock ();
            free(dataType);
            return false;
        }
    }

    List_ForEach(listEntry->appList, RoutingList_RouteMessage_Callback, &routingData);
    if (routingData.routed == false)     // If we've failed to route at all, err
    {
        Statistics_Incr_DataType_RoutingError(p_pDataType);
        rzb_log (LOG_ERR, "%s: Failed to route data to any location", __func__);
        RoutingList_Unlock ();
        free(dataType);
        return false;
    }

    if (routingData.error)
        Statistics_Incr_DataType_RoutingError(p_pDataType);
    else
        Statistics_Incr_DataType_RoutingSuccess(p_pDataType);

    RoutingList_Unlock ();
    free(dataType);
    return true;
}

static int 
RoutingList_RemoveApp_Callback(void *vDataTypeEntry, void *vAppType)
{
    struct RoutingListEntry *dataTypeEntry = vDataTypeEntry;
    struct RoutingListAppEntry *appTypeEntry = NULL;
    unsigned char *appType = vAppType;
    
    appTypeEntry = List_Find(dataTypeEntry->appList, appType);
    if (appTypeEntry != NULL)
    {
        List_Remove(dataTypeEntry->appList, appTypeEntry);
        sg_serial++;
        DBus_Push_Update();
    }
    return LIST_EACH_OK;
}

bool
RoutingList_RemoveApplication (uuid_t appType)
{
    List_ForEach(g_routingList, RoutingList_RemoveApp_Callback, appType);
    return true;
}

bool
RoutingList_RemoveEntry (uuid_t dataType, uuid_t appType)
{
    struct RoutingListEntry *dataTypeEntry = NULL;
    char *dataTypeName = NULL;
    char *appTypeName = NULL;

    RoutingList_Lock ();
    dataTypeEntry = RoutingList_FindDataType (dataType);

    if (dataTypeEntry == NULL)
    {
        appTypeName = UUID_Get_NameByUUID (appType, UUID_TYPE_NUGGET);
        dataTypeName =UUID_Get_NameByUUID (dataType, UUID_TYPE_DATA_TYPE);
        rzb_log (LOG_INFO, "%s: Failed to remove %s:%s, datatype not found", __func__,
                 dataTypeName, appTypeName);
        RoutingList_Unlock ();
        free(appTypeName);
        free(dataTypeName);
        return false;
    }

    RoutingList_RemoveAppType (dataTypeEntry, appType);
    sg_serial++;
    DBus_Push_Update();
    RoutingList_Unlock ();
    return true;
}

int AppType_KeyCmp(void *a, void *id)
{
    struct RoutingAppListEntry * cA = (struct RoutingAppListEntry *)a;
    unsigned char *key = id;
    return uuid_compare(key,cA->uuidAppType);
}
int AppType_Cmp(void *a, void *b)
{
    struct RoutingAppListEntry * cA = (struct RoutingAppListEntry *)a;
    struct RoutingAppListEntry * cB = (struct RoutingAppListEntry *)b;
    if (a==b)
        return 0;
    return uuid_compare(cA->uuidAppType, cB->uuidAppType);
}
void AppType_Destroy(void *a)
{
    //struct RoutingAppListEntry * cA = (struct RoutingAppListEntry *)a;
    // TODO: Need queue ref counting for this
    //Queue_Terminate(ca->pQueue);
    free(a);
}
void * AppType_Clone(void *a)
{
    //XXX: We dont clone the Queue it does not seem worth it
    struct RoutingAppListEntry * cA = (struct RoutingAppListEntry *)a;
    struct RoutingAppListEntry *ret = NULL;
    if ((ret = calloc(1,sizeof(struct RoutingAppListEntry))) == NULL)
        return NULL;
    uuid_copy(ret->uuidAppType, cA->uuidAppType);
    return ret;
}

int DataType_KeyCmp(void *a, void *id)
{
    struct RoutingListEntry * cA = (struct RoutingListEntry *)a;
    unsigned char *key = id;
    return uuid_compare(key,cA->uuidDataType);
}

int DataType_Cmp(void *a, void *b)
{
    struct RoutingListEntry * cA = (struct RoutingListEntry *)a;
    struct RoutingListEntry * cB = (struct RoutingListEntry *)b;
    if (a==b)
        return 0;
    return uuid_compare(cA->uuidDataType, cB->uuidDataType);
}

void DataType_Destroy(void *a)
{
    struct RoutingListEntry * cA = (struct RoutingListEntry *)a;
    List_Destroy(cA->appList);
    free(a);
}
void * DataType_Clone(void *a)
{
    struct RoutingListEntry * cA = (struct RoutingListEntry *)a;
    struct RoutingListEntry *ret = NULL;
    if ((ret = calloc(1,sizeof(struct RoutingListEntry))) == NULL)
        return NULL;
    uuid_copy(ret->uuidDataType, cA->uuidDataType);
    ret->appList = List_Clone(cA->appList);
    if (ret->appList == NULL)
    {
        free(ret);
        return NULL;
    }
    return ret;
}

uint64_t 
RoutingList_GetSerial(void)
{
    uint64_t ret=0;
    RoutingList_Lock();
    ret = sg_serial;
    RoutingList_Unlock();
    return ret;
}

void
RoutingList_SetSerial(uint64_t serial)
{
    RoutingList_Lock();
    sg_serial = serial;
    RoutingList_Unlock();
}
