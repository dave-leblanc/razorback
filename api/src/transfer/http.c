#include "config.h"
#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/log.h>
#include <razorback/hash.h>
#include <curl/curl.h>
#include <string.h>

#include "transfer/core.h"
#if 0


struct CurlData
{
    uint64_t bytesWritten;
    struct Block *block;
};

static char *
generateURL (struct Block *block)
{
    char *path;
    char *filename;

    if ((filename = Transfer_generateFilename (block)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file name", __func__);
        return false;
    }

    if (asprintf(&path, "%c/%c/%c/%c/%s", 
                filename[0], filename[1], filename[2], filename[3], filename) == -1)
    {
        free(filename);
        return NULL;
    }

    free(filename);
    return path;

}

size_t
WriteCurlData (void *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct CurlData *data = (struct CurlData *) userdata;

    if (data->bytesWritten == 0)
    {

        if (data->block->pData != NULL)
            free (data->block->pData);

        if ((data->block->pData = malloc (data->block->pId->iLength)) == NULL)
        {
            rzb_log (LOG_ERR, "%s: Malloctile Dysfunction.", __func__);
            return 0;
        }
    }

    memcpy (data->block->pData + data->bytesWritten, ptr, size * nmemb);

    data->bytesWritten += size * nmemb;

    return size * nmemb;
}

uint32_t
RetrieveOverCurl (struct Block *block, struct TransferTicket *ticket)
{
    char *filename, *url;
    CURL *CURLinstance = NULL;
    struct CurlData *data;

    if ((data = calloc (1, sizeof (struct CurlData))) == NULL)
        return 0;

    data->block = block;

    if ((CURLinstance = curl_easy_init ()) == NULL)
    {
        rzb_log (LOG_ERR, "%s: Failed to initialize curl", __func__);
        free (data);
        return false;
    }

    if (curl_easy_setopt (CURLinstance, CURLOPT_WRITEFUNCTION, &WriteCurlData)
        != 0)
    {
        rzb_log (LOG_ERR, "%s: Could not set curl write callback", __func__);
        free (data);
        curl_easy_cleanup (CURLinstance);
        return false;
    }

    if ((filename = generateURL (block)) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed to generate file name", __func__);
        free (data);
        curl_easy_cleanup (CURLinstance);
        return 0;
    }

    if (asprintf
        (&url, "http://%s:%d/%s", ticket->hostName, ticket->port,
         filename) == -1)
    {
        rzb_log (LOG_ERR, "%s: failed to generate url", __func__);
        free (filename);
        free (data);
        curl_easy_cleanup (CURLinstance);
        return 0;
    }

    if (curl_easy_setopt (CURLinstance, CURLOPT_WRITEDATA, (void *) data) !=
        0)
    {
        rzb_log (LOG_ERR, "%s: Could not set curl write callback arguments",
                 __func__);
        free (url);
        free (filename);
        free (data);
        curl_easy_cleanup (CURLinstance);
        return 0;
    }

    if (curl_easy_setopt (CURLinstance, CURLOPT_URL, url) != 0)
    {
        rzb_log (LOG_ERR, "%s: Could not set curl fetch url", __func__);
        free (url);
        free (filename);
        free (data);
        curl_easy_cleanup (CURLinstance);
        return 0;
    }

    if (curl_easy_perform (CURLinstance) != 0)
    {
        rzb_log (LOG_ERR, "%s: Could not fetch file with curl", __func__);
        free (url);
        free (filename);
        free (data);
        curl_easy_cleanup (CURLinstance);
        return 0;
    }

    free (url);
    free (filename);
    free (data);
    curl_easy_cleanup (CURLinstance);
    return 1;

}
#endif
#if 0
static uint32_t
StoreOverCurl (struct Block *block)
{

    curl_easy_setopt (CURLinstance, CURLOPT_POSTFIELDS,
                      (void *) block->pData);
    curl_easy_setopt (CURLinstance, CURLOPT_POSTFIELDSIZE,
                      (long) block->pId->iLength);

    curl_easy_perform (CURLinstance);


    return 1;
}
#endif
