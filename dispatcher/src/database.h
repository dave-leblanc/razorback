/** @file database.h
 * Database structures and functions
 */

#ifndef RAZORBACK_DATABASE_H
#define RAZORBACK_DATABASE_H
#include <razorback/ntlv.h>
#include <mysql.h>
#include <pthread.h>



#define DB_TYPE_t enum enum_field_types
#define DB_TYPE_TINY        MYSQL_TYPE_TINY
#define DB_TYPE_SHORT       MYSQL_TYPE_SHORT
#define DB_TYPE_INT         MYSQL_TYPE_LONG
#define DB_TYPE_LONG        MYSQL_TYPE_LONG
#define DB_TYPE_LONGLONG    MYSQL_TYPE_LONGLONG
#define DB_TYPE_FLOAT       MYSQL_TYPE_FLOAT
#define DB_TYPE_DOUBLE      MYSQL_TYPE_DOUBLE
#define DB_TYPE_TIME        MYSQL_TYPE_TIME
#define DB_TYPE_DATE        MYSQL_TYPE_DATE
#define DB_TYPE_DATATIME    MYSQL_TYPE_DATETIME
#define DB_TYPE_STRING      MYSQL_TYPE_STRING
#define DB_TYPE_BLOB        MYSQL_TYPE_BLOB


/** Database
 * Holds connection information to the database
 * This structure should be treated as a black box.
 */
struct DatabaseConnection
{
    MYSQL * pHandle;        ///< the database connection
    pthread_mutex_t mutex;  ///< Connection Mutex
};

/** Prepared statement.
 * This structure should be treated as a black box.
 */
struct Statement
{
    MYSQL_STMT *pStmt;              ///< The Mysql Statement
    MYSQL_BIND *pParams;            ///< The Statement Parameters BIND's
    MYSQL_BIND *pResults;           ///< The Results BIND's
    MYSQL_RES *pResult;             ///< The result
    MYSQL_FIELD *pFields;           ///< The field metedata.
    unsigned long iParamCount;      ///< The Parameter count
    unsigned long iFieldCount;      ///< The result count
    my_bool bUpdate;                ///< Update flag
    my_ulonglong iRowCount;          ///< Number of rows
};

/** A field value
 */
struct Field
{
    char *pValue;
    unsigned long iLength;
};

/** Intialiaze the database access library
 */
extern bool Database_Initialize(void);

/** Shutdown the database access library
 */
extern void Database_End(void);

/** Intialiaze per thread memory structures.
 */
extern bool Database_Thread_Initialize(void);
/** Free per thread memory structures.
 */
extern void Database_Thread_End(void);

/** Initializes a database connection
 * @param p_sAddress the ip address
 * @param p_iPort the port number
 * @param p_sUserName the user name
 * @param p_sPassword the password
 * @param p_sDatabaseName the name of the database
 * @return a DatabaseConnection struct on success, NULL on error.
 */
extern struct DatabaseConnection* Database_Connect (
                                 const char * p_sAddress, uint16_t p_iPort,
                                 const char * p_sUserName,
                                 const char * p_sPassword,
                                 const char * p_sDatabaseName);

/** Terminates a database connection
 * @param p_pDB the database
 */
extern void Database_Disconnect (struct DatabaseConnection *p_pDB);


extern struct Statement *
Database_Stmt_Prepare(struct DatabaseConnection *p_pDbCon,
                                const char *p_sQuery, 
                                unsigned long p_iParamCount, DB_TYPE_t *p_pParamTypes,
                                unsigned long p_iResultCount, DB_TYPE_t *p_pResultTypes );

extern void Database_Stmt_Destroy(struct Statement *p_pStmt);
extern bool Database_Stmt_Execute(struct Statement *p_pStmt, char **p_pParams,
                                unsigned long *p_pParamLengths);

unsigned long long Database_Stmt_NumRows(struct Statement *p_pStmt);
extern struct Field * Database_Stmt_FetchRow(struct Statement *p_pStmt);

extern void Database_Stmt_FreeRow(struct Statement *p_pStmt, struct Field *p_pFields);
extern unsigned long long Database_LastInsertId(struct DatabaseConnection *p_pDbCon);


#endif // RAZORBACK_DATABASE_H
