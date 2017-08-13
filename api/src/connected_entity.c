#include "config.h"

#include <razorback/debug.h>
#include <razorback/connected_entity.h>
#include <razorback/log.h>
#include <razorback/thread.h>
#include <razorback/timer.h>
#include <razorback/uuids.h>

#include <signal.h>
#include <errno.h>

#include "connected_entity_private.h"
#include "runtime_config.h"
#include "transfer/core.h"
#define SEARCH_KEY_NUGGET_ID    (1 << 0)
#define SEARCH_KEY_APP_TYPE     (1 << 1)
#define SEARCH_KEY_NUGGET_TYPE  (1 << 2)
#define SEARCH_KEY_DD           (1 << 3)

static int ConnectedEntity_KeyCmp(void *a, void *id);
static int ConnectedEntity_Cmp(void *a, void *b);
static void ConnectedEntity_Delete(void *a);
static int ConnectedEntityHook_KeyCmp(void *a, void *id);
static int ConnectedEntityHook_Cmp(void *a, void *b);
static void ConnectedEntityHook_Delete(void *a);
struct ConnectedEntityKey
{
    int searchKeys;
    unsigned char *nuggetId;
    unsigned char *appType;
    unsigned char *nuggetType;
};


static struct Timer *timer;             ///< The prune timer

struct ConnectedEntityHook
{
    void (*entityRemoved) (struct ConnectedEntity *entity, uint32_t remainingCount);
};

static struct List *sg_pEntityList = NULL;
static struct List *sg_pHookList = NULL;

static void ConnectedEntityList_Prune (void *userData);

bool
ConnectedEntityList_Start (void)
{
    ASSERT (sg_pEntityList == NULL);


    sg_pEntityList = List_Create(LIST_MODE_GENERIC,
            ConnectedEntity_Cmp, // Cmp
            ConnectedEntity_KeyCmp, // KeyCmp
            ConnectedEntity_Delete, // destroy
            NULL, // clone
            NULL, // Lock
            NULL); //Unlock

    sg_pHookList = List_Create(LIST_MODE_GENERIC,
            ConnectedEntityHook_Cmp, // Cmp
            ConnectedEntityHook_KeyCmp, // KeyCmp
            ConnectedEntityHook_Delete, // destroy
            NULL, // clone
            NULL, // Lock
            NULL); //Unlock

    if ((timer = Timer_Create(Config_getHelloTime () / 2, ConnectedEntityList_Prune, NULL)) == NULL)
        return false;

    return true;

    // Thread
}

void
ConnectedEntityList_Stop (void)
{
    if (sg_pEntityList == NULL)
       return;

    Timer_Destroy(timer);
    List_Destroy(sg_pEntityList);    
}

/** Return a entry or NULL
 */
static struct ConnectedEntity *
ConnectedEntityList_GetEntity (struct Message *message)
{
    struct MessageHello *hello = message->message;
    struct ConnectedEntity *ret = NULL;
    struct ConnectedEntityKey key;
    uuid_t source,dest,dispatcher;
    Message_Get_Nuggets(message, source,dest);
    List_Lock (sg_pEntityList);
    key.searchKeys = SEARCH_KEY_NUGGET_ID;
    key.nuggetId = source;
    key.appType = NULL;
    key.nuggetType = NULL;
    ret = List_Find(sg_pEntityList, &key);
    if (ret == NULL)
    {
        if ((ret =
             calloc (1, sizeof (struct ConnectedEntity))) == NULL)
        {
            List_Unlock(sg_pEntityList);
            return NULL;
        }

        uuid_copy (ret->uuidNuggetId, source);
        uuid_copy (ret->uuidNuggetType, hello->uuidNuggetType);
        uuid_copy (ret->uuidApplicationType, hello->uuidApplicationType);
        ret->locality = hello->locality;
        UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);
        if (uuid_compare(dispatcher, ret->uuidNuggetType) == 0)
        {
            if ((ret->dispatcher = calloc(1,sizeof(struct DispatcherEntity))) == NULL)
            {
                free(ret);
                List_Unlock(sg_pEntityList);
                return NULL;
            }
            ret->dispatcher->priority = hello->priority;
            ret->dispatcher->flags = hello->flags;
            ret->dispatcher->port = hello->port;
            ret->dispatcher->protocol = hello->protocol;
            ret->dispatcher->usable = Transport_IsSupported(hello->protocol);
            if ((ret->dispatcher->addressList = List_Clone(hello->addressList)) == NULL)
            {
                free(ret->dispatcher);
                free(ret);
                List_Unlock(sg_pEntityList);
                return NULL;
            }
        }
        List_Push(sg_pEntityList, ret);
    }
    List_Unlock (sg_pEntityList);
    return ret;
}

