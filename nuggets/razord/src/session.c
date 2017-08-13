/*
 *  Copyright (C) 2007-2009 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm, Török Edvin
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
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#ifndef	_WIN32
#include <sys/socket.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_FD_PASSING
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#endif

#include <sys/time.h>
#endif
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stddef.h>
#include <limits.h>

#include "others.h"
#include "scanner.h"
#include "server.h"
#include "session.h"
#include "thrmgr.h"

#include <razorback/log.h>

#ifndef HAVE_FDPASSING
#define FEATURE_FDPASSING 0
#else
#define FEATURE_FDPASSING 1
#endif

static struct
{
    const char *cmd;
    const size_t len;
    enum commands cmdtype;
    int need_arg;
    int support_old;
    int enabled;
} commands[] =
{
    {
    CMD1, sizeof (CMD1) - 1, COMMAND_SCAN, 1, 1, 0}
    ,
    {
    CMD3, sizeof (CMD3) - 1, COMMAND_SHUTDOWN, 0, 1, 0}
    ,
    {
    CMD4, sizeof (CMD4) - 1, COMMAND_RELOAD, 0, 1, 0}
    ,
    {
    CMD5, sizeof (CMD5) - 1, COMMAND_PING, 0, 1, 0}
    ,
    {
    CMD6, sizeof (CMD6) - 1, COMMAND_CONTSCAN, 1, 1, 0}
    ,
        /* must be before VERSION, because they share common prefix! */
    {
    CMD18, sizeof (CMD18) - 1, COMMAND_COMMANDS, 0, 0, 1}
    ,
    {
    CMD7, sizeof (CMD7) - 1, COMMAND_VERSION, 0, 1, 1}
    ,
    {
    CMD8, sizeof (CMD8) - 1, COMMAND_STREAM, 0, 1, 1}
    ,
    {
    CMD10, sizeof (CMD10) - 1, COMMAND_END, 0, 0, 1}
    ,
    {
    CMD11, sizeof (CMD11) - 1, COMMAND_SHUTDOWN, 0, 1, 1}
    ,
    {
    CMD13, sizeof (CMD13) - 1, COMMAND_MULTISCAN, 1, 1, 1}
    ,
    {
    CMD14, sizeof (CMD14) - 1, COMMAND_FILDES, 0, 1, FEATURE_FDPASSING}
    ,
    {
    CMD15, sizeof (CMD15) - 1, COMMAND_STATS, 0, 0, 1}
    ,
    {
    CMD16, sizeof (CMD16) - 1, COMMAND_IDSESSION, 0, 0, 1}
    ,
    {
    CMD17, sizeof (CMD17) - 1, COMMAND_INSTREAM, 0, 0, 1}
    ,
    {
    CMD19, sizeof (CMD19) - 1, COMMAND_DETSTATSCLEAR, 0, 1, 1}
    ,
    {
    CMD20, sizeof (CMD20) - 1, COMMAND_DETSTATS, 0, 1, 1}
};

enum commands
parse_command (const char *cmd, const char **argument, int oldstyle)
{
    size_t i;
    *argument = NULL;
    for (i = 0; i < sizeof (commands) / sizeof (commands[0]); i++)
    {
        const size_t len = commands[i].len;
        if (!strncmp (cmd, commands[i].cmd, len))
        {
            const char *arg = cmd + len;
            if (commands[i].need_arg)
            {
                if (!*arg)
                {               /* missing argument */
                    rzb_log (LOG_DEBUG, "Command %s missing argument!", commands[i].cmd);
                    return COMMAND_UNKNOWN;
                }
                *argument = arg + 1;
            }
            else
            {
                if (*arg)
                {               /* extra stuff after command */
                    rzb_log (LOG_DEBUG, "Command %s has trailing garbage!",
                          commands[i].cmd);
                    return COMMAND_UNKNOWN;
                }
                *argument = NULL;
            }
            if (oldstyle && !commands[i].support_old)
            {
                rzb_log (LOG_DEBUG, "Command sent as old-style when not supported: %s",
                      commands[i].cmd);
                return COMMAND_UNKNOWN;
            }
            return commands[i].cmdtype;
        }
    }
    return COMMAND_UNKNOWN;
}

int
conn_reply_single (const client_conn_t * conn, const char *path,
                   const char *status)
{
    if (conn->id)
    {
        if (path)
            return mdprintf (conn->sd, "%u: %s: %s%c", conn->id, path, status,
                             conn->term);
        return mdprintf (conn->sd, "%u: %s%c", conn->id, status, conn->term);
    }
    if (path)
        return mdprintf (conn->sd, "%s: %s%c", path, status, conn->term);
    return mdprintf (conn->sd, "%s%c", status, conn->term);
}

