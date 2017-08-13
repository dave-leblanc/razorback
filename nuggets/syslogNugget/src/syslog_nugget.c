
#include <razorback/visibility.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block.h>
#include <razorback/hash.h>
#include <razorback/config_file.h>
#include <razorback/messages.h>

#include <stdio.h>

#define SYSLOG_CONFIG "syslog.conf"
UUID_DEFINE(SYSLOG, 0xde, 0x39, 0xe6, 0x0b, 0x07, 0x3f, 0x45, 0x98, 0xa2, 0x75, 0x28, 0x31, 0x01, 0x3e, 0x55, 0xeb);

static uuid_t sg_uuidNuggetId;
static char *alertPrimaryFilter = NULL;
static char *alertChildFilter = NULL;
static char *logFilter = NULL;
static char *eventFilter = NULL;
static conf_int_t alertPrimaryEnable = 0;
static conf_int_t alertChildEnable = 0;
static conf_int_t logEnable = 0;
static conf_int_t eventEnable = 0;
static char * alertPrimaryCommand = NULL;
static char * alertChildCommand = NULL;
static char * logCommand = NULL;
static char * eventCommand = NULL;
static FILE *alertPrimaryPipe = NULL;
static FILE *alertChildPipe = NULL;
static FILE *eventPipe = NULL;
static FILE *logPipe = NULL;

struct RazorbackContext *sg_pContext = NULL;

DECL_ALERT_PRIMARY_FUNC(alertPrimary);
DECL_ALERT_CHILD_FUNC(alertChild);
DECL_OUTPUT_EVENT_FUNC(handleEvent);
DECL_OUTPUT_LOG_FUNC(handleLog);

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"Alert.Primary.Enabled", RZB_CONF_KEY_TYPE_BOOL, &alertPrimaryEnable, NULL },
    {"Alert.Primary.Filter", RZB_CONF_KEY_TYPE_STRING, &alertPrimaryFilter, NULL },
    {"Alert.Primary.Command", RZB_CONF_KEY_TYPE_STRING, &alertPrimaryCommand, NULL},

    {"Alert.Child.Enabled", RZB_CONF_KEY_TYPE_BOOL, &alertChildEnable, NULL },
    {"Alert.Child.Filter", RZB_CONF_KEY_TYPE_STRING, &alertChildFilter, NULL },
    {"Alert.Child.Command", RZB_CONF_KEY_TYPE_STRING, &alertChildCommand, NULL},

    {"Event.Enabled", RZB_CONF_KEY_TYPE_BOOL, &eventEnable, NULL },
    {"Event.Filter", RZB_CONF_KEY_TYPE_STRING, &eventFilter, NULL },
    {"Event.Command", RZB_CONF_KEY_TYPE_STRING, &eventCommand, NULL},

    {"Log.Enabled", RZB_CONF_KEY_TYPE_BOOL, &logEnable, NULL },
    {"Log.Filter", RZB_CONF_KEY_TYPE_STRING, &logFilter, NULL },
    {"Log.Command", RZB_CONF_KEY_TYPE_STRING, &logCommand, NULL},

    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

struct RazorbackOutputHooks sg_alertPrimaryHooks = {
    NULL,
    ">",
    MESSAGE_TYPE_ALERT_PRIMARY,
    alertPrimary,
    NULL,
    NULL,
    NULL
};
struct RazorbackOutputHooks sg_alertChildHooks = {
    NULL,
    ">",
    MESSAGE_TYPE_ALERT_CHILD,
    NULL,
    alertChild,
    NULL,
    NULL
};
struct RazorbackOutputHooks sg_eventHooks = {
    NULL,
    ">",
    MESSAGE_TYPE_OUTPUT_EVENT,
    NULL,
    NULL,
    handleEvent,
    NULL
};
struct RazorbackOutputHooks sg_logHooks = {
    NULL,
    ">",
    MESSAGE_TYPE_OUTPUT_LOG,
    NULL,
    NULL,
    NULL,
    handleLog
};



