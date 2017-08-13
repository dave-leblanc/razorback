#include "config.h"
#include <razorback/debug.h>
#include <razorback/queue_list.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <string.h>

static int QueueList_KeyCmp(void *a, void *id);
static int QueueList_Cmp(void *a, void *b);

SO_PUBLIC struct List *
QueueList_Create (void)
{
    return List_Create(LIST_MODE_GENERIC,
            QueueList_Cmp, // Cmp
            QueueList_KeyCmp, // KeyCmp
            NULL, // Delete
            NULL, // Clone
            NULL, // Lock
            NULL); // Unlock
}

SO_PUBLIC struct Queue *
QueueList_Find (struct List *p_pList, const uuid_t p_pId)
{
    struct QueueListEntry *entry;

    ASSERT (p_pList != NULL);
    ASSERT (p_pId != NULL);
    if (p_pList == NULL)
        return NULL;
    if (p_pId == NULL)
        return NULL;
    if ((entry = List_Find(p_pList, (void *)p_pId)) == NULL)
        return NULL;

    return entry->pQueue;
}

SO_PUBLIC bool
QueueList_Add (struct List * p_pList, struct Queue * p_pQ,
               const uuid_t p_pId)
{
    struct QueueListEntry *l_pEntry;

    ASSERT (p_pList != NULL);
    ASSERT (p_pId != NULL);
    if (p_pList == NULL)
        return false;
    if (p_pId == NULL)
        return false;


    if ((l_pEntry = calloc (1,sizeof (struct QueueListEntry))) == NULL)
    {
        rzb_log (LOG_ERR, "%s: failed due to lack of memory", __func__);
        return false;
    }

    uuid_copy (l_pEntry->uuiKey, p_pId);
    l_pEntry->pQueue = p_pQ;

    return List_Push(p_pList, l_pEntry);
}

SO_PUBLIC bool
QueueList_Remove (struct List *p_pList, const uuid_t p_pId)
{
    struct QueueListEntry *entry;

    ASSERT (p_pList != NULL);
    ASSERT (p_pId != NULL);
    if (p_pList == NULL)
        return false;
    if (p_pId == NULL)
        return false;
    if ((entry = List_Find(p_pList, (void *)p_pId)) == NULL)
        return false;

    List_Remove(p_pList, entry);
    return true;
}

static int QueueList_KeyCmp(void *a, void *id) 
{
    unsigned char *uuid=(unsigned char *)id;
    struct QueueListEntry *entry = (struct QueueListEntry *)a;
    return uuid_compare(uuid,entry->uuiKey);
}
static int QueueList_Cmp(void *a, void *b)
{
    if (a == b)
        return 0;

    return -1;
}



