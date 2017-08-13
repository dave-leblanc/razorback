/****************************************************************************
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
 ****************************************************************************
 * Provides convenience functions for parsing and querying configuration.
 *
 * 6/7/2011 - Initial implementation ... Hui Cao <hcao@sourcefire.com>
 *
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include "sf_snort_packet.h"
#include "sf_types.h"
#include "sfPolicy.h"
#include "sfPolicyUserData.h"
#include "reputation_config.h"
#include "spp_reputation.h"
#include "reputation_debug.h"
#include "reputation_utils.h"
#ifdef SHARED_REP
#include "./shmem/shmem_mgmt.h"
#include <sys/stat.h>
#endif
enum
{
    IP_INSERT_SUCCESS = 0,
    IP_INVALID,
    IP_INSERT_FAILURE,
    IP_INSERT_DUPLICATE,
    IP_MEM_ALLOC_FAILURE
};


/*
 * Default values for configurable parameters.
 */
#define REPUTATION_DEFAULT_MEMCAP          500 /*Mega bytes*/
#define REPUTATION_DEFAULT_REFRESH_PERIOD  60  /*60 seconds*/


/*
 * Min/Max values for each configurable parameter.
 */
#define MIN_MEMCAP 1
#define MAX_MEMCAP 4095
#define MIN_SHARED_MEM_REFRESH_PERIOD 1
#define MAX_SHARED_MEM_REFRESH_PERIOD UINT32_MAX

#define MAX_ADDR_LINE_LENGTH    8192

/*
 * Keyword strings for parsing configuration options.
 */
#define REPUTATION_MEMCAP_KEYWORD        "memcap"
#define REPUTATION_SCANLOCAL_KEYWORD     "scan_local"
#define REPUTATION_BLACKLIST_KEYWORD     "blacklist"
#define REPUTATION_WHITELIST_KEYWORD     "whitelist"
#define REPUTATION_PRIORITY_KEYWORD      "priority"
#define REPUTATION_NESTEDIP_KEYWORD      "nested_ip"
#define REPUTATION_SHAREMEM_KEYWORD      "shared_mem"
#define REPUTATION_SHAREDREFRESH_KEYWORD "shared_refresh"

#define REPUTATION_CONFIG_SECTION_SEPERATORS     ",;"
#define REPUTATION_CONFIG_VALUE_SEPERATORS       " "
#define REPUTATION_SEPARATORS                " \t\r\n"


static bw_list black = {BLACKLISTED};
static bw_list white = {WHITELISTED};

char* NestedIPKeyword[] =
{
        "inner",
        "outer",
        "both",
        NULL
};

#define MAX_MSGS_TO_PRINT      20
static uint32_t num_duplicates;

void **IPtables;
table_flat_t *emptyIPtables;
#ifdef SHARED_REP
ReputationConfig *reputation_shmem_config;
#endif
/*
 * Function prototype(s)
 */
static void IpListInit(uint32_t,ReputationConfig *config);
static void LoadListFile(char *filename, INFO info, ReputationConfig *config);
static void DisplayReputationConfig(ReputationConfig *);

/* ********************************************************************
 * Function: estimateSizeFromEntries
 *
 * Estimate the memory segment size based on number of entries and memcap.
 *
 * Arguments:
 *
 * uint32_t num_entries: number of entries.
 * uint32_t the memcap value set in configuration
 *
 * RETURNS: estimated memory size.
 *********************************************************************/
uint32_t estimateSizeFromEntries(uint32_t num_entries, uint32_t memcap)
{
    uint64_t size;
    uint64_t sizeFromEntries;

    /*memcap value is in Megabytes*/
    size = memcap  << 20;

    if (size > UINT32_MAX)
        size = UINT32_MAX;

    /*Worst case,  15k ~ 2^14 per entry, plus one Megabytes for empty table*/
    if (num_entries > ((UINT32_MAX - (1 << 20))>> 14))
        sizeFromEntries = UINT32_MAX;
    else
        sizeFromEntries = (num_entries  << 14) + (1 << 20);

    if (size > sizeFromEntries)
    {
        size = sizeFromEntries;
    }

    return size;
}
#ifdef SHARED_REP
/****************************************************************************
 *
 * Function: CheckIPlistDir()
 *
 * Purpose: We only check if IP list directory exist and
 *          readable
 * Arguments: None.
 *
 * Returns:
 *   0 : fail
 *   1 : success
 *
 ****************************************************************************/
static int CheckIPlistDir(char *path)
{
   struct stat st;

   if (path == NULL)
       return 0;

   if (stat(path, &st) == -1)
       return 0;

   if (!S_ISDIR(st.st_mode) || (access(path, R_OK) == -1))
   {
      return 0;
   }
   return 1;
}

/* ********************************************************************
 * Function: LoadFileIntoShmem
 *
 * Call back function for shared memory
 * This is called when new files in the list
 * Arguments:
 *
 * void* ptrSegment: start of shared memory segment.
 * ShmemDataFileList** file_list: the list of whitelist/blacklist files
 * int num_files: number of files
 *
 * RETURNS:
 *     0: success
 *     other value fails
 *********************************************************************/

