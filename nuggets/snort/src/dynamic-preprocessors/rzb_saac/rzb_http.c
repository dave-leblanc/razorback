#define _GNU_SOURCE
#include "rzb_includes.h"
#include "rzb_http.h"
#include "rzb_core.h"
#include "rzb_debug.h"
#include "spp_rzb-saac.h"
#define RESPONSE_SIZE_LIMIT 100000

#include <razorback/api.h>
#include <razorback/metadata.h>
#include <razorback/log.h>
#include <razorback/block_pool.h>
#include <razorback/submission.h>
#include <stdio.h>

int
HTTP_CallDetectionFunction (struct RZB_HTTP_Request * request)
{
    uint32_t l_iSf_Flags, l_iEnt_Flags;

    if ((request->pBlockPoolItem) == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Failed to get block\n"));
        return 0;
    }

    // Add the metadata.
    if (request->url != NULL && strlen(request->url) > 0)
        RZB_DataBlock_Metadata_URI(request->pBlockPoolItem, request->url);
    if (request->hostname != NULL && strlen(request->hostname) > 0)
        RZB_DataBlock_Metadata_Hostname(request->pBlockPoolItem, request->hostname);
     
    if (request->pClientRequest != NULL && strlen((char *)request->pClientRequest) > 0)
        RZB_DataBlock_Metadata_HttpRequest(request->pBlockPoolItem, (char *)request->pClientRequest);
    if (request->pServerResponse != NULL && strlen((char *)request->pServerResponse)> 0)
        RZB_DataBlock_Metadata_HttpResponse(request->pBlockPoolItem, (char *)request->pServerResponse);

    RZB_DataBlock_Metadata_Port_Source(request->pBlockPoolItem, htons(request->sport));
    RZB_DataBlock_Metadata_Port_Destination(request->pBlockPoolItem, htons(request->dport));
    if (request->family == AF_INET) 
    {
        RZB_DataBlock_Metadata_IPv4_Source(request->pBlockPoolItem, request->saddr.ip.u6_addr8);
        RZB_DataBlock_Metadata_IPv4_Destination(request->pBlockPoolItem, request->daddr.ip.u6_addr8);
    }
    else
    {
        RZB_DataBlock_Metadata_IPv6_Source(request->pBlockPoolItem, request->saddr.ip.u6_addr8);
        RZB_DataBlock_Metadata_IPv6_Destination(request->pBlockPoolItem, request->daddr.ip.u6_addr8);
    }
    /* We should attache the metadata for the stream here
     */
    BlockPool_FinalizeItem (request->pBlockPoolItem);
    request->pBlockPoolItem->submittedCallback = NULL;

    if (Submission_Submit (request->pBlockPoolItem, 0, &l_iSf_Flags, &l_iEnt_Flags) == RZB_SUBMISSION_OK)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Detection submission was successful\n"));
    }
    else
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Detection submission failed\n"));
    }
    request->pBlockPoolItem = NULL;
    return 0;
}


bool
HTTP_IsServer(uint16_t port)
{
    if (saac_eval_config->httpPorts[port / 8] & (1 << (port % 8)))
        return true;

    return false;
}

int 
HTTP_Inspect(SFSnortPacket *p)
{
    if ((HTTP_IsServer(p->src_port) && (p->flags & FLAG_FROM_SERVER)) ||
            (HTTP_IsServer(p->dst_port) && (p->flags & FLAG_FROM_CLIENT)))
        return STREAM_INSP_HTTP;

    return 0;
}

struct RZB_HTTP_Stream *
HTTP_GetNewSession(SFSnortPacket *p)
{
    struct RZB_HTTP_Stream *ssn;
    ssn = calloc(1, sizeof (struct RZB_HTTP_Stream));
    if (ssn == NULL)
    {
        DynamicPreprocessorFatalMessage("Failed to allocate SaaC HTTP session data\n");
        return NULL; // XXX: Hack to to missing no return attribute
    }

    if (p->flags & SSNFLAG_MIDSTREAM)
    {
       ssn->state = HTTP_STATE_UNKNOWN;
    } 
    else
    {
        ssn->state = REQUEST_STATUS_COLLECTING;
    }
    return ssn;
}

