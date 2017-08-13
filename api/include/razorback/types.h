/** @file types.h
 * Razorback API data types.
 */
#ifndef RAZORBACK_TYPES_H
#define RAZORBACK_TYPES_H


#include <stdint.h>
#ifdef _MSC_VER
#include <WinSock2.h>
#include "safewindows.h"
#define bool BOOL
#define true TRUE
#define false FALSE
#else //_MSC_VER
#include <stdbool.h>
#include <unistd.h>
#endif //_MSC_VER
#include <uuid/uuid.h>
#include <stdlib.h>
#include <openssl/evp.h>

#include <razorback/list.h>

#define UUID_STRING_LENGTH 37   ///< The size of a UUID String including the null

/** Lookup results enum.
 */
typedef enum 
{
    R_SUCCESS = 0,  ///< R_SUCCESS
    R_ERROR = 1,    ///< R_ERROR
    R_FOUND = 2,    ///< R_FOUND
    R_NOT_FOUND = 3,///< R_NOT_FOUND
} Lookup_Result;

/** Hash types
 * @{
 */
#define HASH_TYPE_MD5 1         ///< MD5 Hash
#define HASH_TYPE_SHA1 2        ///< SHA-1 Hash
#define HASH_TYPE_SHA224 3      ///< SHA224 Hash
#define HASH_TYPE_SHA256 4      ///< SHA256 Hash
#define HASH_TYPE_SHA512 5      ///< SHA512 Hash
/// @}

/** Hash Flags
 * @{
 */
#define HASH_FLAG_FINAL 0x00000001  ///< Hash has been finalized.
/// @}

/** Block Hash
 * utilize various algorithms, e.g. MD5, SHA256, etc. to uniquely identify block of data.
 */
struct Hash
{
    uint32_t iType;             ///< The hash Type.
    uint32_t iSize;             ///< size of the data stored, must be the same for all hashes in system
    uint8_t *pData;             ///< actual data of the hash
    EVP_MD_CTX CTX;         	///< Private hash data.
    uint32_t iFlags;            ///< Hash Flags.
};

/** Data Block ID
 * If iLength is zero we don't have the block just the hash.
 */
struct BlockId
{
    struct Hash *pHash;         ///< The hash of the block
    uuid_t uuidDataType;        ///< The UUID of the data type in the block
    uint64_t iLength;           ///< The length of the data in the block
};

/** Data Block tracker.
 */
struct BlockData
{
    char *fileName;				///< The name of the temporary file.
    uint8_t *pointer;			///< Pointer to the data
    FILE *file;					///< File handle for the data.
    bool tempFile;				///< Should file be unlinked during cleanup.
#ifdef _MSC_VER
	HANDLE mfileHandle;			///< File map handle
	HANDLE mapHandle;			///< Memory map handle
#endif
};

/** Data Block
 */
struct Block
{
    struct BlockId *pId;       	///< Block ID
    struct BlockId *pParentId;  ///< Parent Block ID
    struct Block *pParentBlock;	///< Parent Block
    struct List *pMetaDataList; ///< Meta Data List
    struct BlockData data;		///< Block data
};

/** Block Pool Item Data
 */
struct BlockPoolData
{
    uint32_t iLength;           	///< Size of data block
    int iFlags;                 	///< Data Block Flags
    struct BlockData data;			///< Data object
    struct BlockPoolData *pNext;    ///< Next item in the chain
};

/** Block Pool Item
 */
struct BlockPoolItem
{
    struct Mutex *mutex;                              	///< Item lock <- Why is it brown.
    uint32_t iStatus;                                   ///< Status Flags
    struct BlockPoolData *pDataHead;                    ///< Head Item
    struct BlockPoolData *pDataTail;                    ///< Tail Item
    void (*submittedCallback) (struct BlockPoolItem *); ///< Post submission call back
    struct Event *pEvent;								///< Event data for the submission.
    void *userData;										///< User data attached to the item
};

/** Event ID
 */
