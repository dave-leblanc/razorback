#include <razorback/socket.h>
#include <razorback/thread.h>
#include <razorback/thread_pool.h>
#include <razorback/ntlv.h>
#include <razorback/log.h>
#include <razorback/connected_entity.h>
#include <razorback/messages.h>
#include <razorback/nugget.h>
#include <string.h>

#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "global_cache.h"
#include "routing_list.h"
#include "console.h"
#include "judgment_processor.h"
#include "configuration.h"
#include "requeue.h"

#include <libcli.h>

struct DatabaseConnection *sg_dbCon;

#define MODE_CONFIG_NUGGET 10
#define MODE_CONFIG_APP_TYPE 11
#define DECL_COMMAND(x) static int x(struct cli_def *cli, const char *command, char *argv[], int argc)

int
check_auth (const char *username, const char *password)
{
    // TODO: Add support for other auth backends
    if (strcasecmp (username, g_sConsoleUser) != 0)
        return CLI_ERROR;
    if (strcasecmp (password, g_sConsolePassword) != 0)
        return CLI_ERROR;
    return CLI_OK;
}

int
check_enable (const char *password)
{
    // TODO: Add support for other auth backends
    return !strcasecmp (password, g_sConsoleEnablePassword);
}

static int
Console_Print_Nugget (struct ConnectedEntity *p_pEntity, void *userData)
{
    char nugUuid[UUID_STRING_LENGTH];
    char *nugType;
    char *appType;
    const char *state;
    time_t l_tTimeNow;
    struct cli_def *cli = Thread_GetCurrent()->pUserData;
    uint32_t nuggetState = 0;
    l_tTimeNow = time (NULL);
    uuid_t dispatcher;

//    nugType =
        Datastore_Get_UUID_Name_By_UUID (sg_dbCon, p_pEntity->uuidNuggetType,
                             UUID_TYPE_NUGGET_TYPE, &nugType);
 //   appType =
        Datastore_Get_UUID_Name_By_UUID (sg_dbCon, p_pEntity->uuidApplicationType,
                             UUID_TYPE_NUGGET, &appType);
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);
    if (uuid_compare(p_pEntity->uuidNuggetType, dispatcher) == 0)
        return LIST_EACH_OK;

    uuid_unparse (p_pEntity->uuidNuggetId, nugUuid);
    Datastore_Get_Nugget_State(sg_dbCon, p_pEntity->uuidNuggetId, &nuggetState);
    switch (nuggetState)
    {
    case NUGGET_STATE_REQUESTEDREGISTER:
        state = NUGGET_STATE_DESC_REQUESTEDREGISTER;
        break;
    case NUGGET_STATE_REGISTERING:
        state = NUGGET_STATE_DESC_REGISTERING;
        break;
    case NUGGET_STATE_CONFIGURING:
        state = NUGGET_STATE_DESC_CONFIGURING;
        break;
    case NUGGET_STATE_PAUSED:
        state = NUGGET_STATE_DESC_PAUSED;
        break;
    case NUGGET_STATE_RUNNING:
        state = NUGGET_STATE_DESC_RUNNING;
        break;
    case NUGGET_STATE_TERMINATED:
        state = NUGGET_STATE_DESC_TERMINATED;
        break;
    case NUGGET_STATE_ERROR:
        state = NUGGET_STATE_DESC_ERROR;
        break;
    case NUGGET_STATE_REREG:
        state = NUGGET_STATE_DESC_REREG;
        break;
	default:
        state = "Unknown";
        break;
    }
    if (p_pEntity->tTimeOfLastHello > 0)
    {
        cli_print (cli, (char *) "%-20s %-20s %s %-11s %d", nugType,
                   appType, nugUuid, state,
                   (int) (l_tTimeNow - p_pEntity->tTimeOfLastHello));
    }
    else
    {
        cli_print (cli, (char *) "%-20s %-20s %s %-11s (Unknown)", nugType,
                   appType, nugUuid, state);
    }
    free (nugType);
    free (appType);
    return LIST_EACH_OK;
}

DECL_COMMAND(Console_Command_Show_Nugget_Status)
{
    struct Thread *thread = Thread_GetCurrent();
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show nugget status");
        return CLI_OK;
    }
    cli_print (cli, (char *) "Show Nugget Status");
    cli_print (cli, (char *) "%-20s %-20s %-36s %-11s %s", "Nugget Type",
               "App Type", "Nugget ID", "Status", "Dead Time");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    thread->pUserData = cli;
    ConnectedEntityList_ForEach_Entity (Console_Print_Nugget, NULL);
    thread->pUserData = NULL;
    cli_print (cli,
               (char *)
               "====================================================================================================");
    return CLI_OK;
}
static int
Console_Print_Dispatcher (struct ConnectedEntity *entity, void *userData)
{
    char nugUuid[UUID_STRING_LENGTH];
    const char *state = NULL;
    time_t l_tTimeNow;
    struct cli_def *cli = Thread_GetCurrent()->pUserData;
    l_tTimeNow = time (NULL);
    uuid_t dispatcher;

    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);
    if (uuid_compare(entity->uuidNuggetType, dispatcher) != 0)
        return LIST_EACH_OK;

    uuid_unparse (entity->uuidNuggetId, nugUuid);
    if ((entity->dispatcher->flags & DISP_HELLO_FLAG_DD) != 0)
    {
        if ((entity->dispatcher->flags & DISP_HELLO_FLAG_LM) != 0)
            state = "Dedicated, Master";
        else if ((entity->dispatcher->flags & DISP_HELLO_FLAG_LS) != 0)
            state = "Dedicated, Slave";
    }
    else 
    {
        if ((entity->dispatcher->flags & DISP_HELLO_FLAG_LM) != 0)
            state = "Master";
        else if ((entity->dispatcher->flags & DISP_HELLO_FLAG_LS) != 0)
            state = "Slave";
    }

    if (entity->tTimeOfLastHello > 0)
    {
        cli_print (cli, (char *) "%s %-8d %-20s %d", nugUuid, entity->locality, state,
                   (int) (l_tTimeNow - entity->tTimeOfLastHello));
    }
    else
    {
        cli_print (cli, (char *) "%s %-8d %-20s (Unknown)", nugUuid, entity->locality, state);
    }
    return LIST_EACH_OK;
}

