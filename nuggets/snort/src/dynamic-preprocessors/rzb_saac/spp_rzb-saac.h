#ifndef SPP_RZB_SAAC_H
#define SPP_RZB_SAAC_H
#include "rzb_includes.h"

#include "preprocids.h"
#include "sf_dynamic_preproc_lib.h"
#include "sf_dynamic_preprocessor.h"

#include "sfPolicy.h"
#include "sfPolicyUserData.h"
#include "sf_preproc_info.h"

#include "rzb_config.h"
#include <razorback/api.h>

#define PP_SAAC 6868


extern tSfPolicyUserContextId saac_config;

extern struct SaaC_Config *saac_eval_config;
extern struct RazorbackContext *g_pContext;

#endif
