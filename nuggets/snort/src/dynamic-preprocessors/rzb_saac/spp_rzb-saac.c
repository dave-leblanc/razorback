/*
** Copyright (C) 2005-2009 Sourcefire, Inc.
** Copyright (C) 1998-2005 Martin Roesch <roesch@sourcefire.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, RZBston, MA 02111-1307, USA.
*/

/* $Id$ */
/* Snort Preprocessor Plugin Source File RZB */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "preprocids.h"
#include "sf_snort_packet.h"
#include "sf_dynamic_preproc_lib.h"
#include "sf_dynamic_preprocessor.h"
#include "snort_debug.h"
#include "rzb_debug.h"
#include "sf_preproc_info.h"

#define CONF_SEPARATORS " \t\n\r"
#define RZB_CONF "rzb_conf"
#define RZB_EXTS "exts"

#define ERRSTRLEN 1000

#include "spp_rzb-saac.h"
#include "rzb_core.h"
#include "rzb_config.h"

#include <razorback/uuids.h>
#include <razorback/api.h>
#include <razorback/log.h>

#define RZB_COLLECT_DISP_GID 3535
#define RZB_COLLECT_DISP_SID 3535
#define RZB_COLLECT_DISP_MESG "Bad file found"
UUID_DEFINE(SAAC, 0xa3, 0xb1, 0x24, 0xf9, 0xee, 0x08, 0x5f, 0xdf, 0x8f, 0x3e, 0x5e, 0x8d, 0xdd, 0xe0, 0x83, 0xd5);

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 0;
const int BUILD_VERSION = 1;

const char *PREPROC_NAME = "SF_RZB_SaaC_Preprocessor";

tSfPolicyUserContextId saac_config = NULL;
struct SaaC_Config *saac_eval_config = NULL;

struct RazorbackContext *g_pContext = NULL;
void *dlHandle = NULL;          // For the API library

static void RZBCleanExit (int, void *);
static void RZBProcess (void *, void *);

/* list of function prototypes for this preprocessor */
static void RZBInit (char *);

#ifdef SNORT_RELOAD
static void RZBReload (char *);
static void *RZBReloadSwap (void);
static void RZBReloadSwapFree (void *);
#endif

static void _addPortsToStream5Filter(struct SaaC_Config *, tSfPolicyId);

extern char *maxToken;

static void
RZBCleanExit (int signal, void *unused)
{
    Razorback_Shutdown_Context(g_pContext);
}

#ifdef SNORT_RELOAD
static void
RZBReload (char *args)
{
    DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Razorback SaaC RZBReload() not implemented\n"));
}

static void *
RZBReloadSwap (void)
{
    DEBUG_WRAP(DebugMessage(DEBUG_RZB, "Razorback SaaC RZBReloadSwap() not implemented\n"));
    return NULL;
}

static void
RZBReloadSwapFree (void *data)
{
    DEBUG_WRAP(DebugMessage(DEBUG_RZB,"Razorback SaaC RZBReloadSwapFree() not implemented\n"));
}
#endif


void
RZBProcess (void *p, void *context)
{
    tSfPolicyId policy_id = _dpd.getRuntimePolicy();
    SFSnortPacket *sp = (SFSnortPacket *) p;

    if ((sp->payload_size == 0) || !IsTCP(sp) || (sp->payload == NULL))
        return;

    sfPolicyUserPolicySet (saac_config, policy_id);
    
    RZB_Demux(p);
    return;
}

void 
initContext(void *conf)
{
    struct SaaC_Config * pPolicyConfig = (struct SaaC_Config *)conf;
    uuid_t l_uuidNuggetType;
    // Register this collector
    uuid_copy(l_uuidNuggetType, SAAC);

    if (g_pContext == NULL && pPolicyConfig != NULL)
    {
        g_pContext =
            Razorback_Init_Collection_Context (pPolicyConfig->nuggetId,
                                               l_uuidNuggetType);
    }
}

