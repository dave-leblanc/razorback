#ifndef DISPATCHER_CONFIG_H
#define DISPATCHER_CONFIG_H
#include <razorback/types.h>
#include <razorback/config_file.h>
struct ConsoleServiceConf
{
    conf_int_t enable;
    char * bindAddress;
    conf_int_t bindPort;
};

struct CacheHost {
    char * hostname;
    conf_int_t port;
}  __attribute__((__packed__));

extern char *g_sPidFile;
extern uuid_t g_uuidDispatcherId;
extern conf_int_t g_dispatcherPriority;

extern conf_int_t g_cacheThreadsInitial;
extern conf_int_t g_cacheThreadsMax;
extern conf_int_t g_judgmentThreadsInitial;
extern conf_int_t g_judgmentThreadsMax;
extern conf_int_t g_logThreadsInitial;
extern conf_int_t g_logThreadsMax;
extern conf_int_t g_submissionThreadsInitial;
extern conf_int_t g_submissionThreadsMax;

extern char *g_sDatabaseHost;
extern uint32_t g_iDatabasePort;
extern char *g_sDatabaseSchema;
extern char *g_sDatabaseUser;
extern char *g_sDatabasePassword;

extern struct CacheHost *g_cacheHosts;
extern conf_int_t g_cacheHostCount;

extern struct ConsoleServiceConf g_telnetConsole;
extern struct ConsoleServiceConf g_sshConsole;
extern char * g_sConsoleUser;
extern char * g_sConsolePassword;
extern char * g_sConsoleEnablePassword;

extern char **transferHostnames;
extern conf_int_t transferHostnamesCount;
extern conf_int_t transferProtocol;
extern conf_int_t transferBindPort;

extern bool g_outputAlertEnabled;
extern bool g_outputAlertChildEnabled;
extern bool g_outputEventEnabled;
extern bool g_outputInspectionEventEnabled;

extern conf_int_t g_sStorageDeletePolicy;

bool Config_Load(void);

#endif
