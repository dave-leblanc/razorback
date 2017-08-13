/*
 *  Copyright (C) 2007-2009 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm, Trog, Török Edvin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifndef	_WIN32
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#endif
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#ifdef C_SOLARIS
#include <stdio_ext.h>
#endif

#include "razord.h"
#include "fan.h"
#include "server.h"
#include "thrmgr.h"
#include "session.h"
#include "others.h"
#include "shared.h"

#define BUFFSIZE 1024

#include <razorback/log.h>
#include <razorback/socket.h>

int progexit = 0;
pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;
int reload = 0;
time_t reloaded_time = 0;
pthread_mutex_t reload_mutex = PTHREAD_MUTEX_INITIALIZER;
int sighup = 0;

void *event_wake_recv = NULL;
void *event_wake_accept = NULL;

static void
scanner_thread (void *arg)
{
    client_conn_t *conn = (client_conn_t *) arg;
    int ret;
    int virus = 0, errors = 0;

    ret = command (conn, &virus);
    if (ret == -1)
    {
        pthread_mutex_lock (&exit_mutex);
        progexit = 1;
        pthread_mutex_unlock (&exit_mutex);
        errors = 1;
    }
    else
        errors = ret;

    if (conn->filename)
        free (conn->filename);
    rzb_log (LOG_DEBUG, "Finished scanthread");
    if (thrmgr_group_finished (conn->group, virus ? EXIT_OTHER :
                               errors ? EXIT_ERROR : EXIT_OK))
    {
        rzb_log (LOG_DEBUG, "Scanthread: connection shut down (FD %d)", conn->sd);
        /* close connection if we were last in group */
        shutdown(conn->sd, 2);
        close (conn->sd);
    }
    free (conn);
    return;
}

static int syncpipe_wake_recv_w = -1;

void
sighandler_th (int sig)
{
    int action = 0;
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        progexit = 1;
        action = 1;
        break;

#ifdef	SIGHUP
    case SIGHUP:
        sighup = 1;
        action = 1;
        break;
#endif

#ifdef	SIGUSR2
    case SIGUSR2:
        reload = 1;
        action = 1;
        break;
#endif

    default:
        break;                  /* Take no action on other signals - e.g. SIGPIPE */
    }
    /* a signal doesn't always wake poll(), for example on FreeBSD */
    if (action && syncpipe_wake_recv_w != -1)
        if (write (syncpipe_wake_recv_w, "", 1) != 1)
            rzb_log (LOG_DEBUG, "Failed to write to syncpipe");
}


/*
 * zCOMMANDS are delimited by \0
 * nCOMMANDS are delimited by \n
 * Old-style non-prefixed commands are one packet, optionally delimited by \n,
 * with trailing \r|\n ignored
 */
static const char *
get_cmd (struct fd_buf *buf, size_t off, size_t * len, char *term,
         int *oldstyle)
{
    char *pos;
    if (!buf->off || off >= buf->off)
    {
        *len = 0;
        return NULL;
    }

    *term = '\n';
    switch (buf->buffer[off])
    {
        /* commands terminated by delimiters */
    case 'z':
        *term = '\0';
    case 'n':
        pos = memchr (buf->buffer + off, *term, buf->off - off);
        if (!pos)
        {
            /* we don't have another full command yet */
            *len = 0;
            return NULL;
        }
        *pos = '\0';
        if (*term)
        {
            chomp (buf->buffer + off);
        }
        *len = pos - buf->buffer - off;
        *oldstyle = 0;
        return buf->buffer + off + 1;
    default:
        /* one packet = one command */
        if (off)
            return NULL;
        pos = memchr (buf->buffer, '\n', buf->off);
        if (pos)
        {
            *len = pos - buf->buffer;
            *pos = '\0';
        }
        else
        {
            *len = buf->off;
            buf->buffer[buf->off] = '\0';
        }
        chomp (buf->buffer);
        *oldstyle = 1;
        return buf->buffer;
    }
}

