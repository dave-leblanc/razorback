/** @file log.h
 * The Razorback Logging API.
 * 
 * Log levels avaliable are the same as the standard syslog log levels and
 * definitions are imported from syslog.
 *
 * Levels are:
 *      LOG_EMERGE
 *      LOG_ALERT
 *      LOG_CRIT
 *      LOG_ERR
 *      LOG_WARNING
 *      LOG_NOTICE
 *      LOG_INFO
 *      LOG_DEBUG
 *
 */
#ifndef RAZORBACK_LOG_H
#define RAZORBACK_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define LOG_EMERG 0
#define LOG_EMERGE 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#else //_MSC_VER
#include <syslog.h>
#endif

#include <razorback/visibility.h>
#include <razorback/types.h>


/** Log Destinations
 */
typedef enum
{
    RZB_LOG_DEST_FILE,          ///< Write to a file
    RZB_LOG_DEST_SYSLOG,        ///< Write to syslog
    RZB_LOG_DEST_ERR,           ///< Write to stderr
} RZB_LOG_DEST_t;

/** Log a message to the system message log
 * @param level The message level as defined in syslog.h
 * @param fmt The format string for the message
 * @param ... The data to be formatted.
 */
SO_PUBLIC extern void rzb_log (unsigned level, const char *fmt, ...);
SO_PUBLIC extern void
rzb_log_remote (uint8_t level, struct EventId *eventId, const char *fmt, ...);

/** Log a standard error.
 * @param message the log message associated with the error.
 */
SO_PUBLIC extern void rzb_perror (const char *message);

/** Get the currently configured log level
 * @return One of the log levels defined in syslog.h
 */
SO_PUBLIC extern int rzb_get_log_level ();

/** Get the currently configured log destination
 * @return The current log distination.
 */
SO_PUBLIC extern RZB_LOG_DEST_t rzb_get_log_dest ();

/** Set logging to debug mode.
 */
SO_PUBLIC extern void rzb_debug_logging ();
#ifdef __cplusplus
}
#endif
#endif // RAZORBACK_LOG_H
