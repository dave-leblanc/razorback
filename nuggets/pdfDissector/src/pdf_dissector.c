#if defined(__FreeBSD__)
#define _WITH_GETLINE
#endif
#include "config.h"

#include <razorback/debug.h>
#include <razorback/api.h>
#include <razorback/config_file.h>
#include <razorback/ntlv.h>
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/visibility.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>
#include <razorback/block_id.h>
#include <razorback/block_pool.h>
#include <razorback/submission.h>
#include <razorback/socket.h>

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#define PDF_DISSECTOR_CONFIG "pdfdissector.conf"
#define DISSECTOR_PY "dissector.py"
UUID_DEFINE(PDF, 0x8d, 0x41, 0x2e, 0x36, 0xaf, 0xaf, 0x11, 0xdf, 0xb1, 0x23, 0xfb, 0xc1, 0x42, 0xfa, 0x74, 0x9e);

char *jython_path;
char *jython_args;
char *base_path;
char *zynamics_path;
conf_int_t pdf_dissector_port;
char *pdf_dissector_host = NULL;

sem_t sg_sem; // Semaphore to know when submission is complete

static uuid_t sg_uuidNuggetId;
static struct RazorbackContext *sg_pContext;

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(Pdf_Detection);

static struct RazorbackInspectionHooks sg_InspectionHooks = {
	Pdf_Detection,
	processDeferredList,
    NULL,
    NULL
};

