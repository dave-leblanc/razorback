// KaspNugget.cpp : Defines the exported functions for the DLL application.
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

#define KASP_FINISHED 0
#define KASP_INCORRECT_PARAM 1
#define KASP_UNKNOWN_ERR 2
#define KASP_TASK_ERR 3
#define KASP_CANCELED_EXEC 4
#define KASP_PROCESSED 101
#define KASP_SUCCESS 102

#pragma comment(lib, "api.lib")
#include "bobins.h"

#define KASP_CONFIG "kaspersky.conf"
UUID_DEFINE(KASP_NUGGET, 0xdc, 0xb1, 0xd8, 0x1f, 0xe4, 0x4d, 0x44, 0x61, 0xb1, 0x7c, 0x63, 0x65, 0xc0, 0xf5, 0xff, 0x24);
//dcb1d81f-e44d-4461-b17c-6365c0f5ff24
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

struct RazorbackContext *sg_pContext = NULL;
static char *sg_sKaspPath = NULL;

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
    {"KasperskyPath", RZB_CONF_KEY_TYPE_STRING, &sg_sKaspPath, NULL},
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
Command line goes here
#endif
#define CMD_FMT "\"\"%s\"\" scan %s /i0"
DECL_INSPECTION_FUNC(file_handler)
{
	char *cmd;
	FILE   *pPipe;
	char   *buffer, *tmp;
	size_t pos = 0;
	size_t read =0;
	size_t size = 1024;
	int count = 0, i =0;
	char *found, *rawName;
	struct Judgment *judgment = NULL;

	if ((buffer = calloc(size+1, sizeof(char))) == NULL)
	{
		rzb_log(LOG_ERR, "Kaspersky(%s): Failed to allocate output buffer", __func__);
		return JUDGMENT_REASON_ERROR;
	}

	if (asprintf(&cmd, CMD_FMT, sg_sKaspPath, block->data.fileName) == -1)
	{
		rzb_log(LOG_ERR, "Kaspersky(%s): Failed to allocate command string", __func__);
		return JUDGMENT_REASON_ERROR;
	}
	//rzb_log(LOG_DEBUG, "Kaspersky CMD: %s", cmd);
	if( (pPipe = _popen( cmd, "rt" )) == NULL )
	{
		rzb_log(LOG_ERR, "Kaspersky(%s): Failed to open pipe to command", __func__);
		return JUDGMENT_REASON_ERROR;
	}

    /* Read pipe until end of file. */
	while( !feof( pPipe ) )
	{
		
		read = fread( buffer+pos, 1, size-pos, pPipe );
		if( ferror( pPipe ) )      {
			rzb_log(LOG_ERR, "Kaspersky(%s): Read error: %s", __func__, strerror(errno) );
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
				rzb_log(LOG_ERR, "Kaspersky(%s): Realloc failure", __func__);
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
		rzb_log(LOG_ERR, "Kaspersky(%s): Failed to allocate judgment", __func__);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;
	}
	switch (read)
	{

	case KASP_FINISHED:
		break;

	case KASP_INCORRECT_PARAM:
		rzb_log(LOG_ERR, "Kaspersky(%s): Incorrect parameter(s).", __func__);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;

	case KASP_UNKNOWN_ERR:
		rzb_log(LOG_ERR, "Kaspersky(%s): Unknown Error.", __func__);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;

	case KASP_TASK_ERR:
		rzb_log(LOG_ERR, "Kaspersky(%s): Failed to Execute Task.", __func__);
		free(buffer);
		free(cmd);
		return JUDGMENT_REASON_ERROR;

	case KASP_PROCESSED:
	case KASP_SUCCESS:
		judgment->Set_SfFlags = SF_FLAG_BAD;
		judgment->iPriority = 1;
		// Add the report string;
		Metadata_Add_Report(judgment->pMetaDataList,buffer);
		// WTF Why doesnt this work?  It works on linux
		//sscanf(buffer, "Found infections    :%d", &count);
		if ((found = strstr(buffer, "Total detected:")) == NULL) 
		{
			rzb_log(LOG_ERR, "Kaspersky(%s): Failed to find infections.", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
		found += strcspn(found, "0123456789");
		sscanf(found, "%d", &count);
		if (asprintf((char **)&judgment->sMessage, "Kaspersky Found %d infections", count) == -1)
		{
			rzb_log(LOG_ERR, "Kaspersky(%s): Failed to allocate message", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}

		if ((found = strstr(buffer, block->data.fileName)) == NULL)
		{
			rzb_log(LOG_ERR, "Kaspersky(%s): Failed to locate file name", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}

		found += strlen(block->data.fileName);

		for (i =0; i < count;)
		{
			if ((found = strstr(found, block->data.fileName)) == NULL)
			{
				rzb_log(LOG_ERR, "Kaspersky(%s): Failed to locate infection", __func__);
				free(buffer);
				free(cmd);
				return JUDGMENT_REASON_ERROR;
			}
			found += strlen(block->data.fileName)+1;
			tmp = strchr(found, '\n');
			*tmp = '\0';
			// Skip the last space
			rawName = strstr(found, "detected");
			if (rawName == NULL)
			{
				rawName = found;
				*tmp = '\n';
				continue;
			}
			
			rawName += 8;
			while (*rawName == ' ') 
				rawName++;

			Metadata_Add_MalwareName(judgment->pMetaDataList, "Kaspersky", rawName+1);
			*tmp = '\n';
			found = tmp;
			i++;
		}

		Razorback_Render_Verdict(judgment);
		break;

	default:

		rzb_log(LOG_ERR, "Kaspersky(%s): Scan command failed: %Id", __func__, read);
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
    rzb_log(LOG_DEBUG, "Kaspersky Nugget: Initializing");
    

    if (!readMyConfig(NULL, KASP_CONFIG, sg_Config))
	{
		rzb_log(LOG_ERR, "Kaspersky: Failed to read config");
		return false;
	}
    if (sg_sKaspPath == NULL)
    {
        rzb_log(LOG_ERR, "Kaspersky: No log File Configured");
        return false;
    }

    
    if (!UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_uuidList))
    {
        rzb_log(LOG_ERR, "Kaspersky: Failed to get ANY_DATA UUID");
        return false;
    }

    uuid_copy(l_pTempUuid, KASP_NUGGET);


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