int
conn_reply (const client_conn_t * conn, const char *path,
            const char *msg, const char *status)
{
    if (conn->id)
    {
        if (path)
            return mdprintf (conn->sd, "%u: %s: %s %s%c", conn->id, path, msg,
                             status, conn->term);
        return mdprintf (conn->sd, "%u: %s %s%c", conn->id, msg, status,
                         conn->term);
    }
    if (path)
        return mdprintf (conn->sd, "%s: %s %s%c", path, msg, status,
                         conn->term);
    return mdprintf (conn->sd, "%s %s%c", msg, status, conn->term);
}

int
conn_reply_virus (const client_conn_t * conn, const char *file,
                  const char *virname)
{
    if (conn->id)
    {
        return mdprintf (conn->sd, "%u: %s: %s FOUND%c", conn->id, file,
                         virname, conn->term);
    }
    return mdprintf (conn->sd, "%s: %s FOUND%c", file, virname, conn->term);
}

int
conn_reply_error (const client_conn_t * conn, const char *msg)
{
    return conn_reply (conn, NULL, msg, "ERROR");
}

#define BUFFSIZE 1024
int
conn_reply_errno (const client_conn_t * conn, const char *path,
                  const char *msg)
{
    char err[BUFFSIZE + sizeof (". ERROR")];
    strerror_r (errno, err, BUFFSIZE - 1);
    strcat (err, ". ERROR");
    return conn_reply (conn, path, msg, err);
}

/* returns
 *  -1 on fatal error (shutdown)
 *  0 on ok
 *  >0 errors encountered
 */
int
command (client_conn_t * conn, int *virus)
{
    int desc = conn->sd;
    int type = -1;              /* TODO: make this enum */
    int ret = 0;
    int flags = CLI_FTW_STD;

    struct scan_cb_data scandata;
    struct cli_ftw_cbdata data;
    unsigned ok, error, total;
    struct stat sb;
    jobgroup_t *group = NULL;

    if (thrmgr_group_need_terminate (conn->group))
    {
        rzb_log (LOG_ERR, "$Client disconnected while command was active");
        if (conn->scanfd != -1)
            close (conn->scanfd);
        return 1;
    }

    data.data = &scandata;
    memset (&scandata, 0, sizeof (scandata));
    scandata.id = conn->id;
    scandata.group = conn->group;
    scandata.odesc = desc;
    scandata.conn = conn;
    scandata.thr_pool = conn->thrpool;
    scandata.toplevel_path = conn->filename;

    switch (conn->cmdtype)
    {
    case COMMAND_SCAN:
        thrmgr_setactivetask (NULL, "SCAN");
        type = TYPE_SCAN;
        break;
    case COMMAND_CONTSCAN:
        thrmgr_setactivetask (NULL, "CONTSCAN");
        type = TYPE_CONTSCAN;
        break;
    case COMMAND_MULTISCAN:
        {
            int multiscan, max, alive;

            /* use MULTISCAN only for directories (bb #1869) */
            if (stat (conn->filename, &sb) == 0 && !S_ISDIR (sb.st_mode))
            {
                thrmgr_setactivetask (NULL, "CONTSCAN");
                type = TYPE_CONTSCAN;
                break;
            }

            pthread_mutex_lock (&conn->thrpool->pool_mutex);
            multiscan = conn->thrpool->thr_multiscan;
            max = conn->thrpool->thr_max;
            if (multiscan + 1 < max)
                conn->thrpool->thr_multiscan = multiscan + 1;
            else
            {
                alive = conn->thrpool->thr_alive;
                ret = -1;
            }
            pthread_mutex_unlock (&conn->thrpool->pool_mutex);
            if (ret)
            {
                /* multiscan has 1 control thread, so there needs to be at least
                   1 threads that is a non-multiscan controlthread to scan and
                   make progress. */
                rzb_log (LOG_WARNING, "Not enough threads for multiscan. Max: %d, Alive: %d, Multiscan: %d+1", max, alive, multiscan);
                conn_reply (conn, conn->filename,
                            "Not enough threads for multiscan. Increase MaxThreads.",
                            "ERROR");
                return 1;
            }
            flags &= ~CLI_FTW_NEED_STAT;
            thrmgr_setactivetask (NULL, "MULTISCAN");
            type = TYPE_MULTISCAN;
            scandata.group = group = thrmgr_group_new ();
            if (!group)
                return 1;
            break;
        }
    case COMMAND_MULTISCANFILE:
        thrmgr_setactivetask (NULL, "MULTISCANFILE");
        scandata.group = NULL;
        scandata.type = TYPE_SCAN;
        scandata.thr_pool = NULL;
        /* TODO: check ret value */
        ret = scan_callback (NULL, conn->filename, conn->filename, visit_file, &data);  /* callback freed it */
        conn->filename = NULL;
        *virus = scandata.infected;
        if (ret == 1)
        {
            thrmgr_group_terminate (conn->group);
            return 1;
        }
        return scandata.errors > 0 ? scandata.errors : 0;
    case COMMAND_FILDES:
        thrmgr_setactivetask (NULL, "FILDES");
        conn_reply_error (conn, "FILDES support not compiled in.");
        close (conn->scanfd);
        return 0;
    case COMMAND_STATS:
        thrmgr_setactivetask (NULL, "STATS");
        if (conn->group)
            mdprintf (desc, "%u: ", conn->id);
        thrmgr_printstats (desc, conn->term);
        return 0;
    case COMMAND_STREAM:
        thrmgr_setactivetask (NULL, "STREAM");
        ret = scanstream (desc, NULL, conn->term);
//        if (ret == CL_VIRUS)
//            *virus = 1;

        return 0;
    case COMMAND_INSTREAMSCAN:
        thrmgr_setactivetask (NULL, "INSTREAM");
//        ret = scanfd (conn, NULL, desc, 1);
#if 0
        if (ret == CL_VIRUS)
        {
            *virus = 1;
        }
        else if (ret == CL_ETIMEOUT)
        {
            thrmgr_group_terminate (conn->group);
            ret = 1;
        }
        else
            ret = 0;
        if (ftruncate (conn->scanfd, 0) == -1)
        {
            /* not serious, we're going to close it and unlink it anyway */
            rzb_log (LOG_ERR, "*ftruncate failed: %d", errno);
        }
        close (conn->scanfd);
        conn->scanfd = -1;
        cli_unlink (conn->filename);
#endif
        return ret;
    default:
        rzb_log (LOG_ERR, "Invalid command distpached: %d", conn->cmdtype);
        return 1;
    }

    scandata.type = type;
#if 0
    maxdirrec = optget (opts, "MaxDirectoryRecursion")->numarg;
    if (optget (opts, "FollowDirectorySymlinks")->enabled) {}
        //flags |= CLI_FTW_FOLLOW_DIR_SYMLINK;
    if (optget (opts, "FollowFileSymlinks")->enabled) {}
        //flags |= CLI_FTW_FOLLOW_FILE_SYMLINK;

    if (!optget (opts, "CrossFilesystems")->enabled)
        if (stat (conn->filename, &sb) == 0)
            scandata.dev = sb.st_dev;
#endif
    ret =
        cli_ftw (conn->filename, flags, INT_MAX,
                 scan_callback, &data, scan_pathchk);

    if (scandata.group && conn->cmdtype == COMMAND_MULTISCAN)
    {
        thrmgr_group_waitforall (group, &ok, &error, &total);
        pthread_mutex_lock (&conn->thrpool->pool_mutex);
        conn->thrpool->thr_multiscan--;
        pthread_mutex_unlock (&conn->thrpool->pool_mutex);
    }
    else
    {
        error = scandata.errors;
        total = scandata.total;
        ok = total - error - scandata.infected;
    }
    if (ok + error == total && (error != total))
    {
        if (conn_reply_single (conn, conn->filename, "OK") == -1)
            ret = -1;
    }
    *virus = total - (ok + error);

    if (ret == -1)
        thrmgr_group_terminate (conn->group);
    return error;
}

