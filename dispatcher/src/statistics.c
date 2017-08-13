#include <stdio.h>
#include <string.h>
#include "statistics.h"
#include "razorback.h"
#include "config.h"
#include "dispatcher.h"
#include "routing_list.h"

#if WITH_SNMP
#include "snmp/agent.h"
#endif

void Stats_Thread (struct Thread *thread);
bool readyForWrite (struct StatsLog *file);
bool writeRouteStats (struct StatsLog *file);

static pthread_mutex_t sg_dataTypeLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sg_appTypeLock = PTHREAD_MUTEX_INITIALIZER;
struct DataTypeStats *g_stats_dataTypeHead = NULL;
struct AppTypeStats *g_stats_appTypeHead = NULL;

static int
Stats_Init_AppType(void *vUuid, void *o)
{
    struct UUIDListNode *node = (struct UUIDListNode *)vUuid;
    Statistics_Get_AppType(node->uuid);
    return LIST_EACH_OK;
}
static int
Stats_Init_DataType(void *vUuid, void *o)
{
    struct UUIDListNode *node = (struct UUIDListNode *)vUuid;
    Statistics_Get_DataType(node->uuid);
    return LIST_EACH_OK;
}
bool
Statistics_Initialize ()
{
    struct List *list;
    rzb_log (LOG_INFO, "%s: Initializing stats structures...", __func__);
    // start the thread/timer
    // Loop thought the data type and app type lists to pre-populate the data structures
    list = UUID_Get_List(UUID_TYPE_DATA_TYPE);
    List_ForEach(list, Stats_Init_DataType, NULL);
    list = UUID_Get_List(UUID_TYPE_NUGGET);
    List_ForEach(list, Stats_Init_AppType, NULL);
#if WITH_SNMP
    Thread_Launch(SNMP_Agent_Thread, NULL, (char *)"SNMP Thread", NULL);
#endif
    return true;
}

void
Statistics_Lock_AppTypeList()
{
    pthread_mutex_lock(&sg_appTypeLock);
}

void
Statistics_Unlock_AppTypeList()
{
    pthread_mutex_unlock(&sg_appTypeLock);
}

void
Statistics_Lock_DataTypeList()
{
    pthread_mutex_lock(&sg_dataTypeLock);
}

void
Statistics_Unlock_DataTypeList()
{
    pthread_mutex_unlock(&sg_dataTypeLock);
}

