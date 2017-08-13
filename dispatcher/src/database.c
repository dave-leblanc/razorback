/** @file database.h
 * Database structures and functions
 */

#include "config.h"
#include <razorback/debug.h>
#include <razorback/log.h>
#include "database.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


bool
Database_Initialize (void)
{
    if (mysql_library_init(0, NULL, NULL) != 0) {
        rzb_log(LOG_ERR, "%s: Failed to init MySQL library", __func__);
        return false;
    }
    return true;
}

void 
Database_End (void)
{
    mysql_library_end();
}

bool
Database_Thread_Initialize (void)
{
    if (mysql_thread_init() != 0) {
        rzb_log(LOG_ERR, "%s: Failed to init MySQL structures for thread", __func__);
        return false;
    }
    return true;
}

void 
Database_Thread_End (void)
{
    mysql_thread_end();
}

struct DatabaseConnection* 
Database_Connect(const char * p_sAddress,
                     uint16_t p_iPort, const char * p_sUserName,
                     const char * p_sPassword,
                     const char * p_sDatabaseName)
{
    ASSERT (p_sAddress != NULL);
    ASSERT (p_sUserName != NULL);
    ASSERT (p_sPassword != NULL);
    ASSERT (p_sDatabaseName != NULL);

    pthread_mutexattr_t mutexAttr;
    struct DatabaseConnection *l_pConnection;
    my_bool l_bReconnect =1;
    
    if ((l_pConnection = calloc(1, sizeof (struct DatabaseConnection))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate connection", __func__);
        return NULL;
    }

    if ( (l_pConnection->pHandle = mysql_init (NULL)) == NULL )
    {
        rzb_log ( LOG_ERR, "%s: failed due to failure of mysql_init", __func__ );
        free(l_pConnection);
        return NULL;
    }

    mysql_options ( l_pConnection->pHandle, MYSQL_OPT_RECONNECT, &l_bReconnect );

    if ( mysql_real_connect ( l_pConnection->pHandle, p_sAddress, p_sUserName, 
                p_sPassword, p_sDatabaseName, p_iPort, NULL, 0 ) == NULL )
    {
        free(l_pConnection);
        rzb_log ( LOG_ERR, "%s: Failed due to failure of mysql_real_connect: %s", __func__,
                mysql_error (l_pConnection->pHandle ));
        return NULL;
    }
   
    if (mysql_set_character_set(l_pConnection->pHandle, "utf8") != 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to set new client character set: %s", __func__,
                               mysql_character_set_name(l_pConnection->pHandle));
    }
    memset(&mutexAttr, 0, sizeof(pthread_mutexattr_t));
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&l_pConnection->mutex,&mutexAttr);
    return l_pConnection;
}

void
Database_Disconnect (struct DatabaseConnection *p_pConnection)
{
    ASSERT ( p_pConnection != NULL );
    mysql_close ( p_pConnection->pHandle );
    pthread_mutex_destroy(&p_pConnection->mutex);
    free(p_pConnection);
}

