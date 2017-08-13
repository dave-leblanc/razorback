#include "config.h"

#ifdef _MSC_VER
#pragma comment(lib, "api.lib")
#else
#include <unistd.h>

#include <arpa/inet.h>
#endif
#include <getopt.h>
#include <uuid/uuid.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdint.h>

#include <razorback/uuids.h>
#include <razorback/api.h>
#include <razorback/log.h>
#include <razorback/block_pool.h>
#include <razorback/submission.h>
#include <razorback/config_file.h>
#include <razorback/metadata.h>
#include <razorback/lock.h>

UUID_DEFINE(FILE_INJECT, 0xbe, 0x31, 0x20, 0x18, 0x1b, 0x8e, 0x46, 0x2e, 0x90, 0xa8, 0x98, 0x3a, 0x03, 0x6c, 0x8e, 0x00);

// Definitions
#define FILEINJECT_CONFIG "fileInject.conf"

// Function declarations
static void usage (const char *name);
void callback (struct BlockPoolItem *p_pItem);

// Global variables
struct Semaphore *sg_sem; // Semaphore to know when submission is complete
static uuid_t sg_uuidNuggetId;

RZBConfKey_t sg_Config[] = {
    {"NuggetId", RZB_CONF_KEY_TYPE_UUID, &sg_uuidNuggetId, NULL},
    {NULL, RZB_CONF_KEY_TYPE_END, NULL, NULL}
};

int
main (int argc, char **argv)
{

    // Razorback specific variables
    struct RazorbackContext *l_pContext;    // Defines behavior of nugget (collector)
    struct BlockPoolItem *l_pBlock; // Datablock submission structure
    uint32_t l_iSf_Flags, l_iEnt_Flags; // Return codes from Razorback
    uuid_t l_uuidNuggetType; // The UUID defining the kind of nugget (collector)

    // Command line parsing variables
    int optId;                  /* holds the option index for getopt */
    char *fileType = NULL;
    char *fileName = NULL;
    int option_index = 0;

    // Options structure for command line
    static const struct option long_options[] = {
//        {"conf-dir", required_argument, NULL, 'c'},
        {"type", required_argument, NULL, 't'},
        {"file", required_argument, NULL, 'f'},
        {NULL, 0, NULL, 0}
    };


    // Parse the command line
    while (1)
    {
        optId =
            getopt_long (argc, argv, "c:t:f:", long_options, &option_index);

        if (optId == -1)
            break;

        switch (optId)
        {
        case 't':
            fileType = optarg;
            break;
        case 'f':
            fileName = optarg;
            break;
        case '?':              /* getopt errors */
            break;
        default:
            break;
        }
    }

    if (!fileName)
        usage (argv[0]);
    
    readMyConfig(NULL, FILEINJECT_CONFIG, sg_Config);

    // Setup variables for submission
	sg_sem = Semaphore_Create(false, 0);
    uuid_copy(l_uuidNuggetType, FILE_INJECT);

    // Build a context for a NUGGET_FILE_INJECT nugget with a unique UUID
    l_pContext =
//        Razorback_Init_Collection_Context (l_uuidNuggetId, *l_uuidNuggetType);
        RZB_Register_Collector (sg_uuidNuggetId, l_uuidNuggetType);
    if (l_pContext == NULL)
    {
        fprintf (stderr, "Error initializing context");
        exit(-1);
    }
	

    /*  BEGIN RAZORBACK SUBMISSION SEQUENCE */
    //  Create the structure used to submit a data block within our context
    if ((l_pBlock = RZB_DataBlock_Create (l_pContext)) == NULL)
    {
        rzb_log (LOG_ERR, "Failed to allocate new block");
        exit (-1);
    }

    /* Add the data block to the submission structure.  In this case we
     * attach the data in it's entirety to the structure.  We let the
     * api know that we put this block into memory via mmap, so the api
     * can free the block for us once we submit it.
    */
    if (!BlockPool_AddData_FromFile (l_pBlock, fileName, false))
		exit(1);

    if (fileType != NULL)
    {
        // Set the datatype based on the command line options (i.e. PDF_FILE)
        RZB_DataBlock_Set_Type (l_pBlock, fileType);
    }

    /* Let the api know we are done modifying information about the block.
     * The block automatically generates a SHA256 and places it in the
     * submission structure
    */
    RZB_DataBlock_Finalize(l_pBlock);

    /* We set a function pointer for a function to be called after the
     * entire submission process is complete.  In this nugget, we use
     * a semaphore to prevent the process from exiting before the data
     * block has been submitted by the API.
    */
    l_pBlock->submittedCallback = callback;

    /* Add the metadata about the file:
     *      - The File Name
     */
    RZB_DataBlock_Metadata_Filename(l_pBlock, fileName);

    /* Enter the submission sequence.  If there is a local cache hit,
     * the l_iSf_Flags and l_iEnt_Flags variables will be set with the
     * flags from the local cache. Otherwise, the system threads out and
     * returns execution to the nugget.
    */
    RZB_DataBlock_Submit (l_pBlock, 0, &l_iSf_Flags, &l_iEnt_Flags);

    /* Wait for the sem_post of sg_sem.  This will indicate that the 
     * submission process is complete and we can exit.
    */
    Semaphore_Wait(sg_sem);

    // Shutdown the context to let the dispatcher no we are leaving.
    Razorback_Shutdown_Context(l_pContext);
    return 0;
}

void
callback (struct BlockPoolItem *p_pItem)
{
    Semaphore_Post(sg_sem);
}

static void
usage (const char *name)
{
    printf
        ("Usage: %s [--type=<file_type>] --file=<file_name>\n",
         name);
#if 0
    printf
        ("Usage: %s --type=<file_type> --file=<file_name> [--conf=<configuration file>]\n",
         name);
#endif
    exit (-1);
}