int LoadFileIntoShmem(void* ptrSegment, ShmemDataFileList** file_list, int num_files)
{
    table_flat_t *table;
    int i;
    MEM_OFFSET black_ptr;
    MEM_OFFSET white_ptr;
    uint8_t *base;

    segment_meminit((uint8_t*)ptrSegment, reputation_shmem_config->memsize);

#ifdef SUP_IP6
    /*DIR_16x7_4x4 for performance, but memory usage is high
     *Use  DIR_8x16 worst case IPV4 5K, IPV6 15K (bytes)
     *Use  DIR_16x7_4x4 worst case IPV4 500, IPV6 2.5M
     */
    table = sfrt_flat_new(DIR_8x16, IPv6, reputation_shmem_config->numEntries, reputation_shmem_config->memcap);
#else
    table = sfrt_flat_new(DIR_8x4, IPv4, reputation_shmem_config->numEntries, reputation_load_config->memcap);

#endif
    if (table == NULL)
    {
        DynamicPreprocessorFatalMessage("%s(%d): Failed to create IP list.\n",
                *(_dpd.config_file), *(_dpd.config_line));
    }

    reputation_shmem_config->iplist = table;
    base = (uint8_t *)ptrSegment;

    black_ptr = segment_malloc(sizeof(bw_list));
    white_ptr = segment_malloc(sizeof(bw_list));
    *((bw_list *)&base[black_ptr]) = black;
    *((bw_list *)&base[white_ptr]) = white;

    reputation_shmem_config->memCapReached = false;

    /*Reset the log message count*/
    num_duplicates = 0;
    for (i = 0; i < num_files; i++)
    {
        if (BLACK_LIST == file_list[i]->filetype)
            LoadListFile(file_list[i]->filename,black_ptr, reputation_shmem_config);
        else if (WHITE_LIST == file_list[i]->filetype)
            LoadListFile(file_list[i]->filename,white_ptr, reputation_shmem_config);

    }
    if (num_duplicates > MAX_MSGS_TO_PRINT)
        _dpd.logMsg("    Additional address has been redefined but not printed.\n");

    reputation_stats.memoryAllocated = sfrt_flat_usage(reputation_shmem_config->iplist);
    reputation_shmem_config->numEntries = sfrt_flat_num_entries(reputation_shmem_config->iplist);
    _dpd.logMsg("Reputation shared memory usage: %u bytes, total entries: %u\n",
            reputation_stats.memoryAllocated,reputation_shmem_config->numEntries );
    return 0;
}

/* ********************************************************************
 * Function: GetSegmentSizeFromFileList
 *
 * Call back function for shared memory
 * This is called when new files in the list
 *
 * Arguments:
 *
 * ShmemDataFileList** file_list: the list of whitelist/blacklist files
 * int num_files: number of files
 *
 * RETURNS:
 *    uint32_t: segment size
 *********************************************************************/
uint32_t GetSegmentSizeFromFileList(ShmemDataFileList** file_list, int file_count)
{
    int numlines;
    int totalLines = 0;
    int i;

    if (file_count == 0)
    {
        return ZEROSEG;
    }
    for (i = 0; i < file_count; i++)
    {
        errno = 0;
        numlines = numLinesInFile(file_list[i]->filename);
        if ((0 == numlines) && (0 != errno))
        {
            char errBuf[STD_BUF];
#ifdef WIN32
            snprintf(errBuf, STD_BUF, "%s", strerror(errno));
#else
            strerror_r(errno, errBuf, STD_BUF);
#endif
            DynamicPreprocessorFatalMessage( "Unable to open address file %s, Error: %s\n",
                    file_list[i]->filename, errBuf);
        }

        if (totalLines + numlines < totalLines)
        {
            DynamicPreprocessorFatalMessage("Too many entries.\n");
        }

        totalLines += numlines;
    }

    if (totalLines == 0)
    {
        return ZEROSEG;
    }
    reputation_shmem_config->numEntries = totalLines + 1;

    reputation_shmem_config->memsize =  estimateSizeFromEntries(reputation_shmem_config->numEntries, reputation_shmem_config->memcap);
    return reputation_shmem_config->memsize;
}

/* ********************************************************************
 * Function: InitPerProcessZeroSegment
 *
 * Call back function for shared memory
 * This is called during initialization
 *
 * Arguments:
 *
 * void*** data_ptr: (output) the address of shared memory address
 *
 * RETURNS:
 *    uint32_t: segment size
 *********************************************************************/
int InitPerProcessZeroSegment(void*** data_ptr)
{
    /*The size of empty segment is 1 Megabytes*/
    size_t size = 1;
    long maxEntries = 1;
    static bool initiated = false;

    if (true == initiated)
    {
        *data_ptr = (void **)&emptyIPtables;
        return 0;
    }
    reputation_shmem_config->emptySegment = malloc(size*1024*1024);
    segment_meminit((uint8_t*) reputation_shmem_config->emptySegment, size*1024*1024);

    initiated = true;

#ifdef SUP_IP6
    /*DIR_16x7_4x4 for performance, but memory usage is high
     *Use  DIR_8x16 worst case IPV4 5K, IPV6 15K (bytes)
     *Use  DIR_16x7_4x4 worst case IPV4 500, IPV6 2.5M
     */
    emptyIPtables = sfrt_flat_new(DIR_8x16, IPv6, maxEntries, size);
#else
    emptyIPtables = sfrt_flat_new(DIR_8x4, IPv4,  maxEntries, size);

#endif
    if (emptyIPtables == NULL)
    {
        DynamicPreprocessorFatalMessage("%s(%d): Failed to create IP list.\n",
                *(_dpd.config_file), *(_dpd.config_line));
    }

    *data_ptr = (void **)&emptyIPtables;

    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "    Total memory "
            "allocated for empty table: %d bytes\n",
            sfrt_flat_usage(emptyIPtables)););
    return 0;
}

