#ifndef __RAZORD_H__
#define __RAZORD_H__

#include <razorback/api.h>
#include <razorback/config_file.h>
extern conf_int_t sg_maxThreads;
extern conf_int_t sg_maxQueue;
extern conf_int_t sg_idleTimeout;
extern conf_int_t sg_readTimeout;
extern conf_int_t sg_commandTimeout;
extern char *sg_badBlock;
extern conf_int_t mprintf_send_timeout;
extern conf_int_t sg_streamPortMin;
extern conf_int_t sg_streamPortMax;
extern bool sg_fixStale;
extern struct RazorbackContext *rzb_context;    // Defines behavior of nugget (collector)
#endif
