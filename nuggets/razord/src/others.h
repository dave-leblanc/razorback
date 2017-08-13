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

#ifndef __CLAMD_OTHERS_H
#define __CLAMD_OTHERS_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#if 0
#include "shared/optparser.h"
#endif
#include "thrmgr.h"
#if 0
#include "cltypes.h"
#endif
#include <razorback/socket.h>
#ifdef HAVE_POLL
#include <poll.h>
#endif

enum mode
{
    MODE_COMMAND,
    MODE_STREAM,
    MODE_WAITREPLY,
    MODE_WAITANCILL
};

struct fd_buf
{
    char *buffer;
    size_t bufsize;
    size_t off;
    int fd;
    char term;
    int got_newdata;            /* 0: no, 1: yes, -1: error */
    int recvfd;
    /* TODO: these fields don't belong here, there are identical fields in conn
     * too that don't belong there either. */
    enum mode mode;
    int id;
    int dumpfd;
    uint32_t chunksize;
    long quota;
    char *dumpname;
    time_t timeout_at;          /* 0 - no timeout */
    jobgroup_t *group;
};

struct fd_data
{
    pthread_mutex_t *buf_mutex; /* protects buf and nfds */
    struct fd_buf *buf;
    size_t nfds;
#ifdef HAVE_POLL
    struct pollfd *poll_data;
    size_t poll_data_nfds;
#endif
};

#ifdef HAVE_POLL
#define FDS_INIT(mutex) { (mutex), NULL, 0, NULL, 0}
#else
#define FDS_INIT(mutex) { (mutex), NULL, 0}
#endif

int poll_fd (int fd, int timeout_sec, int check_signals);
//void virusaction (const char *filename, const char *virname,
//                  const struct optstruct *opts);
int writen (int fd, void *buff, unsigned int count);
int fds_add (struct fd_data *data, int fd, int listen_only, int timeout);
void fds_remove (struct fd_data *data, int fd);
void fds_cleanup (struct fd_data *data);
int fds_poll_recv (struct fd_data *data, int timeout, int check_signals,
                   void *event);
void fds_free (struct fd_data *data);

#ifdef FANOTIFY
int fan_checkowner (int pid, const struct optstruct *opts);
#endif

#ifdef __GNUC__
int mdprintf(int desc, const char *str, ...) __attribute__((format(printf, 2,3)));
#else
int mdprintf(int desc, const char *str, ...);
#endif
int chomp(char *string);



/* symlink behaviour */
#define CLI_FTW_FOLLOW_FILE_SYMLINK 0x01
#define CLI_FTW_FOLLOW_DIR_SYMLINK  0x02

/* if the callback needs the stat */
#define CLI_FTW_NEED_STAT	    0x04

/* remove leading/trailing slashes */
#define CLI_FTW_TRIM_SLASHES	    0x08
#define CLI_FTW_STD (CLI_FTW_NEED_STAT | CLI_FTW_TRIM_SLASHES)

enum cli_ftw_reason {
    visit_file,
    visit_directory_toplev, /* this is a directory at toplevel of recursion */
    error_mem, /* recommended to return CL_EMEM */
    /* recommended to return CL_SUCCESS below */
    error_stat,
    warning_skipped_link,
    warning_skipped_special,
    warning_skipped_dir
};

/* wrap void*, so that we don't mix it with some other pointer */
struct cli_ftw_cbdata {
    void *data;
};

/* 
 * return CL_BREAK to break out without an error, CL_SUCCESS to continue,
 * or any CL_E* to break out due to error.
 * The callback is responsible for freeing filename when it is done using it.
 * Note that callback decides if directory traversal should continue 
 * after an error, we call the callback with reason == error,
 * and if it returns CL_BREAK we break.
 */
typedef int (*cli_ftw_cb)(struct stat *stat_buf, char *filename, const char *path, enum cli_ftw_reason reason, struct cli_ftw_cbdata *data);

/*
 * returns 1 if the path should be skipped and 0 otherwise
 * uses callback data
 */
typedef int (*cli_ftw_pathchk)(const char *path, struct cli_ftw_cbdata *data);

/*
 * returns 
 *  CL_SUCCESS if it traversed all files and subdirs
 *  CL_BREAK if traversal has stopped at some point
 *  CL_E* if error encountered during traversal and we had to break out
 * This is regardless of virus found/not, that is the callback's job to store.
 * Note that the callback may dispatch async the scan, so that when cli_ftw
 * returns we don't know the infected/notinfected status of the directory yet!
 * Due to this if the callback scans synchronously it should store the infected
 * status in its cbdata.
 * This works for both files and directories. It stats the path to determine
 * which one it is.
 * If it is a file, it simply calls the callback once, otherwise recurses.
 */
int cli_ftw(char *base, int flags, int maxdepth, cli_ftw_cb callback, struct cli_ftw_cbdata *data, cli_ftw_pathchk pathchk);

void cli_qsort(void *a, size_t n, size_t es, int (*cmp)(const void *, const void *));
unsigned int cli_rndnum(unsigned int max);

#endif

