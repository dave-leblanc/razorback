// aviraNugget.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "config.h"
#include <razorback/visibility.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block.h>
#include <razorback/hash.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>
#include <razorback/config_file.h>

#define AVIRA_NORMAL 0
#define AVIRA_CONCERN 1
#define AVIRA_IN_MEM 2
#define AVIRA_ALERT 3
#define AVIRA_HELP 100
#define AVIRA_MACRO 101
#define AVIRA_INVALID_OPTION 203
#define AVIRA_INVALID_PATH 204
#define AVIRA_INVALID_LOG 205
#define AVIRA_MISSING_LIB 210
#define AVIRA_FAILED_CHECK 211
#define AVIRA_FAILED_DEFS 212
#define AVIRA_FAILED_ENGINE 213
#define AVIRA_FAILED_LICENSE 214
#define AVIRA_FAILED_SELF 215
#define AVIRA_NO_ACCESS_FILE 216
#define AVIRA_NO_ACCESS_DIR 217


#pragma comment(lib, "api.lib")
#include "bobins.h"

#define AVG_CONFIG "avira.conf"
UUID_DEFINE(AVIRA_NUGGET, 0xb4, 0xfd, 0x91, 0xd5, 0x29, 0x93, 0x41, 0xed, 0xb9, 0x3e, 0x49, 0xab, 0x6f, 0x03, 0x74, 0x12);
//b4fd91d5-2993-41ed-b93e-49ab6f037412
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

