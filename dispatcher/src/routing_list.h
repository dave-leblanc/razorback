/** @file routinglist.h
 * routinglist structures and functions
 */

#ifndef	RAZORBACK_ROUTINGLIST_H
#define	RAZORBACK_ROUTINGLIST_H

#include "razorback.h"
#include "config.h"
#include "statistics.h"
#include "database.h"

/* Structures
 */
struct RoutingAppListEntry
{
    uuid_t uuidAppType;
    struct Queue *pQueue;
};

struct RoutingListEntry
{
    uuid_t uuidDataType;
    struct List *appList;
};

extern struct List *g_routingList;
/* Functions
 */

/** Initializes the routing list
 * @return true if ok, false if error
 */
extern bool RoutingList_Initialize (void);

/** Terminates the routing list
 */
extern void RoutingList_Terminate (void);

/** Removes an item from the routing list
 * @param p_pBlockType the block type
 * @param p_pApplicationType the application type
 */
extern bool RoutingList_RemoveEntry (uuid_t p_pBlockType,
                                uuid_t p_pApplicationType);
/** Removes an item from the routing list
 * @param p_pApplicationType the application type
 */
extern bool RoutingList_RemoveApplication (uuid_t p_pApplicationType);

/** Adds an item to the routing list
 * @param p_pBlockType the block type
 * @param p_pApplicationType the application type
 */
extern bool RoutingList_Add (uuid_t p_pBlockType, uuid_t p_pApplicationType);

/** Routes a message to one or more apptypes, based on data type
 * @param p_pMessage Message to be routed
 * @param p_pDataType UUID of datatype of the data block
 * @return True on success, false on failure
 */
extern bool
RoutingList_RouteMessage (struct DatabaseConnection *dbCon, struct Message *p_pMessage,
                          uuid_t p_pDataType);

void RoutingList_Lock (void);
void RoutingList_Unlock (void);
uint64_t RoutingList_GetSerial(void);
void RoutingList_SetSerial(uint64_t serial);

int AppType_KeyCmp(void *a, void *id);
int AppType_Cmp(void *a, void *b);
void AppType_Destroy(void *a);
void * AppType_Clone(void *a);
int DataType_KeyCmp(void *a, void *id);
int DataType_Cmp(void *a, void *b);
void DataType_Destroy(void *a);
void * DataType_Clone(void *a);

#endif // RAZORBACK_ROUTINGLIST_H