SO_PUBLIC bool
ConnectedEntityList_Update (struct Message *message)
{
    uuid_t dispatcher;
	struct ConnectedEntity *entity = NULL;
    struct MessageHello *hello = message->message;

    ASSERT (sg_pEntityList != NULL);
    ASSERT (message != NULL);
    ASSERT (message->type == MESSAGE_TYPE_HELLO);
    List_Lock(sg_pEntityList);

    if ((entity = ConnectedEntityList_GetEntity (message)) == NULL)
    {
        rzb_log (LOG_ERR,
                 "%s: Failed due to failure of _GetEntry()", __func__);
        List_Unlock(sg_pEntityList);
        return false;
    }

    entity->tTimeOfLastHello = time (NULL);
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, dispatcher);
    if (uuid_compare(dispatcher, entity->uuidNuggetType) == 0)
    {
        entity->dispatcher->flags = hello->flags;
        entity->dispatcher->priority = hello->priority;
    }
    List_Unlock(sg_pEntityList);
    return true;
}

SO_PUBLIC uint32_t
ConnectedEntityList_GetCount (void)
{
    return List_Length(sg_pEntityList);
}

struct CountEntity
{
    uint32_t count;
    struct ConnectedEntity *entity;
};

static int 
ConnectedEntityList_CountNuggets(void *item, void *userData)
{
    struct ConnectedEntity *entity = item;
    struct CountEntity *counter = userData;
    if ((uuid_compare(counter->entity->uuidNuggetType, entity->uuidNuggetType) == 0) &&
            (uuid_compare(counter->entity->uuidApplicationType, entity->uuidApplicationType) == 0))
    {
        counter->count++;
    }
    return LIST_EACH_OK;
}

static int 
ConnectedEntityList_HookWrapper(void *item, void *userData)
{
    struct ConnectedEntityHook *hook = item;
    struct CountEntity *counter = userData;
    hook->entityRemoved(counter->entity, counter->count-1);
    return LIST_EACH_OK;
}

static int 
ConnectedEntityList_PruneEntity(void *item, void *userData)
{
    struct ConnectedEntity *entity = item;
    struct CountEntity counter;
    time_t l_tTimeNow = time (NULL);
    time_t l_iDeadTime = Config_getDeadTime ();
    if ((entity->tTimeOfLastHello > 0 ) && 
            ( l_tTimeNow - entity->tTimeOfLastHello > l_iDeadTime))
    {
        counter.count = 0;
        counter.entity=entity;
        List_ForEach(sg_pEntityList, ConnectedEntityList_CountNuggets, &counter);
        List_ForEach(sg_pHookList, ConnectedEntityList_HookWrapper, &counter);
        return LIST_EACH_REMOVE;
    }
    return LIST_EACH_OK;
}

static void
ConnectedEntityList_Prune (void *userData)
{
    ASSERT (sg_pEntityList != NULL);

    List_Lock(sg_pEntityList);
    List_Lock(sg_pHookList);
    List_ForEach(sg_pEntityList, ConnectedEntityList_PruneEntity, NULL);
    List_Unlock(sg_pEntityList);
    List_Unlock(sg_pHookList);
}

SO_PUBLIC bool
ConnectedEntityList_AddPruneListener(void (*entityRemoved) (struct ConnectedEntity *entity, uint32_t remainingCount))
{
    struct ConnectedEntityHook *l_pHook;
    if (sg_pHookList == NULL)
        return false;

    if ((l_pHook = calloc(1, sizeof(struct ConnectedEntityHook))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: fail to allocate new node", __func__);
        return false;
    }
    l_pHook->entityRemoved = entityRemoved;
    List_Push(sg_pHookList, l_pHook);
    return true;
}

SO_PUBLIC bool
ConnectedEntityList_ForEach_Entity (int (*function) (struct ConnectedEntity *, void *), void *userData)
{
    List_ForEach(sg_pEntityList, (int (*)(void *, void *))function, userData);
    return true;
}
static struct ConnectedEntity *
ConnectedEntity_Clone(struct ConnectedEntity *orig)
{
    struct ConnectedEntity *ret;
    if ((ret = calloc(1,sizeof(struct ConnectedEntity))) == NULL)
        return NULL;
    memcpy(ret, orig, sizeof(struct ConnectedEntity));
    if ((ret->dispatcher = calloc(1,sizeof(struct DispatcherEntity))) == NULL)
    {
        free(ret);
        return NULL;
    }
    memcpy(ret->dispatcher, orig->dispatcher, sizeof(struct DispatcherEntity));
    if ((ret->dispatcher->addressList = List_Clone(orig->dispatcher->addressList)) == NULL)
    {
        free(ret->dispatcher);
        free(ret);
        return NULL;
    }
    return ret;
}

SO_PUBLIC struct ConnectedEntity *
ConnectedEntityList_GetDedicatedDispatcher(void)
{
    struct ConnectedEntity *ret = NULL;
    struct ConnectedEntity *node;
    struct ConnectedEntityKey key;

    memset(&key, 0, sizeof(struct ConnectedEntityKey));
    key.searchKeys= SEARCH_KEY_DD;

    List_Lock(sg_pEntityList);
    node = List_Find(sg_pEntityList, &key);
    if (node == NULL)
    {
        List_Unlock(sg_pEntityList);
        return NULL;
    }
    ret = ConnectedEntity_Clone(node);
    List_Unlock(sg_pEntityList);

    return ret;
}
static int 
ConnectedEntityList_CollectDispatchers(void *item, void *userData)
{
    struct ConnectedEntity *entity = item;
    struct List *list = userData;
    uuid_t uuid;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, uuid);
    if ((uuid_compare(uuid, entity->uuidNuggetType) == 0))
        List_Push(list, entity);

    return LIST_EACH_OK;
}

