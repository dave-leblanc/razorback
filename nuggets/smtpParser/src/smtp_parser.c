/* Takes a pointer and a length and splits the data into pieces. */
#include "config.h"

#include <stdio.h>
#include <pcre.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <uuid/uuid.h>

#include <rzb_visibility.h>
#include <rzb_detection_api.h>

#define MAX_BOUNDARY_LEN  70  /* Max length of boundary string, defined in RFC 2046 */

#define SAFEMEM_ERROR 0
#define SAFEMEM_SUCCESS 1
#define ERRORRET return SAFEMEM_ERROR;

#include <limits.h>
#include <string.h>

// CHAR_BIT is 8
#define ALPHABET_SIZE (1 << CHAR_BIT)

// Return values for ProcessMessage()
#define SMTP_NOSUBCOMPONENTS 1
#define SMTP_DONE 0
#define SMTP_ENDOFPARTS -1
#define SMTP_OUTOFDATA -2
#define SMTP_NOBOUNDARY -3
#define SMTP_NOHEADER -4


static const DetectionAPI *detection;

//#define SMTP_DEBUG
#ifdef SMTP_DEBUG
#   define DEBUG_RZB(code) code
#else
#   define DEBUG_RZB(code)
#endif
unsigned char *data_start = NULL;

typedef struct _SMTPSearchInfo
{
    int id;
    int index;
    int length;
} SMTPSearchInfo;

typedef struct _SMTPMimeBoundary
{
    char   boundary[2 + MAX_BOUNDARY_LEN + 1];  /* '--' + MIME boundary string + '\0' */
    int    boundary_len;
} SMTPMimeBoundary;

typedef struct _SMTPPcre
{
    pcre       *re;
    pcre_extra *pe;
} SMTPPcre;

SMTPPcre mime_boundary_pcre;
SMTPPcre mime_base64_pcre;

#define PP_COLS 16
static void prettyprint(const unsigned char *data, unsigned size)
{
    unsigned i;
    const unsigned char *dataptr = data;
    char asciigraph[PP_COLS + 1];

    memset(asciigraph, '\x00', sizeof(asciigraph));

    //printf("Datasize: %d\n", size);

#ifdef PACKETDUMPSIZE
    size = (size > PACKETDUMPSIZE) ? PACKETDUMPSIZE : size;
#endif

    for (i=0; i < size; i++, dataptr++)
    {
        printf("%02x ", *dataptr);
        asciigraph[i % PP_COLS] = (char)((isgraph(*dataptr) || (*dataptr == ' ')) ? *dataptr : '.');

        if (i % PP_COLS == (PP_COLS - 1))
        {
            printf("%s\n", asciigraph);
            memset(asciigraph, '\x00', sizeof(asciigraph));
        }
    }

    // Dump any remaining data
    if (i % PP_COLS)
    {
        printf("%*s", (int)(PP_COLS - (i%PP_COLS)) * 3, " ");
        printf("%s\n", asciigraph);
    }
}

#ifdef SMTP_DEBUG
static void PrintSMTPProcessMessageReturnCode(int code)
{
    switch (code)
    {
        case SMTP_DONE:
            printf("SMTP_DONE\n");
            break;
        case SMTP_ENDOFPARTS:
            printf("SMTP_ENDOFPARTS\n");
            break;
        case SMTP_OUTOFDATA:
            printf("SMTP_OUTOFDATA\n");
            break;
        case SMTP_NOBOUNDARY:
            printf("SMTP_NOBOUNDARY\n");
            break;
        case SMTP_NOHEADER:
            printf("SMTP_NOHEADER\n");
            break;
        default:
            printf("Unknown ProcessMessage return (%d)\n", code);
            break;
    }
}

static void PrintPartStats(unsigned char *bufferstart, unsigned char *part_start, unsigned char *part_end)
{
    printf("global offsets -- start:%lu end:%lu ", (unsigned long)(part_start - data_start),
           (unsigned long)(part_end - data_start));
    printf("part_start=%p(%lu) part_end=%p(%lu) length=%lu\n", part_start, (unsigned long)(part_start-bufferstart),
           part_end, (unsigned long)(part_end - bufferstart), (unsigned long)(part_end - part_start));
}
#endif

