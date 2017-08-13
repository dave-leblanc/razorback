#include "config.h"
#include <razorback/debug.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block_id.h>
#include <razorback/hash.h>
#include <razorback/event.h>
#include <razorback/block.h>
#include <razorback/nugget.h>
#include "datastore.h"
#include "global_cache.h"
#include <errno.h>
#include <string.h>

#define SELECT_NUGGET_BY_UUID "SELECT App_Type_UUID,Nugget_Type_UUID,Name,Location,Contact,Notes FROM Nugget WHERE UUID=?"
#define SELECT_NUGGET_BY_UUID_PARAM_COUNT 1
#define SELECT_NUGGET_BY_UUID_PARAMS { DB_TYPE_BLOB }
#define SELECT_NUGGET_BY_UUID_FIELD_COUNT 6
#define SELECT_NUGGET_BY_UUID_FIELDS { DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING }

Lookup_Result
Datastore_Get_NuggetByUUID (struct DatabaseConnection * p_pDbCon,
                                   uuid_t p_uuidSourceNugget, struct Nugget **r_pNugget)
{
    ASSERT ( p_pDbCon != NULL );
    struct Nugget *l_pRet = NULL;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = SELECT_NUGGET_BY_UUID_FIELDS;

    DB_TYPE_t l_eParamTypes[] = SELECT_NUGGET_BY_UUID_PARAMS;
    char *l_pParams[SELECT_NUGGET_BY_UUID_PARAM_COUNT];
    unsigned long l_pParamLengths[SELECT_NUGGET_BY_UUID_PARAM_COUNT];

    struct Field *l_pRow;
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(p_uuidSourceNugget, uuid);
    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;

    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                SELECT_NUGGET_BY_UUID,
                SELECT_NUGGET_BY_UUID_PARAM_COUNT, 
                l_eParamTypes, 
                SELECT_NUGGET_BY_UUID_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    
    if ((l_pRet = calloc(1, sizeof(struct Nugget))) == NULL)
    {
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    uuid_copy(l_pRet->uuidNuggetId, p_uuidSourceNugget);
    if (l_pRow[0].pValue == NULL)
        uuid_clear(l_pRet->uuidApplicationType);
    else
        uuid_parse(l_pRow[0].pValue, l_pRet->uuidApplicationType);

    uuid_parse(l_pRow[1].pValue, l_pRet->uuidNuggetType);

    l_pRet->sName = l_pRow[2].pValue;
    l_pRet->sLocation = l_pRow[3].pValue;
    l_pRet->sContact = l_pRow[4].pValue;
    l_pRet->sNotes = l_pRow[5].pValue;
    l_pRow[2].pValue = NULL;
    l_pRow[3].pValue = NULL;
    l_pRow[4].pValue = NULL;
    l_pRow[5].pValue = NULL;

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_pNugget = l_pRet;
    return R_FOUND;
}

#define GET_NUGGET_STATE "SELECT State FROM Nugget WHERE UUID=?"
#define GET_NUGGET_STATE_PARAM_COUNT 1
#define GET_NUGGET_STATE_PARAMS { DB_TYPE_STRING }
#define GET_NUGGET_STATE_FIELD_COUNT 1
#define GET_NUGGET_STATE_FIELDS { DB_TYPE_LONG }

Lookup_Result
Datastore_Get_Nugget_State (struct DatabaseConnection *p_pDbCon,
                            uuid_t nugget, uint32_t  *state)
{
    ASSERT(p_pDbCon != NULL);

    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = GET_NUGGET_STATE_PARAMS;
    char *l_pParams[GET_NUGGET_STATE_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_NUGGET_STATE_PARAM_COUNT];

    DB_TYPE_t l_eResTypes[] = GET_NUGGET_STATE_FIELDS;
    
    struct Field *l_pRow;
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(nugget, uuid);
    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                GET_NUGGET_STATE,
                GET_NUGGET_STATE_PARAM_COUNT, 
                l_eParamTypes, 
                GET_NUGGET_STATE_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        return R_ERROR;
    }

    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: No rows", __func__);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to fetch row", __func__);
        return R_ERROR;
    }
    if (l_pRow[0].pValue !=NULL)
        memcpy(state, l_pRow[0].pValue, sizeof(uint32_t));
    else 
        *state = 0;

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_FOUND;
}

#define SET_NUGGET_STATE "UPDATE Nugget SET State=? WHERE UUID=?"
#define SET_NUGGET_STATE_PARAM_COUNT 2
#define SET_NUGGET_STATE_PARAMS { DB_TYPE_LONG, DB_TYPE_STRING }
#define SET_NUGGET_STATE_FIELD_COUNT 0

Lookup_Result
Datastore_Set_Nugget_State (struct DatabaseConnection *p_pDbCon,
                            uuid_t nugget, uint32_t  state)
{
    ASSERT(p_pDbCon != NULL);

    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = SET_NUGGET_STATE_PARAMS;
    char *l_pParams[SET_NUGGET_STATE_PARAM_COUNT];
    unsigned long l_pParamLengths[SET_NUGGET_STATE_PARAM_COUNT];

    
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(nugget, uuid);
    l_pParams[0] = (char *)&state;
    l_pParamLengths[0] = 4;
    l_pParams[1] = uuid;
    l_pParamLengths[1] = 36;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                SET_NUGGET_STATE,
                SET_NUGGET_STATE_PARAM_COUNT, 
                l_eParamTypes, 
                SET_NUGGET_STATE_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}
#define RESET_NUGGET_STATE "UPDATE Nugget SET State=0"
#define RESET_NUGGET_STATE_PARAM_COUNT 0
#define RESET_NUGGET_STATE_FIELD_COUNT 0

Lookup_Result
Datastore_Reset_Nugget_States (struct DatabaseConnection *p_pDbCon)
{
    ASSERT(p_pDbCon != NULL);

    struct Statement *l_pStmt = NULL;


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                RESET_NUGGET_STATE,
                RESET_NUGGET_STATE_PARAM_COUNT, 
                NULL,
                RESET_NUGGET_STATE_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, NULL, NULL))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}


#define SELECT_NUGGETS "SELECT UUID, App_Type_UUID, Nugget_Type_UUID, Name, Location, Contact, Notes FROM Nugget"
#define SELECT_NUGGETS_PARAM_COUNT 0
#define SELECT_NUGGETS_FIELD_COUNT 7
#define SELECT_NUGGETS_FIELDS { DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING }

Lookup_Result
Datastore_Get_Nuggets (struct DatabaseConnection * p_pDbCon,
                                   struct Nugget ***r_pNugget, uint64_t *count)
{
    ASSERT ( p_pDbCon != NULL );
    struct Nugget **l_pRet = NULL;
    struct Statement *l_pStmt = NULL;
    uint64_t rowCount = 0;
    DB_TYPE_t l_eResTypes[] = SELECT_NUGGETS_FIELDS;

    struct Field *l_pRow;

    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                SELECT_NUGGETS,
                SELECT_NUGGETS_PARAM_COUNT, 
                NULL, 
                SELECT_NUGGETS_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, NULL, NULL))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((rowCount = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    
    if ((l_pRet = calloc(rowCount, sizeof(struct Nugget*))) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    for (uint64_t i = 0; i< rowCount; i++)
    {
        if ((l_pRet[i] = calloc(1, sizeof(struct Nugget))) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
        if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            for (uint64_t j = 0; j < i; j++)
                Nugget_Destroy(l_pRet[j]);
            free(l_pRet);
            return R_ERROR;
        }
        uuid_parse(l_pRow[0].pValue, l_pRet[i]->uuidNuggetId);
        if (l_pRow[1].pValue == NULL)
            uuid_clear(l_pRet[i]->uuidApplicationType);
        else
            uuid_parse(l_pRow[1].pValue, l_pRet[i]->uuidApplicationType);

        uuid_parse(l_pRow[2].pValue, l_pRet[i]->uuidNuggetType);

        l_pRet[i]->sName = l_pRow[3].pValue;
        l_pRet[i]->sLocation = l_pRow[4].pValue;
        l_pRet[i]->sContact = l_pRow[5].pValue;
        l_pRet[i]->sNotes = l_pRow[6].pValue;
        l_pRow[3].pValue = NULL;
        l_pRow[4].pValue = NULL;
        l_pRow[5].pValue = NULL;
        l_pRow[6].pValue = NULL;

        Database_Stmt_FreeRow(l_pStmt, l_pRow);
    }
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_pNugget = l_pRet;
    *count = rowCount;
    return R_SUCCESS;
}

#define INSERT_BLOCK "INSERT INTO Block (`Hash`, `Size`, `Data_Type_UUID`, `Metabook_ID` ) VALUES (?,?,?,?)"
#define INSERT_BLOCK_PARAM_COUNT 4
#define INSERT_BLOCK_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define INSERT_BLOCK_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Block (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    struct Statement *l_pStmt = NULL;
    uint64_t metabookId = 0;
    
    DB_TYPE_t l_eParamTypes[] = INSERT_BLOCK_PARAMS;
    char *l_pParams[INSERT_BLOCK_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_BLOCK_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(p_pBlockId->uuidDataType, uuid);

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    l_pParams[2] = uuid;
    l_pParamLengths[2] = 36;
    
    l_pParams[3] = (char *)&metabookId;

    pthread_mutex_lock(&p_pDbCon->mutex);

    Datastore_Gen_MetabookId(p_pDbCon, &metabookId);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_BLOCK,
                INSERT_BLOCK_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_BLOCK_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define GEN_METABOOK_ID "INSERT INTO Metabook (Metabook_ID) VALUES (NULL)"
#define GEN_METABOOK_ID_PARAM_COUNT 0
#define GEN_METABOOK_ID_FIELD_COUNT 0

Lookup_Result
Datastore_Gen_MetabookId (struct DatabaseConnection * p_pDbCon, uint64_t *retId)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(retId != NULL);
    struct Statement *l_pStmt = NULL;


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                GEN_METABOOK_ID,
                GEN_METABOOK_ID_PARAM_COUNT, 
                NULL,
                GEN_METABOOK_ID_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, NULL, NULL))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    *retId = Database_LastInsertId(p_pDbCon);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define INSERT_BLOCK_LOCALITY_LINK "INSERT IGNORE INTO LK_Block_Locality (Block_ID, Locality_ID) SELECT Block.Block_ID,? FROM Block WHERE Block.Hash=? AND Block.Size=?"
#define INSERT_BLOCK_LOCALITY_LINK_PARAM_COUNT 3
#define INSERT_BLOCK_LOCALITY_LINK_PARAMS { DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define INSERT_BLOCK_LOCALITY_LINK_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Block_Locality_Link (struct DatabaseConnection * p_pDbCon,
		                           struct BlockId *p_pbId, uint8_t localityId)
{
	ASSERT(p_pDbCon != NULL);
	ASSERT(p_pbId != NULL);
	struct Statement *l_pStmt = NULL;
    uint64_t locality = localityId;