static void 
HTTP_FreeHeaderChain(struct RZB_HTTP_Header *header)
{
    struct RZB_HTTP_Header *prevHeader;
    while (header != NULL)
    {
        if (header->name != NULL)
            free(header->name);
        if (header->value != NULL)
            free(header->value);
        prevHeader = header;
        header = header->next;
        free(prevHeader);
    }
}

void
HTTP_SessionFree(struct RZB_HTTP_Stream *ssn)
{
    struct RZB_HTTP_Request *request, *prevRequest;

    if (ssn == NULL)
        return;

    request = ssn->requestHead;
    prevRequest = NULL;
    while (request != NULL)
    {
        // If we are still collecting we might need to submit
        // as we did not get a content length header.
        if (request->status & REQUEST_STATUS_COLLECTING)
        {
            if ( ( request->amountstored !=0 ) && 
                    (request->filesize == 0))
            {
                DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: Found unsubmitted file, submitting\n", __func__));
                HTTP_CallDetectionFunction (request);
            }
            if ( ( request->amountstored !=0 ) && 
                    (request->filesize != 0))
            {
                DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: Found partial file\n", __func__));
            }
        }

        if (request->pServerResponse != NULL)
            free(request->pServerResponse);

        if (request->pClientRequest != NULL)
            free(request->pClientRequest);

        HTTP_FreeHeaderChain(request->requestHeaders);
        HTTP_FreeHeaderChain(request->responseHeaders);

        if (request->url != NULL)
            free(request->url);

        if (request->hostname != NULL)
            free(request->hostname);

        if (request->pBlockPoolItem != NULL)
            BlockPool_DestroyItem(request->pBlockPoolItem);

        prevRequest = request;
        request = request->next;
        free(prevRequest);
    }

    free(ssn);
}


struct RZB_HTTP_Request * 
HTTP_GetNewRequest(SFSnortPacket *sp)
{
    int pkt_dir;
    struct RZB_HTTP_Request *req;
    if ((req = calloc(1, sizeof (struct RZB_HTTP_Request))) == NULL)
        return NULL;

    pkt_dir = RZB_GetPacketDirection(sp, HTTP_IsServer);
    req->family = sp->family;

    if (sp->family == AF_INET)
    {
        if (pkt_dir == PKT_FROM_CLIENT)
        {
            req->saddr = sp->ip4h->ip_src;
            req->daddr = sp->ip4h->ip_dst;
        }
        else
        {
            req->daddr = sp->ip4h->ip_src;
            req->saddr = sp->ip4h->ip_dst;
        }
    }
    else if (sp->family == AF_INET6)
    {
        if (pkt_dir == PKT_FROM_CLIENT)
        {
            req->saddr = sp->ip6h->ip_src;
            req->daddr = sp->ip6h->ip_dst;
        }
        else
        {
            req->daddr = sp->ip6h->ip_src;
            req->saddr = sp->ip6h->ip_dst;
        }
    }
    if (pkt_dir == PKT_FROM_CLIENT)
    {
        req->sport = sp->src_port;
        req->dport = sp->dst_port;
    }
    else
    {
        req->dport = sp->src_port;
        req->sport = sp->dst_port;
    }
    req->status = REQUEST_STATUS_COLLECTING;
    if (rzb_ssn->http_ssn->requestHead == NULL)
    {
        rzb_ssn->http_ssn->requestHead = req;
        rzb_ssn->http_ssn->requestTail = req;
    }
    else
    {
        req->prev = rzb_ssn->http_ssn->requestTail;
        rzb_ssn->http_ssn->requestTail->next = req;
        rzb_ssn->http_ssn->requestTail = req;
    }
    return req;
}