static void compute_prefix(const char *str, size_t size, int *result)
{
    size_t q;
    int k;
    result[0] = 0;

    k = 0;
    for (q = 1; q < size; q++)
    {
        while (k > 0 && str[k] != str[q])
            k = result[k-1];

        if (str[k] == str[q])
            k++;
        result[q] = k;
    }
}

static void prepare_badcharacter_heuristic(const char *str, size_t size, int *result)
{
    size_t i;

    for (i = 0; i < ALPHABET_SIZE; i++)
        result[i] = -1;

    for (i = 0; i < size; i++)
        result[(size_t) str[i]] = i;
}

static void prepare_goodsuffix_heuristic(const char *normal, size_t size, int *result)
{
    const char *left = normal;
    const char *right = left + size;
    char *reversed;
    char *tmp;
    size_t i;

    int j, k; // originally "const int" within loop below

    int *prefix_normal;
    int *prefix_reversed;


    /* reverse string */
    reversed = malloc(size+1);
    tmp = reversed + size;

    *tmp = 0;
    while (left < right)
        *(--tmp) = *(left++);

    prefix_normal = malloc(size * sizeof(int));
    prefix_reversed = malloc(size * sizeof(int));

    compute_prefix(normal, size, prefix_normal);
    compute_prefix(reversed, size, prefix_reversed);

    for (i = 0; i <= size; i++)
    {
        result[i] = size - prefix_normal[size-1];
    }

    for (i = 0; i < size; i++)
    {
        j = size - prefix_reversed[i];
        k = i - prefix_reversed[i]+1;

        if (result[j] > k)
            result[j] = k;
    }
    free(reversed);
    free(prefix_normal);
    free(prefix_reversed);
}
/*
 * Boyer-Moore search algorithm
 */
static unsigned char *boyermoore_search(unsigned char *haystack, size_t haystack_len, SMTPMimeBoundary *mime_boundary_info)
{
    /*
     * Calc string sizes
     */
    char *needle = mime_boundary_info->boundary;
    size_t needle_len = mime_boundary_info->boundary_len;
    int badcharacter[ALPHABET_SIZE];
    int *goodsuffix;

    size_t s;
    size_t j;
    int k;
    int m;

    DEBUG_RZB(printf("boyermoore_search(%p, %Zu, %s, %Zu)\n", haystack, (long int)haystack_len, needle, (long int)needle_len));

    if (haystack_len < needle_len)
    {
        DEBUG_RZB(printf("not a big enough haystack to support that needle, son.\n"));
        return NULL;
    }

    /*
     * Simple checks
     */
    if (haystack_len == 0)
        return NULL;
    if (needle_len == 0)
        return haystack;

    /*
     * Initialize heuristics
     */
    goodsuffix = malloc((needle_len+1) * sizeof(int));

    prepare_badcharacter_heuristic(needle, needle_len, badcharacter);
    prepare_goodsuffix_heuristic(needle, needle_len, goodsuffix);

    /*
     * Boyer-Moore search
     */
    s = 0;
    while (s <= (haystack_len - needle_len))
    {
        j = needle_len;
        while (j > 0 && needle[j-1] == haystack[s+j-1])
            j--;

        if (j > 0)
        {
            k = badcharacter[(size_t) haystack[s+j-1]];

            if (k < (int)j && (m = j-k-1) > goodsuffix[j])
                s+= m;
            else
                s+= goodsuffix[j];
        }
        else
        {
            return haystack + s;
        }
    }

    free(goodsuffix);

    return NULL; // not found
}

static int inBounds(const uint8_t *start, const uint8_t *end, const uint8_t *p)
{
    if ((p >= start) && (p < end))
        return 1;
    return 0;
}

static int SafeMemCheck(void *dst, size_t n, const void *start, const void *end)
{
    void *tmp;

    if (n < 1)
        return SAFEMEM_ERROR;

    if ((dst == NULL) || (start == NULL) || (end == NULL))
        return SAFEMEM_ERROR;

    tmp = ((uint8_t *)dst) + (n - 1);
    if (tmp < dst)
        return SAFEMEM_ERROR;

    if (!inBounds(start, end, dst) || !inBounds(start, end, tmp))
        return SAFEMEM_ERROR;

    return SAFEMEM_SUCCESS;
}

