/*
 *  Copyright (C) 2007-2009 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifndef _WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#include <signal.h>
#include <errno.h>

#if defined(USE_SYSLOG) && !defined(C_AIX)
#include <syslog.h>
#endif

#ifdef C_LINUX
#include <sys/resource.h>
#endif

#include <getopt.h>

#include <razorback/types.h>
#include <razorback/config_file.h>
#include <razorback/log.h>
#include <razorback/daemon.h>
#include <razorback/socket.h>
#include <razorback/api.h>

#include "server.h"
#include "others.h"
#include "shared.h"
#include "scanner.h"
#include "tcpserver.h"
#include "localserver.h"

#define RAZORD_CONF "razord.conf"
UUID_DEFINE(RAZORD, 0x51, 0x25, 0xad, 0x5b, 0x65, 0x60, 0x4f, 0xda, 0xb9, 0x97, 0xc0, 0xd9, 0xff, 0xcb, 0x06, 0xe1);
struct RazorbackContext *rzb_context = NULL;

void termination_handler (int signum);

short debug_mode = 0;

static void
help (void)
{
    printf ("\n");
    printf ("                      Razorback Collection Daemon\n");
    printf ("           By The Sourcefire VRT: http://labs.snort.org/\n");
    printf ("           (C) 2007-2012 Sourcefire, Inc.\n\n");

    printf ("    --help                   -h             Show this help.\n");
    printf
        ("    --version                -V             Show version number.\n");
    printf
        ("    --debug                                 Enable debug mode.\n");
}

// Definitions
#define FILEINJECT_CONFIG "fileInject.conf"
static uuid_t sg_uuidNuggetId;
static bool logg_lock = false;
static bool logg_time = false;
bool logok = false;

static char *sg_pidFile  = NULL;
static char *sg_user  = NULL;
static bool sg_allowGroups = false;
static bool tcpsock = false;
static char *sg_listenAddress = NULL;
static conf_int_t sg_listenPort = 3310;
static bool localsock = false;
static char *sg_unixPath = NULL;
static char *sg_unixGroup = NULL;
static conf_int_t sg_unixPerms = 0777;
conf_int_t mprintf_send_timeout = 500;
conf_int_t sg_streamPortMin = 1024;
conf_int_t sg_streamPortMax = 65535;
conf_int_t sg_maxThreads = 10;
conf_int_t sg_maxQueue = 100;
conf_int_t sg_idleTimeout = 30;
conf_int_t sg_readTimeout = 120;
conf_int_t sg_commandTimeout = 5;
bool sg_fixStale = true;



RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {"PidFile", RZB_CONF_KEY_TYPE_STRING, &sg_pidFile, NULL},
    {"Log.FileUnlock", RZB_CONF_KEY_TYPE_BOOL, &logg_lock, NULL},
    {"Log.Time", RZB_CONF_KEY_TYPE_BOOL, &logg_time, NULL},
    {"Log.Clean", RZB_CONF_KEY_TYPE_BOOL, &logok, NULL},
    {"User", RZB_CONF_KEY_TYPE_STRING, &sg_user, NULL},
    {"AllowSupplementaryGroups", RZB_CONF_KEY_TYPE_BOOL, &sg_allowGroups, NULL},
    {"TCPSocket.Enabled", RZB_CONF_KEY_TYPE_BOOL, &tcpsock, NULL},
    {"TCPSocket.Address", RZB_CONF_KEY_TYPE_STRING, &sg_listenAddress, NULL},
    {"TCPSocket.Port", RZB_CONF_KEY_TYPE_INT, &sg_listenPort, NULL},
    {"LocalSocket.Enabled", RZB_CONF_KEY_TYPE_BOOL, &localsock, NULL},
    {"LocalSocket.Path", RZB_CONF_KEY_TYPE_STRING, &sg_unixPath, NULL},
    {"LocalSocket.Group", RZB_CONF_KEY_TYPE_STRING, &sg_unixGroup, NULL},
    {"LocalSocket.Mode", RZB_CONF_KEY_TYPE_INT, &sg_unixPerms, NULL},
    {"LocalSocket.FixStale", RZB_CONF_KEY_TYPE_BOOL, &sg_fixStale, NULL},

    {"Stream.PortMin", RZB_CONF_KEY_TYPE_INT, &sg_streamPortMin, NULL},
    {"Stream.PortMax", RZB_CONF_KEY_TYPE_INT, &sg_streamPortMax, NULL},
    {"MaxThreads", RZB_CONF_KEY_TYPE_INT, &sg_maxThreads, NULL},
    {"MaxQueue", RZB_CONF_KEY_TYPE_INT, &sg_maxQueue, NULL},
    {"Timeout.Idle", RZB_CONF_KEY_TYPE_INT, &sg_idleTimeout, NULL},
    {"Timeout.Read", RZB_CONF_KEY_TYPE_INT, &sg_readTimeout, NULL},
    {"Timeout.Command", RZB_CONF_KEY_TYPE_INT, &sg_commandTimeout, NULL},
    {"Timeout.SendBuf", RZB_CONF_KEY_TYPE_INT, &mprintf_send_timeout, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

static struct option sg_options[] = 
{
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"debug", no_argument, NULL, 'd'},
    {NULL, 0, NULL, 0}
};

int
main (int argc, char **argv)
{
    uuid_t l_uuidNuggetType; // The UUID defining the kind of nugget (collector)
    int optId;
    int option_index = 0;

#ifndef	_WIN32
    struct passwd *user = NULL;
    struct rlimit rlim;
#endif
    int i;
    int lsockets[2];
    int nlsockets = 0;
#ifdef C_BSD
    int ret;
#endif
    while (1)
    {
        optId = getopt_long(argc, argv, "hVd", sg_options, &option_index);
        if (optId == -1)
            break;

        switch (optId)
        {
            case 'd':
#ifdef C_LINUX
                rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
                if (setrlimit (RLIMIT_CORE, &rlim) < 0)
                    perror ("setrlimit");
#endif
                debug_mode = 1;
                break;
            case 'V':
//                print_version ();
                return 0;
            case 'h':
                help();
                return 0;
            default:
                break;

        }
    }

    if (!readMyConfig (NULL, RAZORD_CONF, sg_Config))
	{
		rzb_log(LOG_ERR, "%s: Failed to read configuation", __func__);
		return -1;
	}

    /* drop privileges */
