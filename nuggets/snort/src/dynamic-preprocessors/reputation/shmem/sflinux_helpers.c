/* $Id$ */
/****************************************************************************
 *
 * Copyright (C) 2005-2011 Sourcefire, Inc.
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

// @file    sflinux_helpers.c
// @author  Pramod Chandrashekar <pramod@sourcefire.com>

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "shmem_common.h"

int CheckNumaNodes()
{
   char filename[1024];
   int  num_nodes = 0;
   struct dirent *de;
   DIR *dir;

   snprintf(filename, sizeof(filename), "/sys/devices/system/node");

   if ((dir = opendir(filename)))
   {
       while ((de = readdir(dir)))
       {
           if (!de->d_name || strncmp(de->d_name, "node", 4) != 0)
               continue;
           num_nodes++;
       }
   }
   closedir(dir);

   DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
       "Number of numa nodes is %d\n",num_nodes););

   return num_nodes;
}

