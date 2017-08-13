/** @file timer.h
 * Portable timers.
 */
#ifndef RAZORBACK_TIMER_H
#define RAZORBACK_TIMER_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef _MSC_VER
#else //_MSC_VER
#include <signal.h>
#include <time.h>
#endif //_MSC_VER

#ifdef __cplusplus
extern "C" {
#endif

/** Timer structure.
 */
struct Timer
{
#ifdef _MSC_VER
	HANDLE timerQueue;			///< Queue handle
	HANDLE timer;				///< Itemer handle
#else
   timer_t timer;				///< Timer structure.
   struct itimerspec spec;		///< Timer specification.
   struct sigevent *props;		///< Event properties.
#endif
   uint32_t interval;			///< Timer interval in seconds.
   void *userData;				///< User data provided to the event function.
   void (*function)(void *);	///< Function to call when the timer expires, userData will be passed as the argument.
};

/** Create a timer.
 * @param interval Timer interval in seconds.
 * @param handler Function poitner to the routine to run when the timer expires.
 * @param userData Pointer to data to be passed as the argument to handler when the timer expires.
 * @return A new Timer or NULL on error.
 */
SO_PUBLIC extern struct Timer * Timer_Create(uint32_t interval, void (*handler)(void *), void *userData);

/** Destroy a timer.
 * @param timer The Timer to stop and destroy.
 */
SO_PUBLIC extern void Timer_Destroy(struct Timer *timer);

#ifdef __cplusplus
}
#endif
#endif //RAZORBACK_TIMER_H
