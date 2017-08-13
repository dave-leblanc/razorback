#include "rzb_core.h"
#include "spp_rzb-saac.h"
#include "rzb_http.h"
#include "rzb_smtp.h"
#include "rzb_debug.h"
#include "sfPolicyUserData.h"

struct RZB_Stream *rzb_ssn = NULL;



static void 
RZB_SessionFree(void *session_data)
{
    struct RZB_Stream *rzb = (struct RZB_Stream *)session_data;
#ifdef SNORT_RELOAD
    struct SaaC_Config *pPolicyConfig = NULL;
#endif

    if (rzb == NULL)
        return;

#ifdef SNORT_RELOAD
    pPolicyConfig = (struct SaaC_Config *)sfPolicyUserDataGet(rzb->config, rzb->policy_id);

    if (pPolicyConfig != NULL)
    {
        pPolicyConfig->ref_count--;
        if ((pPolicyConfig->ref_count == 0) &&
            (rzb->config != saac_config))
        {
            sfPolicyUserDataClear (rzb->config, rzb->policy_id);
            RZB_FreeConfig(pPolicyConfig);

            /* No more outstanding policies for this config */
            if (sfPolicyUserPolicyGetActive(rzb->config) == 0)
                RZB_FreeConfigs(rzb->config);
        }
    }
#endif
    if (rzb->inspectors & STREAM_INSP_HTTP)
        HTTP_SessionFree(rzb->http_ssn);

    free(rzb);
}

static struct RZB_Stream *
RZB_GetNewSession(SFSnortPacket *p, tSfPolicyId policy_id, int inspectors)
{
    struct RZB_Stream *ssn;
    struct SaaC_Config *pPolicyConfig = NULL;
    pPolicyConfig = (struct SaaC_Config *)sfPolicyUserDataGetCurrent(saac_config);
    ssn = calloc(1, sizeof(struct RZB_Stream));
    if (ssn == NULL)
    {
        DynamicPreprocessorFatalMessage("Failed to allocate SaaC session data\n");
        return NULL;
    }

    rzb_ssn = ssn;
    ssn->inspectors = inspectors;
    if (ssn->inspectors & STREAM_INSP_HTTP) 
    {
        ssn->http_ssn = HTTP_GetNewSession(p);
    }
    else if (ssn->inspectors & STREAM_INSP_SMTP)
    {
        ssn->smtp_ssn = SMTP_GetNewSession(p);
    }
    _dpd.streamAPI->set_application_data(p->stream_session_ptr, PP_SAAC,
                                         ssn, &RZB_SessionFree);

    if (p->stream_session_ptr != NULL)
    {
        /* check to see if we're doing client reassembly in stream */
        if (_dpd.streamAPI->get_reassembly_direction(p->stream_session_ptr) & SSN_DIR_CLIENT)
            ssn->reassembling = true;

        if(!ssn->reassembling)
        {
            _dpd.streamAPI->set_reassembly(p->stream_session_ptr,
                    STREAM_FLPOLICY_FOOTPRINT, SSN_DIR_CLIENT, STREAM_FLPOLICY_SET_ABSOLUTE);
            ssn->reassembling = true;
        }
    }

    ssn->policy_id = policy_id;
    ssn->config = saac_config;
    pPolicyConfig->ref_count++;
    return ssn;
}

int 
RZB_GetPacketDirection(SFSnortPacket *p, bool (*isServer)(uint16_t))
{
    int flags = 0;
    int pkt_direction = PKT_FROM_UNKNOWN;
    if (p->stream_session_ptr != NULL)
    {
        /* set flags to session flags */
        flags = _dpd.streamAPI->get_session_flags(p->stream_session_ptr);
    }

    if (flags & SSNFLAG_MIDSTREAM)
    {
        if (isServer(p->src_port) &&
            !isServer(p->dst_port))
        {
            pkt_direction = PKT_FROM_SERVER;
        }
        else if (!isServer(p->src_port) &&
                 isServer(p->dst_port))
        {
            pkt_direction = PKT_FROM_CLIENT;
        }
    }
    else
    {
        if (p->flags & FLAG_FROM_SERVER)
        {
            pkt_direction = PKT_FROM_SERVER;
        }
        else if (p->flags & FLAG_FROM_CLIENT)
        {
            pkt_direction = PKT_FROM_CLIENT;
        }

        /* if direction is still unknown ... */
        if (pkt_direction == PKT_FROM_UNKNOWN)
        {
            if (isServer(p->src_port) &&
                !isServer(p->dst_port))
            {
                pkt_direction = PKT_FROM_SERVER;
            }
            else if (!isServer(p->src_port) &&
                     isServer(p->dst_port))
            {
                pkt_direction = PKT_FROM_CLIENT;
            }
        }
    }

    return pkt_direction;
}

