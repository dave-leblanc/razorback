#include "config.h"

#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/daemon.h>
#include <razorback/config_file.h>
#include <razorback/uuids.h>

#include <string.h>
#include <getopt.h>

#include <signal.h>



#ifdef _MSC_VER
#pragma comment(lib, "api.lib")
#define sleep(x) Sleep(x*1000)
#include "bobins.h"
#else
#include <ftw.h>
#include <dlfcn.h>
#include <sys/wait.h>
#endif


#define MASTER_CONF "master_nugget.conf"
static char *sg_sPidFile = NULL;
static char *sg_sSoPath = NULL;
static char *sg_sSoVersion;
static uuid_t sg_uuidNuggetId;
struct RazorbackContext *sg_pContext =NULL;

static struct option sg_options[] =
{
    {"debug",no_argument, NULL, 'd'},
    {NULL, 0, NULL, 0}
};

static RZBConfKey_t sg_Master_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"Nuggets.Path", RZB_CONF_KEY_TYPE_STRING, &sg_sSoPath, NULL},
    {"PidFile", RZB_CONF_KEY_TYPE_STRING, &sg_sPidFile, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

struct NuggetHandle
{
    char *name;
#ifdef _MSC_VER
	HINSTANCE dlHandle;
#else
    void *dlHandle;
#endif
    bool (*initNug)(void);
    void (*shutdownNug)(void);
    struct NuggetHandle *pNext;
};

struct NuggetHandle *nuggetList = NULL;


static bool loadNuggets();
static void unloadNuggets();
bool restart = false;
bool end = false;

#ifdef _MSC_VER
#else
void termination_handler (int signum);
/* SIGCHLD handler. */
static void 
sigchld_handler (int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}
#endif

int main (int argc, char ** argv)
{

    int optId; /* holds the option index for getopt*/
    int option_index = 0;
    int debug = 0;
	uuid_t masterNugget;

    while (1)
    {
        optId = getopt_long(argc, argv, "d", sg_options, &option_index);

        if (optId == -1)
            break;

        switch (optId)
        {
            case 'd':
                debug = 1;
                break;
            case '?':  /* getopt errors */
                // TODO: Print errors
                break;
            default:
                // TODO: Print Usage
                break;
        }
    }
    // TODO: Add command line option for config dir.
    if (!readMyConfig (NULL, MASTER_CONF, sg_Master_Config))
	{
		rzb_log(LOG_ERR, "%s: Failed to read configuation", __func__);
		return -1;
	}
#ifdef _MSC_VER
#else
    signal (SIGINT, termination_handler);
    signal (SIGTERM, termination_handler);
    signal (SIGHUP, termination_handler);
    signal (SIGPIPE, termination_handler);
    signal (SIGCHLD, sigchld_handler);

#endif
    if (!debug) {
        if (!rzb_daemonize(NULL, sg_sPidFile)) {
            rzb_log(LOG_ERR, "Failed to daemonize");
            return 1;
        }
    } else {
        rzb_debug_logging();
    }
    if ((sg_pContext= calloc(1, sizeof(struct RazorbackContext))) == NULL)
    {
        rzb_log(LOG_ERR, "Failed to allocate master nugget context");
        return 1;
    }

    
    UUID_Get_UUID(NUGGET_TYPE_MASTER, UUID_TYPE_NUGGET_TYPE, masterNugget);
    uuid_copy(sg_pContext->uuidNuggetId, sg_uuidNuggetId);
    uuid_copy(sg_pContext->uuidNuggetType, masterNugget);
    uuid_clear(sg_pContext->uuidApplicationType);
    sg_pContext->iFlags=CONTEXT_FLAG_STAND_ALONE;
    sg_pContext->inspector.dataTypeCount=0;
    sg_pContext->pCommandHooks=NULL;
    sg_pContext->inspector.hooks=NULL;
    if (!Razorback_Init_Context(sg_pContext)) 
    {
        rzb_log(LOG_ERR, "Master Nugget: Failed to register context");
        return 1;
    }
    loadNuggets();
    while (!end)
    {
        if (restart)
        {
            unloadNuggets();
            loadNuggets();
            restart = false;
        }
        sleep(1);
    }
    unloadNuggets();
    Razorback_Shutdown_Context(sg_pContext);

    return 0;
}


#ifdef _MSC_VER
static bool
processFile (WIN32_FIND_DATAA fileData)
{
	struct NuggetHandle *nugget;
	rzb_log(LOG_DEBUG, "Load Nuggets: Found File: %s", fileData.cFileName);
    if ((nugget = calloc(1,sizeof(struct NuggetHandle))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate nugget handle (%s)", __func__, fileData.cFileName);
        return false;
    }
	if ( asprintf(&nugget->name, "%s\\%s", sg_sSoPath, fileData.cFileName) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate nugget name (%s)", __func__, fileData.cFileName);
        return false;
    }
	if ((nugget->dlHandle = LoadLibraryA(nugget->name)) == NULL)
	{
        free(nugget);
        rzb_log(LOG_ERR, "Load Nuggets: Failed to open %s, %d", fileData.cFileName, GetLastError());
		return false;
    }
	*(void **)&nugget->initNug = GetProcAddress(nugget->dlHandle, "initNug");
    *(void **)&nugget->shutdownNug = GetProcAddress(nugget->dlHandle, "shutdownNug");
	if (nugget->initNug == NULL)
    {
        FreeLibrary(nugget->dlHandle);
        rzb_log(LOG_ERR, "Failed to resolve initNug() for %s.", fileData.cFileName);
        free(nugget);
        return false;
    }
    if (nugget->shutdownNug == NULL)
    {
        FreeLibrary(nugget->dlHandle);
        rzb_log(LOG_ERR, "Failed to resolve shutdownNug() for %s.", fileData.cFileName);
        free(nugget);
        return false;
    }

    if (!nugget->initNug())
    {
        rzb_log(LOG_ERR, "Failed to initialize nugget: %s", fileData.cFileName);
        FreeLibrary(nugget->dlHandle);
        free(nugget);
        return false;
    }
	rzb_log(LOG_INFO, "Functions registered for %s", fileData.cFileName);
    // Add nugget to list.
    nugget->pNext = nuggetList;
    nuggetList = nugget;
	return true;
}
#else
// ftw callback that looks for files with the provided extension
static int 
processFile(const char *name, const struct stat *status, int type) 
{
//    int name_len, offset;
    struct NuggetHandle *nugget;
    char *errstr;
    if (type == FTW_F)
    {
        if (strstr(name,sg_sSoVersion) == &name[strlen(name) - strlen(sg_sSoVersion)])
        {
            rzb_log(LOG_DEBUG, "Load Nuggets: Found File: %s", name);
            if ((nugget = calloc(1,sizeof(struct NuggetHandle))) == NULL)
            {
                rzb_log(LOG_ERR, "%s: Failed to allocate nugget handle (%s)", __func__, name);
                return 0;
            }
            if ( asprintf(&nugget->name, "%s", name) == -1)
            {
                rzb_log(LOG_ERR, "%s: Failed to allocate nugget name (%s)", __func__, name);
                return 0;
            }

            if ((nugget->dlHandle = dlopen(name, RTLD_LOCAL | RTLD_NOW)) == NULL)
            {
                errstr = dlerror();
                free(nugget);
                rzb_log(LOG_ERR, "Load Nuggets: Failed to open %s, %s", name, errstr);
                return 0;
            }

            *(void **)&nugget->initNug = dlsym(nugget->dlHandle, "initNug");
            *(void **)&nugget->shutdownNug = dlsym(nugget->dlHandle, "shutdownNug");
            if (nugget->initNug == NULL)
            {
                dlclose(nugget->dlHandle);
                rzb_log(LOG_ERR, "Failed to resolve initNug() for %s.", name);
                free(nugget);
                return 0;
            }
            if (nugget->shutdownNug == NULL)
            {
                dlclose(nugget->dlHandle);
                rzb_log(LOG_ERR, "Failed to resolve shutdownNug() for %s.", name);
                free(nugget);
                return 0;
            }

            if (!nugget->initNug())
            {
                rzb_log(LOG_ERR, "Failed to initialize nugget: %s", name);
                dlclose(nugget->dlHandle);
                free(nugget);
                return 0;
            }
            rzb_log(LOG_INFO, "Functions registered for %s", name);
            // Add nugget to list.
            nugget->pNext = nuggetList;
            nuggetList = nugget;

        }
    }
    return 0;
}
#endif

static bool loadNuggets()
{   
#ifdef _MSC_VER
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	char * path;
	if (asprintf(&path, "%s\\%s", sg_sSoPath, "\\*.dll") == -1)
		return false;
	
	hFind = FindFirstFileA(path, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		rzb_log(LOG_ERR, "%s: FindFirstFile failed (%d)", __func__, GetLastError());
		return false;
	} 
	 
	while (hFind != INVALID_HANDLE_VALUE)
	{
		if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			processFile(FindFileData);
		}
		if (!FindNextFileA(hFind, &FindFileData))
		{
			if (GetLastError() == ERROR_NO_MORE_FILES)
			{
				FindClose(hFind);
				hFind = INVALID_HANDLE_VALUE;
			}
			else
			{
				rzb_log(LOG_ERR, "%s: Failed to find file", __func__);
				return false;
			}
		}
	}
#else
	int l_iIt, l_iLen;
    if ((l_iLen = asprintf(&sg_sSoVersion, "%s", NUGGET_SO_VERSION))== -1) 
        return false;
   
    // This is ugly there must be a better way
    for (l_iIt =0; l_iIt < l_iLen; l_iIt++) 
        if (sg_sSoVersion[l_iIt] == ':')
            sg_sSoVersion[l_iIt] = '.';

	rzb_log(LOG_DEBUG, "LoadNuggets: SO Version: %s", sg_sSoVersion);
	ftw(sg_sSoPath, processFile, 1);
#endif
    return true;
}


static void unloadNuggets()
{
    struct NuggetHandle *current = nuggetList;
    while(current != NULL)
    {
        rzb_log(LOG_INFO, "Shutting down nugget: %s", current->name);
        current->shutdownNug();
#ifdef _MSC_VER
#else
        dlclose(current->dlHandle);
#endif
        nuggetList = current->pNext;
        free(current->name);
        free(current);
        current = nuggetList;
    }
}


#ifdef _MSC_VER
#else
void
termination_handler (int signum)
{
    switch(signum)
    {
    case SIGPIPE:
        // Ignore it
        break;
    case SIGINT:
    case SIGTERM:
        rzb_log(LOG_INFO, "Master Nugget shutting down");
        end=true;
        break;
    case SIGHUP:
        rzb_log(LOG_INFO, "Master Nugget Reloading Nuggets");
        restart = true;
        break;
    default:
        rzb_log(LOG_ERR, "%s: Master Nugget Unknown signal: %i", __func__, signum);
        break;
    }
}
#endif

