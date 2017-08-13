#ifndef RAZORBACK_DATASTORE_H
#define RAZORBACK_DATASTORE_H

#include <razorback/uuids.h>
#include "database.h"

struct NuggetType
{
    uuid_t uuidId;
    char *sName;
    char *sDescription;
};

struct AppType
{
    uuid_t uuidId;
    uuid_t uuidNuggetType;
    char *sName;
    char *sDescription;
};

struct BlockChange
{
    struct BlockId *parent;
    struct BlockId *child;
};
   

/** Get nugget structure by uuid.
 * @param p_pDbCon The database connection to use
 * @param p_uuidSourceNugget The nugget UUID
 * @param r_pNugget The pointer to store the returned nugget in.
 * @return a value from Lookup_Result
 */
extern Lookup_Result Datastore_Get_NuggetByUUID (struct DatabaseConnection * p_pDbCon,
                                   uuid_t p_uuidSourceNugget, struct Nugget **r_pNugget);

extern Lookup_Result
Datastore_Insert_Block (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId);

extern Lookup_Result
Datastore_Insert_Block_Link (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pParentBlock, struct BlockId *p_pChildBlock);

extern Lookup_Result
Datastore_Insert_Event_Link (struct DatabaseConnection * p_pDbCon,
                                   struct EventId *p_pParentEvent, struct EventId *p_pChildEvent);

extern Lookup_Result
Datastore_Insert_Event (struct DatabaseConnection * p_pDbCon,
                                   struct Event *p_pEvent);

extern Lookup_Result
Datastore_Insert_Block_Inspection (struct DatabaseConnection * p_pDbCon,
                                   struct EventId *p_pEventId, uuid_t p_uuidAppType);
extern Lookup_Result
Datastore_Update_Block_Inspection_Status (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uuid_t p_uuidAppType, uuid_t uuidNuggetId, uint8_t status);

extern Lookup_Result
Datastore_Get_Block_Inspection_AppTypes (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, unsigned char **types, uint64_t *count);

extern Lookup_Result
Datastore_Get_Block_Inspection_Event (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uuid_t appType, struct Event **event);
extern Lookup_Result
Datastore_Get_Block_Inspection_Events_By_Status (struct DatabaseConnection * p_pDbCon, uint8_t status,
        uuid_t appType, struct Event ***r_event, unsigned char **r_appTypes, uint64_t *r_count);

extern Lookup_Result
Datastore_Count_Outstanding_Block_Inspections (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uint64_t *p_rCount);
extern Lookup_Result
Datastore_Gen_MetabookId (struct DatabaseConnection * p_pDbCon, uint64_t *retId);

extern Lookup_Result
Datastore_Insert_Metadata_List (struct DatabaseConnection * p_pDbCon,
                                   struct List *list, uint64_t metabookId);
extern Lookup_Result
Datastore_Get_Block_MetabookId (struct DatabaseConnection *p_pDbCon,
                                     struct BlockId *p_pBlockId,
                                     uint64_t *metabookId);

extern Lookup_Result
Datastore_Get_BlockDisposition (struct BlockId * blockId,
                              struct DatabaseConnection * dbCon,
                              uint32_t * sfFlags, uint32_t * entFlags,
                              bool create);


extern Lookup_Result
Datastore_Set_BlockDisposition (struct BlockId *blockId,
                              struct DatabaseConnection *dbCon,
                              uint32_t sfFlags, uint32_t entFlags);


extern Lookup_Result
Datastore_Insert_Log_Message_With_Event (struct DatabaseConnection * p_pDbCon,
                                    uuid_t uuidNuggetId,
                                    uint8_t p_iPriority, uint8_t *p_sMessage,
                                    struct EventId *p_pEventId);

extern Lookup_Result
Datastore_Insert_Log_Message (struct DatabaseConnection * p_pDbCon, uuid_t uuidNuggetId,
                                    uint8_t p_iPriority, uint8_t *p_sMessage);

extern Lookup_Result 
Datastore_Get_Event_BlockId (struct DatabaseConnection * p_pDbCon,
                                   struct EventId *p_pEventId, struct BlockId **r_pBlockId);
