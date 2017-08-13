
#include <razorback/visibility.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block.h>
#include <razorback/hash.h>
#include <razorback/config_file.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>
#include <razorback/ntlv.h>
#include <razorback/thread.h>
#include <razorback/uuids.h>
#include <libxml/xmlerror.h>
#include <libxml/parser.h>
#include <ftw.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

#define FILE_LOG_CONFIG "scriptNugget.conf"

static const char *rzb_error_tags = "<razorback><error type=%d>%s</error></razorback>";

static char * sg_ScriptDir = NULL;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;
static struct Thread *sg_monitorthread = NULL;

static bool trackScriptName(struct RazorbackContext *context);
bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(file_handler);

static struct RazorbackInspectionHooks sg_InspectionHooks ={
    file_handler,
    processDeferredList,
    NULL,
    NULL
};

RZBConfKey_t sg_Config[] = {
    {"ScriptDir", RZB_CONF_KEY_TYPE_STRING, &sg_ScriptDir, NULL},
    {"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

struct ScriptList {
    struct RazorbackContext *context;
    struct ScriptList *next;
};

static struct ScriptList *sg_scrlist = NULL;

bool 
processDeferredList (struct DeferredList *p_pDefferedList) 
{
    return true;
}

void 
xmlStructuredErrorHandler(void *ctx, xmlErrorPtr p)
{
    rzb_log(LOG_ERR, "XML Error: %s", p->message);
}
void   
xmlGenericErrorHandler (void * ctx, const char * msg, ...)
{
    rzb_log(LOG_ERR, "XML Error: %s", msg);
}
static
void parseDataTypes (xmlDocPtr doc, xmlNodePtr cur, uuid_t **dataTypeList, int *numTypes) {
    xmlChar *value;
    int count = 0;
    uuid_t *list = NULL;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"data_type")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

            if ((list = realloc(list, (count+1) * sizeof(uuid_t))) == NULL) {
                rzb_log(LOG_ERR, "%s: Realloc error.", __func__);
                free(value);
                break;
            }
            
            rzb_log(LOG_INFO, "DataType: %s", value);
            
            if (!UUID_Get_UUID ((const char *)value, UUID_TYPE_DATA_TYPE, list[count])) {
                rzb_log(LOG_ERR, "%s: Could not parse data type uuid.", __func__);
                free(value);
                break;
            }

            count++;
            rzb_log(LOG_INFO, "Data type: %s", value);
            free(value);
        }
        cur = cur->next;
    }

    *numTypes = count;
    *dataTypeList = list;

    return;
}

static
void parseReg (xmlDocPtr doc, xmlNodePtr cur, uuid_t **dataTypeList, int *numTypes, uuid_t *appType, uuid_t *nuggetid) {
    xmlChar *value;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"nugget_id")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if(uuid_parse((char*)value, *nuggetid) == -1) {
                rzb_log(LOG_ERR, "%s: failed to parse nugget id", __func__);
            }
            rzb_log(LOG_INFO, "Nugget ID: %s", value);
            free(value);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"application_type")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            rzb_log(LOG_INFO, "Application Type: %s", value);
            uuid_parse((const char *)value, *appType);
            free(value);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"data_types")) {
            parseDataTypes(doc, cur, dataTypeList, numTypes);
        }
        cur = cur->next;
    }

    return;
}

static
void parseSFflags (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {
    xmlChar *value;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"set")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            judgment->Set_SfFlags = strtoul((const char *)value, NULL, 10);
            rzb_log(LOG_INFO, "SF Flags (set): %s", value);
            free(value);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"unset")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            judgment->Unset_SfFlags = strtoul((const char *)value, NULL, 10);
            rzb_log(LOG_INFO, "SF Flags (unset): %s", value);
            free(value);
        }
        cur = cur->next;
    }

    return;
}

static
void parseENTflags (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {
    xmlChar *value;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"set")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            judgment->Set_EntFlags = strtoul((const char *)value, NULL, 10);
            rzb_log(LOG_INFO, "Ent Flags (set): %s", value);
            free(value);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"unset")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            judgment->Unset_EntFlags = strtoul((const char *)value, NULL, 10);
            rzb_log(LOG_INFO, "Ent Flags (unset): %s", value);
            free(value);
        }
        cur = cur->next;
    }

    return;
}

static
void parseFlags (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
         if (!xmlStrcmp(cur->name, (const xmlChar *)"sourcefire")) {
             parseSFflags (doc, cur, judgment);
         }
         else if (!xmlStrcmp(cur->name, (const xmlChar *)"enterprise")) {
             parseENTflags (doc, cur, judgment);
         }
         cur = cur->next;
    }

    return;
}

