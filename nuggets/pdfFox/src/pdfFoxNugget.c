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

#include "foxparse.h"
#include "foxreport.h"

#define PDFFOX_CONFIG_FILE "pdffox.conf"
UUID_DEFINE(PDFFOX, 0xd7, 0x21, 0xe5, 0xf0, 0xa5, 0xb7, 0x4c, 0xea, 0x9e, 0xae, 0x41, 0x11, 0x11, 0x7d, 0x72, 0xc4);

static uuid_t sg_uuidNuggetId;

static RZBConfKey_t pdffox_config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
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

DECL_INSPECTION_FUNC(file_handler)
{
        struct Judgment *judgment;
        if ((judgment = Judgment_Create(eventId, block->pId)) == NULL) {
            rzb_log(LOG_ERR, "%s: failed to create judgment struct");
            return JUDGMENT_REASON_ERROR;
        }

        initPDFFoxJudgment(judgment);

	if (createPDFTree(block->data.file) == false) {
		rzb_log(LOG_ERR, "PDF Parsing terminated prematurely.");
                Judgment_Destroy(judgment);		
                return JUDGMENT_REASON_ERROR;
	}

        judgment->sMessage = NULL;
        Judgment_Destroy(judgment);

	return JUDGMENT_REASON_DONE;
}


SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_uuidList[1];
    uuid_t l_pTempUuid;    
    
    if (!UUID_Get_UUID(DATA_TYPE_PDF_FILE, UUID_TYPE_DATA_TYPE, l_uuidList[0]))
    {
        rzb_log(LOG_ERR, "File Log Nugget: Failed to get PDF_FILE UUID");
        return false;
    }
    uuid_copy(l_pTempUuid, PDFFOX);

    if (!readMyConfig(NULL, PDFFOX_CONFIG_FILE, pdffox_config)) { 
        return false;
    }
    // XXX: We are not thread safe
    sg_pContext= Razorback_Init_Inspection_Context (
        sg_uuidNuggetId, l_pTempUuid, 1, l_uuidList,
        &sg_InspectionHooks, 1, 1);

	return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}
