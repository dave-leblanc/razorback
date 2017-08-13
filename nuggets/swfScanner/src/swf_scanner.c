#include "config.h"
#include <razorback/visibility.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block.h>
#include <razorback/hash.h>
#include <razorback/config_file.h>
#include <razorback/judgment.h>
#include <razorback/metadata.h>


#include "swf_scanner.h"

#define SWF_SCANNER_CONFIG "swf_scanner.conf"
UUID_DEFINE(FLASH, 0x20, 0xf7, 0x53, 0x80, 0xaf, 0xb0, 0x11, 0xdf, 0x86, 0x6f, 0x7b, 0x86, 0xc6, 0x49, 0x59, 0x6b);
static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;

struct RazorbackContext *sg_pContext = NULL;

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(scanSWF);

static struct RazorbackInspectionHooks sg_InspectionHooks ={
    scanSWF,
    processDeferredList,
    NULL,
    NULL
};

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"Threads.Initial", RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL },
    {"Threads.Max", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL },
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

bool processDeferredList (struct DeferredList *p_pDefferedList)
{
    return true;
}



static void sendWarnings(struct EventId *eventId, struct BlockId *blockId, ErrorCode errCode);

#ifdef VERBOSE

static void printRECT(RECT *struct_rect, char *prefix)
{
    printf("%sRECT.NBits    [%u]\n", prefix, struct_rect->Nbits);
    printf("%sRECT.Xmin     [%u]\n", prefix, struct_rect->Xmin);
    printf("%sRECT.Xmax     [%u]\n", prefix, struct_rect->Xmax);
    printf("%sRECT.Ymin     [%u]\n", prefix, struct_rect->Ymin);
    printf("%sRECT.Ymax     [%u]\n", prefix, struct_rect->Ymax);
}

static void printHEADER(SWFFileInfo *swffile)
{
    printf("SWF.HEADER.Signature [%c%c%c]\n", swffile->header.Signature[0], swffile->header.Signature[1], swffile->header.Signature[2]);
    printf("SWF.HEADER.Version   [%d]\n", swffile->header.Version);
    printf("SWF.HEADER.FileLength [%d]\n", swffile->header.FileLength);
    printf("SWF.HEADER.FrameSize\n");
    printRECT(&(swffile->header.FrameSize), "-- ");
    printf("SWF.HEADER.FrameRate [%d]\n", swffile->header.FrameRate);
    printf("SWF.HEADER.FrameCount [%d]\n", swffile->header.FrameCount);
}

static void printTAGs(TAGList *tags, uint32_t level)
{
    TAG *cur_tag;
    uint16_t tag_type;
    uint32_t tag_index = 0;

    char *prefix;

    if (level == 0)
        prefix = "";
    else
        prefix = "--- ";

    cur_tag = tags->tag_head;

    while (cur_tag != NULL)
    {
        tag_type = cur_tag->TagCode;

        if (tag_type < MAX_TYPE_CODE)
            printf("%sTAG[%d].TagCode [%d][%s]\n", prefix, tag_index, tag_type, TypeName[tag_type]);
        else
            printf("%sTAG[%d].TagCode [%d]\n", prefix, tag_index, tag_type);

        printf("%sTAG[%d].Length [%d]\n", prefix, tag_index, cur_tag->Length);

        cur_tag = cur_tag->next;
        tag_index++;

        if (tag_type == TYPE_DefineSprite)
            printTAGs(&(cur_tag->tags), 1);
    }
}

#endif

/* Decompression function for 'CWS' Flash files. It decompress an entire file at once */

#define CHUNK 0x40000  // 256K

