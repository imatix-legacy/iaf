/*  ----------------------------------------------------------------<Prolog>-
    Name:       iaforacle.c
    Title:      IAF ORACLE DB Management
    Package:    IAF

    Written:    2002/02/15  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/02/21  Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Implement principal functions of iAF db agent.

    Copyright:  Copyright (c) 1991-2002 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/
#define _POSIX_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sqlca.h>
#include <sqlda.h>
#include <sqlcpr.h>

/*- comming from sfl.h ------------------------------------------------------*/

#define streq(s1,s2)    (!strcmp ((s1), (s2)))
#define strneq(s1,s2)   (strcmp ((s1), (s2)))
#define strterm(s)      ((s) [strlen (s)])
typedef unsigned short  Bool;           /*  Boolean TRUE/FALSE value         */
typedef unsigned char   byte;           /*  Single unsigned byte = 8 bits    */
typedef unsigned short  dbyte;          /*  Double byte = 16 bits            */
typedef unsigned short  word;           /*  Alternative for double-byte      */
typedef unsigned long   dword;          /*  Double word >= 32 bits           */
typedef unsigned long   qbyte;          /*  Quad byte = 32 bits              */
typedef struct {                        /*  Memory descriptor                */
    size_t size;                        /*    Size of data part              */
    byte  *data;                        /*    Data part follows here         */
} DESCR;

typedef struct {                        /*  Variable-size descriptor         */
    size_t max_size;                    /*    Maximum size of data part      */
    size_t cur_size;                    /*    Current size of data part      */
    byte  *data;                        /*    Data part follows here         */
} VDESCR;

#if (!defined (TRUE))
#    define TRUE        1               /*  ANSI standard                    */
#    define FALSE       0
#endif

typedef unsigned short  UCODE;
#define UCODE_SIZE      2

#include "sfllist.h"
#include "sflxml.h"
#include "sflxmll.h"
#include "sflmem.h"
#include "sflstr.h"
#include "sfldate.h"
#include "sflsymb.h"
#include "sflhttp.h"

/*- Definitions -------------------------------------------------------------*/
#define BUFFER_SIZE 65535

typedef int (TABLE_FUNC) (char *table);

#define VERSION                    "1.0"
#define PROGRAM_NAME               "iAFOracle"

#define DATA_TYPE_CHAR             1
#define DATA_TYPE_NUMERIC          2
#define DATA_TYPE_DECIMAL          3
#define DATA_TYPE_INTEGER          4
#define DATA_TYPE_SMALL_INT        5
#define DATA_TYPE_FLOAT            6
#define DATA_TYPE_REAL             7
#define DATA_TYPE_DOUBLE           8
#define DATA_TYPE_DATE             9
#define DATA_TYPE_TIME             10
#define DATA_TYPE_TIME_STAMP       11
#define DATA_TYPE_VARCHAR          12
#define DATA_TYPE_LONG_VARCHAR     (-1)
#define DATA_TYPE_BINARY           (-2)
#define DATA_TYPE_VARBINARY        (-3)
#define DATA_TYPE_LONG_VARBINARY   (-4)
#define DATA_TYPE_BIG_INT          (-5)
#define DATA_TYPE_TINY_INT         (-6)
#define DATA_TYPE_BIT              (-7)
#define DATA_TYPE_UNICODE_CHAR     (-8)
#define DATA_TYPE_UNICODE_VARCHAR  (-9)
#define DATA_TYPE_UNICODE_TEXT     (-10)

#define MAX_ITEMS         100           /* Maximum number of select-list
                                         * items or bind variables           */

#define MAX_VNAME_LEN     30            /* Maximum lengths of the _names_ of */
#define MAX_INAME_LEN     30            /* the select-list items or indicator
                                         * variables                         */
#define ERROR_FETCH_NULL    -1405
#define ERROR_NO_DATA_FOUND  1403

/*- Structure ---------------------------------------------------------------*/

/*  Table definition structure                                               */

typedef struct _table_defn
{
    struct _table_defn
        *next, *prev;                   /*  Linked list pointer              */
    char *name;                         /*  Name of table                    */
} TABLE_DEFN;

/*  Columns definition structure                                             */

typedef struct _col_defn
{
    struct _col_defn
          *next, *prev;                 /*  Linked list pointer             */
    char  *table_name;                  /*  Name of the table               */
    char  *name;                        /*  Column name                     */
    short  data_type;                   /*  Type of data                    */
    char  *type_name;                   /*  Name of the data type           */
    int    precision;
    int    length;
    short  scale;
    short  nullable;
    void  *value;
    long   indic;
} COL_DEFN;

/* Index definition structure                                                 */

typedef struct _idx_defn
{
    struct _idx_defn
          *next, *prev;                 /*  Linked list pointer             */
    char  *table_name;
    short  non_unique;
    char  *index_name;
    short  seq_in_index;
    char  *column_name;
    short  primary;
    char   collation;
} IDX_DEFN;

/*- Global Variable ----------------------------------------------------------*/
EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR user_name   [20  + 1];
    VARCHAR user_pwd    [20  + 1];
    VARCHAR table_name  [30  + 1];
    VARCHAR column_name [30  + 1];
    VARCHAR data_type   [106 + 1];
    int     data_length;
    int     data_precision;
    int     data_scale;
    VARCHAR nullable    [1   + 1];
    VARCHAR index_name  [30  + 1];
    VARCHAR index_desc  [4   + 1];
    VARCHAR index_uniq  [9   + 1];  
    int     index_order;
    char    sql_statement[10240];
    EXEC SQL VAR sql_statement IS STRING(10240);
EXEC SQL END DECLARE SECTION;

char
    *user         = NULL,                /*  User name                         */
    *password     = NULL,
    *database     = NULL,
    *table        = NULL,
    *list_file    = NULL,
    *output       = NULL,
    *sql_file     = NULL,
    *exec_file    = NULL,
    *max_rows     = NULL,
    *max_time     = NULL,
    *exp_list     = NULL,
    *dbml_file    = NULL,
    *import_list  = NULL,
    *db_schema    = NULL,
    *summary_file = NULL;
