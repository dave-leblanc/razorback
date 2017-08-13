/*
 fs walk is an rzb collection nugget that will walk the provided
 file system looking for files with the provided extension and pass
 all matching files to the rzb dispatcher
*/

#include <getopt.h>
#include <ftw.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <libgen.h>

#include <razorback/uuids.h>
#include <razorback/metadata.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/block_pool.h>
#include <razorback/submission.h>
#include <razorback/config_file.h>

#define FSWALK_CONFIG "fsWalk.conf"
UUID_DEFINE(FS_WALK, 0xfb, 0xd7, 0x3e, 0x9a, 0xb5, 0x21, 0x11, 0xdf, 0x98, 0x21, 0x00, 0x1c, 0x23, 0x09, 0xdb, 0xa9);


int processNode(const char *name, const struct stat *status, int type);
void fireToDispatcher(const char *);
void usage(char *);

// Globals - ftw doesn't allow arbirary params to be passed to the callback
const char *sg_sExt;
int sg_iExtLen=0;
int sg_iNumSub = 0;
int slow = 0;

void callback(struct BlockPoolItem *p_pItem);
sem_t sg_sem;

struct RazorbackContext *sg_pContext = NULL;
char *sg_sDataType = NULL;
static uuid_t sg_uuidNuggetId;

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};


int main(int argc, char *argv[])
{
    char *l_sPath = NULL;
    uuid_t l_uuidNuggetType;

    int l_iOptId, l_iOptionIndex = 0;

    memset(&sg_sem, 0, sizeof(sem_t));
    sem_init(&sg_sem, 0 , 0);
    static const struct option long_options[] =
    {
        {"type",required_argument, NULL, 't'},
        {"path",required_argument, NULL, 'p'},
        {"ext",required_argument, NULL, 'e'},
        {"slow",no_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
    };

    while (1)
    {
        l_iOptId = getopt_long(argc, argv, "c:t:p:e:s:", long_options, &l_iOptionIndex);

        if (l_iOptId == -1)
            break;

        switch (l_iOptId)
        {
            case 't':
                sg_sDataType = optarg;
                break;
            case 'p':
                l_sPath = optarg;
                break;
            case 'e':
                sg_sExt = optarg;
                sg_iExtLen = strlen(sg_sExt);
                break;
            case 's':
                slow = 1;
                break;
            case '?':  /* getopt errors */
                break;
            default:
                break;
        }
    }

    if (l_sPath == NULL)
    {
        usage(argv[0]);
        exit(-1);
    }
    uuid_copy(l_uuidNuggetType, FS_WALK);

    readMyConfig(NULL, FSWALK_CONFIG, sg_Config);

    // Register as a collection nugget
    sg_pContext = RZB_Register_Collector(sg_uuidNuggetId, l_uuidNuggetType);
    if (sg_pContext == NULL)
    {
        rzb_log(LOG_ERR, "failed to intialize context");
       exit(-1);
    }
   // I go walkin' after midnight out in the moonlight....
   ftw(l_sPath, processNode, 1);

    while (sg_iNumSub > 0)
    {
        sem_wait(&sg_sem);
        sg_iNumSub--;
    }
    Razorback_Shutdown_Context(sg_pContext);
   exit(EXIT_SUCCESS);
}


// ftw callback that looks for files with the provided extension
int processNode(const char *name, const struct stat *status, int type) {
    int name_len, offset;

    if (type == FTW_F)
    {
        name_len = strlen(name);
        if (name_len < sg_iExtLen)
            return 0;
        offset = name_len - sg_iExtLen;

        if (!strncasecmp(sg_sExt, (name + offset), sg_iExtLen))
        {
            if(slow)
                sleep(3);
            RZB_Log(LOG_INFO, "Found - %s",name);
            fireToDispatcher(name);
        }
    }
    return 0;
}

void fireToDispatcher(const char *p_sName)
{
    struct BlockPoolItem *l_pBlock;
    char * tempFileName = NULL;
    char * fileName = NULL;
    uint32_t l_iSf_Flags, l_iEnt_Flags;

    // Create the datablcok data structure
    if ((l_pBlock = RZB_DataBlock_Create(sg_pContext)) == NULL)
    {
        rzb_log(LOG_ERR,"Failed to allocate new block");
        exit (-1);
    }

    // Add the mmaped data
    if (!BlockPool_AddData_FromFile(l_pBlock, (char *)p_sName, false))
    {
        rzb_log(LOG_ERR,"Failed to allocate new block");
        exit (-1);
    }

    // Set the data block's type
    if (sg_sDataType != NULL)
        RZB_DataBlock_Set_Type(l_pBlock, sg_sDataType);

    // No more data to add, finalize the block
    RZB_DataBlock_Finalize(l_pBlock);

    // Set function pointer to be called when submission process is complete
    l_pBlock->submittedCallback = callback;

    if (asprintf(&tempFileName, "%s", p_sName) == -1)
        return;

    fileName = basename(tempFileName);
    if (fileName != NULL)
        RZB_DataBlock_Metadata_Filename(l_pBlock, fileName);
    free(tempFileName);
    // Submit the data block to the Razorback system
    sg_iNumSub++;
    RZB_DataBlock_Submit(l_pBlock,0, &l_iSf_Flags, &l_iEnt_Flags);
}

void callback(struct BlockPoolItem *p_pItem)
{
    RZB_Log(LOG_INFO,"Callback");
    sem_post(&sg_sem);
}

void usage (char *prog)
{
    fprintf(stderr,"usage: %s --path=<path> --ext=<file extension> --type=<file type>\n",prog);

    exit(EXIT_FAILURE);
}