static int
dispatch_command (client_conn_t * conn, enum commands cmd,
                  const char *argument)
{
    int ret = 0;
    int bulk;
    client_conn_t *dup_conn =
        (client_conn_t *) malloc (sizeof (struct client_conn_tag));

    if (!dup_conn)
    {
        rzb_log (LOG_ERR, "Can't allocate memory for client_conn");
        return -1;
    }
    memcpy (dup_conn, conn, sizeof (*conn));
    dup_conn->cmdtype = cmd;
    dup_conn->scanfd = -1;
    bulk = 1;
    switch (cmd)
    {
    case COMMAND_FILDES:
        if (conn->scanfd == -1)
        {
            conn_reply_error (dup_conn, "No file descriptor received.");
            ret = 1;
        }
        dup_conn->scanfd = conn->scanfd;
        /* consume FD */
        conn->scanfd = -1;
        break;
    case COMMAND_SCAN:
    case COMMAND_CONTSCAN:
    case COMMAND_MULTISCAN:
        dup_conn->filename = strdup (argument);
        if (!dup_conn->filename)
        {
            rzb_log (LOG_ERR, "Failed to allocate memory for filename");
            ret = -1;
        }
        break;
    case COMMAND_INSTREAMSCAN:
        dup_conn->scanfd = conn->scanfd;
        conn->scanfd = -1;
        break;
    case COMMAND_STREAM:
    case COMMAND_STATS:
        /* not a scan command, don't queue to bulk */
        bulk = 0;
        /* just dispatch the command */
        break;
    default:
        rzb_log (LOG_ERR, "Invalid command dispatch: %d", cmd);
        ret = -2;
        break;
    }
    if (!dup_conn->group)
        bulk = 0;
    if (!ret
        && !thrmgr_group_dispatch (dup_conn->thrpool, dup_conn->group,
                                   dup_conn, bulk))
    {
        rzb_log (LOG_ERR, "thread dispatch failed");
        ret = -2;
    }
    if (ret)
    {
        free (dup_conn);
    }
    return ret;
}

