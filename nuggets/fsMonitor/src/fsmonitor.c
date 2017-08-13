#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include "fsmonitor.h"

#include <razorback/uuids.h>
#include <razorback/api.h>
#include <razorback/block_pool.h>
#include <razorback/submission.h>
#include <razorback/config_file.h>

#define FSMONITOR_CONFIG "fsMonitor.conf"
UUID_DEFINE(FS_MONITOR, 0xb1, 0x85, 0x6e, 0x90, 0xaf, 0xb1, 0x11, 0xdf, 0xa7, 0xb9, 0xdb, 0x6c, 0xe9, 0x2e, 0x17, 0xa0);

struct RazorbackContext *sg_pContext = NULL;

static uuid_t sg_uuidNuggetId;

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

static void cleanup(int sig) {
   halt_processing = 1;
}

bool
fsMonitor_handle_file(char *p_sName) 
{
    struct BlockPoolItem *l_pBlock;
    uint32_t l_iSf_Flags, l_iEnt_Flags;

    if ((l_pBlock = BlockPool_CreateItem(sg_pContext)) == NULL)
    {
        rzb_log(LOG_ERR,"Failed to allocate new block");
        exit (-1);
    }

    if (!BlockPool_AddData_FromFile(l_pBlock, p_sName, false))
    {
        rzb_log(LOG_ERR,"Failed to allocate new block");
        exit (-1);
    }
    BlockPool_FinalizeItem(l_pBlock);
    l_pBlock->submittedCallback = NULL;


    if (Submission_Submit(l_pBlock,0, &l_iSf_Flags, &l_iEnt_Flags) != RZB_SUBMISSION_OK)
        return false;
    
    return true;
}

int main(int argc, char *argv[])
{
    uuid_t l_uuidNuggetType;

    // We can accept any number of paths to monitor
    if(argc < 2) 
    {
        printf("Usage: %s <path to monitor> ...\n", argv[0]);
        return -1;
    }

    readMyConfig(NULL, FSMONITOR_CONFIG, sg_Config);
    uuid_copy(l_uuidNuggetType, FS_MONITOR);
    sg_pContext = Razorback_Init_Collection_Context(sg_uuidNuggetId, l_uuidNuggetType);
    if (sg_pContext == NULL)
    {
        rzb_log(LOG_ERR, "Failed to initilize context");
        return -1;
    }
    if (!fsMonitor_init()) 
    {
        rzb_log(LOG_ERR, "Failed to init monitor backend");
        return -1;
    }
    // Hijack sigint so we can close cleanly
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGQUIT, cleanup);

    for(int i = 1; i < argc; i++) 
    {
        if (!fsMonitor_add_path(argv[i])) 
        {
            return -1;
        }
    }

    // Read in the config
    fsMonitor_main();

    rzb_log(LOG_INFO, "Cleaning up and shutting down");
    fsMonitor_cleanup();
    return 0;
}