struct ConnectedEntity *
ConnectedEntityList_GetDispatcher(void)
{
    struct List *dispatchers = NULL;
    uint32_t dispatcherCount;
    uint8_t locality = Config_getLocalityId();
    conf_int_t *localities = Config_getLocalityBackupOrder();
    conf_int_t localityCount = Config_getLocalityBackupCount();
    struct ListNode *node;
    struct ConnectedEntity *entity;
    struct ConnectedEntity *ret;
	conf_int_t i;

    dispatchers = List_Create(LIST_MODE_GENERIC,
            ConnectedEntity_Cmp, // Cmp
            ConnectedEntity_KeyCmp, // KeyCmp
            NULL, // destroy
            NULL, // clone
            NULL, // Lock
            NULL); //Unlock
    if (dispatchers == NULL)
        return NULL;

    List_Lock(sg_pEntityList);
    List_ForEach(sg_pEntityList, ConnectedEntityList_CollectDispatchers, dispatchers);
    // There should be a better way to do this without reaching into the list.
    dispatcherCount = List_Length(dispatchers);
    if (dispatcherCount == 0)
    {
        List_Destroy(dispatchers);
        List_Unlock(sg_pEntityList);
        rzb_log(LOG_ERR, "%s: No dispatchers", __func__);
        return NULL;
    }
    // Our locality
    for (node = dispatchers->head; node != NULL; node = node->next)
    {
        entity = node->item;
        if ( (entity->locality == locality) &&
                (entity->dispatcher->usable))
        {
            goto getdispdone;
        }
    }

    // Backup localities in order
    for (i = 0; i < localityCount; i++)
    {
        for (node = dispatchers->head; node != NULL; node = node->next)
        {
            entity = node->item;
            if ( (entity->locality == localities[i]) &&
                    (entity->dispatcher->usable))
            {
                goto getdispdone;
            }
        }
    }
    // Random locality
    for (node = dispatchers->head; node != NULL; node = node->next)
    {
        entity = node->item;
        if (entity->dispatcher->usable)
        {
            goto getdispdone;
        }
    }
            
    if (node == NULL)
    {
        List_Destroy(dispatchers);
        List_Unlock(sg_pEntityList);
        rzb_log(LOG_ERR, "%s: Failed to find dispatcher", __func__);
        return NULL;
    }

getdispdone:
    ret = ConnectedEntity_Clone(entity);
    List_Destroy(dispatchers);
    List_Unlock(sg_pEntityList);
    if (ret == NULL)
        rzb_log(LOG_ERR, "%s: Failed to clone dispatcher", __func__);
    return ret;
}

static int 
ConnectedEntityList_CollectHighDispatcher(void *item, void *userData)
{
    struct ConnectedEntity *entity = item;
    struct DispatcherEntity **cur = userData;

    uuid_t uuid;
    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, uuid);
    if ((uuid_compare(uuid, entity->uuidNuggetType) == 0))
    {
        if (*cur == NULL)
            *cur = entity->dispatcher;
        else if ((*cur)->priority < entity->dispatcher->priority)
            *cur = entity->dispatcher;
    }

    return LIST_EACH_OK;
}


SO_PUBLIC struct ConnectedEntity *
ConnectedEntityList_GetDispatcher_HighestPriority()
{
    struct ConnectedEntity *ret = NULL;
    
    List_Lock(sg_pEntityList);
    List_ForEach(sg_pEntityList, ConnectedEntityList_CollectHighDispatcher, &ret);
    if (ret != NULL)
        ret = ConnectedEntity_Clone(ret);

    List_Unlock(sg_pEntityList);
    return ret;
}

