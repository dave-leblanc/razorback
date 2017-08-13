/* $Id$ */
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

// @file    shmem_datamgmt.h
// @author  Pramod Chandrashekar <pramod@sourcefire.com>

#ifndef _SHMEM_DMGMT_H_
#define _SHMEM_DMGMT_H_

#include <stdint.h>

#define SF_EINVAL  1
#define SF_SUCCESS 0
#define SF_ENOMEM  2
#define SF_EEXIST  3

#define MAX_PATH   512
#define MAX_NAME  1024  
#define MAX_FILES 1024

#define FILE_LIST_BUCKET_SIZE 100

typedef struct _FileList
{
    char*    filename;
    int      filetype;
} ShmemDataFileList;

extern ShmemDataFileList** filelist_ptr;
extern int file_count;

int GetSortedListOfShmemDataFiles(void);
int GetLatestShmemDataSetVersionOnDisk(uint32_t* shmemVersion);
void FreeShmemDataFileList(void);
void PrintDataFiles(void);
#endif