static ErrorCode decompressSWF(SWFFileInfo *swffile, uint8_t *cursor, uint32_t size)
{
    z_stream stream;
    int ret;
    int count_newbuff = 0;
    uint32_t file_length;

    stream.next_in   = cursor + 8;
    stream.avail_in  = size - 8;

    stream.zalloc    = Z_NULL;
    stream.zfree     = Z_NULL;
    stream.opaque    = Z_NULL;

    stream.avail_out = 0;
    inflateInit(&stream);

    swffile->decompressed_data = (uint8_t *)malloc(CHUNK);
    if (swffile->decompressed_data == NULL)
        return SWF_ERROR_MEMORY;

    count_newbuff++;
    memcpy(swffile->decompressed_data, &(swffile->header), 4);

    // The first 8 bytes of SWF file is not compressed
    stream.avail_out = CHUNK - 8;
    stream.next_out  = swffile->decompressed_data + 8;

    while (1)
    {
        if (!stream.avail_out)
        {
            count_newbuff++;
            swffile->decompressed_data = (Bytef *)realloc(swffile->decompressed_data, CHUNK * count_newbuff);
            if (swffile->decompressed_data == NULL)
                return SWF_ERROR_MEMORY;

            stream.next_out  = swffile->decompressed_data + ((count_newbuff - 1) * CHUNK);
            stream.avail_out = CHUNK;
        }

        if (stream.next_in)
            ret = inflate(&stream, Z_NO_FLUSH);
        else
            ret = inflate(&stream, Z_FINISH);

        if (ret != Z_OK && ret != Z_STREAM_END)
            break;

        if (ret == Z_STREAM_END)
        {
            ret = inflateEnd(&stream);

            file_length = stream.total_out + 8;   // 8 bytes = header.Signature, header.Version, header.FileLength

            // store the size of decompressed data to the file length field in the buffer
            *(swffile->decompressed_data + 4) = (uint8_t)file_length;
            *(swffile->decompressed_data + 5) = (uint8_t)(file_length >> 8);
            *(swffile->decompressed_data + 6) = (uint8_t)(file_length >> 16);
            *(swffile->decompressed_data + 7) = (uint8_t)(file_length >> 24);

            // use SWF scanner's own memory instead of the memory provided by the dispatcher
            swffile->bof = swffile->decompressed_data;
            swffile->eof = swffile->decompressed_data + file_length;

            break;
        }
    }

    if (ret != Z_OK)
    {
        free(swffile->decompressed_data);
        swffile->decompressed_data = NULL;
        swffile->compressed = 0;

        return SWF_ERROR_DECOMPRESSION;
    }

    return SWF_OK;
}

static ErrorCode createTAG(TAGList *tag_list)
{
    TAG *new_tag;

    new_tag = (TAG *)malloc(sizeof(TAG));

    if (new_tag == NULL)
        return SWF_ERROR_MEMORY;

    memset(new_tag, 0, sizeof(TAG));

    if (tag_list->tag_head == NULL)
    {
        tag_list->tag_head = new_tag;
        tag_list->tag_tail = new_tag;
    }
    else
    {
        ((TAG *)tag_list->tag_tail)->next = new_tag;
        new_tag->prev            = tag_list->tag_tail;

        tag_list->tag_tail = new_tag;
    }

    return SWF_OK;
}

static void freeTAGs(TAGList *tags)
{
    TAG *tag_head;
    TAG *tag_delete;

    tag_head = tags->tag_head;

    while (tag_head != NULL)
    {
        tag_delete = tag_head;
        tag_head   = tag_head->next;

        tag_delete->prev = NULL;
        tag_delete->next = NULL;

        if (tag_delete->TagCode == TYPE_DefineSprite)
            freeTAGs(&(tag_delete->tags));

        free(tag_delete);
    }

    tags->tag_head = NULL;
    tags->tag_tail = NULL;
}

// Make sure Nbits <= 31

static uint32_t readVarBits(uint8_t **cursor, uint8_t Nbits, uint8_t *bit_offset)
{
    uint32_t multibits_value;    // the most significant bit is the leftmost bit

    uint8_t  i;
    uint8_t *tmp_cursor = *cursor;
    uint8_t  tmp_offset = *bit_offset;

    multibits_value = 0;
    for (i = 0; i < Nbits; i++)
    {
        if (*tmp_cursor & (0x80 >> (tmp_offset % 8)))
            multibits_value |= 1;

        multibits_value <<= 1;
        tmp_offset++;

        if ((tmp_offset % 8) == 0)
            tmp_cursor++;
    }

    multibits_value >>= 1; // to cancel the last left shift

    *cursor = tmp_cursor;
    *bit_offset = tmp_offset;

    return multibits_value;
}

