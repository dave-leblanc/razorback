#include "config.h"
#include <razorback/debug.h>
#include <razorback/log.h>
#include <razorback/uuids.h>
#include <razorback/block_id.h>
#include <razorback/hash.h>
#include "datastore.h"
#include "global_cache.h"
#include <errno.h>
#include <string.h>

#define UUID_TABLE_DATA_TYPE    "UUID_Data_Type"
#define UUID_TABLE_INTEL_TYPE   NULL
#define UUID_TABLE_NTLV_TYPE    "UUID_Metadata_Type"
#define UUID_TABLE_NTLV_NAME    "UUID_Metadata_Name"
#define UUID_TABLE_APP_TYPE     "App_Type"
#define UUID_TABLE_NUGGET_TYPE  "Nugget_Type"


static const char *
Get_UUID_Table(int type)
{
    switch (type)
    {
    case UUID_TYPE_DATA_TYPE:
        return UUID_TABLE_DATA_TYPE;
    case UUID_TYPE_INTEL_TYPE:
        return UUID_TABLE_INTEL_TYPE;
    case UUID_TYPE_NTLV_TYPE:
        return UUID_TABLE_NTLV_TYPE;
    case UUID_TYPE_NTLV_NAME:
        return UUID_TABLE_NTLV_NAME;
    case UUID_TYPE_NUGGET:
        return UUID_TABLE_APP_TYPE;
    case UUID_TYPE_NUGGET_TYPE:
        return UUID_TABLE_NUGGET_TYPE;
    default:
        return NULL;
    }
}
#define SELECT_UUID_BY_NAME "SELECT UUID FROM %s WHERE Name=?"
#define SELECT_UUID_BY_NAME_PARAM_COUNT 1
#define SELECT_UUID_BY_NAME_PARAMS { DB_TYPE_STRING }
#define SELECT_UUID_BY_NAME_FIELD_COUNT 1
#define SELECT_UUID_BY_NAME_FIELDS { DB_TYPE_STRING }

Lookup_Result
Datastore_Get_UUID_By_Name (struct DatabaseConnection * p_pDbCon,
                                   const char *name, int type, uuid_t uuid)
{
    ASSERT ( p_pDbCon != NULL );
    const char *table;
    char *query;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = SELECT_UUID_BY_NAME_FIELDS;

    DB_TYPE_t l_eParamTypes[] = SELECT_UUID_BY_NAME_PARAMS;
    char *l_pParams[SELECT_UUID_BY_NAME_PARAM_COUNT];
    unsigned long l_pParamLengths[SELECT_UUID_BY_NAME_PARAM_COUNT];

    struct Field *l_pRow;
    
    if ((table = Get_UUID_Table(type)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Invalid type", __func__);
        return R_NOT_FOUND;
    }
    if (asprintf(&query, SELECT_UUID_BY_NAME, table) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate query", __func__);
        return R_ERROR;
    }
    l_pParams[0] = (char *)name;
    l_pParamLengths[0] = strlen(name);


    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                query,
                SELECT_UUID_BY_NAME_PARAM_COUNT, 
                l_eParamTypes, 
                SELECT_UUID_BY_NAME_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) != 1)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to fetch row", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        return R_ERROR;
    }
    
    if (l_pRow[0].pValue == NULL)
        uuid_clear(uuid);
    else
        uuid_parse(l_pRow[0].pValue, uuid);

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    free(query);
    return R_FOUND;
}


#define SELECT_UUID_DESC_BY_UUID "SELECT Description FROM %s WHERE UUID=?"
#define SELECT_UUID_DESC_BY_UUID_PARAM_COUNT 1
#define SELECT_UUID_DESC_BY_UUID_PARAMS { DB_TYPE_BLOB }
#define SELECT_UUID_DESC_BY_UUID_FIELD_COUNT 1
#define SELECT_UUID_DESC_BY_UUID_FIELDS { DB_TYPE_STRING }

