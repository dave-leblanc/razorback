/*
 *  Copyright (C) 2007-2009 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm, Trog, Török Edvin
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

/* must be first because it may define _XOPEN_SOURCE */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#ifndef	_WIN32
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include <pthread.h>
pthread_mutex_t mdprintf_mutex = PTHREAD_MUTEX_INITIALIZER;

#if HAVE_POLL
#if HAVE_POLL_H
#include <poll.h>
#else /* HAVE_POLL_H */
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */
#endif /* HAVE_POLL_H */
#endif /* HAVE_POLL */

#include <limits.h>

#include "session.h"
#include "others.h"
#include "razord.h"
#include <razorback/log.h>


/* Function: writen
	Try hard to write the specified number of bytes
*/
int
writen (int fd, void *buff, unsigned int count)
{
    int retval;
    unsigned int todo;
    unsigned char *current;

    todo = count;
    current = (unsigned char *) buff;

    do
    {
        retval = write (fd, current, todo);
        if (retval < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        todo -= retval;
        current += retval;
    }
    while (todo > 0);

    return count;
}

#ifdef HAVE_POLL
static int
realloc_polldata (struct fd_data *data)
{
    if (data->poll_data_nfds == data->nfds)
        return 0;
    if (data->poll_data)
        free (data->poll_data);
    data->poll_data = malloc (data->nfds * sizeof (*data->poll_data));
    if (!data->poll_data)
    {
        rzb_log (LOG_ERR,
                 "realloc_polldata: Memory allocation failed for poll_data");
        return -1;
    }
    data->poll_data_nfds = data->nfds;
    return 0;
}
#endif

int
poll_fd (int fd, int timeout_sec, int check_signals)
{
    int ret;
    struct fd_data fds = FDS_INIT (NULL);

    if (fds_add (&fds, fd, 1, timeout_sec) == -1)
        return -1;
    do
    {
        ret = fds_poll_recv (&fds, timeout_sec, check_signals, NULL);
    }
    while (ret == -1 && errno == EINTR);
    fds_free (&fds);
    return ret;
}

void
fds_cleanup (struct fd_data *data)
{
    struct fd_buf *newbuf;
    unsigned i, j;

    for (i = 0, j = 0; i < data->nfds; i++)
    {
        if (data->buf[i].fd < 0)
        {
            if (data->buf[i].buffer)
                free (data->buf[i].buffer);
            continue;
        }
        if (i != j)
            data->buf[j] = data->buf[i];
        j++;
    }
    if (j == data->nfds)
        return;
    for (i = j; i < data->nfds; i++)
        data->buf[i].fd = -1;
    data->nfds = j;
    rzb_log (LOG_DEBUG, "Number of file descriptors polled: %u fds",
             (unsigned) data->nfds);
    /* Shrink buffer */
    newbuf = realloc (data->buf, j * sizeof (*newbuf));
    if (!j)
        data->buf = NULL;
    else if (newbuf)
        data->buf = newbuf;     /* non-fatal if shrink fails */
}

static int
read_fd_data (struct fd_buf *buf)
{
    ssize_t n;

    buf->got_newdata = 1;
    if (!buf->buffer)           /* listen-only socket */
        return 1;

    if (buf->off >= buf->bufsize)
        return -1;

    /* Read the pending packet, it may contain more than one command, but
     * that is to the cmdparser to handle. 
     * It will handle 1st command, and then move leftover to beginning of buffer
     */
#ifdef HAVE_FD_PASSING
    {
        struct msghdr msg;
        struct cmsghdr *cmsg;
        union
        {
            unsigned char buff[CMSG_SPACE (sizeof (int))];
            struct cmsghdr hdr;
        } b;
        struct iovec iov[1];

        if (buf->recvfd != -1)
        {
            rzb_log (LOG_DEBUG, "Closing unclaimed FD: %d", buf->recvfd);
            close (buf->recvfd);
            buf->recvfd = -1;
        }
        memset (&msg, 0, sizeof (msg));
        iov[0].iov_base = buf->buffer + buf->off;
        iov[0].iov_len = buf->bufsize - buf->off;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_control = b.buff;
        msg.msg_controllen = sizeof (b.buff);

        n = recvmsg (buf->fd, &msg, 0);
        if (n < 0)
            return -1;
        if (msg.msg_flags & MSG_TRUNC)
        {
            rzb_log (LOG_WARNING, "Message truncated at %d bytes", (int) n);
            return -1;
        }
        if (msg.msg_flags & MSG_CTRUNC)
        {
            if (msg.msg_controllen > 0)
                rzb_log (LOG_WARNING,
                         "Control message truncated at %d bytes, %d data read",
                         (int) msg.msg_controllen, (int) n);
            else
                rzb_log (LOG_WARNING,
                         "Control message truncated, no control data received, %d bytes read"
#ifdef C_LINUX
                         "(Is SELinux/AppArmor enabled, and blocking file descriptor passing?)"
#endif
                         "", (int) n);
            return -1;
        }
        if (msg.msg_controllen)
        {
            for (cmsg = CMSG_FIRSTHDR (&msg); cmsg != NULL;
                 cmsg = CMSG_NXTHDR (&msg, cmsg))
            {
                if (cmsg->cmsg_len == CMSG_LEN (sizeof (int)) &&
                    cmsg->cmsg_level == SOL_SOCKET &&
                    cmsg->cmsg_type == SCM_RIGHTS)
                {
                    if (buf->recvfd != -1)
                    {
                        rzb_log (LOG_DEBUG,
                                 "Unclaimed file descriptor received. closing: %d",
                                 buf->recvfd);
                        close (buf->recvfd);
                    }
                    buf->recvfd = *(int *) CMSG_DATA (cmsg);
                    rzb_log (LOG_DEBUG, "Receveived a file descriptor: %d",
                             buf->recvfd);
                }
            }
        }
    }
#else
    n = recv (buf->fd, buf->buffer + buf->off, buf->bufsize - buf->off, 0);
    if (n < 0)
        return -1;
#endif
    buf->off += n;
    return n;
}

static int
buf_init (struct fd_buf *buf, int listen_only, int timeout)
{
    buf->off = 0;
    buf->got_newdata = 0;
    buf->recvfd = -1;
    buf->mode = MODE_COMMAND;
    buf->id = 0;
    buf->dumpfd = -1;
    buf->chunksize = 0;
    buf->quota = 0;
    buf->dumpname = NULL;
    buf->group = NULL;
    buf->term = '\0';
    if (!listen_only)
    {
        if (!buf->buffer)
        {
            buf->bufsize = PATH_MAX + 8;
            /* plus extra space for a \0 so we can make sure every command is \0
             * terminated */
            if (!(buf->buffer = malloc (buf->bufsize + 1)))
            {
                rzb_log (LOG_ERR,
                         "%s: Memory allocation failed for command buffer", __func__);
                return -1;
            }
        }
    }
    else
    {
        if (buf->buffer)
            free (buf->buffer);
        buf->bufsize = 0;
        buf->buffer = NULL;
    }
    if (timeout)
    {
        time (&buf->timeout_at);
        buf->timeout_at += timeout;
    }
    else
    {
        buf->timeout_at = 0;
    }
    return 0;
}

int
fds_add (struct fd_data *data, int fd, int listen_only, int timeout)
{
    struct fd_buf *buf;
    unsigned n;
    if (fd < 0)
    {
        rzb_log (LOG_ERR, "%s: invalid fd passed", __func__);
        return -1;
    }
    /* we may already have this fd, if
     * the old FD got closed, and the kernel reused the FD */
    for (n = 0; n < data->nfds; n++)
        if (data->buf[n].fd == fd)
        {
            /* clear stale data in buffer */
            if (buf_init (&data->buf[n], listen_only, timeout) < 0)
                return -1;
            return 0;
        }

    n++;
    buf = realloc (data->buf, n * sizeof (*buf));
    if (!buf)
    {
        rzb_log (LOG_ERR, "%s: Memory allocation failed", __func__);
        return -1;
    }
    data->buf = buf;
    data->nfds = n;
    data->buf[n - 1].buffer = NULL;
    if (buf_init (&data->buf[n - 1], listen_only, timeout) < 0)
        return -1;
    data->buf[n - 1].fd = fd;
    return 0;
}

static inline void
fds_lock (struct fd_data *data)
{
    if (data->buf_mutex)
        pthread_mutex_lock (data->buf_mutex);
}

static inline void
fds_unlock (struct fd_data *data)
{
    if (data->buf_mutex)
        pthread_mutex_unlock (data->buf_mutex);
}

void
fds_remove (struct fd_data *data, int fd)
{
    size_t i;
    fds_lock (data);
    if (data->buf)
    {
        for (i = 0; i < data->nfds; i++)
        {
            if (data->buf[i].fd == fd)
            {
                data->buf[i].fd = -1;
                break;
            }
        }
    }
    fds_unlock (data);
}

#define BUFFSIZE 1024
/* Wait till data is available to be read on any of the fds,
 * read available data on all fds, and mark them as appropriate.
 * One of the fds should be a pipe, used by the accept thread to wake us.
 * timeout is specified in seconds, if check_signals is non-zero, then
 * poll_recv_fds() will return upon receipt of a signal, even if no data
 * is received on any of the sockets.
 * Must be called with buf_mutex lock held.
 */
/* TODO: handle ReadTimeout */
int
fds_poll_recv (struct fd_data *data, int timeout, int check_signals,
               void *event)
{
    unsigned fdsok = data->nfds;
    size_t i;
    int retval;
    time_t now, closest_timeout;

    /* we must have at least one fd, the control fd! */
    fds_cleanup (data);
#ifndef _WIN32
    if (!data->nfds)
        return 0;
#endif
    for (i = 0; i < data->nfds; i++)
    {
        data->buf[i].got_newdata = 0;
    }

    time (&now);
    if (timeout > 0)
        closest_timeout = now + timeout;
    else
        closest_timeout = 0;
    for (i = 0; i < data->nfds; i++)
    {
        time_t timeout_at = data->buf[i].timeout_at;
        if (timeout_at && timeout_at < now)
        {
            /* timed out */
            data->buf[i].got_newdata = -2;
            /* we must return immediately from poll/select, we have a timeout! */
            closest_timeout = now;
        }
        else
        {
            if (!closest_timeout)
                closest_timeout = timeout_at;
            else if (timeout_at && timeout_at < closest_timeout)
                closest_timeout = timeout_at;
        }
    }
    if (closest_timeout)
        timeout = closest_timeout - now;
    else
        timeout = -1;
    if (timeout > 0)
        rzb_log (LOG_DEBUG, "fds_poll_recv: timeout after %d seconds",
                 timeout);
#ifdef HAVE_POLL
    /* Use poll() if available, preferred because:
     *  - can poll any number of FDs
     *  - can notify of both data available / socket disconnected events
     *  - when it says POLLIN it is guaranteed that a following recv() won't
     *  block (select may say that data is available to read, but a following 
     *  recv() may still block according to the manpage
     */

    if (realloc_polldata (data) == -1)
        return -1;
    if (timeout > 0)
    {
        /* seconds to ms */
        timeout *= 1000;
    }
    for (i = 0; i < data->nfds; i++)
    {
        data->poll_data[i].fd = data->buf[i].fd;
        data->poll_data[i].events = POLLIN;
        data->poll_data[i].revents = 0;
    }
    do
    {
        int n = data->nfds;

        fds_unlock (data);
#ifdef _WIN32
        retval = poll_with_event (data->poll_data, n, timeout, event);
#else
        retval = poll (data->poll_data, n, timeout);
#endif
        fds_lock (data);

        if (retval > 0)
        {
            fdsok = 0;
            /* nfds may change during poll, but not
             * poll_data_nfds */
            for (i = 0; i < data->poll_data_nfds; i++)
            {
                short revents;
                if (data->buf[i].fd < 0)
                    continue;
                if (data->buf[i].fd != data->poll_data[i].fd)
                {
                    /* should never happen */
                    rzb_log (LOG_ERR, "poll_recv_fds FD mismatch");
                    continue;
                }
                revents = data->poll_data[i].revents;
                if (revents & (POLLIN | POLLHUP))
                {
                    rzb_log (LOG_DEBUG, "Received POLLIN|POLLHUP on fd %d",
                             data->poll_data[i].fd);
                }
#ifndef _WIN32
                if (revents & POLLHUP)
                {
                    /* avoid SHUT_WR problem on Mac OS X */
                    int ret = send (data->poll_data[i].fd, &n, 0, 0);
                    if (!ret || (ret == -1 && errno == EINTR))
                        revents &= ~POLLHUP;
                }
#endif
                if (revents & POLLIN)
                {
                    int ret = read_fd_data (&data->buf[i]);
                    /* Data available to be read */
                    if (ret == -1)
                        revents |= POLLERR;
                    else if (!ret)
                        revents = POLLHUP;
                }

                if (revents & (POLLHUP | POLLERR | POLLNVAL))
                {
                    if (revents & (POLLHUP | POLLNVAL))
                    {
                        /* remote disconnected */
                        rzb_log (LOG_DEBUG, "Client disconnected (FD %d)",
                                 data->poll_data[i].fd);
                    }
                    else
                    {
                        /* error on file descriptor */
                        rzb_log (LOG_WARNING, "Error condition on fd %d",
                                 data->poll_data[i].fd);
                    }
                    data->buf[i].got_newdata = -1;
                }
                else
                {
                    fdsok++;
                }
            }
        }
    }
    while (retval == -1 && !check_signals && errno == EINTR);
#else
    {
        fd_set rfds;
        struct timeval tv;
        int maxfd = -1;

        for (i = 0; i < data->nfds; i++)
        {
            int fd = data->buf[i].fd;
            if (fd >= FD_SETSIZE)
            {
                rzb_log (LOG_ERR, "File descriptor is too high for FD_SET");
                return -1;
            }

            maxfd = MAX (maxfd, fd);
        }

        do
        {
            FD_ZERO (&rfds);
            for (i = 0; i < data->nfds; i++)
            {
                int fd = data->buf[i].fd;
                if (fd >= 0)
                    FD_SET (fd, &rfds);
            }
            tv.tv_sec = timeout;
            tv.tv_usec = 0;

            fds_unlock (data);
            retval =
                select (maxfd + 1, &rfds, NULL, NULL,
                        timeout >= 0 ? &tv : NULL);
            fds_lock (data);
            if (retval > 0)
            {
                fdsok = data->nfds;
                for (i = 0; i < data->nfds; i++)
                {
                    if (data->buf[i].fd < 0)
                    {
                        fdsok--;
                        continue;
                    }
                    if (FD_ISSET (data->buf[i].fd, &rfds))
                    {
                        int ret = read_fd_data (&data->buf[i]);
                        if (ret == -1 || !ret)
                        {
                            if (ret == -1)
                                rzb_log (LOG_ERR, "Error condition on fd %d",
                                         data->buf[i].fd);
                            else
                            {
                                /* avoid SHUT_WR problem on Mac OS X */
                                int ret = send (data->buf[i].fd, &i, 0, 0);
                                if (!ret || (ret == -1 && errno == EINTR))
                                    continue;
                                rzb_log (LOG_DEBUG, "Client disconnected");
                            }
                            data->buf[i].got_newdata = -1;
                        }
                    }
                }
            }
            if (retval < 0 && errno == EBADF)
            {
                /* unlike poll(),  select() won't tell us which FD is bad, so
                 * we have to check them one by one. */
                tv.tv_sec = 0;
                tv.tv_usec = 0;
                /* with tv == 0 it doesn't check for EBADF */
                FD_ZERO (&rfds);
                for (i = 0; i < data->nfds; i++)
                {
                    if (data->buf[i].fd == -1)
                        continue;
                    FD_SET (data->buf[i].fd, &rfds);
                    do
                    {
                        retval =
                            select (data->buf[i].fd + 1, &rfds, NULL, NULL,
                                    &tv);
                    }
                    while (retval == -1 && errno == EINTR);
                    if (retval == -1)
                    {
                        data->buf[i].fd = -1;
                    }
                    else
                    {
                        FD_CLR (data->buf[i].fd, &rfds);
                    }
                }
                retval = -1;
                errno = EINTR;
                continue;
            }
        }
        while (retval == -1 && !check_signals && errno == EINTR);
    }
#endif

    if (retval == -1 && errno != EINTR)
    {
#ifdef HAVE_POLL
        rzb_log (LOG_ERR, "poll_recv_fds: poll failed: %s",
                 strerror (errno));
#else
        rzb_log (LOG_ERR, "poll_recv_fds: select failed: %s",
                 strerror (errno));
#endif
    }

    return retval;
}

void
fds_free (struct fd_data *data)
{
    unsigned i;
    fds_lock (data);
    for (i = 0; i < data->nfds; i++)
    {
        if (data->buf[i].buffer)
        {
            free (data->buf[i].buffer);
        }
    }
    if (data->buf)
        free (data->buf);
#ifdef HAVE_POLL
    if (data->poll_data)
        free (data->poll_data);
#endif
    data->buf = NULL;
    data->nfds = 0;
    fds_unlock (data);
}


#ifdef FANOTIFY
int
fan_checkowner (int pid, const struct optstruct *opts)
{
    char path[32];
    struct stat sb;
    const struct optstruct *opt;

    if (!(opt = optget (opts, "OnAccessExcludeUID"))->enabled)
        return 0;

    snprintf (path, sizeof (path), "/proc/%u", pid);
    if (stat (path, &sb) == 0)
    {
        while (opt)
        {
            if (opt->numarg == (long long) sb.st_uid)
                return 1;
            opt = opt->nextarg;
        }
    }
    return 0;
}
#endif

#define ARGLEN(args, str, len)			    \
{						    \
	size_t arglen = 1, i;			    \
	char *pt;				    \
    va_start(args, str);			    \
    len = strlen(str);				    \
    for(i = 0; i < len - 1; i++) {		    \
	if(str[i] == '%') {			    \
	    switch(str[++i]) {			    \
		case 's':			    \
		    pt  = va_arg(args, char *);	    \
		    if(pt)			    \
			arglen += strlen(pt);	    \
		    break;			    \
		case 'f':			    \
		    va_arg(args, double);	    \
		    arglen += 25;		    \
		    break;			    \
		case 'l':			    \
		    va_arg(args, long);		    \
		    arglen += 20;		    \
		    break;			    \
		default:			    \
		    va_arg(args, int);		    \
		    arglen += 10;		    \
		    break;			    \
	    }					    \
	}					    \
    }						    \
    va_end(args);				    \
    len += arglen;				    \
}


int
mdprintf (int desc, const char *str, ...)
{
    va_list args;
    char buffer[512], *abuffer = NULL, *buff;
    int bytes, todo, ret = 0;
    size_t len;

    ARGLEN (args, str, len);
    if (len <= sizeof (buffer))
    {
        len = sizeof (buffer);
        buff = buffer;
    }
    else
    {
        abuffer = malloc (len);
        if (!abuffer)
        {
            len = sizeof (buffer);
            buff = buffer;
        }
        else
        {
            buff = abuffer;
        }
    }
    va_start (args, str);
    bytes = vsnprintf (buff, len, str, args);
    va_end (args);
    buff[len - 1] = 0;

    if (bytes < 0)
    {
        if (len > sizeof (buffer))
            free (abuffer);
        return bytes;
    }
    if ((size_t) bytes >= len)
        bytes = len - 1;

    todo = bytes;
    /* make sure we don't mix sends from multiple threads,
     * important for IDSESSION */
    pthread_mutex_lock (&mdprintf_mutex);
    while (todo > 0)
    {
        ret = send (desc, buff, bytes, 0);
        if (ret < 0)
        {
            struct timeval tv;
            if (errno != EWOULDBLOCK)
                break;
            /* didn't send anything yet */
            pthread_mutex_unlock (&mdprintf_mutex);
            tv.tv_sec = 0;
            tv.tv_usec = mprintf_send_timeout * 1000;
            do
            {
                fd_set wfds;
                FD_ZERO (&wfds);
                FD_SET (desc, &wfds);
                ret = select (desc + 1, NULL, &wfds, NULL, &tv);
            }
            while (ret < 0 && errno == EINTR);
            pthread_mutex_lock (&mdprintf_mutex);
            if (!ret)
            {
                /* timed out */
                ret = -1;
                break;
            }
        }
        else
        {
            todo -= ret;
            buff += ret;
        }
    }
    pthread_mutex_unlock (&mdprintf_mutex);

    if (len > sizeof (buffer))
        free (abuffer);

    return ret < 0 ? -1 : bytes;
}

int
chomp (char *string)
{
    int l;

    if (string == NULL)
        return -1;

    l = strlen (string);

    if (l == 0)
        return 0;

    --l;

    while ((l >= 0) && ((string[l] == '\n') || (string[l] == '\r')))
        string[l--] = '\0';

    return l + 1;
}

#define PATHSEP "/"
struct dirent_data
{
    char *filename;
    const char *dirname;
    struct stat *statbuf;
    long ino;                   /* -1: inode not available */
    int is_dir;                 /* 0 - no, 1 - yes */
};

enum filetype
{
    ft_unknown,
    ft_link,
    ft_directory,
    ft_regular,
    ft_skipped_special,
    ft_skipped_link
};
static inline int
ft_skipped (enum filetype ft)
{
    return ft != ft_regular && ft != ft_directory;
}
/* sort files before directories, and lower inodes before higher inodes */
static int ftw_compare(const void *a, const void *b)
{
    const struct dirent_data *da = a;
    const struct dirent_data *db = b;
    long diff = da->is_dir - db->is_dir;
    if (!diff) {
	diff = da->ino - db->ino;
    }
    return diff;
}

#define FOLLOW_SYMLINK_MASK (CLI_FTW_FOLLOW_FILE_SYMLINK | CLI_FTW_FOLLOW_DIR_SYMLINK)
static int
get_filetype (const char *fname, int flags, int need_stat,
              struct stat *statbuf, enum filetype *ft)
{
    int stated = 0;

    if (*ft == ft_unknown || *ft == ft_link)
    {
        need_stat = 1;

        if ((flags & FOLLOW_SYMLINK_MASK) != FOLLOW_SYMLINK_MASK)
        {
            /* Following only one of directory/file symlinks, or none, may
             * need to lstat.
             * If we're following both file and directory symlinks, we don't need
             * to lstat(), we can just stat() directly.*/
            if (*ft != ft_link)
            {
                /* need to lstat to determine if it is a symlink */
                if (lstat (fname, statbuf) == -1)
                    return -1;
                if (S_ISLNK (statbuf->st_mode))
                {
                    *ft = ft_link;
                }
                else
                {
                    /* It was not a symlink, stat() not needed */
                    need_stat = 0;
                    stated = 1;
                }
            }
            if (*ft == ft_link && !(flags & FOLLOW_SYMLINK_MASK))
            {
                /* This is a symlink, but we don't follow any symlinks */
                *ft = ft_skipped_link;
                return 0;
            }
        }
    }

    if (need_stat)
    {
        if (stat (fname, statbuf) == -1)
            return -1;
        stated = 1;
    }

    if (*ft == ft_unknown || *ft == ft_link)
    {
        if (S_ISDIR (statbuf->st_mode) &&
            (*ft != ft_link || (flags & CLI_FTW_FOLLOW_DIR_SYMLINK)))
        {
            /* A directory, or (a symlink to a directory and we're following dir
             * symlinks) */
            *ft = ft_directory;
        }
        else if (S_ISREG (statbuf->st_mode) &&
                 (*ft != ft_link || (flags & CLI_FTW_FOLLOW_FILE_SYMLINK)))
        {
            /* A file, or (a symlink to a file and we're following file symlinks) */
            *ft = ft_regular;
        }
        else
        {
            /* default: skipped */
            *ft = S_ISLNK (statbuf->st_mode) ?
                ft_skipped_link : ft_skipped_special;
        }
    }
    return stated;
}

static int
handle_filetype (const char *fname, int flags,
                 struct stat *statbuf, int *stated, enum filetype *ft,
                 cli_ftw_cb callback, struct cli_ftw_cbdata *data)
{
    int ret;

    *stated =
        get_filetype (fname, flags, flags & CLI_FTW_NEED_STAT, statbuf, ft);

    if (*stated == -1)
    {
        /*  we failed a stat() or lstat() */
        ret = callback (NULL, NULL, fname, error_stat, data);
        if (ret != 0)
            return ret;
        *ft = ft_unknown;
    }
    else if (*ft == ft_skipped_link || *ft == ft_skipped_special)
    {
        /* skipped filetype */
        ret = callback (stated ? statbuf : NULL, NULL, fname,
                        *ft == ft_skipped_link ?
                        warning_skipped_link : warning_skipped_special, data);
        if (ret != 0)
            return ret;
    }
    return 0;
}


static int cli_ftw_dir (const char *dirname, int flags, int maxdepth,
                        cli_ftw_cb callback, struct cli_ftw_cbdata *data,
                        cli_ftw_pathchk pathchk);
static int
handle_entry (struct dirent_data *entry, int flags, int maxdepth,
              cli_ftw_cb callback, struct cli_ftw_cbdata *data,
              cli_ftw_pathchk pathchk)
{
    if (!entry->is_dir)
    {
        return callback (entry->statbuf, entry->filename, entry->filename,
                         visit_file, data);
    }
    else
    {
        return cli_ftw_dir (entry->dirname, flags, maxdepth, callback, data,
                            pathchk);
    }
}

int
cli_ftw (char *path, int flags, int maxdepth, cli_ftw_cb callback,
         struct cli_ftw_cbdata *data, cli_ftw_pathchk pathchk)
{
    struct stat statbuf;
    enum filetype ft = ft_unknown;
    struct dirent_data entry;
    int stated = 0;
    int ret;

    if (((flags & CLI_FTW_TRIM_SLASHES) || pathchk) && path[0] && path[1])
    {
        char *pathend;
        /* trim slashes so that dir and dir/ behave the same when
         * they are symlinks, and we are not following symlinks */
#ifndef _WIN32
        while (path[0] == *PATHSEP && path[1] == *PATHSEP)
            path++;
#endif
        pathend = path + strlen (path);
        while (pathend > path && pathend[-1] == *PATHSEP)
            --pathend;
        *pathend = '\0';
    }
    if (pathchk && pathchk (path, data) == 1)
        return 0;
    ret =
        handle_filetype (path, flags, &statbuf, &stated, &ft, callback, data);
    if (ret != 0)
        return ret;
    if (ft_skipped (ft))
        return 0;
    entry.statbuf = stated ? &statbuf : NULL;
    entry.is_dir = ft == ft_directory;
    entry.filename = entry.is_dir ? NULL : strdup (path);
    entry.dirname = entry.is_dir ? path : NULL;
    if (entry.is_dir)
    {
        ret =
            callback (entry.statbuf, NULL, path, visit_directory_toplev,
                      data);
        if (ret != 0)
            return ret;
    }
    return handle_entry (&entry, flags, maxdepth, callback, data, pathchk);
}

static int
cli_ftw_dir (const char *dirname, int flags, int maxdepth,
             cli_ftw_cb callback, struct cli_ftw_cbdata *data,
             cli_ftw_pathchk pathchk)
{
    DIR *dd;
#if defined(HAVE_READDIR_R_3) || defined(HAVE_READDIR_R_2)
    union
    {
        struct dirent d;
        char b[offsetof (struct dirent, d_name) + NAME_MAX + 1];
    } result;
#endif
    struct dirent_data *entries = NULL;
    size_t i, entries_cnt = 0;
    int ret;

    if (maxdepth < 0)
    {
        /* exceeded recursion limit */
        ret = callback (NULL, NULL, dirname, warning_skipped_dir, data);
        return ret;
    }

    if ((dd = opendir (dirname)) != NULL)
    {
        struct dirent *dent;
        int err;
        errno = 0;
        ret = 0;
#ifdef HAVE_READDIR_R_3
        while (!(err = readdir_r (dd, &result.d, &dent)) && dent)
        {
#elif defined(HAVE_READDIR_R_2)
        while ((dent = (struct dirent *) readdir_r (dd, &result.d)))
        {
#else
        while ((dent = readdir (dd)))
        {
#endif
            int stated = 0;
            enum filetype ft;
            char *fname;
            struct stat statbuf;
            struct stat *statbufp;

            if (!strcmp (dent->d_name, ".") || !strcmp (dent->d_name, ".."))
                continue;
#ifdef _DIRENT_HAVE_D_TYPE
            switch (dent->d_type)
            {
            case DT_DIR:
                ft = ft_directory;
                break;
            case DT_LNK:
                if (!(flags & FOLLOW_SYMLINK_MASK))
                {
                    /* we don't follow symlinks, don't bother
                     * stating it */
                    errno = 0;
                    continue;
                }
                ft = ft_link;
                break;
            case DT_REG:
                ft = ft_regular;
                break;
            case DT_UNKNOWN:
                ft = ft_unknown;
                break;
            default:
                ft = ft_skipped_special;
                break;
            }
#else
            ft = ft_unknown;
#endif
            fname =
                (char *) malloc (strlen (dirname) +
                                     strlen (dent->d_name) + 2);
            if (!fname)
            {
                ret = callback (NULL, NULL, dirname, error_mem, data);
                if (ret != 0)
                    break;
            }
            if (!strcmp (dirname, PATHSEP))
                sprintf (fname, PATHSEP "%s", dent->d_name);
            else
                sprintf (fname, "%s" PATHSEP "%s", dirname, dent->d_name);

            if (pathchk && pathchk (fname, data) == 1)
            {
                free (fname);
                continue;
            }

            ret =
                handle_filetype (fname, flags, &statbuf, &stated, &ft,
                                 callback, data);
            if (ret != 0)
            {
                free (fname);
                break;
            }

            if (ft_skipped (ft))
            {                   /* skip */
                free (fname);
                errno = 0;
                continue;
            }

            if (stated && (flags & CLI_FTW_NEED_STAT))
            {
                statbufp = malloc (sizeof (*statbufp));
                if (!statbufp)
                {
                    ret =
                        callback (stated ? &statbuf : NULL, NULL, fname,
                                  error_mem, data);
                    free (fname);
                    if (ret != 0)
                        break;
                    else
                    {
                        errno = 0;
                        continue;
                    }
                }
                memcpy (statbufp, &statbuf, sizeof (statbuf));
            }
            else
            {
                statbufp = 0;
            }

            entries_cnt++;
            entries = realloc (entries, entries_cnt * sizeof (*entries));
            if (!entries)
            {
                ret =
                    callback (stated ? &statbuf : NULL, NULL, fname,
                              error_mem, data);
                free (fname);
                if (statbufp)
                    free (statbufp);
                break;
            }
            else
            {
                struct dirent_data *entry = &entries[entries_cnt - 1];
                entry->filename = fname;
                entry->statbuf = statbufp;
                entry->is_dir = ft == ft_directory;
                entry->dirname = entry->is_dir ? fname : NULL;
#ifdef _XOPEN_UNIX
                entry->ino = dent->d_ino;
#else
                entry->ino = -1;
#endif
            }
            errno = 0;
        }
#ifndef HAVE_READDIR_R_3
        err = errno;
#endif
        closedir (dd);
        ret = 0;
        if (err)
        {
            rzb_log (LOG_ERR, "Unable to readdir() directory %s: %s", dirname,
                        strerror (errno));
            /* report error to callback using error_stat */
            ret = callback (NULL, NULL, dirname, error_stat, data);
            if (ret != 0)
            {
                if (entries)
                {
                    for (i = 0; i < entries_cnt; i++)
                    {
                        struct dirent_data *entry = &entries[i];
                        free (entry->filename);
                        free (entry->statbuf);
                    }
                    free (entries);
                }
                return ret;
            }
        }

        if (entries)
        {
            cli_qsort (entries, entries_cnt, sizeof (*entries), ftw_compare);
            for (i = 0; i < entries_cnt; i++)
            {
                struct dirent_data *entry = &entries[i];
                ret =
                    handle_entry (entry, flags, maxdepth - 1, callback, data,
                                  pathchk);
                if (entry->is_dir)
                    free (entry->filename);
                if (entry->statbuf)
                    free (entry->statbuf);
                if (ret != 0)
                    break;
            }
            for (i++; i < entries_cnt; i++)
            {
                struct dirent_data *entry = &entries[i];
                free (entry->filename);
                free (entry->statbuf);
            }
            free (entries);
        }
    }
    else
    {
        ret = callback (NULL, NULL, dirname, error_stat, data);
    }
    return ret;
}

unsigned int cli_rndnum(unsigned int max)
{
    struct timeval tv;
	gettimeofday(&tv, (struct timezone *) 0);
	srand(tv.tv_usec+clock()+rand());
    return 1 + (unsigned int) (max * (rand() / (1.0 + RAND_MAX)));
}