/* ********************************************************************
 * Function: initShareMemory
 *
 * Initialize for shared memory
 * This is called during initialization
 *
 * Arguments:
 *
 * ReputationConfig *config: the configure file
 *
 * RETURNS:
 *    1: success
 *********************************************************************/
void initShareMemory(void *conf)
{
    int segment_number;
    uint32_t snortID;
    ReputationConfig *config = (ReputationConfig *)conf;

    reputation_shmem_config = config;
    if (InitShmemDataMgmtFunctions(InitPerProcessZeroSegment,
            GetSegmentSizeFromFileList,LoadFileIntoShmem))
    {
        DynamicPreprocessorFatalMessage("Unable to initialize DataManagement functions\n");

    }
    /*use snort instance ID to designate server (writer)*/
    snortID = _dpd.getSnortInstance();
    if (SHMEM_SERVER_ID_1 == snortID)
    {
        if ((segment_number = InitShmemWriter(snortID,IPREP,GROUP_0,NUMA_0,
                config->sharedMem.path, &IPtables,config->sharedMem.updateInterval)) == NO_ZEROSEG)
        {
            DynamicPreprocessorFatalMessage("Unable to init share memory writer\n");

        }
    }
    else
    {
        if ((segment_number = InitShmemReader(snortID,IPREP,GROUP_0,NUMA_0,
                config->sharedMem.path, &IPtables,config->sharedMem.updateInterval)) == NO_ZEROSEG)
        {
            DynamicPreprocessorFatalMessage("Unable to init share memory reader\n");

        }
    }

}
#endif
/* ********************************************************************
 * Function: DisplayReputationConfig
 *
 * Display the configuration for the Reputation preprocessor.
 *
 * Arguments:
 *
 * ReputationConfig *config: Reputation preprocessor configuration.
 *
 * RETURNS: Nothing.
 *********************************************************************/
static void DisplayReputationConfig(ReputationConfig *config)
{

    if (config == NULL)
        return;

    _dpd.logMsg("    Memcap: %d %s \n",
            config->memcap,
            config->memcap
            == REPUTATION_DEFAULT_MEMCAP ?
                    "(Default) M bytes" : "M bytes" );
    _dpd.logMsg("    Total number of entries used: %u \n",
            sfrt_flat_num_entries(config->iplist));
    _dpd.logMsg("    Total memory allocated: %u bytes\n",
            sfrt_flat_usage(config->iplist));
    _dpd.logMsg("    Scan local network: %s\n",
            config->scanlocal ?
                    "ENABLED":"DISABLED (Default)");
    _dpd.logMsg("    Reputation priority:  %s \n",
            config->priority
            ==  WHITELISTED?
                    REPUTATION_WHITELIST_KEYWORD "(Default)" : REPUTATION_BLACKLIST_KEYWORD );
    _dpd.logMsg("    Nested IP: %s %s \n",
            NestedIPKeyword[config->nestedIP],
            config->nestedIP
            ==  INNER?
                    "(Default)" : "" );
    if (config->sharedMem.path)
    {
        _dpd.logMsg("    Shared memory supported, Update directory: %s\n",
                config->sharedMem.path );
        _dpd.logMsg("    Shared memory refresh period: %d %s \n",
                config->sharedMem.updateInterval,
                config->sharedMem.updateInterval
                == REPUTATION_DEFAULT_REFRESH_PERIOD ?
                        "(Default) seconds" : "seconds" );
    }
    else
    {
        _dpd.logMsg("    Shared memory is Not supported.\n");

    }
    _dpd.logMsg("\n");
}



/********************************************************************
 * Function: IpListInit
 *
 * Initiate an iplist table
 *
 * Arguments:
 *  Reputation_Config *
 *      The configuration to use.
 *
 * Returns: None
 *
 ********************************************************************/

