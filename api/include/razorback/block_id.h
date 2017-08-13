/** @file block_id.h
 * BlockId functions
 */

#ifndef	RAZORBACK_BLOCK_ID_H
#define	RAZORBACK_BLOCK_ID_H
#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Tests whether two blockids are equal
 * @param p_pA the first BlockId to compare
 * @param p_pB the second BlockId to compare
 * @return true if they are the same, false if they are not
 */
SO_PUBLIC extern bool BlockId_IsEqual (const struct BlockId *p_pA,
                             const struct BlockId *p_pB);

/** Converts a block id to text
 * @param p_pA the block id to convert
 * @param p_sText the destination text 
 */
SO_PUBLIC extern void BlockId_ToText (const struct BlockId *p_pA, uint8_t * p_sText);

/** Constructs a block id wihtout internal parts.
 * @return a new block ID or null on error
 */
SO_PUBLIC extern struct BlockId * BlockId_Create_Shallow (void);

/** Constructs a block id
 * @return a new block ID or null on error
 */
SO_PUBLIC extern struct BlockId * BlockId_Create (void);

/** Returns the size of string required in the ToText function
 * @return the size of the string
 */
SO_PUBLIC extern uint32_t BlockId_StringLength (struct BlockId *p_pB);

/** Destroys a block id
 * @param p_pBlockId the block to destroy
 */
SO_PUBLIC extern void BlockId_Destroy (struct BlockId *p_pBlockId);

/** Copies a block id
 * @param p_pSource the source
 * @return A copy of the block id on success, NULL on an error.
 */
SO_PUBLIC extern struct BlockId * BlockId_Clone (
                          const struct BlockId *p_pSource);

/** Gets binary length of block id
 * @param p_pBlockId the block id
 * @return the size when placed in a binary buffer
 */
SO_PUBLIC extern uint32_t BlockId_BinaryLength (const struct BlockId *p_pBlockId);

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_BLOCKID_H
