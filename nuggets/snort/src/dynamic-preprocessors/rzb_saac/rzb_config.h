#ifndef _RZB_CONFIG_H
#define _RZB_CONFIG_H
#include "rzb_includes.h"

struct SaaC_Config
{
    uuid_t  nuggetId;
    bool    smtpEnable;
    int     smtpPortCount;
    char    smtpPorts[MAXPORTS_STORAGE];
    bool    httpEnable;
    int     httpPortCount;
    char    httpPorts[MAXPORTS_STORAGE];
    int     ref_count;
};


int ProcessConfig(struct SaaC_Config *config, char * args, char *ErrorString, int ErrStrLen);
void RZB_FreeConfig(struct SaaC_Config *config);
void RZB_FreeConfigs(tSfPolicyUserContextId config);

#endif
