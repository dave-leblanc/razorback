#ifndef RAZORBACK_MESSAGES_CNC_CORE_H
#define RAZORBACK_MESSAGES_CNC_CORE_H
#include <razorback/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

void Message_CnC_Bye_Init(void);
void Message_CnC_CacheClear_Init(void);
void Message_CnC_ConfigAck_Init(void);
void Message_CnC_ConfigUpdate_Init(void);
void Message_CnC_Error_Init(void);
void Message_CnC_Go_Init(void);
void Message_CnC_Hello_Init(void);
void Message_CnC_Pause_Init(void);
void Message_CnC_Paused_Init(void);
void Message_CnC_RegReq_Init(void);
void Message_CnC_RegResp_Init(void);
void Message_CnC_Running_Init(void);
void Message_CnC_Term_Init(void);
void Message_CnC_ReReg_Init(void);
#ifdef __cplusplus
}
#endif
#endif