struct DataTypeStats *
Statistics_Get_DataType(uuid_t type)
{
    struct DataTypeStats *node;
    Statistics_Lock_DataTypeList();
    node = g_stats_dataTypeHead;
    while (node != NULL)
    {
        if (uuid_compare(type, node->uuid) == 0)
            break;
        node = node->next;
    }
    // Allocate a new node if required
    if (node == NULL)
    {
        if ((node = calloc(1, sizeof(struct DataTypeStats))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Malotile disfunction", __func__);
            Statistics_Unlock_DataTypeList();
            return NULL;
        }
        pthread_mutex_init(&node->lock, NULL);
        uuid_copy(node->uuid, type);
        node->name = UUID_Get_NameByUUID(type, UUID_TYPE_DATA_TYPE);
        node->next = g_stats_dataTypeHead;
        g_stats_dataTypeHead = node;
    }
    Statistics_Unlock_DataTypeList();
    return node;
}

struct AppTypeStats *
Statistics_Get_AppType(uuid_t type)
{
    struct AppTypeStats *node;
    Statistics_Lock_AppTypeList();
    node = g_stats_appTypeHead;
    while (node != NULL)
    {
        if (uuid_compare(type, node->uuid) == 0)
            break;
        node = node->next;
    }
    // Allocate a new node if required
    if (node == NULL)
    {
        if ((node = calloc(1, sizeof(struct AppTypeStats))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Malotile disfunction", __func__);
            Statistics_Unlock_AppTypeList();
            return NULL;
        }
        pthread_mutex_init(&node->lock, NULL);
        uuid_copy(node->uuid, type);
        node->name = UUID_Get_NameByUUID(type, UUID_TYPE_NUGGET);
        node->next = g_stats_appTypeHead;
        g_stats_appTypeHead = node;
    }
    Statistics_Unlock_AppTypeList();
    return node;
}

void Statistics_Incr_DataType_RoutingRequest(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->routingRequestCount++;
    pthread_mutex_unlock(&stats->lock);
}

void Statistics_Incr_DataType_RoutingSuccess(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->routingSuccessCount++;
    pthread_mutex_unlock(&stats->lock);
}
void Statistics_Incr_DataType_RoutingError(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->routingErrorCount++;
    pthread_mutex_unlock(&stats->lock);
}
void Statistics_Incr_DataType_ConsumerCount(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->consumerCount++;
    pthread_mutex_unlock(&stats->lock);
}

void Statistics_Incr_DataType_CacheRequest(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->cacheRequestCount++;
    pthread_mutex_unlock(&stats->lock);
}

void Statistics_Incr_DataType_CacheHit(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->cacheHitCount++;
    pthread_mutex_unlock(&stats->lock);
}
void Statistics_Incr_DataType_CacheCanHaz(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->cacheResponseCanHaz++;
    pthread_mutex_unlock(&stats->lock);
}
void Statistics_Incr_DataType_CacheError(uuid_t type)
{
    struct DataTypeStats * stats = Statistics_Get_DataType(type);
    pthread_mutex_lock(&stats->lock);
    stats->cacheErrorCount++;
    pthread_mutex_unlock(&stats->lock);
}

extern void Statistics_Incr_AppType_RoutingRequest(uuid_t type)
{
    struct AppTypeStats * stats = Statistics_Get_AppType(type);
    pthread_mutex_lock(&stats->lock);
    stats->routingRequestCount++;
    pthread_mutex_unlock(&stats->lock);
}
extern void Statistics_Incr_AppType_RoutingSuccess(uuid_t type)
{
    struct AppTypeStats * stats = Statistics_Get_AppType(type);
    pthread_mutex_lock(&stats->lock);
    stats->routingSuccessCount++;
    pthread_mutex_unlock(&stats->lock);
}
extern void Statistics_Incr_AppType_RoutingError(uuid_t type)
{
    struct AppTypeStats * stats = Statistics_Get_AppType(type);
    pthread_mutex_lock(&stats->lock);
    stats->routingErrorCount++;
    pthread_mutex_unlock(&stats->lock);
}

void
Stats_Thread (struct Thread *thread)
{
    if (thread == NULL)
        return;

    struct StatsLog statsFile;
    statsFile.name = (char *) "/tmp/rzb.stats";
    statsFile.fh = 0;

    if ((readyForWrite (&statsFile)) != true)
        return;

    while (!Dispatcher_Terminate ())
    {
        sleep (5);
        writeRouteStats (&statsFile);
        fprintf (statsFile.fh, "\n");
        fflush (statsFile.fh);
    }
    return;
}

bool
writeRouteStats (struct StatsLog * file)
{
#if 0
    struct DataTypeStats *currentDataType;
    currentDataType = g_stats->dataTypeHead;

    fprintf (file->fh,
             "R-DTC: %u, R-ATC: %u, R-REC: %u, R-RRC: %u, R-RSC: %u, ",
             g_stats->routing->dataTypeCount,
             g_stats->routing->appTypeCount,
             g_stats->routing->routeEntryCount,
             g_stats->routing->routeRequestCount,
             g_stats->routing->routeSuccessCount);

    while (currentDataType != NULL)
    {
        if (currentDataType->routing != NULL)
            fprintf (file->fh, "R-%s: %u, ", currentDataType->name,
                     currentDataType->routing->routedCount);
        currentDataType = currentDataType->next;
    }
#endif 
    return true;
}

bool
readyForWrite (struct StatsLog * file)
{
    file->fh = fopen (file->name, "a");
    if (file->fh == 0)
    {
        rzb_log (LOG_ERR, "%s: Unable to open stats file for writing", __func__);
        return false;
    }
    return true;
}