struct CE_SlaveSearch {
	uint8_t locality;
	bool found;
};
static int
ConnectedEntityList_CollectSlaveInLocality(void *item, void *userData)
{
    struct ConnectedEntity *entity = item;
    struct CE_SlaveSearch *search = userData;
    uuid_t uuid;

    if (search->found)
    	return LIST_EACH_OK;

    UUID_Get_UUID(NUGGET_TYPE_DISPATCHER, UUID_TYPE_NUGGET_TYPE, uuid);
    if ((uuid_compare(uuid, entity->uuidNuggetType) == 0))
    {
    	if(entity->locality != search->locality)
    		return LIST_EACH_OK;

    	if ((entity->dispatcher->flags & DISP_HELLO_FLAG_LS) != 0)
    		search->found = true;
    }

    return LIST_EACH_OK;
}

SO_PUBLIC bool
ConnectedEntityList_SlaveInLocality(uint8_t locality)
{
	struct CE_SlaveSearch search;
	search.locality = locality;
	search.found = false;

    List_Lock(sg_pEntityList);
    List_ForEach(sg_pEntityList, ConnectedEntityList_CollectSlaveInLocality, &search);
    List_Unlock(sg_pEntityList);
	return search.found;
}

bool
ConnectedEntityList_MarkDispatcherUnusable(uuid_t nuggetId)
{
    struct ConnectedEntity *dispatcher;
    struct ConnectedEntityKey key;
    List_Lock (sg_pEntityList);
    key.searchKeys = SEARCH_KEY_NUGGET_ID;
    key.nuggetId = nuggetId;
    key.appType = NULL;
    key.nuggetType = NULL;
    dispatcher = List_Find(sg_pEntityList, &key);
    if (dispatcher == NULL)
    {
        List_Unlock(sg_pEntityList);
        return false;
    }
    dispatcher->dispatcher->usable = false;
    List_Unlock(sg_pEntityList);
    return true;
}

static int 
ConnectedEntity_KeyCmp(void *a, void *id)
{
    struct ConnectedEntity *entity = a;
    struct ConnectedEntityKey *key = id;
    int ret = -1;
    if ((key->searchKeys & SEARCH_KEY_NUGGET_ID) != 0)
    {
        ASSERT(key->nuggetId != NULL); 
        if (key->nuggetId == NULL) 
            return -1;

        if (uuid_compare(entity->uuidNuggetId, key->nuggetId) == 0)
            ret = 0;
        else if (ret == 0)
            ret = -1;
    }
    if ((key->searchKeys & SEARCH_KEY_APP_TYPE) != 0)
    {
        ASSERT (key->appType != NULL);
        if (key->appType == NULL)
            return -1;
        if ( uuid_compare(entity->uuidApplicationType, key->appType) == 0)
            ret = 0;
        else if (ret == 0)
            ret = -1;
    }
    if ((key->searchKeys & SEARCH_KEY_NUGGET_TYPE) != 0)
    {
        ASSERT(key->nuggetType != NULL);
        if (key->nuggetType == NULL)
            return -1;
        if (uuid_compare(entity->uuidNuggetType, key->nuggetType) == 0)
            ret = 0;
        else if (ret == 0)
            ret = -1;
    }
    if ((key->searchKeys & SEARCH_KEY_DD) != 0)
    {
        if (entity->dispatcher != NULL)
        {
            if ((entity->dispatcher->flags & DISP_HELLO_FLAG_DD) != 0)
                ret = 0;
            else if (ret == 0)
                ret = -1;
        }
    }
    return ret;
}
static int 
ConnectedEntity_Cmp(void *a, void *b)
{
    if (a == b)
        return 0;

    return -1;
}

static void 
ConnectedEntity_Delete(void *a)
{
    ConnectedEntity_Destroy(a);
}

static int 
ConnectedEntityHook_KeyCmp(void *a, void *id)
{
    return -1;    
}
static int 
ConnectedEntityHook_Cmp(void *a, void *b)
{
    if (a == b)
        return 0;

    return -1;
}

static void 
ConnectedEntityHook_Delete(void *a)
{
    free(a);
}

SO_PUBLIC void
ConnectedEntity_Destroy(struct ConnectedEntity *entity)
{
    ASSERT(entity != NULL);
    if (entity == NULL)
        return;
    if (entity->dispatcher != NULL)
    {
        if (entity->dispatcher->addressList != NULL)
            List_Destroy(entity->dispatcher->addressList);

        free(entity->dispatcher);
    }
    free(entity);
}