static ErrorCode parseRECT(SWFFileInfo *swffile, uint8_t **cursor)
{
    uint8_t Nbits;
    uint8_t bit_offset;
    uint16_t rect_size;

    uint8_t *tmp_cursor = *cursor;

    if (tmp_cursor + 1 > swffile->eof)
        return SWF_ERROR_NODATA;

    Nbits = (uint8_t)((*tmp_cursor & 0xF8) >> 3);  // read left most 5 bits
    rect_size = (uint16_t)ceil((double)(5 + (Nbits * 4))/8) ;   // total in bytes

    if (tmp_cursor + rect_size > swffile->eof)
        return SWF_ERROR_NODATA;

    swffile->header.FrameSize.Nbits = Nbits;

    // after Nbits field
    bit_offset = 5;
    swffile->header.FrameSize.Xmin = readVarBits(&tmp_cursor, Nbits, &bit_offset);
    swffile->header.FrameSize.Xmax = readVarBits(&tmp_cursor, Nbits, &bit_offset);
    swffile->header.FrameSize.Ymin = readVarBits(&tmp_cursor, Nbits, &bit_offset);
    swffile->header.FrameSize.Ymax = readVarBits(&tmp_cursor, Nbits, &bit_offset);

    *cursor += rect_size;

    return SWF_OK;
}

static ErrorCode parseHEADER(SWFFileInfo *swffile, uint8_t **cursor)
{
    uint8_t *tmp_cursor = *cursor;

    if (tmp_cursor + 12 > swffile->eof)
    {
        rzb_log(LOG_DEBUG, "Not enough data to parse SWF header");
        return SWF_ERROR_NODATA;
    }

    tmp_cursor += 4;
    swffile->header.FileLength = READ_LITTLE_32(tmp_cursor);
    tmp_cursor += 4;

#if 0
    // File length does not follow the specification all the time
    if (swffile->header.FileLength != (swffile->eof - swffile->bof))
    {
        DEBUG_SWF(printf("swffile->header.FileLength [0x%08x] actual [0x%08x]\n",
                         swffile->header.FileLength, (swffile->eof - swffile->bof));)
        return SWF_ERROR_INVALID_FILELENGTH;
    }
#endif

    parseRECT(swffile, &tmp_cursor);

    if (tmp_cursor + 4 > swffile->eof)
        return SWF_ERROR_NODATA;

    swffile->header.FrameRate  = READ_LITTLE_16(tmp_cursor);
    tmp_cursor += 2;
    swffile->header.FrameCount = READ_LITTLE_16(tmp_cursor);
    tmp_cursor += 2;

    *cursor = tmp_cursor;

    VERBOSE_SWF(printHEADER(swffile);)

    return SWF_OK;
}

static ErrorCode parseTAG(TAGList *tag_list, uint8_t **cursor, uint8_t *end_to_parse)
{
    uint8_t  *tmp_cursor = *cursor;
    uint16_t  type_and_length;
    uint16_t  tag_type;
    uint32_t  tag_length;
    ErrorCode errCode;

    if (tmp_cursor + 2 > end_to_parse)
        return SWF_ERROR_NODATA;

    type_and_length = READ_LITTLE_16(tmp_cursor);
    tmp_cursor += 2;

    tag_type   = (uint16_t)((type_and_length & 0xFFC0) >> 6);
    tag_length = type_and_length & 0x003F;

    if (tag_length == 0x3F)
    {
        if (tmp_cursor + 4 > end_to_parse)
            return SWF_ERROR_NODATA;

        tag_length = READ_LITTLE_32(tmp_cursor);
        tmp_cursor += 4;
    }

    if (tmp_cursor + tag_length > end_to_parse)
        return SWF_ERROR_NODATA;

    if (tmp_cursor + tag_length < tmp_cursor) // integer overflow
        return SWF_ERROR_INVALID_TAGLENGTH;

    // Once a tag structure is allocated, there must be tag_length data
    errCode = createTAG(tag_list);
    if (errCode != SWF_OK)
        return errCode;

    ((TAG *)tag_list->tag_tail)->TagCode   = tag_type;
    ((TAG *)tag_list->tag_tail)->Length    = tag_length;
    ((TAG *)tag_list->tag_tail)->tag_start = *cursor;
    ((TAG *)tag_list->tag_tail)->tag_data  = tmp_cursor;

    *cursor = tmp_cursor + tag_length;

    return SWF_OK;
}

