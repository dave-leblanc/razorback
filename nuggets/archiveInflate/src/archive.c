#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <libgen.h>

#include <archive.h>
#include <archive_entry.h>

#include <uuid/uuid.h>

#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/metadata.h>
#include <razorback/api.h>
#include <razorback/block_pool.h>
#include <razorback/block_id.h>
#include <razorback/event.h>
#include <razorback/submission.h>
#include <razorback/visibility.h>
//UUID_DEFINE(ARCHIVE_INFLATE_NUGGET, 0x81, 0x62, 0xeb, 0x12, 0x66, 0xd0, 0x11, 0xe0, 0xb3, 0xb2, 0xe7, 0x62, 0xe3, 0xf9, 0x85, 0x8a);
UUID_DEFINE(ARCHIVE_INFLATE, 0x84, 0x99, 0x42, 0xcd, 0xee, 0x07, 0x4f, 0xfd, 0xa8, 0xd9, 0xf1, 0xdc, 0xf1, 0xf7, 0x68, 0xc1);

#include <razorback/config_file.h>

#define ARCHIVE_INFLATE_CONFIG "archive_inflate.conf"

static uuid_t sg_uuidNuggetId;
static conf_int_t sg_initThreads = 0;
static conf_int_t sg_maxThreads = 0;
static conf_int_t sg_iCompBz2;
static conf_int_t sg_iCompCompression;
static conf_int_t sg_iCompGzip;
static conf_int_t sg_iCompLzma;
static conf_int_t sg_iCompXz;
static conf_int_t sg_iFormatAr;
static conf_int_t sg_iFormatCpio;
static conf_int_t sg_iFormatIso9660;
static conf_int_t sg_iFormatTar;
static conf_int_t sg_iFormatZip;

static uuid_t sg_pUuidCompBz2;
static uuid_t sg_pUuidCompCompression;
static uuid_t sg_pUuidCompGzip;
static uuid_t sg_pUuidCompLzma;
static uuid_t sg_pUuidCompXz;
static uuid_t sg_pUuidFormatAr;
static uuid_t sg_pUuidFormatCpio;
static uuid_t sg_pUuidFormatIso9660;
static uuid_t sg_pUuidFormatTar;
static uuid_t sg_pUuidFormatZip;

static struct RazorbackContext *sg_pContext;

bool processDeferredList (struct DeferredList *p_pDefferedList);
DECL_INSPECTION_FUNC(file_handler);