/**
 * A Safer Memcpy
 *
 * @param dst where to copy to
 * @param src where to copy from
 * @param n number of bytes to copy
 * @param start start of the dest buffer
 * @param end end of the dst buffer
 *
 * @return SAFEMEM_ERROR on failure, SAFEMEM_SUCCESS on success
 */
static int SafeMemcpy(void *dst, const void *src, size_t n, const void *start, const void *end)
{
    if (SafeMemCheck(dst, n, start, end) != SAFEMEM_SUCCESS)
        ERRORRET;
    if (src == NULL)
        ERRORRET;
    memcpy(dst, src, n);
    return SAFEMEM_SUCCESS;
}

///*
// * Initialize run-time boundary search
// */
static int SMTP_BoundarySearchInit(void)
{
    const char *error;
    int erroffset;

    /* create regex for finding boundary string - since it can be cut across multiple
     * lines, a straight search won't do. Shouldn't be too slow since it will most
     * likely only be acting on a small portion of data */
    mime_boundary_pcre.re = pcre_compile("^Content-Type\\s*:\\s*multipart[^\\n]*boundary\\s*=\\s*\"?([^\\s\"]+)\"?", //"^Content-Type\\s*:\\s*multipart[^\\n]*boundary\\s*=\\s*\"?([^\\s\"]+)\"?",
                                         PCRE_CASELESS | PCRE_DOTALL | PCRE_MULTILINE,
                                         &error, &erroffset, NULL);
    if (mime_boundary_pcre.re == NULL)
    {
        printf("Failed to compile pcre regex for getting boundary "
               "in a multipart SMTP message: %s\n", error);
        return(-1);
    }

    mime_boundary_pcre.pe = pcre_study(mime_boundary_pcre.re, 0, &error);

    if (error != NULL)
    {
        printf("Failed to study pcre regex for getting boundary "
               "in a multipart SMTP message: %s\n", error);
        return(-1);
    }

    return 1;
}

///*
// * Initialize run-time boundary search
// */
static int SMTP_Base64SearchInit(void)
{
    const char *error;
    int erroffset;

    /* create regex for finding boundary string - since it can be cut across multiple
     * lines, a straight search won't do. Shouldn't be too slow since it will most
     * likely only be acting on a small portion of data */
    mime_base64_pcre.re = pcre_compile("^Content-Transfer-Encoding\\s*:\\s*base64\\s*$",
                                       PCRE_CASELESS | PCRE_DOTALL | PCRE_MULTILINE,
                                       &error, &erroffset, NULL);
    if (mime_base64_pcre.re == NULL)
    {
        printf("Failed to compile pcre regex for finding base64 encoding "
               "in a multipart SMTP message: %s\n", error);
        return(-1);
    }

    mime_base64_pcre.pe = pcre_study(mime_base64_pcre.re, 0, &error);

    if (error != NULL)
    {
        printf("Failed to study pcre regex for finding base64 encoding "
               "in a multipart SMTP message: %s\n", error);
        return(-1);
    }

    return 1;
}


static int SMTP_GetBoundary(const uint8_t *data, int data_len, SMTPMimeBoundary *mime_boundary_info)
{
    int result;
    int ovector[9];
    int ovecsize = 9;
    const char *boundary;
    int boundary_len;
    int ret;
    char *mime_boundary;
    int  *mime_boundary_len;

    mime_boundary = &(mime_boundary_info->boundary[0]);
    mime_boundary_len = &(mime_boundary_info->boundary_len);

    /* result will be the number of matches (including submatches) */
    result = pcre_exec(mime_boundary_pcre.re, mime_boundary_pcre.pe,
                       (const char *)data, data_len, 0, 0, ovector, ovecsize);
    if (result < 0)
    {
        DEBUG_RZB(printf("pcre not found %d\n", result));
        return -1;
    }

    result = pcre_get_substring((const char *)data, ovector, result, 1/*2*/, &boundary);

    if (result < 0)
        return -1;

    //printf("string=%s\n", &(data[ovector[1]]));
    DEBUG_RZB(printf("boundary = %s\n", boundary));

    boundary_len = strlen(boundary);
    if (boundary_len > MAX_BOUNDARY_LEN)
    {
        /* XXX should we alert? breaking the law of RFC */
        boundary_len = MAX_BOUNDARY_LEN;
    }

    mime_boundary[0] = '-';
    mime_boundary[1] = '-';
    ret = SafeMemcpy(mime_boundary + 2, boundary, boundary_len,
                     mime_boundary + 2, mime_boundary + 2 + MAX_BOUNDARY_LEN);

    pcre_free_substring(boundary);

    if (ret != SAFEMEM_SUCCESS)
    {
        return -1;
    }

    *mime_boundary_len = 2 + boundary_len;
    mime_boundary[*mime_boundary_len] = '\0';

    return(ovector[1]); // offset of byte AFTER the boundary string
}

