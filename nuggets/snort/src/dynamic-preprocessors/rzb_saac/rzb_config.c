#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "rzb_config.h"
#include "sfPolicy.h"
#include "sfPolicyUserData.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>


#define CONF_SEPARATORS " \t\n\r"

#define SMTP_ENABLE     "smtp_enable"
#define SMTP_PORTS      "smtp_ports"
#define HTTP_ENABLE     "http_enable"
#define HTTP_PORTS      "http_ports"

#define NUGGET_ID       "nugget_id"
#define DATA_TYPES      "datatypes"

/*
**  Port list delimiters
*/
#define START_PORT_LIST "{"
#define END_PORT_LIST   "}"


static int 
ProcessNuggetId(struct SaaC_Config *config, char *ErrorString, int ErrStrLen)
{
    char *pcToken;

    pcToken = strtok(NULL, CONF_SEPARATORS);
    if(!pcToken)
    {
        snprintf(ErrorString, ErrStrLen,
                      "Invalid port list format.");

        return -1;
    }
    if (uuid_parse (pcToken, config->nuggetId) != 0)
    {
        snprintf(ErrorString, ErrStrLen,
                      "Invalid Nugget ID UUID.");

        return -1;
    }
    return 0;
}

static int ProcessPorts(int *port_count, char *ports,
                        char *ErrorString, int ErrStrLen)
{
    char *pcToken;
    char *pcEnd;
    int  iPort;
    int  iEndPorts = 0;

    pcToken = strtok(NULL, CONF_SEPARATORS);
    if(!pcToken)
    {
        snprintf(ErrorString, ErrStrLen,
                      "Invalid port list format.");

        return -1;
    }

    if(strcmp(START_PORT_LIST, pcToken))
    {
        snprintf(ErrorString, ErrStrLen,
                      "Must start a port list with the '%s' token.",
                      START_PORT_LIST);

        return -1;
    }

    memset(ports, 0, MAXPORTS_STORAGE);

    while ((pcToken = strtok(NULL, CONF_SEPARATORS)) != NULL)
    {
        if(!strcmp(END_PORT_LIST, pcToken))
        {
            iEndPorts = 1;
            break;
        }

        iPort = strtol(pcToken, &pcEnd, 10);

        /*
        **  Validity check for port
        */
        if(*pcEnd)
        {
            snprintf(ErrorString, ErrStrLen, "Invalid port number.");

            return -1;
        }

        if(iPort < 0 || iPort > MAXPORTS-1)
        {
            snprintf(ErrorString, ErrStrLen,
                          "Invalid port number.  Must be between 0 and 65535.");

            return -1;
        }

        ports[iPort/8] |= (1 << (iPort % 8) );

        if(*port_count < MAXPORTS)
            (*port_count)++;
    }

    if(!iEndPorts)
    {
        snprintf(ErrorString, ErrStrLen,
                      "Must end port configuration with '%s'.",
                      END_PORT_LIST);

        return -1;
    }

    return 0;
}





int 
ProcessConfig(struct SaaC_Config *config, char * args, char *ErrorString, int ErrStrLen)
{
    char *itemToken;
    int ret;

    itemToken = strtok (args, CONF_SEPARATORS);
    while (itemToken)
    {
        if (!itemToken)
        {
            DynamicPreprocessorFatalMessage
                ("%s(%d)strtok returned NULL when it should not.\n",
                 __FILE__, __LINE__);
        }

        if (strcmp (HTTP_ENABLE, itemToken) == 0)
        {
            config->httpEnable=true;
        }
        else if (strcmp (HTTP_PORTS, itemToken) == 0)
        {
            ret = ProcessPorts(&config->httpPortCount, config->httpPorts, 
                    ErrorString, ErrStrLen);

            if (ret != 0)
                return ret;
        }
        else if (strcmp (SMTP_ENABLE, itemToken) == 0)
        {
            config->smtpEnable = true;
        }
        else if (strcmp (SMTP_PORTS, itemToken) == 0)
        {
            ret = ProcessPorts(&config->smtpPortCount, config->smtpPorts, 
                    ErrorString, ErrStrLen);

            if (ret != 0)
                return ret;
        }
        else if (strcmp (NUGGET_ID, itemToken) == 0)
        {
            ret = ProcessNuggetId(config, ErrorString, ErrStrLen);

            if (ret != 0)
                return ret;
        }
        else
        {
            DynamicPreprocessorFatalMessage
                ("%s(%d) Invalid arguments to RZB SaaC configuration: %s.\n",
                 *_dpd.config_file, *_dpd.config_line, itemToken);
        }
        itemToken = strtok (NULL, CONF_SEPARATORS);
    }
    return 0;
}

void
RZB_FreeConfig(struct SaaC_Config *config)
{
    free(config);
}


static int RZB_FreeConfigsPolicy(
        tSfPolicyUserContextId config,
        tSfPolicyId policyId,
        void* pData
        )
{
    struct SaaC_Config *pPolicyConfig = (struct SaaC_Config *)pData;

    //do any housekeeping before freeing RZBConfig
    sfPolicyUserDataClear (config, policyId);
    RZB_FreeConfig(pPolicyConfig);

    return 0;
}

void RZB_FreeConfigs(tSfPolicyUserContextId config)
{
    if (config == NULL)
        return;

    sfPolicyUserDataIterate (config, RZB_FreeConfigsPolicy);
    sfPolicyConfigDelete(config);
}


