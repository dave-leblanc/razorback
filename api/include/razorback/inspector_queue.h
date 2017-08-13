/** @file inspector_queue.h
 * InspectorQueue functions
 */

#ifndef	RAZORBACK_INSPECTORQUEUE_H
#define	RAZORBACK_INSPECTORQUEUE_H

#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes the inspector queue
 * @param p_iFlags flags
 * @param p_pApplicationType the application type
 * @return a pointer to the Queue object or NULL on an error.
 */
SO_PUBLIC extern struct Queue * InspectorQueue_Initialize (uuid_t p_pApplicationType,
                                       int p_iFlags);

/** Terminates the inspector queue
 */
SO_PUBLIC extern void InspectorQueue_Terminate (uuid_t p_pApplicationType);

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_INSPECTORQUEUE_H