int
HTTP_ReadHeaderData (const u_int8_t ** in_cursor, const u_int8_t * end_of_data,
              u_int8_t ** pBuffer, size_t *pBuffer_size, u_int16_t payload_size)
{
    const u_int8_t *cursor = *in_cursor;
    u_int32_t bytesavailable =0;
    int l_iFoundEnd = 0;
    size_t l_iPosition = 0, i =0;
    u_int8_t *buffer;
    size_t buffer_size;

    u_int8_t *dataptr;

    buffer = *pBuffer;
    buffer_size = *pBuffer_size;

    if (cursor > end_of_data)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB, "%s: cursor at end of data\n",__func__));
        return (HTTP_HEADER_INCOMPLETE);
    }
    
    /* If there is data already in the buffer then the trailer can be 
     * located in the following places (split between the 2 buffers):
     * Note: buffer_size does not include the NULL at the end of the buffer.
     * cData = pBuffer
     * nData = cursor
     * \r           \n          \r          \n
     * cData[-3]    cData[-2]   cData[-1]   nData[0]
     * cData[-2]    cData[-1]   nData[0]    nData[1]
     * cData[-1]    nData[0]    nData[1]    nData[2]
     *
     */
    if (buffer != NULL) 
    {
        if ((buffer_size >= 3) &&
                (buffer[buffer_size -3] == '\r' ) &&
                (buffer[buffer_size -2] == '\n' ) &&
                (buffer[buffer_size -1] == '\r' ) &&
                (cursor[0] == '\n' ))
        {
            bytesavailable =1;
            l_iFoundEnd = 1;
        }
        else if ((buffer_size >= 2) &&
                (buffer[buffer_size -2] == '\r' ) &&
                (buffer[buffer_size -1] == '\n' ) &&
                (cursor[0] == '\r' ) &&
                (cursor[1] == '\n' ))
        {
            bytesavailable =2;
            l_iFoundEnd = 1;
        } 
        else if ((buffer_size >= 1) &&
                (buffer[buffer_size -1] == '\r' ) &&
                (cursor[0] == '\n' ) &&
                (cursor[1] == '\r' ) &&
                (cursor[2] == '\n' ))
        {
            bytesavailable =3;
            l_iFoundEnd = 1;
        }
    } 

    if (l_iFoundEnd == 0)
    {

        l_iPosition = 0;
        while ((end_of_data - cursor) >= l_iPosition+3) 
        {
            if ((cursor[l_iPosition] == '\r') &&
                    (cursor[l_iPosition+1] == '\n') &&
                    (cursor[l_iPosition+2] == '\r') &&
                    (cursor[l_iPosition+3] == '\n'))
            {
                bytesavailable = l_iPosition+4;
                l_iFoundEnd = 1;
                break;
            }
            l_iPosition++;
        }
        
    }

    if (bytesavailable ==0 )
    {
        bytesavailable = end_of_data - cursor;
    }
    if (buffer_size+bytesavailable > RESPONSE_SIZE_LIMIT)
    {
        return HTTP_HEADER_ERROR;
    }

    /* Allocate some space for the new lump of data
     *  Iff pServerResponse is null then malloc will create a new memory 
     *  area in the way malloc would.
     */
    if (buffer == NULL )
    {
        buffer = malloc( bytesavailable * sizeof(u_int8_t)+1);
        *pBuffer = buffer;
    }
    else 
    {
        buffer = realloc( buffer, buffer_size+(bytesavailable * sizeof(u_int8_t))+1);
        *pBuffer = buffer;
    }

    if ((buffer) == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_RZB,
            "%s: unable to allocate response buffer!\n", __func__));
        return HTTP_HEADER_ERROR;
    }

    // Set our write pointer to the end of the last block
    if (buffer_size == 0)
        dataptr = buffer;
    else
        dataptr = buffer + buffer_size; 

    // Copy in the data.
    for (i =0; i < bytesavailable; i++)
    {
        *dataptr++ = *cursor++;
    }
    
    *in_cursor = cursor;
    buffer_size += bytesavailable;
    *pBuffer_size = buffer_size;

    buffer[buffer_size]='\0';

    if (l_iFoundEnd == 0) 
        return HTTP_HEADER_INCOMPLETE;
    else 
        return HTTP_HEADER_COMPLETE;

}

struct RZB_HTTP_Header *
HTTP_FindHeader(struct RZB_HTTP_Header *head, const char* name, bool caseCmp)
{
    int match =0;
    while (head != NULL)
    {
        if (caseCmp)
            match = strcmp(head->name, name);
        else
            match = strcasecmp(head->name, name);

        if (match == 0)
            return head;

        head = head->next;
    }
    return NULL;
}

