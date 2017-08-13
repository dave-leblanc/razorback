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

#include "others.h"
#include "server.h"
#include "tcpserver.h"

#include <razorback/log.h>
#include <razorback/socket.h>
static struct Socket *ssocket = NULL;
int
tcpserver (const char *address, const uint16_t port)
{
    if ((ssocket = Socket_Listen((uint8_t *)address, port)) == NULL)
    {
        rzb_log(LOG_ERR, "Failed to listen on socket");
        return -1;
    }
    return ssocket->iSocket;
}
