/** @file thread_pool.h
 * Thread worker pool API.
 */

#ifndef	RAZORBACK_THREAD_POOL_H
#define	RAZORBACK_THREAD_POOL_H

#include <razorback/visibility.h>
#include <razorback/types.h>
#include <razorback/api.h>
#include <razorback/list.h>

#ifdef __cplusplus
extern "C" {
#endif
struct ThreadPool;
/**
 * Thread pool item
 */
struct ThreadPoolItem
{
    struct Thread *thread;		///< The worker thread.
    int id;						///< The worker ID
    struct ThreadPool *pool;	///< The pool the worker belongs to.
};

/**
 * Thread pool container.
 */
struct ThreadPool
{
    size_t limit;                           ///< Maximum number of threads
    int nextId;                             ///< Id of the next thread
    struct RazorbackContext *context;       ///< Context to spawn threads in
    void (*mainFunction) (struct Thread *); ///< Main function for spawned threads
    const char *namePattern;                ///< Name pattern for threads
    struct List *list;						///< Worker list.
};

/** Create a ThreadPool
 * @param intialThread The number of threads to launch on creation.
 * @param maxThreads The maximum number of workers allowed in the pool.
 * @param context The context of the worker threads.
 * @param namePattern The printf pattern for the worker thread names, must contain %d
 * @param mainFunction The main routine for the threads.
 * @return A new ThreadPool or NULL on error.
 */
SO_PUBLIC extern struct ThreadPool *
ThreadPool_Create(int initialThreads, int maxThreads, struct RazorbackContext *context, const char* namePattern, void (*mainFunction) (struct Thread *));

/** Launch a worker
 * @param pool The ThreadPool to spawn a worker in.
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool
ThreadPool_LaunchWorker(struct ThreadPool *pool);

/** Launch several workers.
 * @param pool The ThreadPool to spawn the workers in.
 * @param count The number of workers to spawn.
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool
ThreadPool_LaunchWorkers(struct ThreadPool *pool, int count);

/** Kill a worker
 * @param pool The ThreadPool to which the worker belongs.
 * @param id The workers id.
 * @return true on success, false on error.
 */
SO_PUBLIC extern bool
ThreadPool_KillWorker(struct ThreadPool *pool, int id);

/** Kill all workers
 * @param pool The ThreadPool to kill the workers in.
 * @return true on success, false on error
 */
SO_PUBLIC extern bool
ThreadPool_KillWorkers(struct ThreadPool *pool);

#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_THREAD_POOL_H

