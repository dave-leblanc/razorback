#ifndef RZB_SMTP_H
#define RZB_SMTP_H
#include "rzb_includes.h"

#define SMTP_STATE_UNKNOWN 0

struct RZB_SMTP_Stream
{
    int state;
    int session_flags;
};

bool SMTP_IsServer(uint16_t port);
int SMTP_Inspect(SFSnortPacket *p);
struct RZB_SMTP_Stream * SMTP_GetNewSession(SFSnortPacket *);
#endif