struct EventId
{
    uuid_t uuidNuggetId;            ///< Id of the nugget creating the event
    uint64_t iSeconds;              ///< Time Stamp
    uint64_t iNanoSecs;             ///< Time Stamp

};

/** Event
 */
struct Event
{
    struct EventId *pId;            ///< The event id.
    struct EventId *pParentId;      ///< The parent event id.
    struct Event *pParent;      	///< The parent event
    uuid_t uuidApplicationType;     ///< Application Type
    struct Block *pBlock;           ///< The data block
    struct List *pMetaDataList; 	///< Meta Data List
};

/** Judgment
 */
struct Judgment
{
    uuid_t uuidNuggetId;            ///< The nugget submitting
    uint64_t iSeconds;              ///< Time Stamp
    uint64_t iNanoSecs;             ///< Time Stamp
    struct EventId *pEventId;       ///< Event Id
    struct BlockId *pBlockId;       ///< Block Id
    uint8_t iPriority;              ///< Meh, Dodgy, YF, YRF
    struct List *pMetaDataList; 	///< Meta Data List
    uint32_t iGID;                  ///< The GID
    uint32_t iSID;                  ///< The SID
    uint32_t Set_SfFlags;           ///< The blocks Sourcefire flags
    uint32_t Set_EntFlags;          ///< The blocks enterprise flags
    uint32_t Unset_SfFlags;         ///< The blocks Sourcefire flags
    uint32_t Unset_EntFlags;        ///< The blocks enterprise flags
    uint8_t *sMessage;              ///< The message
};

/** Nugget information.
 */
struct Nugget 
{
    uuid_t uuidNuggetId;		///< Nugget ID
    uuid_t uuidApplicationType;	///< Application Type
    uuid_t uuidNuggetType;		///< Nugget Type
    char *sName;				///< Name
    char *sLocation;			///< Location
    char *sContact;				///< Contact
    char *sNotes;				///< Notes
};

/** Deferred Data Block List
 */
struct DeferredList
{
    uint8_t stuff;
};



/** Sourcefire Flags
 * @{
 */
#define SF_FLAG_GOOD        0x00000001	///< Block is good
#define SF_FLAG_BAD         0x00000002	///< Block is bad
#define SF_FLAG_WHITE_LIST  0x00000004	///< Block is white listed
#define SF_FLAG_BLACK_LIST  0x00000008	///< Block is black listed
#define SF_FLAG_DIRTY       0x00000010	///< Block is marked for re-inspection
#define SF_FLAG_CANHAZ      0x00000020	///< Block data is required.
#define SF_FLAG_PROCESSING  0x00000040	///< Block is being processed currently
// Duplication Intended
#define SF_FLAG_DODGY       0x00000080	///< Block is neither good nor bad
#define SF_FLAG_SUSPICIOUS  0x00000080	///< Block is neither good nor bad

#define SF_FLAG_ALL         0xffffffff	///< All flags mask.
/// @}

/** Judgment types
 * @{
 */
#define JUDGMENT_REASON_DONE 		0	///< Inspection has completed
#define JUDGMENT_REASON_ALERT 		1	///< Inspection alert output
#define JUDGMENT_REASON_ERROR 		2	///< Inspection failed
#define JUDGMENT_REASON_DEFERRED 	3	///< Inspection enqueued and results deferred.
#define JUDGMENT_REASON_PENDING 	4	///< Inspection result current pending (@note This is never sent in a message, it is the state the dispatcher stores for an inspection record.)
/// @}

/** Transfer protocols
 * @{
 */
#define TRANSFER_METHOD_FILE 	0	///< Transfer to block store shared file system
#define TRANSFER_METHOD_SSH		1	///< Transfer via SSH+SFTP
#define TRANSFER_METHOD_HTTP 	2	///< Transfer via HTTP
/// @}

/** Submission types.
 * @{
 */
#define SUBMISSION_REASON_EVENT 	0	///< Submission from event.
#define SUBMISSION_REASON_REQUESTED 1	///< Submission due to administrative request.
/// @}

#endif //RAZORBACK_TYPES_H