#ifndef _WIN32
    if (geteuid () == 0 && sg_user != NULL)
    {
        if ((user = getpwnam (sg_user)) == NULL)
        {
            rzb_log (LOG_ERR, "Can't get information about user %s.",
                     sg_user);
            return 1;
        }
        if (sg_allowGroups)
        {
#ifdef HAVE_INITGROUPS
            if (initgroups (sg_user, user->pw_gid))
            {
                rzb_log (LOG_ERR, "initgroups() failed.");
                return 1;
            }
#else
            rzb_log(LOG_ERR, 
                "AllowSupplementaryGroups: initgroups() is not available, please disable AllowSupplementaryGroups");
            return 1;
#endif
        }
        else
        {
#ifdef HAVE_SETGROUPS
            if (setgroups (1, &user->pw_gid))
            {
                rzb_log (LOG_ERR, "setgroups() failed.");
                return 1;
            }
#endif
        }
        if (setgid (user->pw_gid))
        {
            rzb_log (LOG_ERR, "setgid(%d) failed.",
                     (int) user->pw_gid);
            return 1;
        }

        if (setuid (user->pw_uid))
        {
            rzb_log (LOG_ERR, "setuid(%d) failed.",
                     (int) user->pw_uid);
            return 1;
        }
    }
#endif

    if (!tcpsock && !localsock)
    {
        rzb_log (LOG_ERR, "Please define server type (local and/or TCP).");
        return 1;

    }

#ifndef _WIN32
    if (user)
        rzb_log (LOG_INFO, "Running as user %s (UID %u, GID %u)", user->pw_name,
              user->pw_uid, user->pw_gid);
#endif

#if defined(RLIMIT_DATA) && defined(C_BSD)
    if (getrlimit (RLIMIT_DATA, &rlim) == 0)
    {
        /* bb #1941.
         * On 32-bit FreeBSD if you set ulimit -d to >2GB then mmap() will fail
         * too soon (after ~120 MB).
         * Set limit lower than 2G if on 32-bit */
        uint64_t lim = rlim.rlim_cur;
        if (sizeof (void *) == 4 && lim > (1ULL << 31))
        {
            rlim.rlim_cur = 1ULL << 31;
            if (setrlimit (RLIMIT_DATA, &rlim) < 0)
                rzb_log (LOG_ERR, "setrlimit(RLIMIT_DATA) failed: %s",
                      strerror (errno));
            else
                rzb_log (LOG_INFO, "Running on 32-bit system, and RLIMIT_DATA > 2GB, lowering to 2GB!");
        }
    }