static int ContentIsBase64(const uint8_t *data, int data_len)
{
    int result;
    int ovector[9];
    int ovecsize = 9;

    result = pcre_exec(mime_base64_pcre.re, mime_base64_pcre.pe,
                       (const char *)data, data_len, 0, 0, ovector, ovecsize);
    return(result);

}

static int SMTP_FindMessageData(uint8_t *buffer, int length, uint8_t **beginning)
{
    int i;

    const char begofdata[] = "DATA";

    length -= sizeof(begofdata) - 1;  // Optimization so later we don't have to worry about if we have room for search text

    for (i=0; i < length; i++)
    {

        // See if this is the start of the data block
        if (strncmp(begofdata, (const char *)&(buffer[i]), sizeof(begofdata) - 1) == 0)
        {
            i += strlen(begofdata);

            // Jump over "\r?\n"
            //    This is pretty fast and loose.  Do we/how do we handle if there is "DATA" at the
            //    beginning of the line but it's not actually followed with "\r?\n"?
            if (i + 1 < length && buffer[i] == '\r') i++;
            if (i + 1 < length && buffer[i] == '\n') i++;

            // Note it's up to the calling function to determine we haven't just stepped
            // off the end of the incoming data!
            *beginning = &(buffer[i]);
            return 1;
        }

        // No match.  Look for the end of line so we can try again
        while ((i < length) && (buffer[i] != '\n'))
        {
            i++; // Skip!
        }
    }

    return 0;
}


static int SMTP_FindEndOfHeader(uint8_t *buffer, int length, uint8_t **end)
{
    int i = 0;

    if (length < 2)
        return 0;

    if (buffer[i] == '\n' && buffer[i+1] == '\n')
    {
        *end = &(buffer[i+1]);
        return 1;
    }

    for (i=2; i < length; i++)
    {
        if (buffer[i] == '\n' && ((buffer[i-1] == '\n') || ((buffer[i-1] == '\r') && (buffer[i-2] == '\n'))))
        {
            *end = &(buffer[i+1]);
            return 1;
        }
    }

    return 0;
}

static int SendToDispatcher(uint8_t *data, int len, unsigned eventID)
{
    BLOCK_META_DATA *mdata = NULL;
    uint8_t *data_copy;

    // Init the metadata structure
    if((mdata = calloc(1, sizeof(BLOCK_META_DATA))) == NULL) {
       perror("Error allocating mdata\n");
       return R_MALLOC_FAIL;
    }

    // Fill in the required fields
    mdata->timestamp = (unsigned)time(NULL);
    if (data && len)
    {
        if ((data_copy = malloc(len)) == NULL)
        {
            perror("Error allocating data copy\n");
            free(mdata);
            return R_MALLOC_FAIL;
        }
        memcpy(data_copy, data, len);
    }
    else
        data_copy = NULL;
    mdata->data = data_copy;
    mdata->size = len;
    mdata->src_ip.family = AF_INET;
    mdata->src_ip.ip.ipv4.s_addr = htonl(0x01010101);
    mdata->dst_ip.family = AF_INET;
    mdata->dst_ip.ip.ipv4.s_addr = htonl(0x02020202);
    mdata->ip_proto = 6;
    mdata->src_port = 25;
    mdata->dst_port = 8000;
    uuid_copy(mdata->datatype, detection->file_type_lookup(data_copy, len));

    printf("\n\n\nVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n");
    printf("*********************** Sample SendToDispatcher() ************************\n");
    prettyprint(data_copy, len);
    printf("*********************** Sample SendToDispatcher() ************************\n");
    printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n\n\n");

    // Finally, send our data (sendData will free mdata)
    detection->sendData(mdata);

    return(1);
}


