#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <yara.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ftw.h>
#include <razorback/debug.h>
#include <razorback/api.h>
#include <razorback/config_file.h>
#include <razorback/ntlv.h>
#include <razorback/uuids.h>
#include <razorback/log.h>
#include <razorback/visibility.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>

#define YARA_CONFIG_FILE    "yara.conf"
#define METADATA_FLAG_SET            0x00000001
#define METADATA_FLAG_NOT_SET        0x00000000
UUID_DEFINE(YARA, 0x2f, 0x63, 0x11, 0x18, 0x42, 0xcf, 0x11, 0xe0, 0x83, 0xc8, 0x00, 0x0c, 0x29, 0x8f, 0xbd, 0xa4);
char *rule_path;
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;
static struct Mutex *loadMutex;
static YARA_CONTEXT *loadContext;
struct YaraData 
{
    struct BlockId *blockId;
    struct EventId *eventId;
};

static RZBConfKey_t yara_config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    { "RULE_PATH", RZB_CONF_KEY_TYPE_STRING, &rule_path, NULL },
    {"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    { NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL }
};

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(yara_handler);
DECL_NUGGET_THREAD_INIT(yara_thread_init);
DECL_NUGGET_THREAD_CLEANUP(yara_thread_cleanup);


static struct RazorbackContext *sg_pContext;

static struct RazorbackInspectionHooks sg_InspectionHooks = {
    yara_handler,
    processDeferredList,
    yara_thread_init,
    yara_thread_cleanup
};



int detection_callback(RULE* rule, void* data);
int load_rule_file(const char *, const struct stat *, int);
void report_error(const char* file_name, int line_number, const char* error_message);

// The yara context that will hold all loaded files

// We just need to start the scan for detection and let the callback do the alerting
DECL_INSPECTION_FUNC(yara_handler) {
    YARA_CONTEXT *context = threadData;
    struct YaraData *data;
    if ((data = calloc(1,sizeof(struct YaraData))) == NULL)
        return JUDGMENT_REASON_ERROR;
    data->eventId = eventId;
    data->blockId = block->pId;

	yr_scan_mem(block->data.pointer, block->pId->iLength, context, detection_callback, data);
    free(data);

    return JUDGMENT_REASON_DONE;
}

// Add all rule files in te RULE_PATH to the yara context
int load_rule_file(const char *rule_file_path, const struct stat *stats, int flag) {
    if (flag != FTW_F)
        return 0;
    FILE *rule_file;

    if((rule_file = fopen(rule_file_path, "r")) == NULL) {
        fprintf(stderr, "Error opening rule file: %s\n", rule_file_path);
        return 1;
    }

    yr_push_file_name(loadContext, rule_file_path);

    if(yr_compile_file(rule_file, loadContext)) {
        fprintf(stderr, "Error compiling rule file");
        return 1;
    }
    fclose(rule_file);
	
    return 0;
}

// Check for the requested metadata key and copy the value into the flags field
static int check_meta_int (META *metadata, int *flags, const char *key)
{
    META *metanode;

    if((metanode = lookup_meta(metadata, key)) != NULL){
        if(metanode->type == META_TYPE_INTEGER){
                *flags = metanode->integer;
                return METADATA_FLAG_SET;
        } else 
            rzb_log(LOG_ERR, "%s: Incorrect metadata type in Yara rules file", __func__);
    }

    return METADATA_FLAG_NOT_SET;
}

// If the provided key is set to true, set the provided flag in the flags field
static int check_meta_bool (META *metadata, int *flags, const char *key, int flag)
{
    META *metanode;

    if((metanode = lookup_meta(metadata, key)) != NULL){
        if(metanode->type == META_TYPE_BOOLEAN){
            if(metanode->boolean){
                *flags |= flag;
                return METADATA_FLAG_SET;
            }
        } else 
            rzb_log(LOG_ERR, "%s: Incorrect metadata type in Yara rules file", __func__);
    }

    return METADATA_FLAG_NOT_SET;
}

