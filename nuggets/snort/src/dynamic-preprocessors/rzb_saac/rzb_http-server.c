#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include "rzb_includes.h"
#include "spp_rzb-saac.h"
#include "rzb_debug.h"
#include "rzb_core.h"
#include "rzb_http.h"

#include "sf_snort_plugin_api.h"
#include "sfPolicyUserData.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#include <razorback/log.h>
#include <razorback/block_pool.h>
#include <razorback/submission.h>

static int IsStreamIgnored ();
static void IgnoreStream (SFSnortPacket * sp);

#define HTTP_SERVER_RETURN_ERROR    2
#define HTTP_SERVER_RETURN_IGNORE   1
#define HTTP_SERVER_RETURN_PROCESS  0

static int
HTTP_ReadData (const u_int8_t ** in_cursor, const u_int8_t * end_of_data,
              struct RZB_HTTP_Request * request, u_int16_t payload_size)
{
    const u_int8_t *cursor = *in_cursor;
    size_t l_iPos = 0;
    u_int32_t bytesavailable =0;

    u_int8_t *filedataptr;

    if (cursor > end_of_data)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: Cursor at end of data\n", __func__));
        return HTTP_DATA_ERROR;
    }

    /* 
     * If we dont have an item in the block pool to store the data in create one.
     */
    if ((request->pBlockPoolItem) == NULL)
    {
        request->pBlockPoolItem = BlockPool_CreateItem(g_pContext);

        if ((request->pBlockPoolItem) == NULL)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_RZB,
                "%s unable to allocate file contents buffer!\n", __func__));
            return HTTP_DATA_ERROR;
        }

        request->amountstored = 0;
    }

    bytesavailable = end_of_data - cursor;
    if (request->filesize > 0 &&
            ((request->amountstored+bytesavailable) > request->filesize))
    {
        bytesavailable = request->filesize - request->amountstored;
    }


    // ZDNOTE Need to verify there is enough space left in the buffer before copy

    if ((filedataptr = malloc(bytesavailable*sizeof(u_int8_t))) == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Failed to allocate new buffer\n"));
        return HTTP_DATA_ERROR;
    }

    while (l_iPos < bytesavailable)
    {
        filedataptr[l_iPos] = *cursor++;
        l_iPos++;
    }
    BlockPool_AddData (request->pBlockPoolItem, filedataptr, bytesavailable,
                       BLOCK_POOL_DATA_FLAG_MALLOCD);

    *in_cursor = cursor;
    request->amountstored += bytesavailable;

    if (request->amountstored < request->filesize)
        return HTTP_DATA_INCOMPLETE;
    
    if (request->amountstored == 0 && request->filesize == 0)
        return HTTP_DATA_INCOMPLETE;

    if (request->amountstored == request->filesize)
        return HTTP_DATA_COMPLETE;

    if ( (request->filesize == 0) && (request->amountstored > 0))
        return HTTP_DATA_INCOMPLETE;
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: Bad stored calculation\n", __func__));
        return HTTP_DATA_ERROR;
    }
}

int
ReadToNextResponse (const u_int8_t ** in_cursor, const u_int8_t * end_of_data,
              struct RZB_HTTP_Request * request, u_int16_t payload_size)
{
    const u_int8_t *cursor = *in_cursor;
    size_t l_iPos = 0;
    u_int32_t bytesavailable =0;

    if (cursor > end_of_data)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: Cursor at end of data\n", __func__));
        return (HTTP_STATE_UNKNOWN);
    }

    bytesavailable = end_of_data - cursor;

    // Make sure that we dont read past the end
    if (request->filesize > 0 &&
            ((request->amountstored+bytesavailable) > request->filesize))
    {
        bytesavailable = request->filesize - request->amountstored;
    }


    while (l_iPos < bytesavailable)
    {
        cursor++;
        l_iPos++;
    }
    *in_cursor = cursor;

    if ( (request->amountstored < request->filesize) ||
            (request->amountstored == 0 && request->filesize == 0))
        return HTTP_SKIP_TO_NEXT_RESPONSE;
    else if (request->amountstored == request->filesize)
        return HTTP_STATE_COLLECTING;
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: Bad read length calculation\n", __func__));
        return (HTTP_STATE_UNKNOWN);
    }
}


