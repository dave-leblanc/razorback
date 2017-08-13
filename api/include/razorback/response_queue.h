/** @file response_queue.h
 * ResponseQueue functions
 */

#ifndef	RAZORBACK_RESPONSEQUEUE_H
#define	RAZORBACK_RESPONSEQUEUE_H

#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes the response queue
 * @param p_pCollectorId the queue to add it to
 * @param p_iFlags falgs
 * @return a pointer to the queue or null on error.
 */
SO_PUBLIC extern struct Queue * ResponseQueue_Initialize (uuid_t p_pCollectorId, int p_iFlags);

/** Terminates the response queue
 */
SO_PUBLIC extern void ResponseQueue_Terminate (uuid_t p_pCollectorId);

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_RESPONSEQUEUE_H