#endif


    if (sg_streamPortMin < 1024 || sg_streamPortMin > sg_streamPortMax || sg_streamPortMax > 65535)
    {
        rzb_log (LOG_ERR, "Invalid StreamPortMin/StreamPortMax: %d, %d", sg_streamPortMin,
              sg_streamPortMax);
        return 1;
    }

    if (tcpsock)
    {
        rzb_log(LOG_INFO, "TCP Socket %s:%d", sg_listenAddress, (uint16_t)sg_listenPort);
        if ((lsockets[nlsockets] = tcpserver (sg_listenAddress, (uint16_t)sg_listenPort)) == -1)
        {
            return 1;
        }
        nlsockets++;
    }
#ifndef _WIN32
    if (localsock)
    {
        mode_t umsk = umask (0777);  /* socket is created with 000 to avoid races */
        if ((lsockets[nlsockets] = localserver (sg_unixPath)) == -1)
        {
            umask (umsk);
            return 1;
        }
        umask (umsk);           /* restore umask */
        if (sg_unixGroup != NULL)
        {
            struct group *pgrp = getgrnam (sg_unixGroup);
            if (!pgrp)
            {
                rzb_log (LOG_ERR, "Unknown group %s", sg_unixGroup);
                return 1;
            }
            if (chown (sg_unixPath, -1, pgrp->gr_gid))
            {
                rzb_log (LOG_ERR, "Failed to change socket ownership to group %s",
                      sg_unixGroup);
                return 1;
            }
        }

        if (chmod (sg_unixPath, sg_unixPerms & 0666))
        {
            rzb_log (LOG_ERR, "Cannot set socket permission to %d", sg_unixPerms);
            return 1;
        }

        nlsockets++;
    }

    /* fork into background */
    if (debug_mode == 0)
    {
#ifdef C_BSD
        /* workaround for OpenBSD bug, see https://wwws.clamav.net/bugzilla/show_bug.cgi?id=885 */
        for (ret = 0; ret < nlsockets; ret++)
        {
            fcntl (lsockets[ret], F_SETFL,
                   fcntl (lsockets[ret], F_GETFL) | O_NONBLOCK);
        }
#endif //C_BSD
        if (!rzb_daemonize (NULL, sg_pidFile))
        {
            rzb_log (LOG_ERR, "daemonize() failed: %s", strerror (errno));
            return 1;
        }
#ifdef C_BSD
        for (ret = 0; ret < nlsockets; ret++)
        {
            fcntl (lsockets[ret], F_SETFL,
                   fcntl (lsockets[ret], F_GETFL) & ~O_NONBLOCK);
        }
#endif //C_BSD
        if (!debug_mode)
            if (chdir ("/") == -1)
                rzb_log (LOG_ERR, "^Can't change current working directory to root");

    }
    else
        rzb_debug_logging();

    signal (SIGINT, termination_handler);
    signal (SIGTERM, termination_handler);
    signal (SIGHUP, termination_handler);
    signal (SIGPIPE, termination_handler);
#endif
    uuid_copy(l_uuidNuggetType, RAZORD);

    // Build a context for a NUGGET_FILE_INJECT nugget with a unique UUID
    rzb_context =
        RZB_Register_Collector (sg_uuidNuggetId, l_uuidNuggetType);

    if (rzb_context == NULL)
    {
        rzb_log (LOG_ERR, "Error initializing context");
        return 1;
    }

    recvloop_th (lsockets, nlsockets);


    while (0);

    rzb_log (LOG_DEBUG, "Closing the main socket%s.", (nlsockets > 1) ? "s" : "");

    for (i = 0; i < nlsockets; i++)
    {
        close(lsockets[i]);
    }

#ifndef _WIN32
    if (nlsockets && localsock)
    {
        if (unlink (sg_unixPath) == -1)
            rzb_log (LOG_ERR, "Can't unlink the socket file %s", sg_unixPath);
        else
            rzb_log (LOG_INFO, "Socket file removed.");
    }
#endif
    if (rzb_context != NULL)
        Razorback_Shutdown_Context(rzb_context);
    return 0;
}

void
termination_handler (int signum)
{
    switch(signum)
    {
    case SIGHUP:
    case SIGPIPE:
        // Ignore it
        break;
    case SIGINT:
    case SIGTERM:
        rzb_log(LOG_INFO, "Razord shutting down");
        progexit = 1;
        break;
    default:
        rzb_log(LOG_ERR, "%s: Razord Unknown signal: %i", __func__, signum);
        break;
    }
}