Lookup_Result
Datastore_Get_UUID_Description_By_UUID (struct DatabaseConnection *p_pDbCon, uuid_t p_uuid, int type, char **ret)
{
    ASSERT ( p_pDbCon != NULL );
    const char *table;
    char *query;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = SELECT_UUID_DESC_BY_UUID_FIELDS;

    DB_TYPE_t l_eParamTypes[] = SELECT_UUID_DESC_BY_UUID_PARAMS;
    char *l_pParams[SELECT_UUID_DESC_BY_UUID_PARAM_COUNT];
    unsigned long l_pParamLengths[SELECT_UUID_DESC_BY_UUID_PARAM_COUNT];

    struct Field *l_pRow;
    char uuid[UUID_STRING_LENGTH];
    
    if ((table = Get_UUID_Table(type)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Invalid type", __func__);
        *ret = NULL;
        return R_NOT_FOUND;
    }
    if (asprintf(&query, SELECT_UUID_DESC_BY_UUID, table) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate query", __func__);
        *ret = NULL;
        return R_ERROR;
    }
    uuid_unparse_lower(p_uuid, uuid);
    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;


    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                query,
                SELECT_UUID_DESC_BY_UUID_PARAM_COUNT, 
                l_eParamTypes, 
                SELECT_UUID_DESC_BY_UUID_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) != 1)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to fetch row", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_ERROR;
    }
    
    if (l_pRow[0].pValue == NULL)
        *ret = NULL;
    else
    {
        *ret = l_pRow[0].pValue;
        l_pRow[0].pValue = NULL;
    }

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    free(query);
    return R_FOUND;
}

#define SELECT_UUID_NAME_BY_UUID "SELECT Name FROM %s WHERE UUID=?"
#define SELECT_UUID_NAME_BY_UUID_PARAM_COUNT 1
#define SELECT_UUID_NAME_BY_UUID_PARAMS { DB_TYPE_BLOB }
#define SELECT_UUID_NAME_BY_UUID_FIELD_COUNT 1
#define SELECT_UUID_NAME_BY_UUID_FIELDS { DB_TYPE_STRING }