	DB_TYPE_t l_eParamTypes[] = INSERT_BLOCK_LOCALITY_LINK_PARAMS;
	char *l_pParams[INSERT_BLOCK_LOCALITY_LINK_PARAM_COUNT];
	unsigned long l_pParamLengths[INSERT_BLOCK_LOCALITY_LINK_PARAM_COUNT];

	l_pParams[0] = (char *)&locality;
    
	l_pParams[1] = (char *)p_pbId->pHash->pData;
	l_pParamLengths[1] = p_pbId->pHash->iSize;

	l_pParams[2] = (char *)&p_pbId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon,
                INSERT_BLOCK_LOCALITY_LINK,
                INSERT_BLOCK_LOCALITY_LINK_PARAM_COUNT,
                l_eParamTypes,
                INSERT_BLOCK_LOCALITY_LINK_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define GET_BLOCK_LOCALITIES "SELECT Locality_ID FROM LK_Block_Locality LEFT JOIN Block USING (Block_ID) WHERE Block.Hash=? AND Block.Size=?"
#define GET_BLOCK_LOCALITIES_PARAM_COUNT 2
#define GET_BLOCK_LOCALITIES_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG}
#define GET_BLOCK_LOCALITIES_FIELD_COUNT 1
#define GET_BLOCK_LOCALITIES_FIELDS { DB_TYPE_LONGLONG }

Lookup_Result
Datastore_Get_Block_Localities (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pbId, uint8_t **locality, uint32_t *numLocalities)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pbId != NULL);
	ASSERT(numLocalities != NULL);

    struct Statement *l_pStmt = NULL;
    unsigned long long rowCount = 0;
    uint32_t i = 0;

    DB_TYPE_t l_eParamTypes[] = GET_BLOCK_LOCALITIES_PARAMS;
    char *l_pParams[GET_BLOCK_LOCALITIES_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_BLOCK_LOCALITIES_PARAM_COUNT];

    DB_TYPE_t l_eResTypes[] = GET_BLOCK_LOCALITIES_FIELDS;

    struct Field *l_pRow;

    l_pParams[0] = (char *)p_pbId->pHash->pData;

    l_pParamLengths[0] = p_pbId->pHash->iSize;

    l_pParams[1] = (char *)&p_pbId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon,
                GET_BLOCK_LOCALITIES,
                GET_BLOCK_LOCALITIES_PARAM_COUNT,
                l_eParamTypes,
                GET_BLOCK_LOCALITIES_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((rowCount = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((*locality = (uint8_t *)calloc(1, rowCount)) == NULL) {
		Database_Stmt_Destroy(l_pStmt);
		pthread_mutex_unlock(&p_pDbCon->mutex);
		return R_ERROR;
	}

    for (i = 0; i < rowCount; i++) {
        if (l_pRow[i].pValue != NULL)
            memcpy((*locality) + i, l_pRow[i].pValue, sizeof(uint8_t));
        else
            *((*locality) + i) = 0;
	}

	*numLocalities = rowCount;

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define INSERT_BLOCK_LINK "INSERT IGNORE INTO LK_Block_Block (Parent_ID, Child_ID) SELECT Block1.Block_ID,Block2.Block_ID from Block as Block1,Block as Block2 where Block1.Hash=? AND Block1.Size=? AND Block2.Hash=? AND Block2.Size=?"
#define INSERT_BLOCK_LINK_PARAM_COUNT 4
#define INSERT_BLOCK_LINK_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define INSERT_BLOCK_LINK_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Block_Link (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pParentBlock, struct BlockId *p_pChildBlock)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pParentBlock != NULL);
    ASSERT(p_pChildBlock != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_BLOCK_LINK_PARAMS;
    char *l_pParams[INSERT_BLOCK_LINK_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_BLOCK_LINK_PARAM_COUNT];

    l_pParams[0] = (char *)p_pParentBlock->pHash->pData;
    l_pParamLengths[0] = p_pParentBlock->pHash->iSize;

    l_pParams[1] = (char *)&p_pParentBlock->iLength;

    l_pParams[2] = (char *)p_pChildBlock->pHash->pData;
    l_pParamLengths[2] = p_pChildBlock->pHash->iSize;

    l_pParams[3] = (char *)&p_pChildBlock->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_BLOCK_LINK,
                INSERT_BLOCK_LINK_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_BLOCK_LINK_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}
#define INSERT_EVENT_LINK "INSERT IGNORE INTO LK_Event_Event (Parent_ID, Child_ID) SELECT Event1.Event_ID,Event2.Event_ID from Event as Event1,Event as Event2 where Event1.Nugget_UUID=? AND Event1.Time_Secs=? AND Event1.Time_NanoSecs=? AND Event2.Nugget_UUID=? AND Event2.Time_Secs=? AND Event2.Time_NanoSecs=?"
#define INSERT_EVENT_LINK_PARAM_COUNT 6
#define INSERT_EVENT_LINK_PARAMS { DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }
#define INSERT_EVENT_LINK_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Event_Link (struct DatabaseConnection * p_pDbCon,
                                   struct EventId *p_pParentEvent, struct EventId *p_pChildEvent)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pParentEvent != NULL);
    ASSERT(p_pChildEvent != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_EVENT_LINK_PARAMS;
    char *l_pParams[INSERT_EVENT_LINK_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_EVENT_LINK_PARAM_COUNT];

    char parentUuid[UUID_STRING_LENGTH];
    char childUuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(p_pParentEvent->uuidNuggetId, parentUuid);
    l_pParams[0] = parentUuid;
    l_pParamLengths[0] = 36;

    l_pParams[1] = (char *)&p_pParentEvent->iSeconds;
    l_pParams[2] = (char *)&p_pParentEvent->iNanoSecs;

    uuid_unparse_lower(p_pChildEvent->uuidNuggetId, childUuid);
    l_pParams[3] = childUuid;
    l_pParamLengths[3] = 36;

    l_pParams[4] = (char *)&p_pChildEvent->iSeconds;
    l_pParams[5] = (char *)&p_pChildEvent->iNanoSecs;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_EVENT_LINK,
                INSERT_EVENT_LINK_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_EVENT_LINK_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define INSERT_EVENT "INSERT INTO Event (`Block_ID`, `Nugget_UUID`, `Time_Secs`, `Time_NanoSecs`, `Metabook_ID`) SELECT `Block_ID`, ?, ?, ?, ? FROM `Block` WHERE  `Hash`=? AND `Size`=?"
#define INSERT_EVENT_PARAM_COUNT 6
#define INSERT_EVENT_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define INSERT_EVENT_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Event (struct DatabaseConnection * p_pDbCon,
                                   struct Event *p_pEvent)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pEvent != NULL);
    struct Statement *l_pStmt = NULL;
    uint64_t metabookId = 0;

    DB_TYPE_t l_eParamTypes[] = INSERT_EVENT_PARAMS;
    char *l_pParams[INSERT_EVENT_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_EVENT_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(p_pEvent->pId->uuidNuggetId, uuid);
    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;

    l_pParams[1] = (char *)&p_pEvent->pId->iSeconds;
    l_pParams[2] = (char *)&p_pEvent->pId->iNanoSecs;
    l_pParams[3] = (char *)&metabookId;

    l_pParams[4] = (char *)p_pEvent->pBlock->pId->pHash->pData;
    l_pParamLengths[4] = p_pEvent->pBlock->pId->pHash->iSize;

    l_pParams[5] = (char *)&p_pEvent->pBlock->pId->iLength;
    l_pParamLengths[5] = 4;

    pthread_mutex_lock(&p_pDbCon->mutex);
    Datastore_Gen_MetabookId(p_pDbCon, &metabookId);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_EVENT,
                INSERT_EVENT_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_EVENT_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    if (Datastore_Insert_Metadata_List(p_pDbCon, p_pEvent->pMetaDataList, metabookId) != R_SUCCESS)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to insert event metadata", __func__);
        return R_ERROR;
    }
    pthread_mutex_unlock(&p_pDbCon->mutex);
    if (p_pEvent->pParentId != NULL)
    {
        // Create the link
        return Datastore_Insert_Event_Link (p_pDbCon, p_pEvent->pParentId, p_pEvent->pId);
    }
    return R_SUCCESS;
}

#define GET_BLOCK_DISP "SELECT SF_Flags, ENT_Flags FROM Block WHERE `Hash`=? AND `Size`=?"
#define GET_BLOCK_DISP_PARAM_COUNT 2
#define GET_BLOCK_DISP_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define GET_BLOCK_DISP_FIELD_COUNT 2
#define GET_BLOCK_DISP_FIELDS { DB_TYPE_LONG, DB_TYPE_LONG }

static Lookup_Result
Database_Get_BlockDisposition (struct DatabaseConnection *p_pDbCon,
                                     struct BlockId *p_pBlockId,
                                     uint32_t * p_pSfFlags, 
                                     uint32_t * p_pEntFlags)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    ASSERT(p_pSfFlags != NULL);
    ASSERT(p_pEntFlags != NULL);

    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = GET_BLOCK_DISP_PARAMS;
    char *l_pParams[GET_BLOCK_DISP_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_BLOCK_DISP_PARAM_COUNT];

    DB_TYPE_t l_eResTypes[] = GET_BLOCK_DISP_FIELDS;
    
    struct Field *l_pRow;

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;

    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                GET_BLOCK_DISP,
                GET_BLOCK_DISP_PARAM_COUNT, 
                l_eParamTypes, 
                GET_BLOCK_DISP_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (l_pRow[0].pValue !=NULL)
        memcpy(p_pSfFlags, l_pRow[0].pValue, sizeof(uint32_t));
    else 
        *p_pSfFlags = 0;

    if (l_pRow[1].pValue !=NULL)
        memcpy(p_pEntFlags, l_pRow[1].pValue, sizeof(uint32_t));
    else 
        *p_pEntFlags = 0;
    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_FOUND;
}

