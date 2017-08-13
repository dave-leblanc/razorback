#ifndef RAZORBACK_COMMAND_AND_CONTROL_H
#define RAZORBACK_COMMAND_AND_CONTROL_H
#include <razorback/types.h>
#include <razorback/api.h>
#include <razorback/lock.h>
#ifdef __cplusplus
extern "C" {
#endif
extern bool CommandAndControl_Start (struct RazorbackContext *p_pContext);
extern void CommandAndControl_Shutdown(void);

extern bool CommandAndControl_SendBye (struct RazorbackContext *context);
extern struct Mutex * sg_mPauseLock;

extern void CommandAndControl_Pause();
extern void CommandAndControl_Unpause();

#ifdef __cplusplus
}
#endif
#endif
