#include "config.h"
#include <razorback/debug.h>
#include <razorback/config_file.h>
#include "configuration.h"

#include <string.h>

#define DISPATCHER_CONF "dispatcher.conf"

char *g_sPidFile = NULL;
conf_int_t g_dispatcherPriority = 0;
conf_int_t g_cacheThreadsInitial = 0;
conf_int_t g_cacheThreadsMax = 0;

conf_int_t g_judgmentThreadsInitial =0;
conf_int_t g_judgmentThreadsMax =0;

conf_int_t g_logThreadsInitial = 0;
conf_int_t g_logThreadsMax = 0;

conf_int_t g_submissionThreadsInitial = 0;
conf_int_t g_submissionThreadsMax = 0;

char *g_sDatabaseHost = NULL;
uint32_t g_iDatabasePort = 0;
char *g_sDatabaseSchema = NULL;
char *g_sDatabaseUser = NULL;
char *g_sDatabasePassword = NULL;

struct ConsoleServiceConf g_telnetConsole;
struct ConsoleServiceConf g_sshConsole;
char * g_sConsoleUser = NULL;
char * g_sConsolePassword = NULL;
char * g_sConsoleEnablePassword = NULL;

struct CacheHost *g_cacheHosts = NULL;
conf_int_t g_cacheHostCount = 0;
char **transferHostnames;
conf_int_t transferHostnamesCount = 0;
conf_int_t transferProtocol = 0;
conf_int_t transferBindPort = 0;

bool g_outputAlertEnabled = false;
bool g_outputAlertChildEnabled = false;
bool g_outputEventEnabled = false;
bool g_outputInspectionEventEnabled = false;

conf_int_t g_sStorageDeletePolicy;

static RZBConfKey_t listCacheHost[] =
{
    {"Host", RZB_CONF_KEY_TYPE_STRING, NULL, NULL},
    {"Port", RZB_CONF_KEY_TYPE_INT, NULL, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

static struct ConfList cacheHostList = {
    (void **)&g_cacheHosts,
    &g_cacheHostCount,
    listCacheHost
};

static struct ConfArray transferHostnamesArray = {
    RZB_CONF_KEY_TYPE_STRING,
    (void **)&transferHostnames,
    &transferHostnamesCount,
    NULL
};

static bool parseDeletePolicy(const char *string, conf_int_t *val) {
	if (!strncasecmp (string, "None", 4)) {
		*val = 0;
		return true;
	}
	else if (!strncasecmp (string, "Good", 4)) {
		*val = SF_FLAG_GOOD;
		return true;
	}
	else if (!strncasecmp (string, "Bad", 3)) {
		*val = SF_FLAG_GOOD | SF_FLAG_BAD;
		return true;
	}
	return false;
}

static RZBConfCallBack deletePolicyCallback = {
	&parseDeletePolicy
};

// config file definition
static RZBConfKey_t Dispatcher_Config[] = {
    {"PidFile", RZB_CONF_KEY_TYPE_STRING, &g_sPidFile, NULL},

    {"Dispatcher.Id", RZB_CONF_KEY_TYPE_UUID, &g_uuidDispatcherId, NULL},
    {"Dispatcher.Priority", RZB_CONF_KEY_TYPE_INT, &g_dispatcherPriority, NULL},
    {"Threads.Cache.Initial", RZB_CONF_KEY_TYPE_INT, &g_cacheThreadsInitial, NULL },
    {"Threads.Cache.Max", RZB_CONF_KEY_TYPE_INT, &g_cacheThreadsMax, NULL },

    {"Threads.Judgment.Initial", RZB_CONF_KEY_TYPE_INT, &g_judgmentThreadsInitial, NULL },
    {"Threads.Judgment.Max", RZB_CONF_KEY_TYPE_INT, &g_judgmentThreadsMax, NULL },

    {"Threads.Log.Initial", RZB_CONF_KEY_TYPE_INT, &g_logThreadsInitial, NULL },
    {"Threads.Log.Max", RZB_CONF_KEY_TYPE_INT, &g_logThreadsMax, NULL },

    {"Threads.Submission.Initial", RZB_CONF_KEY_TYPE_INT, &g_submissionThreadsInitial, NULL },
    {"Threads.Submission.Max", RZB_CONF_KEY_TYPE_INT, &g_submissionThreadsMax, NULL },

    {"Database.Schema", RZB_CONF_KEY_TYPE_STRING, &g_sDatabaseSchema,
     NULL},
    {"Database.Host", RZB_CONF_KEY_TYPE_STRING, &g_sDatabaseHost, NULL},
    {"Database.Port", RZB_CONF_KEY_TYPE_INT, &g_iDatabasePort, NULL},
    {"Database.Username", RZB_CONF_KEY_TYPE_STRING, &g_sDatabaseUser,
     NULL},
    {"Database.Password", RZB_CONF_KEY_TYPE_STRING, &g_sDatabasePassword,
     NULL},
    
    {"GlobalCache", RZB_CONF_KEY_TYPE_LIST, NULL, &cacheHostList},

    {"Console.Telnet.Enable", RZB_CONF_KEY_TYPE_INT, &g_telnetConsole.enable, NULL},
    {"Console.Telnet.Bind.Address", RZB_CONF_KEY_TYPE_STRING, &g_telnetConsole.bindAddress, NULL},
    {"Console.Telnet.Bind.Port", RZB_CONF_KEY_TYPE_INT, &g_telnetConsole.bindPort, NULL},
    {"Console.SSH.Enable", RZB_CONF_KEY_TYPE_INT, &g_sshConsole.enable, NULL},
    {"Console.SSH.Bind.Address", RZB_CONF_KEY_TYPE_STRING, &g_sshConsole.bindAddress, NULL},
    {"Console.SSH.Bind.Port", RZB_CONF_KEY_TYPE_INT, &g_sshConsole.bindPort, NULL},
    {"Console.User", RZB_CONF_KEY_TYPE_STRING, &g_sConsoleUser, NULL},
    {"Console.Password", RZB_CONF_KEY_TYPE_STRING, &g_sConsolePassword, NULL},
    {"Console.EnablePassword", RZB_CONF_KEY_TYPE_STRING, &g_sConsoleEnablePassword, NULL},
    
    {"Transfer.Protocol", RZB_CONF_KEY_TYPE_INT, &transferProtocol, NULL},
    {"Transfer.Port", RZB_CONF_KEY_TYPE_INT, &transferBindPort, NULL},
    {"Transfer.Hostnames", RZB_CONF_KEY_TYPE_ARRAY, NULL, &transferHostnamesArray},
    {"Output.Alert.Enabled", RZB_CONF_KEY_TYPE_BOOL, &g_outputAlertEnabled, NULL},
    {"Output.AlertChild.Enabled", RZB_CONF_KEY_TYPE_BOOL, &g_outputAlertChildEnabled, NULL},
    {"Output.Event.Enabled", RZB_CONF_KEY_TYPE_BOOL, &g_outputEventEnabled, NULL},
    {"Output.InspectionEvent.Enabled", RZB_CONF_KEY_TYPE_BOOL, &g_outputInspectionEventEnabled, NULL},

	{"Storage.DeletePolicy", RZB_CONF_KEY_TYPE_PARSED_STRING, &g_sStorageDeletePolicy, &deletePolicyCallback},
    
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

bool
Config_Load(void)
{
    memset(&g_telnetConsole, 0, sizeof(struct ConsoleServiceConf));
    memset(&g_sshConsole, 0, sizeof(struct ConsoleServiceConf));

    // read the configuration
    return readMyConfig (NULL, DISPATCHER_CONF, Dispatcher_Config);
}