static int 
RZB_Setup(SFSnortPacket *p, int *ssn_state, int *ssn_flags, bool (*isServer)(uint16_t))
{
    int flags = 0;
    int pkt_dir;
    int missing_in_rebuilt;

    if (p->stream_session_ptr != NULL)
    {
        /* set flags to session flags */
        flags = _dpd.streamAPI->get_session_flags(p->stream_session_ptr);
    }

    /* Figure out direction of packet */
    pkt_dir = RZB_GetPacketDirection(p, isServer);

    /* Check to see if there is a reassembly gap.  If so, we won't know
     * what state we're in when we get the _next_ reassembled packet */
    if ((pkt_dir != PKT_FROM_SERVER) &&
        (p->flags & FLAG_REBUILT_STREAM))
    {
        missing_in_rebuilt =
            _dpd.streamAPI->missing_in_reassembled(p->stream_session_ptr, SSN_DIR_CLIENT);

        if ((*ssn_flags) & FLAG_NEXT_STATE_UNKNOWN)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Found gap in previous reassembly buffer - "
                                    "set state to unknown\n"););
            *ssn_state = STATE_UNKNOWN;
            *ssn_flags &= ~FLAG_NEXT_STATE_UNKNOWN;
        }

        if (missing_in_rebuilt == SSN_MISSING_BOTH)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Found missing packets before and after "
                                    "in reassembly buffer - set state to unknown and "
                                    "next state to unknown\n"););
            *ssn_state = STATE_UNKNOWN;
            *ssn_flags |= FLAG_NEXT_STATE_UNKNOWN;
        }
        else if (missing_in_rebuilt == SSN_MISSING_BEFORE)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Found missing packets before "
                                    "in reassembly buffer - set state to unknown\n"););
            *ssn_state = STATE_UNKNOWN;
        }
        else if (missing_in_rebuilt == SSN_MISSING_AFTER)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Found missing packets after "
                                    "in reassembly buffer - set next state to unknown\n"););
            *ssn_flags |= FLAG_NEXT_STATE_UNKNOWN;
        }
    }

    return pkt_dir;
}
static void 
RZB_ProcessPacket(SFSnortPacket *p, int pkt_dir, 
        int *state, int *session_flags,
        int (*processClientPacket)(SFSnortPacket *), 
        int (*processServerPacket)(SFSnortPacket *))
{
    if (*state == STATE_UNKNOWN)
        return;

    if (pkt_dir == PKT_FROM_CLIENT)
    {
        processClientPacket(p);
    }
    else
    {
        if (p->flags & FLAG_STREAM_INSERT)
        {
            /* Packet will be rebuilt, so wait for it */
            return;
        }
        else if (rzb_ssn->reassembling && !(p->flags & FLAG_REBUILT_STREAM))
        {
            /* If this isn't a reassembled packet and didn't get
             * inserted into reassembly buffer, there could be a
             * problem.  If we miss syn or syn-ack that had window
             * scaling this packet might not have gotten inserted
             * into reassembly buffer because it fell outside of
             * window, because we aren't scaling it */
            *session_flags |= FLAG_GOT_NON_REBUILT;
            *state = STATE_UNKNOWN;
        }
        else if (rzb_ssn->reassembling && (*session_flags & FLAG_GOT_NON_REBUILT))
        {
            /* This is a rebuilt packet.  If we got previous packets
             * that were not rebuilt, state is going to be messed up
             * so set state to unknown. It's likely this was the
             * beginning of the conversation so reset state */
            *state = STATE_UNKNOWN;
            *session_flags &= ~FLAG_GOT_NON_REBUILT;
        }
        /* Process as a server packet */
        processServerPacket(p);
    }
}

void 
RZB_Demux(SFSnortPacket *p)
{
    tSfPolicyId policy_id = _dpd.getRuntimePolicy();
    int inspectors =0;
    int pkt_dir;

    rzb_ssn = (struct RZB_Stream *)_dpd.streamAPI->get_application_data(p->stream_session_ptr, PP_SAAC);
    if (rzb_ssn != NULL)
        saac_eval_config = (struct SaaC_Config *)sfPolicyUserDataGet(rzb_ssn->config, rzb_ssn->policy_id);
    else
        saac_eval_config = (struct SaaC_Config *)sfPolicyUserDataGetCurrent(saac_config);

    if (saac_eval_config == NULL)
        return;

    if (rzb_ssn == NULL)
    {
        if (saac_eval_config->httpEnable)
            inspectors |= HTTP_Inspect(p);

        if (saac_eval_config->smtpEnable)
            inspectors |= SMTP_Inspect(p);
            

        if (inspectors == 0)
            return;
        rzb_ssn = RZB_GetNewSession(p, policy_id, inspectors);
    }
    if (rzb_ssn->inspectors & STREAM_INSP_HTTP)
    {
        pkt_dir = RZB_Setup(p, &rzb_ssn->http_ssn->state, &rzb_ssn->http_ssn->session_flags, HTTP_IsServer);
        RZB_ProcessPacket(p, pkt_dir, 
                &rzb_ssn->http_ssn->state, 
                &rzb_ssn->http_ssn->session_flags, 
                HTTP_ProcessFromClient, 
                HTTP_ProcessFromServer);
    }
    if (rzb_ssn->inspectors & STREAM_INSP_SMTP)
    {
        pkt_dir = RZB_Setup(p, &rzb_ssn->smtp_ssn->state, &rzb_ssn->smtp_ssn->session_flags, SMTP_IsServer);
    }

}