static struct RazorbackInspectionHooks sg_InspectionHooks ={
    file_handler,
    processDeferredList,
    NULL,
    NULL
};

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"Threads.Initial",  RZB_CONF_KEY_TYPE_INT, &sg_initThreads, NULL},
    {"Threads.Max",  RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL},

    {"Compression.bz2",         RZB_CONF_KEY_TYPE_INT, &sg_iCompBz2, NULL},
    {"Compression.compression", RZB_CONF_KEY_TYPE_INT, &sg_iCompCompression, NULL},
    {"Compression.gzip",        RZB_CONF_KEY_TYPE_INT, &sg_iCompGzip, NULL},
    {"Compression.lzma",        RZB_CONF_KEY_TYPE_INT, &sg_iCompLzma, NULL},
    {"Compression.xz",          RZB_CONF_KEY_TYPE_INT, &sg_iCompXz, NULL},
    {"Format.ar",               RZB_CONF_KEY_TYPE_INT, &sg_iFormatAr, NULL},
    {"Format.cpio",             RZB_CONF_KEY_TYPE_INT, &sg_iFormatCpio, NULL},
    {"Format.iso9660",          RZB_CONF_KEY_TYPE_INT, &sg_iFormatIso9660, NULL},
    {"Format.tar",              RZB_CONF_KEY_TYPE_INT, &sg_iFormatTar, NULL},
    {"Format.zip",              RZB_CONF_KEY_TYPE_INT, &sg_iFormatZip, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

#define DATA_TYPE_COUNT 10

/* pupulates an array of uuid's that the nugget supports extracting */
static void register_file_types( uuid_t *file_type_uuids, uint32_t *file_type_count);

bool 
processDeferredList (struct DeferredList *p_pDefferedList) 
{
    return true;
}

SO_PUBLIC DECL_NUGGET_INIT
{
    uuid_t l_uuidList[DATA_TYPE_COUNT];
    uint32_t count =0;
    uuid_t l_pTempUuid;    

    if (!readMyConfig(NULL, ARCHIVE_INFLATE_CONFIG, sg_Config))
    {
        rzb_log(LOG_ERR, "Archive Inflate Nugget: Failed to read config file");
        return false;
    }
    register_file_types(l_uuidList, &count);

    uuid_copy(l_pTempUuid, ARCHIVE_INFLATE);
    if((sg_pContext = Razorback_Init_Inspection_Context (
            sg_uuidNuggetId, l_pTempUuid, count, l_uuidList,
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

/** populate the list of valid file types
 * @param file_type_uuids pointer to array of uuids
 * @param file_type_count number of uuids added to the array
 */
static void 
register_file_types( uuid_t *file_type_uuids, uint32_t *file_type_count) 
{
    uint32_t l_iCount = 0;

    /* BZIP2 Compression format */
    if (sg_iCompBz2 != 0)
    {
        UUID_Get_UUID(DATA_TYPE_BZ2_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidCompBz2);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidCompBz2);
    }
    /* GZIP Compression format */
    if (sg_iCompGzip != 0)
    {
        UUID_Get_UUID(DATA_TYPE_GZIP_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidCompGzip);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidCompGzip);
    }
    /* There is actually a compression format know as compression */
    if (sg_iCompCompression != 0)
    {
        UUID_Get_UUID(DATA_TYPE_COMPRESSION_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidCompCompression);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidCompCompression);
    }
    /* LZMA */
    if (sg_iCompLzma != 0)
    {
        UUID_Get_UUID(DATA_TYPE_LZMA_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidCompLzma);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidCompLzma);
    }
    /* XZ */
    if (sg_iCompXz != 0)
    {
        UUID_Get_UUID(DATA_TYPE_XZ_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidCompXz);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidCompXz);
    }
    /* Unix archive format, still used to packaged libraries */
    if (sg_iFormatAr != 0)
    {
        UUID_Get_UUID(DATA_TYPE_AR_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidFormatAr);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidFormatAr);
    }
    /* Wouldn't seem right to not have tar */
    if (sg_iFormatTar != 0)
    {
        UUID_Get_UUID(DATA_TYPE_TAR_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidFormatTar);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidFormatTar);
    }
    /* Zip archives */
    if (sg_iFormatZip != 0)
    {
        UUID_Get_UUID(DATA_TYPE_ZIP_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidFormatZip);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidFormatZip);
    }
    /* CPIO */
    if (sg_iFormatCpio != 0)
    {
        UUID_Get_UUID(DATA_TYPE_CPIO_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidFormatCpio);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidFormatCpio);
    }
    /* Compact Disc File System */
    if (sg_iFormatIso9660 != 0)
    {
        UUID_Get_UUID(DATA_TYPE_ISO9660_FILE, UUID_TYPE_DATA_TYPE, sg_pUuidFormatIso9660);
        uuid_copy(file_type_uuids[l_iCount++], sg_pUuidFormatIso9660);
    }

    *file_type_count = l_iCount;
}

/** registers all the supported archive_read_support_XXX() functions
 * for the archive we are going to extract.
 * @param p_uuidDataType the data type of the archive we are extracting
 * @param l_pArchive the archive object to be extracted
 */
static void 
register_read_supports( uuid_t p_uuidDataType, struct archive *l_pArchive )
{
    if ((sg_iCompBz2 != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidCompBz2) == 0))
    {
        archive_read_support_compression_bzip2(l_pArchive);
        archive_read_support_format_raw(l_pArchive);
    }

    else if ((sg_iCompGzip != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidCompGzip) == 0))
    {
        archive_read_support_compression_gzip(l_pArchive);
        archive_read_support_format_raw(l_pArchive);
    }

    else if ((sg_iCompCompression != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidCompCompression) == 0))
    {
        archive_read_support_compression_compress(l_pArchive);
        archive_read_support_format_raw(l_pArchive);
    }

    else if ((sg_iCompLzma != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidCompLzma) == 0))
    {
        archive_read_support_compression_lzma(l_pArchive);
        archive_read_support_format_raw(l_pArchive);
    }

    else if ((sg_iCompXz != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidCompXz) == 0))
    {
        archive_read_support_compression_xz(l_pArchive);
        archive_read_support_format_raw(l_pArchive);
    }

    else if ((sg_iFormatTar != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidFormatTar) == 0))
        archive_read_support_format_tar(l_pArchive);

    else if ((sg_iFormatAr != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidFormatAr) == 0))
        archive_read_support_format_ar(l_pArchive);

    else if ((sg_iFormatZip != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidFormatZip) == 0))
        archive_read_support_format_zip(l_pArchive);

    else if ((sg_iFormatCpio != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidFormatCpio) == 0))
        archive_read_support_format_cpio(l_pArchive);

    else if ((sg_iFormatIso9660 != 0) &&
            (uuid_compare(p_uuidDataType, sg_pUuidFormatIso9660) == 0))
        archive_read_support_format_iso9660(l_pArchive);

    else
        rzb_log(LOG_ERR, "%s: How did I get here!!!", __func__);
}