DECL_COMMAND(Console_Command_Show_Dispatcher_Neighbours)
{
    struct Thread *thread = Thread_GetCurrent();
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show dispatcher neighbors");
        return CLI_OK;
    }
    cli_print (cli, (char *) "Dispatcher Neighbors");
    cli_print (cli, (char *) "%-36s %-8s %-20s %s", "Nugget ID", "Locality", "Status", "Dead Time");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    thread->pUserData = cli;
    ConnectedEntityList_ForEach_Entity (Console_Print_Dispatcher, NULL);
    thread->pUserData = NULL;
    cli_print (cli,
               (char *)
               "====================================================================================================");
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Show_Dispatcher_Status)
{
    char nugUuid[UUID_STRING_LENGTH];
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show dispatcher status");
        return CLI_OK;
    }
    uuid_unparse (sg_rbContext.uuidNuggetId, nugUuid);
    cli_print (cli, (char *) "Dispatcher Status");
    cli_print (cli, (char *) 
               "====================================================================================================");
    cli_print (cli, (char *)"Nugget_ID: %s", nugUuid);
    cli_print (cli, (char *)"Locality: %d", sg_rbContext.locality);
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) != 0)
    {
        if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0)
            cli_print (cli, (char *)"Status: Dedicated, Master");
        else if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) != 0)
            cli_print (cli, (char *)"Status: Dedicated, Slave");
    }
    else 
    {
        if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0)
            cli_print (cli, (char *)"Status: Master");
        else if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) != 0)
            cli_print (cli, (char *)"Status: Slave");
    }
    cli_print(cli, (char*)"Transfer Protocol: %d", sg_rbContext.dispatcher.protocol);
    cli_print(cli, (char*)"Transfer Port: %d", sg_rbContext.dispatcher.port);
    // Todo - Print advertised hostnames
    //
    cli_print (cli,
               (char *)
               "====================================================================================================");
    return CLI_OK;
}


DECL_COMMAND(Console_Command_Show_Nugget_Registrations)
{
    struct Nugget **nuggets;
    uint64_t count;
    char nugUuid[UUID_STRING_LENGTH];
    char *nugType = NULL;
    char *appType = NULL;
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show nugget registrations");
        return CLI_OK;
    }
    cli_print (cli, (char *) "Show Nugget Registrations");
    cli_print (cli, (char *) "%-36s %-20s %-20s %s", "Nugget ID", "Nugget Type",
               "App Type", "Name");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    if (Datastore_Get_Nuggets(sg_dbCon, &nuggets, &count) != R_SUCCESS)
    {
        cli_print(cli, (char *)"Database Error: See dispatcher log for details.");
        return CLI_OK;
    }
    for (uint64_t i =0; i < count; i++)
    {

            Datastore_Get_UUID_Name_By_UUID (sg_dbCon, nuggets[i]->uuidNuggetType,
                                 UUID_TYPE_NUGGET_TYPE, &nugType);
            Datastore_Get_UUID_Name_By_UUID (sg_dbCon, nuggets[i]->uuidApplicationType,
                                 UUID_TYPE_NUGGET, &appType);
        uuid_unparse (nuggets[i]->uuidNuggetId, nugUuid);
        cli_print (cli, (char *) "%s %-20s %-20s %s", nugUuid, nugType,
               appType, nuggets[i]->sName);
        Nugget_Destroy(nuggets[i]); 
        free(appType);
        free(nugType);
    }
    free(nuggets);
    cli_print (cli,
               (char *)
               "====================================================================================================");
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Show_Nugget_AppTypes)
{
    struct  AppType *appTypes;
    uint64_t count;
    char uuid[UUID_STRING_LENGTH];
    char *nugType = NULL;
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show nugget apptypes");
        return CLI_OK;
    }
    cli_print (cli, (char *) "Show Nugget Application Types");
    cli_print (cli, (char *) "%-36s %-20s %s", "Type ID", "Nugget Type",
               "Name");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    if (Datastore_Get_AppTypes(sg_dbCon, &appTypes, &count) != R_SUCCESS)
    {
        cli_print(cli, (char *)"Database Error: See dispatcher log for details.");
        return CLI_OK;
    }
    for (uint64_t i =0; i < count; i++)
    {

//        nugType = (char *)
            Datastore_Get_UUID_Name_By_UUID (sg_dbCon, appTypes[i].uuidNuggetType,
                                 UUID_TYPE_NUGGET_TYPE, &nugType);
        uuid_unparse (appTypes[i].uuidId, uuid);
        cli_print (cli, (char *) "%s %-20s %s", uuid, nugType,
               appTypes[i].sName);
        
        free(nugType);
    }
    free(appTypes);
    cli_print (cli,
               (char *)
               "====================================================================================================");
    return CLI_OK;
}

static int
Console_Print_UUID(void *vUuid, void *vCli)
{
    char uuid[UUID_STRING_LENGTH];
    struct UUIDListNode *current = (struct UUIDListNode *)vUuid;
    struct cli_def *cli = (struct cli_def *)vCli;

    uuid_unparse (current->uuid, uuid);
    cli_print (cli, (char *) "%s %-20s %s", uuid, current->sName,
           current->sDescription);
    return LIST_EACH_OK;
}

DECL_COMMAND(Console_Command_Show_UUIDs)
{
    struct List* list;
    int type;

    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: %s", command);
        return CLI_OK;
    }
    if (strcmp(command, "show datatypes") == 0)
    {
        cli_print (cli, (char *) "Show Data Types");
        type = UUID_TYPE_DATA_TYPE;
    }
    else if (strcmp(command, "show metadata types") == 0)
    {
        cli_print (cli, (char *) "Show Metadata Types");
        type = UUID_TYPE_NTLV_TYPE;
    }
    else
    {
        cli_print (cli, (char *) "Show Metadata Names");
        type = UUID_TYPE_NTLV_NAME;
    }
    Datastore_Get_UUIDs(sg_dbCon, type, &list);

    cli_print (cli, (char *) "%-36s %-20s %s", "Type ID", "Name",
               "Description");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    List_ForEach(list, Console_Print_UUID, cli);
    cli_print (cli,
               (char *)
               "====================================================================================================");
    List_Destroy(list);
    return CLI_OK;
}

