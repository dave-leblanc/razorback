#include "rzb_includes.h"
#include "rzb_smtp.h"
#include "rzb_core.h"
#include "spp_rzb-saac.h"

bool
SMTP_IsServer(uint16_t port)
{
    if (saac_eval_config->smtpPorts[port / 8] & (1 << (port % 8)))
        return true;

    return false;
}

int
SMTP_Inspect(SFSnortPacket *p)
{
    if ((SMTP_IsServer(p->src_port) && (p->flags & FLAG_FROM_SERVER)) ||
            (SMTP_IsServer(p->dst_port) && (p->flags & FLAG_FROM_CLIENT)))
        return STREAM_INSP_SMTP;

    return 0;
}

struct RZB_SMTP_Stream *
SMTP_GetNewSession(SFSnortPacket *p)
{
    struct RZB_SMTP_Stream *ssn;
    ssn = calloc(1, sizeof (struct RZB_SMTP_Stream));
    if (ssn == NULL)
    {
        DynamicPreprocessorFatalMessage("Failed to allocate SaaC SMTP session data\n");
    }

    if (p->flags & SSNFLAG_MIDSTREAM)
    {
        ssn->state = SMTP_STATE_UNKNOWN;
    }
    return ssn;
}