static ErrorCode parseDefineSpriteTAG(TAG *definesprite_tag)
{
    ErrorCode errCode    = SWF_OK;
    uint8_t  *tag_cursor = definesprite_tag->tag_data;
    uint8_t  *end_of_tag = tag_cursor + definesprite_tag->Length;

    // In order to include tag array,
    // the length of DefineSprite tag should be bigger than or equal to 6
    if (definesprite_tag->Length < 6)
        return SWF_ERROR_INVALID_TAGLENGTH;

    tag_cursor += 4; // to skip over Sprite ID and FrameCount fields

    while (errCode == SWF_OK && tag_cursor < end_of_tag)
        errCode = parseTAG(&(definesprite_tag->tags), &tag_cursor, end_of_tag);

    return errCode;
}

static ErrorCode parseSWF(SWFFileInfo *swffile, uint8_t *cursor, uint32_t size)
{
    ErrorCode errCode;

    if ((*cursor == 'F' || *cursor == 'C')
        && *(cursor + 1) == 'W' && *(cursor + 2) == 'S')
    {
        swffile->header.Signature[0] = 'F';
        swffile->header.Signature[1] = 'W';
        swffile->header.Signature[2] = 'S';

        if (*cursor == 'C')
            swffile->compressed = 1;
    }
    else
        return SWF_ERROR_INVALID_SIG;

    swffile->header.Version = *(cursor + 3);

    if (swffile->compressed)
    {
        if (decompressSWF(swffile, cursor, size) == SWF_OK)
            cursor = swffile->decompressed_data;
        else
            return SWF_ERROR_DECOMPRESSION;
    }

    errCode = parseHEADER(swffile, &cursor);

    // parseHEADER returns either SWF_OK or SWF_ERROR_NODATA
    // We don't need to parse further in this case if SWF_ERROR_NODATA takes place in the header
    if (errCode != SWF_OK)
        return errCode;

    while (errCode == SWF_OK && cursor < swffile->eof)
    {
        errCode = parseTAG(&(swffile->tags), &cursor, swffile->eof);

        if (errCode == SWF_OK && ((TAG *)swffile->tags.tag_tail)->TagCode == TYPE_DefineSprite)
            errCode = parseDefineSpriteTAG(swffile->tags.tag_tail);
    }


    VERBOSE_SWF(printTAGs(&(swffile->tags), 0);)

    return errCode;
}

static ErrorCode scanActionRecords(TAG *action_tag, SWFDetectionID *detectID)
{
    uint8_t *action_cursor = action_tag->tag_data;
    uint8_t *end_of_tag    = action_tag->tag_data + action_tag->Length;

    uint8_t  action_code;
    uint16_t action_length;

    uint8_t *detect_cursor;

    *detectID = SWFDetectionID_NONE;

    while (action_cursor < end_of_tag)
    {
        action_code = *action_cursor;
        action_cursor++;

        if (action_code < 0x80)
            continue;
        else
        {
            if (action_cursor + 2 > end_of_tag)
                return SWF_ERROR_NODATA;

            action_length = READ_LITTLE_16(action_cursor);
            action_cursor += 2;

            if (action_code == ACTIONCODE_ActionDefineFunction || action_code == ACTIONCODE_ActionDefineFunction2)
            {
                detect_cursor = action_cursor;

                while (*detect_cursor != '\0') detect_cursor++;

                if (detect_cursor >= action_cursor + action_length)
                {
                    *detectID = SWFDetectionID_CVE_2005_2628;
                    return SWF_OK;
                }
            }

            action_cursor += action_length;
        }
    }

    return SWF_OK;
}

