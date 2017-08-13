/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2009 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

/*
 * priv.h file
 * This include file contains everything you shouldn't deal with in
 * user programs. Consider that anything in this file might change
 * without notice; libssh.h file will keep backward compatibility
 * on binary & source
 */

#ifndef _LIBSSH_PRIV_H
#define _LIBSSH_PRIV_H

#include "config.h"

#ifdef _WIN32

/* Imitate define of inttypes.h */
# ifndef PRIdS
#  define PRIdS "Id"
# endif

# ifndef PRIu64
#  if __WORDSIZE == 64
#   define PRIu64 "lu"
#  else
#   define PRIu64 "llu"
#  endif /* __WORDSIZE */
# endif /* PRIu64 */

# ifdef _MSC_VER
#  include <stdio.h>

/* On Microsoft compilers define inline to __inline on all others use inline */
#  undef inline
#  define inline __inline

#ifndef strcasecmp
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#endif
#  define strtoull _strtoui64
#  define isblank(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r')

#  define usleep(X) Sleep(((X)+1000)/1000)

#  undef strtok_r
#  define strtok_r strtok_s

#  if defined(HAVE__SNPRINTF_S)
#   undef snprintf
#   define snprintf(d, n, ...) _snprintf_s((d), (n), _TRUNCATE, __VA_ARGS__)
#  else /* HAVE__SNPRINTF_S */
#   if defined(HAVE__SNPRINTF)
#     undef snprintf
#     define snprintf _snprintf
#   else /* HAVE__SNPRINTF */
#    if !defined(HAVE_SNPRINTF)
#     error "no snprintf compatible function found"
#    endif /* HAVE_SNPRINTF */
#   endif /* HAVE__SNPRINTF */
#  endif /* HAVE__SNPRINTF_S */

#  if defined(HAVE__VSNPRINTF_S)
#   undef vsnprintf
#   define vsnprintf(s, n, f, v) _vsnprintf_s((s), (n), _TRUNCATE, (f), (v))
#  else /* HAVE__VSNPRINTF_S */
#   if defined(HAVE__VSNPRINTF)
#    undef vsnprintf
#    define vsnprintf _vsnprintf
#   else
#    if !defined(HAVE_VSNPRINTF)
#     error "No vsnprintf compatible function found"
#    endif /* HAVE_VSNPRINTF */
#   endif /* HAVE__VSNPRINTF */
#  endif /* HAVE__VSNPRINTF_S */

# endif /* _MSC_VER */

int gettimeofday(struct timeval *__p, void *__t);

#else /* _WIN32 */

#include <unistd.h>
#define PRIdS "zd"

#endif /* _WIN32 */

#include "libssh/libssh.h"
#include "libssh/callbacks.h"

/* some constants */
#define MAX_PACKET_LEN 262144
#define ERROR_BUFFERLEN 1024
#define CLIENTBANNER1 "SSH-1.5-libssh-" SSH_STRINGIFY(LIBSSH_VERSION)
#define CLIENTBANNER2 "SSH-2.0-libssh-" SSH_STRINGIFY(LIBSSH_VERSION)
#define KBDINT_MAX_PROMPT 256 /* more than openssh's :) */

#ifndef __FUNCTION__
#if defined(__SUNPRO_C)
#define __FUNCTION__ __func__
#endif
#endif

#define enter_function() (void)session
#define leave_function() (void)session

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/* forward declarations */
struct ssh_common_struct;
struct ssh_kex_struct;

int ssh_get_key_params(ssh_session session, ssh_key *privkey);

/* LOGGING */
#define SSH_LOG(session, priority, ...) \
    ssh_log_common(&session->common, priority, __FUNCTION__, __VA_ARGS__)
void ssh_log_common(struct ssh_common_struct *common,
                    int verbosity,
                    const char *function,
                    const char *format, ...) PRINTF_ATTRIBUTE(4, 5);
void ssh_log_function(int verbosity,
                      const char *function,
                      const char *buffer);


/* ERROR HANDLING */

/* error handling structure */
struct error_struct {
    int error_code;
    char error_buffer[ERROR_BUFFERLEN];
};

#define ssh_set_error(error, code, ...) \
    _ssh_set_error(error, code, __FUNCTION__, __VA_ARGS__)
void _ssh_set_error(void *error,
                    int code,
                    const char *function,
                    const char *descr, ...) PRINTF_ATTRIBUTE(4, 5);

#define ssh_set_error_oom(error) \
    _ssh_set_error_oom(error, __FUNCTION__)
void _ssh_set_error_oom(void *error, const char *function);

#define ssh_set_error_invalid(error) \
    _ssh_set_error_invalid(error, __FUNCTION__)
void _ssh_set_error_invalid(void *error, const char *function);





/* client.c */

int ssh_send_banner(ssh_session session, int is_server);

/* connect.c */
socket_t ssh_connect_host(ssh_session session, const char *host,const char
        *bind_addr, int port, long timeout, long usec);
socket_t ssh_connect_host_nonblocking(ssh_session session, const char *host,
		const char *bind_addr, int port);

/* in base64.c */
ssh_buffer base64_to_bin(const char *source);
unsigned char *bin_to_base64(const unsigned char *source, int len);

/* gzip.c */
int compress_buffer(ssh_session session,ssh_buffer buf);
int decompress_buffer(ssh_session session,ssh_buffer buf, size_t maxlen);

/* match.c */
int match_hostname(const char *host, const char *pattern, unsigned int len);




/** Free memory space */
#define SAFE_FREE(x) do { if ((x) != NULL) {free(x); x=NULL;} } while(0)

/** Zero a structure */
#define ZERO_STRUCT(x) memset((char *)&(x), 0, sizeof(x))

/** Zero a structure given a pointer to the structure */
#define ZERO_STRUCTP(x) do { if ((x) != NULL) memset((char *)(x), 0, sizeof(*(x))); } while(0)

/** Get the size of an array */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/** Overwrite the complete string with 'X' */
#define BURN_STRING(x) do { if ((x) != NULL) memset((x), 'X', strlen((x))); } while(0)

/**
 * This is a hack to fix warnings. The idea is to use this everywhere that we
 * get the "discarding const" warning by the compiler. That doesn't actually
 * fix the real issue, but marks the place and you can search the code for
 * discard_const.
 *
 * Please use this macro only when there is no other way to fix the warning.
 * We should use this function in only in a very few places.
 *
 * Also, please call this via the discard_const_p() macro interface, as that
 * makes the return type safe.
 */
#define discard_const(ptr) ((void *)((uintptr_t)(ptr)))

/**
 * Type-safe version of discard_const
 */
#define discard_const_p(type, ptr) ((type *)discard_const(ptr))

#endif /* _LIBSSH_PRIV_H */
/* vim: set ts=4 sw=4 et cindent: */