static
void parseEntry (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {
    xmlChar *type, *value;
    bool (*metafp)(struct List *, const char *) = NULL;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"type")) {
            type = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            rzb_log(LOG_INFO, "Type: %s", type);
            
            if (!strcmp((const char *)type, "REPORT")) {
                metafp = Metadata_Add_Report;
            }   
            else if (!strcmp((const char *)type, "FILENAME")) {
                metafp = Metadata_Add_Filename;
            }   
            else if (!strcmp((const char *)type, "HOSTNAME")) {
                metafp = Metadata_Add_Hostname;
            }   
            else if (!strcmp((const char *)type, "PATH")) {
                //Metadata_Add_Path (judgment->pMetaDataList, (const char *)value);
            }   
//            else if (!strcmp((const char *)type, "MALWARENAME")) {
//                metafp = Metadata_Add_MalwareName;
//            }
            
            free(type);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"data") && metafp != NULL) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            rzb_log(LOG_INFO, "Value: %s", value);
            (*metafp)(judgment->pMetaDataList, (const char *)value);
            free(value);
        }

        cur = cur->next;
    }
    

    return;
}

static
void parseMetadata (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"entry")) {
            parseEntry(doc, cur, judgment);
        }
        cur = cur->next;
    }   
 
    return;
}

static
void parseVerdict (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {
    xmlChar *value;

    value = xmlGetProp(cur, (const xmlChar *)"priority");
    rzb_log(LOG_INFO, "Priority: %s", value);
    judgment->iPriority = (uint8_t )strtoul((const char *)value, NULL, 10);
    free(value);

    value = xmlGetProp(cur, (const xmlChar *)"gid");
    rzb_log(LOG_INFO, "GID: %s", value);
    judgment->iGID = strtoul((const char *)value, NULL, 10);
    free(value);

    value = xmlGetProp(cur, (const xmlChar *)"sid");
    rzb_log(LOG_INFO, "SID: %s", value);
    judgment->iSID = strtoul((const char *)value, NULL, 10);
    free(value);

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"metadata")) {
            parseMetadata(doc, cur, judgment);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"message")) {
            value = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            judgment->sMessage = value;
            rzb_log(LOG_INFO, "Message: %s", value);
            //Don't free this
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"flags")) {
            parseFlags(doc, cur, judgment);
        }
        cur = cur->next;
    }

    return;
}

static
void parseLog (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {
    xmlChar *value, *message;
    uint8_t level;

    value = xmlGetProp(cur, (const xmlChar *)"level");
    rzb_log(LOG_INFO, "Level: %s", value);
    level = (uint8_t)strtoul((const char *)value, NULL, 10);

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"message")) {
            message = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            rzb_log_remote (level, judgment->pEventId, (const char *)message);
            rzb_log(LOG_INFO, "Message: %s", message);
            free(message);
        }
        cur = cur->next;
    }
    
    free(value);

    return;
}

static
void parseResponse (xmlDocPtr doc, xmlNodePtr cur, struct Judgment *judgment) {

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"verdict")) {
            parseVerdict(doc, cur, judgment);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"log")) {
            parseLog(doc, cur, judgment);
        }
        cur = cur->next;
    }

    return;
}

