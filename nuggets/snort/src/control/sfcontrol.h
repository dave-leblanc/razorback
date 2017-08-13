/****************************************************************************
 *
 * Copyright (C) 2011-2011 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#ifndef __SF_CONTROL_H__
#define __SF_CONTROL_H__

#define CONTROL_FILE    "SNORT.sock"

#define CS_TYPE_HUP_DAQ     0x0001
#define CS_TYPE_MAX         0x1FFF
#define CS_HEADER_VERSION   0x0001

typedef struct _CS_MESSAGE_HEADER
{
    /* All values must be in network byte order */
    uint16_t version;
    uint16_t type;
    uint32_t length;    /* Does not include the header */
} CSMessageHeader;

typedef int (*OOBPreControlFunc)(uint16_t type, const uint8_t *data, uint32_t length, void **new_context);
typedef int (*IBControlFunc)(uint16_t type, void *new_context, void **old_context);
typedef void (*OOBPostControlFunc)(uint16_t type, void *old_context);

#endif

