#ifndef RAZORBACK_MESSAGES_CORE_H
#define RAZORBACK_MESSAGES_CORE_H
#include <razorback/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Message * Message_Create(uint32_t type, uint32_t version, size_t msgSize);
void Message_Destroy(struct Message *message);
bool Message_Setup(struct Message *message);

void MessageBlockSubmission_Init(void);
void MessageCacheReq_Init(void);
void MessageCacheResp_Init(void);
void MessageInspectionSubmission_Init(void);
void MessageJudgmentSubmission_Init(void);
void MessageLogSubmission_Init(void);
void MessageLogSubmission_Init(void);
void MessageAlertPrimary_Init(void);
void MessageAlertChild_Init(void);
void MessageOutputLog_Init(void);
void MessageOutputEvent_Init(void);

#ifdef __cplusplus
}
#endif

#endif
