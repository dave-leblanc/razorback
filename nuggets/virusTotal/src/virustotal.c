#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <time.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <json/json.h>
#include <arpa/inet.h>

#include <razorback/config_file.h>
#include <razorback/log.h>
#include <razorback/api.h>
#include <razorback/hash.h>
#include <razorback/uuids.h>
#include <razorback/visibility.h>
#include <razorback/metadata.h>
#include <razorback/judgment.h>

#define VIRUSTOTAL_CONFIG_FILE         "virustotal.conf"
#define VIRUSTOTAL_URL                 "https://www.virustotal.com/api/get_file_report.json"
#define VIRUSTOTAL_DATA_FORMAT         "resource=%s&key=%s"
#define VIRUSTOTAL_DATA_LENGTH         4096
#define VIRUSTOTAL_MAX_RESULT_SIZE     8192
UUID_DEFINE(VT, 0x92, 0x6b, 0x5b, 0xea, 0xaf, 0xb2, 0x11, 0xdf, 0x80, 0x07, 0xfb, 0x6c, 0xd1, 0x23, 0x6c, 0x62);
char *api_key;
static uuid_t sg_uuidNuggetId;

static RZBConfKey_t virustotal_config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"API_KEY", RZB_CONF_KEY_TYPE_STRING, &api_key, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};


typedef struct page_result
{
    char data[VIRUSTOTAL_MAX_RESULT_SIZE+1];
    int size;
} page_result;

static struct RazorbackContext *sg_pContext = NULL;

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

size_t
result_handler (void *buffer, size_t size, size_t nmemb, void *userp)
{

    // Make sure we have something to work with
    if ((buffer != NULL) && (nmemb > 0))
    {
        strncat (((page_result *) userp)->data, buffer,
                 VIRUSTOTAL_MAX_RESULT_SIZE);
        ((page_result *) userp)->size += nmemb;
    }
    else
    {
        printf ("result_handler: received no data to work with\n");
    }

    return nmemb;
}

