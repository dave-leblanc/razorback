#ifndef	RAZORBACK_API_INTERNAL_H
#define	RAZORBACK_API_INTERNAL_h

#include <razorback/types.h>
#ifdef __cplusplus
extern "C" {
#endif

extern bool Razorback_ForEach_Context (int (*function) (struct RazorbackContext *, void *), void *userData);

void Razorback_Destroy_Context(struct RazorbackContext *context);
int Kill_Output_Thread(void *ut, void *ud);

#ifdef __cplusplus
}
#endif
#endif