static int
Console_Print_ThreadPool_Thread(void *vItem, void *vUserData)
{
    struct ThreadPoolItem *item = vItem;
    struct cli_def *cli = vUserData;
    cli_print(cli, (char *)"%-10d %s", item->id, item->thread->sName);
    return LIST_EACH_OK;
}
void Console_Print_ThreadPool(struct cli_def *cli, struct ThreadPool *pool)
{
    cli_print (cli, (char *) "%-10s %-20s", "Thread ID", "Thread Name");
    cli_print (cli, (char *) "=========================================");
    List_ForEach(pool->list, Console_Print_ThreadPool_Thread, cli);
    cli_print (cli, (char *) "=========================================");

}
DECL_COMMAND(Console_Command_Show_Threads)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show threads");
        return CLI_OK;
    }
    cli_print (cli, (char *) "Show Threads");
    cli_print (cli, (char *) "Cache Threads");
    Console_Print_ThreadPool(cli, g_cacheThreadPool);
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) "Judgment Threads");
    Console_Print_ThreadPool(cli, g_judgmentThreadPool);
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) "Log Threads");
    Console_Print_ThreadPool(cli, g_logThreadPool);
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) "Submission Threads");
    Console_Print_ThreadPool(cli, g_submissionThreadPool);
    cli_print (cli, (char *) " ");
    return CLI_OK;
}

struct Console_RT_Context
{
    struct RoutingListEntry *dataType;
    struct cli_def *cli;
};

static int 
Console_Print_AppType(void *vAppType, void *userData)
{
    struct RoutingAppListEntry *appType = vAppType;
    struct Console_RT_Context *ctx = userData;
    char *dataTypeName = NULL;
    char *appTypeName = NULL;
    Datastore_Get_UUID_Name_By_UUID (sg_dbCon, ctx->dataType->uuidDataType, UUID_TYPE_DATA_TYPE, &dataTypeName);
    Datastore_Get_UUID_Name_By_UUID (sg_dbCon, appType->uuidAppType, UUID_TYPE_NUGGET, &appTypeName);
    cli_print (ctx->cli, (char *) "%-20s %-20s", dataTypeName, appTypeName);
    free (appTypeName);
    free (dataTypeName);
    return LIST_EACH_OK;
}
static int 
Console_Print_DataType(void *vDataType, void *userData)
{
    struct Console_RT_Context *ctx = userData;
    ctx->dataType = vDataType;
    List_ForEach(ctx->dataType->appList, Console_Print_AppType, userData);
    return LIST_EACH_OK;
}
DECL_COMMAND(Console_Command_Show_Routes)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: show routes");
        return CLI_OK;
    }
    struct Console_RT_Context ctx;
    uint64_t serial =0;
    RoutingList_Lock();
    serial = RoutingList_GetSerial();

    ctx.cli = cli;
    cli_print (cli, (char *) "Routing Table");
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) "Serial: %ju", serial);
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) "%-20s %-20s", "Data Type", "App Type");
    cli_print (cli, (char *) "=========================================");
    List_ForEach(g_routingList, Console_Print_DataType, &ctx);
    cli_print (cli, (char *) "=========================================");
    RoutingList_Unlock();
    return CLI_OK;
}
static int
Console_Print_Cache_Stats(void *vUuid, void *vCli)
{
    struct DataTypeStats *statsNode;
    struct UUIDListNode *current = (struct UUIDListNode *)vUuid;
    struct cli_def *cli = (struct cli_def *)vCli;

    statsNode = Statistics_Get_DataType(current->uuid);
    pthread_mutex_lock(&statsNode->lock);
    cli_print (cli, (char *) "%-20s %10u %10u %10u %10u", current->sName,
            statsNode->cacheRequestCount,
            statsNode->cacheHitCount,
            statsNode->cacheResponseCanHaz,
            statsNode->cacheErrorCount);
    pthread_mutex_unlock(&statsNode->lock);

    return LIST_EACH_OK;
}

