
#include <razorback/visibility.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block.h>
#include <razorback/hash.h>
#include <razorback/config_file.h>

#ifdef _MSC_VER
#pragma comment(lib, "api.lib")
#define BLK_SIZE_FMT "%I64u"
#include "bobins.h"
#else
#include <ftw.h>
#include <dlfcn.h>
#include <sys/wait.h>
#define BLK_SIZE_FMT "%ju"
#endif

#define FILE_LOG_CONFIG "file_log.conf"
UUID_DEFINE(FILE_LOG, 0x74, 0xc1, 0x35, 0xc6, 0x50, 0xb9, 0x11, 0xe0, 0x9e, 0xd2, 0xe3, 0x1d, 0x9c, 0x72, 0xc9, 0xf1);

static uuid_t sg_uuidNuggetId;
static char * sg_sLogFile;
static char * sg_sDataBlockDir;

struct RazorbackContext *sg_pContext = NULL;

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(file_handler);

static struct RazorbackInspectionHooks sg_InspectionHooks ={
    file_handler,
    processDeferredList,
    NULL,
    NULL
};

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"LogFile", RZB_CONF_KEY_TYPE_STRING, &sg_sLogFile, NULL},
    {"DataBlockDir", RZB_CONF_KEY_TYPE_STRING, &sg_sDataBlockDir, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

static FILE *logFile = NULL;

bool 
processDeferredList (struct DeferredList *p_pDefferedList) 
{
    return true;
}

DECL_INSPECTION_FUNC(file_handler)
{

	char uuid_out[UUID_STRING_LENGTH];
    char *l_sTemp;
    char * l_sHash;
    char *l_sFileName;
    FILE *l_pFileHandle;

    fprintf(logFile, "%d,", (int)time(NULL));
    uuid_unparse(block->pId->uuidDataType, uuid_out);
    fprintf(logFile, "%s,", uuid_out);
    l_sTemp =UUID_Get_NameByUUID(block->pId->uuidDataType, UUID_TYPE_DATA_TYPE);
    fprintf(logFile, "%s,", l_sTemp);
    fprintf(logFile,  BLK_SIZE_FMT "\n", block->pId->iLength);
    fflush(logFile);
    rzb_log_remote(LOG_INFO, eventId, "%s, %s, " BLK_SIZE_FMT, uuid_out, l_sTemp, block->pId->iLength);
    free(l_sTemp);
  
    if (sg_sDataBlockDir != NULL)
    {
        l_sHash =Hash_ToText(block->pId->pHash);
        if (asprintf(&l_sFileName, "%s/block-%s", sg_sDataBlockDir, l_sHash) == -1)
            return JUDGMENT_REASON_ERROR;

        l_pFileHandle =fopen(l_sFileName, "w");
        fwrite(block->data.pointer, block->pId->iLength, 1, l_pFileHandle);
        fclose(l_pFileHandle);
        free(l_sFileName);
        free(l_sHash);
    }
    return JUDGMENT_REASON_DONE;
}

SO_PUBLIC DECL_NUGGET_INIT
{
	uuid_t l_pTempUuid;
	uuid_t l_uuidList;
    rzb_log(LOG_DEBUG, "File Log Nugget: Initializing");
    sg_sDataBlockDir = NULL;
    sg_sLogFile = NULL;
    

    if (!readMyConfig(NULL, FILE_LOG_CONFIG, sg_Config))
	{
		rzb_log(LOG_ERR, "File Log Nugget: Failed to read config");
		return false;
	}
    if (sg_sLogFile == NULL)
    {
        rzb_log(LOG_ERR, "File Log Nugget: No log File Configured");
        return false;
    }

    
    if (!UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_uuidList))
    {
        rzb_log(LOG_ERR, "File Log Nugget: Failed to get ANY_DATA UUID");
        return false;
    }

    uuid_copy(l_pTempUuid, FILE_LOG);

	if((logFile = fopen(sg_sLogFile, "a+")) == NULL) {
        rzb_perror("File Log Nugget: Failed to open log file: %s");
		return false;
	}

    if ((sg_pContext = Razorback_Init_Inspection_Context (
        sg_uuidNuggetId, l_pTempUuid, 1, &l_uuidList,
        &sg_InspectionHooks,1,1)) == NULL)
    {
        return false;
    }
    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}