static int
print_ver (int desc, char term)
{
    return mdprintf (desc, "Razorback 0.4.1%c",  term);
}

static void
print_commands (int desc, char term)
{
    unsigned i, n;
    print_ver (desc, '|');
    mdprintf (desc, " COMMANDS:");
    n = sizeof (commands) / sizeof (commands[0]);
    for (i = 0; i < n; i++)
    {
        mdprintf (desc, " %s", commands[i].cmd);
    }
    mdprintf (desc, "%c", term);
}

/* returns:
 *  <0 for error
 *     -1 out of memory
 *     -2 other
 *   0 for async dispatched
 *   1 for command completed (connection can be closed)
 */
int
execute_or_dispatch_command (client_conn_t * conn, enum commands cmd,
                             const char *argument)
{
    int desc = conn->sd;
    char term = conn->term;
    /* execute commands that can be executed quickly on the recvloop thread,
     * these must:
     *  - not involve any operation that can block for a long time, such as disk
     *  I/O
     *  - send of atomic message is allowed.
     * Dispatch other commands */
    if (conn->group)
    {
        switch (cmd)
        {
        case COMMAND_FILDES:
        case COMMAND_SCAN:
        case COMMAND_END:
        case COMMAND_INSTREAM:
        case COMMAND_INSTREAMSCAN:
        case COMMAND_VERSION:
        case COMMAND_PING:
        case COMMAND_STATS:
        case COMMAND_COMMANDS:
            /* These commands are accepted inside IDSESSION */
            break;
        default:
            /* these commands are not recognized inside an IDSESSION */
            conn_reply_error (conn, "Command invalid inside IDSESSION.");
            rzb_log (LOG_DEBUG, "SESSION: command is not valid inside IDSESSION: %d",
                  cmd);
            conn->group = NULL;
            return 1;
        }
    }

    switch (cmd)
    {
    case COMMAND_SHUTDOWN:
        pthread_mutex_lock (&exit_mutex);
        progexit = 1;
        pthread_mutex_unlock (&exit_mutex);
        return 1;
    case COMMAND_RELOAD:
        pthread_mutex_lock (&reload_mutex);
        reload = 1;
        pthread_mutex_unlock (&reload_mutex);
        mdprintf (desc, "RELOADING%c", term);
        /* we set reload flag, and we'll reload before closing the
         * connection */
        return 1;
    case COMMAND_PING:
        if (conn->group)
            mdprintf (desc, "%u: PONG%c", conn->id, term);
        else
            mdprintf (desc, "PONG%c", term);
        return conn->group ? 0 : 1;
    case COMMAND_VERSION:
        {
            if (conn->group)
                mdprintf (desc, "%u: ", conn->id);
            print_ver (desc, conn->term);
            return conn->group ? 0 : 1;
        }
    case COMMAND_COMMANDS:
        {
            if (conn->group)
                mdprintf (desc, "%u: ", conn->id);
            print_commands (desc, conn->term);
            return conn->group ? 0 : 1;
        }
    case COMMAND_DETSTATSCLEAR:
        {
            return 1;
        }
    case COMMAND_DETSTATS:
        {
            return 1;
        }
    case COMMAND_INSTREAM:
        {
#if 0
            int rc =
                cli_gentempfd (optget
                               (conn->opts, "TemporaryDirectory")->strarg,
                               &conn->filename, &conn->scanfd);
            if (rc != CL_SUCCESS)
                return rc;
            conn->quota = optget (conn->opts, "StreamMaxLength")->numarg;
#endif
            conn->mode = MODE_STREAM;
            return 0;
        }
    case COMMAND_STREAM:
    case COMMAND_MULTISCAN:
    case COMMAND_CONTSCAN:
    case COMMAND_STATS:
    case COMMAND_FILDES:
    case COMMAND_SCAN:
    case COMMAND_INSTREAMSCAN:
        return dispatch_command (conn, cmd, argument);
    case COMMAND_IDSESSION:
        conn->group = thrmgr_group_new ();
        if (!conn->group)
            return 1;
        return 0;
    case COMMAND_END:
        if (!conn->group)
        {
            /* end without idsession? */
            conn_reply_single (conn, NULL, "UNKNOWN COMMAND");
            return 1;
        }
        /* need to close connection  if we were last in group */
        return 1;
        /*case COMMAND_UNKNOWN: */
    default:
        conn_reply_single (conn, NULL, "UNKNOWN COMMAND");
        return 1;
    }
}

