/** @file block.h
 * Block functions
 */

#ifndef	RAZORBACK_BLOCK_H
#define	RAZORBACK_BLOCK_H
#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Constructs a block
 * @return a new Block or NULL on error.
 */
SO_PUBLIC extern struct Block * Block_Create (void);

/** Destroys a block id
 * @param p_pBlockId the block to destroy
 */
SO_PUBLIC extern void Block_Destroy (struct Block *p_pBlock);

/** Copies a block id
 * @param p_pDestination the destination
 * @param p_pSource the source
 */
SO_PUBLIC extern struct Block * Block_Clone (const struct Block *p_pSource);

/** Calculate the Wire Size of A Block
 * @param the block
 * @return the size
 */
SO_PUBLIC extern uint32_t Block_BinaryLength (struct Block *p_pBlock);

/** Add MetaData to a block.
 * @param block The block to add metadata to.
 * @param uuidName The UUID of the metadata name
 * @param uuidType The UUID of the metadata data type.
 * @param data The data
 * @param size The size of the data.
 * @return true on success false on error.
 */
SO_PUBLIC extern bool
Block_MetaData_Add(struct Block *block, uuid_t uuidName, uuid_t uuidType, uint8_t *data, uint32_t size);

/** Add Filename MetaData to a block.
 * @param block The block.
 * @param fileName The file name.
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool
Block_MetaData_Add_FileName(struct Block *block, const char * fileName);
#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_BLOCKID_H
