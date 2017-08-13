/** @file daemon.h
 * Razorback deamon helper.
 */
#ifndef __RAZORBACK_DAEMON_H__
#define __RAZORBACK_DAEMON_H__

#include <razorback/visibility.h>
#include <razorback/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Daemonize the application.
 * @return true on successful daemonisation false on error.
 */
SO_PUBLIC extern bool rzb_daemonize (void (*sighandler) (int), const char *pidFile);
#ifdef __cplusplus
}
#endif
#endif /* __RAZORBACK_DAEMON_H__ */