struct acceptdata
{
    struct fd_data fds;
    struct fd_data recv_fds;
    pthread_cond_t cond_nfds;
    int max_queue;
    int commandtimeout;
    int syncpipe_wake_recv[2];
    int syncpipe_wake_accept[2];
};

#define ACCEPTDATA_INIT(mutex1, mutex2) { FDS_INIT(mutex1), FDS_INIT(mutex2), PTHREAD_COND_INITIALIZER, 0, 0, {-1, -1}, {-1, -1}}

static void *
acceptloop_th (void *arg)
{
    char buff[BUFFSIZE + 1];
    size_t i;
    struct acceptdata *data = (struct acceptdata *) arg;
    struct fd_data *fds = &data->fds;
    struct fd_data *recv_fds = &data->recv_fds;
    int max_queue = data->max_queue;
    int commandtimeout = data->commandtimeout;

    pthread_mutex_lock (fds->buf_mutex);
    for (;;)
    {

        /* Block waiting for data to become available for reading */
        int new_sd = fds_poll_recv (fds, -1, 0, event_wake_accept);
#ifdef _WIN32
        ResetEvent (event_wake_accept);
#endif
        /* TODO: what about sockets that get rm-ed? */
        if (!fds->nfds)
        {
            /* no more sockets to poll, all gave an error */
            rzb_log (LOG_ERR, "Main socket gone: fatal");
            break;
        }

        if (new_sd == -1 && errno != EINTR)
        {
            rzb_log (LOG_ERR, "Failed to poll sockets, fatal");
            pthread_mutex_lock (&exit_mutex);
            progexit = 1;
            pthread_mutex_unlock (&exit_mutex);
            break;
        }

        /* accept() loop */
        for (i = 0; i < fds->nfds && new_sd >= 0; i++)
        {
            struct fd_buf *buf = &fds->buf[i];
            if (!buf->got_newdata)
                continue;
#ifndef _WIN32
            if (buf->fd == data->syncpipe_wake_accept[0])
            {
                /* dummy sync pipe, just to wake us */
                if (read (buf->fd, buff, sizeof (buff)) < 0)
                {
                    rzb_log (LOG_WARNING, "Syncpipe read failed");
                }
                continue;
            }
#endif
            if (buf->got_newdata == -1)
            {
                rzb_log (LOG_DEBUG, "Acceptloop closed FD: %d", buf->fd);
                shutdown (buf->fd, 2);
                close (buf->fd);
                buf->fd = -1;
                continue;
            }

            /* don't accept unlimited number of connections, or
             * we'll run out of file descriptors */
            pthread_mutex_lock (recv_fds->buf_mutex);
            while (recv_fds->nfds > (unsigned) max_queue)
            {
                pthread_mutex_lock (&exit_mutex);
                if (progexit)
                {
                    pthread_mutex_unlock (&exit_mutex);
                    break;
                }
                pthread_mutex_unlock (&exit_mutex);
                pthread_cond_wait (&data->cond_nfds, recv_fds->buf_mutex);
            }
            pthread_mutex_unlock (recv_fds->buf_mutex);

            pthread_mutex_lock (&exit_mutex);
            if (progexit)
            {
                pthread_mutex_unlock (&exit_mutex);
                break;
            }
            pthread_mutex_unlock (&exit_mutex);

            /* listen only socket */
            new_sd = accept(fds->buf[i].fd, NULL, NULL);

            if (new_sd >= 0)
            {
                int ret, flags;
#ifdef F_GETFL
                flags = fcntl (new_sd, F_GETFL, 0);
                if (flags != -1)
                {
                    if (fcntl (new_sd, F_SETFL, flags | O_NONBLOCK) == -1)
                    {
                        rzb_log (LOG_WARNING, "Can't set socket to nonblocking mode, errno %d", errno);
                    }
                }
                else
                {
                    rzb_log (LOG_WARNING, "Can't get socket flags, errno %d", errno);
                }
#else
                rzb_log (LOG_WARNING, "Nonblocking sockets not available!");
#endif
                rzb_log (LOG_DEBUG, "Got new connection, FD %d", new_sd);
                pthread_mutex_lock (recv_fds->buf_mutex);
                ret = fds_add (recv_fds, new_sd, 0, commandtimeout);
                pthread_mutex_unlock (recv_fds->buf_mutex);

                if (ret == -1)
                {
                    rzb_log (LOG_ERR, "fds_add failed");
                    close (new_sd);
                    continue;
                }

                /* notify recvloop */
#ifdef _WIN32
                SetEvent (event_wake_recv);
#else
                if (write (data->syncpipe_wake_recv[1], "", 1) == -1)
                {
                    rzb_log (LOG_ERR, "write syncpipe failed");
                    continue;
                }
#endif
            }
            else if (errno != EINTR)
            {
                /* very bad - need to exit or restart */
#ifdef HAVE_STRERROR_R
                strerror_r (errno, buff, BUFFSIZE);
                rzb_log (LOG_ERR, "accept() failed: %s", buff);
#else
                rzb_log (LOG_ERR, "accept() failed");
#endif
                /* give the poll loop a chance to close disconnected FDs */
                break;
            }

        }

        /* handle progexit */
        pthread_mutex_lock (&exit_mutex);
        if (progexit)
        {
            pthread_mutex_unlock (&exit_mutex);
            break;
        }
        pthread_mutex_unlock (&exit_mutex);
    }
    pthread_mutex_unlock (fds->buf_mutex);

    for (i = 0; i < fds->nfds; i++)
    {
        if (fds->buf[i].fd == -1)
            continue;
        rzb_log (LOG_DEBUG, "Shutdown: closed fd %d", fds->buf[i].fd);
        shutdown (fds->buf[i].fd, 2);
        close (fds->buf[i].fd);
    }
    fds_free (fds);

    pthread_mutex_lock (&exit_mutex);
    progexit = 1;
    pthread_mutex_unlock (&exit_mutex);
#ifdef _WIN32
    SetEvent (event_wake_recv);
#else
    if (write (data->syncpipe_wake_recv[1], "", 1) < 0)
    {
        rzb_log (LOG_DEBUG, "Syncpipe write failed");
    }
#endif
    return NULL;
}

