/** @file uuids.h
 * UUID List API.
 */
#ifndef RAZORBACK_UUIDS_H
#define RAZORBACK_UUIDS_H

#include <uuid/uuid.h>

#include <razorback/visibility.h>
#include <razorback/types.h>
#ifdef __cplusplus
extern "C" {
#endif

struct UUIDListNode
{
    uuid_t uuid;
    char *sName;
    char *sDescription;
    struct UUIDListNode *pNext;
};
/** UUID Types
 * @{
 */
#define UUID_TYPE_DATA_TYPE 1   ///< Data Type
#define UUID_TYPE_INTEL_TYPE 2  ///< Intel Type
#define UUID_TYPE_NTLV_TYPE 3   ///< NTLV Type
#define UUID_TYPE_NUGGET 4      ///< Nugget
#define UUID_TYPE_NUGGET_TYPE 5 ///< Nugget Type
#define UUID_TYPE_NTLV_NAME 6   ///< NTLV Name
/// @}
//


/** Well Known UUID's
 * @{
 */
#define NUGGET_TYPE_CORRELATION "CORRELATION"
#define NUGGET_TYPE_INTEL "INTEL"
#define NUGGET_TYPE_DEFENSE "DEFENSE"
#define NUGGET_TYPE_OUTPUT "OUTPUT"
#define NUGGET_TYPE_COLLECTION "COLLECTION"
#define NUGGET_TYPE_INSPECTION "INSPECTION"
#define NUGGET_TYPE_MASTER "MASTER"
#define NUGGET_TYPE_DISPATCHER "DISPATCHER"

#define DATA_TYPE_ANY_DATA "ANY_DATA"
#define DATA_TYPE_FLASH_FILE "FLASH_FILE"
#define DATA_TYPE_JAVASCRIPT "JAVASCRIPT"
#define DATA_TYPE_OLE_FILE "OLE_FILE"
#define DATA_TYPE_PAR2_FILE "PAR2_FILE"
#define DATA_TYPE_PAR_FILE "PAR_FILE"
#define DATA_TYPE_PDF_FILE "PDF_FILE"
#define DATA_TYPE_PE_FILE "PE_FILE"
#define DATA_TYPE_RAR_FILE "RAR_FILE"
#define DATA_TYPE_SHELL_CODE "SHELL_CODE"
#define DATA_TYPE_SMTP_CAPTURE "SMTP_CAPTURE"
#define DATA_TYPE_TAR_FILE "TAR_FILE"
#define DATA_TYPE_ZIP_FILE "ZIP_FILE"
#define DATA_TYPE_BZ2_FILE "BZ2_FILE"
#define DATA_TYPE_GZIP_FILE "GZIP_FILE"
#define DATA_TYPE_COMPRESSION_FILE "COMPRESSION_FILE"
#define DATA_TYPE_LZMA_FILE "LZMA_FILE"
#define DATA_TYPE_XZ_FILE "XZ_FILE"

#define DATA_TYPE_AR_FILE "AR_FILE"
#define DATA_TYPE_CPIO_FILE "CPIO_FILE"
#define DATA_TYPE_ISO9660_FILE "ISO9660_FILE"
#define DATA_TYPE_ELF_FILE "ELF_FILE"

#define NTLV_NAME_SOURCE "SOURCE"
#define NTLV_NAME_DEST "DEST"
#define NTLV_NAME_FILENAME "FILENAME"
#define NTLV_NAME_HOSTNAME "HOSTNAME"
#define NTLV_NAME_PATH "PATH"
#define NTLV_NAME_MALWARENAME "MALWARENAME"
#define NTLV_NAME_REPORT "REPORT"
#define NTLV_NAME_CVE "CVE"
#define NTLV_NAME_BID "BID"
#define NTLV_NAME_OSVDB "OSVDB"
#define NTLV_NAME_URI "URI"
#define NTLV_NAME_HTTP_REQUEST "HTTP_REQUEST"
#define NTLV_NAME_HTTP_RESPONSE "HTTP_RESPONSE"

#define NTLV_TYPE_IPv6_ADDR "IPv6_ADDR"
#define NTLV_TYPE_IPv4_ADDR "IPv4_ADDR"
#define NTLV_TYPE_PORT "PORT"
#define NTLV_TYPE_STRING "STRING"
#define NTLV_TYPE_IPPROTO "IPPROTO"

/// @}

/** Get the UUID for the listed name and type
 * @param p_sName The UUID name
 * @param p_uType The UUID type
 * @param r_uuid The place to put the uuid
 * @return true on success false on error
 */
SO_PUBLIC extern bool UUID_Get_UUID (const char *p_sName, int p_iType, uuid_t r_uuid);

/** Get the description for the listed name and type
 * The string should be free'd when its finished with.
 * @param p_sName The UUID name
 * @param p_uType The UUID type
 * @return NULL on error
 */
SO_PUBLIC extern char *UUID_Get_Description (const char *p_sName, int p_iType);

/** Get the name for the listed uuid and type
 * The string should be free'd when its finished with.
 * @param p_uuid The UUID
 * @param p_iType The UUID type
 * @return NULL on error
 */

SO_PUBLIC extern char *UUID_Get_NameByUUID (uuid_t p_uuid, int p_iType);
/** Get the description for the listed uuid and type
 * The string should be free'd when its finished with.
 * @param p_uuid The UUID
 * @param p_iType The UUID type
 * @return NULL on error
 */
SO_PUBLIC extern char *UUID_Get_DescriptionByUUID (uuid_t p_uuid, int p_iType);

/** Get the UUID in string form for the listed name and type
 * The string should be free'd when its finished with.
 * @param p_sName The UUID name
 * @param p_uType The UUID type
 * @return NULL on error
 */
SO_PUBLIC extern char *UUID_Get_UUIDAsString (const char *p_sName, int p_iType);

SO_PUBLIC extern struct List * UUID_Create_List (void);
SO_PUBLIC extern bool UUID_Add_List_Entry(struct List *list, uuid_t uuid, const char *name, const char *desc);

SO_PUBLIC extern struct List * UUID_Get_List(int type);
SO_PUBLIC extern size_t UUIDList_BinarySize(struct List *list);

#ifdef __cplusplus
}
#endif
#endif