DECL_COMMAND(Console_Command_Show_Cache_Stats)
{
    struct List* list;

    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: %s", command);
        return CLI_OK;
    }
    Datastore_Get_UUIDs(sg_dbCon, UUID_TYPE_DATA_TYPE, &list);
    cli_print (cli, (char *) "Data Types");
    cli_print (cli, (char *) "%-20s %10s %10s %10s %10s", "Name", "Requests", "Hit", "Can Haz", "Error");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    List_ForEach(list, Console_Print_Cache_Stats, cli);
    cli_print (cli,
               (char *)
               "====================================================================================================");
    List_Destroy(list);
    return CLI_OK;
}
static int
Console_Print_DataType_Stats(void *vUuid, void *vCli)
{
    struct DataTypeStats *statsNode;
    struct UUIDListNode *current = (struct UUIDListNode *)vUuid;
    struct cli_def *cli = (struct cli_def *)vCli;
    if (uuid_is_null(current->uuid) == 1)
        return LIST_EACH_OK;

    statsNode = Statistics_Get_DataType(current->uuid);
    pthread_mutex_lock(&statsNode->lock);
    cli_print (cli, (char *) "%-20s %-10u %-10u %-10u", current->sName,
            statsNode->routingRequestCount,
            statsNode->routingSuccessCount,
            statsNode->routingErrorCount);
    pthread_mutex_unlock(&statsNode->lock);

    return LIST_EACH_OK;
}
DECL_COMMAND(Console_Command_Show_Routing_Stats)
{
    struct List* list;
    struct AppTypeStats *appStatsNode;
    struct  AppType *appTypes;
    uuid_t inspector;
    uint64_t count;
    char *nugType = NULL;

    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: %s", command);
        return CLI_OK;
    }
    Datastore_Get_UUID_By_Name(sg_dbCon, NUGGET_TYPE_INSPECTION, UUID_TYPE_NUGGET_TYPE, inspector);
    Datastore_Get_UUIDs(sg_dbCon, UUID_TYPE_DATA_TYPE, &list);
    cli_print (cli, (char *) "Data Types");
    cli_print (cli, (char *) "%-20s %-10s %-10s %-10s", "Name", "Request", "Success", "Error");
    cli_print (cli,
               (char *)
               "====================================================================================================");
    List_ForEach(list, Console_Print_DataType_Stats, cli);
    cli_print (cli,
               (char *)
               "====================================================================================================");
    List_Destroy(list);


    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) " ");
    cli_print (cli, (char *) "App Types");
    cli_print (cli, (char *) "%-20s %-10s %-10s %-10s", "Name", "Request", "Success", "Error");

    cli_print (cli,
               (char *)
               "====================================================================================================");
    if (Datastore_Get_AppTypes(sg_dbCon, &appTypes, &count) != R_SUCCESS)
    {
        cli_print(cli, (char *)"Database Error: See dispatcher log for details.");
        return CLI_OK;
    }
    for (uint64_t i =0; i < count; i++)
    {

        if (uuid_compare(inspector, appTypes[i].uuidNuggetType) != 0)
            continue;

        Datastore_Get_UUID_Name_By_UUID (sg_dbCon, appTypes[i].uuidId,
                                 UUID_TYPE_NUGGET, &nugType);
        appStatsNode = Statistics_Get_AppType(appTypes[i].uuidId);
        cli_print (cli, (char *) "%-20s %-10u %-10u %-10u", nugType,
                appStatsNode->routingRequestCount,
                appStatsNode->routingSuccessCount,
                appStatsNode->routingErrorCount);
        
        free(nugType);
        if (appTypes[i].sName != NULL)
            free(appTypes[i].sName);
        if (appTypes[i].sDescription != NULL)
            free(appTypes[i].sDescription);
    }
    free(appTypes);
    cli_print (cli,
               (char *)
               "====================================================================================================");
    return CLI_OK;
}
DECL_COMMAND(Console_Command_Dispatcher_Promote)
{
    pthread_mutex_lock(&contextLock);
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LS) == 0)
    {
        cli_print(cli, (char *)"Can't promote a master dispatcher");
        pthread_mutex_unlock(&contextLock);
        return CLI_OK;
    }
    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LM;
    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LF;
    sg_rbContext.dispatcher.flags &= ~DISP_HELLO_FLAG_LS;
    pthread_mutex_unlock(&contextLock);
    return CLI_OK;
}
DECL_COMMAND(Console_Command_Dispatcher_Demote)
{
    pthread_mutex_lock(&contextLock);
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) == 0)
    {
        cli_print(cli, (char *)"Can't demote a slave dispatcher");
        pthread_mutex_unlock(&contextLock);
        return CLI_OK;
    }
    if (!ConnectedEntityList_SlaveInLocality(sg_rbContext.locality))
    {
		cli_print(cli, (char *)"Can't demote a master without a running slave dispatcher");
        pthread_mutex_unlock(&contextLock);
        return CLI_OK;
    }
    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LS;
    sg_rbContext.dispatcher.flags |= DISP_HELLO_FLAG_LF;
    sg_rbContext.dispatcher.flags &= ~DISP_HELLO_FLAG_LM;
    updateDispatcherState();
    pthread_mutex_unlock(&contextLock);
    return CLI_OK;
}


DECL_COMMAND(Console_Command_Flush_Cache_Global)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: flush cache global");
        return CLI_OK;
    }
    if (clearGlobalEntry (ALL, FULL) == R_SUCCESS)
        cli_print (cli, (char *) "Flush global cache: Success");
    else
        cli_print (cli, (char *) "Flush global cache: Failed");

    return CLI_OK;
}

DECL_COMMAND(Console_Command_Flush_Cache_Local)
{
    struct Message *msg;
    struct RazorbackContext *context;
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: flush cache local");
        return CLI_OK;
    }

    context = Thread_GetCurrentContext ();
    cli_print (cli, (char *) "Flush local cache: Success");
    if ((msg =MessageCacheClear_Initialize ( context->uuidNuggetId)) == NULL)
    {
        cli_print (cli, (char *) "Flush local cache: Failed (create message)");
        return CLI_OK;
    }

    if (!Queue_Put (g_ccWrite, msg))
    {
        cli_print (cli, (char *) "Flush local cache: Failed (send message)");
    }
    msg->destroy(msg);
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Flush_Cache_All)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: flush cache all");
        return CLI_OK;
    }
    Console_Command_Flush_Cache_Global (cli, command, argv, argc);
    Console_Command_Flush_Cache_Local (cli, command, argv, argc);
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Launch_CacheProcessor)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: launch cacheProcessor");
        return CLI_OK;
    }
    if (ThreadPool_LaunchWorker(g_cacheThreadPool))
        cli_print (cli, (char *) "Launched new cache processing thread");
    else
        cli_print (cli, (char *) "Failed to launch new cache processing thread");
    return CLI_OK;
}
DECL_COMMAND(Console_Command_Launch_JudgmentProcessor)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: launch judgmentProcessor");
        return CLI_OK;
    }
    if (ThreadPool_LaunchWorker(g_judgmentThreadPool))
        cli_print (cli, (char *) "Launched new judgment processing thread");
    else
        cli_print (cli, (char *) "Faile to launch new judgment processing thread");
    return CLI_OK;
}
DECL_COMMAND(Console_Command_Launch_LogProcessor)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: launch logProcessor");
        return CLI_OK;
    }
    if (ThreadPool_LaunchWorker(g_logThreadPool))
        cli_print (cli, (char *) "Launched new log processing thread");
    else
        cli_print (cli, (char *) "Failed to launch new log processing thread");
    return CLI_OK;
}
DECL_COMMAND(Console_Command_Launch_SubmissionProcessor)
{
    if (argc > 0)
    {
        cli_print (cli, (char *) "Usage: launch submissionProcessor");
        return CLI_OK;
    }
    if (ThreadPool_LaunchWorker(g_submissionThreadPool))
        cli_print (cli, (char *) "Launched new submission processing thread");
    else
        cli_print (cli, (char *) "Failed to launch new submission processing thread");
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Requeue_Block)
{
    struct RequeueRequest *req;
    const char *pos;
    size_t i;
    if (argc >= 1 || argc == 0)
    {
        if (argc == 0 || argv[argc-1][strlen(argv[argc-1])-1] == '?' || argc != 2)
        {
            cli_print(cli, (char *)"Usage: %s <Hash> <Size>", command);
            return CLI_OK;
        }
    }
    if (!sg_rbContext.regOk)
    {
        cli_print(cli, (char *)"Dispatcher not yet fully operational!");
        cli_print(cli, (char *)"Can't process request at this time.");
        return CLI_OK;
    }
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) == 0)
    {
        cli_print (cli, (char *)"Dispatcher not master!");
        cli_print (cli, (char *)"Can't process request at this time.");
        return CLI_OK;
    }
    if ((req = calloc(1, sizeof(struct RequeueRequest))) == NULL)
    {
        cli_print(cli, (char *)"Failed to allocate request");
        return CLI_OK;
    }
    if ((req->blockId = calloc(1,sizeof(struct BlockId))) == NULL)
    {
        cli_print(cli, (char *)"Failed to allocate request block id");
        return CLI_OK;
    }
    sscanf(argv[1], "%ju", &req->blockId->iLength);

    if ((req->blockId->pHash = Hash_Create()) == NULL)
    {
        cli_print(cli, (char *)"Failed to allocate hash");
        return CLI_OK;
    }
    Hash_Update(req->blockId->pHash, (uint8_t *)"a", 1);
    Hash_Finalize(req->blockId->pHash);
    pos = argv[0];
    for(i = 0; i < req->blockId->pHash->iSize; i++) 
    {
        sscanf(pos, "%2hhx", &req->blockId->pHash->pData[i]);
        pos += 2;
    }
    req->blockId->pHash->iFlags = HASH_FLAG_FINAL;
    cli_print(cli, (char *)"%s", Hash_ToText(req->blockId->pHash));

    req->type=REQUEUE_TYPE_BLOCK;
    
    Requeue_Request(req);

    return CLI_OK;
}

