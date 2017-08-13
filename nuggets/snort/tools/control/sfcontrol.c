/*
**  $Id$
**
**  sfcontrol.c
**
**  Copyright (C) 2002-2011 Sourcefire, Inc.
**  Author(s):  Ron Dempster <rdempster@sourcefire.com>
**
**  NOTES
**  5.5.11 - Initial Source Code. Dempster
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License Version 2 as
**  published by the Free Software Foundation.  You may not use, modify or
**  distribute this program under any other version of the GNU General
**  Public License.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "sfcontrol.h"

#ifndef PATH_MAX
#define PATH_MAX 4096  // FIXTHIS not needed in 292
#endif

static void DumpHex(FILE *fp, const uint8_t *data, unsigned len)
{
    char str[18];
    unsigned i;
    unsigned pos;
    char c;

    for (i=0, pos=0; i<len; i++, pos++)
    {
        if (pos == 17)
        {
            str[pos] = 0;
            fprintf(fp, "  %s\n", str);
            pos = 0;
        }
        else if (pos == 8)
        {
            str[pos] = ' ';
            pos++;
            fprintf(fp, "%s", " ");
        }
        c = (char)data[i];
        if (isprint(c) && (c == ' ' || !isspace(c)))
            str[pos] = c;
        else
            str[pos] = '.';
        fprintf(fp, "%02X ", data[i]);
    }
    if (pos)
    {
        str[pos] = 0;
        for (; pos < 17; pos++)
        {
            if (pos == 8)
            {
                str[pos] = ' ';
                pos++;
                fprintf(fp, "%s", "    ");
            }
            else
            {
                fprintf(fp, "%s", "   ");
            }
        }
        fprintf(fp, "  %s\n", str);
    }
}

static int SendMessage(int socket_fd, const CSMessageHeader *msg, ssize_t len)
{
    ssize_t numsent;
    unsigned total_len = sizeof(*msg) + len;
    unsigned total = 0;

    do
    {
        numsent = write(socket_fd, (*(uint8_t **)&msg) + total, total_len - total);
        if (!numsent)
            return 0;
        else if (numsent > 0)
            total += numsent;
        else if (errno != EINTR && errno != EAGAIN)
            return -1;
    } while (total < total_len);
    return 1;
}

static int ReadData(int socket_fd, uint8_t *buffer, uint32_t length)
{
    ssize_t numread;
    unsigned total = 0;

    do
    {
        numread = read(socket_fd, buffer + total, length - total);
        if (!numread)
            return 0;
        else if (numread > 0)
            total += numread;
        else if (errno != EINTR && errno != EAGAIN)
            return -1;
    } while (total < length);
    if (total < length)
        return 0;
    return 1;
}

static int ReadResponse(int socket_fd, CSMessageHeader *hdr)
{
    ssize_t numread;
    unsigned total = 0;

    do
    {
        numread = read(socket_fd, (*(uint8_t **)&hdr) + total, sizeof(*hdr) - total);
        if (!numread)
            return 0;
        else if (numread > 0)
            total += numread;
        else if (errno != EINTR && errno != EAGAIN)
            return -1;
    } while (total < sizeof(*hdr));
    if (total < sizeof(*hdr))
        return 0;

    hdr->length = ntohl(hdr->length);
    hdr->version = ntohs(hdr->version);
    hdr->type = ntohs(hdr->type);
    return 1;
}

void ConnectToUnixSocket(const char * const name, int * const psock)
{
    struct sockaddr_un sunaddr;
    int sock = -1;
    int rval;

    memset(&sunaddr, 0, sizeof(sunaddr));
    rval = snprintf(sunaddr.sun_path, sizeof(sunaddr.sun_path), "%s", name);
    if (rval < 0 || rval >= (int)sizeof(sunaddr.sun_path))
    {
        fprintf(stderr, "Socket name '%s' is too long\n", name);
        exit(-1);
    }

    sunaddr.sun_family = AF_UNIX;

    /* open the socket */
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Error opening socket: %s\n", strerror(errno));
        exit(-1);
    }

    if (connect(sock, (struct sockaddr *) &sunaddr, sizeof(sunaddr)) == -1)
    {
        fprintf(stderr, "Unable to connect to UNIX socket at %s: %s\n", name, strerror(errno));
        close(sock);
        exit(-1);
    }

    *psock = sock;
}

int main(int argc, char *argv[])
{
    int rval;
    char socket_fn[PATH_MAX];
    int socket_fd;
    char *p;
    CSMessageHeader msg;
    unsigned long type;
    const char *sep;
    ssize_t len;

    if (argc != 3 || !*argv[1] || !*argv[2])
    {
        fprintf(stderr, "Usage %s <snort log dir> <command>\n", argv[0]);
        exit(-1);
    }
    type = strtoul(argv[2], &p, 0);
    if (*p || type > CS_TYPE_MAX)
    {
        fprintf(stderr, "Usage %s <snort log dir> <command>\n", argv[0]);
        exit(-1);
    }
    len = strlen(argv[1]);
    if (len && argv[1][len - 1] == '/')
        sep = "";
    else
        sep = "/";
    snprintf(socket_fn, sizeof(socket_fn), "%s%s%s", argv[1], sep, CONTROL_FILE);
    ConnectToUnixSocket(socket_fn, &socket_fd);
    msg.version = htons(CS_HEADER_VERSION);
    msg.type = htons((uint16_t)type);
    msg.length = 0;
    if ((rval = SendMessage(socket_fd, &msg, 0)) < 0)
    {
        fprintf(stderr, "Failed to send the message: %s\n", strerror(errno));
        close(socket_fd);
        exit(-1);
    }
    else if (!rval)
    {
        fprintf(stderr, "Server closed the socket\n");
        close(socket_fd);
        exit(-1);
    }

    if ((rval = ReadResponse(socket_fd, &msg)) < 0)
    {
        fprintf(stderr, "Failed to read the response: %s\n", strerror(errno));
        close(socket_fd);
        exit(-1);
    }
    else if (!rval)
    {
        fprintf(stderr, "Server closed the socket before sending a response\n");
        close(socket_fd);
        exit(-1);
    }
    printf("Version %04X response %04X with %u bytes returned\n", msg.version, msg.type, msg.length);
    if (msg.version == CS_HEADER_VERSION && msg.length && msg.length <= 4096)
    {
        uint8_t data[4096];

        if ((rval = ReadData(socket_fd, data, msg.length)) < 0)
        {
            fprintf(stderr, "Failed to read the response data: %s\n", strerror(errno));
            close(socket_fd);
            exit(-1);
        }
        else if (!rval)
        {
            fprintf(stderr, "Server closed the socket before sending the response data\n");
            close(socket_fd);
            exit(-1);
        }
        DumpHex(stdout, data, msg.length);
    }
    return 0;
}