static int
copy_data(struct archive *p_pArchive, struct BlockPoolItem *p_pItem)
{
    uint8_t *l_pData = NULL;
    ssize_t l_iSize = 0;

    for (;;) {
        if ((l_pData = malloc(1024))== NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to malloc new space", __func__);
            return ARCHIVE_FATAL;
        }

        l_iSize = archive_read_data(p_pArchive, l_pData, 1024);
        if (l_iSize == 0)
        {
            free(l_pData);
            return ARCHIVE_OK;
        }
        if (l_iSize < 0)
            return ARCHIVE_FATAL;

        BlockPool_AddData(p_pItem, l_pData, l_iSize, BLOCK_POOL_DATA_FLAG_MALLOCD);

    }
}

DECL_INSPECTION_FUNC(file_handler)
{
    struct archive *l_pArchive;
    struct archive_entry *l_pEntry;
    int l_iRet;
    uint32_t l_iSfFlags, l_iEntFlags;
    char *tempFileName;
    char *fileName;
    uuid_t fileNameUUID;
    uint32_t fileNameLen;
    char *fileNameMade;

    struct BlockPoolItem *l_pItem;

    /* open the archive */
    l_pArchive = archive_read_new();
    register_read_supports(block->pId->uuidDataType, l_pArchive);

    if( archive_read_open_memory(l_pArchive, block->data.pointer, block->pId->iLength) )
    {
        rzb_log(LOG_WARNING, "%s", archive_error_string(l_pArchive));
        return JUDGMENT_REASON_ERROR;
    }

    while( true )
    {
        /* Read the entry from the archive */
        l_iRet = archive_read_next_header(l_pArchive, &l_pEntry);

        if( l_iRet == ARCHIVE_EOF )
        {
            break;
        }

        if(l_iRet != ARCHIVE_OK)
        {
            rzb_log(LOG_ERR, "Archive Inflate Error: %s", archive_error_string(l_pArchive));
            break;
        }
           
        if (asprintf(&tempFileName, "%s", archive_entry_pathname(l_pEntry)) == -1)
        {
            rzb_log(LOG_ERR, "Archive Inflate Error: failed to allocate tempFileName");
            break;
        }

        rzb_log(LOG_INFO, "Extracting Path:%s",tempFileName);

        l_pItem = BlockPool_CreateItem(sg_pContext);
        /* Extract the data */
        if (copy_data(l_pArchive, l_pItem) != ARCHIVE_OK)
        {
            BlockPool_DestroyItem(l_pItem);
            free(tempFileName);
            break;
        }
        l_pItem->pEvent->pBlock->pParentId = BlockId_Clone(block->pId);
        l_pItem->pEvent->pParentId = EventId_Clone(eventId);
        if ((fileName = basename(tempFileName)) == NULL)
            RZB_Log(LOG_ERR, "Failed to setup filename");
        else
        {
            if (strlen(fileName) > 0)
            {
            	if (strcmp(fileName, "data") ==0)
            	{
					if (!UUID_Get_UUID(NTLV_NAME_FILENAME, UUID_TYPE_NTLV_NAME, fileNameUUID))
					{
						rzb_log(LOG_ERR, "%s: Failed to lookup name uuid", __func__);
					}
					else
					{
						if (Metadata_Get_String(eventMetadata, fileNameUUID, &fileNameLen, (const char **)&fileName))
						{
							if (asprintf(&fileNameMade, "Uncompressed data stream from: %s", fileName) == -1)
							{
								rzb_log(LOG_ERR, "%s: Failed to allocate file name", __func__);
							}
							else
							{
								Metadata_Add_Filename(l_pItem->pEvent->pMetaDataList, fileNameMade);
								free(fileNameMade);
							}
						}
					}
            	}
            	else
					RZB_DataBlock_Metadata_Filename(l_pItem, fileName);
            }
        }

        BlockPool_FinalizeItem(l_pItem);
        l_pItem->submittedCallback = NULL;
        
        if (Submission_Submit(l_pItem, 0, &l_iSfFlags, &l_iEntFlags) != RZB_SUBMISSION_OK)
        {
            rzb_log(LOG_INFO, "Archive Inflate: Bad submission");
        }
        free(tempFileName);

    }

    archive_read_close(l_pArchive);
    archive_read_finish(l_pArchive);

    return JUDGMENT_REASON_DONE;
}
