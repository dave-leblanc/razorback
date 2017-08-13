#include "config.h"

#include <razorback/debug.h>
#include <razorback/log.h>
#include <razorback/list.h>
#include <razorback/string_list.h>
#include <stdlib.h>
#include <string.h>



static int
StringList_ItemSize(const char *item, uint32_t *counter)
{
    *counter += strlen(item)+1;
    return LIST_EACH_OK;
}

SO_PUBLIC uint32_t
StringList_Size (struct List * list)
{
    uint32_t size = 0;

    ASSERT (list != NULL);
    if (list == NULL)
        return 0;

    List_ForEach(list, (int (*)(void *, void *))StringList_ItemSize, &size);

    return size + sizeof (uint32_t);
}

static int 
String_KeyCmp(void *a, void *id)
{
    char *item = a;
    char *key = id;
    if (key == item)
        return 0;

    return strcmp(item,key);
}

static int
String_Cmp(void *a, void *b)
{
    char *iA = a;
    char *iB = b;
    if (a == b)
        return 0;
    return strcmp(iA, iB);
}

static void 
String_Delete(void *a)
{
    char *item = a;
    free(item);
}

void *
String_Clone(void *o)
{
    char *item;
    char *orig = o;
    if ((item = calloc(strlen(orig)+1, sizeof(char))) == NULL)
        return NULL;
    strcpy (item, orig);
    return item; 
}


SO_PUBLIC struct List *
StringList_Create (void)
{
    return List_Create(LIST_MODE_GENERIC, 
            String_Cmp,
            String_KeyCmp,
            String_Delete,
            String_Clone, NULL, NULL);
}

SO_PUBLIC bool 
StringList_Add (struct List *p_pList, const char *string)
{
    char *new;
    if (( new = calloc(strlen(string)+1, sizeof(char))) == NULL)
        return false;
    strcpy(new,string);
    List_Push(p_pList, new);
    return true;
}