static ErrorCode scanTAGs(TAGList *tags, SWFDetectionID *detectID, uint16_t frame_count)
{
    TAG      *cur_tag;
    uint8_t  *detect_cursor;
    uint8_t  *tmp_cursor;
    ErrorCode sub_error;

    uint32_t  endtag_found    = 0;
    uint32_t  count_showframe = 0;  // for CVE-2007-5400

    uint16_t  definesprite_framecount;

    *detectID = SWFDetectionID_NONE;
    cur_tag   = tags->tag_head;

    while (cur_tag != NULL)
    {
        /* CVE-2007-0071 */
        if (cur_tag->TagCode == TYPE_DefineSceneAndFrameLabelData)
        {
            detect_cursor = cur_tag->tag_data;

            if (cur_tag->Length >= 5)
            {
                if (detect_cursor[0] & 0x80 && detect_cursor[1] & 0x80
                    && detect_cursor[2] & 0x80 && detect_cursor[3] & 0x80
                    && detect_cursor[4] & 0x08)
                {

                    rzb_log(LOG_DEBUG, "SceneCount[0] = 0x%02x, [1] = 0x%02x, [2] = 0x%02x,  [3] = 0x%02x,  [4] = 0x%02x\n",
                                     detect_cursor[0], detect_cursor[1], detect_cursor[2], detect_cursor[3], detect_cursor[4]);

                    *detectID = SWFDetectionID_CVE_2007_0071;
                    return SWF_OK;
                }
            }
            else
                return SWF_ERROR_NODATA;
        }

        /* CVE-2007-5400 */
        /* some compreessed SWF file ends up with extra data after End tag, we need to ignore them */
        if (cur_tag->TagCode == TYPE_ShowFrame && !endtag_found)
            count_showframe++;

        if (cur_tag->TagCode == TYPE_End)
            endtag_found = 1;

        /* CVE-2005-2628 */
        if (cur_tag->TagCode == TYPE_DoAction)
        {
            sub_error = scanActionRecords(cur_tag, detectID);

            if (sub_error != SWF_OK || *detectID != SWFDetectionID_NONE)
                return sub_error;
        }

        if (cur_tag->TagCode == TYPE_DefineSprite && cur_tag->tags.tag_head != NULL)
        {
            if (cur_tag->Length >= 6)   // 6 is minium size of DefineSprite
            {
                tmp_cursor = cur_tag->tag_data + 2;
                definesprite_framecount = READ_LITTLE_16(tmp_cursor);

                sub_error = scanTAGs(&(cur_tag->tags), detectID, definesprite_framecount);

                if (sub_error != SWF_OK || *detectID != SWFDetectionID_NONE)
                    return sub_error;
            }
        }

        cur_tag = cur_tag->next;
    }

    /* CVE-2007-5400 */
    /* CVE-2006-0323 */
    /* Remove these checks because of false positives, functionality pushed to reworked flash parser.
    if (frame_count != count_showframe)
    {
        rzb_log(LOG_ERR,"Header.FrameCount [0x%08x], actual count [0x%08x]\n", frame_count, count_showframe);
        *detectID = SWFDetectionID_CVE_2007_5400;
        return SWF_OK;
    }
    */

    return SWF_OK;
}

/* any anomalies found during parsing, report them */
static void sendWarnings(struct EventId *eventId, struct BlockId *blockId, ErrorCode errCode)
{
    const char *warning_msg;
    struct Judgment *judgment;
    if ((judgment = Judgment_Create(eventId, blockId)) == NULL)
    {
        rzb_log(LOG_ERR, "SWF Scanner: Failed to allocate judgment");
        return;
    }   
    switch (errCode)
    {
        case SWF_ERROR_INVALID_SIG:
            warning_msg = "Warning: SWF signature is not valid";
            break;
        case SWF_ERROR_NODATA:
        case SWF_ERROR_INVALID_TAGLENGTH:
            warning_msg = "Warning: The inspected file may be truncated";
            break;
        case SWF_ERROR_MEMORY:
            warning_msg = "Warning: There is not memory related error";
            break;
        case SWF_ERROR_DECOMPRESSION:
            warning_msg = "Warning: Decompression failed";
            break;
        default:
            warning_msg = "Warning: unhandled warning";
    }
    judgment->Set_SfFlags = SF_FLAG_DODGY;
    judgment->iPriority = 2;
    if (asprintf((char **)&judgment->sMessage, "%s", warning_msg) == -1)
    {
        rzb_log(LOG_ERR, "SWF Scanner: Failed to allocate message");
        Judgment_Destroy(judgment);
        return;
    }
    Razorback_Render_Verdict(judgment);
}