#define UPDATE_BLOCK_DISP "UPDATE Block SET `SF_Flags`=?, `ENT_Flags`=? WHERE `Hash`=? AND `Size`=?"
#define UPDATE_BLOCK_DISP_PARAM_COUNT 4
#define UPDATE_BLOCK_DISP_PARAMS { DB_TYPE_LONG, DB_TYPE_LONG, DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define UPDATE_BLOCK_DISP_FIELD_COUNT 0

static Lookup_Result
Database_Update_BlockDisposition (struct DatabaseConnection *p_pDbCon,
                                     struct BlockId *p_pBlockId,
                                     uint32_t p_pSfFlags, 
                                     uint32_t p_pEntFlags)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = UPDATE_BLOCK_DISP_PARAMS;
    char *l_pParams[UPDATE_BLOCK_DISP_PARAM_COUNT];
    unsigned long l_pParamLengths[UPDATE_BLOCK_DISP_PARAM_COUNT];

    l_pParams[0] = (char *)&p_pSfFlags;
    l_pParamLengths[0] = 4;

    l_pParams[1] = (char *)&p_pEntFlags;
    l_pParamLengths[1] = 4;

    l_pParams[2] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[2] = p_pBlockId->pHash->iSize;

    l_pParams[3] = (char *)&p_pBlockId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                UPDATE_BLOCK_DISP,
                UPDATE_BLOCK_DISP_PARAM_COUNT, 
                l_eParamTypes, 
                UPDATE_BLOCK_DISP_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define INSERT_BLOCK_INSPECTION "INSERT INTO Block_Inspection (`Block_ID`, `Event_ID`, `Inspector_Type_UUID`, `Status` ) SELECT Event.Block_ID, Event.Event_ID, ?, ? FROM Event WHERE Event.Time_Secs=? AND Event.Time_NanoSecs=? AND Event.Nugget_UUID=?"
#define INSERT_BLOCK_INSPECTION_PARAM_COUNT 5
#define INSERT_BLOCK_INSPECTION_PARAMS { DB_TYPE_BLOB, DB_TYPE_TINY, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_STRING }
#define INSERT_BLOCK_INSPECTION_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Block_Inspection (struct DatabaseConnection * p_pDbCon,
                                   struct EventId *eventId, uuid_t p_uuidAppType)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(eventId != NULL);
    struct Statement *l_pStmt = NULL;
    uint8_t status = JUDGMENT_REASON_PENDING;
    DB_TYPE_t l_eParamTypes[] = INSERT_BLOCK_INSPECTION_PARAMS;
    char *l_pParams[INSERT_BLOCK_INSPECTION_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_BLOCK_INSPECTION_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH];
    char uuid2[UUID_STRING_LENGTH];
    uuid_unparse_lower(p_uuidAppType, uuid);
    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;
    l_pParams[1] = (char *)&status; 

    uuid_unparse_lower(eventId->uuidNuggetId, uuid2);

    l_pParams[2] = (char *)&eventId->iSeconds;
    l_pParams[3] = (char *)&eventId->iNanoSecs;
    l_pParams[4] = uuid2;
    l_pParamLengths[4] = 36;
    

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_BLOCK_INSPECTION,
                INSERT_BLOCK_INSPECTION_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_BLOCK_INSPECTION_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define UPDATE_BLOCK_INSPECTION "UPDATE Block_Inspection LEFT JOIN `Block` USING (`Block_ID`) SET Status=?, Inspector_UUID=? WHERE Block.Hash=? AND Block.Size=? AND Inspector_Type_UUID=?"
#define UPDATE_BLOCK_INSPECTION_PARAM_COUNT 5
#define UPDATE_BLOCK_INSPECTION_PARAMS { DB_TYPE_TINY, DB_TYPE_BLOB, DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_BLOB }
#define UPDATE_BLOCK_INSPECTION_FIELD_COUNT 0

Lookup_Result
Datastore_Update_Block_Inspection_Status (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uuid_t p_uuidAppType, uuid_t uuidNuggetId, uint8_t status)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = UPDATE_BLOCK_INSPECTION_PARAMS;
    char *l_pParams[UPDATE_BLOCK_INSPECTION_PARAM_COUNT];
    unsigned long l_pParamLengths[UPDATE_BLOCK_INSPECTION_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH], uuid2[UUID_STRING_LENGTH];
    l_pParams[0] = (char *)&status;

    if (uuidNuggetId == NULL)
    {
        l_pParams[1] = NULL;
        l_pParamLengths[1] = 0;
    }
    else
    {
        uuid_unparse_lower(uuidNuggetId, uuid);
        l_pParams[1] = uuid;
        l_pParamLengths[1] = 36;
    }

    l_pParams[2] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[2] = p_pBlockId->pHash->iSize;

    l_pParams[3] = (char *)&p_pBlockId->iLength;

    uuid_unparse_lower(p_uuidAppType, uuid2);
    l_pParams[4] = uuid2;
    l_pParamLengths[4] = 36;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                UPDATE_BLOCK_INSPECTION,
                UPDATE_BLOCK_INSPECTION_PARAM_COUNT, 
                l_eParamTypes, 
                UPDATE_BLOCK_INSPECTION_FIELD_COUNT,
                NULL)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}
#define GET_BLOCK_INSP_APPTYPES "SELECT Inspector_Type_UUID FROM Block LEFT JOIN Block_Inspection USING (Block_ID) WHERE Hash=? and Size=?"
#define GET_BLOCK_INSP_APPTYPES_PARAM_COUNT 2
#define GET_BLOCK_INSP_APPTYPES_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define GET_BLOCK_INSP_APPTYPES_FIELD_COUNT 1
#define GET_BLOCK_INSP_APPTYPES_FIELDS { DB_TYPE_STRING }

