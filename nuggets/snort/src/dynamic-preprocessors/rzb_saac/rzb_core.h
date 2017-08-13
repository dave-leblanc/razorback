#ifndef _RZB_CORE_H
#define _RZB_CORE_H
#include "rzb_includes.h"
#include "sfPolicyUserData.h"

#define STREAM_INSP_HTTP 1
#define STREAM_INSP_SMTP (1 << 1)

#define STATE_UNKNOWN          0

#define FLAG_NEXT_STATE_UNKNOWN 1
#define FLAG_GOT_NON_REBUILT    (1 << 1)

#define PKT_FROM_UNKNOWN 0
#define PKT_FROM_CLIENT 1
#define PKT_FROM_SERVER 2

void RZB_Demux(SFSnortPacket *p);
int RZB_GetPacketDirection(SFSnortPacket *p, bool (*isServer)(uint16_t));



struct RZB_Stream
{
    int state;
    int inspectors;
    bool reassembling;

    struct RZB_HTTP_Stream *http_ssn;
    struct RZB_SMTP_Stream *smtp_ssn;

    tSfPolicyId policy_id;
    tSfPolicyUserContextId config;
};

extern struct RZB_Stream *rzb_ssn;
#endif