static const char *
parse_dispatch_cmd (client_conn_t * conn, struct fd_buf *buf, size_t * ppos,
                    int *error, int readtimeout)
{
    const char *cmd = NULL;
    int rc;
    size_t cmdlen;
    char term;
    int oldstyle;
    size_t pos = *ppos;
    /* Parse & dispatch commands */
    while ((conn->mode == MODE_COMMAND) &&
           (cmd = get_cmd (buf, pos, &cmdlen, &term, &oldstyle)) != NULL)
    {
        const char *argument;
        enum commands cmdtype;
        if (conn->group && oldstyle)
        {
            rzb_log (LOG_DEBUG, "Received oldstyle command inside IDSESSION: %s", cmd);
            conn_reply_error (conn,
                              "Only nCMDS\\n and zCMDS\\0 are accepted inside IDSESSION.");
            *error = 1;
            break;
        }
        cmdtype = parse_command (cmd, &argument, oldstyle);
        rzb_log (LOG_DEBUG, "got command %s (%u, %u), argument: %s",
              cmd, (unsigned) cmdlen, (unsigned) cmdtype,
              argument ? argument : "");
        if (cmdtype == COMMAND_FILDES)
        {
            if (buf->buffer + buf->off <= cmd + strlen ("FILDES"))
            {
                /* we need the extra byte from recvmsg */
                conn->mode = MODE_WAITANCILL;
                buf->mode = MODE_WAITANCILL;
                /* put term back */
                buf->buffer[pos + cmdlen] = term;
                cmdlen = 0;
                rzb_log (LOG_DEBUG, "RECVTH: mode -> MODE_WAITANCILL");
                break;
            }
            /* eat extra \0 for controlmsg */
            cmdlen++;
            rzb_log (LOG_DEBUG, "RECVTH: FILDES command complete");
        }
        conn->term = term;
        buf->term = term;

        if ((rc = execute_or_dispatch_command (conn, cmdtype, argument)) < 0)
        {
            rzb_log (LOG_ERR, "Command dispatch failed");
        }
        if (thrmgr_group_need_terminate (conn->group))
        {
            rzb_log (LOG_DEBUG, "Receive thread: have to terminate group");
            *error = 1;
            break;
        }
        if (*error || !conn->group || rc)
        {
            if (rc && thrmgr_group_finished (conn->group, EXIT_OK))
            {
                rzb_log (LOG_DEBUG, "Receive thread: closing conn (FD %d), group finished", conn->sd);
                /* if there are no more active jobs */
                shutdown (conn->sd, 2);
                close (conn->sd);
                buf->fd = -1;
                conn->group = NULL;
            }
            else if (conn->mode != MODE_STREAM)
            {
                rzb_log (LOG_DEBUG, "mode -> MODE_WAITREPLY");
                /* no more commands are accepted */
                conn->mode = MODE_WAITREPLY;
                /* Stop monitoring this FD, it will be closed either
                 * by us, or by the scanner thread. 
                 * Never close a file descriptor that is being
                 * monitored by poll()/select() from another thread,
                 * because this can lead to subtle bugs such as:
                 * Other thread closes file descriptor -> POLLHUP is
                 * set, but the poller thread doesn't wake up yet.
                 * Another client opens a connection and sends some
                 * data. If the socket reuses the previous file descriptor,
                 * then POLLIN is set on the file descriptor too.
                 * When poll() wakes up it sees POLLIN | POLLHUP
                 * and thinks that the client has sent some data,
                 * and closed the connection, so clamd closes the
                 * connection in turn resulting in a bug.
                 *
                 * If we wouldn't have poll()-ed the file descriptor
                 * we closed in another thread, but rather made sure
                 * that we don't put a FD that we're about to close
                 * into poll()'s list of watched fds; then POLLHUP
                 * would be set, but the file descriptor would stay
                 * open, until we wake up from poll() and close it.
                 * Thus a new connection won't be able to reuse the
                 * same FD, and there is no bug.
                 * */
                buf->fd = -1;
            }
        }
        /* we received a command, set readtimeout */
        time (&buf->timeout_at);
        buf->timeout_at += readtimeout;
        pos += cmdlen + 1;
        if (conn->mode == MODE_STREAM)
        {
            /* TODO: this doesn't belong here */
            buf->dumpname = conn->filename;
            buf->dumpfd = conn->scanfd;
            rzb_log (LOG_DEBUG, "Receive thread: INSTREAM: %s fd %u", buf->dumpname,
                  buf->dumpfd);
        }
        if (conn->mode != MODE_COMMAND)
        {
            rzb_log (LOG_DEBUG, "Breaking command loop, mode is no longer MODE_COMMAND");
            break;
        }
        conn->id++;
    }
    *ppos = pos;
    buf->mode = conn->mode;
    buf->id = conn->id;
    buf->group = conn->group;
    buf->quota = conn->quota;
    if (conn->scanfd != -1 && conn->scanfd != buf->dumpfd)
    {
        rzb_log (LOG_DEBUG, "Unclaimed file descriptor received, closing: %d",
              conn->scanfd);
        close (conn->scanfd);
        /* protocol error */
        conn_reply_error (conn,
                          "PROTOCOL ERROR: ancillary data sent without FILDES.");
        *error = 1;
        return NULL;
    }
    if (!*error)
    {
        /* move partial command to beginning of buffer */
        if (pos < buf->off)
        {
            memmove (buf->buffer, &buf->buffer[pos], buf->off - pos);
            buf->off -= pos;
        }
        else
            buf->off = 0;
        if (buf->off)
            rzb_log (LOG_DEBUG, "Moved partial command: %lu", (unsigned long) buf->off);
        else
            rzb_log (LOG_DEBUG, "Consumed entire command");
    }
    *ppos = pos;
    return cmd;
}