/* base64decode assumes the input data terminates with '=' and/or at the end of the input buffer
 * at inbuf_size.  If extra characters exist within inbuf before inbuf_size is reached, it will
 * happily decode what it can and skip over what it can't.  This is consistent with other decoders
 * out there.  So, either terminate the string, set inbuf_size correctly, or at least be sure the
 * data is valid up until the point you care about.  Note base64 data does NOT have to end with
 * '=' and won't if the number of bytes of input data is evenly divisible by 3.
 *
 * Return is 0 on success, negative on input error ('=' not in third or fourth position, implying
 * we're not aligned properly, etc), positive on output error (outbuf too small, output truncated
 * because unable to extract a full four byte base64 block at the end of input buffer, etc)
 *
 * On return, *bytes_written contains the number of valid bytes in the output buffer.
*/

static int base64_decode(unsigned char *inbuf, int inbuf_size, unsigned char **outbuf, int *bytes_written)
{

/* Our lookup table for decoding base64 */
static const unsigned int decode64tab[256] = {
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,62 ,100,100,100, 63,
         52, 53, 54, 55, 56, 57, 58, 59, 60, 61,100,100,100, 99,100,100,
        100,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
         15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,100,100,100,100,100,
        100, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
         41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,
        100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};

    const unsigned char *cursor, *endofinbuf;
    unsigned char *outbuf_ptr, *end_of_outbuf;
    unsigned char base64data[4], *base64data_ptr; /* temporary holder for current base64 chunk */
    unsigned char tableval_a, tableval_b, tableval_c, tableval_d;

    uint32_t n;

    int outbuf_size = 0;

    int error = 0;


    // Allocate space for the data
    outbuf_size = inbuf_size * 4 / 3;

    if (outbuf_size % 4)
        outbuf_size += 4 - (outbuf_size % 4);

    DEBUG_RZB(printf("inbuf_size = %d, outbuf_size = %d\n", inbuf_size, outbuf_size));

    *outbuf = malloc(inbuf_size * 4 / 3);

    if (!*outbuf)
    {   // out of memory
        DEBUG_RZB(printf("Out of memory, can't base64_decode!\n"));
        return -1;
    }

    base64data_ptr = base64data;
    endofinbuf = inbuf + inbuf_size;
    end_of_outbuf = *outbuf + outbuf_size;

    /* Strip non-base64 chars from inbuf and decode */
    n = 0;
    *bytes_written = 0;
    cursor = inbuf;
    outbuf_ptr = *outbuf;

    while (cursor < endofinbuf)
    {
        if (decode64tab[*cursor] != 100)
        {
            *base64data_ptr++ = *cursor;
            n++;  /* Number of base64 bytes we've stored */
            if (!(n % 4))
            {
                /* We have four databytes upon which to operate */

                if ((base64data[0] == '=') || (base64data[1] == '='))
                {
                    /* Error in input data */
                    error = -1;
                    break;
                }

                /* retrieve values from lookup table */
                tableval_a = decode64tab[base64data[0]];
                tableval_b = decode64tab[base64data[1]];
                tableval_c = decode64tab[base64data[2]];
                tableval_d = decode64tab[base64data[3]];

                if (outbuf_ptr < end_of_outbuf)
                {
                    *outbuf_ptr++ = (unsigned char)((tableval_a << 2) | (tableval_b >> 4));
                }
                else
                {
                    error = 1;
                    break;
                }

                if (outbuf_ptr >= end_of_outbuf)
                {
                    error = 1;
                    break;
                }

                if (base64data[2] != '=')
                {
                    *outbuf_ptr++ = (unsigned char)((tableval_b << 4) | (tableval_c >> 2));
                }
                else
                { // we have an '=' in third byte
                    if (base64data[3] != '=')
                    { // make sure there's also '=' in fourth
                        error = -1;
                        break;
                    }
                    else
                    {  // valid terminator for base64 data
                        break;
                    }
                }

                if (outbuf_ptr >= end_of_outbuf)
                {
                    error = 1;
                    break;
                }

                if ((base64data[3] != '=') && (outbuf_ptr < end_of_outbuf))
                {
                    *outbuf_ptr++ = (unsigned char)((tableval_c << 6) | tableval_d);
                }
                else
                {
                    // valid end of base64 data
                    break;
                }

                /* Reset our decode pointer for the next group of four */
                base64data_ptr = base64data;
            }
        }
        cursor++;
    }

    // If we had an incomplete block of input, tell the caller we have truncated
    // data (unless we already have a more serious error)
    if (!error && (n % 4))
    {
        error = 2;
    }

    if (outbuf_ptr < end_of_outbuf)
    {
        *outbuf_ptr = '\0';
        *bytes_written = outbuf_ptr - *outbuf;
    }
    else
    {
        (*outbuf)[outbuf_size - 1] = '\0';
        *bytes_written = outbuf_size - 1;
    }

    return error;
}


