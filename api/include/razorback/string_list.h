/** @file ntlv.h
 * Name Type Length Value data field wrapper.
 */
#ifndef	RAZORBACK_STRING_LIST_H
#define	RAZORBACK_STRING_LIST_H
#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

SO_PUBLIC extern struct List * StringList_Create(void);
/** Add a new entry to a user data list
 * @param *p_pList The destination
 * @param string The string to add.
 * @return true on success, false on failure.
 */
SO_PUBLIC extern bool StringList_Add (struct List *p_pList, const char *string);


#if 0
SO_PUBLIC extern bool NTLVList_Get (struct List *p_pList, uuid_t uuidName,
                          uuid_t uuidType, uint32_t *p_iSize,
                          const uint8_t ** p_pData);
#endif
/** Get the size of the items in a list
 * @param *p_pList The list
 * @return The size of the items in the list.
 */
SO_PUBLIC extern uint32_t StringList_Size (struct List *list);

#ifdef __cplusplus
}
#endif
#endif
