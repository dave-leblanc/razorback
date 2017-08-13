#include "config.h"

#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/messages.h>
#include <razorback/thread.h>
#include <razorback/nugget.h>

#include "dispatcher.h"
#include "database.h"
#include "datastore.h"
#include "global_cache.h"
#include "configuration.h"

#include <errno.h>
#include <sys/time.h>

#define DISP_MASK  (SF_FLAG_GOOD | SF_FLAG_BAD)


void 
FlagCopier_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);
    struct Queue *queue;
    struct Message *message = NULL;
    struct BlockChange *blockIds = NULL;
    struct BlockChange *blockChange = NULL;
    struct Block *parentBlock = NULL, *childBlock = NULL;
    Lookup_Result result;
    uint32_t count;

    uint32_t sfFlags = 0, entFlags = 0;
    uint32_t oldSfFlags = 0, oldEntFlags = 0;
    uint64_t parentCount=0, eventCount=0;
    struct Nugget *nugget= NULL;
    char *dest = NULL;
    char dataType[UUID_STRING_LENGTH];
    char appType[UUID_STRING_LENGTH];
    char nuggetId[UUID_STRING_LENGTH];

    // a database connection
    struct DatabaseConnection *dbCon;
    // Name here is irrelevant.
    if ((queue = Queue_Create (JUDGMENT_QUEUE, QUEUE_FLAG_SEND, MESSAGE_MODE_JSON)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Create Output", __func__);
        return;
    }


    // initialize the database
    if (!Database_Thread_Initialize()) 
    {
        rzb_log(LOG_ERR, "%s: Failed to intialiaze database thread info", __func__);
        return;
    }
    if ((dbCon = Database_Connect
        (g_sDatabaseHost, g_iDatabasePort, g_sDatabaseUser,
         g_sDatabasePassword, g_sDatabaseSchema)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Database_Connect", __func__);
        return;
    }
    if (Datastore_Get_NuggetByUUID (dbCon, g_uuidDispatcherId, &nugget) != R_FOUND)
    {
        rzb_log(LOG_ERR, "%s: Failed to get nugget", __func__);
        return;
    }
    uuid_unparse(nugget->uuidApplicationType, appType);
    uuid_unparse(nugget->uuidNuggetId, nuggetId);

    // run
    while (!Thread_IsStopped (p_pThread))
    {
        count =0;
        blockIds= NULL;
        result = Datastore_Get_BlockIds_To_Set_Bad(dbCon, &blockIds, &count);
        if (result != R_SUCCESS)
        {
            rzb_log(LOG_ERR, "%s, Failed to get event block id's", __func__);
            sleep(5);
            continue;
        }
        blockChange = blockIds;
        for (uint32_t i = 0; i < count; i++)
        {
            GlobalCache_Lock();


            result = Datastore_Get_BlockDisposition(blockChange->parent, dbCon, &sfFlags, &entFlags, false);
            oldSfFlags = sfFlags;
            oldEntFlags = entFlags;

            if (result != R_FOUND)
            {
                rzb_log(LOG_ERR, "%s, Failed to get current block disposition", __func__);
                GlobalCache_Unlock();
                BlockId_Destroy(blockChange->parent);
                BlockId_Destroy(blockChange->child);
                blockChange++;
                continue;
            }
            sfFlags = (sfFlags & ~(DISP_MASK)) | SF_FLAG_BAD;
            
            if (Datastore_Set_BlockDisposition(blockChange->parent, dbCon, sfFlags, entFlags) != R_SUCCESS)
                rzb_log(LOG_ERR, "%s: Failed to set block flags", __func__);

            GlobalCache_Unlock();
            // Output
            if (g_outputAlertChildEnabled)
            {

                if ((Datastore_Count_Block_Parents(dbCon, blockChange->parent, &parentCount) != R_SUCCESS) ||
                        (Datastore_Count_Block_Events(dbCon, blockChange->parent, &eventCount) != R_SUCCESS))
                {
                    BlockId_Destroy(blockChange->parent);
                    BlockId_Destroy(blockChange->child);
                    blockChange++;
                    continue;
                }
                if (Datastore_Get_Block(dbCon, blockChange->parent, &parentBlock) != R_FOUND)
                {
                    BlockId_Destroy(blockChange->parent);
                    BlockId_Destroy(blockChange->child);
                    blockChange++;
                    continue;
                }
                if (Datastore_Get_Block(dbCon, blockChange->child, &childBlock) != R_FOUND)
                {
                    Block_Destroy(parentBlock);
                    BlockId_Destroy(blockChange->parent);
                    BlockId_Destroy(blockChange->child);
                    blockChange++;
                    continue;
                }
                message = MessageAlertChild_Initialize(parentBlock, childBlock, nugget, 
                        parentCount, eventCount,
                        sfFlags, entFlags,
                        oldSfFlags, oldEntFlags);

                if (message == NULL)
                {
                    Block_Destroy(parentBlock);
                    Block_Destroy(childBlock);
                    BlockId_Destroy(blockChange->parent);
                    BlockId_Destroy(blockChange->child);
                    blockChange++;
                    rzb_log(LOG_ERR, "%s: Failed create output", __func__);
                    continue;
                }
                uuid_unparse(parentBlock->pId->uuidDataType, dataType);
                if (asprintf(&dest, "/topic/ChildAlert.%s.%s.%s", dataType, appType, nuggetId) == -1)
                {
                    message->destroy(message);
                    BlockId_Destroy(blockChange->parent);
                    BlockId_Destroy(blockChange->child);
                    blockChange++;
                    rzb_log(LOG_ERR, "%s: Failed create destination", __func__);
                    continue;
                }
                if (!Queue_Put_Dest(queue, message, dest))
                {
                    rzb_log(LOG_ERR, "%s: Failed send message", __func__);
                }
                free(dest);
                ((struct MessageAlertChild *)message->message)->nugget = NULL;
                message->destroy(message); // This takes care of te blocks
            }
            BlockId_Destroy(blockChange->parent);
            BlockId_Destroy(blockChange->child);
            blockChange++;
        }
        if (blockIds == NULL)
            free(blockIds);
        // TODO: Config file
        sleep(2);
    }

    Nugget_Destroy(nugget);
    // terminate database
    Database_Disconnect (dbCon);

    // done
    return;
}
