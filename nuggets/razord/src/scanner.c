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
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#ifndef	_WIN32
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <pthread.h>

#if defined(HAVE_READDIR_R_3) || defined(HAVE_READDIR_R_2)
#include <limits.h>
#include <stddef.h>
#endif

#include "razord.h"
#include "others.h"
#include "scanner.h"
#include "shared.h"
#include "thrmgr.h"
#include "server.h"

#include <razorback/log.h>
#include <razorback/api.h>
#include <razorback/block_pool.h>
#include <razorback/metadata.h>
#include <razorback/submission.h>


extern int progexit;
#define FILEBUFF 8192
#define BUFFSIZE 1024
int
scan_callback (struct stat *sb, char *filename, const char *msg,
               enum cli_ftw_reason reason, struct cli_ftw_cbdata *data)
{
    struct scan_cb_data *scandata = data->data;
    char *virname;
    int ret;
    int type = scandata->type;
    struct BlockPoolItem *item;
    uint32_t l_iSf_Flags =0 , l_iEnt_Flags = 0; // Return codes from Razorback

    /* detect disconnected socket, 
     * this should NOT detect half-shutdown sockets (SHUT_WR) */
    if (send (scandata->conn->sd, &ret, 0, 0) == -1 && errno != EINTR)
    {
        rzb_log (LOG_DEBUG, "Client disconnected while command was active!");
        thrmgr_group_terminate (scandata->conn->group);
        if (reason == visit_file)
            free (filename);
        return -1;
    }

    if (thrmgr_group_need_terminate (scandata->conn->group))
    {
        rzb_log (LOG_WARNING, "Client disconnected while scanjob was active");
        if (reason == visit_file)
            free (filename);
        return -1;
    }
    scandata->total++;
    switch (reason)
    {
    case error_mem:
        if (msg)
            rzb_log (LOG_ERR, "Memory allocation failed during cli_ftw() on %s", msg);
        else
            rzb_log (LOG_ERR, "Memory allocation failed during cli_ftw()");
        scandata->errors++;
        return -1;
    case error_stat:
        conn_reply_errno (scandata->conn, msg, "lstat() failed:");
        rzb_log (LOG_WARNING, "lstat() failed on: %s", msg);
        scandata->errors++;
        return 0;
    case warning_skipped_dir:
        rzb_log (LOG_WARNING, "Directory recursion limit reached, skipping %s", msg);
        return 0;
    case warning_skipped_link:
        rzb_log (LOG_DEBUG, "Skipping symlink: %s", msg);
        return 0;
    case warning_skipped_special:
        if (msg == scandata->toplevel_path)
            conn_reply (scandata->conn, msg, "Not supported file type",
                        "ERROR");
        rzb_log (LOG_DEBUG, "Not supported file type: %s", msg);
        return 0;
    case visit_directory_toplev:
        return 0;
    case visit_file:
        break;
    }

    if (sb && sb->st_size == 0)
    {                           /* empty file */
        if (msg == scandata->toplevel_path)
            conn_reply_single (scandata->conn, filename, "Empty file");
        free (filename);
        return 0;
    }

    if (type == TYPE_MULTISCAN)
    {
        client_conn_t *client_conn =
            (client_conn_t *) calloc (1, sizeof (struct client_conn_tag));
        if (client_conn)
        {
            client_conn->scanfd = -1;
            client_conn->sd = scandata->odesc;
            client_conn->filename = filename;
            client_conn->cmdtype = COMMAND_MULTISCANFILE;
            client_conn->term = scandata->conn->term;
            client_conn->options = scandata->options;
            client_conn->group = scandata->group;
        }
        else
        {
            rzb_log (LOG_ERR, "Can't allocate memory for client_conn");
            scandata->errors++;
            free (filename);
            return -1;
        }
        return 0;
    }

    if (access (filename, R_OK))
    {
        if (conn_reply (scandata->conn, filename, "Access denied.", "ERROR")
            == -1)
        {
            free (filename);
            return -1;
        }
        rzb_log (LOG_DEBUG, "Access denied: %s", filename);
        scandata->errors++;
        free (filename);
        return 0;
    }