DECL_COMMAND(Console_Command_Requeue_Status)
{
    struct RequeueRequest *req;
    if (argc >= 1 || argc == 0)
    {
        if (argc == 0 || argv[argc-1][strlen(argv[argc-1])-1] == '?' || argc != 1)
        {
            cli_print(cli, (char *)"Usage: %s <AppType|ALL>", command);
            return CLI_OK;
        }
    }

    if (!sg_rbContext.regOk)
    {
        cli_print(cli, (char *)"Dispatcher not yet fully operational!");
        cli_print(cli, (char *)"Can't process request at this time.");
        return CLI_OK;
    }
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) == 0)
    {
        cli_print (cli, (char *)"Dispatcher not master!");
        cli_print (cli, (char *)"Can't process request at this time.");
        return CLI_OK;
    }
    if ((req = calloc(1, sizeof(struct RequeueRequest))) == NULL)
    {
        cli_print(cli, (char *)"Failed to allocate request");
        return CLI_OK;
    }
    if (strcmp(command, "requeue errors") == 0)
        req->type=REQUEUE_TYPE_ERROR;
    else 
        req->type=REQUEUE_TYPE_PENDING;

    if (strcmp(argv[0], "ALL") != 0)
    {
        if (Datastore_Get_UUID_By_Name(sg_dbCon, argv[0], UUID_TYPE_NUGGET, req->appType) != R_FOUND)
        {
            cli_print(cli, (char *)"Invalid App Type: %s", argv[0]);
            free(req);
            return CLI_OK;
        }
    }

    Requeue_Request(req);
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Register_UUID)
{
    uuid_t uuid;
    int type;
    if (argc >= 1 || argc == 0)
    {
        if (argc == 0 || argv[argc-1][strlen(argv[argc-1])-1] == '?' || argc != 3)
        {
            cli_print(cli, (char *)"Usage: %s <NAME> <UUID> <Description>", command);
            return CLI_OK;
        }
    }
    if (strcmp(argv[1], "gen") == 0)
    {
        uuid_generate(uuid);
        char c[UUID_STRING_LENGTH];
        uuid_unparse(uuid, c);
        cli_print(cli, (char *)"Generated UUID: %s", c);
    }
    else
    {
        if (uuid_parse(argv[1], uuid) == -1)
        {
            cli_print(cli, (char *)"Invalid UUID: %s", argv[1]);
            return CLI_OK;
        }
    }
    if (strcmp(command, "register datatype") == 0)
        type = UUID_TYPE_DATA_TYPE;
    else if (strcmp(command, "register metadata type") == 0)
        type = UUID_TYPE_NTLV_TYPE;
    else
        type = UUID_TYPE_NTLV_NAME;

    Datastore_Insert_UUID(sg_dbCon, argv[0], uuid, argv[2], type);
    Datastore_Load_UUID_List(sg_dbCon, type);

    if (type == UUID_TYPE_DATA_TYPE) // Populate the stats tree
        Statistics_Get_DataType(uuid);

    return CLI_OK;
}

