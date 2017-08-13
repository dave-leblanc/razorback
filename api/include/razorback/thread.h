/** @file thread.h
 * Threading API.
 */

#ifndef	RAZORBACK_THREAD_H
#define	RAZORBACK_THREAD_H

#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/lock.h>
#include <razorback/api.h>
#ifdef _MSC_VER
typedef DWORD rzb_thread_t;
#else //_MSC_VER
#include <pthread.h>
typedef pthread_t rzb_thread_t;
#endif //_MSC_VER

#ifdef __cplusplus
extern "C" {
#endif

/** Thread
 * Purpose:	hold the information about a thread
 */
struct Thread
{
#ifdef _MSC_VER
	HANDLE hThread;
#endif //_MSC_VER
    rzb_thread_t iThread;       			///< pthread Thread info.
    struct Mutex * mMutex;  				///< mutex protecting this struct
    bool bRunning;              			///< true if executing, false if not:  must be managed explicitly by thread function
    void *pUserData;						///< Additional info for the thread
    char *sName;            				///< The thread name
    struct RazorbackContext *pContext; 		///< The Thread Context
    void (*mainFunction) (struct Thread *); ///< Thread Main Function
    bool bShutdown;         				///< Shutdown Flag
    int refs;								///< Reference count
    //void (*interrupt)(struct Thread *);		///< Cancellation handler for a blocking function.
};

/** Create a new thread
 * @param *function The function the thread will execute
 * @param *userData The thread user data
 * @param *name The name of the thread
 * @param *context The initial context of the thread
 * @return Null on error a new Thread on success.
 */
SO_PUBLIC extern struct Thread *Thread_Launch (void (*function) (struct Thread *),
                                     void *userData, char *name,
                                     struct RazorbackContext *context);

/** Change the registered context of a running thread.
 * @param thread the thread to change
 * @param context the new context
 * @return The old context
 */
SO_PUBLIC extern struct RazorbackContext * Thread_ChangeContext(struct Thread *thread,
                                    struct RazorbackContext *context);

/** Get the registered context of a running thread.
 * @param thread the thread to change
 * @return The current context for the thread.
 */
SO_PUBLIC extern struct RazorbackContext * Thread_GetContext(struct Thread *p_pThread);

/** Get the running context for the current thread.
 * @return The current context.
 */
SO_PUBLIC extern struct RazorbackContext * Thread_GetCurrentContext(void);

/** Destroy a threads data
 * @param *thread The thread to destroy
 */
SO_PUBLIC extern void Thread_Destroy (struct Thread *thread);

/** Checks whether a thread is running or not
 * @param *thread The thread the test
 * @return true if running, false if not
 */
SO_PUBLIC extern bool Thread_IsRunning (struct Thread *thread);

/** Check if a thread has finished.
 * @param thread The thread to test.
 * @return true if the thread has exited, false if its still running.
 */
SO_PUBLIC extern bool Thread_IsStopped (struct Thread *thread);

/** Suspend execution of the calling thread until the target thread therminates.
 * @param thread The target thread.
 */
SO_PUBLIC extern void Thread_Join(struct Thread *thread);
/** Interrupt the target thread.
 * @param thread The target thread.
 */
SO_PUBLIC extern void Thread_Interrupt(struct Thread *thread);
/** Stop the target thread.
 * @param thread The target thread.
 */
SO_PUBLIC extern void Thread_Stop (struct Thread *thread);
SO_PUBLIC extern void Thread_StopAndJoin (struct Thread *thread);
SO_PUBLIC extern void Thread_InterruptAndJoin (struct Thread *thread);
/** Cause the current thread to yield execution to other runnable threads.
 */
SO_PUBLIC extern void Thread_Yield(void);

/** Get the number of currently running threads.
 * @return the number of currently running threads.
 */
SO_PUBLIC extern uint32_t Thread_getCount (void);

/** Get the current thread.
 */
SO_PUBLIC extern struct Thread *Thread_GetCurrent(void);
/** Get the current thread ID.
 */
SO_PUBLIC extern rzb_thread_t Thread_GetCurrentId(void);

SO_PUBLIC extern int Thread_KeyCmp(void *a, void *id);
SO_PUBLIC extern int Thread_Cmp(void *a, void *b);

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_THREAD_H