// Check the custom Sourcefire flags from metadata in the Yara rules file
static int handle_metadata (META *metadata, int *sf_flags, int *ent_flags)
{
    int flags_set = 0; // Used for error checking on sf_flags overwrite

    // Check for individual flags to set
    check_meta_bool(metadata, sf_flags, "rzb_dodgy", SF_FLAG_DODGY);
    check_meta_bool(metadata, sf_flags, "rzb_good", SF_FLAG_GOOD);
    check_meta_bool(metadata, sf_flags, "rzb_bad", SF_FLAG_BAD);
    check_meta_bool(metadata, sf_flags, "rzb_white_list", SF_FLAG_WHITE_LIST);
    check_meta_bool(metadata, sf_flags, "rzb_black_list", SF_FLAG_BLACK_LIST);

    flags_set = *sf_flags;

    // Check for enterprise flag
    check_meta_int(metadata, ent_flags, "rzb_entflags");

    // If sfflags were already set, overwrite previous fields, but log an error
    if((check_meta_int(metadata, sf_flags, "rzb_sfflags")) == METADATA_FLAG_SET && flags_set)
        rzb_log(LOG_ERR, "%s:  Overwrote previous flags with rzb_sfflags metadata", __func__);

    // If both good and bad flags are set, prefer the bad flag and log an error
    if((*sf_flags & SF_FLAG_GOOD) && (*sf_flags & SF_FLAG_BAD)){
	*sf_flags ^= SF_FLAG_GOOD;
	rzb_log(LOG_ERR, "%s:  Block set to both good and bad, clearing good flag", __func__);
    }

    return 0;
}

// This call back is called once there is a successful detection
int detection_callback(RULE* rule, void* data) {
    struct Judgment *judgment = NULL;
    struct YaraData * yData = (struct YaraData *) data;
	int sf_flags = 0;
	int ent_flags = 0;
	META *metadata;

	if(rule->flags & RULE_FLAGS_MATCH) {

		metadata = rule->meta_list_head;

		if(metadata != NULL)
            handle_metadata(metadata, &sf_flags, &ent_flags);

        if ((judgment = Judgment_Create(yData->eventId, yData->blockId)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate judgment structure", __func__);
            return -1;
        }
		// Add our message
		if (asprintf((char **)&judgment->sMessage, "Yara signature detected: %s\n", rule->identifier) == -1)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate judgment structure", __func__);
            return -1;
        }

	    // Set SF and Ent flags, as well as priroity
        if(sf_flags == 0)
            judgment->Set_SfFlags=SF_FLAG_BAD;
        else
            judgment->Set_SfFlags=sf_flags;
        if(ent_flags != 0)
            judgment->Set_EntFlags=ent_flags;
        judgment->iPriority = 1;

        // Render verdict
        Razorback_Render_Verdict(judgment);
        Judgment_Destroy(judgment);
	}

	return 0;
}

void report_error(const char* file_name, int line_number, const char* error_message)
{
    rzb_log(LOG_ERR, "%s:%d: %s", file_name, line_number, error_message);
}


SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_pYaraUuid;
    uuid_t l_pList[1];
    UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_pList[0]);
    if ((loadMutex = Mutex_Create(MUTEX_MODE_NORMAL)) == NULL)
        return false;

    uuid_copy(l_pYaraUuid, YARA);

    if (!readMyConfig(NULL, YARA_CONFIG_FILE, yara_config)) {
        rzb_log(LOG_ERR, "%s: Yara Nugget: Failed to read config file", __func__);
        return false;
    }
    
	yr_init();
	
	
    sg_pContext = Razorback_Init_Inspection_Context (
            sg_uuidNuggetId, l_pYaraUuid, 1, l_pList,
            &sg_InspectionHooks, sg_initThreads, sg_maxThreads);

    return true;
}

DECL_NUGGET_THREAD_INIT(yara_thread_init)
{
    YARA_CONTEXT *context;
	if((context = yr_create_context()) == NULL) {
		rzb_log(LOG_ERR, "%s: Yara Nugget: Error creating yara context", __func__);
		return false;
	}
	
	context->error_report_function = report_error;
	Mutex_Lock(loadMutex);
    loadContext= context;
	if(ftw(rule_path, &load_rule_file, 20) != 0) {
		rzb_log(LOG_ERR, "%s: Yara Nugget: Error loading rules", __func__);
		yr_destroy_context(context);
        Mutex_Unlock(loadMutex);
		return false;
	}
    loadContext= NULL;
    Mutex_Unlock(loadMutex);

    *threadData = context;
    return true;
}

DECL_NUGGET_THREAD_CLEANUP(yara_thread_cleanup)
{
    yr_destroy_context((YARA_CONTEXT *)threadData);
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}


bool processDeferredList (struct DeferredList *p_pDefferedList) {
    return true;
}

