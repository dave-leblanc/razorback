#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#ifdef _MSC_VER
#include <varargs.h>
#include "bobins.h"
#endif

#include <razorback/debug.h>
#include <razorback/log.h>
#include <razorback/queue.h>
#include <razorback/thread.h>
#include <razorback/messages.h>
#include "runtime_config.h"


#include "init.h"

#define LOG_CONF_FILE "logging.conf"

static struct Queue *sg_logQueue = NULL;
static char level_strings[][9] = {
    "Debug",
    "Alert",
    "Critical",
    "Error",
    "Warning",
    "Notice",
    "Info",
    "Debug"
};


bool
configureLogging (void)
{
#ifdef _MSC_VER
#else //_MSC_VER
    if (Config_getLogDest() == RZB_LOG_DEST_SYSLOG)
        openlog (NULL, LOG_PID, Config_getLogFacility());
#endif //_MSC_VER

    if (( sg_logQueue = Queue_Create(LOG_QUEUE, QUEUE_FLAG_SEND, Razorback_Get_Message_Mode())) == NULL)
    {
        return false;
    }
    return true;
}

SO_PUBLIC void
rzb_perror (const char *fmt)
{
    rzb_log (LOG_ERR, fmt, strerror (errno));
}

SO_PUBLIC void
rzb_log (unsigned level, const char *fmt, ...)
{
    char *msg = NULL;
	va_list argp;
    RZB_LOG_DEST_t log_dest = Config_getLogDest();
    if (level > (unsigned) Config_getLogLevel())
    {
        return;
    }
    
    va_start (argp, fmt);
    
    if (log_dest != RZB_LOG_DEST_SYSLOG)
    {
        if (vasprintf (&msg, fmt, argp) == -1)
            return;
    }

    switch (log_dest)
    {
    case RZB_LOG_DEST_SYSLOG:
#ifdef _MSC_VER
		UNIMPLEMENTED();
#else
        vsyslog (level, fmt, argp);
#endif
        break;
    case RZB_LOG_DEST_FILE:
        break;
    case RZB_LOG_DEST_ERR:
    default:
        fprintf (stderr, "%s: %s\n", level_strings[level], msg);
        break;
    }
    va_end (argp);
    if (msg != NULL)
        free (msg);
}

SO_PUBLIC void
rzb_log_remote (uint8_t level, struct EventId *eventId, const char *fmt, ...)
{
    struct Message *message;
    char *msg = NULL;
    struct RazorbackContext *l_pContext;
	va_list argp;

    if (level > (unsigned) Config_getLogLevel())
    {
        return;
    }
    
    va_start (argp, fmt);

    if (vasprintf (&msg, fmt, argp) == -1)
        return;
    va_end (argp);
    
    l_pContext = Thread_GetCurrentContext ();

    if ((message = MessageLog_Initialize(l_pContext->uuidNuggetId,
            level, msg, eventId)) == NULL)
    {
        free (msg);
        return;
    }

    Queue_Put(sg_logQueue, message);
    message->destroy(message);
    free (msg);
}

SO_PUBLIC int
rzb_get_log_level ()
{
    return Config_getLogLevel();
}

SO_PUBLIC RZB_LOG_DEST_t
rzb_get_log_dest ()
{
    return Config_getLogDest();
}

SO_PUBLIC void
rzb_debug_logging ()
{
    Config_setLogLevel(LOG_DEBUG);
    Config_setLogDest(RZB_LOG_DEST_ERR);
}