static bool
Database_AllocateBindPointers(struct Statement *p_pStmt)
{
    ASSERT (p_pStmt != NULL);

    unsigned long l_iPos = 0;
    unsigned long l_iSize=0;


    for (l_iPos = 0; l_iPos < p_pStmt->iFieldCount; l_iPos++)
    {
        switch  (p_pStmt->pResults[l_iPos].buffer_type)
        {
        case MYSQL_TYPE_TINY:
            l_iSize = 1;
            break;
        case MYSQL_TYPE_SHORT:
            l_iSize = 2;
            break;
        case MYSQL_TYPE_LONG:
            l_iSize = 4;
            break;
        case MYSQL_TYPE_LONGLONG:
            l_iSize = 8;
            break;
        case MYSQL_TYPE_FLOAT:
            l_iSize = 4;
            break;
        case MYSQL_TYPE_DOUBLE:
            l_iSize = 8;
            break;
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
            l_iSize = (unsigned long)sizeof(MYSQL_TIME);
            break;
        case MYSQL_TYPE_STRING:
            // Null Pad the String
            l_iSize = p_pStmt->pFields[l_iPos].max_length+1;
            break;
        case MYSQL_TYPE_BLOB:
            l_iSize = p_pStmt->pFields[l_iPos].max_length;
            break;
        default:
            rzb_log(LOG_ERR, "%s: Uknown datatype, skiping, %lu", __func__, l_iPos);
            continue;
        }
        p_pStmt->pResults[l_iPos].buffer_length = l_iSize;

        if ((p_pStmt->pResults[l_iPos].buffer = calloc(p_pStmt->pResults[l_iPos].buffer_length, sizeof(char))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: failed to allocate pointer for index:%lu, size:%lu", __func__, l_iPos, p_pStmt->pResults[l_iPos].buffer_length);
            return false;
        }
    }
    return true;
}

static bool
Database_AllocBindArray(MYSQL_BIND **p_pBind, DB_TYPE_t *p_pTypes, 
                                    unsigned long p_iCount)
{
    ASSERT (p_pBind !=NULL);
    ASSERT (p_iCount == 0 || p_pTypes != NULL);

    MYSQL_BIND *l_pBinds;
    unsigned long l_iPos = 0;
    if ((l_pBinds = calloc(p_iCount, sizeof (MYSQL_BIND))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate array", __func__);
        return false;
    }
    for (l_iPos = 0; l_iPos < p_iCount; l_iPos++)
    {
        l_pBinds[l_iPos].buffer_type=p_pTypes[l_iPos];
        if ((l_pBinds[l_iPos].length = calloc(1, sizeof(unsigned long))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate length", __func__);
            return false;
        }
        if ((l_pBinds[l_iPos].error = calloc(1, sizeof(my_bool))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate error", __func__);
            return false;
        }
        if ((l_pBinds[l_iPos].is_null = calloc(1, sizeof(my_bool))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate is_null", __func__);
            return false;
        }
        
    }
    *p_pBind = l_pBinds;
    return true;
}

struct Statement *
Database_Stmt_Prepare(struct DatabaseConnection *p_pDbCon,
                                    const char *p_sQuery, 
                                    unsigned long p_iParamCount, DB_TYPE_t *p_pBindParams,
                                    unsigned long p_iResultCount, DB_TYPE_t *p_pResultTypes )
{
    ASSERT ( p_pDbCon != NULL );
    ASSERT (p_sQuery);

    struct Statement *l_pStmt = NULL;

    if ((l_pStmt = calloc(1, sizeof (struct Statement))) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to allocate statement", __func__);
        return NULL;
    }

    l_pStmt->bUpdate = 1;
    l_pStmt->iParamCount = p_iParamCount;
    l_pStmt->iFieldCount = p_iResultCount;

    if ((l_pStmt->pStmt = mysql_stmt_init(p_pDbCon->pHandle)) == NULL)
    {
        rzb_log(LOG_ERR, "%s: Failed to create prepared statement", __func__);
        Database_Stmt_Destroy(l_pStmt);
        return NULL;
    }
    if (mysql_stmt_prepare(l_pStmt->pStmt, p_sQuery, strlen(p_sQuery)) != 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to prepare statement: %s", __func__, mysql_stmt_error(l_pStmt->pStmt));
        Database_Stmt_Destroy(l_pStmt);
        return NULL;
    }
    if (mysql_stmt_attr_set(l_pStmt->pStmt, STMT_ATTR_UPDATE_MAX_LENGTH, &l_pStmt->bUpdate) != 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to set attributes", __func__);
        Database_Stmt_Destroy(l_pStmt);
        return NULL;
    }
    if (mysql_stmt_param_count(l_pStmt->pStmt) != p_iParamCount)
    {
        rzb_log(LOG_ERR, "%s: Param count does not match markers", __func__);
        Database_Stmt_Destroy(l_pStmt);
        return NULL;
    }
    if (p_iParamCount > 0)
    {
        if (!Database_AllocBindArray(&l_pStmt->pParams, p_pBindParams, p_iParamCount))
        {
            rzb_log(LOG_ERR, "%s: Param bind allocation failure", __func__);
            Database_Stmt_Destroy(l_pStmt);
            return NULL;
        }
    }
    if (p_iResultCount > 0)
    {
        if (!Database_AllocBindArray(&l_pStmt->pResults, p_pResultTypes, p_iResultCount))
        {
            rzb_log(LOG_ERR, "%s: Result bind allocation failure", __func__);
            Database_Stmt_Destroy(l_pStmt);
            return NULL;
        }
    }
    return l_pStmt;
}


bool
Database_Stmt_Execute(struct Statement *p_pStmt, char **p_pParams, unsigned long *p_pParamLengths)
{
    ASSERT (p_pStmt != NULL);
    unsigned long l_iPos = 0;
    if (p_pStmt->iParamCount > 0)
    {
        for (l_iPos = 0; l_iPos < p_pStmt->iParamCount; l_iPos++)
        {
            p_pStmt->pParams[l_iPos].buffer=p_pParams[l_iPos];
            p_pStmt->pParams[l_iPos].buffer_length=p_pParamLengths[l_iPos];
            *p_pStmt->pParams[l_iPos].length=p_pParamLengths[l_iPos];

            if (p_pParams[l_iPos] == NULL)
                *p_pStmt->pParams[l_iPos].is_null =1;

        }
        if (mysql_stmt_bind_param(p_pStmt->pStmt, p_pStmt->pParams) != 0)
        {
            rzb_log(LOG_ERR, "%s: Failed to bind paramaters: %s", __func__, mysql_stmt_error(p_pStmt->pStmt));
            return false;
        }
    }

    if (mysql_stmt_execute(p_pStmt->pStmt) != 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to execute: %s", __func__,  mysql_stmt_error(p_pStmt->pStmt));
        return false;
    }
    if (mysql_stmt_store_result(p_pStmt->pStmt) != 0)
    {
        rzb_log(LOG_ERR, "%s: Failed to store result: %s", __func__, mysql_stmt_error(p_pStmt->pStmt));
        return false;
    }

    if (mysql_stmt_field_count(p_pStmt->pStmt) != p_pStmt->iFieldCount)
    {
        rzb_log(LOG_ERR, "%s: Col count does not match", __func__);
        return false;
    }
    
    if (p_pStmt->iFieldCount >0)
    {
        if ((p_pStmt->pResult = mysql_stmt_result_metadata(p_pStmt->pStmt)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to get result metadata: %s", __func__, mysql_stmt_error(p_pStmt->pStmt));
            return false;
        }
       
        if ((p_pStmt->pFields = mysql_fetch_fields(p_pStmt->pResult)) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to get field data: %s", __func__, mysql_stmt_error(p_pStmt->pStmt));

            return false;
        }
        p_pStmt->iRowCount = mysql_stmt_num_rows(p_pStmt->pStmt);
    }
    return true;
}

struct Field *
Database_Stmt_FetchRow(struct Statement *p_pStmt)
{
    ASSERT (p_pStmt != NULL);
    struct Field *l_pRet = NULL;
    int l_iRet = 0;
    unsigned long l_iPos = 0;

    if (p_pStmt->iFieldCount > 0)
    {
        if (!Database_AllocateBindPointers(p_pStmt))
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate bind data buffers", __func__);
            return NULL;
        }
        if (mysql_stmt_bind_result(p_pStmt->pStmt, p_pStmt->pResults) != 0)
        {
            rzb_log(LOG_ERR, "%s: Failed to bind results: %s", __func__, mysql_stmt_error(p_pStmt->pStmt));
            return NULL;
        }
    }
    

    l_iRet = mysql_stmt_fetch(p_pStmt->pStmt);
    if (l_iRet == 1)
    {
        rzb_log(LOG_ERR, "%s: Failed to fetch result: %s", __func__, mysql_stmt_errno(p_pStmt->pStmt));
        errno = EFAULT;
        return NULL;
    }
    if (l_iRet == MYSQL_NO_DATA)
    {
        rzb_log(LOG_ERR, "%s: Failed to fetch result, no data", __func__);
        errno = EINVAL;
        return NULL;
    }
    if (l_iRet == MYSQL_DATA_TRUNCATED)
    {
        rzb_log(LOG_ERR, "%s: Fetch result, data truncated", __func__);
    }

    if (p_pStmt->iFieldCount > 0)
    {
        if ((l_pRet = calloc(p_pStmt->iFieldCount, sizeof(struct Field))) == NULL)
        {
            rzb_log(LOG_ERR, "%s: Failed to allocate fields", __func__);
            errno = EFAULT;
            return NULL;
        }
        for (l_iPos = 0; l_iPos < p_pStmt->iFieldCount; l_iPos++)
        {
            if (*p_pStmt->pResults[l_iPos].is_null == 1)
            {
                l_pRet[l_iPos].pValue = NULL;
                l_pRet[l_iPos].iLength = 0;
            }
            else
            {
                l_pRet[l_iPos].pValue = p_pStmt->pResults[l_iPos].buffer;
                l_pRet[l_iPos].iLength = *p_pStmt->pResults[l_iPos].length;
            }
            p_pStmt->pResults[l_iPos].buffer=NULL;
        }
    }
    return l_pRet;
}

unsigned long long
Database_Stmt_NumRows(struct Statement *p_pStmt)
{
    return (unsigned long long)p_pStmt->iRowCount;
}

void 
Database_Stmt_FreeRow(struct Statement *p_pStmt, struct Field *p_pFields)
{
    unsigned long l_iPos;
    for (l_iPos = 0; l_iPos < p_pStmt->iFieldCount; l_iPos++)
    {
        if (p_pFields[l_iPos].pValue != NULL)
            free(p_pFields[l_iPos].pValue);
    }
    free(p_pFields);
}

unsigned long long
Database_LastInsertId(struct DatabaseConnection *p_pDbCon)
{
    return mysql_insert_id(p_pDbCon->pHandle);
}

void
Database_Stmt_Destroy(struct Statement *p_pStmt)
{
    ASSERT (p_pStmt != NULL);
    unsigned long l_iPos =0;

    if (p_pStmt->pStmt != NULL) 
    {
        mysql_stmt_close(p_pStmt->pStmt);
    }

    if (p_pStmt->pResult != NULL)
        mysql_free_result(p_pStmt->pResult);

        
    if (p_pStmt->pParams != NULL)
    {
        for (l_iPos = 0; l_iPos < p_pStmt->iParamCount; l_iPos++)
        {
            if (p_pStmt->pParams[l_iPos].length != NULL)
                free(p_pStmt->pParams[l_iPos].length);

            if (p_pStmt->pParams[l_iPos].error != NULL)
                free(p_pStmt->pParams[l_iPos].error);

            if (p_pStmt->pParams[l_iPos].is_null != NULL)
                free(p_pStmt->pParams[l_iPos].is_null);

        }
        free(p_pStmt->pParams);
    }

    if (p_pStmt->pResults != NULL)
    {
        for (l_iPos = 0; l_iPos < p_pStmt->iFieldCount; l_iPos++)
        {
            if (p_pStmt->pResults[l_iPos].buffer != NULL)
                free(p_pStmt->pResults[l_iPos].buffer);

            if (p_pStmt->pResults[l_iPos].length != NULL)
                free(p_pStmt->pResults[l_iPos].length);

            if (p_pStmt->pResults[l_iPos].error != NULL)
                free(p_pStmt->pResults[l_iPos].error);

            if (p_pStmt->pResults[l_iPos].is_null != NULL)
                free(p_pStmt->pResults[l_iPos].is_null);

        }
        free(p_pStmt->pResults);
    }
    free(p_pStmt);
}