extern Lookup_Result
Datastore_Get_Block_Inspection_AppTypes (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, unsigned char **types, uint64_t *count)
{
    ASSERT ( p_pDbCon != NULL );
    unsigned char * l_pRet = NULL;
    struct Statement *l_pStmt = NULL;
    uint64_t rowCount = 0;
    DB_TYPE_t l_eResTypes[] = GET_BLOCK_INSP_APPTYPES_FIELDS;
    DB_TYPE_t l_eParamTypes[] = GET_BLOCK_INSP_APPTYPES_PARAMS;
    char *l_pParams[GET_BLOCK_INSP_APPTYPES_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_BLOCK_INSP_APPTYPES_PARAM_COUNT];

    struct Field *l_pRow;

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                GET_BLOCK_INSP_APPTYPES,
                GET_BLOCK_INSP_APPTYPES_PARAM_COUNT, 
                l_eParamTypes, 
                GET_BLOCK_INSP_APPTYPES_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((rowCount = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    
    if ((l_pRet = calloc(rowCount * 16, sizeof(char))) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    for (uint64_t i = 0; i< rowCount; i++)
    {
        if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            free(l_pRet);
            return R_ERROR;
        }
        uuid_parse(l_pRow[0].pValue, &(l_pRet[i*16]));

        Database_Stmt_FreeRow(l_pStmt, l_pRow);
    }
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *types = l_pRet;
    *count = rowCount;
    return R_SUCCESS;
}

#define GET_INSP_EVENT "SELECT Event.Nugget_UUID, Event.Time_Secs, Event.Time_NanoSecs, Event.Metabook_ID, Event2.Nugget_UUID, Event2.Time_Secs, Event2.Time_NanoSecs FROM Block_Inspection LEFT JOIN Event USING (Event_ID) LEFT JOIN Block ON (Block_Inspection.Block_ID=Block.Block_ID) LEFT JOIN LK_Event_Event ON Event.Event_ID=Child_ID LEFT JOIN Event AS Event2 ON (Event2.Event_ID = Parent_ID) WHERE Block.Hash=? AND Block.Size=? AND Inspector_Type_UUID=?"
#define GET_INSP_EVENT_PARAM_COUNT 3
#define GET_INSP_EVENT_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_STRING }
#define GET_INSP_EVENT_FIELD_COUNT 7
#define GET_INSP_EVENT_FIELDS { DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }


Lookup_Result
Datastore_Get_Block_Inspection_Event (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uuid_t appType, struct Event **r_event)
{
    ASSERT ( p_pDbCon != NULL );
    struct Event *event = NULL;
    uint64_t metabookId;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = GET_INSP_EVENT_FIELDS;

    DB_TYPE_t l_eParamTypes[] = GET_INSP_EVENT_PARAMS;
    char *l_pParams[GET_INSP_EVENT_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_INSP_EVENT_PARAM_COUNT];

    struct Field *l_pRow;

    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(appType, uuid);

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    l_pParams[2] = uuid;
    l_pParamLengths[2] = 36;
    
    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                GET_INSP_EVENT,
                GET_INSP_EVENT_PARAM_COUNT, 
                l_eParamTypes, 
                GET_INSP_EVENT_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((event = calloc(1,sizeof(struct Event))) == NULL)
    {
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((event->pId = calloc(1,sizeof(struct EventId))) == NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Datastore_Get_Block (p_pDbCon, p_pBlockId, &event->pBlock) != R_FOUND)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    uuid_parse(l_pRow[0].pValue, event->pId->uuidNuggetId);
    memcpy(&event->pId->iSeconds, l_pRow[1].pValue, sizeof(uint64_t));
    memcpy(&event->pId->iNanoSecs, l_pRow[2].pValue, sizeof(uint64_t));
    memcpy(&metabookId, l_pRow[3].pValue, sizeof(uint64_t));

    if (metabookId == 0) // No metadata
    {
        if ((event->pMetaDataList = NTLVList_Create()) == NULL)
        {
            Event_Destroy(event);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
    }
    else
    {
        if (Datastore_Get_Metadata(p_pDbCon, metabookId, &event->pMetaDataList) != R_SUCCESS)
        {
            Event_Destroy(event);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
    }

    if (l_pRow[4].pValue == NULL)
    {
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        *r_event = event;
        return R_FOUND;
    }
    if ((event->pParentId = calloc(1,sizeof(struct EventId))) == NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    uuid_parse(l_pRow[4].pValue, event->pParentId->uuidNuggetId);
    memcpy(&event->pParentId->iSeconds, l_pRow[5].pValue, sizeof(uint64_t));
    memcpy(&event->pParentId->iNanoSecs, l_pRow[6].pValue, sizeof(uint64_t));
    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_event = event;
    return R_FOUND;
}

#define GET_INSP_EVENTS_BY_STATUS "SELECT Event.Nugget_UUID, Event.Time_Secs, Event.Time_NanoSecs, Event.Metabook_ID, Event2.Nugget_UUID, Event2.Time_Secs, Event2.Time_NanoSecs, Block.Hash, Block.Size, Block.Data_Type_UUID, Block_Inspection.Inspector_Type_UUID FROM Block_Inspection LEFT JOIN Event USING (Event_ID) LEFT JOIN Block ON (Block_Inspection.Block_ID=Block.Block_ID) LEFT JOIN LK_Event_Event ON Event.Event_ID=Child_ID LEFT JOIN Event AS Event2 ON (Event2.Event_ID = Parent_ID) WHERE Block_Inspection.Status=?"
#define GET_INSP_EVENTS_APP_TYPE_CLAUSE "AND Block_Inspection.Inspector_Type_UUID=?"
#define GET_INSP_EVENTS_BY_STATUS_PARAM_COUNT 2
#define GET_INSP_EVENTS_BY_STATUS_PARAMS { DB_TYPE_TINY, DB_TYPE_STRING }
#define GET_INSP_EVENTS_BY_STATUS_FIELD_COUNT 11
#define GET_INSP_EVENTS_BY_STATUS_FIELDS { DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_STRING, DB_TYPE_STRING }


Lookup_Result
Datastore_Get_Block_Inspection_Events_By_Status (struct DatabaseConnection * p_pDbCon, uint8_t status,
        uuid_t appType, struct Event ***r_event, unsigned char **r_appTypes, uint64_t *r_count)
{
    ASSERT ( p_pDbCon != NULL );
    struct Event **event = NULL;
    struct Event *cur = NULL;
    unsigned char *appTypes;
    struct BlockId *id;
    uint64_t rowCount = 0;
    uint64_t metabookId;
    struct Statement *l_pStmt = NULL;
    int paramCount = 1;
    char *query;

    DB_TYPE_t l_eResTypes[] = GET_INSP_EVENTS_BY_STATUS_FIELDS;

    DB_TYPE_t l_eParamTypes[] = GET_INSP_EVENTS_BY_STATUS_PARAMS;
    char *l_pParams[GET_INSP_EVENTS_BY_STATUS_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_INSP_EVENTS_BY_STATUS_PARAM_COUNT];

    struct Field *l_pRow;

    char uuid[UUID_STRING_LENGTH];

    l_pParams[0] = (char *)&status;

    if (appType == NULL)
    {
        query = (char *)GET_INSP_EVENTS_BY_STATUS;
        l_pParams[1] = NULL;
        l_pParamLengths[1] = 0;
    }
    else
    {
        if (asprintf(&query, "%s %s", GET_INSP_EVENTS_BY_STATUS, GET_INSP_EVENTS_APP_TYPE_CLAUSE) == -1 )
            return R_ERROR;
        paramCount = 2;
        uuid_unparse_lower(appType, uuid);
        l_pParams[1] = uuid;
        l_pParamLengths[1] = 36;
    }

    pthread_mutex_lock(&p_pDbCon->mutex);

    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                query,
                paramCount,
                l_eParamTypes, 
                GET_INSP_EVENTS_BY_STATUS_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        if (appType != NULL)
            free(query);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        if (appType != NULL)
            free(query);
        return R_ERROR;
    }
    if ((rowCount = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        if (appType != NULL)
            free(query);
        return R_NOT_FOUND;
    }
    if ((event = calloc(rowCount,sizeof(struct Event*))) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        if (appType != NULL)
            free(query);
        return R_ERROR;
    }
    if ((appTypes = calloc(rowCount*16,sizeof(char))) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        if (appType != NULL)
            free(query);
        return R_ERROR;
    }


    for (uint64_t i = 0; i< rowCount; i++)
    {
        if ((cur = calloc(1,sizeof(struct Event))) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            if (appType != NULL)
                free(query);
            return R_ERROR;
        }

        if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            if (appType != NULL)
                free(query);
            return R_ERROR;
        }
        if ((cur->pId = calloc(1,sizeof(struct EventId))) == NULL)
        {
            Event_Destroy(cur);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            if (appType != NULL)
                free(query);
            return R_ERROR;
        }
        if ((id = BlockId_Create_Shallow()) == NULL)
        {
            Event_Destroy(cur);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            if (appType != NULL)
                free(query);
            return R_ERROR;
        }

        if ((id->pHash = Hash_Create()) == NULL)
        {
            Event_Destroy(cur);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            if (appType != NULL)
                free(query);
            return R_ERROR;
        }
        // XXX: Ugly hack to load a hash.
        free(id->pHash->pData);
        id->pHash->pData = (uint8_t *)l_pRow[7].pValue;
        id->pHash->iSize = l_pRow[7].iLength;
        id->pHash->iFlags = HASH_FLAG_FINAL;
        l_pRow[7].pValue = NULL;

        memcpy(&id->iLength, l_pRow[8].pValue, sizeof(uint64_t));

        uuid_parse(l_pRow[9].pValue, id->uuidDataType);


        if (Datastore_Get_Block (p_pDbCon, id, &cur->pBlock) != R_FOUND)
        {
            Event_Destroy(cur);
            BlockId_Destroy(id);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            if (appType != NULL)
                free(query);
            return R_ERROR;
        }
        BlockId_Destroy(id);

        uuid_parse(l_pRow[10].pValue, &(appTypes[i*16]));
        uuid_parse(l_pRow[0].pValue, cur->pId->uuidNuggetId);
        memcpy(&cur->pId->iSeconds, l_pRow[1].pValue, sizeof(uint64_t));
        memcpy(&cur->pId->iNanoSecs, l_pRow[2].pValue, sizeof(uint64_t));
        memcpy(&metabookId, l_pRow[3].pValue, sizeof(uint64_t));

        if (metabookId == 0) // No metadata
        {
            if ((cur->pMetaDataList = NTLVList_Create()) == NULL)
            {
                Event_Destroy(cur);
                Database_Stmt_FreeRow(l_pStmt, l_pRow);
                Database_Stmt_Destroy(l_pStmt);
                pthread_mutex_unlock(&p_pDbCon->mutex);
                if (appType != NULL)
                    free(query);
                return R_ERROR;
            }
        }
        else
        {
            if (Datastore_Get_Metadata(p_pDbCon, metabookId, &cur->pMetaDataList) != R_SUCCESS)
            {
                if (appType != NULL)
                    free(query);
                Event_Destroy(cur);
                Database_Stmt_FreeRow(l_pStmt, l_pRow);
                Database_Stmt_Destroy(l_pStmt);
                pthread_mutex_unlock(&p_pDbCon->mutex);
                return R_ERROR;
            }
        }

        if (l_pRow[4].pValue == NULL)
        {
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            event[i] = cur;
            continue;
        }
        if ((cur->pParentId = calloc(1,sizeof(struct EventId))) == NULL)
        {
            Event_Destroy(cur);
            if (appType != NULL)
                free(query);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
        uuid_parse(l_pRow[4].pValue, cur->pParentId->uuidNuggetId);
        memcpy(&cur->pParentId->iSeconds, l_pRow[5].pValue, sizeof(uint64_t));
        memcpy(&cur->pParentId->iNanoSecs, l_pRow[6].pValue, sizeof(uint64_t));

        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        event[i] =cur;
    }

    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    if (appType != NULL)
        free(query);
    *r_event = event;
    *r_count = rowCount;
    *r_appTypes = appTypes;
    return R_FOUND;
}


#define COUNT_BLOCK_INSPECTION "SELECT count(*) FROM Block_Inspection LEFT JOIN Block USING (`Block_ID`)  WHERE Block.Hash=? and Block.Size=? AND `Status`=?"
#define COUNT_BLOCK_INSPECTION_PARAM_COUNT 3
#define COUNT_BLOCK_INSPECTION_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_TINY }
#define COUNT_BLOCK_INSPECTION_FIELD_COUNT 1
#define COUNT_BLOCK_INSPECTION_FIELDS { DB_TYPE_LONGLONG }


Lookup_Result
Datastore_Count_Outstanding_Block_Inspections (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uint64_t *p_rCount)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    struct Statement *l_pStmt = NULL;
    struct Field *l_pRow;
    uint8_t status = JUDGMENT_REASON_PENDING;

    DB_TYPE_t l_eResTypes[] = COUNT_BLOCK_INSPECTION_FIELDS;
    DB_TYPE_t l_eParamTypes[] = COUNT_BLOCK_INSPECTION_PARAMS;
    char *l_pParams[COUNT_BLOCK_INSPECTION_PARAM_COUNT];
    unsigned long l_pParamLengths[COUNT_BLOCK_INSPECTION_PARAM_COUNT];

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;
    l_pParams[2] = (char *)&status;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                COUNT_BLOCK_INSPECTION,
                COUNT_BLOCK_INSPECTION_PARAM_COUNT, 
                l_eParamTypes, 
                COUNT_BLOCK_INSPECTION_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to get row", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to get rows", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (l_pRow[0].pValue !=NULL)
    {
        memcpy(p_rCount, l_pRow[0].pValue, sizeof(uint64_t));
    }
    else 
    {
        *p_rCount = 0;
    }
    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

struct MD_Userdata
{
    struct DatabaseConnection * dbCon;
    uint64_t metabookId;
};
#define INSERT_METADATA "INSERT INTO Metadata (`Metadata_Type_UUID`, `Metadata_Name_UUID`, `Metabook_ID`, `Metadata` ) VALUES (?,?,?,?)"
#define INSERT_METADATA_PARAM_COUNT 4
#define INSERT_METADATA_PARAMS { DB_TYPE_BLOB, DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_BLOB }
#define INSERT_METADATA_FIELD_COUNT 0

int
Datastore_Insert_Metadata (struct NTLVItem *item, struct MD_Userdata *ud)
{
    ASSERT(item != NULL);
    ASSERT(ud != NULL);

    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_METADATA_PARAMS;
    char *l_pParams[INSERT_METADATA_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_METADATA_PARAM_COUNT];
    char uuid1[UUID_STRING_LENGTH], uuid2[UUID_STRING_LENGTH];

    uuid_unparse_lower(item->uuidType, uuid1);
    uuid_unparse_lower(item->uuidName, uuid2);

    l_pParams[0] = uuid1;
    l_pParamLengths[0] = 36;

    l_pParams[1] = uuid2;
    l_pParamLengths[1] = 36;

    l_pParams[2] = (char *)&ud->metabookId;

    l_pParams[3] = (char *)item->pData;
    l_pParamLengths[3] = item->iLength;



    pthread_mutex_lock(&ud->dbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(ud->dbCon, 
                INSERT_METADATA,
                INSERT_METADATA_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_METADATA_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&ud->dbCon->mutex);
        return LIST_EACH_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&ud->dbCon->mutex);
        return LIST_EACH_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&ud->dbCon->mutex);
    return LIST_EACH_OK;
}

Lookup_Result
Datastore_Insert_Metadata_List (struct DatabaseConnection * p_pDbCon,
                                   struct List *list, uint64_t metabookId)
{
    struct MD_Userdata userData;
    userData.dbCon = p_pDbCon;
    userData.metabookId = metabookId;
    pthread_mutex_lock(&p_pDbCon->mutex);

    if (!List_ForEach(list, (int (*)(void *, void *))Datastore_Insert_Metadata, &userData))
    {
        rzb_log(LOG_ERR, "%s: Failed to insert metadata item", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}



#define GET_BLOCK_METABOOK_ID "SELECT Metabook_ID FROM Block WHERE `Hash`=? AND `Size`=?"
#define GET_BLOCK_METABOOK_ID_PARAM_COUNT 2
#define GET_BLOCK_METABOOK_ID_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define GET_BLOCK_METABOOK_ID_FIELD_COUNT 1
#define GET_BLOCK_METABOOK_ID_FIELDS { DB_TYPE_LONGLONG }

Lookup_Result
Datastore_Get_Block_MetabookId (struct DatabaseConnection *p_pDbCon,
                                     struct BlockId *p_pBlockId,
                                     uint64_t *metabookId)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    ASSERT(metabookId != NULL);

    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = GET_BLOCK_METABOOK_ID_PARAMS;
    char *l_pParams[GET_BLOCK_METABOOK_ID_PARAM_COUNT];
    unsigned long l_pParamLengths[GET_BLOCK_METABOOK_ID_PARAM_COUNT];

    DB_TYPE_t l_eResTypes[] = GET_BLOCK_METABOOK_ID_FIELDS;
    
    struct Field *l_pRow;

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;

    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                GET_BLOCK_METABOOK_ID,
                GET_BLOCK_METABOOK_ID_PARAM_COUNT, 
                l_eParamTypes, 
                GET_BLOCK_METABOOK_ID_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        return R_ERROR;
    }

    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: No rows", __func__);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to fetch row", __func__);
        return R_ERROR;
    }
    if (l_pRow[0].pValue !=NULL)
        memcpy(metabookId, l_pRow[0].pValue, sizeof(uint64_t));
    else 
        *metabookId = 0;

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_FOUND;
}


Lookup_Result
Datastore_Get_BlockDisposition (struct BlockId *blockId,
                              struct DatabaseConnection *dbCon,
                              uint32_t * sfFlags, uint32_t * entFlags,
                              bool create)
{
    uint32_t l_iSfFlags, l_iEntFlags;
    Lookup_Result l_eLookupResult, memcachedResult;

    GlobalCache_Lock ();

    l_eLookupResult =
        checkGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                          &l_iSfFlags, &l_iEntFlags, BADHASH);

    // If we miss the cache check the DB.
    if (l_eLookupResult != R_FOUND)
    {
        if ((l_eLookupResult =
             checkGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                               &l_iSfFlags, &l_iEntFlags,
                               GOODHASH)) != R_FOUND)
        {

            l_eLookupResult = Database_Get_BlockDisposition (dbCon, blockId,
                                                             &l_iSfFlags,
                                                             &l_iEntFlags);
            switch (l_eLookupResult)
            {
            case R_NOT_FOUND:
                if (create)
                {
                    l_iSfFlags = SF_FLAG_PROCESSING;
                    l_iEntFlags = 0;
                    if (Datastore_Insert_Block (dbCon, blockId) != R_SUCCESS)
                    {
                        rzb_log (LOG_INFO, "%s: Faile to insert block", __func__);
                    }

                    if (Database_Update_BlockDisposition (dbCon, blockId,
                                                      l_iSfFlags,
                                                      l_iEntFlags) != R_SUCCESS)
                    {
                        rzb_log (LOG_INFO, "%s: Failed to update block", __func__);
                    }

                    memcachedResult =
                        addGlobalEntry (blockId->pHash->pData,
                                        blockId->pHash->iSize, l_iSfFlags,
                                        l_iEntFlags, GOODHASH);
                    if (memcachedResult != R_SUCCESS)
                        rzb_log (LOG_ERR, "%s: Could not update global cache",
                                 __func__);

                }
                break;

            case R_FOUND:
                rzb_log (LOG_INFO, "%s: Block Found", __func__);
                if (l_iSfFlags & SF_FLAG_BAD)
                    memcachedResult =
                        addGlobalEntry (blockId->pHash->pData,
                                        blockId->pHash->iSize, l_iSfFlags,
                                        l_iEntFlags, BADHASH);
                else
                    memcachedResult =
                        addGlobalEntry (blockId->pHash->pData,
                                        blockId->pHash->iSize, l_iSfFlags,
                                        l_iEntFlags, GOODHASH);

                if (memcachedResult != R_SUCCESS)
                    rzb_log (LOG_ERR, "%s: Could not update global cache",
                             __func__);
                break;

            case R_SUCCESS:
            case R_ERROR:
            default:
                // OH CRAP
                rzb_log (LOG_ERR,
                         "%s: Bad lookup result, how did i get here???? %d",
                         __func__, l_eLookupResult);
                break;
            }
        }
    }

    *sfFlags = l_iSfFlags;
    *entFlags = l_iEntFlags;
    GlobalCache_Unlock ();
    return l_eLookupResult;
}


Lookup_Result
Datastore_Set_BlockDisposition (struct BlockId * blockId,
                              struct DatabaseConnection * dbCon,
                              uint32_t sfFlags, uint32_t entFlags)
{
    Lookup_Result l_eLookupResult, memcachedResult;
    uint32_t l_iSfFlags, l_iEntFlags;
    CacheType cacheType;

    GlobalCache_Lock ();
    if (Database_Update_BlockDisposition (dbCon, blockId, sfFlags, entFlags)
        != R_SUCCESS)
    {
        rzb_log (LOG_ERR, "%s: Failed to update flags in DB", __func__);
        GlobalCache_Unlock ();
        return R_ERROR;
    }

    if ((sfFlags & SF_FLAG_BAD) == SF_FLAG_BAD)
    {
        removeGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                           GOODHASH);
        cacheType = BADHASH;
    }
    else
    {
        removeGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                           BADHASH);
        cacheType = GOODHASH;
    }
    l_eLookupResult =
        checkGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                          &l_iSfFlags, &l_iEntFlags, cacheType);
    memcachedResult = R_ERROR;
    switch (l_eLookupResult)
    {
    case R_FOUND:
        memcachedResult =
            updateGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                               sfFlags, entFlags, cacheType);
        break;
    case R_NOT_FOUND:
        memcachedResult =
            addGlobalEntry (blockId->pHash->pData, blockId->pHash->iSize,
                            sfFlags, entFlags, cacheType);
        break;
    default:
        // OH CRAP
        rzb_log (LOG_ERR, "%s: Bad lookup result, how did i get here???? %d",
                 __func__, l_eLookupResult);
        break;
    }
    if (memcachedResult != R_SUCCESS)
        rzb_log (LOG_ERR, "%s: Could not update global cache", __func__);

    GlobalCache_Unlock ();
    return memcachedResult;
}