int
ProcessServerHeader (struct RZB_HTTP_Request * request)
{
    u_int8_t *cursor = request->pServerResponse;
    const u_int8_t *end_of_data = request->pServerResponse + request->iResponseSize -1;

    int ret = HTTP_SERVER_RETURN_PROCESS;

    struct RZB_HTTP_Header *header;


    // Check for HTTP/1.[01] header
    if ((strncasecmp ((const char *) request->pServerResponse, "http/1.", 7) != 0)
            || (request->pServerResponse[7] != '0' && request->pServerResponse[7] != '1'))
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: not a valid HTTP version\n", __func__));
        return HTTP_SERVER_RETURN_ERROR;

    }
#if 0
    if (memcmp (cursor, "200", 3) != 0)
    {
        ret = (SERVER_RETURN_NOT_OK);
        if (strstr((char *)cursor, "Connection: close\r\n") != NULL)
            return IGNORE_STREAM;
    }
#endif

    // Find the end of the response line 
    // and move the cursor to the begining of the next header
    cursor = (u_int8_t *)strstr((char *)cursor, "\r\n");
    cursor++;
    cursor++;

    if (cursor >= end_of_data)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB,
                  "%s: not enough data 2!\n", __func__));
        return HTTP_SERVER_RETURN_ERROR;
    }
    request->responseHeaders = HTTP_TokenizeHeaders(cursor);
    // Now, we're going to see if we can find a Content-Length header.
    // By definition, it has to be at the start of a line.  So, we're just going
    // To look for newlines and every time we find one, see if we're now looking
    // at Content-Length:
    //
    header = HTTP_FindHeader(request->responseHeaders, "Content-Length", false);
    

    if (header != NULL)
        request->filesize = strtoul (header->value, (char **) (&cursor), 10);
        

    return ret;
}


