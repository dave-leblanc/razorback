/** #file event.h
 * Event functions.
 */
#ifndef RAZORBACK_EVENT_H
#define RAZORBACK_EVENT_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Create an EventId.
 * @return A new EventId or NULL on error.
 */
SO_PUBLIC extern struct EventId * EventId_Create (void);

/** Clone an event ID.
 * @param event The EventId to clone
 * @return A new event object or NULL on error.
 */
SO_PUBLIC extern struct EventId * EventId_Clone (struct EventId *event);

/** Destroy an EventID
 * @param event The EventId to destroy.
 */
SO_PUBLIC extern void EventId_Destroy (struct EventId *event);

/** Create an Event.
 * @return A new event or NULL on error.
 */
SO_PUBLIC extern struct Event * Event_Create (void);

/** Destroy an Event.
 * @param event The Event to destroy.
 */
SO_PUBLIC extern void Event_Destroy (struct Event *event);

/** Get the serialize binary length of an event.
 * @param event The event to use.
 * @return The serialized length.
 */
SO_PUBLIC extern uint32_t Event_BinaryLength (struct Event *event);

#ifdef __cplusplus
}
#endif
#endif
