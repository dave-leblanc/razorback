/** @file block_pool.h
 * Data block storage pool.
 */
#ifndef RAZORBACK_BLOCK_POOL_H
#define RAZORBACK_BLOCK_POOL_H
#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Block Pool Item States
 * @{
 */
#define BLOCK_POOL_STATUS_MASK                  0x000000FF ///< Mask for status bits.
#define BLOCK_POOL_STATUS_COLLECTING            0x00000001 ///< Collector is adding data
#define BLOCK_POOL_STATUS_FINALIZED             0x00000002 ///< Collector finished adding data
#define BLOCK_POOL_STATUS_CHECK_GLOBAL_CACHE    0x00000004 ///< Waiting for GC Check
#define BLOCK_POOL_STATUS_CHECKING_GLOBAL_CACHE 0x00000008 ///< Waiting for GC Check
#define BLOCK_POOL_STATUS_SUBMIT_DATA           0x00000010 ///< Submit block
#define BLOCK_POOL_STATUS_PAGED                 0x00000011 ///< Block is currently paging out
#define BLOCK_POOL_STATUS_DESTROY               0x00000012 ///< Destroy this item

#define BLOCK_POOL_FLAG_MASK                    0xFF000000 ///< Mask for Flags bits
#define BLOCK_POOL_FLAG_MAY_REUSE               0x01000000 ///< This item should not be fully removed from the pool
#define BLOCK_POOL_FLAG_UPDATE                  0x02000000 ///< This item is being submitted as an update.
#define BLOCK_POOL_FLAG_EVENT_ONLY              0x04000000 ///< This item is being submitted with no block data.

/// @}

#define BLOCK_POOL_DATA_FLAG_FILE       0x01    ///< Data block is mapped to a file
#define BLOCK_POOL_DATA_FLAG_MALLOCD    0x02    ///< Data block is malloc'd
#define BLOCK_POOL_DATA_FLAG_MANAGED    0x04    ///< Data block is managed by the user



/** Create a new item in the pool.
 * @return NULL on error or a pointer to a BlockPoolItem
 */
SO_PUBLIC extern struct BlockPoolItem *BlockPool_CreateItem (struct RazorbackContext *p_pContext);

/** Add data to a item in the block pool
 * @param *p_pItem the item to add data to.
 * @param *p_sName the data type name
 * @return true on success false on error.
 */
SO_PUBLIC extern bool BlockPool_SetItemDataType(struct BlockPoolItem *p_pItem, char * p_sName);

/** Add data to a item in the block pool
 * @param *p_pItem the item to add data to.
 * @param *p_pData the data to add.
 * @param p_iLength the length of the data to add.
 * @param p_iFlags the data block flags
 * @return true on success false on error.
 */
SO_PUBLIC extern bool BlockPool_AddData (struct BlockPoolItem *p_pItem, uint8_t * p_pData,
                               uint32_t p_iLength, int p_iFlags);
SO_PUBLIC extern bool BlockPool_AddData_FromFile(struct BlockPoolItem *item, char *fileName, bool tempFile);

/** Finalize the block pool item for submission
 * @param *p_pItem the item to finalize
 * @ return true on success false on failure.
 */
SO_PUBLIC extern bool BlockPool_FinalizeItem (struct BlockPoolItem *p_pItem);

/** Remove an item from the block pool.
 * @param *p_pItem the item to remove.
 * @return true on success false on error.
 */
SO_PUBLIC extern bool BlockPool_DestroyItem (struct BlockPoolItem *p_pItem);

/** Add MetaData to a block pool item.
 * @param block The block pool item to add metadata to.
 * @param uuidName The UUID of the metadata name
 * @param uuidType The UUID of the metadata data type.
 * @param data The data
 * @param size The size of the data.
 * @return true on success false on error.
 */
SO_PUBLIC extern bool
BlockPool_MetaData_Add(struct BlockPoolItem *block, 
                            uuid_t uuidName, uuid_t uuidType, 
                            uint8_t *data, uint32_t size);
#ifdef __cplusplus
}
#endif
#endif
