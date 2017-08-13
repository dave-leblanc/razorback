#include "config.h"
#include <razorback/debug.h>
#include <razorback/types.h>
#include <razorback/log.h>
#include <razorback/file.h>
#include <razorback/messages.h>
#include <razorback/thread.h>
#include "dispatcher.h"
#include "dbus/messages.h"
#include "routing_list.h"
#include <errno.h>
#include <signal.h>
#include <time.h>

#define DBUS_QUEUE "/topic/DBUS"

static struct Thread *sg_dbusThread = NULL;
static timer_t sg_timer;
static struct Queue *writeQueue = NULL;

static void DBus_Thread (struct Thread *);
static bool DBus_ArmTimer (void);
static void DBus_Timer (union sigval val);

void 
DBus_Init(void)
{
    DBus_RT_Announce_Init(); // reg the announce message
    DBus_RT_Request_Init(); // reg the update message
    DBus_RT_Update_Init(); // reg the request message
    DBus_File_Delete_Init();
	if ((writeQueue = Queue_Create (DBUS_QUEUE, QUEUE_FLAG_SEND, MESSAGE_MODE_JSON)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Initialize", __func__);
        return;
    }
    sg_dbusThread = Thread_Launch (DBus_Thread, NULL, (char *)"DBus Thread", &sg_rbContext);
    DBus_ArmTimer();
}

static bool
DBus_ArmTimer (void)
{
    struct sigevent *l_pProps;
    struct itimerspec l_itsTimerSpec;
    l_itsTimerSpec.it_value.tv_sec = 10;
    l_itsTimerSpec.it_value.tv_nsec = 1;
    l_itsTimerSpec.it_interval.tv_sec = 10;
    l_itsTimerSpec.it_interval.tv_nsec = 1;
    if ((l_pProps = calloc (1, sizeof (struct sigevent))) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: Failed to malloc timer properties", __func__);
        return false;
    }
    l_pProps->sigev_notify = SIGEV_THREAD;
    l_pProps->sigev_value.sival_ptr = NULL;
    l_pProps->sigev_notify_function = &DBus_Timer;
    if (timer_create (CLOCK_REALTIME, l_pProps, &sg_timer) == -1)
    {
        rzb_log (LOG_ERR, "%s: Failed call to timer_create", __func__);
        free(l_pProps);
        return false;
    }
    if (timer_settime (sg_timer, 0, &l_itsTimerSpec, NULL) == -1)
    {
        rzb_log (LOG_ERR, "%s: Failed to arm timer.", __func__);
        free(l_pProps);
        return false;
    }
    free(l_pProps);
    return true;
}

static void 
DBus_Handle_Announce(struct Message *message)
{
    struct MessageRT_Announce *announce = message->message;
    uuid_t source,dest;
    struct Message *request;
    uint64_t serial = 0;
    // Don't process messages when we are DD
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) != 0)
        return;
    RoutingList_Lock();
    serial = RoutingList_GetSerial();
    //rzb_log(LOG_DEBUG, "%s: Routing table announcement: Serial Theirs: %ju Ours: %ju", __func__, announce->serial, serial);
    if (announce->serial <= serial) // We dont care we are already the same
    {
        RoutingList_Unlock();
        return;
    }
    Message_Get_Nuggets(message,source,dest);
    request = RT_Request_Initialize(source);
    Queue_Put(writeQueue, request);
    request->destroy(request);
    RoutingList_Unlock();
}
static void 
DBus_Handle_Request(struct Message *message)
{
    struct Message *announce;
    uuid_t source,dest;
    Message_Get_Nuggets(message, source,dest);
    announce = RT_Update_Initialize_To(RT_UPDATE_REASON_REQUEST, source);
    Queue_Put(writeQueue, announce);
    announce->destroy(announce);
}

static int 
DBus_Handle_Update_AT(void *vAppType, void *vDataType)
{
    struct RoutingAppListEntry *appType = vAppType;
    unsigned char *dataType = vDataType;
    RoutingList_Add(dataType, appType->uuidAppType);
    return LIST_EACH_OK;
}
static int 
DBus_Handle_Update_DT(void *vDataType, void *userData)
{
    struct RoutingListEntry *dataType = vDataType;
    List_ForEach(dataType->appList, DBus_Handle_Update_AT, dataType->uuidDataType);
    return LIST_EACH_OK;
}
static void 
DBus_Handle_Update(struct Message *message)
{
    struct MessageRT_Update *update = message->message;
    uint64_t serial = 0;
    // Don't process messages when we are DD
    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) != 0)
        return;
    rzb_log(LOG_DEBUG, "%s: Routing table update: Serial: %ju, Reason: %u", __func__, update->serial, update->reason);
    RoutingList_Lock();
    serial = RoutingList_GetSerial();
    if (update->serial <= serial) // We dont care we are already the same
    {
        RoutingList_Unlock();
        return;
    }
    // Clear the list ready to fill it with new stuff
    List_Clear(g_routingList);
    List_ForEach(update->list, DBus_Handle_Update_DT, NULL);
    RoutingList_SetSerial(update->serial);
    RoutingList_Unlock();
}

