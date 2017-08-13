#ifndef RAZORBACK_DISPATCHER_H
#define RAZORBACK_DISPATCHER_H
#include <razorback/config_file.h>
#include "statistics.h"

extern pthread_mutex_t contextLock;
extern struct RazorbackContext sg_rbContext;

extern struct ThreadPool *g_cacheThreadPool;
extern struct ThreadPool *g_judgmentThreadPool;
extern struct ThreadPool *g_logThreadPool;
extern struct ThreadPool *g_submissionThreadPool;

extern struct Queue *g_ccWrite;

extern bool Dispatcher_Terminate (void);

/** Defines for ConnectedEntityEntry.iState
 */
#define NUGGET_STATE_UNKNOWN		  	0x00000000
#define NUGGET_STATE_REQUESTEDREGISTER  0x00000001
#define NUGGET_STATE_REGISTERING        0x00000002
#define NUGGET_STATE_CONFIGURING        0x00000004
#define NUGGET_STATE_PAUSED             0x00000008
#define NUGGET_STATE_RUNNING            0x0000000a
#define NUGGET_STATE_TERMINATED         0x0000000b
#define NUGGET_STATE_ERROR              0x0000000c
#define NUGGET_STATE_REREG				0x0000000d

#define NUGGET_STATE_DESC_REQUESTEDREGISTER    "Reg Request"
#define NUGGET_STATE_DESC_REGISTERING          "Registering"
#define NUGGET_STATE_DESC_CONFIGURING          "Configuring"
#define NUGGET_STATE_DESC_PAUSED               "Paused"
#define NUGGET_STATE_DESC_RUNNING              "Running"
#define NUGGET_STATE_DESC_TERMINATED           "Terminated"
#define NUGGET_STATE_DESC_ERROR                "Error"
#define NUGGET_STATE_DESC_REREG                "Re-Registering"

bool updateDispatcherState(void);

#endif //RAZORBACK_DISPATCHER_H