#define INSERT_LOG_WITH_EVENT "INSERT INTO `Log_Message` (`Event_ID`, `Nugget_UUID`, `Priority`, `Message`) SELECT `Event_ID`, ?, ?, ? FROM `Event` WHERE  `Nugget_UUID`=? AND `Time_Secs`=? AND `Time_NanoSecs`=?"
#define INSERT_LOG_WITH_EVENT_PARAM_COUNT 6
#define INSERT_LOG_WITH_EVENT_PARAMS { DB_TYPE_BLOB, DB_TYPE_TINY, DB_TYPE_STRING, DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }
#define INSERT_LOG_WITH_EVENT_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Log_Message_With_Event (struct DatabaseConnection * p_pDbCon,
                                    uuid_t uuidNuggetId,
                                    uint8_t p_iPriority, uint8_t *p_sMessage,
                                    struct EventId *p_pEventId)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pEventId != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_LOG_WITH_EVENT_PARAMS;
    char *l_pParams[INSERT_LOG_WITH_EVENT_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_LOG_WITH_EVENT_PARAM_COUNT];
    char uuid1[UUID_STRING_LENGTH], uuid2[UUID_STRING_LENGTH];

    uuid_unparse_lower(uuidNuggetId, uuid1);
    uuid_unparse_lower(p_pEventId->uuidNuggetId, uuid2);

    l_pParams[0] = uuid1;
    l_pParamLengths[0] = 36;

    l_pParams[1] = (char *)&p_iPriority;

    l_pParams[2] = (char *)p_sMessage;
    l_pParamLengths[2] = strlen((const char *)p_sMessage);

    l_pParams[3] = uuid2;
    l_pParamLengths[3] = 36;

    l_pParams[4] = (char *)&p_pEventId->iSeconds;
    l_pParams[5] = (char *)&p_pEventId->iNanoSecs;


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_LOG_WITH_EVENT,
                INSERT_LOG_WITH_EVENT_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_LOG_WITH_EVENT_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define INSERT_LOG "INSERT INTO `Log_Message` (`Event_ID`, `Nugget_UUID`, `Priority`, `Message`) VALUES (NULL, ?, ?, ?)"