static RZBConfKey_t sg_Config[] = {
	{"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL	},
    { "JYTHON_PATH", RZB_CONF_KEY_TYPE_STRING, &jython_path, NULL },
    { "JYTHON_ARGS", RZB_CONF_KEY_TYPE_STRING, &jython_args, NULL },
    { "BASE_PATH", RZB_CONF_KEY_TYPE_STRING, &base_path, NULL },
    { "ZYNAMICS_PATH", RZB_CONF_KEY_TYPE_STRING, &zynamics_path, NULL },
    { "PDF_DISSECTOR_PORT", RZB_CONF_KEY_TYPE_INT, &pdf_dissector_port, NULL },
    { "PDF_DISSECTOR_HOST", RZB_CONF_KEY_TYPE_STRING, &pdf_dissector_host, NULL },
    { NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL }
};

void
callback (struct BlockPoolItem *p_pItem)
{
    sem_post (&sg_sem);
}

DECL_INSPECTION_FUNC(Pdf_Detection)
{
    char tmpFilePath[L_tmpnam];
    char *cmd = NULL;
    size_t len = 0;
	FILE *pf = NULL;
	struct Socket *sock = NULL;
    char *line = NULL;
	struct Judgment *judgment = NULL;
	ssize_t sd_size = 0;
    uint8_t *data_block;
	char *short_data = NULL;
	struct BlockPoolItem *l_pBlock = NULL;
    uint32_t sfFlags, entFlags;
    rzb_log(LOG_DEBUG, "pdf_nugget received %ju bytes of data", block->pId->iLength);


    if (asprintf(&cmd, "FILE %s\n", block->data.fileName) == -1) 
    {
		rzb_log(LOG_ERR, "Could not create file command for passing");
        // OH FYUCK
        remove(tmpFilePath);
        return JUDGMENT_REASON_ERROR;
    }

	if((sock = Socket_Connect((uint8_t *)pdf_dissector_host, pdf_dissector_port)) == NULL) {
		rzb_perror("Error connecting to pdf dissector service: %s");
        return JUDGMENT_REASON_ERROR;
	}
	
	if((pf = fdopen(sock->iSocket, "r")) == NULL) {
		rzb_perror("Error creating file pointer: %s\n");
		Socket_Close(sock);
        return JUDGMENT_REASON_ERROR;
	}
	
	if(!Socket_Tx(sock, strlen(cmd), (uint8_t *)cmd)) {
		rzb_perror("Error sending file information: %s");
		Socket_Close(sock);
        return JUDGMENT_REASON_ERROR;
	}
		
    // ALERT
    // <msg data>
    // REPORT
    // <size of data>
    // <data>
    // ENDALERT
    
    // DATA
    // <Type name>
    // <size of data>
    // <data>
    // ?ALERT?++
    // ENDDATA
    int state = 0;
    // Read all alerts generated
    while (!feof(pf))
    {
        if (getline(&line, &len, pf) > 0)
        {
            if (state == 0 || state == 2) 
            {
                if (!strncmp(line, "ALERT", 5))
                {
                    if (state == 0)
                    {
                        if((judgment = Judgment_Create(eventId, block->pId)) == NULL) {
                            rzb_perror("Unable to create judgment: %s");	
                            return JUDGMENT_REASON_ERROR;
                        }
                    }
                    else
                    {
                        // XXX: FILTHY HACK DONT DO THIS
                        if((judgment = Judgment_Create(l_pBlock->pEvent->pId, block->pId)) == NULL) {
                            rzb_perror("Unable to create judgment: %s");	
                            return JUDGMENT_REASON_ERROR;
                        }
                    }

                    // Set up our static fields
                    judgment->iPriority = 1;
                    judgment->Set_SfFlags = SF_FLAG_BAD;
                    free(line);
                    line = NULL;
                    len =0;
                    // Read the next line which should have our alert text
                    if (getline(&line, &len, pf) > 0)
                    {
                        line[strlen(line) - 1] = 0x0;
                        judgment->sMessage = (uint8_t *)strdup(line);	
                    }
                    state++;
                }
                else if (!strncmp(line, "DATA", 4) && state == 0)
                {
                    if((l_pBlock = RZB_DataBlock_Create(sg_pContext)) == NULL) {
                        rzb_perror("Error creating DataBlock: %s");
                        return JUDGMENT_REASON_ERROR;
                    }
                    free(line);
                    line= NULL;
                    len=0;
                    if (getline(&line, &len, pf) <= 0)
                    {
                        rzb_log(LOG_ERR, "Error reading type");
                        BlockPool_DestroyItem(l_pBlock);
                        return JUDGMENT_REASON_ERROR;
                    }
                    if (strncmp(line, "ANY", 3) != 0)
                    {
                        line[strlen(line) - 1] = 0x0;
                        RZB_DataBlock_Set_Type(l_pBlock, line);
                    }
                    l_pBlock->pEvent->pBlock->pParentId = BlockId_Clone(block->pId);
                    //Our next line should just be an integer value with our data length
                    if (fscanf(pf, "%zi\n", &sd_size) == EOF)
                    {
                        rzb_log(LOG_ERR, "Error reading data size");
                        BlockPool_DestroyItem(l_pBlock);
                        return JUDGMENT_REASON_ERROR;
                    }
                    if ((data_block = malloc(sd_size)) == NULL)
                    {
                        rzb_log(LOG_ERR, "Error allocating data");
                        BlockPool_DestroyItem(l_pBlock);
                        return JUDGMENT_REASON_ERROR;
                    }
                    // Read our data
                    if (fread(data_block, sd_size, 1, pf) != 1)
                    {
                        rzb_log(LOG_ERR, "Error allocating data");
                        free(data_block);
                        BlockPool_DestroyItem(l_pBlock);
                        return JUDGMENT_REASON_ERROR;
                    }
                    BlockPool_AddData(l_pBlock, data_block, sd_size, BLOCK_POOL_DATA_FLAG_MALLOCD);
                    BlockPool_FinalizeItem(l_pBlock);
                    l_pBlock->submittedCallback = callback;
                    if (Submission_Submit(l_pBlock, BLOCK_POOL_FLAG_MAY_REUSE, &sfFlags, &entFlags) != RZB_SUBMISSION_OK)
                    {
                        rzb_log(LOG_ERR, "Error submitting");
                        BlockPool_DestroyItem(l_pBlock);
                        return JUDGMENT_REASON_ERROR;
                    }
                    // Wait for submission so we know this is in the DB
                    sem_wait (&sg_sem);
                    state = 2;
                }
                else if (!strncmp(line, "ENDDATA", 7))
                {
                    BlockPool_DestroyItem(l_pBlock);
                    state=0;
                }
            }
            else if (state == 1 || state == 3) 
            {
                if (!strncmp(line, "REPORT", 6))
                {
                    // Our next line should just be an integer value with our data length
                    if (fscanf(pf, "%zi\n", &sd_size) != EOF)
                    {
                        rzb_log(LOG_DEBUG, "Received report data: %d bytes", sd_size);
                        // Make sure we actually have data to work with
                        if (sd_size > 0)
                        {
                            // Allocate our storage for our data
                            if ((short_data = malloc(sd_size)) != NULL)
                            {
                                // Read our data
                                if (fread(short_data, sd_size, 1, pf) != 1)
                                {
                                    free(short_data);
                                    short_data = NULL;
                                }
                            }

                            Metadata_Add_Report(judgment->pMetaDataList, short_data);
                            free(short_data);
                        }
                    }
                }
                else if (!strncmp(line, "ENDALERT", 8))
                {
                    Razorback_Render_Verdict(judgment);
                    Judgment_Destroy(judgment);
                    state--;
                }
            }
            free(line);
            line=NULL;
            len=0;
        }
    }
    pclose(pf);
    free(cmd);

    // Clean up
    if (remove(tmpFilePath) != 0)
    {
        rzb_log(LOG_ERR, "Error while deleting temp file %s", tmpFilePath);
        return JUDGMENT_REASON_ERROR;
    }
    if (line)
        free(line);

	return JUDGMENT_REASON_DONE;
}

SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_pList[1];
	uuid_t l_pPdfUuid;
    sem_init (&sg_sem, 0, 0);

	UUID_Get_UUID(DATA_TYPE_PDF_FILE, UUID_TYPE_DATA_TYPE, l_pList[0]);
    uuid_copy(l_pPdfUuid, PDF);	

	if (!readMyConfig(NULL, PDF_DISSECTOR_CONFIG, sg_Config)) {
		rzb_log(LOG_ERR, "%s: PDF Dissector Nugget: Failed to read nugget configuration file", __func__);
		return false;
	}

	sg_pContext = Razorback_Init_Inspection_Context (
		sg_uuidNuggetId, l_pPdfUuid, 1, l_pList,
		&sg_InspectionHooks, 1,1);
	
	return true;
}

bool
processDeferredList (struct DeferredList *p_pDefferedList)
{
    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}