int
HTTP_ProcessFromServer (SFSnortPacket * sp)
{
    const u_int8_t *cursor = sp->payload;
    const u_int8_t *end_of_data;

    struct RZB_HTTP_Request * request;

    if (rzb_ssn->http_ssn->state == HTTP_STATE_UNKNOWN)
        return 0;

    if (rzb_ssn->http_ssn->state == HTTP_STATE_IGNORE)
        return 0;
   
    if (rzb_ssn->http_ssn->requestHead == NULL)
        request = NULL;
    else 
        request = rzb_ssn->http_ssn->requestTail;

    cursor = sp->payload;

    end_of_data = sp->payload + sp->payload_size;

    while (cursor < end_of_data)
    {
        switch (rzb_ssn->http_ssn->state)
        {
        case HTTP_STATE_COLLECTING:
            if (request != NULL && !(request->status & REQUEST_STATUS_COLLECTING))
                request = NULL;

            if (request == NULL)
            {
                request = HTTP_GetNewRequest(sp);
            }
            if (!( request->status & REQUEST_STATUS_HAVE_RESPONSE))
            {
                switch (HTTP_ReadHeaderData (&cursor, end_of_data, 
                            &request->pServerResponse, 
                            &request->iResponseSize,
                            sp->payload_size))
                {
                case HTTP_HEADER_COMPLETE:
                    request->status |= REQUEST_STATUS_HAVE_RESPONSE;
                    break;
                case HTTP_HEADER_INCOMPLETE:
                    break;
                case HTTP_HEADER_ERROR:
                    request->status &= ~REQUEST_STATUS_COLLECTING;
                    rzb_ssn->http_ssn->state = HTTP_STATE_IGNORE;
                    break;
                }
                break;
            }
            if ( (request->status & REQUEST_STATUS_HAVE_RESPONSE) &&
                    !(request->status & REQUEST_STATUS_DONE_RESPONSE))
            {
                switch (ProcessServerHeader (request))
                {
                case HTTP_SERVER_RETURN_PROCESS:
                    request->status |= REQUEST_STATUS_DONE_RESPONSE;
                    break;
                case HTTP_SERVER_RETURN_IGNORE:
                    request->status &= ~REQUEST_STATUS_COLLECTING;
                    rzb_ssn->http_ssn->state = HTTP_SKIP_TO_NEXT_RESPONSE;
                    break;
                case HTTP_SERVER_RETURN_ERROR:
                    request->status &= ~REQUEST_STATUS_COLLECTING;
                    rzb_ssn->http_ssn->state = HTTP_STATE_IGNORE;
                    break;
                }
                break;
            }
            if ( (request->status & REQUEST_STATUS_DONE_RESPONSE) &&
                    !(request->status & REQUEST_STATUS_HAVE_DATA))
            { 
                switch (HTTP_ReadData (&cursor, end_of_data, request,
                                  sp->payload_size))
                {
                case HTTP_DATA_COMPLETE:
                    request->status |= REQUEST_STATUS_HAVE_DATA;
                    break;
                case HTTP_DATA_INCOMPLETE:
                    break;
                case HTTP_DATA_ERROR:
                    request->status &= ~REQUEST_STATUS_COLLECTING;
                    rzb_ssn->http_ssn->state = HTTP_STATE_IGNORE;
                    break;
                }
            }
            if ( (request->status & REQUEST_STATUS_HAVE_DATA) &&
                    (request->status & REQUEST_STATUS_DONE_REQUEST))
            {

                DEBUG_WRAP(DebugMessage(DEBUG_RZB, "WE HAVE A COMPLETE FILE! rzb_ssn->http_ssn=%p\n", rzb_ssn->http_ssn));
//                HTTP_DumpRequest(request);
                // This means we got all of our data.  Call the detection function.
                if (request->amountstored !=0)
                    HTTP_CallDetectionFunction (request);

                request->status |= REQUEST_STATUS_SUBMITTED_DATA; 
                request->status &= ~REQUEST_STATUS_COLLECTING;

                request = NULL;

                break;
            }
            else if (request->status & REQUEST_STATUS_HAVE_DATA)
            {
                request = NULL;
                break;
            }
            // Should never reach here, this is just a safe guard.
            break;
        case HTTP_SKIP_TO_NEXT_RESPONSE:
            // Read data, skipping until we find a server response.
            // We can only do this if we know the content lenght.
            // If there was no content length then we can assume the 
            // next bit of data is the response header.
            if (request != NULL)
                rzb_ssn->http_ssn->state =
                    ReadToNextResponse (&cursor, end_of_data, request,
                                  sp->payload_size);
            else 
                rzb_ssn->http_ssn->state = HTTP_STATE_COLLECTING;
            
            break;
        case HTTP_STATE_IGNORE:
            request = NULL;
            return 0;
            break;

        default:
            DEBUG_WRAP(DebugMessage(DEBUG_RZB,
                      "%s: Unhandled ruledate state (%d). Bailing.\n",
                       __func__, rzb_ssn->http_ssn->state));
            IgnoreStream (sp);
            break;
        }
    }

    if (IsStreamIgnored ())
        return (-1);
    else
        return (0);
}


// Partially debug / hackery, partially something we'll probably want to keep
static void
IgnoreStream (SFSnortPacket * sp)
{

    // Set state to ignore and clear out the list
    rzb_ssn->http_ssn->state = HTTP_STATE_IGNORE;

    //FreeFileInfoList (ruledata);
///    _dpd.streamAPI->stop_inspection (sp->stream_session_ptr,
///                                     sp, SSN_DIR_BOTH, -1, 0);

}

static int
IsStreamIgnored ()
{
    if (rzb_ssn->http_ssn->state == HTTP_STATE_IGNORE )
        return (1);

    return (0);
}