#define INSERT_LOG_PARAM_COUNT 3
#define INSERT_LOG_PARAMS { DB_TYPE_BLOB, DB_TYPE_TINY, DB_TYPE_STRING }
#define INSERT_LOG_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Log_Message (struct DatabaseConnection * p_pDbCon, uuid_t uuidNuggetId,
                                    uint8_t p_iPriority, uint8_t *p_sMessage)
{
    ASSERT(p_pDbCon != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_LOG_PARAMS;
    char *l_pParams[INSERT_LOG_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_LOG_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(uuidNuggetId, uuid);

    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;

    l_pParams[1] = (char *)&p_iPriority;

    l_pParams[2] = (char *)p_sMessage;
    l_pParamLengths[2] = strlen((const char *)p_sMessage);


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_LOG,
                INSERT_LOG_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_LOG_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}







#define SELECT_EVENT_BLOCK_ID "SELECT Hash, Data_Type_UUID, Size FROM Event LEFT JOIN Block USING (Block_ID) WHERE Nugget_UUID=? AND Time_Secs=? AND Time_NanoSecs=?"
#define SELECT_EVENT_BLOCK_ID_PARAM_COUNT 3
#define SELECT_EVENT_BLOCK_ID_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }
#define SELECT_EVENT_BLOCK_ID_FIELD_COUNT 3
#define SELECT_EVENT_BLOCK_ID_FIELDS { DB_TYPE_BLOB, DB_TYPE_STRING, DB_TYPE_LONGLONG }

Lookup_Result
Datastore_Get_Event_BlockId (struct DatabaseConnection * p_pDbCon,
                                   struct EventId *p_pEventId, struct BlockId **r_pBlockId)
{
    ASSERT ( p_pDbCon != NULL );
    struct BlockId *l_pRet = NULL;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = SELECT_EVENT_BLOCK_ID_FIELDS;

    DB_TYPE_t l_eParamTypes[] = SELECT_EVENT_BLOCK_ID_PARAMS;
    char *l_pParams[SELECT_EVENT_BLOCK_ID_PARAM_COUNT];
    unsigned long l_pParamLengths[SELECT_EVENT_BLOCK_ID_PARAM_COUNT];

    struct Field *l_pRow;

    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(p_pEventId->uuidNuggetId, uuid);

    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;
    
    l_pParams[1] = (char *)&p_pEventId->iSeconds;
    l_pParams[2] = (char *)&p_pEventId->iNanoSecs;
    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                SELECT_EVENT_BLOCK_ID,
                SELECT_EVENT_BLOCK_ID_PARAM_COUNT, 
                l_eParamTypes, 
                SELECT_EVENT_BLOCK_ID_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    
    if ((l_pRet = BlockId_Create_Shallow()) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((l_pRet->pHash = Hash_Create()) == NULL)
    {
        BlockId_Destroy(l_pRet);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    // XXX: Ugly hack to load a hash.
    free(l_pRet->pHash->pData);
    l_pRet->pHash->pData = (uint8_t *)l_pRow[0].pValue;
    l_pRet->pHash->iSize = l_pRow[0].iLength;
    l_pRet->pHash->iFlags = HASH_FLAG_FINAL;
    l_pRow[0].pValue = NULL;

    uuid_parse(l_pRow[1].pValue, l_pRet->uuidDataType);

    memcpy(&l_pRet->iLength, l_pRow[2].pValue, sizeof(uint64_t));

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_pBlockId = l_pRet;
    return R_FOUND;
}
#define INSERT_ALERT "INSERT INTO Alert (`Event_ID`, `Block_ID`, `Nugget_UUID`, `Time_Secs`, `Time_NanoSecs`, `Metabook_ID`, `GID`, `SID`, `Priority`, `Message` ) SELECT `Event_ID`, `Block_ID`, ?, ?, ?, ?, ?, ?, ?, ? FROM `Event` WHERE  `Nugget_UUID`=? AND `Time_Secs`=? AND `Time_NanoSecs`=?"
#define INSERT_ALERT_PARAM_COUNT 11
#define INSERT_ALERT_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG, DB_TYPE_LONG, DB_TYPE_LONG, DB_TYPE_SHORT, DB_TYPE_STRING, DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }
#define INSERT_ALERT_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Alert (struct DatabaseConnection * p_pDbCon,
                                   struct Judgment *p_pJudgment)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pJudgment != NULL);
    struct Statement *l_pStmt = NULL;
    uint64_t metabookId = 0;

    DB_TYPE_t l_eParamTypes[] = INSERT_ALERT_PARAMS;
    char *l_pParams[INSERT_ALERT_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_ALERT_PARAM_COUNT];
    char uuid1[UUID_STRING_LENGTH], uuid2[UUID_STRING_LENGTH];
    uuid_unparse_lower(p_pJudgment->uuidNuggetId, uuid1);
    uuid_unparse_lower(p_pJudgment->pEventId->uuidNuggetId, uuid2);

    l_pParams[0] = uuid1;
    l_pParamLengths[0] = 36;

    l_pParams[1] = (char *)&p_pJudgment->iSeconds;
    l_pParams[2] = (char *)&p_pJudgment->iNanoSecs;
    l_pParams[3] = (char *)&metabookId;

    l_pParams[4] = (char *)&p_pJudgment->iGID;
    l_pParams[5] = (char *)&p_pJudgment->iSID;
    l_pParams[6] = (char *)&p_pJudgment->iPriority;
    l_pParams[7] = (char *)p_pJudgment->sMessage;
    l_pParamLengths[7] = strlen((char *)p_pJudgment->sMessage);

    l_pParams[8] = uuid2;
    l_pParamLengths[8] = 36;
    l_pParams[9] = (char *)&p_pJudgment->pEventId->iSeconds;
    l_pParams[10] = (char *)&p_pJudgment->pEventId->iNanoSecs;

    pthread_mutex_lock(&p_pDbCon->mutex);
    Datastore_Gen_MetabookId(p_pDbCon, &metabookId);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_ALERT,
                INSERT_ALERT_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_ALERT_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    if (Datastore_Insert_Metadata_List(p_pDbCon, p_pJudgment->pMetaDataList, metabookId) != R_SUCCESS)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to insert event metadata", __func__);
        return R_ERROR;
    }
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}





#define SELECT_BLOCK_SET_BAD "SELECT DISTINCT ParentBlock.Hash, ParentBlock.Data_Type_UUID, ParentBlock.Size, ChildBlock.Hash, ChildBlock.Data_Type_UUID, ChildBlock.Size  FROM LK_Block_Block LEFT JOIN Block AS ParentBlock ON (Parent_ID=ParentBlock.Block_ID) LEFT JOIN Block AS ChildBlock ON (Child_ID=ChildBlock.Block_ID) WHERE (LK_Block_Block.SF_Flags & 0x00000002) != 0 AND (ParentBlock.SF_Flags & 0x00000002) = 0"
#define SELECT_BLOCK_SET_BAD_PARAM_COUNT 0
#define SELECT_BLOCK_SET_BAD_FIELD_COUNT 6
#define SELECT_BLOCK_SET_BAD_FIELDS { DB_TYPE_BLOB, DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_STRING, DB_TYPE_LONGLONG }

Lookup_Result
Datastore_Get_BlockIds_To_Set_Bad (struct DatabaseConnection * p_pDbCon,
                                   struct BlockChange **r_pBlockChange, uint32_t *count)
{
    ASSERT ( p_pDbCon != NULL );
    struct BlockChange *l_pRet = NULL;
    struct Statement *l_pStmt = NULL;
    uint32_t rows = 0;
    DB_TYPE_t l_eResTypes[] = SELECT_BLOCK_SET_BAD_FIELDS;