SO_PUBLIC DECL_NUGGET_INIT
{
    rzb_log(LOG_DEBUG, "Syslog Nugget: Initializing");
    uuid_t l_pTempUuid;

    readMyConfig(NULL, SYSLOG_CONFIG, sg_Config);

    uuid_copy(l_pTempUuid, SYSLOG);

    if ((sg_pContext = Razorback_Init_Output_Context (
        sg_uuidNuggetId, l_pTempUuid)) == NULL)
    {
        return false;
    }
    if (alertPrimaryEnable == 1 && 
            (alertPrimaryFilter != NULL) && 
            (alertPrimaryCommand != NULL))
    {
        sg_alertPrimaryHooks.pattern = alertPrimaryFilter;
        if ((alertPrimaryPipe = popen(alertPrimaryCommand, "w")) == NULL)
            return false;

        Razorback_Output_Launch(sg_pContext, &sg_alertPrimaryHooks);
    }
    if (alertChildEnable == 1 && 
            (alertChildFilter != NULL) && 
            (alertChildCommand != NULL))
    {
        sg_alertChildHooks.pattern = alertChildFilter;
        if ((alertChildPipe = popen(alertChildCommand, "w")) == NULL)
            return false;
        Razorback_Output_Launch(sg_pContext, &sg_alertChildHooks);
    }
    if (eventEnable == 1 && 
            (eventFilter != NULL) && 
            (eventCommand != NULL))
    {
        sg_eventHooks.pattern = eventFilter;
        if ((eventPipe = popen(eventCommand, "w")) == NULL)
            return false;
        Razorback_Output_Launch(sg_pContext, &sg_eventHooks);
    }
    if (logEnable == 1 && 
            (logFilter != NULL) && 
            (logCommand != NULL))
    {
        sg_logHooks.pattern = logFilter;
        if ((logPipe = popen(logCommand, "w")) == NULL)
            return false;
        Razorback_Output_Launch(sg_pContext, &sg_logHooks);
    }
    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{

    Razorback_Shutdown_Context(sg_pContext);

    if (alertPrimaryPipe != NULL)
        pclose(alertPrimaryPipe);

    if (alertChildPipe != NULL)
        pclose(alertChildPipe);

    if (eventPipe != NULL)
        pclose(eventPipe);

    if (logPipe != NULL)
        pclose(logPipe);
}

DECL_ALERT_PRIMARY_FUNC(alertPrimary)
{
    const char *goodBad;
    const char *dodgy;
    char *hash, *name;
    if ((message->SF_Flags & SF_FLAG_BAD) != 0)
        goodBad = "Bad";
    else 
        goodBad = "Good";
    if ((message->SF_Flags & SF_FLAG_DODGY) != 0)
        dodgy = " Dodgy";
    else
        dodgy = "";
    hash = Hash_ToText(message->block->pId->pHash);
    name = UUID_Get_NameByUUID(message->block->pId->uuidDataType, UUID_TYPE_DATA_TYPE);
    fprintf(alertPrimaryPipe, 
            "Alert (%s%s) Hash: %s Size: %ju Data Type: %s SF_Flags: 0x%08x Ent_Flags: 0x%08x\n", 
            goodBad, dodgy, hash, message->block->pId->iLength, name, 
            message->SF_Flags, message->Ent_Flags);
    fflush(alertPrimaryPipe);
    free(name);
    free(hash);
    return true;
}
DECL_ALERT_CHILD_FUNC(alertChild)
{
    char *childHash, *parentHash, *childType, *parentType;
    childHash = Hash_ToText(message->child->pId->pHash);
    parentHash = Hash_ToText(message->block->pId->pHash);
    parentType = UUID_Get_NameByUUID(message->block->pId->uuidDataType, UUID_TYPE_DATA_TYPE);
    childType = UUID_Get_NameByUUID(message->child->pId->uuidDataType, UUID_TYPE_DATA_TYPE);

    fprintf(alertChildPipe, 
            "Alert Block ( Hash: %s Size: %ju Data Type: %s  SF_Flags: 0x%08x Ent_Flags: 0x%08x ) Child ( Hash: %s Size: %ju Data Type: %s)\n", 
            parentHash, message->block->pId->iLength, parentType, 
            message->SF_Flags, message->Ent_Flags,
            childHash, message->child->pId->iLength, childType);
    fflush(alertChildPipe);
    free(childHash);
    free(parentHash);
    free(childType);
    free(parentType);
    return true;
}
DECL_OUTPUT_EVENT_FUNC(handleEvent)
{
    char *hash, *type;
    hash = Hash_ToText(message->event->pBlock->pId->pHash);
    type = UUID_Get_NameByUUID(message->event->pBlock->pId->uuidDataType, UUID_TYPE_DATA_TYPE);
    fprintf(eventPipe, 
            "Event Block Hash: %s Size: %ju Data Type: %s\n", 
            hash, message->event->pBlock->pId->iLength, type);
    fflush(eventPipe);
    free(hash);
    free(type);
    return true;
}

DECL_OUTPUT_LOG_FUNC(handleLog)
{
    fprintf(logPipe, "%s\n", message->message);
    fflush(logPipe);
    return true;
}



