/** Binary Buffer Implimentation 
 * @file binary_buffer.h
 */
#ifndef RAZORBACK_BINARY_BUFFER_H
#define RAZORBACK_BINARY_BUFFER_H

#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/ntlv.h>

#ifdef __cplusplus
extern "C" {
#endif
/** Wire Format Message
 */
struct BinaryBuffer
{
    uint32_t iFlags;            ///< the flags, USER SHOULD NOT CHANGE
    uint32_t iLength;           ///< the length of the buffer
    uint32_t iOffset;           ///< the cursor for reading and writing to the buffer.
    uint8_t *pBuffer;           ///< the data buffer
};

///** Given that the first four bytes of pBuffer are the network order of the length, set iLength
// * @param p_pBB the buffer
// */
//extern void BinaryBuffer_CorrectLength (struct BinaryBuffer *p_pBB);
//
//
/** Create a buffer for the message with header p_pMesgHeader and copy the header into the buffer.
 * @param *p_pBuffer The buffer object
 * @param *p_pMesgHeader The header of the message the buffer is being created for.
 * @return true on success false on failure.
 */
//extern bool BinaryBuffer_CreateMessage (struct BinaryBuffer *p_pBuffer,
//                                      struct MessageHeader *p_pMesgHeader);

struct BinaryBuffer *
BinaryBuffer_CreateFromMessage (struct Message *message);

/** Create a buffer
 * @param p_iSize The size of the buffer
 * @return true on success, false on failure
*/
extern struct BinaryBuffer *BinaryBuffer_Create (uint32_t p_iSize);

/** frees the memory allocated at pBuffer unless iFlags shows local
 * @param *p_pBB     the object
 */
extern void BinaryBuffer_Destroy (struct BinaryBuffer *p_pBB);

/** Primative type PUT and GET functions.
 * @{
 */

/** Prepares a binary buffer for put commands
 * @param p_pBB the buffer
 */
extern void BinaryBuffer_PrepareForPut (struct BinaryBuffer *p_pBB);

/** appends p_iValue to the buffer
 * @param *p_pBuffer    the object
 * @param p_iValue            the value to append
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Put_uint8_t (struct BinaryBuffer *p_pBuffer,
                                      uint8_t p_iValue);

/** appends p_iValue to the buffer
 * @param *p_pBuffer     the object
 * @param p_iValue           the value to append
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Put_uint16_t (struct BinaryBuffer *p_pBuffer,
                                       uint16_t p_iValue);

/** appends p_iValue to the buffer
 * @param *p_pBuffer     the object
 * @param p_iValue           the value to append
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Put_uint32_t (struct BinaryBuffer *p_pBuffer,
                                       uint32_t p_iValue);

/** appends p_iValue to the buffer
 * @param *p_pBuffer     the object
 * @param p_iValue           the value to append
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Put_uint64_t (struct BinaryBuffer *p_pBuffer,
                                       uint64_t p_iValue);

/** appends p_iValue to the buffer, changes iLength and length in pBuffer
 * @param *p_pBuffer The buffer object
 * @param p_iSize The length of the byte array
 * @param *p_pByteArray  the byte array to append.
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Put_ByteArray (struct BinaryBuffer *p_pBuffer,
                                        uint32_t p_iSize,
                                        const uint8_t * p_pByteArray);

/** appends p_iValue to the buffer, changes iLength and length in pBuffer
 * @param *p_pBuffer The buffer object
 * @param p_sString The string
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Put_String (struct BinaryBuffer *p_pBuffer,
                                     const uint8_t * p_sString);

/** Prepares a binary buffer for get commands
 * @param p_pBB the buffer
 * @return true if ok, otherwise false
 */
extern bool BinaryBuffer_PrepareForGet (struct BinaryBuffer *p_pBB);

/** reads the next value from buffer
 * @param *p_pBuffer         the object
 * @param *p_pValue          the value read
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Get_uint8_t (struct BinaryBuffer *p_pBuffer,
                                      uint8_t * p_pValue);

/** reads the next value from buffer
 * @param *p_pBuffer     the object
 * @param *p_pValue         the value read
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Get_uint16_t (struct BinaryBuffer *p_pBuffer,
                                       uint16_t * p_pValue);

/** reads the next value from buffer, changes iOffset
 * @param *p_pBuffer     the object
 * @param *p_pValue         the value read
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Get_uint32_t (struct BinaryBuffer *p_pBuffer,
                                       uint32_t * p_pValue);

/** reads the next value from buffer, changes iOffset
 * @param *p_pBuffer     the object
 * @param *p_pValue         the value read
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Get_uint64_t (struct BinaryBuffer *p_pBuffer,
                                       uint64_t * p_pValue);


/** reads a null terminated string from the buffer
 * @param *p_pBuffer     the object
 * @param *r_pString      This pointer will be updated to the new string.
 * @return true on success false on failure.
 */
extern uint8_t * BinaryBuffer_Get_String (struct BinaryBuffer *p_pBuffer);

/** reads the next value from buffer, changes iOffset
 * @param *p_pBuffer the object
 * @param p_iSize the size of p_sTerminatedByteArray
 * @param p_pByteArray the value read, of p_iSize
 * @return true on success false on failure.
 */
extern bool BinaryBuffer_Get_ByteArray (struct BinaryBuffer *p_pBuffer,
                                        uint32_t p_iSize,
                                        uint8_t * p_pByteArray);

/// @}

/** Get a uuid from the message.
 * @param *p_pBuffer the message buffer.
 * @param uuid the uuid.
 * @return true on success false on error.
 */
extern bool BinaryBuffer_Get_UUID (struct BinaryBuffer *p_pBuffer,
                                   uuid_t p_uuid);
/** Add a uuid to the message.
 * @param *p_pBuffer the message buffer.
 * @param uuid the uuid.
 * @return true on success false on error.
 */
extern bool BinaryBuffer_Put_UUID (struct BinaryBuffer *p_pBuffer,
                                   uuid_t p_uuid);

/** Add a NTLV list to the message
 * @param *p_pBuffer the message buffer
 * @param *p_pList the list.
 * @return true on success false on error.
 */
extern bool BinaryBuffer_Put_NTLVList (struct BinaryBuffer *p_pBuffer,
                                       struct List *p_pList);

/** Get a NTLV list from the message
 * @param *p_pBuffer the message buffer
 * @param *p_pList the list.
 * @return true on success false on error.
 */
extern bool BinaryBuffer_Get_NTLVList (struct BinaryBuffer *p_pBuffer,
                                       struct List **p_pList);

/** puts a hash into a binary buffer
 * @param p_pBuffer the binary buffer
 * @param p_pHash the hash
 * @return true if ok, false on failure.
 */
extern bool BinaryBuffer_Put_Hash (struct BinaryBuffer *p_pBuffer,
                                   const struct Hash *p_pHash);

/** gets a hash from a binary buffer
 * @param p_pBuffer the binary buffer
 * @param p_pHash the hash
 * @return true if ok, false on error.
 */
extern bool BinaryBuffer_Get_Hash (struct BinaryBuffer *p_pBuffer,
                                   struct Hash **p_pHash);

/** Puts a block id into a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pId the block id
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Put_BlockId (struct BinaryBuffer *p_pBuffer,
                                      struct BlockId *p_pId);

/** Gets a block id from a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pId the block id
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Get_BlockId (struct BinaryBuffer *p_pBuffer,
                                      struct BlockId **p_pId);

/** Puts a block into a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pBlock the block
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Put_Block (struct BinaryBuffer *p_pBuffer,
                                    struct Block *p_pBlock);

/** Gets a block from a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pBlock the block
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Get_Block (struct BinaryBuffer *p_pBuffer,
                                    struct Block **p_pBlock);

/** Puts an event into a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pEvent the block
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Put_Event (struct BinaryBuffer *p_pBuffer,
                                    struct Event *p_pEvent);

/** Gets an event from a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pEvent the block
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Get_Event (struct BinaryBuffer *p_pBuffer,
                                    struct Event **p_pEvent);

/** Puts an event id into a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pEventId the event id
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Put_EventId (struct BinaryBuffer *p_pBuffer,
                                    struct EventId *p_pEventId);

/** Gets an event id from a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pEventId the event id
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Get_EventId (struct BinaryBuffer *p_pBuffer,
                                    struct EventId **p_pEventId);

/** Puts an judgment into a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pJudgment the judgment
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Put_Judgment (struct BinaryBuffer *p_pBuffer,
                                    struct Judgment *p_pJudgment);

/** Gets an judgment from a binary buffer
 * @param p_pBuffer the buffer
 * @param p_pJudgment the judgment
 * @return true if ok, false if EFAULT
 */
extern bool BinaryBuffer_Get_Judgment (struct BinaryBuffer *p_pBuffer,
                                    struct Judgment **p_pJudgment);

bool
BinaryBuffer_Get_UUIDList(struct BinaryBuffer *buffer, struct List **r_list);
bool
BinaryBuffer_Put_UUIDList(struct BinaryBuffer *buffer, struct List *list);

bool
BinaryBuffer_Get_StringList(struct BinaryBuffer *buffer, struct List **r_list);

bool
BinaryBuffer_Put_StringList(struct BinaryBuffer *buffer, struct List *list);
#ifdef __cplusplus
}
#endif
#endif //RAZORBACK_BINARY_BUFFER_H