DECL_INSPECTION_FUNC(file_handler)
{
    int outpipe[2];
    pid_t child;
    char *error;
    xmlDocPtr doc;
    xmlNodePtr cur;
    xmlSetStructuredErrorFunc(NULL, xmlStructuredErrorHandler);
    xmlSetGenericErrorFunc(NULL, xmlGenericErrorHandler);

    struct Judgment *judgment = NULL;

    char *scriptName;

    scriptName = (char *)Thread_GetCurrentContext()->userData;



    if (pipe(outpipe) != 0) {
        rzb_log(LOG_ERR, "%s: failed to create pipe", __func__);
        return JUDGMENT_REASON_ERROR;
    }

    if ((child = fork()) < 0) {
        rzb_log(LOG_ERR, "%s: Failed to fork process.", __func__);
        close(outpipe[0]);
        close(outpipe[1]);
        return JUDGMENT_REASON_ERROR;
    }
    else if (child > 0) {
        //Parent
        close(outpipe[1]);

        if (waitpid(child, NULL, 0) < 0) {
            rzb_log(LOG_ERR, "%s: Script did not terminate properly.", __func__);
            close(outpipe[0]);
            return JUDGMENT_REASON_ERROR;
        }
    
        doc = xmlReadFd(outpipe[0], NULL, NULL, 0);

        if (doc == NULL) {
            rzb_log(LOG_ERR, "%s: Failed to parse XML output.", __func__);
            close(outpipe[0]);
            return JUDGMENT_REASON_DONE;
        }

        cur = xmlDocGetRootElement(doc);

        if (cur == NULL) {
            rzb_log(LOG_ERR, "%s: Empty XML document.", __func__);
            close(outpipe[0]);
            return JUDGMENT_REASON_DONE;
        }

        if (xmlStrcmp(cur->name, (const xmlChar *) "razorback")) {
            rzb_log(LOG_INFO, "%s: Document root tag should be razorback.", __func__);
            close(outpipe[0]);
            return JUDGMENT_REASON_ERROR;
        }

        cur = cur->xmlChildrenNode;
        while (cur != NULL) {
            if (!xmlStrcmp(cur->name, (const xmlChar *)"response")) {
                judgment = Judgment_Create(eventId, block->pId);
                parseResponse(doc, cur, judgment);
            }
            else if (!xmlStrcmp(cur->name, (const xmlChar *)"error")) {
                error = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                rzb_log(LOG_ERR, "%s: %s", __func__, error);
                free(error);
                close(outpipe[0]);
                return JUDGMENT_REASON_ERROR;
            }
            cur = cur->next;
        }

        if (judgment != NULL) {
            if (Razorback_Render_Verdict(judgment) == false) {
                rzb_log(LOG_ERR, "%s: failed to render judgment", __func__);
                Judgment_Destroy(judgment);
                close(outpipe[0]);
                return JUDGMENT_REASON_ERROR;
            }
            Judgment_Destroy(judgment);
        }
        


        close(outpipe[0]);
    }
    else  {
        //Child
        close(outpipe[0]);
        if (outpipe[1] != STDOUT_FILENO) {
            if (dup2(outpipe[1], STDOUT_FILENO) != STDOUT_FILENO) {
                //Write error to pipe
                exit(-1);
            }
            close(outpipe[1]);
        }

        if (execl(scriptName, scriptName, block->data.fileName, (char *)NULL) < 0) {
            printf(rzb_error_tags, 1, "Failed to exec script");
            exit(-1);
        }
    }

    return JUDGMENT_REASON_DONE;
}

static
bool registerScript(const char *scriptName) {

    int dataTypeCount = 0;
    uuid_t *dataTypeList = NULL;
    uuid_t appType;
    uuid_t nugid;
    char *error;

    struct RazorbackContext *context;
    pid_t child;
    xmlDocPtr doc;
    xmlNodePtr cur;

    int outpipe[2];


    if (pipe(outpipe) != 0) {
        rzb_log(LOG_ERR, "%s: Failed to create pipe.", __func__);
        return false;
    }

    if ((child = fork()) < 0) {
        rzb_log(LOG_ERR, "%s: Failed to fork process.", __func__);
        close(outpipe[0]);
        close(outpipe[1]);
        return false;
    }
    else if (child == 0) {

        close(outpipe[0]);
        if (outpipe[1] != STDOUT_FILENO) {
            if (dup2(outpipe[1], STDOUT_FILENO) != STDOUT_FILENO) {
                rzb_log(LOG_ERR, "%s: Failed to redirect stdout to pipe.", __func__);
                //Write some error message to pipe
                exit(-1);
            }
            close(outpipe[1]);
        }

        if (execl(scriptName, scriptName, "--register", (char *)NULL) < 0) {
            //Write some error message to pipe
            exit(-1);
        }
    }

    //Parent
    close(outpipe[1]);

    rzb_log(LOG_INFO, "registering :%s...", scriptName);

    if (waitpid(child, NULL, 0) < 0) {
        rzb_log(LOG_ERR, "%s: Script did not terminate properly.", __func__);
        close(outpipe[0]);
        return false;
    }

    doc = xmlReadFd(outpipe[0], NULL, NULL, 0);

    if (doc == NULL) {
        rzb_log(LOG_ERR, "%s: Failed to parse XML output.", __func__);
        close(outpipe[0]);
        return false;
    }

    cur = xmlDocGetRootElement(doc);

    if (cur == NULL) {
        rzb_log(LOG_ERR, "%s: Empty XML document.", __func__);
        close(outpipe[0]);
        return false;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) "razorback")) {
        rzb_log(LOG_INFO, "%s: Document root tag should be razorback.", __func__);
        close(outpipe[0]);
        return false;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {

        if (!xmlStrcmp(cur->name, (const xmlChar *)"registration")) {
            parseReg(doc, cur, &dataTypeList, &dataTypeCount, &appType, &nugid);
        }
        else if (!xmlStrcmp(cur->name, (const xmlChar *)"error")) {
            error = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            rzb_log(LOG_ERR, "%s: %s", __func__, error);
            free(error);
            close(outpipe[0]);
            return false;
        }
        cur = cur->next;
    }

    close(outpipe[0]);

    if ((context = Razorback_Init_Inspection_Context (
        nugid, appType, dataTypeCount, dataTypeList,
        &sg_InspectionHooks, sg_initThreads, sg_maxThreads)) == NULL) 
    {
        rzb_log(LOG_ERR, "%s: failed to register context for %s", __func__, scriptName);
        return false;
    }

    if ((context->userData = (char *)malloc(strlen(scriptName)+1)) == NULL) {
        rzb_log(LOG_ERR, "%s: failed to malloc space for context user data", __func__);
        return false;
    }

    strcpy(context->userData, scriptName);

    if (!trackScriptName(context)) {
        rzb_log(LOG_ERR, "%s: failed to add script %s to tracking list", __func__, scriptName);
        return false;;
    }

    rzb_log(LOG_INFO, "Done registering...");

    return true;
}

