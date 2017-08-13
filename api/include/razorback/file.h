/** @file file.h
 * File handling functions.
 */
#ifndef RAZORBACK_FILE_H
#define RAZORBACK_FILE_H

#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Delete a file from the block store.
 * @param block The block to remove
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool File_Delete(struct Block *block);

#ifdef __cplusplus
}
#endif
#endif