static void IpListInit(uint32_t maxEntries, ReputationConfig *config)
{
    uint8_t *base;

    if (config->iplist == NULL)
    {
        uint32_t mem_size;
        mem_size = estimateSizeFromEntries(maxEntries, config->memcap);
        config->localSegment = malloc(mem_size);
        segment_meminit((uint8_t*)config->localSegment,mem_size);
        base = (uint8_t *)config->localSegment;

#ifdef SUP_IP6
        /*DIR_16x7_4x4 for performance, but memory usage is high
         *Use  DIR_8x16 worst case IPV4 5K, IPV6 15K (bytes)
         *Use  DIR_16x7_4x4 worst case IPV4 500, IPV6 2.5M
         */
        config->iplist = sfrt_flat_new(DIR_8x16, IPv6, maxEntries, config->memcap);
#else
        config->iplist = sfrt_flat_new(DIR_8x4, IPv4,  maxEntries, config->memcap);

#endif
        config->local_black_ptr = segment_malloc(sizeof(bw_list));
        config->local_white_ptr = segment_malloc(sizeof(bw_list));

        *((bw_list *)&base[config->local_black_ptr]) = black;
        *((bw_list *)&base[config->local_white_ptr]) = white;

        if (config->iplist == NULL)
        {
            DynamicPreprocessorFatalMessage("%s(%d): Failed to create IP list.\n",
                    *(_dpd.config_file), *(_dpd.config_line));
            return;
        }
    }
}

/********************************************************************
 * Function: AddIPtoList
 *
 * Add ip address to config file
 *
 * Arguments:
 *  sfip_t *: ip address
 *  void *: information about the file.
 *  ReputationConfig *:      The configuration to be update.
 *
 * Returns:
 *  IP_INSERT_SUCCESS=0,
 *  IP_INSERT_FAILURE,
 *  IP_INSERT_DUPLICATE
 *
 ********************************************************************/

static int AddIPtoList(sfip_t *ipAddr,INFO info, ReputationConfig *config)
{
    int iRet;
    int iFinalRet = IP_INSERT_SUCCESS;
    /*This variable is used to check whether a more generic address
     * overrides specific address
     */
    uint32_t usageBeforeAdd;
    uint32_t usageAfterAdd;

#ifndef SUP_IP6
    if (ipAddr->family == AF_INET6)
    {
        return RT_INSERT_FAILURE;
    }
#endif
    if (ipAddr->family == AF_INET)
    {
        ipAddr->ip32[0] = ntohl(ipAddr->ip32[0]);
    }
    else if (ipAddr->family == AF_INET6)
    {
        int i;
        for(i = 0; i < 4 ; i++)
            ipAddr->ip32[i] = ntohl(ipAddr->ip32[i]);
    }
#ifdef DEBUG_MSGS

    if (NULL != sfrt_flat_lookup((void *)ipAddr, config->iplist))
    {
        DebugMessage(DEBUG_REPUTATION, "Find address before insert: %s \n",sfip_to_str(ipAddr) );

    }
    else
    {
        DebugMessage(DEBUG_REPUTATION, "Can't find address before insert: %s \n",sfip_to_str(ipAddr) );

    }
#endif

    usageBeforeAdd =  sfrt_flat_usage(config->iplist);

    /*Check whether the same or more generic address is already in the table*/
    if (NULL != sfrt_flat_lookup((void *)ipAddr, config->iplist))
    {
        iFinalRet = IP_INSERT_DUPLICATE;
    }


#ifdef SUP_IP6
    iRet = sfrt_flat_insert((void *)ipAddr, (unsigned char)ipAddr->bits, info, RT_FAVOR_TIME, config->iplist);
#else
    iRet = sfrt_flat_insert((void *)&(ipAddr->ip.u6_addr32[0]), (unsigned char)ipAddr->bits, info, RT_FAVOR_TIME, config->iplist);
#endif
    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Unused memory: %d \n",segment_unusedmem()););


    if (RT_SUCCESS == iRet)
    {
        totalNumEntries++;
#ifdef DEBUG_MSGS

        DebugMessage(DEBUG_REPUTATION, "Number of entries input: %d, in table: %d \n",
                totalNumEntries,sfrt_flat_num_entries(config->iplist) );
        DebugMessage(DEBUG_REPUTATION, "Memory allocated: %d \n",sfrt_flat_usage(config->iplist) );
        if (NULL != sfrt_flat_lookup((void *)ipAddr, config->iplist))
        {
            DebugMessage(DEBUG_REPUTATION, "Find address after insert: %s \n",sfip_to_str(ipAddr) );

        }
#endif
    }
    else if (MEM_ALLOC_FAILURE == iRet)
    {
        iFinalRet = IP_MEM_ALLOC_FAILURE;
        DEBUG_WRAP( DebugMessage(DEBUG_REPUTATION, "Insert error: %d for address: %s \n",iRet, sfip_to_str(ipAddr) ););
    }
    else
    {
        iFinalRet = IP_INSERT_FAILURE;
        DEBUG_WRAP( DebugMessage(DEBUG_REPUTATION, "Insert error: %d for address: %s \n",iRet, sfip_to_str(ipAddr) ););

    }

    usageAfterAdd = sfrt_flat_usage(config->iplist);
    /*Compare in the same scale*/
    if (usageAfterAdd  > (config->memcap << 20))
    {
        iFinalRet = IP_MEM_ALLOC_FAILURE;
    }
    /*Check whether there a more specific address will be overridden*/
    if (usageBeforeAdd > usageAfterAdd )
    {
        iFinalRet = IP_INSERT_DUPLICATE;
    }

    return iFinalRet;

}

