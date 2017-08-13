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

// @file    shmem_datamgmt.c
// @author  Pramod Chandrashekar <pramod@sourcefire.com>

#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>

#include "shmem_config.h"
#include "shmem_common.h"

static const char* const MODULE_NAME = "ShmemFileMgmt";

// FIXTHIS eliminate these globals
ShmemDataFileList **filelist_ptr = NULL;
int file_count = 0;

static int StringCompare(const void *elem1, const void *elem2)
{
    ShmemDataFileList * const *a = elem1;
    ShmemDataFileList * const *b = elem2;

    return strcmp((*a)->filename,(*b)->filename);
}

static int AllocShmemDataFileList()
{
    if ((filelist_ptr = (ShmemDataFileList**)
        realloc(filelist_ptr,(file_count + FILE_LIST_BUCKET_SIZE)*
            sizeof(ShmemDataFileList*))) == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
            "Cannot allocate memory to store shmem data files\n"););
        return SF_ENOMEM;
    }
    return SF_SUCCESS;
}

static void FreeShmemDataFileListFiles()
{
    int i;

    if (!filelist_ptr)
        return;

    for(i = 0; i < file_count; i++)
    {
        free(filelist_ptr[i]->filename);
        free(filelist_ptr[i]);
    }
    file_count = 0;
}

static int ReadShmemDataFiles()
{
    char   filename[MAX_PATH];
    struct dirent *de;
    DIR    *dd;
    int    max_files  = MAX_FILES;
    char   *ext_end   = NULL;
    int    type       = 0;
    int    counter    = 0;
    int    startup    = 1;

    FreeShmemDataFileListFiles();

    if ((dd = opendir(shmusr_ptr->path)) == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
            "Could not open %s to read IPRep data files\n",shmusr_ptr->path););
        return SF_EINVAL;
    }
    while ((de = readdir(dd)) != NULL && max_files)
    {
        //DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Files are %s\n",de->d_name););
        if (strstr(de->d_name, ".blf") || strstr(de->d_name, ".wlf"))
        {
            //no need to check for NULL, established there is a period in strstr
            ext_end = (char*)strrchr(de->d_name,'.'); 

            if (strncmp(ext_end,".blf",4) == 0)
                type = BLACK_LIST;
            else if (strncmp(ext_end,".wlf",4) == 0)
                type = WHITE_LIST;

            if (type == 0) continue;
            
            counter++;

            if (startup || counter == FILE_LIST_BUCKET_SIZE)
            {
                startup=0;
                counter=0;
                if (AllocShmemDataFileList())
                    return SF_ENOMEM;
            }    

            if ((filelist_ptr[file_count] = (ShmemDataFileList*)
                malloc(sizeof(ShmemDataFileList))) == NULL)
            {
                DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
                    "Cannot allocate memory to store file information\n"););
                return SF_ENOMEM;
            }
            snprintf(filename, sizeof(filename), "%s/%s", shmusr_ptr->path,de->d_name);
            filelist_ptr[file_count]->filename = strdup(filename);
            filelist_ptr[file_count]->filetype = type;
            max_files--;
            file_count++;
            type = 0;
        }
    }
    closedir(dd);
    return SF_SUCCESS;
}

int GetSortedListOfShmemDataFiles()
{
    int rval;

    if ((rval = ReadShmemDataFiles()) != SF_SUCCESS)
       return rval;

    qsort(filelist_ptr,file_count,sizeof(*filelist_ptr),StringCompare); 
    return rval;
}    

//valid version values are 1 through UINT_MAX
int GetLatestShmemDataSetVersionOnDisk(uint32_t* shmemVersion)
{
    unsigned long tmpVersion;
    FILE *fp;
    char line[MAX_PATH];
    char version_file[MAX_PATH];
    const char *const key = "VERSION"; 
    char* keyend_ptr      = NULL;

    snprintf(version_file, sizeof(version_file),
        "%s/%s",shmusr_ptr->path,VERSION_FILENAME);

    if ((fp = fopen(version_file, "r")) == NULL)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
            "Error opening file at: %s\n", version_file););
        return NO_FILE;
    }

    while (fgets(line,sizeof(line),fp))
    {
        char *strptr;
        if ( !strncmp(line,"#",1) )
            continue;
        if ( (strptr = strstr(line, key )) && (strptr == line) )
        {
            keyend_ptr  = line;
            keyend_ptr += strlen(key) + 1;
            tmpVersion  = strtoul(keyend_ptr,NULL,0);
            break;
        }
    }   

    if (!keyend_ptr)
    {
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
            "Invalid file format %s\n", version_file););
        return NO_FILE;
    }    

    if (tmpVersion > UINT_MAX) //someone tampers with the file
        *shmemVersion = 1; 
    else 
        *shmemVersion = (uint32_t)tmpVersion;

    fclose(fp);

    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
        "version information being returned is %u\n", *shmemVersion););

    return SF_SUCCESS;
}    

void PrintDataFiles()
{
    int i;

    if (file_count)
    {
        for (i=0;i<file_count;i++)
        {
            DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION,
                "File %s of type %d found \n",
                filelist_ptr[i]->filename, filelist_ptr[i]->filetype););
        }
    }
    return;
}

void FreeShmemDataFileList()
{
    FreeShmemDataFileListFiles();
    free(filelist_ptr);
    return;
}

