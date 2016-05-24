/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflodbc.h
    Title:      ODBC Database interface - header file
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
#if !defined (_SFLODBC_INCLUDED) && defined (DBIO_ODBC)
#define _SFLODBC_INCLUDED
#include "sfldbio.h"
#include <sqlext.h>

typedef struct {
    COMMON_DBIO_CTX
    HENV  environment;                  /*  ODBC environment                 */
    HDBC  connection;                   /*  ODBC connection handle           */
    HSTMT statement;                    /*  Current ODBC statement           */
    char  *table_name;                  /*  Table name                       */
} ODBCHANDLE;

#ifdef __cplusplus
extern "C" {
#endif

void      *dbio_odbc_connect    (char *db_name, char *user, char *pwd);
void       dbio_odbc_disconnect (void *context);
int        dbio_odbc_commit     (void *context);
int        dbio_odbc_rollback   (void *context);
ODBCHANDLE *alloc_odbc_handle    (char *table_name, void *context);
void       free_odbc_handle     (ODBCHANDLE *handle);
void       set_odbc_connection  (void *context);

#ifdef __cplusplus
}
#endif

#endif