    thrmgr_setactivetask (filename,
                          type == TYPE_MULTISCAN ? "MULTISCANFILE" : NULL);

    if ((item = RZB_DataBlock_Create (rzb_context)) == NULL)
    {
        rzb_log (LOG_ERR, "Failed to allocate new block");
        return -1;
    }

    if (!BlockPool_AddData_FromFile (item, filename, false))
		return -1;
    RZB_DataBlock_Finalize(item);
    RZB_DataBlock_Metadata_Filename(item, filename);
    ret = RZB_DataBlock_Submit (item, 0, &l_iSf_Flags, &l_iEnt_Flags);

    if (ret == RZB_SUBMISSION_ERROR)
    {
        scandata->errors++;
        if (conn_reply (scandata->conn, filename, "Razorback Error", "ERROR")
            == -1)
        {
            free (filename);
            return -1;
        }
        rzb_log (LOG_INFO, "%s: %s ERROR", filename, strerror (ret));
    }
    if (ret == RZB_SUBMISSION_NO_TYPE)
    {
       rzb_log (LOG_INFO, "%s: No Type", filename);
       free(filename);
       return 0;
    }

    thrmgr_setactivetask (NULL, NULL);

    if (thrmgr_group_need_terminate (scandata->conn->group))
    {
        free (filename);
        rzb_log (LOG_DEBUG, "Client disconnected while scanjob was active");
        return -1;
    }

    if ((l_iSf_Flags & SF_FLAG_BAD) != 0)
    {
        if (asprintf(&virname, "SF_0x%08x_Ent_0x%08x", l_iSf_Flags, l_iEnt_Flags) == -1)
        {
            free(filename);
            return -1;
        }
        scandata->infected++;
        if (conn_reply_virus (scandata->conn, filename, virname) == -1)
        {
            free (filename);
            return -1;
        }
        rzb_log (LOG_INFO, "%s: %s FOUND", filename, virname);
        free(virname);
    }
    else if (logok)
    {
        rzb_log (LOG_INFO, "%s: OK", filename);
    }

    free (filename);

    if (type == TYPE_SCAN)
    {
        /* virus -> break */
        return ret;
    }

    /* keep scanning always */
    return 0;
}