DECL_INSPECTION_FUNC(file_handler)
{
    CURL *curl = NULL;
    CURLcode res;
    char request_data[VIRUSTOTAL_DATA_LENGTH], *md5 =
        NULL;
    char *line;
    page_result result;
    struct json_object *doc = NULL, *tmp = NULL, *report = NULL;
    struct lh_entry *entry = NULL;
    struct Hash *l_pHash;
    uint8_t ret = JUDGMENT_REASON_DONE;
    int vtResult =0;
    struct Judgment *judgment = NULL;
    char *reportStr = NULL;
    char *oldReport = NULL;

    if (block->pId->pHash->iType == HASH_TYPE_MD5)
        l_pHash = block->pId->pHash;
    else
    {
        if ((l_pHash = Hash_Create_Type (HASH_TYPE_MD5)) == NULL)
            return JUDGMENT_REASON_ERROR;
    
        Hash_Update(l_pHash, block->data.pointer, block->pId->iLength);
        Hash_Finalize(l_pHash);

    }

    memset (&result, 0, sizeof (result));

    // Init curl
    if ((curl = curl_easy_init ()) == NULL)
    {
        rzb_log(LOG_ERR, "VirusTotal: Failed to initialize curl");
        ret= JUDGMENT_REASON_ERROR;
        goto cleanup;
    }


    // Create the request
    curl_easy_setopt (curl, CURLOPT_URL, VIRUSTOTAL_URL);

    // Make sure we have md5 to request
    if ((md5 = Hash_ToText(l_pHash)) == NULL)
        return JUDGMENT_REASON_ERROR;

    rzb_log(LOG_INFO, "VirusTotal: Searching for: %s", md5);

    // Create our data
    snprintf (request_data, VIRUSTOTAL_DATA_LENGTH,
              VIRUSTOTAL_DATA_FORMAT, md5, api_key);

    // Add the data to the post request
    curl_easy_setopt (curl, CURLOPT_POSTFIELDS, request_data);

    // Make our callback handle the results
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, result_handler);

    // Make sure we pass the metaData to the handler
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &result);

    // Send the request
    res = curl_easy_perform (curl);

    // Hopefully we got everything
    if ( (res != 0) || (result.data == NULL))
    {
        rzb_log(LOG_ERR, "VirusTotal: Curl failure");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }
    if  (strlen (result.data) < 4)
    {
        rzb_log(LOG_ERR, "VirusTotal: Result to short");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }
    // Nothing detected
    if (strncmp (result.data, "None", 4) == 0)
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: Nothing found");
        goto cleanup;
    }

    // Start by parsing the json
    doc = json_tokener_parse (result.data);

        // Make sure the md5 was found
    if (doc == NULL || (is_error(doc)))
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: Failed to get result");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }
    vtResult = json_object_get_int (json_object_object_get(doc, "result"));
    if (vtResult == -1)
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: Incorrect API key");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }
    if (vtResult == -2)
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: API request rate exceeded");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }
    if (vtResult == 0)
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: Object not found at virus total: %s", md5);
        goto cleanup;
    }

    report = json_object_object_get (doc, "report");

    // Make sure the report is in the proper format
    if (json_object_array_length (report) != 2)
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: Invalid report format");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }

    // Begin converting our report with the submission date
    if (asprintf (&reportStr, "First submitted: %s\n\nDetections:\n",
              json_object_get_string
              (json_object_array_get_idx (report, 0))) == -1)
    {
        rzb_log_remote(LOG_INFO, eventId, "VirusTotal: maloctile disfunction");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }

    // We'll need this list a few times
    tmp = json_object_array_get_idx (report, 1);

    // Get the AV vender list and results
    entry = json_object_get_object (tmp)->head;

    if ((judgment = Judgment_Create(eventId, block->pId)) == NULL)
    {
        rzb_log(LOG_ERR, "VirusTotal: Failed to create judgment");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }

    if (asprintf((char **)&judgment->sMessage, "VirusTotal reported block bad") == -1)
    {
        rzb_log(LOG_ERR, "VirusTotal: Failed to create judgment");
        ret = JUDGMENT_REASON_ERROR;
        goto cleanup;
    }
    judgment->iPriority =1;
    judgment->Set_SfFlags = SF_FLAG_BAD;

    // Hopefully we have something to work with
    while (entry != NULL)
    {
        if (strlen(json_object_get_string(json_object_object_get(tmp, (char *) entry->k))) == 0)
        {
            entry = entry->next;
            continue;
        }

        if (asprintf (&line, "%s:%s",
                  (char *) entry->k,
                  json_object_get_string
                  (json_object_object_get
                   (tmp, (char *) entry->k))) == -1)
        {
            rzb_log(LOG_ERR, "Maloctile disfunction");
            ret = JUDGMENT_REASON_DONE;
            goto cleanup;
        }

        Metadata_Add_MalwareName(judgment->pMetaDataList, (char *) entry->k, json_object_get_string(json_object_object_get(tmp, (char *) entry->k)));
        oldReport = reportStr;
       
        if (asprintf(&reportStr, "%s\t%s:%s\n", oldReport, (char *) entry->k, json_object_get_string(json_object_object_get(tmp, (char *) entry->k))) == -1)
        {
            rzb_log(LOG_ERR, "Maloctile disfunction");
            ret = JUDGMENT_REASON_DONE;
            free(oldReport);
            goto cleanup;
        }
        free(oldReport);

        free(line);
        line = NULL;
        entry = entry->next;
    }

    Metadata_Add_Report(judgment->pMetaDataList, reportStr);
    free(reportStr);
    Razorback_Render_Verdict(judgment);
cleanup:
    // Cleanup
    if (curl != NULL)
        curl_easy_cleanup (curl);

    if (md5 != NULL)
        free (md5);

    if (tmp != NULL)
        free (tmp);

    if (report != NULL)
        free (report);

    if (doc != NULL && !(is_error(doc)))
        free (doc);
    if (judgment != NULL)
        Judgment_Destroy(judgment);

    curl = NULL;
    doc = NULL;
    report = NULL;
    tmp = NULL;

    if (block->pId->pHash->iType != l_pHash->iType)
        Hash_Destroy(l_pHash);

    return ret;
}


SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_uuidList[1];
    uuid_t l_pTempUuid;    
    
    if (!UUID_Get_UUID(DATA_TYPE_ANY_DATA, UUID_TYPE_DATA_TYPE, l_uuidList[0]))
    {
        rzb_log(LOG_ERR, "File Log Nugget: Failed to get ANY_DATA UUID");
        return false;
    }
    uuid_copy(l_pTempUuid, VT);

    if (!readMyConfig (NULL, VIRUSTOTAL_CONFIG_FILE, virustotal_config))
    {
        return false;
    }

    sg_pContext= Razorback_Init_Inspection_Context (
        sg_uuidNuggetId, l_pTempUuid, 1, l_uuidList,
        &sg_InspectionHooks, 1, 1);

    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    if (sg_pContext != NULL) 
    {
        Razorback_Shutdown_Context(sg_pContext);
    }
}