Lookup_Result
Datastore_Get_UUID_Name_By_UUID (struct DatabaseConnection *p_pDbCon, uuid_t p_uuid, int type, char **ret)
{
    ASSERT ( p_pDbCon != NULL );
    const char *table;
    char *query;
    struct Statement *l_pStmt = NULL;
    DB_TYPE_t l_eResTypes[] = SELECT_UUID_NAME_BY_UUID_FIELDS;

    DB_TYPE_t l_eParamTypes[] = SELECT_UUID_NAME_BY_UUID_PARAMS;
    char *l_pParams[SELECT_UUID_NAME_BY_UUID_PARAM_COUNT];
    unsigned long l_pParamLengths[SELECT_UUID_NAME_BY_UUID_PARAM_COUNT];

    struct Field *l_pRow;
    char uuid[UUID_STRING_LENGTH];
    
    if ((table = Get_UUID_Table(type)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Invalid type", __func__);
        *ret = NULL;
        return R_NOT_FOUND;
    }
    if (asprintf(&query, SELECT_UUID_NAME_BY_UUID, table) == -1)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate query", __func__);
        *ret = NULL;
        return R_ERROR;
    }
    uuid_unparse_lower(p_uuid, uuid);
    l_pParams[0] = uuid;
    l_pParamLengths[0] = 36;


    pthread_mutex_lock(&p_pDbCon->mutex);


    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                query,
                SELECT_UUID_NAME_BY_UUID_PARAM_COUNT, 
                l_eParamTypes, 
                SELECT_UUID_NAME_BY_UUID_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare", __func__);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, l_pParams, l_pParamLengths))
    {
        rzb_log(LOG_ERR, "%s: Failed to execute", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_ERROR;
    }
    if (Database_Stmt_NumRows(l_pStmt) != 1)
    {
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_NOT_FOUND;
    }

    if ((l_pRow = Database_Stmt_FetchRow(l_pStmt)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to fetch row", __func__);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        free(query);
        *ret = NULL;
        return R_ERROR;
    }
    
    if (l_pRow[0].pValue == NULL)
        *ret = NULL;
    else
    {
        *ret = l_pRow[0].pValue;
        l_pRow[0].pValue = NULL;
    }

    Database_Stmt_FreeRow(l_pStmt, l_pRow);
    Database_Stmt_Destroy(l_pStmt);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    free(query);
    return R_FOUND;
}

// This is a filthy hack,  due to the utf8ness of the DB connection we have to convert the UUID to hex 
// encoding on the DB server before we pull it back, then convert it back to binary in the application.
#define GET_UUIDS "select uuid AS UUID, Name, Description from %s"
#define GET_UUIDS_PARAM_COUNT 0
#define GET_UUIDS_FIELD_COUNT 3
#define GET_UUIDS_FIELDS { DB_TYPE_STRING, DB_TYPE_STRING, DB_TYPE_STRING }

Lookup_Result
Datastore_Get_UUIDs (struct DatabaseConnection * p_pDbCon,
                        uint32_t type, struct List **rList)
{
    ASSERT ( p_pDbCon != NULL );
    struct Statement *l_pStmt = NULL;
    const char *table;
    char *query;
    struct List *list;
    size_t num= 0;
    size_t pos= 0;
    uuid_t uuid;

    DB_TYPE_t l_eResTypes[] = GET_UUIDS_FIELDS;

    struct Field *l_pRow;

    if ((table = Get_UUID_Table(type)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Invalid type", __func__);
        return R_ERROR;
    }
    if (asprintf(&query, GET_UUIDS, table) == -1)
    {
        return R_ERROR;
    }
    pthread_mutex_lock(&p_pDbCon->mutex);

    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                query,
                GET_UUIDS_PARAM_COUNT, 
                NULL,
                GET_UUIDS_FIELD_COUNT,
                l_eResTypes)) == NULL)
    {
        free(query);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    if (!Database_Stmt_Execute(l_pStmt, NULL, NULL))
    {
        free(query);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    num = Database_Stmt_NumRows(l_pStmt);

    if ((list = UUID_Create_List()) == NULL)
    {
        free(query);
        Database_Stmt_Destroy(l_pStmt);
        pthread_mutex_unlock(&p_pDbCon->mutex);
        return R_ERROR;
    }
    while ((pos < num) && (l_pRow = Database_Stmt_FetchRow(l_pStmt)) != NULL)
    {
        // XXX: Filthy hack
        uuid_parse(l_pRow[0].pValue, uuid);
        UUID_Add_List_Entry(list, uuid, l_pRow[1].pValue, l_pRow[2].pValue);
        Database_Stmt_FreeRow(l_pStmt, l_pRow);
        pos++;
    }
    

    Database_Stmt_Destroy(l_pStmt);
    free(query);
    pthread_mutex_unlock(&p_pDbCon->mutex);
    *rList = list;
    return R_FOUND;
}

#define INSERT_UUID "INSERT INTO %s (`Name`, `UUID`, `Description`) VALUES (?,?,?)"
#define INSERT_UUID_PARAM_COUNT 3
#define INSERT_UUID_PARAMS { DB_TYPE_STRING, DB_TYPE_BLOB, DB_TYPE_STRING }
#define INSERT_UUID_FIELD_COUNT 0

Lookup_Result
Datastore_Insert_UUID (struct DatabaseConnection * p_pDbCon,
                                    char *name, uuid_t uuidId,
                                    char *description, int type)
{
    ASSERT(p_pDbCon != NULL);
    struct Statement *l_pStmt = NULL;
    const char *table = NULL;
    char *query = NULL;
    DB_TYPE_t l_eParamTypes[] = INSERT_UUID_PARAMS;
    char *l_pParams[INSERT_UUID_PARAM_COUNT];
    unsigned long l_pParamLengths[INSERT_UUID_PARAM_COUNT];
    char uuid[UUID_STRING_LENGTH];

    l_pParams[0] = name;
    l_pParamLengths[0] = strlen(name);
    uuid_unparse_lower(uuidId, uuid);
    l_pParams[1] = uuid;
    l_pParamLengths[1] = 36;

    l_pParams[2] = description;
    l_pParamLengths[2] = strlen(description);
    if ((table = Get_UUID_Table(type)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Invalid type", __func__);
        return R_ERROR;
    }
    if (asprintf(&query, INSERT_UUID, table) == -1)
    {
        return R_ERROR;
    }

    pthread_mutex_lock(&p_pDbCon->mutex);
    if ((l_pStmt = Database_Stmt_Prepare(p_pDbCon, 
                query,
                INSERT_UUID_PARAM_COUNT, 
                l_eParamTypes, 
                INSERT_UUID_FIELD_COUNT,
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
        free(query);
        return R_ERROR;
    }
    free(query);
    Database_Stmt_Destroy(l_pStmt);
    
    pthread_mutex_unlock(&p_pDbCon->mutex);
    return R_SUCCESS;
}

Lookup_Result
Datastore_Load_UUID_List(struct DatabaseConnection *dbCon,int type)
{
    struct List *list, *list2;
    if (Datastore_Get_UUIDs(dbCon, type, &list) == R_ERROR)
    {
        return R_ERROR;
    }
    list2 = UUID_Get_List(type);
    List_Lock(list2);
    List_Clear(list2);
    list2->head = list->head;
    list2->tail = list->tail;
    list2->length = list->length;
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    List_Destroy(list);
    List_Unlock(list2);
    return R_SUCCESS;
}
