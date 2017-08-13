#include "config.h"

#include <razorback/thread_pool.h>
#include <razorback/thread.h>
#include <razorback/log.h>

#ifdef _MSC_VER
#include "bobins.h"
#endif


static int 
TP_KeyCmp(void *a, void *id)
{
    struct ThreadPoolItem *item = (struct ThreadPoolItem *)a;
    int *key = (int *)id;
    if (*key == item->id)
        return 0;

    return -1;
}

static int
TP_Cmp(void *a, void *b)
{
    struct ThreadPoolItem *iA = (struct ThreadPoolItem *)a;
    struct ThreadPoolItem *iB = (struct ThreadPoolItem *)b;
    if (a == b)
        return 0;
    if (iA->id == iB->id)
        return 0;

    return -1;
}

static void 
TP_Destroy(void *a)
{
    struct ThreadPoolItem *worker = (struct ThreadPoolItem *)a;
    Thread_Destroy(worker->thread);
    free(a);
}

static void 
ThreadPool_Main(struct Thread *thread)
{
    struct ThreadPoolItem *worker = thread->pUserData;
    struct ThreadPool *pool = worker->pool;
    thread->pUserData = NULL;

    pool->mainFunction(thread);

    List_Remove(pool->list, worker);
}

SO_PUBLIC struct ThreadPool *
ThreadPool_Create(int initialThreads, 
        int maxThreads, 
        struct RazorbackContext *context, 
        const char *namePattern,
        void (*mainFunction) (struct Thread *))
{
    struct ThreadPool *pool;
	int i;
    if ((pool = (struct ThreadPool *)calloc(1,sizeof(struct ThreadPool))) == NULL)
        return NULL;
    if ((pool->list = List_Create( LIST_MODE_GENERIC,
                    TP_Cmp,
                    TP_KeyCmp,
                    TP_Destroy,
                    NULL,
                    NULL,
                    NULL)) == NULL)
    {
        free(pool);
        return NULL;
    }

    pool->limit = maxThreads;
    pool->context = context;
    pool->mainFunction = mainFunction;
    pool->namePattern = namePattern;

    for (i = 0; i < initialThreads; i++)
        ThreadPool_LaunchWorker(pool);

    return pool;
}


SO_PUBLIC bool
ThreadPool_LaunchWorker(struct ThreadPool *pool)
{
    struct ThreadPoolItem *item;
    char* name;
    List_Lock(pool->list);
    if (List_Length(pool->list) >= pool->limit)
    {
        List_Unlock(pool->list);
        return false;
    }

    if (( item = (struct ThreadPoolItem *)calloc(1,sizeof(struct ThreadPoolItem))) == NULL)
    {
        List_Unlock(pool->list);
        return false;
    }
    item->id = pool->nextId++;
    item->pool = pool;

    if (asprintf(&name, pool->namePattern, item->id) == -1)
    {   
        List_Unlock(pool->list);
        free(item);
        return false;
    }
    item->thread = Thread_Launch(ThreadPool_Main, item, name, pool->context);
    List_Push(pool->list, item);
    List_Unlock(pool->list);
    return true;
}

SO_PUBLIC bool
ThreadPool_LaunchWorkers(struct ThreadPool *pool, int count)
{
	int i=0;
    List_Lock(pool->list);
	
    for (i = 0; i < count; i++)
    {
        if (!ThreadPool_LaunchWorker(pool))
        {
            List_Unlock(pool->list);
            return false;
        }
    }
    List_Unlock(pool->list);
    return true;
}

SO_PUBLIC bool
ThreadPool_KillWorker(struct ThreadPool *pool, int id)
{
    struct ThreadPoolItem *worker;
    worker = (struct ThreadPoolItem *)List_Find(pool->list, &id);

    if (worker == NULL)
        return false;

    Thread_InterruptAndJoin(worker->thread);
    return true;
}

static int
ThreadPool_Kill(void *vItem, void *userData)
{
    struct ThreadPoolItem *item = (struct ThreadPoolItem *)vItem;
    Thread_Interrupt(item->thread);
    return LIST_EACH_REMOVE;
}


SO_PUBLIC bool
ThreadPool_KillWorkers(struct ThreadPool *pool)
{
    size_t count;
    List_ForEach(pool->list, ThreadPool_Kill, NULL);

    for (count = List_Length(pool->list); count > 0; count = List_Length(pool->list)) 
        Thread_Yield();
        
    return true;
}