DECL_COMMAND(Console_Command_Register_AppType)
{
    uuid_t uuid;
    if (argc >= 1 || argc == 0)
    {
        if (argc == 0 || argv[argc-1][strlen(argv[argc-1])-1] == '?' || argc != 4)
        {
            cli_print(cli, (char *)"Usage: %s <NAME> <TYPE> <UUID> <Description>", command);
            return CLI_OK;
        }
    }
    if (Datastore_Get_UUID_By_Name(sg_dbCon, argv[1], UUID_TYPE_NUGGET_TYPE, uuid) != R_FOUND)
    {
        cli_print(cli, (char *)"Invalid Nugget Type: %s", argv[1]);
        return CLI_OK;
    }
    if (strcmp(argv[2], "gen") == 0)
    {
        uuid_generate(uuid);
        char c[UUID_STRING_LENGTH];
        uuid_unparse(uuid, c);
        cli_print(cli, (char *)"Generated UUID: %s", c);
    }
    else
    {
        if (uuid_parse(argv[2], uuid) == -1)
        {
            cli_print(cli, (char *)"Invalid UUID: %s", argv[2]);
            return CLI_OK;
        }
    }

    Datastore_Insert_App_Type(sg_dbCon, argv[0], uuid, argv[3], argv[1]);    
    Datastore_Load_UUID_List(sg_dbCon, UUID_TYPE_NUGGET);
    // Populate the stats tree
    Statistics_Get_AppType(uuid);
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Register_Nugget_InspOrCol)
{
    uuid_t uuid, uuidAppType, uuidNuggetType;
    if (strcmp(command, "register nugget collection") == 0)
        Datastore_Get_UUID_By_Name(sg_dbCon, NUGGET_TYPE_COLLECTION, UUID_TYPE_NUGGET_TYPE, uuidNuggetType);
    else
        Datastore_Get_UUID_By_Name(sg_dbCon, NUGGET_TYPE_INSPECTION, UUID_TYPE_NUGGET_TYPE, uuidNuggetType);

    if (argc >= 1 || argc == 0)
    {
        if (argc == 0 || argv[argc-1][strlen(argv[argc-1])-1] == '?' || argc != 3)
        {
            cli_print(cli, (char *)"Usage: %s <App_Type> <NAME> <UUID>", command);
            return CLI_OK;
        }
    }

    if (Datastore_Get_UUID_By_Name(sg_dbCon, argv[0], UUID_TYPE_NUGGET, uuidAppType) != R_FOUND)
    {
        cli_print(cli, (char *)"Invalid Nugget Type: %s", argv[0]);
        return CLI_OK;
    }

    if (strcmp(argv[2], "gen") == 0)
    {
        uuid_generate(uuid);
        char c[UUID_STRING_LENGTH];
        uuid_unparse(uuid, c);
        cli_print(cli, (char *)"Generated UUID: %s", c);
    }
    else
    {
        if (uuid_parse(argv[2], uuid) == -1)
        {
            cli_print(cli, (char *)"Invalid UUID: %s", argv[2]);
            return CLI_OK;
        }
    }
    Datastore_Insert_Nugget(sg_dbCon, argv[1], uuid, uuidAppType, uuidNuggetType);    
    return CLI_OK;
}

DECL_COMMAND(Console_Command_Register_Nugget_Master)
{
    uuid_t uuid, uuidAppType, uuidNuggetType;
    Datastore_Get_UUID_By_Name(sg_dbCon, NUGGET_TYPE_MASTER, UUID_TYPE_NUGGET_TYPE, uuidNuggetType);
    uuid_clear(uuidAppType);

    if (argc >= 1 || argc == 0)
    {
        if (argc == 0 || argv[argc-1][strlen(argv[argc-1])-1] == '?' || argc != 2)
        {
            cli_print(cli, (char *)"Usage: %s <NAME> <UUID>", command);
            return CLI_OK;
        }
    }
#if 0
    cli_print(cli, (char *) "Arg Count: %i", argc);
    for (int i=0; i < argc; i++)
    {
        cli_print (cli, (char *) "Arg[%i]: %s", i, argv[i]);
    }
#endif
    if (strcmp(argv[1], "gen") == 0)
    {
        uuid_generate(uuid);
        char c[UUID_STRING_LENGTH];
        uuid_unparse(uuid, c);
        cli_print(cli, (char *)"Generated UUID: %s", c);
    }
    else
    {
        if (uuid_parse(argv[1], uuid) == -1)
        {
            cli_print(cli, (char *)"Invalid UUID: %s", argv[1]);
            return CLI_OK;
        }
    }
    Datastore_Insert_Nugget(sg_dbCon, argv[0], uuid, uuidAppType, uuidNuggetType);    
    return CLI_OK;
}

DECL_COMMAND(Console_Config_Nugget_Name)
{
    struct Nugget *nugget = Thread_GetCurrent()->pUserData;
    if (argc != 1 || (argv[0][strlen(argv[0])-1] == '?'))
    {
        cli_print(cli, (char *)"Usage: %s <String>", command);
        return CLI_OK;
    }
    free(nugget->sName);

    if (asprintf(&nugget->sName, "%s", argv[0]) == -1)
        nugget->sName = NULL;

    return CLI_OK;
}

DECL_COMMAND(Console_Config_Nugget_Contact)
{
    struct Nugget *nugget = Thread_GetCurrent()->pUserData;
    if (argc != 1 || (argv[0][strlen(argv[0])-1] == '?'))
    {
        cli_print(cli, (char *)"Usage: %s <String>", command);
        return CLI_OK;
    }
    free(nugget->sContact);

    if (asprintf(&nugget->sContact, "%s", argv[0]) == -1)
        nugget->sContact = NULL;

    return CLI_OK;
}

DECL_COMMAND(Console_Config_Nugget_Location)
{
    struct Nugget *nugget = Thread_GetCurrent()->pUserData;
    if (argc != 1 || (argv[0][strlen(argv[0])-1] == '?'))
    {
        cli_print(cli, (char *)"Usage: %s <String>", command);
        return CLI_OK;
    }
    free(nugget->sLocation);

    if (asprintf(&nugget->sLocation, "%s", argv[0]) == -1)
        nugget->sLocation = NULL;

    return CLI_OK;
}

DECL_COMMAND(Console_Config_Nugget_Notes)
{
    struct Nugget *nugget = Thread_GetCurrent()->pUserData;
    if (argc != 1 || (argv[0][strlen(argv[0])-1] == '?'))
    {
        cli_print(cli, (char *)"Usage: %s <String>", command);
        return CLI_OK;
    }
    free(nugget->sNotes);

    if (asprintf(&nugget->sNotes, "%s", argv[0]) == -1)
        nugget->sNotes = NULL;

    return CLI_OK;
}

//////
////// Configuration Modes
//////

