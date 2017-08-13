#ifndef SNMP_CONFIG_H
#define SNMP_CONFIG_H
#include "config.h"
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>

/* If you have sys/time.h on your machine: */
#include <sys/time.h>

/* Else pick one of these instead:
*    #include <time.h>
*       #include <sys/timeb.h>
*          */

/* if on windows: */
#ifdef WIN32
#include <winsock.h>
#endif

/* put a proper extern in front of certain functions */
#define NETSNMP_IMPORT extern

/* don't use inline constructs (define to inline if you want them) */
#define NETSNMP_INLINE inline
#define NETSNMP_STATIC_INLINE static inline

/* run man signal and see what the user function is supposed to return
*       to avoid warnings, or leave as "void" below if you don't care about
*             any warnings you might be seeing */
#define RETSIGTYPE void

/* tell the Net-SNMP core functionality headers that we've done the
*       minimal requirements */
#define NET_SNMP_CONFIG_H
#include <net-snmp/net-snmp-includes.h>

#endif
