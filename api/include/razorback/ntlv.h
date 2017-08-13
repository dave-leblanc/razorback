/** @file ntlv.h
 * Name Type Length Value data field wrapper.
 */
#ifndef	RAZORBACK_NTLV_H
#define	RAZORBACK_NTLV_H
#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Name Type Length Block
 */
struct NTLVItem
{
    uuid_t uuidName;            ///< The UUID of the data type name.
    uuid_t uuidType;            ///< The UUID of the data type in this block
    uint32_t iLength;           ///< The length of the data in this block
    uint8_t *pData;             ///< The data
};

SO_PUBLIC extern struct List * NTLVList_Create(void);
/** Add a new entry to a user data list
 * @param *p_pList The destination
 * @param uuidName The name
 * @param uuidType The type
 * @param p_iSize The size
 * @param *p_pData The data
 * @return true on success, false on failure.
 */
SO_PUBLIC extern bool NTLVList_Add (struct List *p_pList, uuid_t uuidName,
                          uuid_t uuidType, uint32_t p_iSize,
                          const uint8_t * p_pData);

SO_PUBLIC extern bool NTLVList_Get (struct List *p_pList, uuid_t uuidName,
                          uuid_t uuidType, uint32_t *p_iSize,
                          const uint8_t ** p_pData);

/** Get the size of the items in a list
 * @param *p_pList The list
 * @return The size of the items in the list.
 */
SO_PUBLIC extern uint32_t NTLVList_Size (struct List *list);

#ifdef __cplusplus
}
#endif
#endif