/* static const unsigned char* parse_dispatch_cmd(client_conn_t *conn, struct fd_buf *buf, size_t *ppos, int *error, const struct optstruct *opts, int readtimeout) */
static int
handle_stream (client_conn_t * conn, struct fd_buf *buf,
               int *error, size_t * ppos,
               int readtimeout)
{
    int rc;
    size_t pos = *ppos;
    size_t cmdlen;

    rzb_log (LOG_DEBUG, "mode == MODE_STREAM");
    /* we received a chunk, set readtimeout */
    time (&buf->timeout_at);
    buf->timeout_at += readtimeout;
    if (!buf->chunksize)
    {
        /* read chunksize */
        if (buf->off >= 4)
        {
            uint32_t cs = *(uint32_t *) buf->buffer;
            buf->chunksize = ntohl (cs);
            rzb_log (LOG_DEBUG, "Got chunksize: %u", buf->chunksize);
            if (!buf->chunksize)
            {
                /* chunksize 0 marks end of stream */
                conn->scanfd = buf->dumpfd;
                conn->term = buf->term;
                buf->dumpfd = -1;
                buf->mode = buf->group ? MODE_COMMAND : MODE_WAITREPLY;
                if (buf->mode == MODE_WAITREPLY)
                    buf->fd = -1;
                rzb_log (LOG_DEBUG, "Chunks complete");
                buf->dumpname = NULL;
                if ((rc =
                     execute_or_dispatch_command (conn, COMMAND_INSTREAMSCAN,
                                                  NULL)) < 0)
                {
                    rzb_log (LOG_ERR, "Command dispatch failed");
                    *error = 1;
                }
                else
                {
                    pos = 4;
                    memmove (buf->buffer, &buf->buffer[pos], buf->off - pos);
                    buf->off -= pos;
                    *ppos = 0;
                    buf->id++;
                    return 0;
                }
            }
            if (buf->chunksize > (uint32_t)buf->quota)
            {
                rzb_log (LOG_WARNING, "INSTREAM: Size limit reached, (requested: %lu, max: %lu)", (unsigned long) buf->chunksize, (unsigned long) buf->quota);
                conn_reply_error (conn, "INSTREAM size limit exceeded.");
                *error = 1;
                *ppos = pos;
                return -1;
            }
            else
            {
                buf->quota -= buf->chunksize;
            }
            rzb_log (LOG_DEBUG, "Quota: %lu", buf->quota);
            pos = 4;
        }
        else
            return -1;
    }
    else
        pos = 0;
    if (pos + buf->chunksize < buf->off)
        cmdlen = buf->chunksize;
    else
        cmdlen = buf->off - pos;
    buf->chunksize -= cmdlen;
#if 0
    if (cli_writen (buf->dumpfd, buf->buffer + pos, cmdlen) < 0)
    {
        conn_reply_error (conn, "Error writing to temporary file");
        rzb_log (LOG_ERR, "!INSTREAM: Can't write to temporary file.\n");
        *error = 1;
    }
#endif
    rzb_log (LOG_ERR, "$Processed %lu bytes of chunkdata", cmdlen);
    pos += cmdlen;
    if (pos == buf->off)
    {
        buf->off = 0;
    }
    *ppos = pos;
    return 0;
}