/********************************************************************
 * Function:
 *
 * Load one IP list file
 *
 * Arguments:
 *  char *: the line to be processed
 *  void *: information about the file.
 *  ReputationConfig *:      The configuration to be update.
 *
 * Returns:
 *  IP_INSERT_SUCCESS,
 *  IP_INSERT_FAILURE,
 *  IP_INSERT_DUPLICATE
 *
 ********************************************************************/

static int ProcessLine(char *line, INFO info, ReputationConfig *config)
{
    sfip_t ipAddr;
    char *lineBuff;
    char *nextBuff;
    char *arg = NULL;

    if (!line)
        return IP_INSERT_SUCCESS;
    lineBuff = strdup(line);
    if (NULL == lineBuff)
        return IP_MEM_ALLOC_FAILURE;
    if((arg = strtok_r(lineBuff, REPUTATION_SEPARATORS, &nextBuff)) != NULL)
    {
        int iRet;
        if (Reputation_IsEmptyStr(arg))
        {
            free(lineBuff);
            return IP_INSERT_SUCCESS;
        }

        if(sfip_pton(arg, &ipAddr) != SFIP_SUCCESS)
        {
            free(lineBuff);
            return IP_INVALID;

        }
        iRet = AddIPtoList(&ipAddr, info, config);
        if( IP_INSERT_SUCCESS != iRet)
        {
            free(lineBuff);
            return iRet;
        }
        if ((arg = strtok_r(nextBuff, REPUTATION_SEPARATORS, &nextBuff)) != NULL)
        {
            if (!Reputation_IsEmptyStr(arg))
            {
                free(lineBuff);
                return IP_INSERT_FAILURE;
            }
        }

    }
    free(lineBuff);
    return IP_INSERT_SUCCESS;
}
/********************************************************************
 * Function: LoadListFile
 *
 * Load one IP list file
 *
 * Arguments:
 *  filename: file name string
 *  info: information about the file.
 *  ReputationConfig *:  The configuration to be update.
 *
 * Returns:
 *  None
 *
 ********************************************************************/

static void LoadListFile(char *filename, INFO info, ReputationConfig *config)
{

    char list_buf[MAX_ADDR_LINE_LENGTH+1];
    char *lb = list_buf;

    int addrline = 0;
    FILE *fp = NULL;
    char *cmt = NULL;

    if ((NULL == filename)||(0 == info)|| (NULL == config)||config->memCapReached)
        return;

    if((fp = fopen(filename, "r")) == NULL)
    {
        char errBuf[STD_BUF];
#ifdef WIN32
        snprintf(errBuf, STD_BUF, "%s", strerror(errno));
#else
        strerror_r(errno, errBuf, STD_BUF);
#endif
        DynamicPreprocessorFatalMessage("%s(%d) => Unable to open address file %s, Error: %s\n",
                *(_dpd.config_file), *(_dpd.config_line), filename, errBuf);
    }

    while((fgets(lb, MAX_ADDR_LINE_LENGTH, fp)) != NULL)
    {
        int iRet;
        addrline++;

        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Reputation configurations: %s\n",lb ););
        /* remove comments */
        if((cmt = strchr(lb, '#')) != NULL)
        {
            *cmt = '\0';
        }
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Reputation configurations: %s\n",lb ););
        /* process the line */
        iRet = ProcessLine(lb, info, config);
        if (IP_INSERT_FAILURE == iRet)
        {
            DynamicPreprocessorFatalMessage("%s(%d) => Can't insert IP Address \"%s\"\n",
                    filename, addrline, lb);
        }
        else if (IP_INVALID == iRet)
        {
            DynamicPreprocessorFatalMessage("%s(%d) => Invalid IP Address \"%s\"\n",
                    filename, addrline, lb);
        }
        else if (IP_MEM_ALLOC_FAILURE == iRet)
        {
            _dpd.logMsg("    WARNING: %s(%d) => Memcap %d Mbytes reached when inserting IP Address \"%s\"\n",
                    filename, addrline, config->memcap,lb);
            config->memCapReached = true;
            return;
        }
        else if (IP_INSERT_DUPLICATE == iRet)
        {
            if (num_duplicates < MAX_MSGS_TO_PRINT)
            {
            _dpd.logMsg("    %s(%d) => This address has been defined already:\"%s\"\n",
                    filename, addrline, lb );
            }
            num_duplicates++;
        }

        lb = list_buf;
    }

    fclose(fp);

}

/********************************************************************
 * Function: Reputation_FreeConfig
 *
 * Frees a reputation configuration
 *
 * Arguments:
 *  Reputation_Config *
 *      The configuration to free.
 *
 * Returns: None
 *
 ********************************************************************/
void Reputation_FreeConfig (ReputationConfig *config)
{

    if (config == NULL)
        return;


    if (config->emptySegment != NULL)
    {
        free(config->emptySegment);
    }

    if (config->localSegment != NULL)
    {
        free(config->localSegment);
    }

    if(config->sharedMem.path)
        free(config->sharedMem.path);
    free(config);
}


/*********************************************************************
 * Function: EstimateNumEntries
 *
 * First pass to decide iplist table size.
 *
 * Arguments:
 *
 * ReputationConfig *config: Reputation preprocessor configuration.
 * argp:              Pointer to string containing the config arguments.
 *
 * RETURNS:     int. estimated number of Entries based on number of lines
 *********************************************************************/
