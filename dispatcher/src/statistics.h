#ifndef RZB_STATS_H
#define RZB_STATS_H

#include <razorback/types.h>
#include <razorback/thread.h>
#include "config.h"
#include "routing_list.h"
#include <stdio.h>
#include <sys/stat.h>

struct DataTypeStats
{
    uuid_t uuid;
    char *name;
    uint32_t routingRequestCount;
    uint32_t routingSuccessCount;
    uint32_t routingErrorCount;
    uint32_t consumerCount;
    uint32_t cacheRequestCount;
    uint32_t cacheHitCount;
    uint32_t cacheResponseCanHaz;
    uint32_t cacheErrorCount;
    struct DataTypeStats *next;
    pthread_mutex_t lock;
};

struct AppTypeStats
{
    uuid_t uuid;
    char *name;
    uint32_t routingRequestCount;
    uint32_t routingSuccessCount;
    uint32_t routingErrorCount;
    uint32_t onlineCount;
    struct AppTypeStats *next;
    pthread_mutex_t lock;
};


struct StatsLog
{
    FILE *fh;                   // File Handle
    char *name;                 // Pointer to name of the file
};

extern struct DataTypeStats *g_stats_dataTypeHead;
extern struct AppTypeStats *g_stats_appTypeHead;

extern bool Statistics_Initialize ();

extern void Statistics_Lock_DataTypeList();
extern void Statistics_Unlock_DataTypeList();
extern void Statistics_Lock_AppTypeList();
extern void Statistics_Unlock_AppTypeList();

extern struct AppTypeStats *Statistics_Get_AppType(uuid_t type);
extern struct DataTypeStats *Statistics_Get_DataType(uuid_t type);

extern void Statistics_Incr_DataType_RoutingRequest(uuid_t type);
extern void Statistics_Incr_DataType_RoutingSuccess(uuid_t type);
extern void Statistics_Incr_DataType_RoutingError(uuid_t type);
extern void Statistics_Incr_DataType_ConsumerCount(uuid_t type);
extern void Statistics_Decr_DataType_ConsumerCount(uuid_t type);

extern void Statistics_Incr_DataType_CacheRequest(uuid_t type);
extern void Statistics_Incr_DataType_CacheHit(uuid_t type);
extern void Statistics_Incr_DataType_CacheCanHaz(uuid_t type);
extern void Statistics_Incr_DataType_CacheError(uuid_t type);

extern void Statistics_Incr_AppType_RoutingRequest(uuid_t type);
extern void Statistics_Incr_AppType_RoutingSuccess(uuid_t type);
extern void Statistics_Incr_AppType_RoutingError(uuid_t type);

#endif /* RZB_STATS_H */