extern Lookup_Result
Datastore_Insert_Alert (struct DatabaseConnection * p_pDbCon,
                                   struct Judgment *p_pJudgment);

extern Lookup_Result
Datastore_Get_BlockIds_To_Set_Bad (struct DatabaseConnection * p_pDbCon,
                                   struct BlockChange **r_pBlockChange, uint32_t *count);

extern Lookup_Result
Datastore_Insert_App_Type (struct DatabaseConnection * p_pDbCon,
                                    char *name, uuid_t uuidId,
                                    char *description, char *nuggetType);
extern Lookup_Result
Datastore_Get_Nuggets (struct DatabaseConnection * p_pDbCon,
                                   struct Nugget ***r_pNugget, uint64_t *count);

extern Lookup_Result
Datastore_Insert_Nugget (struct DatabaseConnection * p_pDbCon,
                                    char *name, uuid_t uuidId,
                                    uuid_t uuidAppType, uuid_t uuidNuggetType);
extern Lookup_Result
Datastore_Update_Nugget (struct DatabaseConnection * p_pDbCon, struct Nugget *nugget);

extern Lookup_Result
Datastore_Get_Nugget_State (struct DatabaseConnection * p_pDbCon, uuid_t nuggetId, uint32_t *state);
extern Lookup_Result
Datastore_Set_Nugget_State (struct DatabaseConnection * p_pDbCon, uuid_t nuggetId, uint32_t state);

extern Lookup_Result
Datastore_Reset_Nugget_States (struct DatabaseConnection *p_pDbCon);

extern Lookup_Result
Datastore_Get_AppTypes (struct DatabaseConnection * p_pDbCon,
                                   struct AppType **r_pAppType, uint64_t *count);

extern Lookup_Result
Datastore_Get_Metadata (struct DatabaseConnection * p_pDbCon, uint64_t metabookId,
                                   struct List **list);
extern Lookup_Result
Datastore_Get_Block (struct DatabaseConnection * p_pDbCon, struct BlockId *blockId,
                                   struct Block **block);
extern Lookup_Result
Datastore_Get_Event (struct DatabaseConnection * p_pDbCon, struct EventId *eventId,
                                   struct Event **event);
extern Lookup_Result
Datastore_Get_Event_Chain (struct DatabaseConnection * p_pDbCon, struct EventId *eventId,
                                   struct Event **r_event, struct Block **block);

extern Lookup_Result
Datastore_Count_Block_Parents (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uint64_t *p_rCount);
extern Lookup_Result
Datastore_Count_Block_Events (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uint64_t *p_rCount);

// Stuff in datastore_uuid.c
extern Lookup_Result
Datastore_Get_UUID_By_Name (struct DatabaseConnection * p_pDbCon,
                                   const char *name, int type, uuid_t uuid);

extern Lookup_Result
Datastore_Get_UUID_Description_By_UUID (struct DatabaseConnection *p_pDbCon, uuid_t p_uuid, int type, char **ret);
extern Lookup_Result
Datastore_Get_UUID_Name_By_UUID (struct DatabaseConnection *p_pDbCon, uuid_t p_uuid, int type, char **ret);

extern Lookup_Result
Datastore_Get_UUIDs (struct DatabaseConnection * p_pDbCon,
                        uint32_t type, struct List **rList);
extern Lookup_Result
Datastore_Insert_UUID (struct DatabaseConnection * p_pDbCon,
                                    char *name, uuid_t uuidId,
                                    char *description, int type);
extern Lookup_Result
Datastore_Load_UUID_List(struct DatabaseConnection *dbCon,int type);


extern Lookup_Result
Datastore_Get_Block_Localities (struct DatabaseConnection * p_pDbCon,
		                           struct BlockId *p_pbId, uint8_t **locality, uint32_t *numLocalities);

extern Lookup_Result
Datastore_Insert_Block_Locality_Link (struct DatabaseConnection * p_pDbCon,
	                               struct BlockId *p_pbId, uint8_t localityId);

#endif // RAZORBACK_DATASTORE_H