int EstimateNumEntries(ReputationConfig *config, u_char* argp)
{
    char* cur_sectionp = NULL;
    char* next_sectionp = NULL;
    char* argcpyp = NULL;
    int totalLines = 0;


    /*Default values*/

    argcpyp = strdup( (char*) argp );

    if ( !argcpyp )
    {
        DynamicPreprocessorFatalMessage("Could not allocate memory to parse Reputation options.\n");
        return 0;
    }

    cur_sectionp = strtok_r( argcpyp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Arguments token: %s\n",cur_sectionp ););

    while ( cur_sectionp )
    {
        char* next_tokenp = NULL;
        char* cur_tokenp =  strtok_r( cur_sectionp, REPUTATION_CONFIG_VALUE_SEPERATORS, &next_tokenp);

        if (!cur_tokenp)
        {
            cur_sectionp = strtok_r( next_sectionp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
            continue;
        }

        if ( !strcmp( cur_tokenp, REPUTATION_MEMCAP_KEYWORD ))
        {
            int value;
            char *endStr = NULL;

            cur_tokenp = strtok_r(next_tokenp, REPUTATION_CONFIG_VALUE_SEPERATORS, &next_tokenp);

            if ( !cur_tokenp )
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => No option to '%s'.\n",
                        *(_dpd.config_file), *(_dpd.config_line), REPUTATION_MEMCAP_KEYWORD);
            }

            value = _dpd.SnortStrtol( cur_tokenp, &endStr, 10);

            if (( *endStr) || (errno == ERANGE))
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Bad value specified for %s. "
                        "Please specify an integer between %d and %d.\n",
                        *(_dpd.config_file), *(_dpd.config_line),
                        REPUTATION_MEMCAP_KEYWORD, MIN_MEMCAP, MAX_MEMCAP);
            }

            if (value < MIN_MEMCAP || value > MAX_MEMCAP)
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Value specified for %s is out of "
                        "bounds.  Please specify an integer between %d and %d.\n",
                        *(_dpd.config_file), *(_dpd.config_line),
                        REPUTATION_MEMCAP_KEYWORD, MIN_MEMCAP, MAX_MEMCAP);
            }
            config->memcap = (uint32_t) value;

        }
        else if ( !strcmp( cur_tokenp, REPUTATION_BLACKLIST_KEYWORD )
                ||!strcmp( cur_tokenp, REPUTATION_WHITELIST_KEYWORD ))
        {
            int numlines;
            cur_tokenp = strtok_r( next_tokenp, REPUTATION_CONFIG_VALUE_SEPERATORS, &next_tokenp);
            DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Check list size %s\n",cur_tokenp ););
            if(cur_tokenp == NULL)
            {
                DynamicPreprocessorFatalMessage("%s(%d) => Bad list filename in IP List.\n",
                        *(_dpd.config_file), *(_dpd.config_line));
            }
            errno = 0;
            numlines = numLinesInFile(cur_tokenp);
            if ((0 == numlines) && (0 != errno))
            {
                char errBuf[STD_BUF];

#ifdef WIN32
                snprintf(errBuf, STD_BUF, "%s", strerror(errno));
#else
                strerror_r(errno, errBuf, STD_BUF);
#endif
                DynamicPreprocessorFatalMessage("%s(%d) => Unable to open address file %s, Error: %s\n",
                        *(_dpd.config_file), *(_dpd.config_line), cur_tokenp, errBuf);
            }

            if (totalLines + numlines < totalLines)
            {
                DynamicPreprocessorFatalMessage("%s(%d) => Too many entries in one file.\n",
                        *(_dpd.config_file), *(_dpd.config_line));
            }

            totalLines += numlines;

        }
#ifdef SHARED_REP
        else if ( !strcmp( cur_tokenp, REPUTATION_SHAREMEM_KEYWORD ))
        {

            if (Reputation_IsEmptyStr(next_tokenp))
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Missing argument for %s,"
                        " please specify a path\n",
                        *(_dpd.config_file), *(_dpd.config_line), REPUTATION_SHAREMEM_KEYWORD);
            }

            if (!CheckIPlistDir(next_tokenp))
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Can't find or access the path: %s\n",
                        *(_dpd.config_file), *(_dpd.config_line), next_tokenp);
            }

            config->sharedMem.path = strdup( (char*) next_tokenp );

            if ( !config->sharedMem.path )
            {
                DynamicPreprocessorFatalMessage("Could not allocate memory to parse Reputation options.\n");

            }

            config->sharedMem.updateInterval = REPUTATION_DEFAULT_REFRESH_PERIOD;

        }
#endif

        cur_sectionp = strtok_r( next_sectionp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Arguments token: %s\n",cur_sectionp ););
    }

    free(argcpyp);
    return totalLines;
}

/*********************************************************************
 * Function: ParseReputationArgs
 *
 *  Parses and processes the configuration arguments
 *  supplied in the Reputation preprocessor rule.
 *
 * Arguments:
 *
 * ReputationConfig *config: Reputation preprocessor configuration.
 * argp:              Pointer to string containing the config arguments.
 *
 * RETURNS:     Nothing.
 *********************************************************************/
