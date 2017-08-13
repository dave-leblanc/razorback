// agvNugget.cpp : Defines the exported functions for the DLL application.
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

#define AVG_NOT_FOUND 0
#define AVG_USER_STOP 1
#define AVG_ERROR 2
#define AVG_WARNING 3
#define AVG_PUP_DETECTED 4
#define AVG_VIRUS 5
#define AVG_PWDARCHIVE 6

#pragma comment(lib, "api.lib")
#include "bobins.h"

#define AVG_CONFIG "avg.conf"
UUID_DEFINE(AVG_NUGGET, 0xcf, 0x40, 0xe4, 0xcf, 0x9d, 0x07, 0x4f, 0xf5, 0x9b, 0xb0, 0x39, 0x22, 0x89, 0xde, 0xdc, 0xb5);
//cf40e4cf-9d07-4ff5-9bb0-392289dedcb5
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

struct RazorbackContext *sg_pContext = NULL;
static char *sg_sAvgPath = NULL;

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
    {"AvgPath", RZB_CONF_KEY_TYPE_STRING, &sg_sAvgPath, NULL},
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
C:\Program Files (x86)\AVG\AVG2012>avgscanx.exe /heur /arc /thoroughscan /scan=e:\razorback-data\21
684418 /macrow /pwdw /priority=high /ads
#endif
#define CMD_FMT "\"\"%s\"\" /heur /arc /thoroughscan /macrow /pwdw /priority=high /ads /scan=%s"
DECL_INSPECTION_FUNC(file_handler)
{
	char *cmd;
	FILE   *pPipe;
	char   *buffer, *tmp, *tmp2;
	size_t pos = 0;
	size_t read =0;
	size_t size = 1024;
	int count = 0, i =0;
	char *found, *rawName;
	char *tmpMessage;
	struct Judgment *judgment = NULL;

	if ((buffer = calloc(size+1, sizeof(char))) == NULL)
	{
		rzb_log(LOG_ERR, "AVG(%s): Failed to allocate output buffer", __func__);
		return JUDGMENT_REASON_ERROR;
	}

	if (asprintf(&cmd, CMD_FMT, sg_sAvgPath, block->data.fileName) == -1)
	{
		rzb_log(LOG_ERR, "AVG(%s): Failed to allocate command string", __func__);
		return JUDGMENT_REASON_ERROR;
	}
	//rzb_log(LOG_DEBUG, "AVG CMD: %s", cmd);
	if( (pPipe = _popen( cmd, "rt" )) == NULL )
	{
		rzb_log(LOG_ERR, "AVG(%s): Failed to open pipe to command", __func__);
		return JUDGMENT_REASON_ERROR;
	}

    /* Read pipe until end of file. */
	while( !feof( pPipe ) )
	{
		
		
		read = fread( buffer+pos, 1, size-pos, pPipe );
		if( ferror( pPipe ) )      {
			rzb_log(LOG_ERR, "AVG(%s): Read error: %s", __func__, strerror(errno) );
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
				rzb_log(LOG_ERR, "AVG(%s): Realloc failure", __func__);
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
		rzb_log(LOG_ERR, "AVG(%s): Failed to allocate judgment", __func__);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;
	}
	switch (read)
	{
	case AVG_NOT_FOUND:
		break;
	case AVG_WARNING:
		if (asprintf(&judgment->sMessage, "AVG Generated a warning") == -1)
		{
			rzb_log(LOG_ERR, "AVG(%s): Failed to allocate message", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
	case AVG_PUP_DETECTED:
		if (read == AVG_PUP_DETECTED)
		{
			if (asprintf(&judgment->sMessage, "AVG detected a potentially unwanted program") == -1)
			{
				rzb_log(LOG_ERR, "AVG(%s): Failed to allocate message", __func__);
				free(buffer);
				free(cmd);
				return JUDGMENT_REASON_ERROR;
			}
		}
	case AVG_PWDARCHIVE:
		if (read == AVG_PWDARCHIVE)
		{
			if (asprintf(&judgment->sMessage, "AVG detected a password protected archive") == -1)
			{
				rzb_log(LOG_ERR, "AVG(%s): Failed to allocate message", __func__);
				free(buffer);
				free(cmd);
				return JUDGMENT_REASON_ERROR;
			}		
		}	
		// Dodgy
		judgment->Set_SfFlags = SF_FLAG_DODGY;
		judgment->iPriority = 1;
		Metadata_Add_Report(judgment->pMetaDataList,buffer);
		Razorback_Render_Verdict(judgment);
		break;
	case AVG_VIRUS:
		

		judgment->Set_SfFlags = SF_FLAG_BAD;
		judgment->iPriority = 1;
		// Add the report string;
		Metadata_Add_Report(judgment->pMetaDataList,buffer);
		// WTF Why doesnt this work?  It works on linux
		//sscanf(buffer, "Found infections    :%d", &count);
		found = strstr(buffer, "Found infections");
		found += strcspn(found, "0123456789");
		sscanf(found, "%d", &count);
		if (asprintf((char **)&judgment->sMessage, "AVG Found %d infections: ", count) == -1)
		{
			rzb_log(LOG_ERR, "AVG(%s): Failed to allocate message", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
		
		for (i =0; i <= count; i++)
		{
			if ((found = strstr(found == NULL ? buffer : found , block->data.fileName)) == NULL)
			{
				//rzb_log(LOG_ERR, "AVG(%s): Failed to locate infection name", __func__);
				continue; // Break so we still send the judment with missing metadata.
			}
			found += strlen(block->data.fileName)+1;
			tmp = strchr(found, '\n');
			*tmp = '\0';
			// Skip the last space
			tmp2 = strrchr(found, ' '); *tmp2 = '\0';
			rawName = strrchr(found, ' ');
			if (rawName == NULL)
			{
				rzb_log(LOG_ERR, "NULL");
				rawName = found;
			}
			Metadata_Add_MalwareName(judgment->pMetaDataList, "AVG", rawName+1);
			if (asprintf(&tmpMessage, "%s %s", judgment->sMessage, rawName+1) == -1)
				rzb_log(LOG_ERR, "AVG(%s): Failed to update judgment message", __func__);
			else
			{
				free(judgment->sMessage);
				judgment->sMessage = tmpMessage;
			}
			*tmp = '\n';
			*tmp2 = ' ';
		}
		
		Razorback_Render_Verdict(judgment);
		break;
	case AVG_USER_STOP:
		rzb_log_remote(LOG_ERR, eventId, "AVG: Scan process was interupted by a user");
	case AVG_ERROR:
		if (read == AVG_ERROR)
			rzb_log_remote(LOG_ERR, eventId, "AVG: Scan process failed with an error");
	default:
		rzb_log(LOG_ERR, "AVG(%s): Scan command failed: %Id", __func__, read);
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
    rzb_log(LOG_DEBUG, "AVG Nugget: Initializing");
    

    if (!readMyConfig(NULL, AVG_CONFIG, sg_Config))
	{
		rzb_log(LOG_ERR, "AVG Nugget: Failed to read config");
		return false;
	}
    if (sg_sAvgPath == NULL)
    {
        rzb_log(LOG_ERR, "AVG Nugget: No AVG Path Configured");
        return false;
    }

    
    if (!UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_uuidList))
    {
        rzb_log(LOG_ERR, "AVG Nugget: Failed to get ANY_DATA UUID");
        return false;
    }

    uuid_copy(l_pTempUuid, AVG_NUGGET);


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