struct RZB_HTTP_Header *
HTTP_TokenizeHeaders(const u_int8_t *origBuffer)
{
    struct RZB_HTTP_Header *ret = NULL;
    struct RZB_HTTP_Header *last = NULL;
    char *buffer, *bufferEnd;
    char  *cursor = NULL;
    char *sep = NULL;
    char *end = NULL;

    if (asprintf(&buffer, "%s", origBuffer) == -1)
        return NULL;

    bufferEnd = buffer +strlen(buffer);
    cursor = buffer;

    while (cursor != NULL)
    {
        end = strstr(cursor, "\r\n");
        if (end == NULL)
            break;

        *end = '\0';
        if (strlen(cursor) <= 4)
            goto header_tok_again;

        if ((last = calloc(1,sizeof(struct RZB_HTTP_Header))) == NULL)
            goto header_tok_error;

        
        sep = strstr(cursor, ": ");

        if (sep == NULL)
            goto header_tok_again;
        *sep = '\0'; sep++; 
        *sep = '\0'; sep++; 
        if (asprintf(&last->name, "%s", cursor) == -1)
            goto header_tok_error;

        if (asprintf(&last->value, "%s", sep) == -1)
            goto header_tok_error;

        if (last->name != NULL && last->value != NULL)
        {
            last->next = ret;
            ret = last;
        }
        else
        {
            if (last->name != NULL)
                free(last->name);
            if (last->value != NULL)
                free(last->value);
            free(last);
        }

header_tok_again:
        cursor = end +2;
        if (cursor > bufferEnd)
            cursor = NULL;
    }

    free(buffer);
    return ret;


header_tok_error:
    last = ret;
    while (last != NULL)
    {
        if (last->name != NULL)
            free(last->name);
        if (last->value != NULL)
            free(last->value);

        ret = last;
        last = last->next;
        free(ret);
    }
    free(buffer);
    return NULL;
}

void 
HTTP_DumpRequest(struct RZB_HTTP_Request *request)
{
    struct RZB_HTTP_Header *head;
    char saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];
    inet_ntop(request->family, request->saddr.ip.u6_addr8, saddr, INET6_ADDRSTRLEN);
    inet_ntop(request->family, request->daddr.ip.u6_addr8, daddr, INET6_ADDRSTRLEN);
    printf("====== HTTP Request Dump Start ======\n");
    printf("Source Address: %s\n", saddr);
    printf("Dest Address: %s\n", daddr);
    printf("Source Port: %u\n", request->sport);
    printf("Dest Port: %u\n", request->dport);
    printf("URL: %s\n", request->url);
    printf("Host: %s\n", request->hostname);
    printf("Status: %x\n", request->status);
    printf("Request Size: %zu\n", request->iRequestSize);
    printf("Response Size: %zu\n", request->iResponseSize);
    printf("Data Size (content-length): %u\n", request->filesize);
    printf("Data Size (stored): %u\n", request->amountstored);
    printf("== Request ==\n");
    printf("%s", request->pClientRequest);
    printf("== Request Headers ==\n");
    head = request->requestHeaders;
    while(head != NULL)
    {
        printf("Name: '%s', Value: '%s'\n", head->name, head->value);
        head = head->next;
    }
    printf("== Response ==\n");
    printf("%s", request->pServerResponse);
    printf("== Response Headers ==\n");
    head = request->responseHeaders;
    while(head != NULL)
    {
        printf("Name: '%s', Value: '%s'\n", head->name, head->value);
        head = head->next;
    }
    printf("====== HTTP Request Dump End ======\n");
}
void 
HTTP_DumpStream()
{
    struct RZB_HTTP_Request *cur;
    printf("====== HTTP Stream Dump Start ======\n");
    cur = rzb_ssn->http_ssn->requestHead;
    printf("State: %d\n", rzb_ssn->http_ssn->state);
    printf("Session Flags: %x\n", rzb_ssn->http_ssn->session_flags);
    while(cur != NULL)
    {
        HTTP_DumpRequest(cur);
        cur = cur->next;
    }
    printf("====== HTTP Stream Dump End ======\n");
}