void ParseReputationArgs(ReputationConfig *config, u_char* argp)
{
    char* cur_sectionp = NULL;
    char* next_sectionp = NULL;
    char* argcpyp = NULL;

    if (config == NULL)
        return;

    _dpd.logMsg("Reputation config: \n");

    /*Default values*/
    config->memcap = REPUTATION_DEFAULT_MEMCAP;
    config->priority = WHITELISTED;
    config->nestedIP = INNER;
    config->localSegment = NULL;
    config->emptySegment = NULL;
    config->memsize = 0;
    config->memCapReached = false;

    /* Sanity check(s) */
    if ( !argp )
    {
        _dpd.logMsg("    WARNING: Can't find any whitelist/blacklist entries. "
                "Reputation Preprocessor disabled\n");
        return;
    }

    argcpyp = strdup( (char*) argp );

    if ( !argcpyp )
    {
        DynamicPreprocessorFatalMessage("Could not allocate memory to parse Reputation options.\n");
        return;
    }

    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Reputation configurations: %s\n",argcpyp ););

    /*We need to parse the memcap, numEntries earlier, then create iplist table*/

    config->numEntries = EstimateNumEntries(config, argp );

    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Estimated number of entries: %d\n",config->numEntries ););

    if ((config->numEntries <= 0) && (!config->sharedMem.path))
    {
        _dpd.logMsg("    WARNING: Can't find any whitelist/blacklist entries. "
                "Reputation Preprocessor disabled\n");
        free(argcpyp);
        return;
    }
    if (!config->sharedMem.path)
        IpListInit(config->numEntries + 1,config);

    cur_sectionp = strtok_r( argcpyp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
    DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Arguments token: %s\n",cur_sectionp ););
    /*Reset the log message count*/
    num_duplicates = 0;
    while ( cur_sectionp )
    {

        char* cur_config;
        char* cur_tokenp = 	strtok( cur_sectionp, REPUTATION_CONFIG_VALUE_SEPERATORS);

        if (!cur_tokenp)
        {
            cur_sectionp = strtok_r( next_sectionp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
            continue;
        }
        cur_config = cur_tokenp;

        if ( !strcmp( cur_tokenp, REPUTATION_SCANLOCAL_KEYWORD ))
        {
            config->scanlocal = 1;
        }
        else if ( !strcmp( cur_tokenp, REPUTATION_MEMCAP_KEYWORD ))
        {
            cur_tokenp = strtok( NULL, REPUTATION_CONFIG_VALUE_SEPERATORS);
            /* processed before */
        }
        else if ( !strcmp( cur_tokenp, REPUTATION_BLACKLIST_KEYWORD ))
        {
            cur_tokenp = strtok( NULL, REPUTATION_CONFIG_VALUE_SEPERATORS);
            DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Loading blacklist from %s\n",cur_tokenp ););
            if(cur_tokenp == NULL)
            {
                DynamicPreprocessorFatalMessage("%s(%d) => Bad list filename in IP List.\n",
                        *(_dpd.config_file), *(_dpd.config_line));
            }
            if (!config->sharedMem.path)
                LoadListFile(cur_tokenp, config->local_black_ptr, config);
            else
            {
                _dpd.logMsg("    WARNING: %s(%d) => List file %s is not loaded "
                        "when using shared memory\n",
                        *(_dpd.config_file), *(_dpd.config_line), cur_tokenp);
            }
        }

        else if ( !strcmp( cur_tokenp, REPUTATION_WHITELIST_KEYWORD ))
        {
            cur_tokenp = strtok( NULL, REPUTATION_CONFIG_VALUE_SEPERATORS);
            DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Loading whitelist from %s\n",cur_tokenp ););
            if(cur_tokenp == NULL)
            {
                DynamicPreprocessorFatalMessage("%s(%d) => Bad list filename in IP List.\n",
                        *(_dpd.config_file), *(_dpd.config_line));
            }

            if (!config->sharedMem.path)
                LoadListFile(cur_tokenp, config->local_white_ptr, config);
            else
            {
                _dpd.logMsg("    WARNING: %s(%d) => List file %s is not loaded "
                        "when using shared memory\n",
                        *(_dpd.config_file), *(_dpd.config_line), cur_tokenp);
            }
        }
        else if ( !strcmp( cur_tokenp, REPUTATION_PRIORITY_KEYWORD ))
        {

            cur_tokenp = strtok( NULL, REPUTATION_CONFIG_VALUE_SEPERATORS);
            if (!cur_tokenp)
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Missing argument for %s\n",
                        *(_dpd.config_file), *(_dpd.config_line), REPUTATION_PRIORITY_KEYWORD);
                return;
            }

            if((strlen(REPUTATION_BLACKLIST_KEYWORD) == strlen (cur_tokenp))
                    && !strcmp(REPUTATION_BLACKLIST_KEYWORD,cur_tokenp))
            {
                config->priority = BLACKLISTED;
            }
            else if((strlen(REPUTATION_WHITELIST_KEYWORD) == strlen (cur_tokenp))
                    && !strcmp(REPUTATION_WHITELIST_KEYWORD,cur_tokenp))
            {
                config->priority = WHITELISTED;
            }
            else
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Invalid argument: %s for %s,"
                        " Use [%s] or [%s]\n",
                        *(_dpd.config_file), *(_dpd.config_line), cur_tokenp,
                        REPUTATION_PRIORITY_KEYWORD,
                        REPUTATION_BLACKLIST_KEYWORD, REPUTATION_WHITELIST_KEYWORD);
                return;
            }

        }
        else if ( !strcmp( cur_tokenp, REPUTATION_NESTEDIP_KEYWORD ))
        {
            int i = 0;
            char NestIPKeyworBuff[STD_BUF];
            NestIPKeyworBuff[0]  = '\0';
            cur_tokenp = strtok( NULL, REPUTATION_CONFIG_VALUE_SEPERATORS);
            if (!cur_tokenp)
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Missing argument for %s\n",
                        *(_dpd.config_file), *(_dpd.config_line), REPUTATION_NESTEDIP_KEYWORD);
                return;
            }
            while(NULL != NestedIPKeyword[i])
            {
                if((strlen(NestedIPKeyword[i]) == strlen (cur_tokenp))
                        && !strcmp(NestedIPKeyword[i],cur_tokenp))
                {
                    config->nestedIP = (NestedIP) i;
                    break;
                }
                _dpd.printfappend(NestIPKeyworBuff, STD_BUF, "[%s] ", NestedIPKeyword[i] );
                i++;
            }
            if (NULL == NestedIPKeyword[i])
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Invalid argument: %s for %s, use %s\n",
                        *(_dpd.config_file), *(_dpd.config_line), cur_tokenp,
                        REPUTATION_NESTEDIP_KEYWORD, NestIPKeyworBuff);
                return;
            }

        }