    struct Field *l_pRow;

    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                SELECT_BLOCK_SET_BAD,
                SELECT_BLOCK_SET_BAD_PARAM_COUNT, 
                NULL, 
                SELECT_BLOCK_SET_BAD_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, NULL, NULL))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((rows = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_SUCCESS;
    }
    
    if ((l_pRet = calloc(rows, sizeof(struct BlockChange)))== NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    for (uint32_t i =0; i < rows; i++)
    {
        if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
        if ( ((l_pRet[i].parent = calloc(1,sizeof(struct BlockId))) == NULL) ||
                ((l_pRet[i].child = calloc(1,sizeof(struct BlockId))) == NULL))
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
        if ( ((l_pRet[i].parent->pHash = Hash_Create()) == NULL) ||
                ((l_pRet[i].child->pHash = Hash_Create()) == NULL))
        {
            free(l_pRet);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
        // XXX: Ugly hack to load a hash.
        free(l_pRet[i].parent->pHash->pData);
        free(l_pRet[i].child->pHash->pData);
        l_pRet[i].parent->pHash->pData = (uint8_t *)l_pRow[0].pValue;
        l_pRet[i].parent->pHash->iSize = l_pRow[0].iLength;
        l_pRet[i].parent->pHash->iFlags = HASH_FLAG_FINAL;
        l_pRow[0].pValue = NULL;

        uuid_parse(l_pRow[1].pValue, l_pRet[i].parent->uuidDataType);
    
        memcpy(&l_pRet[i].parent->iLength, l_pRow[2].pValue, sizeof(uint64_t));

        l_pRet[i].child->pHash->pData = (uint8_t *)l_pRow[3].pValue;
        l_pRet[i].child->pHash->iSize = l_pRow[3].iLength;
        l_pRet[i].child->pHash->iFlags = HASH_FLAG_FINAL;
        l_pRow[3].pValue = NULL;

        uuid_parse(l_pRow[4].pValue, l_pRet[i].child->uuidDataType);
    
        memcpy(&l_pRet[i].child->iLength, l_pRow[5].pValue, sizeof(uint64_t));

        Database_Stmt_FreeRow(l_pStmt, l_pRow);
    }
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_pBlockChange = l_pRet;
    *count=rows;
    return R_SUCCESS;
}



#define INSERT_APP_TYPE "INSERT INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) SELECT ?, ?, `UUID`, ? FROM `Nugget_Type` WHERE  `Name`=?"
#define INSERT_APP_TYPE_PARAM_COUNT 4
#define INSERT_APP_TYPE_PARAMS { DB_TYPE_STRING, DB_TYPE_BLOB, DB_TYPE_STRING, DB_TYPE_STRING }
#define INSERT_APP_TYPE_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_App_Type (struct DatabaseConnection * p_pDbCon,
                                    char *name, uuid_t uuidId,
                                    char *description, char *nuggetType)
{
    ASSERT(p_pDbCon != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_APP_TYPE_PARAMS;
    char *l_pParams[INSERT_APP_TYPE_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_APP_TYPE_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(uuidId, uuid);

    l_pParams[0] = name;
    l_pParamLengths[0] = strlen(name);

    l_pParams[1] = uuid;
    l_pParamLengths[1] = 36;

    l_pParams[2] = description;
    l_pParamLengths[2] = strlen(description);

    l_pParams[3] = nuggetType;
    l_pParamLengths[3] = strlen(nuggetType);


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_APP_TYPE,
                INSERT_APP_TYPE_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_APP_TYPE_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define INSERT_NUGGET "INSERT INTO `Nugget` (`Name`, `UUID`, `App_Type_UUID`, `Nugget_Type_UUID`) VALUES (?, ?, ?, ?)"
#define INSERT_NUGGET_PARAM_COUNT 4
#define INSERT_NUGGET_PARAMS { DB_TYPE_STRING, DB_TYPE_BLOB, DB_TYPE_BLOB, DB_TYPE_BLOB }
#define INSERT_NUGGET_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_Nugget (struct DatabaseConnection * p_pDbCon,
                                    char *name, uuid_t uuidId,
                                    uuid_t uuidAppType, uuid_t uuidNuggetType)
{
    ASSERT(p_pDbCon != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = INSERT_NUGGET_PARAMS;
    char *l_pParams[INSERT_NUGGET_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_NUGGET_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH], uuid2[UUID_STRING_LENGTH], uuid3[UUID_STRING_LENGTH];

    uuid_unparse_lower(uuidId, uuid);

    l_pParams[0] = name;
    l_pParamLengths[0] = strlen(name);

    l_pParams[1] = uuid;
    l_pParamLengths[1] = 36;

    if (uuid_is_null(uuidAppType) == 1)
    {            
        l_pParams[2] = NULL;
        l_pParamLengths[2] = 0;
    }
    else
    {
        uuid_unparse_lower(uuidAppType, uuid2);
        l_pParams[2] = uuid2;
        l_pParamLengths[2] = 36;
    }
    uuid_unparse_lower(uuidNuggetType, uuid3);
    l_pParams[3] = uuid3;
    l_pParamLengths[3] = 36;


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                INSERT_NUGGET,
                INSERT_NUGGET_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_NUGGET_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define UPDATE_NUGGET "UPDATE `Nugget` SET `Name`=?, `App_Type_UUID`=?, `Nugget_Type_UUID`=?, `Contact`=?, `Location`=?, `Notes`=? WHERE `UUID`=?"
#define UPDATE_NUGGET_PARAM_COUNT 7
#define UPDATE_NUGGET_PARAMS { DB_TYPE_STRING, DB_TYPE_BLOB, DB_TYPE_BLOB, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_BLOB }
#define UPDATE_NUGGET_FIELD_COUNT 0

Lookup_Result
Datastore_Update_Nugget (struct DatabaseConnection * p_pDbCon, struct Nugget *nugget)
{
    ASSERT(p_pDbCon != NULL);
    struct Statement *l_pStmt = NULL;

    DB_TYPE_t l_eParamTypes[] = UPDATE_NUGGET_PARAMS;
    char *l_pParams[UPDATE_NUGGET_PARAM_COUNT];
    unsigned long l_pParamLengths[UPDATE_NUGGET_PARAM_COUNT];
    char uuid1[UUID_STRING_LENGTH], uuid2[UUID_STRING_LENGTH], uuid3[UUID_STRING_LENGTH];

    l_pParams[0] = nugget->sName;
    l_pParamLengths[0] = strlen(nugget->sName);

    
    if (uuid_is_null(nugget->uuidApplicationType) == 1)
    {            
        l_pParams[1] = NULL;
        l_pParamLengths[1] = 0;
    }
    else
    {
        uuid_unparse_lower(nugget->uuidApplicationType, uuid1);
        l_pParams[1] = uuid1;
        l_pParamLengths[1] = 36;
    }
    uuid_unparse_lower(nugget->uuidNuggetType, uuid2);
    l_pParams[2] = uuid2;
    l_pParamLengths[2] = 36;

    if (nugget->sContact == NULL)
    {            
        l_pParams[3] = NULL;
        l_pParamLengths[3] = 0;
    }
    else
    {
        l_pParams[3] = (char *)nugget->sContact;
        l_pParamLengths[3] = strlen(nugget->sContact);
    }

    if (nugget->sLocation == NULL)
    {            
        l_pParams[4] = NULL;
        l_pParamLengths[4] = 0;
    }
    else
    {
        l_pParams[4] = (char *)nugget->sLocation;
        l_pParamLengths[4] = strlen(nugget->sLocation);
    }

    if (nugget->sNotes == NULL)
    {            
        l_pParams[5] = NULL;
        l_pParamLengths[5] = 0;
    }
    else
    {
        l_pParams[5] = (char *)nugget->sNotes;
        l_pParamLengths[5] = strlen(nugget->sNotes);
    }

    uuid_unparse_lower(nugget->uuidNuggetId, uuid3);
    l_pParams[6] = uuid3;
    l_pParamLengths[6] = 36;


    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                UPDATE_NUGGET,
                UPDATE_NUGGET_PARAM_COUNT, 
                l_eParamTypes, 
                UPDATE_NUGGET_FIELD_COUNT,
                NULL)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        return R_ERROR;
    }

    Database_Stmt_Destroy(l_pStmt);
    
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}



#define SELECT_APP_TYPES "SELECT UUID, Nugget_Type_UUID, Name, Description FROM App_Type"
#define SELECT_APP_TYPES_PARAM_COUNT 0
#define SELECT_APP_TYPES_FIELD_COUNT 4
#define SELECT_APP_TYPES_FIELDS { DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING }

Lookup_Result
Datastore_Get_AppTypes (struct DatabaseConnection * p_pDbCon,
                                   struct AppType **r_pAppType, uint64_t *count)
{
    ASSERT ( p_pDbCon != NULL );
    struct AppType *l_pRet = NULL;
    struct Statement *l_pStmt = NULL;
    uint64_t rowCount = 0;
    DB_TYPE_t l_eResTypes[] = SELECT_APP_TYPES_FIELDS;

    struct Field *l_pRow;

    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                SELECT_APP_TYPES,
                SELECT_APP_TYPES_PARAM_COUNT, 
                NULL, 
                SELECT_APP_TYPES_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, NULL, NULL))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((rowCount = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    
    if ((l_pRet = calloc(rowCount, sizeof(struct AppType))) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    for (uint64_t i = 0; i< rowCount; i++)
    {
        if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            free(l_pRet);
            return R_ERROR;
        }
        uuid_parse(l_pRow[0].pValue, l_pRet[i].uuidId);
        uuid_parse(l_pRow[1].pValue, l_pRet[i].uuidNuggetType);

        l_pRet[i].sName = l_pRow[2].pValue;
        l_pRet[i].sDescription = l_pRow[3].pValue;
        l_pRow[2].pValue = NULL;
        l_pRow[3].pValue = NULL;

        Database_Stmt_FreeRow(l_pStmt, l_pRow);
    }
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_pAppType = l_pRet;
    *count = rowCount;
    return R_SUCCESS;
}

#define LOAD_METADATA "SELECT Metadata_Type_UUID, Metadata_Name_UUID, Metadata FROM Metadata WHERE Metabook_ID=?"
#define LOAD_METADATA_PARAM_COUNT 1
#define LOAD_METADATA_PARAMS { DB_TYPE_LONGLONG }
#define LOAD_METADATA_FIELD_COUNT 3
#define LOAD_METADATA_FIELDS { DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_BLOB }


Lookup_Result
Datastore_Get_Metadata (struct DatabaseConnection * p_pDbCon, uint64_t metabookId,
                                   struct List **r_list)
{
    ASSERT ( p_pDbCon != NULL );
    struct List *list = NULL;
    struct Statement *l_pStmt = NULL;
    uint32_t rows = 0;
    uuid_t type, name;
    DB_TYPE_t l_eResTypes[] = LOAD_METADATA_FIELDS;

    DB_TYPE_t l_eParamTypes[] = LOAD_METADATA_PARAMS;
    char *l_pParams[LOAD_METADATA_PARAM_COUNT];
    unsigned long l_pParamLengths[LOAD_METADATA_PARAM_COUNT];

    struct Field *l_pRow;


    l_pParams[0] = (char *)&metabookId;
    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                LOAD_METADATA,
                LOAD_METADATA_PARAM_COUNT, 
                l_eParamTypes, 
                LOAD_METADATA_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((list = NTLVList_Create()) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((rows = Database_Stmt_NumRows(l_pStmt)) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        *r_list = list;
        return R_SUCCESS;
    }
    for (uint32_t i =0; i < rows; i++)
    {
        if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
        {
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
        uuid_parse(l_pRow[0].pValue, type);
        uuid_parse(l_pRow[1].pValue, name);
        NTLVList_Add(list, name, type, l_pRow[2].iLength, (uint8_t *)l_pRow[2].pValue);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
    }
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_list=list;
    return R_SUCCESS;
}

Lookup_Result
Datastore_Get_Block (struct DatabaseConnection * p_pDbCon, struct BlockId *blockId,
                                           struct Block **r_block)
{
    struct Block *block;
    uint64_t metabookId = 0;
    if ((block = calloc(1,sizeof(struct Block))) == NULL)
        return R_ERROR;

    if ((block->pId = BlockId_Clone(blockId)) == NULL)
    {
        Block_Destroy(block);
        return R_ERROR;
    }

    if (Datastore_Get_Block_MetabookId(p_pDbCon, blockId, &metabookId) != R_FOUND)
    {
        Block_Destroy(block);
        return R_ERROR;
    }

    if (metabookId == 0) // No metadata
    {
        if ((block->pMetaDataList = NTLVList_Create()) == NULL)
        {
            Block_Destroy(block);
            return R_ERROR;
        }
    }
    else
    {
        if (Datastore_Get_Metadata(p_pDbCon, metabookId, &block->pMetaDataList) != R_SUCCESS)
        {
            Block_Destroy(block);
            return R_ERROR;
        }
    }
    *r_block = block;
    return R_FOUND;
}

#define LOAD_EVENT "SELECT Event.Metabook_ID, Hash, Data_Type_UUID, Size, Event2.Nugget_UUID, Event2.Time_Secs, Event2.Time_NanoSecs FROM Event LEFT JOIN Block USING (Block_ID) LEFT JOIN LK_Event_Event ON Event.Event_ID=Child_ID LEFT JOIN Event AS Event2 ON (Event2.Event_ID = Parent_ID) WHERE Event.Nugget_UUID=? AND Event.Time_Secs=? AND Event.Time_NanoSecs=?"
#define LOAD_EVENT_PARAM_COUNT 3
#define LOAD_EVENT_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }
#define LOAD_EVENT_FIELD_COUNT 7
#define LOAD_EVENT_FIELDS { DB_TYPE_LONGLONG, DB_TYPE_BLOB, DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_STRING, DB_TYPE_LONGLONG, DB_TYPE_LONGLONG }

Lookup_Result
Datastore_Get_Event (struct DatabaseConnection * p_pDbCon, struct EventId *eventId,
                                           struct Event **r_event)
{
    ASSERT ( p_pDbCon != NULL );
    struct Event *event = NULL;
    uint64_t metabookId;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = LOAD_EVENT_FIELDS;

    DB_TYPE_t l_eParamTypes[] = LOAD_EVENT_PARAMS;
    char *l_pParams[LOAD_EVENT_PARAM_COUNT];
    unsigned long l_pParamLengths[LOAD_EVENT_PARAM_COUNT];

    struct Field *l_pRow;

    char uuid[UUID_STRING_LENGTH];

    uuid_unparse_lower(eventId->uuidNuggetId, uuid);

    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;
    
    l_pParams[1] = (char *)&eventId->iSeconds;
    l_pParams[2] = (char *)&eventId->iNanoSecs;
    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                LOAD_EVENT,
                LOAD_EVENT_PARAM_COUNT, 
                l_eParamTypes, 
                LOAD_EVENT_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((event = calloc(1,sizeof(struct Event))) == NULL)
    {
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((event->pId = EventId_Clone(eventId)) == NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if ((event->pBlock = calloc(1,sizeof(struct Block)))== NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((event->pBlock->pId = BlockId_Create_Shallow()) == NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((event->pBlock->pId->pHash = Hash_Create()) == NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    memcpy(&metabookId, l_pRow[0].pValue, sizeof(uint64_t));

    // XXX: Ugly hack to load a hash.
    free(event->pBlock->pId->pHash->pData);
    event->pBlock->pId->pHash->pData = (uint8_t *)l_pRow[1].pValue;
    event->pBlock->pId->pHash->iSize = l_pRow[1].iLength;
    event->pBlock->pId->pHash->iFlags = HASH_FLAG_FINAL;
    l_pRow[1].pValue = NULL;

    uuid_parse(l_pRow[2].pValue, event->pBlock->pId->uuidDataType);

    memcpy(&event->pBlock->pId->iLength, l_pRow[3].pValue, sizeof(uint64_t));
    if (metabookId == 0) // No metadata
    {
        if ((event->pMetaDataList = NTLVList_Create()) == NULL)
        {
            Event_Destroy(event);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
    }
    else
    {
        if (Datastore_Get_Metadata(p_pDbCon, metabookId, &event->pMetaDataList) != R_SUCCESS)
        {
            Event_Destroy(event);
            Database_Stmt_FreeRow(l_pStmt, l_pRow);
            Database_Stmt_Destroy(l_pStmt);
            pthread_mutex_unlock(&p_pDbCon->mutex);
            return R_ERROR;
        }
    }

    if (l_pRow[4].pValue == NULL)
    {
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        *r_event = event;
        return R_FOUND;
    }
    if ((event->pParentId = calloc(1,sizeof(struct EventId))) == NULL)
    {
        Event_Destroy(event);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    uuid_parse(l_pRow[4].pValue, event->pParentId->uuidNuggetId);
    memcpy(&event->pParentId->iSeconds, l_pRow[5].pValue, sizeof(uint64_t));
    memcpy(&event->pParentId->iNanoSecs, l_pRow[6].pValue, sizeof(uint64_t));
    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *r_event = event;
    return R_FOUND;
}

Lookup_Result
Datastore_Get_Event_Chain (struct DatabaseConnection * p_pDbCon, struct EventId *eventId,
                                           struct Event **r_event, struct Block **r_block)
{
    struct Event *event= NULL, *currentEvent=NULL;
    struct Block *block= NULL, *currentBlock=NULL;

    if (Datastore_Get_Event(p_pDbCon, eventId, &event) != R_FOUND)
        return R_ERROR;
    if (Datastore_Get_Block(p_pDbCon, event->pBlock->pId, &block) != R_FOUND)
        return R_ERROR;

    currentEvent = event;
    currentBlock = block;
    while (currentEvent->pParentId != NULL)
    {
        if (Datastore_Get_Event(p_pDbCon, currentEvent->pParentId, &currentEvent->pParent) != R_FOUND)
            return false;
        // Destroy the parent ID
        EventId_Destroy(currentEvent->pParentId);
        currentEvent->pParentId = NULL;
        if (Datastore_Get_Block(p_pDbCon, currentEvent->pBlock->pId, &currentBlock->pParentBlock) != R_FOUND)
            return false;

        currentEvent = currentEvent->pParent;
        currentBlock = currentBlock->pParentBlock;
    }
    *r_event = event;
    *r_block = block;
    return R_SUCCESS;
}

#define COUNT_BLOCK_PARENTS "SELECT count(*) FROM LK_Block_Block LEFT JOIN Block ON (Block_ID=Child_ID)  WHERE Block.Hash=? and Block.Size=?"
#define COUNT_BLOCK_PARENTS_PARAM_COUNT 2
#define COUNT_BLOCK_PARENTS_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define COUNT_BLOCK_PARENTS_FIELD_COUNT 1
#define COUNT_BLOCK_PARENTS_FIELDS { DB_TYPE_LONGLONG }


Lookup_Result
Datastore_Count_Block_Parents (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uint64_t *p_rCount)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    struct Statement *l_pStmt = NULL;
    struct Field *l_pRow;

    DB_TYPE_t l_eResTypes[] = COUNT_BLOCK_PARENTS_FIELDS;
    DB_TYPE_t l_eParamTypes[] = COUNT_BLOCK_PARENTS_PARAMS;
    char *l_pParams[COUNT_BLOCK_PARENTS_PARAM_COUNT];
    unsigned long l_pParamLengths[COUNT_BLOCK_PARENTS_PARAM_COUNT];

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                COUNT_BLOCK_PARENTS,
                COUNT_BLOCK_PARENTS_PARAM_COUNT, 
                l_eParamTypes, 
                COUNT_BLOCK_PARENTS_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to get row", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to get rows", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (l_pRow[0].pValue !=NULL)
    {
        memcpy(p_rCount, l_pRow[0].pValue, sizeof(uint64_t));
    }
    else 
    {
        *p_rCount = 0;
    }
    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

#define COUNT_BLOCK_EVENTS "SELECT count(*) FROM Event LEFT JOIN Block USING (Block_ID)  WHERE Block.Hash=? and Block.Size=?"
#define COUNT_BLOCK_EVENTS_PARAM_COUNT 2
#define COUNT_BLOCK_EVENTS_PARAMS { DB_TYPE_BLOB, DB_TYPE_LONGLONG }
#define COUNT_BLOCK_EVENTS_FIELD_COUNT 1
#define COUNT_BLOCK_EVENTS_FIELDS { DB_TYPE_LONGLONG }


Lookup_Result
Datastore_Count_Block_Events (struct DatabaseConnection * p_pDbCon,
                                   struct BlockId *p_pBlockId, uint64_t *p_rCount)
{
    ASSERT(p_pDbCon != NULL);
    ASSERT(p_pBlockId != NULL);
    struct Statement *l_pStmt = NULL;
    struct Field *l_pRow;

    DB_TYPE_t l_eResTypes[] = COUNT_BLOCK_EVENTS_FIELDS;
    DB_TYPE_t l_eParamTypes[] = COUNT_BLOCK_EVENTS_PARAMS;
    char *l_pParams[COUNT_BLOCK_EVENTS_PARAM_COUNT];
    unsigned long l_pParamLengths[COUNT_BLOCK_EVENTS_PARAM_COUNT];

    l_pParams[0] = (char *)p_pBlockId->pHash->pData;
    l_pParamLengths[0] = p_pBlockId->pHash->iSize;

    l_pParams[1] = (char *)&p_pBlockId->iLength;

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                COUNT_BLOCK_EVENTS,
                COUNT_BLOCK_EVENTS_PARAM_COUNT, 
                l_eParamTypes, 
                COUNT_BLOCK_EVENTS_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare statement", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute statement", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) == 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to get row", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to get rows", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (l_pRow[0].pValue !=NULL)
    {
        memcpy(p_rCount, l_pRow[0].pValue, sizeof(uint64_t));
    }
    else 
    {
        *p_rCount = 0;
    }
    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);

    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}


