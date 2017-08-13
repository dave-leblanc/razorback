// avastNugget.cpp : Defines the exported functions for the DLL application.
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

#define AVAST_CONFIG "avast.conf"
UUID_DEFINE(AVAST_NUGGET, 0xd8, 0x9b, 0x17, 0x95, 0xd3, 0x18, 0x4e, 0xa1, 0xb0, 0x04, 0xaf, 0xbf, 0x3a, 0xc8, 0xf7, 0x9d);
//d89b1795-d318-4ea1-b004-afbf3ac8f79d
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

struct RazorbackContext *sg_pContext = NULL;
static char *sg_sAvastPath = NULL;

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(file_handler);

static struct RazorbackInspectionHooks sg_InspectionHooks ={
    file_handler,
    processDeferredList,
};

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"AvastPath", RZB_CONF_KEY_TYPE_STRING, &sg_sAvastPath, NULL},
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
C:\Program Files\AVAST Software\Avast>ashCmd.exe --console --checkgenuine=1 
	  --continue=4 --dontpanic --heuristics=100 --soundoff --archivetype=A ashBase.dll
#endif
#define CMD_FMT "\"\"%s\"\" --console --checkgenuine=1 --continue=4 --dontpanic --heuristics=100 --soundoff --archivetype=A %s"
DECL_INSPECTION_FUNC(file_handler)
{
	PVOID OldValue = NULL;
	char *cmd;
	FILE   *pPipe;
	char   *buffer, *tmp;//, *tmp2;
	size_t pos = 0;
	size_t read =0;
	size_t size = 1024;
	int count = 0, i =0;
	char *found, *rawName;
	struct Judgment *judgment = NULL;
	rzb_log(LOG_ERR, "Avast(%s): hello", __func__);
	if ((buffer = calloc(size+1, sizeof(char))) == NULL)
	{
		rzb_log(LOG_ERR, "Avast(%s): Failed to allocate output buffer", __func__);
		return JUDGMENT_REASON_ERROR;
	}

	if (asprintf(&cmd, CMD_FMT, sg_sAvastPath, block->data.fileName) == -1)
	{
		rzb_log(LOG_ERR, "Avast(%s): Failed to allocate command string", __func__);
		return JUDGMENT_REASON_ERROR;
	}
	//rzb_log(LOG_DEBUG, "Avast CMD: %s", cmd);	
	Wow64DisableWow64FsRedirection(&OldValue);
	if( (pPipe = _popen( cmd, "rt" )) == NULL )
	{
		rzb_log(LOG_ERR, "Avast(%s): Failed to open pipe to command", __func__);
		return JUDGMENT_REASON_ERROR;
	}
	Wow64RevertWow64FsRedirection(OldValue);
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
	//rzb_log(LOG_ERR, "Exit: %Id", read);
	if ( read != 0)
	{
		if ((judgment = Judgment_Create(eventId, block->pId)) == NULL)
		{
			rzb_log(LOG_ERR, "Avast(%s): Failed to allocate judgment", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
		judgment->Set_SfFlags = SF_FLAG_BAD;
		judgment->iPriority = 1;
		// Add the report string;
		Metadata_Add_Report(judgment->pMetaDataList,buffer);
		// WTF Why doesnt this work?  It works on linux
		//sscanf(buffer, "Found infections    :%d", &count);
		
		//rzb_log(LOG_ERR, "--- %s", buffer);
		
		found = buffer;
		
		if ((found = strstr(found, block->data.fileName)) == NULL)
		{
			rzb_log(LOG_ERR, "Avast(%s): Failed to locate infection name", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
		found += strlen(block->data.fileName)+1;
		tmp = strchr(found, '\n');
		*tmp = '\0';
		// Skip the last space
		//tmp2 = strrchr(found, ' '); *tmp2 = '\0';
		rawName = strrchr(found, '\t');
		if (rawName == NULL)
		{
			//rzb_log(LOG_ERR, "NULL");
			rawName = found-1;
		}

		Metadata_Add_MalwareName(judgment->pMetaDataList, "Avast", rawName+1);
		
		if (asprintf((char **)&judgment->sMessage, "Avast Found: %s", rawName+1) == -1)
		{
			rzb_log(LOG_ERR, "Avast(%s): Failed to allocate message", __func__);
			free(buffer);
			free(cmd);
			return JUDGMENT_REASON_ERROR;
		}
		*tmp = '\n';
		//*tmp2 = ' ';
		Razorback_Render_Verdict(judgment);
		Judgment_Destroy(judgment);
	}
	free(cmd);
	free(buffer);
    return JUDGMENT_REASON_DONE;
}

SO_PUBLIC DECL_NUGGET_INIT
{
	uuid_t l_pTempUuid;
	uuid_t l_uuidList;
    rzb_log(LOG_DEBUG, "Avast Nugget: Initializing");
    

    if (!readMyConfig(NULL, AVAST_CONFIG, sg_Config))
	{
		rzb_log(LOG_ERR, "Avast Nugget: Failed to read config");
		return false;
	}
    if (sg_sAvastPath == NULL)
    {
        rzb_log(LOG_ERR, "Avast Nugget: No log File Configured");
        return false;
    }

    
    if (!UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_uuidList))
    {
        rzb_log(LOG_ERR, "Avast Nugget: Failed to get ANY_DATA UUID");
        return false;
    }

    uuid_copy(l_pTempUuid, AVAST_NUGGET);


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

