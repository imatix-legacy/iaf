/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflodbc.sqc
    Title:      odbc Database interface
    Package:    SFL

    Written:    99/04/19  Pascal Antonnaux <pascal@imatix.com>
    Revised:    99/04/20

    Synopsis:   Defines structures and constants for the odbc interface.

    Copyright:  Copyright (c) 1991-99 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/
#include "prelude.h"
#include "sflsymb.h"
#include "sflcons.h"
#include "sflmem.h"
#ifndef DBIO_ODBC
#    define DBIO_ODBC
#endif
#include "sflodbc.h"

/*- Global definition -------------------------------------------------------*/

typedef struct {
    COMMON_DBIO_CTX
    char dbname   [50];
    char user     [50];
    char password [50];
    HENV environment;                   /*  ODBC environment                 */
    HDBC connection;                    /*  ODBC connection handle           */
  
} ODBC_CONNECT_CTX;

/*- Glabal variable ---------------------------------------------------------*/
static DBIO_ERR
    error;                              /* Global error stucture             */

/*- Local functions declaration ---------------------------------------------*/
static char *set_odbc_error_value (HENV env, HDBC connect);

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_odbc_connect

    Synopsis: Connect to a odbc database.
    ---------------------------------------------------------------------[>]-*/

void *
dbio_odbc_connect
(char *name, char *user, char *pwd)
{
    ODBC_CONNECT_CTX
        *handle = NULL;
    HENV
        environment;                    /*  ODBC environment                 */
    HDBC
        connection;                     /*  ODBC connection handle           */
    RETCODE
        retcode;

    ASSERT (name);

    retcode = SQLAllocEnv (&environment); /* Environment handle              */
    if (retcode != SQL_SUCCESS)
      {
        coprintf ("DBIO ODBC: dbio_odbc_connect (SQLAllocEnv) of %s (%ld)",
                  name, retcode);
        return (NULL);
      }
    retcode = SQLAllocConnect (environment, &connection);
    if (retcode == SQL_SUCCESS
    ||  retcode == SQL_SUCCESS_WITH_INFO)
      {
        /* Set login timeout to 20 seconds.                                  */
        SQLSetConnectOption (connection, SQL_LOGIN_TIMEOUT, 20);
        /* Connect to data source                                            */
        retcode = SQLConnect(connection, name, SQL_NTS, user, SQL_NTS,
                             pwd, SQL_NTS);
        if (retcode == SQL_SUCCESS
        ||  retcode == SQL_SUCCESS_WITH_INFO)
          {
            handle = mem_alloc (sizeof (ODBC_CONNECT_CTX));
            if (handle)
              {
                memset (handle, 0, sizeof (ODBC_CONNECT_CTX));
                handle-> environment = environment;
                handle-> connection  = connection;
                strcpy (handle-> dbname, name);
                if (user && strused (user))
                    strcpy (handle-> user, user);
                if (pwd && strused (pwd))
                    strcpy (handle-> password, pwd);
              }
            else
                coprintf ("DBIO ODBC: dbio_odbc_connect %s Memory allocation error",
                          name);
          }
        else
          {
            coprintf ("DBIO ODBC: dbio_odbc_connect (SQLConnect) %s (%s)",
                      name, set_odbc_error_value (environment, connection));
            SQLFreeConnect (connection);
            SQLFreeEnv     (environment);
          }
      }
    else
      {
        coprintf ("DBIO ODBC: dbio_odbc_connect (SQLAllocConnect) %s (%s)",
                  name, set_odbc_error_value (environment, SQL_NULL_HDBC));
        SQLFreeEnv (environment);
      }

    return ((void *)handle);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_odbc_disconnect

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
dbio_odbc_disconnect (void *context)
{
    ODBC_CONNECT_CTX
        *ctx;

    if (context)
      {
        ctx = (ODBC_CONNECT_CTX *)context;

        SQLDisconnect  (ctx-> connection);
        SQLFreeConnect (ctx-> connection);
        SQLFreeEnv     (ctx-> environment);

        mem_free (ctx);
      }

}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_odbc_commit

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

int
dbio_odbc_commit (void *context)
{
    ODBC_CONNECT_CTX
        *ctx;
    RETCODE
        retcode;
    int
        feedback = TRUE;

    if (context)
      {
        ctx = (ODBC_CONNECT_CTX *)context;
        retcode = SQLTransact (ctx-> environment, ctx-> connection, SQL_COMMIT);
        if (retcode != SQL_SUCCESS
        &&  retcode != SQL_SUCCESS_WITH_INFO)
          {
            coprintf ("DBIO ODBC: Error dbio_odbc_commit (%s)",
                      set_odbc_error_value (ctx-> environment,
                                            ctx-> connection));
            feedback = FALSE;
          }
      }

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_odbc_rollback

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

int
dbio_odbc_rollback (void *context)
{
    ODBC_CONNECT_CTX
        *ctx;
    RETCODE
        retcode;
    int
        feedback = TRUE;

    if (context)
      {
        ctx = (ODBC_CONNECT_CTX *)context;
        retcode = SQLTransact (ctx-> environment, ctx-> connection, SQL_ROLLBACK);
        if (retcode != SQL_SUCCESS
        &&  retcode != SQL_SUCCESS_WITH_INFO)
          {
            coprintf ("DBIO ODBC: Error dbio_odbc_rollback (%s)",
                      set_odbc_error_value (ctx-> environment,
                                            ctx-> connection));
            feedback = FALSE;
          }
      }

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: alloc_odbc_handle

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

ODBCHANDLE *
alloc_odbc_handle    (char *table_name, void *context)
{
    ODBCHANDLE
        *handle = NULL;                 /* odbc Context handle                */
    ODBC_CONNECT_CTX
        *ctx;
    RETCODE
        retcode;
    HSTMT
        statement;
    if (context)
      {
        ctx = (ODBC_CONNECT_CTX *)context;
        retcode = SQLAllocStmt (ctx-> connection, &statement);
        if (retcode != SQL_SUCCESS
        &&  retcode != SQL_SUCCESS_WITH_INFO)
          {         
            coprintf ("DBIO ODBC: Error in alloc_odbc_handle (%s, %s)",
                      table_name, set_odbc_error_value (ctx-> environment,
                                                        ctx-> connection));
          }
        else
          {
            handle = mem_alloc (sizeof (ODBCHANDLE));
            if (handle)
              {
                memset (handle, 0, sizeof (ODBCHANDLE));
                handle-> environment = ctx-> environment;
                handle-> connection  = ctx-> connection;
                handle-> statement   = statement;
                if (table_name && strused (table_name))
                    handle-> table_name = mem_strdup (table_name);
              }
            else
                coprintf ("DBIO ODBC: Error in alloc_odbc_handle (Memory Allocation)");
          }
      }
 
    return (handle);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_odbc_connection

    Synopsis: Reconnect to a odbc database.
    ---------------------------------------------------------------------[>]-*/

void
set_odbc_connection  (void *context)
{
    ODBC_CONNECT_CTX
        *handle;

    ASSERT (context);
    handle = (ODBC_CONNECT_CTX *)context;

}

/*###########################################################################*/
/*                         LOCAL FUNCTION DEFINITION                        #*/
/*###########################################################################*/

/*  -------------------------------------------------------------------------
    Function: set_odbc_error_value

    Synopsis: Get error message value
    -------------------------------------------------------------------------*/

static char *
set_odbc_error_value (HENV env, HDBC connect)
{
    long
        size;
    short
        size2;

    memset (&error, 0, sizeof (error));    
    SQLError (env, connect, SQL_NULL_HSTMT, error.code_msg, &size,
              error.message, ERR_MSG_SIZE - 1, &size2);

    return (error.message);
}