static void
RZBInit (char *args)
{
    char ErrorString[ERRSTRLEN];
    int iErrStrLen = ERRSTRLEN;

    tSfPolicyId policy_id = _dpd.getParserPolicy();
    struct SaaC_Config * pPolicyConfig = NULL;

    if ((args == NULL) || (strlen (args) == 0))
    {
        DynamicPreprocessorFatalMessage
            ("%s(%d) No arguments to RZB SaaC configuration.\n",
             *_dpd.config_file, *_dpd.config_line);
    }
    if (saac_config == NULL)
    {
        //create a context
        saac_config = sfPolicyConfigCreate();
        if (saac_config == NULL)
        {
            DynamicPreprocessorFatalMessage("Not enough memory to create SaaC "
                                            "configuration.\n");
        }
        /* Put the preprocessor function into the function list */
        _dpd.addPreprocExit (RZBCleanExit, NULL, PRIORITY_LAST, PP_SAAC);
    }

    sfPolicyUserPolicySet (saac_config, policy_id);
    pPolicyConfig = (struct SaaC_Config *)sfPolicyUserDataGetCurrent(saac_config);

    if (pPolicyConfig != NULL)
    {
        DynamicPreprocessorFatalMessage("Can only configure SaaC preprocessor once.\n");
    }

    pPolicyConfig = (struct SaaC_Config *)calloc(1, sizeof(struct SaaC_Config));
    if (pPolicyConfig == NULL)
    {
        DynamicPreprocessorFatalMessage("Not enough memory to create SaaC "
                                        "configuration.\n");
    }

    sfPolicyUserDataSetCurrent(saac_config, pPolicyConfig);


    if (ProcessConfig(pPolicyConfig, args, ErrorString, iErrStrLen) != 0)
    {
        DynamicPreprocessorFatalMessage
            ("Failed to parse RZB SaaC Config\n");
    }

    if (uuid_is_null(pPolicyConfig->nuggetId) == 1)
    {
        DynamicPreprocessorFatalMessage
            ("No nugget id present in RZB SaaC Config\n");
    }

    if(!(pPolicyConfig->httpEnable || pPolicyConfig->smtpEnable))
        return;

    _dpd.addPreproc (RZBProcess, PRIORITY_TUNNEL, PP_SAAC,
                     PROTO_BIT__TCP);

    if (_dpd.streamAPI == NULL)
    {
        DynamicPreprocessorFatalMessage("Streaming & reassembly must be enabled "
                "for SaaC preprocessor\n");
    }


    _addPortsToStream5Filter(pPolicyConfig, policy_id);
    _dpd.addPostConfigFunc(initContext, pPolicyConfig);

}

void
DYNAMIC_PREPROC_SETUP(void)
{
    /* link the preprocessor keyword to the init function in
       the preproc list */
#ifndef SNORT_RELOAD
    _dpd.registerPreproc ("rzb", RZBInit);
#else
    _dpd.registerPreproc ("rzb", RZBInit, RZBReload, RZBReloadSwap,
                          RZBReloadSwapFree);
#endif
}



static void _addPortsToStream5Filter(struct SaaC_Config *config, tSfPolicyId policy_id)
{
    unsigned int portNum;

    if (config == NULL)
        return;

    if (config->httpEnable)
    {
        for (portNum = 0; portNum < MAXPORTS; portNum++)
        {
            if(config->httpPorts[(portNum/8)] & (1<<(portNum%8)))
            {
                //Add port the port
                _dpd.streamAPI->set_port_filter_status(IPPROTO_TCP, (uint16_t)portNum,
                                                       PORT_MONITOR_SESSION, policy_id, 1);
            }
        }
    }
    if (config->smtpEnable)
    {
        for (portNum = 0; portNum < MAXPORTS; portNum++)
        {
            if(config->smtpPorts[(portNum/8)] & (1<<(portNum%8)))
            {
                //Add port the port
                _dpd.streamAPI->set_port_filter_status(IPPROTO_TCP, (uint16_t)portNum,
                                                       PORT_MONITOR_SESSION, policy_id, 1);
            }
        }
    }
}