int
recvloop_th (int socketds[], unsigned nsockets)
{
    int readtimeout, ret = 0;
    unsigned int options = 0;
#ifndef	_WIN32
    struct rlimit rlim;
#endif
    char buff[BUFFSIZE + 1];
    size_t i, j, rr_last = 0;
    pthread_t accept_th;
    pthread_mutex_t fds_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t recvfds_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct acceptdata acceptdata =
        ACCEPTDATA_INIT (&fds_mutex, &recvfds_mutex);
    struct fd_data *fds = &acceptdata.recv_fds;
    threadpool_t *thr_pool;

#ifdef FANOTIFY
    pthread_t fan_pid;
    pthread_attr_t fan_attr;
    struct thrarg *tharg = NULL;    /* shut up gcc */
#endif

#ifndef _WIN32
    if (getrlimit (RLIMIT_CORE, &rlim) == 0)
    {
        rzb_log (LOG_DEBUG, "Limits: Core-dump limit is %lu.",
              (unsigned long) rlim.rlim_cur);
    }
#endif


    acceptdata.commandtimeout = sg_commandTimeout;
    readtimeout = sg_readTimeout;
    acceptdata.max_queue = sg_maxQueue;

#if !defined(_WIN32) && defined(RLIMIT_NOFILE)
    if (getrlimit (RLIMIT_NOFILE, &rlim) == 0)
    {
        /* Condition to not run out of file descriptors:
         * MaxThreads * MaxRecursion + (MaxQueue - MaxThreads) + CLAMDFILES < RLIMIT_NOFILE 
         * CLAMDFILES is 6: 3 standard FD + logfile + 2 FD for reloading the DB
         * */
#ifdef C_SOLARIS
#ifdef HAVE_ENABLE_EXTENDED_FILE_STDIO
        if (enable_extended_FILE_stdio (-1, -1) == -1)
        {
            rzb_log (LOG_WARNING, "Unable to set extended FILE stdio, clamd will be limited to max 256 open files");
            rlim.rlim_cur = rlim.rlim_cur > 255 ? 255 : rlim.rlim_cur;
        }
#elif !defined(_LP64)
        if (rlim.rlim_cur > 255)
        {
            rlim.rlim_cur = 255;
            rzb_log (LOG_WARNING, "Solaris only supports 256 open files for 32-bit processes, you need at least Solaris 10u4, or compile as 64-bit to support more!");
        }
#endif
#endif
    }
#endif

#if 0
    if (optget (opts, "ScanOnAccess")->enabled)
#ifdef FANOTIFY
    {
        do
        {
            if (pthread_attr_init (&fan_attr))
                break;
            pthread_attr_setdetachstate (&fan_attr, PTHREAD_CREATE_JOINABLE);
            if (!(tharg = (struct thrarg *) malloc (sizeof (struct thrarg))))
                break;
            tharg->opts = opts;
            tharg->engine = engine;
            tharg->options = options;
            if (!pthread_create (&fan_pid, &fan_attr, fan_th, tharg))
                break;
            free (tharg);
            tharg = NULL;
        }
        while (0);
        if (!tharg)
            rzb_log (LOG_ERR, "!Unable to start on-access scan\n");
    }
#else
        rzb_log (LOG_ERR, "!On-access scan is not available\n");
#endif
#endif


    for (i = 0; i < nsockets; i++)
        if (fds_add (&acceptdata.fds, socketds[i], 1, 0) == -1)
        {
            rzb_log (LOG_ERR, "fds_add failed");
            return 1;
        }
#ifdef _WIN32
    event_wake_accept = CreateEvent (NULL, TRUE, FALSE, NULL);
    event_wake_recv = CreateEvent (NULL, TRUE, FALSE, NULL);
#else
    if (pipe (acceptdata.syncpipe_wake_recv) == -1 ||
        (pipe (acceptdata.syncpipe_wake_accept) == -1))
    {

        rzb_log (LOG_ERR, "pipe failed");
        exit (-1);
    }
    syncpipe_wake_recv_w = acceptdata.syncpipe_wake_recv[1];

    if (fds_add (fds, acceptdata.syncpipe_wake_recv[0], 1, 0) == -1 ||
        fds_add (&acceptdata.fds, acceptdata.syncpipe_wake_accept[0], 1, 0))
    {
        rzb_log (LOG_ERR, "failed to add pipe fd");
        exit (-1);
    }
#endif

    if ((thr_pool =
         thrmgr_new (sg_maxThreads, sg_idleTimeout, sg_maxQueue,
                     scanner_thread)) == NULL)
    {
        rzb_log (LOG_ERR, "thrmgr_new failed");
        exit (-1);
    }

    if (pthread_create (&accept_th, NULL, acceptloop_th, &acceptdata))
    {
        rzb_log (LOG_ERR, "pthread_create failed");
        exit (-1);
    }

    for (;;)
    {
        int new_sd;

        /* Block waiting for connection on any of the sockets */
        pthread_mutex_lock (fds->buf_mutex);
        fds_cleanup (fds);
        /* signal that we can accept more connections */
        if (fds->nfds <= (unsigned) sg_maxQueue)
            pthread_cond_signal (&acceptdata.cond_nfds);
        new_sd =
            fds_poll_recv (fds, -1, 1,
                           event_wake_recv);
#ifdef _WIN32
        ResetEvent (event_wake_recv);
#else
        if (!fds->nfds)
        {
            /* at least the dummy/sync pipe should have remained */
            rzb_log (LOG_ERR, "All recv() descriptors gone: fatal");
            pthread_mutex_lock (&exit_mutex);
            progexit = 1;
            pthread_mutex_unlock (&exit_mutex);
            pthread_mutex_unlock (fds->buf_mutex);
            break;
        }
#endif
        if (new_sd == -1 && errno != EINTR)
        {
            rzb_log (LOG_ERR, "Failed to poll sockets, fatal");
            pthread_mutex_lock (&exit_mutex);
            progexit = 1;
            pthread_mutex_unlock (&exit_mutex);
        }


        if (fds->nfds)
            i = (rr_last + 1) % fds->nfds;
        for (j = 0; j < fds->nfds && new_sd >= 0;
             j++, i = (i + 1) % fds->nfds)
        {
            size_t pos = 0;
            int error = 0;
            struct fd_buf *buf = &fds->buf[i];
            if (!buf->got_newdata)
                continue;

#ifndef _WIN32
            if (buf->fd == acceptdata.syncpipe_wake_recv[0])
            {
                /* dummy sync pipe, just to wake us */
                if (read (buf->fd, buff, sizeof (buff)) < 0)
                {
                    rzb_log (LOG_WARNING, "Syncpipe read failed");
                }
                continue;
            }
#endif
            if (buf->got_newdata == -1)
            {
                if (buf->mode == MODE_WAITREPLY)
                {
                    rzb_log (LOG_DEBUG, "mode WAIT_REPLY -> closed");
                    buf->fd = -1;
                    thrmgr_group_terminate (buf->group);
                    thrmgr_group_finished (buf->group, EXIT_ERROR);
                    continue;
                }
                else
                {
                    rzb_log (LOG_DEBUG, "client read error or EOF on read");
                    error = 1;
                }
            }

            if (buf->fd != -1 && buf->got_newdata == -2)
            {
                rzb_log (LOG_DEBUG, "Client read timed out");
                mdprintf (buf->fd, "COMMAND READ TIMED OUT");
                error = 1;
            }

            rr_last = i;
            if (buf->mode == MODE_WAITANCILL)
            {
                buf->mode = MODE_COMMAND;
                rzb_log (LOG_DEBUG, "mode -> MODE_COMMAND");
            }
            while (!error && buf->fd != -1 && buf->buffer && pos < buf->off &&
                   buf->mode != MODE_WAITANCILL)
            {
                client_conn_t conn;
                const char *cmd = NULL;
                int rc;
                /* New data available to read on socket. */

                memset (&conn, 0, sizeof (conn));
                conn.scanfd = buf->recvfd;
                buf->recvfd = -1;
                conn.sd = buf->fd;
                conn.options = options;
                conn.thrpool = thr_pool;
                conn.group = buf->group;
                conn.id = buf->id;
                conn.quota = buf->quota;
                conn.filename = buf->dumpname;
                conn.mode = buf->mode;
                conn.term = buf->term;

                /* Parse & dispatch command */
                cmd =
                    parse_dispatch_cmd (&conn, buf, &pos, &error, 
                                        readtimeout);

                if (conn.mode == MODE_COMMAND && !cmd)
                    break;
                if (!error)
                {
                    if (buf->mode == MODE_WAITREPLY && buf->off)
                    {
                        /* Client is not supposed to send anything more */
                        rzb_log (LOG_WARNING, "Client sent garbage after last command: %lu bytes", (unsigned long) buf->off);
                        buf->buffer[buf->off] = '\0';
                        rzb_log (LOG_DEBUG, "Garbage: %s", buf->buffer);
                        error = 1;
                    }
                    else if (buf->mode == MODE_STREAM)
                    {
                        rc = handle_stream (&conn, buf, &error, &pos,
                                            readtimeout);
                        if (rc == -1)
                            break;
                        else
                            continue;
                    }
                }
                if (error)
                {
                    conn_reply_error (&conn, "Error processing command.");
                }
            }
            if (error)
            {
                if (buf->dumpfd != -1)
                {
                    close (buf->dumpfd);
                    if (buf->dumpname)
                    {
                        //cli_unlink (buf->dumpname);
                        free (buf->dumpname);
                    }
                    buf->dumpfd = -1;
                }
                thrmgr_group_terminate (buf->group);
                if (thrmgr_group_finished (buf->group, EXIT_ERROR))
                {
                    rzb_log (LOG_DEBUG, "Shutting down socket after error (FD %d)",
                          buf->fd);
                    //XXX
                    shutdown (buf->fd, 2);
                    close (buf->fd);
                }
                else
                    rzb_log (LOG_DEBUG, "Socket not shut down due to active tasks");
                buf->fd = -1;
            }
        }
        pthread_mutex_unlock (fds->buf_mutex);

        /* handle progexit */
        pthread_mutex_lock (&exit_mutex);
        if (progexit)
        {
            pthread_mutex_unlock (&exit_mutex);
            pthread_mutex_lock (fds->buf_mutex);
            for (i = 0; i < fds->nfds; i++)
            {
                if (fds->buf[i].fd == -1)
                    continue;
                thrmgr_group_terminate (fds->buf[i].group);
                if (thrmgr_group_finished (fds->buf[i].group, EXIT_ERROR))
                {
                    rzb_log (LOG_DEBUG, "Shutdown closed fd %d", fds->buf[i].fd);
                    shutdown (fds->buf[i].fd, 2);
                    close (fds->buf[i].fd);
                    fds->buf[i].fd = -1;
                }
            }
            pthread_mutex_unlock (fds->buf_mutex);
            break;
        }
        pthread_mutex_unlock (&exit_mutex);

    }

    pthread_mutex_lock (&exit_mutex);
    progexit = 1;
    pthread_mutex_unlock (&exit_mutex);
#ifdef _WIN32
    SetEvent (event_wake_accept);
#else
    if (write (acceptdata.syncpipe_wake_accept[1], "", 1) < 0)
    {
        rzb_log (LOG_WARNING, "Write to syncpipe failed");
    }
#endif
    /* Destroy the thread manager.
     * This waits for all current tasks to end
     */
    rzb_log (LOG_DEBUG, "Waiting for all threads to finish");
    thrmgr_destroy (thr_pool);
#ifdef FANOTIFY
    if (optget (opts, "ScanOnAccess")->enabled)
    {
        rzb_log (LOG_ERR, "Stopping on-access scan");
        pthread_kill (fan_pid, SIGUSR1);
        pthread_join (fan_pid, NULL);
    }
#endif

    pthread_join (accept_th, NULL);
    fds_free (fds);

#ifdef _WIN32
    CloseHandle (event_wake_accept);
    CloseHandle (event_wake_recv);
#else
    close (acceptdata.syncpipe_wake_accept[1]);
    close (acceptdata.syncpipe_wake_recv[1]);
#endif

    rzb_log (LOG_DEBUG, "Shutting down the main socket%s.", (nsockets > 1) ? "s" : "");
    for (i = 0; i < nsockets; i++)
        close (socketds[i]);

#if 0
    if ((opt = optget (opts, "PidFile"))->enabled)
    {
        if (unlink (opt->strarg) == -1)
            rzb_log (LOG_ERR, "!Can't unlink the pid file %s", opt->strarg);
        else
            rzb_log (LOG_ERR, "Pid file removed.");
    }
#endif


    return ret;
}
