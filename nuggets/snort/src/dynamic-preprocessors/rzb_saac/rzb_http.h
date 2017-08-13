#ifndef RZB_HTTP_H
#define RZB_HTTP_H
#include "rzb_includes.h"
#define SAAC_HTTP 6880

#define NRTSID                  0xa5a5a5a5
#define INVALIDSTREAMIDX        0xFFFFFFFF
#define OUTOFSTREAMINFOSTORAGE  0xFFFFFFFF

#define HTTP_HEADER_ERROR                   0
#define HTTP_HEADER_COMPLETE                1 
#define HTTP_HEADER_INCOMPLETE              2

#define HTTP_DATA_ERROR                     0
#define HTTP_DATA_COMPLETE                  1 
#define HTTP_DATA_INCOMPLETE                2



#define HTTP_STATE_UNKNOWN                  0
#define HTTP_STATE_COLLECTING               1
#define HTTP_SKIP_TO_NEXT_RESPONSE          2
#define HTTP_STATE_IGNORE                   3

#define REQUEST_STATUS_COLLECTING           1
#define REQUEST_STATUS_HAVE_REQUEST         (1 << 1)
#define REQUEST_STATUS_DONE_REQUEST         (1 << 2)
#define REQUEST_STATUS_HAVE_RESPONSE        (1 << 3)
#define REQUEST_STATUS_DONE_RESPONSE        (1 << 4)
#define REQUEST_STATUS_HAVE_DATA            (1 << 5)
#define REQUEST_STATUS_SUBMITTED_DATA       (1 << 6)
#define REQUEST_STATUS_SUBMITTED_REQUEST    (1 << 7)

struct RZB_HTTP_Header
{
    char *name;
    char *value;
    struct RZB_HTTP_Header *next;
};

struct RZB_HTTP_Request
{
    /// Metadata
    /// @{
    char *url;
    char *hostname;
    int family;
    sfip_t saddr;
    sfip_t daddr;
    u_int16_t sport;
    u_int16_t dport;
    /// @}
    
    /// Response
    /// @{
    unsigned int filesize;
    unsigned int amountstored;
    struct BlockPoolItem *pBlockPoolItem;
    u_int8_t *pServerResponse;
    size_t iResponseSize;
    struct RZB_HTTP_Header *responseHeaders;
    /// @}
    
    /// Request
    /// @{
    u_int8_t *pClientRequest;
    size_t iRequestSize;
    struct RZB_HTTP_Header *requestHeaders;
    /// @}

    u_int8_t status;

    struct RZB_HTTP_Request *prev;
    struct RZB_HTTP_Request *next;
};
   
struct RZB_HTTP_Stream
{
    int state;
    int session_flags;
    struct RZB_HTTP_Request *requestHead;
    struct RZB_HTTP_Request *requestTail;
    struct RZB_HTTP_Request *currentRequest; ///< For Request Processing
};

// rzb_http.c
bool HTTP_IsServer(uint16_t port);
int HTTP_Inspect(SFSnortPacket *p);
struct RZB_HTTP_Stream * HTTP_GetNewSession(SFSnortPacket *);
struct RZB_HTTP_Request * HTTP_GetNewRequest(SFSnortPacket *);
void HTTP_SessionFree(struct RZB_HTTP_Stream *);
int HTTP_CallDetectionFunction (struct RZB_HTTP_Request *);
int HTTP_ReadHeaderData (const u_int8_t ** in_cursor, const u_int8_t * end_of_data,
              u_int8_t ** buffer, size_t * buffer_size, u_int16_t payload_size);
void 
HTTP_DumpRequest(struct RZB_HTTP_Request *request);
void HTTP_DumpStream();
struct RZB_HTTP_Header *
HTTP_TokenizeHeaders(const u_int8_t *origBuffer);
struct RZB_HTTP_Header *
HTTP_FindHeader(struct RZB_HTTP_Header *head, const char* name, bool caseCmp);

// rzb_http-client.c
int HTTP_ProcessFromClient (SFSnortPacket *);

// rzb_http-server.c
int HTTP_ProcessFromServer (SFSnortPacket *);

#endif // RZB_HTTP_H