DECL_COMMAND(Console_Config_Nugget)
{
    struct Nugget **nuggets;
    struct Nugget * nugget;
    uint64_t count;
    uuid_t nuggetId;
    char nugUuid[UUID_STRING_LENGTH];
    char *nugType = NULL;
    if (argc < 1)
    {
        cli_print(cli, (char *)"Specify a nugget to configure");
        return CLI_OK;
    }

    if (argv[0][strlen(argv[0])-1] == '?')
    {
        cli_print (cli, (char *) "%-36s %-20s %s", "Nugget ID", "Nugget Type", "Nugget Name");
        if (Datastore_Get_Nuggets(sg_dbCon, &nuggets, &count) != R_SUCCESS)
        {
            cli_print(cli, (char *)"Database Error: See dispatcher log for details.");
            return CLI_OK;
        }
        for (uint64_t i =0; i < count; i++)
        {

            uuid_unparse (nuggets[i]->uuidNuggetId, nugUuid);
            if (strlen(argv[0]) == 1 || strncmp(nugUuid, argv[0], strlen(argv[0]) -1) == 0)
            {
//                nugType = (char *)
                    Datastore_Get_UUID_Name_By_UUID (sg_dbCon, nuggets[i]->uuidNuggetType,
                                         UUID_TYPE_NUGGET_TYPE, &nugType);
                cli_print (cli, (char *) "%s %-20s %s", nugUuid, nugType,
                       nuggets[i]->sName);
                free(nugType);
            }
            Nugget_Destroy(nuggets[i]);
        }
        free(nuggets);
        return CLI_OK;
    }

    if (uuid_parse(argv[0], nuggetId) != 0)
    {
        cli_print(cli, (char *)"Invalid UUID: %s", argv[0]);
        return CLI_OK;
    }

    if (Datastore_Get_NuggetByUUID(sg_dbCon, nuggetId, &nugget) != R_FOUND)
    {
        cli_print(cli, (char *)"Unknown Nugget: %s", argv[0]);
        return CLI_OK;
    }

    Thread_GetCurrent()->pUserData = nugget;
    cli_set_configmode(cli, MODE_CONFIG_NUGGET, (char *)"nugget");
    return CLI_OK;
}

DECL_COMMAND(Console_Config_Nugget_Save)
{
    struct Nugget * nugget = Thread_GetCurrent()->pUserData;
    
    if (Datastore_Update_Nugget(sg_dbCon, nugget) != R_SUCCESS)
        cli_print(cli, (char *)"Failed to save nugget state");
    else
        cli_set_configmode(cli, MODE_CONFIG, NULL);

    Nugget_Destroy(nugget);
    return CLI_OK;
}