int
scan_pathchk (const char *path, struct cli_ftw_cbdata *data)
{
#if 0
    struct scan_cb_data *scandata = data->data;
    const struct optstruct *opt;
    struct stat statbuf;

    if ((opt = optget (scandata->opts, "ExcludePath"))->enabled)
    {
        while (opt)
        {
            if (match_regex (path, opt->strarg) == 1)
            {
                if (scandata->type != TYPE_MULTISCAN)
                    conn_reply_single (scandata->conn, path, "Excluded");
                return 1;
            }
            opt = (const struct optstruct *) opt->nextarg;
        }
    }

    if (!optget (scandata->opts, "CrossFilesystems")->enabled)
    {
        if (stat (path, &statbuf) == 0)
        {
            if (statbuf.st_dev != scandata->dev)
            {
                if (scandata->type != TYPE_MULTISCAN)
                    conn_reply_single (scandata->conn, path,
                                       "Excluded (another filesystem)");
                return 1;
            }
        }
    }
#endif
    return 0;
}
#if 0
int scanfd (const client_conn_t * conn, unsigned long int *scanned,
            int odesc, int stream)
{
    int ret, fd = conn->scanfd;
    const char *virname;
    struct stat statbuf;
    char fdstr[32];
    const char *reply_fdstr;

    if (stream)
    {
        struct sockaddr_in sa;
        socklen_t salen = sizeof (sa);
        if (getpeername (conn->sd, (struct sockaddr *) &sa, &salen)
            || salen > sizeof (sa) || sa.sin_family != AF_INET)
            strncpy (fdstr, "instream(local)", sizeof (fdstr));
        else
            snprintf (fdstr, sizeof (fdstr), "instream(%s@%u)",
                      inet_ntoa (sa.sin_addr), ntohs (sa.sin_port));
        reply_fdstr = "stream";
    }
    else
    {
        snprintf (fdstr, sizeof (fdstr), "fd[%d]", fd);
        reply_fdstr = fdstr;
    }
    if (fstat (fd, &statbuf) == -1 || !S_ISREG (statbuf.st_mode))
    {
        rzb_log (LOG_ERR, "%s: Not a regular file. ERROR\n", fdstr);
        if (conn_reply (conn, reply_fdstr, "Not a regular file", "ERROR") ==
            -1)
            return -1;
        return -1;
    }

    thrmgr_setactivetask (fdstr, NULL);
//    ret =
//        cl_scandesc_callback (fd, &virname, scanned, engine, options,
//                              &context);
    thrmgr_setactivetask (NULL, NULL);

    if (thrmgr_group_need_terminate (conn->group))
    {
        rzb_log (LOG_ERR, "*Client disconnected while scanjob was active\n");
        return ret == CL_ETIMEOUT ? ret : CL_BREAK;
    }

    if (ret == CL_VIRUS)
    {
        if (conn_reply_virus (conn, reply_fdstr, virname) == -1)
            ret = CL_ETIMEOUT;
        rzb_log (LOG_ERR, "%s: %s FOUND\n", fdstr, virname);
    }
    else if (ret != CL_CLEAN)
    {
        if (conn_reply (conn, reply_fdstr, strerror (ret), "ERROR") == -1)
            ret = CL_ETIMEOUT;
        rzb_log (LOG_ERR, "%s: %s ERROR\n", fdstr, strerror (ret));
    }
    else
    {
        if (conn_reply_single (conn, reply_fdstr, "OK") == CL_ETIMEOUT)
            ret = CL_ETIMEOUT;
        if (logok)
            rzb_log (LOG_ERR, "%s: OK\n", fdstr);
    }
    return ret;
}
#endif 