static void sendDetectionResult(struct EventId *eventId, struct BlockId *blockId, SWFDetectionID detectID)
{
    struct Judgment *judgment;
    if (detectID >= SWFDetectionID_END)
    {
        rzb_log(LOG_ERR, "SWF Scanner: Invalid detection id");
        return;
    }
    if ((judgment = Judgment_Create(eventId, blockId)) == NULL)
    {
        rzb_log(LOG_ERR, "SWF Scanner: Failed to allocate judgment");
        return;
    }
    for (int i =0; i < DetectionResults[detectID]->cveCount; i++)
    {
        Metadata_Add_CVE(judgment->pMetaDataList, DetectionResults[detectID]->cve[i]);
    }
    for (int i =0; i < DetectionResults[detectID]->bidCount; i++)
    {
        Metadata_Add_BID(judgment->pMetaDataList, DetectionResults[detectID]->bid[i]);
    }

    judgment->iPriority = 1;
    judgment->Set_SfFlags = SF_FLAG_BAD;
    if (asprintf((char **)&judgment->sMessage, "%s", DetectionResults[detectID]->description) == -1)
    {
        rzb_log(LOG_ERR, "SWF Scanner: Failed to allocate message");
        Judgment_Destroy(judgment);
        return;
    }
    Razorback_Render_Verdict(judgment);
}

DECL_INSPECTION_FUNC(scanSWF)
{
    SWFFileInfo    swffile;
    ErrorCode      errCode;
    SWFDetectionID detectID = SWFDetectionID_NONE;
    unsigned char *cursor = block->data.pointer;
    uint64_t       size   = block->pId->iLength;

    if (cursor == NULL || size == 0 || size > 0x7FFFFFFF)
    {
        rzb_log(LOG_ERR, "SWF Scanner: Invalid data");
        return JUDGMENT_REASON_ERROR;
    }

    memset(&swffile, 0, sizeof(SWFFileInfo));

    swffile.bof = cursor;
    if (cursor + size <= cursor)
    {
        rzb_log(LOG_ERR, "SWF Scanner: file size[0x%08x] is too big", size);
        return JUDGMENT_REASON_ERROR;
    }

    swffile.eof = cursor + size;

    errCode = parseSWF(&swffile, cursor, size);

    if (errCode != SWF_OK)
        sendWarnings(eventId, block->pId, errCode);

    errCode = scanTAGs(&(swffile.tags), &detectID, swffile.header.FrameCount);

    if (errCode != SWF_OK)
        sendWarnings(eventId, block->pId, errCode);

    /* if some legitimate tags are scanned */
    if ( (swffile.tags.tag_head != NULL) && 
            (detectID != SWFDetectionID_NONE))
    {
        sendDetectionResult(eventId, block->pId, detectID);
    }

    /* epilog */
    freeTAGs(&(swffile.tags));

    if (swffile.compressed)
        free(swffile.decompressed_data);

    return JUDGMENT_REASON_DONE;
}

SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t list[1];

    if (!readMyConfig(NULL, SWF_SCANNER_CONFIG, sg_Config))
        return false;
    
    if (!UUID_Get_UUID(DATA_TYPE_FLASH_FILE, UUID_TYPE_DATA_TYPE, list[0]))
    {
        rzb_log(LOG_ERR, "SWF Scanner: Failed to get SWF_FILE UUID");
        return false;
    }

    uuid_t l_pTempUuid;
    uuid_copy(l_pTempUuid, FLASH);

    if ((sg_pContext = Razorback_Init_Inspection_Context (
        sg_uuidNuggetId, l_pTempUuid, 1, list,
        &sg_InspectionHooks, sg_initThreads, sg_maxThreads)) == NULL)
    {
        return false;
    }
    return true;
}

SO_PUBLIC DECL_NUGGET_SHUTDOWN
{
    Razorback_Shutdown_Context(sg_pContext);
}