static int ProcessMessage(uint8_t *data, int data_len, unsigned eventID)
{
#ifdef SMTP_DEBUG
    int retval = SMTP_NOBOUNDARY;
#endif

    unsigned char *end_of_header;
    unsigned char *part_start, *part_end;

    unsigned char *base64_decoded_data = NULL;
    int base64_decoded_size = 0;

    SMTPMimeBoundary mime_boundary_info;

    int base64_encoded = 0;

    DEBUG_RZB(printf("ProcessMessage(%p, %d) start\n", data, data_len));

    if (data_len < 2)
        return SMTP_OUTOFDATA;

    if (memcmp(data, "--", 2) == 0)
    {
        DEBUG_RZB(printf("end of parts\n"));
        return SMTP_ENDOFPARTS;
    }

    // Find end of header
    if (SMTP_FindEndOfHeader(data, data_len, &end_of_header) <= 0)
    {
        DEBUG_RZB(printf("Did not find end of email header.  No message text found.\n"));
        SendToDispatcher(data, data_len, eventID);
        return SMTP_NOHEADER;
    }

#ifdef SMTP_DEBUG
    printf("Header data -\n");
    prettyprint(data, end_of_header - data);
#endif

    // Are we base64-encoded?
    if (ContentIsBase64(data, end_of_header - data /*data_len*/) > 0)
    {
        DEBUG_RZB(printf("Content is base64-encoded\n"));
        base64_encoded = 1;
    }

    // Get the SMTP boundary string
    if (SMTP_GetBoundary(data, end_of_header - data/*data_len*/, &mime_boundary_info) <= 0)
    {
        DEBUG_RZB(printf("SMTP_GetBoundary() <= 0\n"));

        if (!base64_encoded)
        {
            SendToDispatcher(/*data*/ end_of_header, data + data_len - end_of_header, eventID);
//         build_and_send_data(end_of_header, data + data_len - end_of_header, eventID);
        }
        else
        {
            DEBUG_RZB(printf("need to base64 decode data\n"));
#ifdef SMTP_DEBUG
            retval =
#endif
                base64_decode(end_of_header, data + data_len - end_of_header, &base64_decoded_data, &base64_decoded_size);
            DEBUG_RZB(printf("base64_decode() = %d. base64_decoded_size = %d\n", retval, base64_decoded_size));
            SendToDispatcher(base64_decoded_data, base64_decoded_size, eventID);
//         build_and_send_data(base64_decoded_data, base64_decoded_size, eventID);
            free(base64_decoded_data);
        }

        DEBUG_RZB(printf("returning SMTP_DONE\n"));
        return SMTP_NOBOUNDARY; // Means no MIME components
    }

    // I'm not sure how we would get here if we determine we are base64-encoded, but...
    base64_encoded = 0;

    // Okay, now we know we probably have subcomponents.

    // Initialize at the first boundary
    DEBUG_RZB(printf("finding part_start\n"));
    part_start = boyermoore_search(end_of_header, data + data_len - end_of_header, &mime_boundary_info);

    if (!part_start)
    {
        DEBUG_RZB(printf("No parts!\n"));
        SendToDispatcher(data, data_len, eventID);
        return(SMTP_NOSUBCOMPONENTS);
    }

    part_start += mime_boundary_info.boundary_len;
//      part_end = boyermoore_search(part_start, data + data_len - part_start, mime_boundary_info->boundary, mime_boundary_info->boundary_len);

    // Now loop for subcomponents
    DEBUG_RZB(printf("finding part_end 1\n"));
    while ((part_end = boyermoore_search(part_start, data + data_len - part_start, &mime_boundary_info)) != NULL)
    {
        DEBUG_RZB(printf("top of loop\n"));

        // Make sure we're still in the message/MIME part
        if (part_start > data + data_len)
        {
            DEBUG_RZB(printf("part_start > data + data_len\n"));
            break;
        }

        if (part_start < data)
        { // This can't happen (How's that for a code review grep, eh?)
            DEBUG_RZB(printf("part_start is before the buffer starts.  wtf!\n"));
            break;
        }

        // Process this thing for subcomponents
        DEBUG_RZB(PrintPartStats(data, part_start, part_end));
        if ((
#ifdef SMTP_DEBUG
             retval =
#endif
             ProcessMessage(part_start, part_end - part_start, eventID)) == SMTP_NOSUBCOMPONENTS)
        {
            DEBUG_RZB(printf("SMTP_NOSUBCOMPONENTS\n"));
            SendToDispatcher(data, data_len, eventID);
        }
        else
        {
            DEBUG_RZB(printf("More subcomponents, retval = %d", retval));
            DEBUG_RZB(PrintSMTPProcessMessageReturnCode(retval));
        }

        // Skip to the beginning of the next part.  There is no start boundary for additional parts
        part_start = part_end;// + mime_boundary_info->boundary_len; // Add is at top of loop plus bounds check
        part_start += mime_boundary_info.boundary_len;
        DEBUG_RZB(printf("looping.  Moved part_start, finding part_end\n"));
    }

    DEBUG_RZB(printf("returning from ProcessMessage()\n"));
    DEBUG_RZB(PrintSMTPProcessMessageReturnCode(retval));
    return SMTP_DONE;
}