int scanstream (int odesc, unsigned long int *scanned,
                char term)
{
    int ret, sockfd, acceptd;
    int bread, retval, firsttimeout, timeout;
    unsigned int port = 0, portscan, min_port, max_port;
    //unsigned long int quota = 0, maxsize = 0;
    short bound = 0;
    char *virname;
    char buff[FILEBUFF];
    char peer_addr[32];
    struct sockaddr_in server;
    struct sockaddr_in peer;
    socklen_t addrlen;
    struct BlockPoolItem *item;
    uint32_t l_iSf_Flags, l_iEnt_Flags; // Return codes from Razorback
    uint8_t *tmp;


    min_port = sg_streamPortMin;
    max_port = sg_streamPortMax;

    /* search for a free port to bind to */
    port = cli_rndnum (max_port - min_port);
    bound = 0;
    for (portscan = 0; portscan < 1000; portscan++)
    {
        port = (port - 1) % (max_port - min_port + 1);

        memset ((char *) &server, 0, sizeof (server));
        server.sin_family = AF_INET;
        server.sin_port = htons (min_port + port);
        server.sin_addr.s_addr = htonl (INADDR_ANY);

        if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
            continue;

        if (bind
            (sockfd, (struct sockaddr *) &server,
             sizeof (struct sockaddr_in)) == -1)
            close (sockfd);
        else
        {
            bound = 1;
            break;
        }
    }
    port += min_port;

    timeout = sg_readTimeout;
    firsttimeout = sg_commandTimeout;

    if (!bound && !portscan)
    {
        rzb_log (LOG_ERR, "ScanStream: Can't find any free port.");
        mdprintf (odesc, "Can't find any free port. ERROR%c", term);
        close (sockfd);
        return -1;
    }
    else
    {
        listen (sockfd, 1);
        if (mdprintf (odesc, "PORT %u%c", port, term) <= 0)
        {
            rzb_log (LOG_ERR, "ScanStream: error transmitting port.");
            close (sockfd);
            return -1;
        }
    }

    retval = poll_fd (sockfd, firsttimeout, 0);
    if (!retval || retval == -1)
    {
        const char *reason = !retval ? "timeout" : "poll";
        mdprintf (odesc, "Accept %s. ERROR%c", reason, term);
        rzb_log (LOG_ERR, "ScanStream %u: accept %s.", port, reason);
        close (sockfd);
        return -1;
    }

    addrlen = sizeof (peer);
    if ((acceptd =
         accept (sockfd, (struct sockaddr *) &peer, &addrlen)) == -1)
    {
        close (sockfd);
        mdprintf (odesc, "accept() ERROR%c", term);
        rzb_log (LOG_ERR, "ScanStream %u: accept() failed.", port);
        return -1;
    }

    *peer_addr = '\0';
    inet_ntop (peer.sin_family, &peer.sin_addr, peer_addr,
               sizeof (peer_addr));
    rzb_log (LOG_DEBUG, "Accepted connection from %s on port %u, fd %d", peer_addr, port,
          acceptd);

    if ((item = RZB_DataBlock_Create (rzb_context)) == NULL)
    {
        rzb_log (LOG_ERR, "Failed to allocate new block");
        return -1;
    }

    while ((retval = poll_fd (acceptd, timeout, 0)) == 1)
    {
        /* only read up to max */
        bread = recv (acceptd, buff, FILEBUFF, 0);
        if (bread <= 0)
            break;
        if ((tmp = malloc(bread)) == NULL)
        {
            shutdown (sockfd, 2);
            close (sockfd);
            close (acceptd);
            return -1;
        }
        memcpy(tmp, buff, bread); 
        BlockPool_AddData(item, tmp, bread, BLOCK_POOL_DATA_FLAG_MALLOCD);

    }

    switch (retval)
    {
    case 0:                    /* timeout */
        mdprintf (odesc, "read timeout ERROR%c", term);
        rzb_log (LOG_ERR, "ScanStream(%s@%u): read timeout.", peer_addr, port);
        break;
    case -1:
        mdprintf (odesc, "read poll ERROR%c", term);
        rzb_log (LOG_ERR, "ScanStream(%s@%u): read poll failed.", peer_addr, port);
        break;
    }

    if (retval != 1)
    {
        close (acceptd);
        close (sockfd);
        return -1;
    }

    thrmgr_setactivetask (peer_addr, NULL);
    RZB_DataBlock_Finalize(item);
    ret = RZB_DataBlock_Submit (item, 0, &l_iSf_Flags, &l_iEnt_Flags);
    thrmgr_setactivetask (NULL, NULL);

    close (acceptd);
    close (sockfd);
    if (ret == RZB_SUBMISSION_ERROR)
    {
        if (retval == 1)
        {
            mdprintf (odesc, "stream: %s ERROR%c", strerror (ret), term);
            rzb_log (LOG_ERR, "stream(%s@%u): %s submission error", peer_addr, port,
                  strerror (ret));
        }
    }
    if (ret == RZB_SUBMISSION_NO_TYPE)
    {
            mdprintf (odesc, "stream: OK%c", term);
            rzb_log (LOG_ERR, "stream(%s@%u): submission with no data type", peer_addr, port);
            return 0;
    }
    if ((l_iSf_Flags & SF_FLAG_BAD) != 0)
    {
        if (asprintf(&virname, "SF_0x%08x_Ent_0x%08x", l_iSf_Flags, l_iEnt_Flags) == -1)
        {
            return -1;
        }
        mdprintf (odesc, "stream: %s FOUND%c", virname, term);
        rzb_log (LOG_ERR, "stream(%s@%u): %s FOUND", peer_addr, port, virname);
        free(virname);
    }
    else
    {
        mdprintf (odesc, "stream: OK%c", term);
        if (logok)
            rzb_log (LOG_ERR, "stream(%s@%u): OK", peer_addr, port);
    }

    return ret;
}
