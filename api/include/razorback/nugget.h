/** @file nugget.h
 * Nugget descriptor functions.
 */
#ifndef RAZORBACK_NUGGET_H
#define RAZORBACK_NUGGET_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Destroy a nugget descriptor.
 * @param nugget The Nugget to destroy.
 */
SO_PUBLIC extern void Nugget_Destroy(struct Nugget *nugget);

#ifdef __cplusplus
}
#endif
#endif
