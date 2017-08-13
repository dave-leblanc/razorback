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

#ifndef __SCANNER_H
#define __SCANNER_H

#define TYPE_SCAN	0
#define TYPE_CONTSCAN	1
#define TYPE_MULTISCAN	2

#include <sys/types.h>
#if 0
#include "libclamav/clamav.h"
#include "shared/optparser.h"
#endif
#include "thrmgr.h"
#include "session.h"
#include <razorback/socket.h>

struct scan_cb_data
{
    int scantype;
    int odesc;
    int type;
    int infected;
    int errors;
    int total;
    int id;
    const client_conn_t *conn;
    const char *toplevel_path;
    unsigned long scanned;
    unsigned int options;
    threadpool_t *thr_pool;
    jobgroup_t *group;
    dev_t dev;
};

//int scanfd (const client_conn_t * conn, unsigned long int *scanned,
//            int odesc, int stream);
int scanstream (int odesc, unsigned long int *scanned,
                char term);
int scan_callback (struct stat *sb, char *filename, const char *msg,
                   enum cli_ftw_reason reason, struct cli_ftw_cbdata *data);
int scan_pathchk (const char *path, struct cli_ftw_cbdata *data);
void hash_callback (int fd, unsigned long long size, const unsigned char *md5,
                    const char *virname, void *ctx);

#endif
