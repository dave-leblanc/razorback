#ifdef HAVE_CONFIG_H
#define _GNU_SOURCE
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "rzb_debug.h"
#include "rzb_core.h"
#include "rzb_http.h"

#include "sf_snort_plugin_api.h"
#include "sfPolicyUserData.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre.h>


#define HTTP_CLIENT_SUCESS 0
#define HTTP_CLIENT_ERROR 1

int 
ProcessClientHeader(struct RZB_HTTP_Request *request)
{
    const char *cursor = (char *) request->pClientRequest;
    const char *end_of_data = (char *)(request->pClientRequest + request->iRequestSize -1);
    const char *uri;
    struct RZB_HTTP_Header *header;
    // Find the begining of the URL;
    cursor = strstr(cursor, " ");
    if (cursor == NULL)
        return HTTP_CLIENT_ERROR;
    uri = ++cursor;

    cursor = strstr(cursor, " ");
    if (cursor == NULL)
        return HTTP_CLIENT_ERROR;

    request->url = calloc(cursor - uri +1, sizeof (char));
    if (request->url == NULL)
        return HTTP_CLIENT_ERROR;

    memcpy(request->url, uri, cursor-uri);
    // Find the end of the response line and null out the line endings,
    // and move the cursor to the begining of the next header
    cursor = strstr(cursor, "\r\n");
    if (cursor == NULL)
        return HTTP_CLIENT_ERROR;

    cursor++;
    cursor++;

    if (cursor >= end_of_data)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, ("ProcessClientHeader not enough data 2!\n")));
        return HTTP_CLIENT_ERROR;
    }
    request->requestHeaders = HTTP_TokenizeHeaders((u_int8_t *)cursor);

    header = HTTP_FindHeader(request->requestHeaders, "Host", false);
    if (header != NULL)
    {
        if (asprintf(&request->hostname, "%s", header->value) == -1)
            return HTTP_CLIENT_ERROR;
    }

    return HTTP_CLIENT_SUCESS;
}

static struct RZB_HTTP_Request *
HTTP_GetCurrentRequest(SFSnortPacket *sp)
{
    if (rzb_ssn->http_ssn->currentRequest == NULL)
       rzb_ssn->http_ssn->currentRequest = rzb_ssn->http_ssn->requestHead;

next_session:
    if (rzb_ssn->http_ssn->currentRequest == NULL)
        rzb_ssn->http_ssn->currentRequest = HTTP_GetNewRequest(sp);
    
    if (rzb_ssn->http_ssn->currentRequest->status & REQUEST_STATUS_HAVE_REQUEST)
    {
        rzb_ssn->http_ssn->currentRequest = rzb_ssn->http_ssn->currentRequest->next;
        goto next_session;
    }

    return rzb_ssn->http_ssn->currentRequest;
}

int
HTTP_ProcessFromClient (SFSnortPacket * sp)
{
    const u_int8_t *cursor = sp->payload;
    const u_int8_t *end_of_data;

    struct RZB_HTTP_Request *request;

    if (rzb_ssn->http_ssn->state == HTTP_STATE_UNKNOWN)
        return 0;

    if (rzb_ssn->http_ssn->state == HTTP_STATE_IGNORE)
        return 0;
 
    request = HTTP_GetCurrentRequest(sp);
    if (request == NULL)
        return -1;

    cursor = sp->payload;

    end_of_data = sp->payload + sp->payload_size;

    while (cursor < end_of_data)
    {
        if ( !(request->status & REQUEST_STATUS_COLLECTING) &&
                !(request->status & REQUEST_STATUS_HAVE_REQUEST))
        {
            // Skip this request in the stream
            
            break;
        }
        // Advance to the next request, we have 
        if (request->status & REQUEST_STATUS_HAVE_REQUEST)
        {
            request = HTTP_GetCurrentRequest(sp);
            continue;
        }

        if (!(request->status & REQUEST_STATUS_HAVE_REQUEST))
        {
            switch (HTTP_ReadHeaderData (&cursor, end_of_data, 
                        &request->pClientRequest, 
                        &request->iRequestSize,
                        sp->payload_size))
            {
            case HTTP_HEADER_COMPLETE:
                request->status |= REQUEST_STATUS_HAVE_REQUEST;
                break;
            case HTTP_HEADER_INCOMPLETE:
                break;
            case HTTP_HEADER_ERROR:
                request->status &= ~REQUEST_STATUS_COLLECTING;
                rzb_ssn->http_ssn->state = HTTP_STATE_IGNORE;
                break;
            }
        }
        if ( (request->status & REQUEST_STATUS_HAVE_REQUEST) &&
                !(request->status & REQUEST_STATUS_DONE_REQUEST))
        {
            switch (ProcessClientHeader (request))
            {
            case HTTP_CLIENT_SUCESS:
                request->status |= REQUEST_STATUS_DONE_REQUEST;
                break;
            case HTTP_CLIENT_ERROR:
                request->status &= ~REQUEST_STATUS_COLLECTING;
                rzb_ssn->http_ssn->state = HTTP_STATE_IGNORE;
                break;
            }
            
        }

        if ( (request->status & REQUEST_STATUS_DONE_REQUEST) &&
                (request->status & REQUEST_STATUS_HAVE_DATA))
        {
            DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Client: WE HAVE A COMPLETE FILE! rzb_ssn->http_ssn=%p\n", rzb_ssn->http_ssn));
            //HTTP_DumpRequest(request);

            HTTP_CallDetectionFunction(request);
            request->status |= REQUEST_STATUS_SUBMITTED_DATA; 
            request->status &= ~REQUEST_STATUS_COLLECTING;
            continue;
        }

    }
    return 0;
}
