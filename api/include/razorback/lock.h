/** @file lock.h
 * Locking functions.
 */
#ifndef RAZORBACK_LOCK_H
#define RAZORBACK_LOCK_H

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef _MSC_VER
#else //_MSC_VER
#include <pthread.h>
#include <semaphore.h>
#endif //_MSC_VER

#ifdef __cplusplus
extern "C" {
#endif

#define MUTEX_MODE_NORMAL 		0	///< None recursively lockable mutex
#define MUTEX_MODE_RECURSIVE 	1	///< Recursively lockable mutex

/** Mutex Control Structure
 */
struct Mutex
{
#ifdef _MSC_VER
	HANDLE recursiveLock;			///< Recursive lock handle (win32)
	CRITICAL_SECTION cs;			///< None recursive lock handle (win32)
#else
    pthread_mutex_t lock;			///< PThreads lock structure.
    pthread_mutexattr_t attrs;		///< PThreads lock attributes.
#endif
	int mode;						///< Lock mode
};

/** Semaphore Control Structure
 */
struct Semaphore
{
#ifdef _MSC_VER
	HANDLE sem;	///< Semaphore handle (win32)
#else
    sem_t sem;	///< PThreads semaphore
#endif
};


/** Create a mutex.
 * @param mode One of MUTEX_MODE_NORMAL or MUTEX_MODE_RECURSIVE
 * @return A new mutex structure or NULL on error.
 */
SO_PUBLIC extern struct Mutex * Mutex_Create(int mode);

/** Lock a mutex
 * @param mutex The mutex to lock.
 * @return true on success false on failure.
 */
SO_PUBLIC extern bool Mutex_Lock(struct Mutex *mutex);

/** Unlock a mutex
 * @param mutex The mutex to unlock.
 * @return true on success false on failure.
 */
SO_PUBLIC extern bool Mutex_Unlock(struct Mutex *mutex);

/** Destroy a mutex
 * @param mutex The mutex to destroy.
 */
SO_PUBLIC extern void Mutex_Destroy(struct Mutex *mutex);

/** Create a semaphore
 * @param shared True if share, false if not.
 * @param value Initial count.
 * @return A new semaphore structure or NULL on error.
 */
SO_PUBLIC extern struct Semaphore * Semaphore_Create(bool shared, unsigned int value);

/** Post to a semaphore
 * @param sem The semaphore to post to.
 * @return true on success false on failure.
 */
SO_PUBLIC extern bool Semaphore_Post(struct Semaphore *sem);
/** Wait for a slot on the semaphore
 * @param sem The semaphore to wait on.
 * @return true on success false on failure.
 */
SO_PUBLIC extern bool Semaphore_Wait(struct Semaphore *sem);
SO_PUBLIC extern bool Semaphore_TimedWait(struct Semaphore *sem);

/** Destroy a semaphore
 * @param sem The semaphore to destroy.
 */
SO_PUBLIC extern void Semaphore_Destroy(struct Semaphore *sem);

#ifdef __cplusplus
}
#endif
#endif //RAZORBACK_LOCK_H
