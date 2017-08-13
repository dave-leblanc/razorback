#include "config.h"
#if defined(__FreeBSD__)
#define _WITH_GETLINE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <uuid/uuid.h>
#include <errno.h>
#include <arpa/inet.h>

#include <razorback/debug.h>
#include <razorback/config_file.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>
#include <razorback/visibility.h>
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/api.h>

#define OFFICECAT_CONFIG_FILE "officecat.conf"
UUID_DEFINE(OFFICE_CAT, 0x73, 0x39, 0x2f, 0x48, 0xaf, 0xaf, 0x11, 0xdf, 0xa5, 0x19, 0x8b, 0x58, 0xa5, 0x42, 0xa7, 0xe9);

const char *officecat_path;
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

static RZBConfKey_t officecat_config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    { "OFFICECAT_PATH", RZB_CONF_KEY_TYPE_STRING, &officecat_path, NULL },
    {"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    { NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL }
};

static struct RazorbackContext *sg_pContext;

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(file_handler);

static struct RazorbackInspectionHooks sg_InspectionHooks = {
    file_handler,
    processDeferredList,
    NULL,
    NULL
};

bool
processDeferredList (struct DeferredList *p_pDefferedList)
{
    return true;
}

#ifdef DEBUG
#define DEBUG_RZB(code) code
#else
#define DEBUG_RZB(code)
#endif

/***********************************************************

*  Title: RZB_scan_officecat

*  Authors: Alain Zidouemba, Sourcefire, Inc.

*  Inputs:
	struct cl_engine * engine: pointer to scan engine
	const char * mount_dir:		path to the directory
					used to mount tmpfs
	char * buffer:			buffer you want to scan
	int buffer_size:		size of the buffer to scan

*  Output:
	int:    0       if successful
                !0      if unsuccessful
* Modified:
	const char ** virname: 		virus name returned, if virus found
					"No virus detected", if no virus found

*  Description:
	Scan a buffer with a ClamAV engine that has been
	initialized with RZB_start_clamav.

***********************************************************/
DECL_INSPECTION_FUNC(file_handler)
{
	FILE *pf;
	char cmd [512];
    char *p;
    char *output_str = NULL;
    int second_to_last_line = 0;
    size_t j =0, line_len =0;
	char message[2048];
	char vuln_name[1024];
    struct Judgment *judgment;

	if ((judgment = Judgment_Create(eventId, block->pId)) == NULL) {
		rzb_log(LOG_ERR, "%s: failed to create judgment struct");
		return JUDGMENT_REASON_ERROR;
	}

	// Build our cmd to run
	snprintf(cmd, sizeof(cmd), "%s %s", officecat_path, block->data.fileName);

	// Create our dissector
    if((pf = popen(cmd, "r")) == NULL)
	{
		rzb_log(LOG_ERR, "Error while running '%s': (%d) %s", cmd, errno, strerror(errno));
    }
    else
	{

		judgment->Set_SfFlags = 1;
		judgment->Set_EntFlags = 0;
		judgment->Unset_SfFlags = 0;
		judgment->Unset_EntFlags = 0;
        judgment->iGID = 6;
        judgment->iSID = 0;

		while(!feof(pf))
		{
            line_len = getline(&output_str, &j, pf);
            if (line_len == 0 ) {
                if (output_str != NULL)
                {
                    free(output_str);
                    output_str = NULL;
                    j = 0;
                }
                continue;
            }

            if (output_str == NULL) {
                rzb_log(LOG_ERR, "Error reading result.");
                return JUDGMENT_REASON_ERROR;
            }
            output_str[strlen(output_str)-1] = '\0';


            if ((p = strstr(output_str, "CORRUPTED:")) != NULL)
            {
            }

            else if ((p = strstr(output_str, "VULNERABLE")) != NULL)
            {
            }

            else if ((p = strstr(output_str, "CVE")) != NULL)
            {
                snprintf(vuln_name, sizeof(vuln_name), "%s", p);
                // TODO: Send result JUDGMENT_REASON_BAD
        		snprintf(message, sizeof(message), "THE FOLLOWING HAS BEEN FOUND BY OFFICECAT: %s", vuln_name);
                rzb_log(LOG_INFO,"%s", message);
                judgment->iSID = 1;
				judgment->sMessage = (uint8_t *)message;
				judgment->iPriority = 1;
				judgment->Set_SfFlags = 2;
				Metadata_Add_CVE(judgment->pMetaDataList, vuln_name);
                Razorback_Render_Verdict(judgment);
            }

            else if ((p = strstr(output_str, "MS")) != NULL)
            {
            }

            else if ((p = strstr(output_str, "embedded ActiveX")) != NULL)
            {
            }

            else if ((p = strstr(output_str, "SAFE")) != NULL)
            {
            }

            else if ((p = strstr(output_str, "Type: ")) != NULL)
            {
            }

            else if ((strlen (output_str) > 5)  && (second_to_last_line == 1))
            {
			}
            free(output_str);
            output_str = NULL;
            j=0;
		}
	}

	judgment->sMessage = NULL;
    Judgment_Destroy(judgment);

	return JUDGMENT_REASON_DONE;
}


SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_uuidList[1];
    uuid_t l_pTempUuid;    
    
    if (!UUID_Get_UUID(DATA_TYPE_OLE_FILE, UUID_TYPE_DATA_TYPE, l_uuidList[0]))
    {
        rzb_log(LOG_ERR, "File Log Nugget: Failed to get OLE_FILE UUID");
        return false;
    }
    uuid_copy(l_pTempUuid, OFFICE_CAT);

    if (!readMyConfig(NULL, OFFICECAT_CONFIG_FILE, officecat_config)) { 
        return false;
    }
    sg_pContext= Razorback_Init_Inspection_Context (
        sg_uuidNuggetId, l_pTempUuid, 1, l_uuidList,
        &sg_InspectionHooks, sg_initThreads, sg_maxThreads);

	return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}