static void RZB_SMTP_Detection_Nugget(BLOCK_META_DATA *metaData)
{
    unsigned char *beg_of_message;
#ifdef SMTP_DEBUG
    int retval;
#endif

    // We only handle SMTP NUGTYPE, so not bothering to check type as the Dispatcher should
    // only send data to us when it's handled.

    if (SMTP_FindMessageData(metaData->data, metaData->size, &beg_of_message) <= 0)
    {
        DEBUG_RZB(printf("Could not find DATA SMTP command.  No message found!\n"));
        DEBUG_RZB(printf("Continuing, assuming it's just the message itself\n"));
        //return(-1);
        beg_of_message = metaData->data;
    }

#ifdef SMTP_DEBUG
    retval =
#endif
        ProcessMessage(beg_of_message, metaData->data + metaData->size - beg_of_message, metaData->eventid);
    DEBUG_RZB(PrintSMTPProcessMessageReturnCode(retval));
}


UUID_DEFINE(SMTP_PARSER_NUGGET, 0x7a, 0xb0, 0x66, 0x9b, 0x37, 0x82, 0x58, 0x88, 0xb4, 0x51, 0x34, 0x79, 0xbf, 0xb6, 0xcd, 0xd4);

SO_PUBLIC HRESULT initNug(const DetectionAPI *detectionObj)
{
    uuid_t list;

    uuid_copy(list, MAIL_CAPTURE);

    if (SMTP_BoundarySearchInit() <= 0)
    {
        printf("Failed to init pcre search structure for boundary\n");
        return(R_FAIL);
    }

    if (SMTP_Base64SearchInit() <= 0)
    {
        printf("Failed to init pcre search structure for base64\n");
        return(R_FAIL);
    }

    detection = detectionObj;

    detection->registerHandler(&RZB_SMTP_Detection_Nugget, (const uuid_t *)&list, 1, SMTP_PARSER_NUGGET);

    return R_SUCCESS;
}

