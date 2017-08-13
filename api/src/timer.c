#include "config.h"
#include <razorback/debug.h>
#include <razorback/log.h>
#include <razorback/timer.h>

#ifdef _MSC_VER
static bool Timer_Init_Win32(struct Timer *);
static void CALLBACK Timer_Wrapper(void * lpParam, bool TimerOrWaitFired);
static void Timer_Destroy_Win32(struct Timer *);
#else //_MSC_VER
static bool Timer_Init_Posix(struct Timer *);
static void Timer_Destroy_Posix(struct Timer *);
static void Timer_Wrapper(union sigval val);
#endif

SO_PUBLIC struct Timer * 
Timer_Create(uint32_t interval, void (*handler)(void *), void *userData)
{
    struct Timer *ret;
    if ((ret = (struct Timer *)calloc(1,sizeof(struct Timer))) == NULL)
        return NULL;
    ret->function = handler;
    ret->userData = userData;
    ret->interval = interval;
#ifdef _MSC_VER
    if (!Timer_Init_Win32(ret))
#else //_MSC_VER
    if (!Timer_Init_Posix(ret))
#endif
    {
        Timer_Destroy(ret);
        return NULL;
    }
    return ret;
}
SO_PUBLIC void 
Timer_Destroy(struct Timer *timer)
{
#ifdef _MSC_VER
    Timer_Destroy_Win32(timer);
#else //_MSC_VER
    Timer_Destroy_Posix(timer);
#endif
    free(timer);
}


#ifdef _MSC_VER

static bool 
Timer_Init_Win32(struct Timer *timer)
{
	if ((timer->timerQueue = CreateTimerQueue()) == NULL)
		return false;
	if (!CreateTimerQueueTimer( &timer->timer, timer->timerQueue, 
			(WAITORTIMERCALLBACK)Timer_Wrapper, timer , (timer->interval * 1000), (timer->interval * 1000), 0))
		return false;

    return true;
}

static void 
Timer_Destroy_Win32(struct Timer *timer)
{
	DeleteTimerQueue(timer->timerQueue);
}

static void CALLBACK 
Timer_Wrapper(void * lpParam, bool TimerOrWaitFired)
{
	struct Timer *timer = (struct Timer *)lpParam;
	timer->function(timer->userData);
}

#else //_MSC_VER
static bool 
Timer_Init_Posix(struct Timer *timer)
{
    if ((timer->props = calloc(1,sizeof(struct sigevent))) == NULL)
        return false;

    timer->props->sigev_notify = SIGEV_THREAD;
    timer->props->sigev_value.sival_ptr = timer;
    timer->props->sigev_notify_function = Timer_Wrapper;
    timer->spec.it_value.tv_sec = timer->interval;
    timer->spec.it_value.tv_nsec = 1;
    timer->spec.it_interval.tv_sec = timer->interval;
    timer->spec.it_interval.tv_nsec = 1;

    if (timer_create (CLOCK_REALTIME, timer->props, &timer->timer) == -1)
    {
        rzb_perror
            ("Timer_Init_Posix: Failed call to timer_create: %s");
        return false;
    }
    if (timer_settime (timer->timer, 0, &timer->spec, NULL) == -1)
    {
        rzb_log (LOG_ERR, "%s: C&C Arm Hello Timer: Failed to arm timer.", __func__);
        return false;
    }

    return true;
}

static void 
Timer_Destroy_Posix(struct Timer *timer)
{
    timer_delete(timer->timer);
    free(timer->props);
}

static void 
Timer_Wrapper(union sigval val)
{
    struct Timer *timer = val.sival_ptr;
    timer->function(timer->userData);
}
#endif