struct RazorbackContext *sg_pContext = NULL;
static char *sg_sAviraPath = NULL;

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
    {"AviraPath", RZB_CONF_KEY_TYPE_STRING, &sg_sAviraPath, NULL},
	{"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

static FILE *logFile = NULL;

bool 
processDeferredList (struct DeferredList *p_pDefferedList) 
{
    return true;
}
#if 0
C:\Program Files (x86)\Avira\AntiVir Desktop\scancl.exe --dmse --allfiles --scaninarchive --heurlevel=3 --verboselog
#endif
#define CMD_FMT "\"\"%s\"\" --dmse --allfiles --scaninarchive --heurlevel=3 --verboselog %s"
DECL_INSPECTION_FUNC(file_handler)
{
	char *cmd;
	FILE   *pPipe;
	char   *buffer, *tmp;
	size_t pos = 0;
	size_t read =0;
	size_t size = 1024;
	int count = 0;
	char *found;
	char *tmpMessage;
	struct Judgment *judgment = NULL;

	if ((buffer = calloc(size+1, sizeof(char))) == NULL)
	{
		rzb_log(LOG_ERR, "Avira(%s): Failed to allocate output buffer", __func__);
		return JUDGMENT_REASON_ERROR;
	}

	if (asprintf(&cmd, CMD_FMT, sg_sAviraPath, block->data.fileName) == -1)
	{
		rzb_log(LOG_ERR, "Avira(%s): Failed to allocate command string", __func__);
		return JUDGMENT_REASON_ERROR;
	}
	//rzb_log(LOG_DEBUG, "Avira CMD: %s", cmd);
	if( (pPipe = _popen( cmd, "rt" )) == NULL )
	{
		rzb_log(LOG_ERR, "Avira(%s): Failed to open pipe to command", __func__);
		return JUDGMENT_REASON_ERROR;
	}

    /* Read pipe until end of file. */
	while( !feof( pPipe ) )
	{
		
		
		read = fread( buffer+pos, 1, size-pos, pPipe );
		if( ferror( pPipe ) )      {
			rzb_log(LOG_ERR, "Avira(%s): Read error: %s", __func__, strerror(errno) );
			break;
		}
		
		//read = strlen(buffer) -pos;
		if (size == (pos+read))
		{
			tmp = buffer;
			buffer = realloc(buffer, size+1024);
			if (buffer == NULL)
			{
				free(tmp);
				rzb_log(LOG_ERR, "Avira(%s): Realloc failure", __func__);
				return JUDGMENT_REASON_ERROR;
			}
			size +=1024;
		}
		pos +=read;
	}
	buffer[pos] = '\0';
    /* Close pipe and print return value of pPipe. */
	read = _pclose( pPipe );
	if ((judgment = Judgment_Create(eventId, block->pId)) == NULL)
	{
		rzb_log(LOG_ERR, "Avira(%s): Failed to allocate judgment", __func__);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;
	}
	switch (read)
	{
	case AVIRA_NORMAL:
		break;
	case AVIRA_MACRO:
		if (asprintf(&judgment->sMessage, "Avira detected a macro in the file") == -1)
		{
			rzb_log(LOG_ERR, "Avira(%s): Failed to allocate message", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
		// Dodgy
		judgment->Set_SfFlags = SF_FLAG_DODGY;
		judgment->iPriority = 1;
		Metadata_Add_Report(judgment->pMetaDataList,buffer);
		Razorback_Render_Verdict(judgment);
		break;
	case AVIRA_IN_MEM:
		rzb_log_remote(LOG_ERR, eventId, "Avira: Found memory resident infection on inspector");
		break;
	case AVIRA_CONCERN:
	case AVIRA_ALERT:
		judgment->Set_SfFlags = SF_FLAG_BAD;
		judgment->iPriority = 1;
		// Add the report string;
		Metadata_Add_Report(judgment->pMetaDataList,buffer);
		// WTF Why doesnt this work?  It works on linux
		//sscanf(buffer, "Found infections    :%d", &count);
		found = strstr(buffer, "Infected...");
		if (found != NULL)
		{
			found += strcspn(found, "0123456789");
			sscanf(found, "%d", &count);
			if (asprintf((char **)&judgment->sMessage, "Avira Found %d infections: ", count) == -1)
			{
				rzb_log(LOG_ERR, "Avira(%s): Failed to allocate message", __func__);
				free(buffer);
				free(cmd);
				return JUDGMENT_REASON_ERROR;
			}
		}
		else
		{
			if (asprintf((char **)&judgment->sMessage, "Avira Found: ") == -1)
			{
				rzb_log(LOG_ERR, "Avira(%s): Failed to allocate message", __func__);
				free(buffer);
				free(cmd);
				return JUDGMENT_REASON_ERROR;
			}
		}
		found = strstr(buffer , "ALERT:");
		while (found != NULL)
		{
			
			found = strchr(found, '[');
			tmp = strchr(found, ']');
			*tmp = '\0';
			// Skip the last space
			Metadata_Add_MalwareName(judgment->pMetaDataList, "Avira", found+1);
			if (asprintf(&tmpMessage, "%s %s", judgment->sMessage, found+1) == -1)
				rzb_log(LOG_ERR, "Avira(%s): Failed to update judgment message", __func__);
			else
			{
				free(judgment->sMessage);
				judgment->sMessage = tmpMessage;
			}
			*tmp = ']';
			found = strstr(found , "ALERT:");
		}
		
		Razorback_Render_Verdict(judgment);
		break;
	

	case AVIRA_HELP:
	case AVIRA_INVALID_OPTION:
	case AVIRA_INVALID_PATH:
	case AVIRA_INVALID_LOG:
	case AVIRA_MISSING_LIB:
	case AVIRA_FAILED_CHECK:
	case AVIRA_FAILED_DEFS:
	case AVIRA_FAILED_ENGINE:
	case AVIRA_FAILED_LICENSE:
	case AVIRA_FAILED_SELF:
	case AVIRA_NO_ACCESS_FILE:
	case AVIRA_NO_ACCESS_DIR:
	default:
		rzb_log(LOG_ERR, "Avira(%s): Scan command failed: %Id", __func__, read);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;
	}

	Judgment_Destroy(judgment);
	free(cmd);
	free(buffer);
    return JUDGMENT_REASON_DONE;
}

SO_PUBLIC DECL_NUGGET_INIT
{
	uuid_t l_pTempUuid;
	uuid_t l_uuidList;
    rzb_log(LOG_DEBUG, "Avira Nugget: Initializing");
    

    if (!readMyConfig(NULL, AVG_CONFIG, sg_Config))
	{
		rzb_log(LOG_ERR, "Avira Nugget: Failed to read config");
		return false;
	}
    if (sg_sAviraPath == NULL)
    {
        rzb_log(LOG_ERR, "Avira Nugget: No log File Configured");
        return false;
    }

    
    if (!UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_uuidList))
    {
        rzb_log(LOG_ERR, "Avira Nugget: Failed to get ANY_DATA UUID");
        return false;
    }

    uuid_copy(l_pTempUuid, AVIRA_NUGGET);


    if ((sg_pContext = Razorback_Init_Inspection_Context (
        sg_uuidNuggetId, l_pTempUuid, 1, &l_uuidList,
        &sg_InspectionHooks, sg_initThreads, sg_maxThreads)) == NULL)
    {
        return false;
    }
    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}

