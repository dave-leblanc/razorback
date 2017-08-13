#include "config.h"
#include <razorback/debug.h>
#include <razorback/lock.h>
#include <razorback/log.h>

static bool Mutex_Init(struct Mutex *);

SO_PUBLIC struct Mutex * 
Mutex_Create(int mode)
{
    struct Mutex *ret;
    if ((ret = (struct Mutex *)calloc(1,sizeof(struct Mutex))) == NULL)
        return NULL;
	ret->mode = mode;
    if (!Mutex_Init(ret))
    {
        Mutex_Destroy(ret);
        return NULL;
    }
    return ret;
}

SO_PUBLIC bool 
Mutex_Lock(struct Mutex *mutex)
{
    ASSERT(mutex != NULL);
    if (mutex == NULL)
        return false;
#ifdef _MSC_VER
	switch (mutex->mode)
    {
    case MUTEX_MODE_RECURSIVE:
		WaitForSingleObject(mutex->recursiveLock, INFINITE);
        break;
    case MUTEX_MODE_NORMAL:
		EnterCriticalSection(&mutex->cs);
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid mutex mode: %d", mutex->mode);
        return false;
    }
#else //_MSC_VER
    if (pthread_mutex_lock(&mutex->lock) != 0)
        return false;
#endif //_MSC_VER
    return true;
}

SO_PUBLIC bool 
Mutex_Unlock(struct Mutex *mutex)
{
    ASSERT(mutex != NULL);
    if (mutex == NULL) 
        return false;
#ifdef _MSC_VER
	switch (mutex->mode)
    {
    case MUTEX_MODE_RECURSIVE:
		ReleaseMutex(mutex->recursiveLock);
        break;
    case MUTEX_MODE_NORMAL:
		LeaveCriticalSection(&mutex->cs);
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid mutex mode: %d", mutex->mode);
        return false;
    }
#else //_MSC_VER
    if (pthread_mutex_unlock(&mutex->lock) != 0)
        return false;
#endif //_MSC_VER
    return true;
}

SO_PUBLIC void 
Mutex_Destroy(struct Mutex *mutex)
{
    ASSERT(mutex != NULL);
    if (mutex == NULL)
        return;
#ifdef _MSC_VER
	switch (mutex->mode)
    {
    case MUTEX_MODE_RECURSIVE:
		CloseHandle(mutex->recursiveLock);
        break;
    case MUTEX_MODE_NORMAL:
		DeleteCriticalSection(&mutex->cs);
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid mutex mode: %d", mutex->mode);
    }
	
#else //_MSC_VER
    pthread_mutex_destroy(&mutex->lock);
    pthread_mutexattr_destroy(&mutex->attrs);
#endif //_MSC_VER
    free(mutex);
}

SO_PUBLIC struct Semaphore * 
Semaphore_Create(bool shared, unsigned int value)
{
    struct Semaphore *ret;
    if (( ret = (struct Semaphore *)calloc(1,sizeof(struct Semaphore))) == NULL)
        return NULL;
#ifdef _MSC_VER
	ret->sem = CreateSemaphore(NULL,0,LONG_MAX,NULL);
	if (!ret->sem)
	{
		free(ret);
		return NULL;
	}

#else //_MSC_VER
    sem_init (&ret->sem, (shared ? 1 : 0), value);
#endif //_MSC_VER
    return ret;
}

SO_PUBLIC bool 
Semaphore_Post(struct Semaphore *sem)
{
    ASSERT(sem != NULL);
    if (sem == NULL)
        return false;
#ifdef _MSC_VER
	ReleaseSemaphore(sem->sem, 1, NULL);
#else //_MSC_VER
    if (sem_post(&sem->sem) != 0)
        return false;
#endif //_MSC_VER
    return true;
}

SO_PUBLIC bool 
Semaphore_TimedWait(struct Semaphore *sem)
{
    ASSERT(sem != NULL);
    if (sem == NULL)
        return false;
	UNIMPLEMENTED();
#ifdef _MSC_VER
	UNIMPLEMENTED();
#else //_MSC_VER
    if (sem_wait(&sem->sem) != 0)
        return false;
#endif //_MSC_VER
    return true;
}

SO_PUBLIC bool 
Semaphore_Wait(struct Semaphore *sem)
{
    ASSERT(sem != NULL);
    if (sem == NULL)
        return false;
#ifdef _MSC_VER
	WaitForSingleObject(sem->sem, INFINITE);
#else //_MSC_VER
    if (sem_wait(&sem->sem) != 0)
        return false;
#endif //_MSC_VER
    return true;
}

SO_PUBLIC void 
Semaphore_Destroy(struct Semaphore *sem)
{
    ASSERT(sem != NULL);
    if (sem == NULL)
        return;
#ifdef _MSC_VER
	CloseHandle(sem->sem);
#else //_MSC_VER
    sem_destroy(&sem->sem);
#endif //_MSC_VER
    free(sem);
}
    


#ifdef _MSC_VER
static bool Mutex_Init(struct Mutex *mutex)
{
    ASSERT(mutex != NULL);
    if (mutex == NULL) 
        return false;
	switch (mutex->mode)
    {
    case MUTEX_MODE_RECURSIVE:
		mutex->recursiveLock = CreateMutex( 
			NULL,              // default security attributes
			FALSE,             // initially not owned
			NULL);				// unnamed mutex
		if (mutex->recursiveLock == NULL)
			return false;
        break;
    case MUTEX_MODE_NORMAL:
		InitializeCriticalSection(&mutex->cs);
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid mutex mode: %d", mutex->mode);
        return false;
    }
	
    return true;
}
#else //_MSC_VER

static bool Mutex_Init(struct Mutex *mutex)
{
    ASSERT(mutex != NULL);
    if (mutex == NULL) 
        return false;
    pthread_mutexattr_init(&mutex->attrs);

    switch (mutex->mode)
    {
    case MUTEX_MODE_RECURSIVE:
        pthread_mutexattr_settype(&mutex->attrs, PTHREAD_MUTEX_RECURSIVE);
        break;
    case MUTEX_MODE_NORMAL:
        break;
    default:
        rzb_log(LOG_ERR, "%s: Invalid mutex mode: %d", mutex->mode);
        return false;
    }
    pthread_mutex_init(&mutex->lock, &mutex->attrs); 
    return true;
}
#endif //_MSC_VER