static
bool trackScriptName(struct RazorbackContext *context) {
    
    struct ScriptList *node, *temp;
    
    node = (struct ScriptList *)malloc(sizeof(struct ScriptList));
    
    node->context = context;
    node->next = NULL;

    if (sg_scrlist == NULL) {
        sg_scrlist = node;
    }
    else {
        temp = sg_scrlist;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }

    return true;
}

static
bool isRegistered(const char *name) {
    struct ScriptList *node;

    if (sg_scrlist == NULL)
        return false;

    node = sg_scrlist;

    do {
        if (!strcmp(node->context->userData, name))
            return true;
        node = node->next;
    } while (node != NULL);

    return false;

}

static 
void scriptRollCall() {
	struct ScriptList *node, *temp, **lookbehind;
    FILE *file;

	if (sg_scrlist == NULL)
		return;

	node = sg_scrlist;
	lookbehind = &sg_scrlist;

	do {

		if ((file = fopen(node->context->userData, "r")) == NULL) {
			//File is gone
			//Remove from list
            temp = node;
			*lookbehind = node->next;
			node = node->next;

			rzb_log(LOG_DEBUG, "%s: Script %s not found. Shutting down context.", __func__, temp->context->userData);
			//stop context
            Razorback_Shutdown_Context(temp->context); 

			free(temp);
		}
		else {
			fclose(file);

			lookbehind = &node->next;
			node = node->next;
		}
	} while (node != NULL);

	return;
}

// ftw callback that looks for files with the provided extension
static int processFile(const char *name, const struct stat *status, int type) {
    if (type == FTW_F)
    {
        if (S_ISREG(status->st_mode)) {
            if (!isRegistered(name)) {
                if (registerScript(name) == false) {
                    rzb_log(LOG_ERR, "%s: failed to register script %s", __func__, name);
                }
            }
        }
    }
    return 0;
}
static
void scriptMonitor(struct Thread *thread) {
    xmlSetStructuredErrorFunc(NULL, xmlStructuredErrorHandler);
    xmlSetGenericErrorFunc(NULL, xmlGenericErrorHandler);
   
    while(!Thread_IsStopped(thread)) {
		scriptRollCall();
        ftw(sg_ScriptDir, processFile, 1);
		sleep(10);
    }    
}

SO_PUBLIC DECL_NUGGET_INIT
{
    if (!readMyConfig(NULL, FILE_LOG_CONFIG, sg_Config)) 
        return false;

    if (sg_ScriptDir == NULL)
    {
        rzb_log(LOG_ERR, "ScriptNugget(%s): Configuration failed, ScriptDir not specified", __func__);
        return false;
    }
    if ((sg_monitorthread = Thread_Launch
        (scriptMonitor, NULL, (char *)"scriptMonitor", NULL)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to launch thread.", __func__);
        return false;
    }
    
    rzb_log(LOG_INFO, "Initialized--------");

    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    //Shutdown monitor thread and contexts
    Thread_StopAndJoin(sg_monitorthread);

    while (sg_scrlist != NULL) {
        Razorback_Shutdown_Context(sg_scrlist->context);
        sg_scrlist = sg_scrlist->next;
    }

}
