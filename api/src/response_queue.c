#include "config.h"
#include <razorback/debug.h>
#include <razorback/response_queue.h>
#include <razorback/queue.h>
#include <razorback/queue_list.h>
#include <razorback/block_id.h>
#include <razorback/log.h>
#include <razorback/api.h>
#include <errno.h>
#include "binary_buffer.h"

/** Globals
*/
static struct List *sg_qlResponseQueue;
static bool sg_bResponseInitialized = false;

static void
ResponseQueue_GetQueueName (uuid_t p_pCollectorId, char * p_sQueueName)
{
    Queue_GetQueueName ("/topic/RESPONSE", p_pCollectorId,
                        p_sQueueName);
}

SO_PUBLIC struct Queue *
ResponseQueue_Initialize (uuid_t p_pCollectorId, int p_iFlags)
{
    // the name
    char l_sQueueName[128];
    // the queue from the list
    struct Queue *l_pQueue;

    // setup the global variables
    if (!sg_bResponseInitialized)
    {
        sg_qlResponseQueue = QueueList_Create();
        sg_bResponseInitialized = true;
    }

    // transform to correct name
    ResponseQueue_GetQueueName (p_pCollectorId, l_sQueueName);

    // does this queue already exist?
    // if so, done
    l_pQueue = QueueList_Find (sg_qlResponseQueue, p_pCollectorId);
    if (l_pQueue != NULL)
        return l_pQueue;

    // initialize the queue
    if ((l_pQueue = Queue_Create (l_sQueueName, p_iFlags, Razorback_Get_Message_Mode())) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Initialize", __func__);
        return NULL;
    }

    // find the queue
    if (!QueueList_Add (sg_qlResponseQueue, l_pQueue, p_pCollectorId))
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of QueueList_Add", __func__);
        return NULL;
    }


    // done
    return l_pQueue;
}

SO_PUBLIC void
ResponseQueue_Terminate (uuid_t p_pCollectorId)
{
    // the queue from the list
    struct Queue *l_pQueue;

    // if never initialized, do nothing
    if (!sg_bResponseInitialized)
        return;

    // find the queue and terminate it
    l_pQueue = QueueList_Find (sg_qlResponseQueue, p_pCollectorId);
    if (l_pQueue == NULL)
        return;
    Queue_Terminate (l_pQueue);
    QueueList_Remove(sg_qlResponseQueue, p_pCollectorId);
}