static int
    nb_columns;
static LIST
    type_list,
    table_list,
    index_list,
    columns;
static char
    message_code [20 + 1],
    message      [255];
static XML_ITEM
    *xml_item = NULL;
static long
    max_row = 0;
static char
    buffer [BUFFER_SIZE + 1];

static short
    indic1,
    indic2,
    indic3,
    indic4,
    indic5,
    indic6;
static Bool
    dropall       = FALSE,
    force2zero    = TRUE,
    import        = FALSE,
    export        = FALSE,
    encode_data   = FALSE;
static FILE
    *out_file     = NULL;

SQLDA *bind_dp;
SQLDA *select_dp;

char
    *where_clause = NULL;

/* Include incremental parser for big xml file                               */
#define DB_ORACLE
/*#include "dbpars.c"*/


/*- Functions definition ----------------------------------------------------*/
void      free_resource              (void);
void      display_usage              (const char *program_name);
int       print_table_struct         (char *table);
int       export_all_tables          (void);
int       export_table               (char *table_name);
void      print_dbml                 (char *table);
int       get_all_columns_definition (char *table_name);
void      get_all_index_definition   (char *table_name);
int       get_all_table_name         (void);
void      free_all_columns_name      (LIST *col_head);
void      free_all_index_name        (LIST *index_head);
void      free_all_table_name        (LIST *table_head);
void      free_column                (COL_DEFN *column);
char *    get_pk_name                (char *table_name);
void      set_data_type              (COL_DEFN *column);
void      execute_table_list         (char *list_file, TABLE_FUNC *function,
                                      Bool table_exist);
int       allocate_descriptor        (void);
int       set_bind_variables         (void);
void      free_descriptor            (void);
void      describe_select_list       (void);
long      exec_select                (LIST *col_list, char *table, char *where,
                                      char *order_by);
void      save_record_to_xml         (LIST *col_list, long recno);
Bool      no_db_error                (void);

/*- MAIN --------------------------------------------------------------------*/

int
main (int argc, char *argv [])
{
    int
        feedback = 0;
    int
        argn;                           /*  Argument number                  */
    Bool
        args_ok = TRUE;                 /*  Were the arguments okay?         */
    Bool
        help = FALSE;
    char
        **argparm;                      /*  Argument parameter to pick-up    */

    list_reset (&columns);
    list_reset (&index_list);
    list_reset (&table_list);
    list_reset (&type_list);

    argparm = NULL;                     /*  Argument parameter to pick-up    */
    for (argn = 1; argn < argc; argn++)
      {
        /*  If argparm is set, we have to collect an argument parameter      */
        if (argparm && *argv [argn] != '-')
          {
            *argparm = mem_strdup (argv [argn]);
            argparm = NULL;
          }
        else
        if (argparm
        && *argv [argn] != '-'
        && argparm      != &exp_list
        && argparm      != &import_list
           )
          {
            args_ok = FALSE;
            break;
          }
        else
        if (*argv [argn] == '-')
          {
            argparm = NULL;
            switch (argv [argn][1])
              {
                /*  These switches take a parameter                          */
                case 'c': argparm    = &summary_file; break;
                case 'u': argparm    = &user;         break;
                case 'p': argparm    = &password;     break;
                case 'g': argparm    = &table;        break;
                case 'l': argparm    = &list_file;    break;
                case 'o': argparm    = &output;       break;
                case 'x': argparm    = &sql_file;     break;
                case 'm': argparm    = &max_rows;     break;
                case 't': argparm    = &max_time;     break;
                case 'r': argparm    = &dbml_file;    break;
                case 'h': help       = TRUE;          break;
                case 'n': force2zero = TRUE;          break;
                case 's': 
                    if (argv [argn][2] == '\0')
                          argparm    = &exec_file;
                    else
                          argparm    = &db_schema;
                    break;
                case 'd': argparm = &database;
                    break;
                case 'i':
                    argparm = &import_list;
                    import  = TRUE;
                    if (argv [argn][2] == 'e')
                        dropall = TRUE;
                    break;
                case 'e':
                    if (argv [argn][2] == 'n')
                        encode_data = TRUE;
                    else
                      {
                        export  = TRUE;
                        argparm = &exp_list;
                      }
                    break;
                /*  Anything else is an error                                */
                default:
                    args_ok = FALSE;
              }
          }
        else
          {
            args_ok = FALSE;
            break;
          }
      }


    if (output)
        out_file = fopen (output, "w");
    else
        out_file  = stdout;

    if (help || (argc == 1))
      {
        display_usage (argv[0]);
        free_resource ();
        return (feedback);
      }

    /*  If there was a missing parameter or an argument error, quit          */
    if (argparm && argparm != &import_list && argparm != &exp_list)
      {
        puts ("Argument missing - type 'iaforacle -h' for help");
        free_resource ();
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        puts ("Invalid arguments - type 'iaforacle -h' for help");
        free_resource ();
        exit (EXIT_FAILURE);
      }
    if (database == NULL)
      {
        puts ("A database name is required - type 'iaforacle -h' for help");
        free_resource ();
        exit (EXIT_FAILURE);
      }


    if (max_rows)
        max_row = atol (max_rows);

    strcpy ((char *)user_name.arr, user);   
    user_name.len = strlen ((char *)user_name.arr);
    strcpy ((char *)user_pwd.arr, password);
    user_pwd.len = strlen ((char *)user_pwd.arr);


    EXEC SQL CONNECT :user_name IDENTIFIED BY :user_pwd;
    if (no_db_error ())
      {
        if (table)
            print_table_struct (table);
/*        if (list_file)
            execute_table_list (list_file, print_table_struct, TRUE);
*/
        if (export)
          {
            if (exp_list)
                execute_table_list (exp_list, export_table, TRUE);
            else
                export_all_tables  ();
          }
/*
        if (exec_file)
            execute_select_request (exec_file);
        if (sql_file)
            execute_sql_file (sql_file);
        if (dbml_file)
            review_table (dbml_file);
        if (import)
          {
            fprintf (out_file, "<import_result>\n");
            get_data_type_info ();
            get_all_table_name ();
            if (import_list)
                execute_table_list (import_list, new_import_table, FALSE);
            else
                import_all_tables ();
            fprintf (out_file, "</import_result>\n");
          }

        if (summary_file)
            print_database_summary (summary_file);
*/
        EXEC SQL COMMIT WORK RELEASE;
      }

    if (out_file && out_file != stdout)
        fclose (out_file);

    free_resource ();

    mem_assert ();
    return (feedback);
}