#ifdef SHARED_REP
        else if ( !strcmp( cur_tokenp, REPUTATION_SHAREMEM_KEYWORD ))
        {
            cur_sectionp = strtok_r( next_sectionp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
            continue;
            /* processed before */
        }
        else if ( !strcmp( cur_tokenp, REPUTATION_SHAREDREFRESH_KEYWORD ))
        {
            unsigned long value;
            char *endStr = NULL;

            if (!config->sharedMem.path)
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Specify option '%s' when using option '%s'.\n",
                        *(_dpd.config_file), *(_dpd.config_line),
                        REPUTATION_SHAREMEM_KEYWORD, REPUTATION_SHAREDREFRESH_KEYWORD);
            }
            cur_tokenp = strtok(NULL, REPUTATION_CONFIG_VALUE_SEPERATORS);

            if ( !cur_tokenp )
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => No option to '%s'.\n",
                        *(_dpd.config_file), *(_dpd.config_line), REPUTATION_SHAREDREFRESH_KEYWORD);
            }

            value = _dpd.SnortStrtoul( cur_tokenp, &endStr, 10);

            if ( *endStr)
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Bad value specified for %s. "
                        "Please specify an integer between %u and %u.\n",
                        *(_dpd.config_file), *(_dpd.config_line),
                        REPUTATION_SHAREDREFRESH_KEYWORD,
                        MIN_SHARED_MEM_REFRESH_PERIOD, MAX_SHARED_MEM_REFRESH_PERIOD);
            }

            if (value < MIN_SHARED_MEM_REFRESH_PERIOD || value > MAX_SHARED_MEM_REFRESH_PERIOD
                    || (errno == ERANGE))
            {
                DynamicPreprocessorFatalMessage(" %s(%d) => Value specified for %s is out of "
                        "bounds.  Please specify an integer between %u and %u.\n",
                        *(_dpd.config_file), *(_dpd.config_line),
                        REPUTATION_SHAREDREFRESH_KEYWORD, MIN_SHARED_MEM_REFRESH_PERIOD,
                        MAX_SHARED_MEM_REFRESH_PERIOD);
            }
            config->sharedMem.updateInterval = (uint32_t) value;

        }
#endif
        else
        {
            DynamicPreprocessorFatalMessage(" %s(%d) => Invalid argument: %s\n",
                    *(_dpd.config_file), *(_dpd.config_line), cur_tokenp);
            return;
        }
        /*Check whether too many parameters*/
        if (NULL != strtok( NULL, REPUTATION_CONFIG_VALUE_SEPERATORS))
        {
            DynamicPreprocessorFatalMessage("%s(%d) => Too many arguments: %s\n",
                    *(_dpd.config_file), *(_dpd.config_line), cur_config);

        }
        cur_sectionp = strtok_r( next_sectionp, REPUTATION_CONFIG_SECTION_SEPERATORS, &next_sectionp);
        DEBUG_WRAP(DebugMessage(DEBUG_REPUTATION, "Arguments token: %s\n",cur_sectionp ););
    }
    if (num_duplicates > MAX_MSGS_TO_PRINT)
        _dpd.logMsg("    Additional %u addresses have been redefined but not printed.\n", num_duplicates - MAX_MSGS_TO_PRINT);
    DisplayReputationConfig(config);
    config->numEntries = sfrt_flat_num_entries(config->iplist);
    reputation_stats.memoryAllocated = sfrt_flat_usage(config->iplist);
    free(argcpyp);
}