static void
DBus_Handle_Delete(struct Message *message)
{
	struct MessageFile_Delete *deletion = message->message;
	struct Block *block;

	//If Local Master
	if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_LM) != 0)
		return;

	if (deletion->localityId != sg_rbContext.locality)
		return;
	
	if ((block = (struct Block *)calloc(1, sizeof(struct Block))) == NULL) {
		rzb_log(LOG_ERR, "%s: Failed to due to failure of Block calloc", __func__);
		return;
	}

    if ((block->pId = BlockId_Clone(deletion->bid)) == NULL) {
		rzb_log(LOG_ERR, "%s: Failed to due to failure of BlockId_Clone", __func__);
		Block_Destroy(block);
		return;
	}

	if (File_Delete(block) == false) {
		rzb_log(LOG_ERR, "%s: Failed to due to failure of File_Delete", __func__);
		Block_Destroy(block);
		return;
	}

	Block_Destroy(block);
	
	return;
}

static void 
DBus_Thread (struct Thread *p_pThread)
{
    ASSERT (p_pThread != NULL);
    struct Message *message;
    struct Queue *queue = NULL;
    uuid_t source,dest;
    if ((queue = Queue_Create (DBUS_QUEUE, QUEUE_FLAG_SEND |QUEUE_FLAG_RECV, MESSAGE_MODE_JSON)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: failed due to failure of Queue_Initialize", __func__);
        return;
    }

    // run
    while (!Thread_IsStopped(p_pThread))
    {
        // get next message
        if ((message = Queue_Get (queue)) == NULL)
        {
            // check for timeout
            if (errno == EAGAIN)
                continue;
            // error
            rzb_log (LOG_ERR,
                     "%s: Dropped message due to failure of Queue_Get", __func__);
            // drop the message
            continue;
        }
        if (!sg_rbContext.regOk)
        {
            message->destroy(message);
            continue;
        }
        Message_Get_Nuggets(message,source,dest);
        // Ignore messages from me
        if (uuid_compare(source,sg_rbContext.uuidNuggetId) == 0)
        {
            message->destroy(message);
            continue;
        }
        // If its not a broadcast message, make sure its to us.
        if (uuid_is_null(dest) == 0)
        {
            if (uuid_compare(dest,sg_rbContext.uuidNuggetId) != 0)
            {
                message->destroy(message);
                continue;
            }
        }
        switch (message->type)
        {
        case MESSAGE_TYPE_RT_ANNOUNCE:
            DBus_Handle_Announce(message);
            break;
        case MESSAGE_TYPE_RT_REQUEST:
            DBus_Handle_Request(message);
            break;
        case MESSAGE_TYPE_RT_UPDATE:
            DBus_Handle_Update(message);
            break;
		case MESSAGE_TYPE_FILE_DELETE:
			DBus_Handle_Delete(message);
			break;
		default:
            rzb_log(LOG_ERR, "%s: Invalid message type: %u", __func__, message->type);
            break;
        }
        message->destroy(message);
    }
}


static void 
DBus_Timer (union sigval val) 
{
    struct Message * message = NULL;
    if (!sg_rbContext.regOk)
        return;

    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) == 0)
        return;

    message = RT_Announce_Initialize();
    Queue_Put(writeQueue,message);
    message->destroy(message);
}

bool 
DBus_Push_Update(void)
{
    struct Message *message = NULL;
    if (!sg_rbContext.regOk)
        return true;

    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) == 0)
        return true;

    message = RT_Update_Initialize(RT_UPDATE_REASON_CHANGE);
    Queue_Put(writeQueue,message);
    message->destroy(message);
    return true;
}

bool
DBus_File_Delete(struct BlockId *bid, uint8_t locality)
{
	struct Message * message = NULL;
    struct Block * block = NULL;

    //Not sure what should go here
    if (!sg_rbContext.regOk)
        return true;

    if ((sg_rbContext.dispatcher.flags & DISP_HELLO_FLAG_DD) == 0)
        return true;

    if (sg_rbContext.locality == locality) {

	    if ((block = (struct Block *)calloc(1, sizeof(struct Block))) == NULL) {
    	    rzb_log(LOG_ERR, "%s: Failed to due to failure of Block calloc", __func__);
        	return false;
 		}
     
        block->pId = bid;

	    if (File_Delete(block) == false) {
		    rzb_log(LOG_ERR, "%s: Failed to due to failure of File_Delete", __func__);
			free(block);
            return false;
		}
        
        free(block);
        return true;
	}

    message = File_Delete_Initialize(bid, locality);
	if (!Queue_Put(writeQueue,message)) {
		rzb_log(LOG_DEBUG, "%s: Failed because of failure of Queue_Put", __func__);
	}
	message->destroy(message);
	return true;
}