/*- Local functions ---------------------------------------------------------*/

/*  ---------------------------------------------------------------------[<]-
    Function: free_resource

    Synopsis: Free all allocated resource
    ---------------------------------------------------------------------[>]-*/

void
free_resource (void)
{
    if (user        != NULL)
        mem_strfree (&user);
    if (password    != NULL)
        mem_strfree (&password);
    if (database    != NULL)
        mem_strfree (&database);
    if (table       != NULL)
        mem_strfree (&table);
    if (list_file   != NULL)
        mem_strfree (&list_file);
    if (output      != NULL)
        mem_strfree (&output);
    if (sql_file    != NULL)
        mem_strfree (&sql_file);
    if (exec_file   != NULL)
        mem_strfree (&exec_file);
    if (max_rows    != NULL)
        mem_strfree (&max_rows);
    if (max_time    != NULL)
        mem_strfree (&max_time);
    if (exp_list    != NULL)
        mem_strfree (&exp_list);
    if (dbml_file   != NULL)
        mem_strfree (&dbml_file);
    if (import_list != NULL)
        mem_strfree (&import_list);
    if (summary_file != NULL)
        mem_strfree (&summary_file);
    if (db_schema != NULL)
        mem_strfree (&db_schema);

    free_all_columns_name (&columns);
    free_all_index_name   (&index_list);
    free_all_table_name   (&table_list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_db_error_message

    Synopsis: Retrieve error message value. 
              1 if no more record or duplicate record, 0 if hard error
    ---------------------------------------------------------------------[>]-*/

Bool
no_db_error (void)
{
   Bool
       feedback = TRUE;

    memset (message_code, 0, 21);
    memset (message,      0, 255);


    if (sqlca.sqlcode != 0)
        feedback = FALSE;

    if (feedback == FALSE
    &&  sqlca.sqlcode != ERROR_FETCH_NULL
    &&  sqlca.sqlcode != ERROR_NO_DATA_FOUND)
        printf ("SQL error code %d: %s\n", sqlca.sqlcode, sqlca.sqlerrm.sqlerrmc);

   return (feedback);
}

void display_usage (const char *program_name)
{
    fprintf (out_file, "%s version %s\n", program_name, VERSION);
    fprintf (out_file, "Command-Line Oracle Access Program.\n");
    fprintf (out_file, "Copyright (c) 1996-2002 iMatix - http://www.imatix.com\n\n");

    fprintf (out_file, "syntax:\n");
    fprintf (out_file, "    %s -<function> -<options>\n\n", program_name);

    fprintf (out_file, "function :\n");
    fprintf (out_file, " -e  [listfile]    Export tables listed in list file. Default is all tables\n");
/*    fprintf (out_file, " -i  [listfile]    Import tables listed in list file. Default is all tables\n");
    fprintf (out_file, " -ie [listfile]    Import tables listed in list file. Default is all tables\n");
    fprintf (out_file, "                   this import delete record before inport\n");
    fprintf (out_file, " -x  sqlfile       Execute SQL file. which can contain multiple SQL statement\n");
    fprintf (out_file, "                   delimited by ';' (not newlines).\n");
    fprintf (out_file, " -s  filename      Execute a formatted SELECT statement.\n");
    fprintf (out_file, "                   Return data as XML file.\n");
    fprintf (out_file, " -g  tablename     Get structure of table name.\n");
    fprintf (out_file, " -l  listfile      Get structure of tables listed in XML file as follows : \n");
    fprintf (out_file, "                      REQUEST (no attributes)\n");
    fprintf (out_file, "                          TABLE (NAME)\n");
    fprintf (out_file, " -c  filename      Get record count of each tables\n");
    fprintf (out_file, "                  Returns result as XML file.\n");
*/
    fprintf (out_file, "\noptions :\n");
    fprintf (out_file, " -d   database    ODBC Data Source name\n");
    fprintf (out_file, " -u   user        Connect username (if required)\n");
    fprintf (out_file, " -p   password    Connect password (if required)\n");
    fprintf (out_file, " -o   output      Specifies output file for actions. Default is stdout.\n");
    fprintf (out_file, " -m   maxrows     Max desired rows for select or export.\n");
    fprintf (out_file, " -t   maxtime     Max desired time for select, export or import(msecs)\n");
    fprintf (out_file, " -sch schema      Name of schema (or catalog) used to list all tables\n");
    fprintf (out_file, " -en              Encode data\n");
}


/*  -------------------------------------------------------------------------
    Function: print_table_struct

    Synopsis: Print structure and index of a table in DBML format.
    Returns : 0 on success, -1 otherwise
    -------------------------------------------------------------------------*/

int
print_table_struct (char *table)
{
    get_all_columns_definition (table);
    if (nb_columns > 0)
      {
        get_all_index_definition (table);
        print_dbml (table);
      }
    free_all_columns_name (&columns);
    free_all_index_name   (&index_list);

    return 0;                           /* print will never fail             */
}


/*  -------------------------------------------------------------------------
    Function: export_all_table
    Synopsis: export all table structure and data.
    returns:  0 on succes, -1 on error
    -------------------------------------------------------------------------*/

int
export_all_tables (void)
{
    int rc;

    TABLE_DEFN
        *table;

    if (get_all_table_name () == 0)
        return 0;
    table = table_list.next;

    fputs ("<export_result>\n", out_file);
    while ((void *)table != (void *)&table_list)
      {
            rc = export_table (table-> name);
            if (rc != 0)
                return rc;            /* we break on first error */
        table = table-> next;
      }
    fputs ("</export_result>\n", out_file);
    return 0;
}


/*  -------------------------------------------------------------------------
    Function: export_table
    Synopsis: export table structure and data.
    Returns:  0 on success, -1 on error
    -------------------------------------------------------------------------*/
int
export_table (char *table_name)
{
    static char
        table      [50],
        tmp_buffer [255],
        file_name  [50];
    FILE
        *old = NULL;
    long
        recno = 0;


    strcpy (table, table_name);
    sprintf (tmp_buffer, "\"%s\"", strlwc (table));
    fprintf (out_file, "<export encode = \"%d\" table =%-30s", encode_data, tmp_buffer);

    sprintf (file_name, "%s.xml", table);
    if (out_file)
      {
        old = out_file;
        out_file = fopen (file_name, "w");
      }

    get_all_columns_definition (table_name);
    if (nb_columns > 0)
      {
        get_all_index_definition (table_name);
        fputs ("<export>", out_file);
        print_dbml  (table_name);
        recno = exec_select (&columns, table_name, where_clause, NULL);
        fputs ("</export>", out_file);
      }
    free_all_columns_name (&columns);
    free_all_index_name   (&index_list);

    if (out_file)
        fclose (out_file);
    out_file = old;

    sprintf (tmp_buffer, "\"%ld\"", recno);
    fprintf (out_file, " row=%-8s/>\n", tmp_buffer);

    return 0;
}


/*  -------------------------------------------------------------------------
    Function: get_all_columns_definition

    Synopsis: Get table definition.
    -------------------------------------------------------------------------*/

int
get_all_columns_definition (char *name)
{
    COL_DEFN
        *col;

    nb_columns = 0;

    strcpy ((char *)table_name.arr, name);
    table_name.len = strlen ((char *)table_name.arr);

    EXEC SQL DECLARE GET_ALL_COLUMNS CURSOR FOR
        SELECT 
            COLUMN_NAME,
            DATA_TYPE,
            DATA_LENGTH,
            DATA_PRECISION,
            DATA_SCALE,
            NULLABLE
        FROM USER_TAB_COLUMNS WHERE TABLE_NAME = :table_name
        ORDER BY COLUMN_ID;


    EXEC SQL OPEN  GET_ALL_COLUMNS ; 

    do
      {
        memset (&column_name, 0, sizeof (column_name));
        memset (&data_type,   0, sizeof (data_type));
        memset (&nullable,    0, sizeof (nullable));
        data_length    = 0;
        data_precision = 0;
        data_scale     = 0;
        EXEC SQL FETCH GET_ALL_COLUMNS INTO 
            :column_name:indic1,
            :data_type:indic2,
            :data_length:indic3,
            :data_precision:indic4,
            :data_scale:indic5,
            :nullable:indic6;

        if (no_db_error ())
          {
            column_name.arr [column_name.len] = '\0';
            data_type.arr   [data_type.len]   = '\0';
            nullable.arr    [nullable.len]    = '\0';

            col = mem_alloc (sizeof (COL_DEFN));
            if (col)
              {
                memset (col, 0, sizeof (COL_DEFN));
                list_reset (col);
                col-> name       = mem_strdup ((char *)column_name.arr);
                col-> type_name  = mem_strdup ((char *)data_type.arr);
                col-> table_name = mem_strdup ((char *)table_name.arr);
                col-> precision  = data_precision;
                col-> length     = data_length;
                if (data_length > BUFFER_SIZE)
                    col-> length    = BUFFER_SIZE;
                col-> scale      = data_scale;
                col-> nullable   = (nullable.arr [0] == 'Y');
                set_data_type (col);
                list_relink_before (col, &columns);
                nb_columns++;
              }
          }
      }
    while (sqlca.sqlcode == 0);
      
    EXEC SQL CLOSE  GET_ALL_COLUMNS ; 
    return (nb_columns);
}

/*  -------------------------------------------------------------------------
    Function: set_data_type 

    Synopsis: Attribute a data type number from Oracle type name, data length
    and scale.
    -------------------------------------------------------------------------*/

void
set_data_type (COL_DEFN *column)
{
    if (lexcmp (column-> type_name, "NUMBER") == 0)
      {
        if (column-> scale > 0)
            column-> data_type = DATA_TYPE_DECIMAL;
        else
            column-> data_type = DATA_TYPE_NUMERIC;
      }
    else
    if (lexcmp (column-> type_name, "VARCHAR2") == 0)
        column-> data_type = DATA_TYPE_VARCHAR;
    else
    if (lexcmp (column-> type_name, "DATE") == 0)
        column-> data_type = DATA_TYPE_TIME_STAMP;
    else
    if (lexcmp (column-> type_name, "ROWID") == 0)
        column-> data_type = DATA_TYPE_VARCHAR;
    else
    if (lexcmp (column-> type_name, "LONG") == 0)
        column-> data_type = DATA_TYPE_NUMERIC;
    else
    if (lexcmp (column-> type_name, "CHAR") == 0)
        column-> data_type = DATA_TYPE_CHAR;
    else
    if (lexcmp (column-> type_name, "NVARCHAR2") == 0)
        column-> data_type = DATA_TYPE_UNICODE_VARCHAR;
    else
    if (lexcmp (column-> type_name, "NCHAR") == 0)
        column-> data_type = DATA_TYPE_UNICODE_CHAR;
}

/*  -------------------------------------------------------------------------
    Function: get_all_index_definition

    Synopsis: Get index table definition.
    -------------------------------------------------------------------------*/

void
get_all_index_definition   (char *table)
{
    IDX_DEFN
        *idx;
    char
        *pk_name;

    pk_name = get_pk_name (table);

    strcpy ((char *)table_name.arr, table);
    table_name.len = strlen ((char *)table_name.arr);

    EXEC SQL DECLARE GET_ALL_INDEXES CURSOR FOR
        SELECT 
            C.INDEX_NAME,
            C.COLUMN_NAME,
            C.COLUMN_POSITION,
            I.UNIQUENESS
        FROM USER_IND_COLUMNS C, USER_INDEXES I 
        WHERE C.TABLE_NAME = :table_name
          AND I.TABLE_NAME = C.TABLE_NAME
          AND I.INDEX_NAME = C.INDEX_NAME
        ORDER BY C.TABLE_NAME, C.INDEX_NAME, C.COLUMN_POSITION;
        
    EXEC SQL OPEN  GET_ALL_INDEXES ; 

    do
      {
        memset (&column_name, 0, sizeof (column_name));
        memset (&index_name,  0, sizeof (index_name));
        memset (&index_desc,  0, sizeof (index_desc));
        memset (&index_uniq,  0, sizeof (index_uniq));
        index_order = 0;

        EXEC SQL FETCH GET_ALL_INDEXES INTO 
            :index_name:indic1,
            :column_name:indic2,
            :index_order:indic3,
            :index_uniq:indic5;

        if (no_db_error ())
          {
            index_name.arr  [index_name.len]  = '\0';
            column_name.arr [column_name.len] = '\0';
            index_desc.arr  [0]  = 'A';
            index_uniq.arr  [index_uniq.len]  = '\0';

            idx = mem_alloc (sizeof (IDX_DEFN));
            if (idx)
              {
                memset (idx, 0, sizeof (IDX_DEFN));
                list_reset (idx);

                idx-> table_name   = mem_strdup (table);
                idx-> index_name   = mem_strdup ((char *)index_name.arr);
                idx-> column_name  = mem_strdup ((char *)column_name.arr);
                idx-> seq_in_index = index_order;
                idx-> non_unique   = streq ((char *)index_uniq.arr, "NONUNIQUE");
                idx-> collation    = *index_desc.arr;
                if (pk_name
                &&  *pk_name
                &&  idx-> index_name
                &&  streq (pk_name, idx-> index_name))
                    idx-> primary = TRUE;
                list_relink_before (idx, &index_list);
              }
          }
      }
    while (sqlca.sqlcode == 0);

    EXEC SQL CLOSE  GET_ALL_INDEXES ; 
}


/*  -------------------------------------------------------------------------
    Function: get_pk_name

    Synopsis: Return primary key value.
    -------------------------------------------------------------------------*/

char *
get_pk_name (char *table)
{
    static char
        feedback [50];

    strcpy ((char *)table_name.arr, table);
    table_name.len = strlen ((char *)table_name.arr);
    memset (&index_name,  0, sizeof (index_name));
    *feedback = '\0';

    EXEC SQL SELECT
        CONSTRAINT_NAME
    INTO
        :index_name:indic1
    FROM USER_CONSTRAINTS
    WHERE CONSTRAINT_TYPE = 'P' 
      AND TABLE_NAME      = :table_name;

    if (no_db_error ())
      {
        index_name.arr  [index_name.len]  = '\0';
        strcrop ((char *)index_name.arr);
        strcpy (feedback, (char *)index_name.arr);
      }      
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_all_table_name

    Synopsis: Get all table name from a data source.
              Return the number of tables.
    ---------------------------------------------------------------------[>]-*/

int
get_all_table_name  (void)
{
    int
        nb_tables = 0;
    TABLE_DEFN
        *table;

    EXEC SQL DECLARE GET_ALL_TABLE CURSOR FOR
        SELECT TABLE_NAME FROM USER_TABLES;


    EXEC SQL OPEN  GET_ALL_TABLE ; 
    do {
        EXEC SQL FETCH GET_ALL_TABLE INTO :table_name;
        if (sqlca.sqlcode == 0)
          {
            if (table_name.len > 0)
              {
                table_name.arr [table_name.len] = '\0';
                table = mem_alloc (sizeof (TABLE_DEFN));
                if (table)
                  {
                    memset (table, 0, sizeof (TABLE_DEFN));
                    list_reset (table);
               
                    table-> name   = mem_strdup ((char *)table_name.arr);
                    list_relink_before (table, &table_list);
                    nb_tables++;
                  }
              }            
          }
      }
    while (sqlca.sqlcode == 0);

    EXEC SQL CLOSE  GET_ALL_TABLE ; 

    return (nb_tables);
}


/*  -------------------------------------------------------------------------
    Function: free_all_columns_name

    Synopsis: Free column definition list
    -------------------------------------------------------------------------*/

void
free_all_columns_name (LIST *col_head)
{
    COL_DEFN
        *column,
        *next;

   column = (COL_DEFN *)col_head-> next;
   while ((void *)column != (void *)col_head)
     {
       next = column-> next;

       free_column (column);
       column = next;
     }
    list_reset (col_head);
}


/*  -------------------------------------------------------------------------
    Function: free_column

    Synopsis: Free a column from definition list
    -------------------------------------------------------------------------*/

void
free_column (COL_DEFN *column)
{
       mem_strfree (&column-> name);
       mem_strfree (&column-> type_name);
       mem_strfree (&column-> table_name);
       if (column-> value)
           mem_strfree ((char **)&column-> value);
       list_unlink (column);
       mem_free    (column);
}


/*  -------------------------------------------------------------------------
    Function: free_all_index_name

    Synopsis: Free column definition list
    -------------------------------------------------------------------------*/

void
free_all_index_name (LIST *index_head)
{
    IDX_DEFN
        *index,
        *next;

    index = (IDX_DEFN *)index_head-> next;
    while ((void *)index != (void *)index_head)
      {
        next = index-> next;

        if (index-> table_name)
            mem_free (index-> table_name);
        if (index-> index_name)
            mem_free (index-> index_name);
        if (index-> column_name)
            mem_free (index-> column_name);
        mem_free (index);

        index = next;
      }
   list_reset (index_head);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_all_table_name

    Synopsis: Free table definition list
    ---------------------------------------------------------------------[>]-*/

void
free_all_table_name (LIST *table_head)
{
    TABLE_DEFN
        *table,
        *next;

   table = (TABLE_DEFN *)table_head-> next;
   while ((void *)table != (void *)table_head)
     {
       next = table-> next;
       mem_strfree (&table-> name);
       mem_free    (table);
       table = next;
     }
   list_reset (table_head);
}


/*  -------------------------------------------------------------------------
    Function: print_dbml

    Synopsis: Print table structure in dbml format.
    -------------------------------------------------------------------------*/

void
print_dbml (char *table_name)
{
    COL_DEFN
        *column,
        *next;
    IDX_DEFN
        *idx,
        *idx_next;
    char
        code = 'A',
        *last_idx;
    long
        date;

    date = date_now ();

   fprintf (out_file, "<table\n    description = \"\"\n");
   fprintf (out_file, "    created     = \"%ld\"\n",  date);
   fprintf (out_file, "    updated     = \"%ld\"\n",  date);
   fprintf (out_file, "    name        = \"%s\" >\n", table_name);


   column = (COL_DEFN *)columns.next;
   while ((void *)column != (void *)&columns)
     {
       next = column-> next;
       fprintf (out_file, "<field name    = \"%s\"\n", column-> name);
       switch (column-> data_type)
         {
           case DATA_TYPE_CHAR:
           case DATA_TYPE_VARCHAR:
           case DATA_TYPE_LONG_VARCHAR:
               fputs   ("       type    = \"TEXTUAL\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"%d\"\n", column-> length);
               break;
           case DATA_TYPE_NUMERIC:
           case DATA_TYPE_DECIMAL:
               fputs   ("       type    = \"NUMERIC\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"%d\"\n", column-> precision);
               fprintf (out_file, "       decimal = \"%d\"\n", column-> scale);
               break;
               break;
           case DATA_TYPE_INTEGER:
           case DATA_TYPE_TINY_INT:
           case DATA_TYPE_SMALL_INT:
               fputs   ("       type    = \"NUMERIC\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"%d\"\n", column-> precision);
               fprintf (out_file, "       decimal = \"%d\"\n", column-> scale);
               break;
           case DATA_TYPE_FLOAT:
           case DATA_TYPE_REAL:
           case DATA_TYPE_DOUBLE:
               fputs   ("       type    = \"NUMERIC\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"%d\"\n", column-> precision);
               if (column-> scale > 0)
                   fprintf (out_file, "       decimal = \"%d\"\n", column-> scale);
               else
                   fprintf (out_file, "       decimal = \"5\"\n");
               break;
           case DATA_TYPE_DATE:
               fputs   ("       type    = \"DATE\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"8\"\n");
               break;
           case DATA_TYPE_TIME:
               fputs   ("       type    = \"TIME\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"8\"\n");
               break;
           case DATA_TYPE_TIME_STAMP:
               fputs   ("       type    = \"TIMESTAMP\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"26\"\n");
               break;
           case DATA_TYPE_BINARY:
               break;
           case DATA_TYPE_VARBINARY:
               break;
           case DATA_TYPE_LONG_VARBINARY:
               fputs   ("       type    = \"BINARY\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"%d\"\n", column-> length);
               break;
               break;
           case DATA_TYPE_BIG_INT:
               break;
           case DATA_TYPE_BIT:
               fputs   ("       type    = \"NUMERIC\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"1\"\n");
               fprintf (out_file, "       decimal = \"0\"\n");
               break;

           case DATA_TYPE_UNICODE_CHAR:
           case DATA_TYPE_UNICODE_VARCHAR:
           case DATA_TYPE_UNICODE_TEXT:
               fputs   ("       type    = \"UNICODE_TEXTUAL\"\n", out_file);
               if (column-> type_name)
                   fprintf (out_file, "       ntype   = \"%s\"\n", column-> type_name);
               fprintf (out_file, "       size    = \"%d\"\n", column-> length / UCODE_SIZE);
               break;
         }

       fprintf (out_file, "       nulls   = \"%c\" />\n",
                 column-> nullable? 'Y': 'N');
       column = next;
     }
   fputs (" ", out_file);
   idx = (IDX_DEFN *)index_list.next;
   last_idx = NULL;
   while ((void *)idx != (void *)&index_list)
     {
       idx_next = idx-> next;
       if (idx-> index_name)
         {
           if (idx-> seq_in_index == 1)
             {
               if (last_idx
               &&  strneq (last_idx, idx-> index_name))
                   fputs ("</key>\n", out_file);

               fprintf (out_file, "<key name=\"%s\" duplicates=\"%s\" code=\"%c\" >\n",
                         idx-> index_name,
                         idx-> non_unique? "TRUE": "FALSE",
                         idx-> primary? 'P': code++);
             }
           last_idx = idx-> index_name;
           fprintf (out_file, "  <field name=\"%s\" sort=\"%s\" />\n",
                     idx-> column_name,
                     (idx-> collation == 'D')? "DESCENDING": "ASCENDING");

         }
       idx = idx_next;
     }
   if (last_idx)
       fputs ("</key>\n", out_file);

   fputs ("</table>\n", out_file);
}

/*  -------------------------------------------------------------------------
    Function: exec_select

    Synopsis: Execute a select statement
    -------------------------------------------------------------------------*/

long
exec_select (LIST *col_list, char *table, char *where, char *order_by)
{
    COL_DEFN
        *column,
        *next;
    long
        recno = 0;

    /* Set select statement                                                  */
    strcpy (buffer, "SELECT ");

    column = (COL_DEFN *)col_list-> next;
    while ((void *)column != (void *)col_list)
      {
        next = column-> next;
        if (column->data_type == DATA_TYPE_TIME_STAMP)
            sprintf (&strterm (buffer), "TO_CHAR (%s, 'YYYY-MM-DD HH24:MI:SS'), ",
                     column-> name);
        else
            sprintf (&strterm (buffer), "%s, ", column-> name);
        column = next;
      }
    buffer [strlen (buffer) - 2] = ' ';
    sprintf (&strterm (buffer), "FROM %s", table);
    if (where)
        sprintf (&strterm (buffer), " WHERE %s", where);
    if (order_by)
        sprintf (&strterm (buffer), " ORDER BY %s", order_by);

    if (allocate_descriptor () == 0)
        return (0);

    strcpy (sql_statement, buffer);
    EXEC SQL PREPARE S FROM :sql_statement;

    EXEC SQL DECLARE db_cursor CURSOR FOR S;

    if (set_bind_variables () != 0)
      {
        free_descriptor ();
        return (0);
      }

    EXEC SQL OPEN db_cursor USING DESCRIPTOR bind_dp;

    describe_select_list ();


    while (sqlca.sqlcode == 0)
      {
        EXEC SQL FETCH db_cursor USING DESCRIPTOR select_dp;

        if (sqlca.sqlcode == 0)
            save_record_to_xml (col_list, ++recno);
      }

    free_descriptor ();
    EXEC SQL CLOSE db_cursor;

    return (recno);
}


/*  -------------------------------------------------------------------------
    Function: get_struct_from_list

    Synopsis: Load a xml request file a export structre of table in request.
    -------------------------------------------------------------------------*/

void
execute_table_list (char *list_file, TABLE_FUNC *function, Bool table_exist)
{
    XML_ITEM
        *child   = NULL,
        *request = NULL;
    char
        *table_mask;
    TABLE_DEFN
        *table_def,
        *next;
    int rc;


    rc = xml_load_file (&xml_item, ".", list_file, FALSE);
    if (rc != XML_NOERROR)
      {
        if (xml_item != NULL)
            xml_free (xml_item);
        return ;
      }

    if (table_exist)
      {
        if (get_all_table_name () == 0)
          {
            xml_free (xml_item);
            return;
          }
      }

    FORCHILDREN (request, xml_item)
      {
        FORCHILDREN (child, request)
          {
            table_mask = xml_get_attr (child,   "name",   NULL);
            where_clause = xml_get_attr (child, "where", NULL);
            if (table_mask)
              {
                if (table_exist)
                  {
                    table_def = (TABLE_DEFN *)table_list.next;
                    while ((void *)table_def != (void *)&table_list)
                      {
                        next = table_def-> next;
                        if (lexwcmp (table_def-> name, table_mask) == 0)
                          {
                            rc = (*function) (table_def-> name);
                            if (rc != 0)
                              {
                                xml_free (xml_item);
                                return ;
                              }
                          }
                        table_def = next;
                      }
                  }
                else
                  {
                    rc = (*function) (table_mask);
                    if (rc != 0)
                      {
                        xml_free (xml_item);
                        return ;
                      }
                  }
             }
          }
      }
    xml_free (xml_item);
    where_clause = NULL;
}

int
allocate_descriptor (void)
{
    int
        index;
    /* Allocate descriptor                                                   */
    bind_dp = sqlald (MAX_ITEMS, MAX_VNAME_LEN, MAX_INAME_LEN);
    if (bind_dp == (SQLDA *)0)
    {
        printf("Error: Cannot allocate memory for bind descriptor.\n");
        return (0);
    }

    select_dp = sqlald (MAX_ITEMS, MAX_VNAME_LEN, MAX_INAME_LEN);
    if (select_dp == (SQLDA *)0)
    {
        sqlclu (bind_dp);
        printf("Error: Cannot allocate memory for select descriptor.\n");
        return (0);
    }
    select_dp-> N = MAX_ITEMS;

    /* Allocate the pointers to the indicator variables, and the
       actual data. */
    for (index = 0; index < MAX_ITEMS; index++)
      {
        bind_dp->   I [index] = (short *) mem_alloc (sizeof (short));
        select_dp-> I [index] = (short *) mem_alloc (sizeof (short));
        bind_dp->   V [index] = (char *)  mem_alloc (1);
        select_dp-> V [index] = (char *)  mem_alloc (1);
      }
    return (1);
}

/*  -------------------------------------------------------------------------
    Function: set_bind_variables

    Synopsis: 
    -------------------------------------------------------------------------*/

int
set_bind_variables (void)
{

    /* Describe any bind variables (input host variables)                    */

    bind_dp-> N = MAX_ITEMS;            /* Initialize count of array elements*/
    EXEC SQL DESCRIBE BIND VARIABLES FOR S INTO bind_dp;

    /* If F is negative, there were more bind variables
       than originally allocated by sqlald(). */
    if (bind_dp->F < 0)
    {
        printf ("error: Too many bind variables (%d), maximum is %d.",
                 -bind_dp->F, MAX_ITEMS);
        return (-1);
    }

    /* Set the maximum number of array elements in the
       descriptor to the number found. */
    bind_dp-> N = bind_dp-> F;

    return (0);
}

void
free_descriptor (void)
{
    int
        index;

    if (bind_dp != NULL 
    &&  select_dp != NULL)
      {
        for (index = 0; index < MAX_ITEMS; index++)
          {
            if (bind_dp-> V [index] != (char *) 0)
                mem_free (bind_dp-> V [index]);

            mem_free (bind_dp-> I [index]);
            if (select_dp-> V [index] != (char *) 0)
                mem_free (select_dp-> V [index]);
            mem_free (select_dp-> I [index]);
          }

        /* Free space used by the descriptors themselves.                    */
        sqlclu (bind_dp);
        bind_dp = NULL;
        sqlclu (select_dp);
        select_dp = NULL;
      }  
}


/*  -------------------------------------------------------------------------
    Function: describe_select_list

    Synopsis: 
    -------------------------------------------------------------------------*/

void
describe_select_list (void)
{
    int
        index,
        null_ok,
        precision,
        scale;

    select_dp-> N = MAX_ITEMS;

    EXEC SQL DESCRIBE SELECT LIST FOR S INTO select_dp;
    /* If F is negative, there were more select-list
       items than originally allocated by sqlald(). */
    if (select_dp-> F < 0)
      {
        printf ("Error: Too many select-list items (%d), maximum is %d",
                 -(select_dp-> F), MAX_ITEMS);
        return;
      }

    /* Set the maximum number of array elements in the
       descriptor to the number found. */
    select_dp-> N = select_dp-> F;

    /* Allocate storage for each select-list item.

       sqlprc() is used to extract precision and scale
       from the length (select_dp->L[i]).

       sqlnul() is used to reset the high-order bit of
       the datatype and to check whether the column
       is NOT NULL.

       CHAR    datatypes have length, but zero precision and
               scale.  The length is defined at CREATE time.

       NUMBER  datatypes have precision and scale only if
               defined at CREATE time.  If the column
               definition was just NUMBER, the precision
               and scale are zero, and you must allocate
               the required maximum length.

       DATE    datatypes return a length of 7 if the default
               format is used.  This should be increased to
               9 to store the actual date character string.
               If you use the TO_CHAR function, the maximum
               length could be 75, but will probably be less
               (you can see the effects of this in SQL*Plus).

       ROWID   datatype always returns a fixed length of 18 if
               coerced to CHAR.

       LONG and
       LONG RAW datatypes return a length of 0 (zero),
                so you need to set a maximum.  In this example,
                it is 240 characters.
    */

    /* Turn off high-order bit of datatype (in this example,
       it does not matter if the column is NOT NULL). */
    for (index = 0; index < select_dp-> F; index++)
      {
        sqlnul (&(select_dp-> T [index]),
                &(select_dp-> T [index]),
                &null_ok);

        switch (select_dp->T [index])
          {
            case  1 :                   /* CHAR datatype: no change in length
                                           needed, except possibly for TO_CHAR
                                           conversions (not handled here).   */
                break;
            case  2 :                   /* NUMBER datatype: use sqlprc() to
                                           extract precision and scale.      */
                sqlprc (&(select_dp->L [index]), &precision, &scale);
                /* Allow for maximum size of NUMBER.                         */
                if (precision == 0)
                    precision = 40;
                /* Also allow for decimal point and possible sign.           */
                /* convert NUMBER datatype to FLOAT if scale > 0,
                   INT otherwise.                                            */
                if (scale > 0)
                    select_dp-> L [index] = sizeof (float);
                else
                    select_dp-> L [index] = sizeof (int);
                break;

            case  8 :                   /* LONG datatype                     */
                select_dp-> L [index] = 240;
                break;

            case 11 :                   /* ROWID datatype                    */
                select_dp-> L [index] = 18;
                break;

            case 12 :                  /* DATE datatype                      */
                select_dp-> L [index] = 9;
                break;

            case 23 :                  /* RAW datatype                       */
                break;

            case 24 :                  /* LONG RAW datatype                  */
                select_dp->L [index] = 240;
                break;
        }
        /* Allocate space for the select-list data values.
           sqlald() reserves a pointer location for
           V[i] but does not allocate the full space for
           the pointer.                                                      */

         if (select_dp-> T [index] != 2)
           {
             select_dp-> V [index] = (char *)mem_realloc (
                                             select_dp-> V [index],
                                             select_dp-> L [index] + 1);
             memset (select_dp-> V [index], 0,
                     select_dp-> L [index] + 1);
           }
         else
             select_dp-> V [index] = (char *)mem_realloc(
                                             select_dp-> V [index],
                                             select_dp-> L [index]);


        /* Coerce ALL datatypes except for LONG RAW and NUMBER to
           character.                                                        */
        if (   select_dp-> T [index] != 24
            && select_dp-> T [index] != 2)
            select_dp-> T [index] = 1;

        /* Coerce the datatypes of NUMBERs to float or int depending on
           the scale. */
        if (select_dp-> T [index] == 2)
          {
            if (scale > 0)
              select_dp-> T [index] = 4;  /* float                            */
            else
              select_dp-> T [index] = 3;  /* int                              */
          }
    }
}


/*  -------------------------------------------------------------------------
    Function: save_record_to_xml

    Synopsis: Save a record to a XML file
    -------------------------------------------------------------------------*/

void
save_record_to_xml (LIST *col_list, long recno)
{
    COL_DEFN
        *column,
        *next;
    int
        index = 0;
    char
        value [4096],
        *buf;

   fprintf (out_file, "<record\nrecno=\"%ld\"\n", recno);

   column = (COL_DEFN *)col_list-> next;
   while ((void *)column != (void *)col_list)
     {
       next = column-> next;

       if (*select_dp-> I [index] < 0)
         {
           if (select_dp-> T [index] == 4) 
               sprintf (buffer, "%-*c",(int)select_dp-> L [index]+3, ' ');
            else
               sprintf (buffer, "%-*c",(int)select_dp-> L [index], ' ');
         }
       else
         {
           if (select_dp-> T [index] == 3)   /* int datatype                      */
               sprintf (buffer, "%-*d", (int)select_dp-> L [index], 
                                    *(int *)select_dp-> V [index]);
           else 
           if (select_dp-> T [index] == 4)   /* float datatype                    */
               sprintf (buffer, "%-*f", (int)select_dp-> L [index], 
                                     *(float *)select_dp-> V [index]);
           else
             {
                                            /* character string                  */
               sprintf (value, "%-*s", (int)select_dp-> L [index],
                                              select_dp-> V [index]);
               buf = value;
               http_encode_meta (buffer, &buf, BUFFER_SIZE, FALSE);
             }
          }
       strcrop (buffer);
       if (*buffer)
           fprintf (out_file, "%s=\"%s\"\n", column-> name, buffer);
       index++;
       column = next;
     }
    fprintf (out_file, "/>\n");
}