// Console Initialization
static void
Console_Init_Commands (struct cli_def *p_cli)
{
    struct cli_command *cShow;
    struct cli_command *cFlush;
    struct cli_command *cRegister;
    struct cli_command *cDisp;
    struct cli_command *cTemp;
    //struct cli_command *cTemp2;

    // Command: show
    // Priv:    user
    // Mode:    exec
    cShow =
        cli_register_command (p_cli, NULL, (char *) "show", NULL,
                              PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                              (char *) "Show system information");

    // Command: show cache
    // Priv:    user
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, cShow, (char *) "cache", NULL,
                              PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                              (char *) "Show Cache Information");
    // Command: show cache stats
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "stats",
                          Console_Command_Show_Cache_Stats, 
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show cache statistics");

    // Command: show datatypes
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cShow, (char *) "datatypes",
                          Console_Command_Show_UUIDs,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show data types");

    // Command: show dispatcher
    // Priv:    user
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, cShow, (char *) "dispatcher", NULL,
                              PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                              (char *) "Show Dispatcher Information");

    // Command: show dispatcher neighbours
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "neighbours",
                          Console_Command_Show_Dispatcher_Neighbours,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show the state of online dispatchers");

    // Command: show dispatcher status
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "status",
                          Console_Command_Show_Dispatcher_Status,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show the state of this dispatcher");

    // Command: show metadata
    // Priv:    user
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, cShow, (char *) "metadata", NULL,
                              PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                              (char *) "Show Metadata Information");
    // Command: show metadata types
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "types",
                          Console_Command_Show_UUIDs,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show metadata types");

    // Command: show metadata names
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "names",
                          Console_Command_Show_UUIDs,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show metadata names");

    // Command: show nugget
    // Priv:    user
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, cShow, (char *) "nugget", NULL,
                              PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                              (char *) "Show Nugget Information");

    // Command: show nugget status
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "status",
                          Console_Command_Show_Nugget_Status,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show the state of online nuggets");

    // Command: show nugget registrations
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "registrations",
                          Console_Command_Show_Nugget_Registrations,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show registered nuggets");

    // Command: show nugget apptypes
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "apptypes",
                          Console_Command_Show_Nugget_AppTypes,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show registered nugget application types");
    // Command: show threads
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cShow, (char *) "threads",
                          Console_Command_Show_Threads,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show the currently running threads");

    // Command: show routing
    // Priv:    user
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, cShow, (char *) "routing", NULL,
                              PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                              (char *) "Show Routing Information");

    // Command: show routing summary
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "summary",
                          Console_Command_Show_Routes, PRIVILEGE_UNPRIVILEGED,
                          MODE_EXEC,
                          (char *)
                          "Show the current data block routing table");

    // Command: show routing stats
    // Priv:    user
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "stats",
                          Console_Command_Show_Routing_Stats, 
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Show routing table statistics");

    // Command: dispatcher
    // Priv:    enable
    // Mode:    exec
    cDisp =
        cli_register_command (p_cli, NULL, (char *) "dispatcher", NULL,
                              PRIVILEGE_PRIVILEGED, MODE_EXEC,
                              (char *) "Dispatcher operational commands");

    // Command: dispatcher promote
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cDisp, (char *) "promote",
                          Console_Command_Dispatcher_Promote,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Promote the dispatcher from slave to master");
    // Command: dispatcher demote
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cDisp, (char *) "demote",
                          Console_Command_Dispatcher_Demote,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Demote the dispatcher from master to slave");
    // Command: flush
    // Priv:    enable
    // Mode:    exec
    cFlush =
        cli_register_command (p_cli, NULL, (char *) "flush", NULL,
                              PRIVILEGE_PRIVILEGED, MODE_EXEC,
                              (char *) "Flush system state");
    // Command: flush cache
    // Priv:    enable
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, cFlush, (char *) "cache", NULL,
                              PRIVILEGE_PRIVILEGED, MODE_EXEC,
                              (char *) "Flush caches");
    // Command: flush cache global
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "global",
                          Console_Command_Flush_Cache_Global,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Flush the global cache");

    // Command: flush cache local
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "local",
                          Console_Command_Flush_Cache_Local,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Flush the local cache in all nuggets");

    // Command: flush cache all
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "all",
                          Console_Command_Flush_Cache_All,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *)
                          "Flush the local cache in all nuggets and global cache");

    // Command: launch
    // Priv:    enable
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, NULL, (char *) "launch", NULL,
                              PRIVILEGE_PRIVILEGED, MODE_EXEC,
                              (char *) "Launch processing threads");

    // Command: launch cacheProcessor
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "cacheProcessor",
                          Console_Command_Launch_CacheProcessor,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Launch a cache processing thread");

    // Command: launch judgmentProcessor
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "judgmentProcessor",
                          Console_Command_Launch_JudgmentProcessor,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Launch a judgment processing thread");

    // Command: launch logProcessor
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "logProcessor",
                          Console_Command_Launch_LogProcessor,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Launch a log processing thread");

    // Command: launch submissionProcessor
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "submissionProcessor",
                          Console_Command_Launch_SubmissionProcessor,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Launch a judgment processing thread");

    // Command: requeue
    // Priv:    enable
    // Mode:    exec
    cTemp =
        cli_register_command (p_cli, NULL, (char *) "requeue", NULL,
                              PRIVILEGE_PRIVILEGED, MODE_EXEC,
                              (char *) "Requeue blocks for inspection");

    // Command: requeue block
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "block",
                          Console_Command_Requeue_Block,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Requeue a block for inspection");
    // Command: requeue errors
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "errors",
                          Console_Command_Requeue_Status,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Requeue blocks that errored during inspection for reinspection");

    // Command: requeue pending
    // Priv:    enable
    // Mode:    exec
    cli_register_command (p_cli, cTemp, (char *) "pending",
                          Console_Command_Requeue_Status,
                          PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                          (char *) "Requeue blocks that are marked pending inspection for inspection");

    // Command: register
    // Priv:    enable
    // Mode:    configure
    cRegister = 
        cli_register_command (p_cli, NULL, (char *) "register", NULL,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register components with the system");

    // Command: register apptype
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cRegister, (char *) "apptype",
                          Console_Command_Register_AppType,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *) "Register a new application type");

    // Command: register datatype
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cRegister, (char *) "datatype",
                          Console_Command_Register_UUID,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *) "Register a new data type");
    // Command: register metadata
    // Priv:    enable
    // Mode:    configure
    cTemp =
        cli_register_command (p_cli, cRegister, (char *) "metadata", NULL,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register metadata info with the system");

    // Command: register metadata name
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cTemp, (char *) "name",
                          Console_Command_Register_UUID,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register metadata name with the system");

    // Command: register metadata type
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cTemp, (char *) "type",
                          Console_Command_Register_UUID,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register metadata type with the system");
    // Command: register nugget
    // Priv:    enable
    // Mode:    configure
    cTemp =
        cli_register_command (p_cli, cRegister, (char *) "nugget", NULL,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register components with the system");

    // Command: register nugget collection
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cTemp, (char *) "collection",
                          Console_Command_Register_Nugget_InspOrCol,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register collection nugget with the system");

    // Command: register nugget inspection
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cTemp, (char *) "inspection",
                          Console_Command_Register_Nugget_InspOrCol,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register inspection nugget with the system");

    // Command: register nugget master
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, cTemp, (char *) "master",
                          Console_Command_Register_Nugget_Master,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Register master nugget with the system");

    // Command: nugget
    // Priv:    enable
    // Mode:    configure
    cli_register_command (p_cli, NULL, (char *) "nugget",
                          Console_Config_Nugget,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG,
                          (char *)"Update registered nugget information");

    // Command: name
    // Priv:    enable
    // Mode:    configure nugget
    cli_register_command (p_cli, NULL, (char *) "name",
                          Console_Config_Nugget_Name,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG_NUGGET,
                          (char *)"Set nugget name");
    
    // Command: location
    // Priv:    enable
    // Mode:    configure nugget
    cli_register_command (p_cli, NULL, (char *) "location",
                          Console_Config_Nugget_Location,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG_NUGGET,
                          (char *)"Set nugget location");
    // Command: contact
    // Priv:    enable
    // Mode:    configure nugget
    cli_register_command (p_cli, NULL, (char *) "contact",
                          Console_Config_Nugget_Contact,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG_NUGGET,
                          (char *)"Set nugget contact");
    // Command: notes
    // Priv:    enable
    // Mode:    configure nugget
    cli_register_command (p_cli, NULL, (char *) "notes",
                          Console_Config_Nugget_Notes,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG_NUGGET,
                          (char *)"Set nugget notes");
    // Command: save
    // Priv:    enable
    // Mode:    configure nugget
    cli_register_command (p_cli, NULL, (char *) "save",
                          Console_Config_Nugget_Save,
                          PRIVILEGE_PRIVILEGED, MODE_CONFIG_NUGGET,
                          (char *)"Save nugget and exit configuration");
}

void
Console_Thread (struct Thread *p_pThread)
{
    struct ConsoleSession *session;
    struct cli_def *cli;

    session = (struct ConsoleSession *) p_pThread->pUserData;

    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to intialiaze database thread info", __func__);
        return;
    }

    cli = cli_init ();
    cli_set_banner (cli, (char *) "Razorback(TM) Management Interface");
    cli_set_hostname (cli, (char *) "razorback");
    rzb_log (LOG_DEBUG, "%s: Launched", __func__);
    Console_Init_Commands (cli);
    if (!session->authenticated)
        cli_set_auth_callback (cli, check_auth);

    cli_set_enable_callback (cli, check_enable);
    cli_loop (cli, session->socket->iSocket);
    cli_done (cli);
    Socket_Close (session->socket);

    if (session->quitCallback != NULL)
        session->quitCallback(session);

    rzb_log (LOG_DEBUG, "%s: Exiting", __func__);
}

bool Console_CLI_Initialize()
{
    if ((sg_dbCon = Database_Connect
        (g_sDatabaseHost, g_iDatabasePort, g_sDatabaseUser,
         g_sDatabasePassword, g_sDatabaseSchema)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Database_Connect", __func__);
        return false;
    }
    return true;
}
