/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafodbc.c
    Title:      IAF ODBC DB Management
    Package:    IAF

    Written:    1999/08/31  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/02/21  Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Implement principal functions of iAF db agent.

    Copyright:  Copyright (c) 1991-2002 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/
#include "sfl.h"
#include "sflodbc.h"

/*- Definitions -------------------------------------------------------------*/

#define AUTO_COMMIT_COUNT          200

#define STR_LEN                    128
#define REM_LEN                    255

#define BUFFER_SIZE                65534
#define DATE_SIZE                  10
#define TIME_SIZE                  8
#define TIMESTAMP_SIZE             26


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


typedef int (TABLE_FUNC) (char *table);

#define VERSION                    "1.8"

#define DBMS_TYPE_MSACCESS         1
#define DBMS_TYPE_MSSQL            2
#define DBMS_TYPE_ORACLE           3

#define IS_VALID_COL ((col-> value && col-> indic >= 0) || col-> nullable == FALSE \
        || (force2zero && (col-> data_type == DATA_TYPE_NUMERIC                   \
                       ||  col-> data_type == DATA_TYPE_DECIMAL                   \
                       ||  col-> data_type == DATA_TYPE_INTEGER                   \
                       ||  col-> data_type == DATA_TYPE_SMALL_INT                 \
                       ||  col-> data_type == DATA_TYPE_FLOAT                     \
                       ||  col-> data_type == DATA_TYPE_REAL                      \
                       ||  col-> data_type == DATA_TYPE_DOUBLE                    \
                       ||  col-> data_type == DATA_TYPE_BIG_INT                   \
                       ||  col-> data_type == DATA_TYPE_TINY_INT                  \
                       ||  col-> data_type == DATA_TYPE_BIT)))

#define IS_VALID_COL_NULL (col-> value == NULL && col-> nullable == TRUE \
        && !(             (col-> data_type == DATA_TYPE_NUMERIC                   \
                       ||  col-> data_type == DATA_TYPE_DECIMAL                   \
                       ||  col-> data_type == DATA_TYPE_INTEGER                   \
                       ||  col-> data_type == DATA_TYPE_SMALL_INT                 \
                       ||  col-> data_type == DATA_TYPE_FLOAT                     \
                       ||  col-> data_type == DATA_TYPE_REAL                      \
                       ||  col-> data_type == DATA_TYPE_DOUBLE                    \
                       ||  col-> data_type == DATA_TYPE_BIG_INT                   \
                       ||  col-> data_type == DATA_TYPE_TINY_INT                  \
                       ||  col-> data_type == DATA_TYPE_BIT)))

#define IS_VALID_COL_ZERO (col-> value == NULL && col-> nullable == TRUE \
        && (               col-> data_type == DATA_TYPE_NUMERIC                   \
                       ||  col-> data_type == DATA_TYPE_DECIMAL                   \
                       ||  col-> data_type == DATA_TYPE_INTEGER                   \
                       ||  col-> data_type == DATA_TYPE_SMALL_INT                 \
                       ||  col-> data_type == DATA_TYPE_FLOAT                     \
                       ||  col-> data_type == DATA_TYPE_REAL                      \
                       ||  col-> data_type == DATA_TYPE_DOUBLE                    \
                       ||  col-> data_type == DATA_TYPE_BIG_INT                   \
                       ||  col-> data_type == DATA_TYPE_TINY_INT                  \
                       ||  col-> data_type == DATA_TYPE_BIT))

/*- Structure ---------------------------------------------------------------*/

/*  Table definition structure                                               */

typedef struct _table_defn
{
    struct _table_defn
        *next, *prev;                   /*  Linked list pointer              */
    char *qualifier;                    /*  Table qualifier identifier       */
    char *owner;                        /*  Table owner                      */
    char *name;                         /*  Name of table                    */
    char *remark;                       /*  Description of the table         */
    char *type;                         /*  Type, one of the following:      *
                                         *  "TABLE", "VIEW", "SYSTEM TABLE", *
                                         *  "GLOBAL TEMPORARY", "ALIAS",     *
                                         *  "LOCAL TEMPORARY", "SYNONYM" or  *
                                         *   specific type identifier.       */
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
    short  radix;
    short  nullable;
    char  *remark;
    void  *value;
    SDWORD indic;
    Bool   in_primary_key;              /* Indicate if field is in primary key */
} COL_DEFN;

/* Index definition structure                                                 */

typedef struct _idx_defn
{
    struct _idx_defn
          *next, *prev;                 /*  Linked list pointer             */
    char  *qualifier;
    char  *owner;
    char  *table_name;
    short  non_unique;
    char  *index_qualifier;
    char  *index_name;
    short  type;
    short  seq_in_index;
    char  *column_name;
    char   collation;
    char  *filter;
    Bool   primary;
} IDX_DEFN;

/* ODBC driver data type                                                      */

typedef struct _type_defn
{
    struct _type_defn
          *next, *prev;                 /*  Linked list pointer             */
    char *type_name;
    short data_type;
    int   precision;
    char *literal_prefix;
    char *literal_suffix;
    char *create_params;
    short nullable;
    short case_sensitive;
    short searchable;
    short unsign_attr;
} TYPE_DEFN;


/*- Global Variable ----------------------------------------------------------*/
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
ODBCHANDLE
    *handle      = NULL;
static int
    nb_columns;
static LIST
    type_list,
    table_list,
    index_list,
    columns;
static HDBC
    h_connection;
static HENV
    h_environ;
static HSTMT
    h_statement;
static RETCODE
    return_code;
static char
    message_code [20 + 1],
    message      [SQL_MAX_MESSAGE_LENGTH];
static XML_ITEM
    *xml_item = NULL;
static long
    max_row = 0;
static char
    buffer [BUFFER_SIZE + 1];

static char
    pk_name         [STR_LEN + 1],
    col_name        [STR_LEN + 1],
    type_name       [STR_LEN + 1],
    remark          [REM_LEN + 1],
    qualifier       [STR_LEN + 1],
    owner           [STR_LEN + 1],
    table_name      [STR_LEN + 1],
    index_qualifier [STR_LEN + 1],
    index_name      [STR_LEN + 1],
    filter          [STR_LEN + 1],
    column_name     [STR_LEN + 1],
    type            [STR_LEN + 1],
    prefix          [STR_LEN + 1],
    suffix          [STR_LEN + 1],
    collation       [2];
static Bool
    database_arg       = FALSE,
    dropall            = FALSE,
    incremental_import = FALSE,
    force2zero         = TRUE,
    have_identity      = FALSE,
    import             = FALSE,
    export             = FALSE,
    encode_data        = FALSE;
static FILE
    *out_file          = NULL;
static int
    dbms_type          = 0;
static char
    *primary_key       = NULL,
    *where_clause      = NULL;

/*- Functions definition ----------------------------------------------------*/
void      free_resource              (void);
void      table_free_handle          (void *context);
void      print_dbml                 (char *table);
int       get_all_columns_definition (char *table_name);
void      get_all_index_definition   (char *table_name);
int       get_all_table_name         (void);
void      get_data_type_info         (void);
void      free_data_type_info        (LIST *type_head);
void      free_all_columns_name      (LIST *col_head);
void      free_all_index_name        (LIST *index_head);
void      free_all_table_name        (LIST *table_head);
void      free_column                (COL_DEFN *column);
char     *get_pk_name                (char *table_name);
int       print_table_struct         (char *table);
void      execute_table_list         (char *list_file, TABLE_FUNC *function, Bool table_exist);
void      allocate_column_value      (LIST *col_list);
void      bind_all_columns           (LIST *col_list);
int       export_table               (char *table_name);
int       export_all_tables          (void);
void      save_record_to_xml         (LIST *col_list, long recno);
void      execute_select_request     (char *select_request);
void      load_col_from_table_list   (char *table_list);
COL_DEFN *copy_col_def               (COL_DEFN *col);
COL_DEFN *get_select_col             (LIST *col_list, char *col_name);
long      exec_select                (LIST *col_list, char *table, char *where,
                                      char *order_by);
void      execute_sql_file           (char *sql_file);
void      review_table               (char *dbml_file);
char     *update_table_struct        (XML_ITEM *xml_item, LIST *col_list);
char     *get_dbml_struct            (XML_ITEM *xml_item, LIST *col_list);
short     get_odbc_type              (char *type, char *native_type,  COL_DEFN *col);
char     *search_type_in_list        (short odbc_type);
Bool      create_table               (char *table_name, LIST *col_list);
int       import_all_tables          (void);
void      import_table               (char *table_name);
Bool      import_row                 (char *table_name, XML_ITEM *xml_item,
                                      LIST *col_list, Bool encoded_data);
void      set_column_value           (COL_DEFN *column, char *value, Bool encoded_data);
void      drop_all_record            (char *table_name);

int       get_db_error_message       (void);
void      display_usage              (const char *program_name);
char     *value2string               (COL_DEFN *column, COL_DEFN *last);
char     *value2wherestring          (COL_DEFN *col, COL_DEFN *last);
Bool      update_row                 (LIST *col_list, COL_DEFN *last, char *table_name);
char     *get_real_table_name        (LIST *table_list, char *table_name);
char     *get_pk_name_from_list      (char *table);
void      print_dsn_list             (void);
Bool      print_dsn                  (dbyte direction, char *type);

static void print_database_summary (char *filename);
static long get_record_count       (LIST *col_list, char *table);
static int  get_dbms_type          (void);
static void set_field_primary_key_flag (LIST *col_list);

/* Include incremental parser for big xml file                               */
#define DB_ODBC
#include "dbpars.c"


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
                case 'd': 
                    argparm = &database;
                    database_arg = TRUE;
                    break;
                case 'i':
                    argparm = &import_list;
                    import  = TRUE;
                    if (argv [argn][2] == 'e')
                        dropall = TRUE;
                    else
                    if (argv [argn][2] == 'i')
                        incremental_import = TRUE;
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
    if (argparm 
    &&  argparm != &import_list 
    &&  argparm != &exp_list
    &&  argparm != &database)
      {
        puts ("Argument missing - type 'iafodbc -h' for help");
        free_resource ();
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        puts ("Invalid arguments - type 'iafodbc -h' for help");
        free_resource ();
        exit (EXIT_FAILURE);
      }
    if (database == NULL && database_arg == FALSE)
      {
        puts ("A database name is required - type 'iafodbc -h' for help");
        free_resource ();
        exit (EXIT_FAILURE);
      }


    if (max_rows)
        max_row = atol (max_rows);

    if (database_arg && database == NULL)
      {
        print_dsn_list ();
        free_resource ();
        mem_assert ();
        return (feedback);
      }

    if (dbio_connect (database, user, password, NULL, TRUE, DB_TYPE_ODBC))
      {
        handle = dbio_get_handle (DB_TYPE_ODBC, "common", database, -1);
        if (handle)
          {
            handle-> free_handle = table_free_handle;
            h_connection = handle-> connection;
            h_environ    = handle-> environment;
            h_statement  = handle-> statement;

            dbms_type = get_dbms_type ();
            if (db_schema == NULL
            &&  dbms_type == DBMS_TYPE_ORACLE)
                db_schema = mem_strdup (user);

            if (table)
              print_table_struct (table);
            if (list_file)
                execute_table_list (list_file, print_table_struct, TRUE);
            if (export)
              {
                if (exp_list)
                    execute_table_list (exp_list, export_table, TRUE);
                else
                    export_all_tables  ();
              }
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
          }
        dbio_disconnect ();
      }

    if (out_file && out_file != stdout)
        fclose (out_file);
//    console_capture (NULL, CONSOLE_PLAIN);

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
    free_data_type_info   (&type_list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_db_error_message

    Synopsis: Retrieve error message value. 
              1 if no more record or duplicate record, 0 if hard error
    ---------------------------------------------------------------------[>]-*/

int
get_db_error_message (void)
{
   int
       feedback = 0;
   long
        error_code;
   short
        size;

    memset (message_code, 0, 21);
    memset (message,      0, SQL_MAX_MESSAGE_LENGTH);

   SQLError (h_environ, h_connection, h_statement, message_code,
             &error_code, message, SQL_MAX_MESSAGE_LENGTH - 1, &size);
   if (strcmp (message_code, "23000") == 0
   ||  strcmp (message_code, "00000") == 0)
            feedback = 1;

   return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: table_free_handle

    Synopsis: Free all resource for a table connection
    ---------------------------------------------------------------------[>]-*/
void
table_free_handle (void *context)
{
    ODBCHANDLE
        *handle;
    handle = (ODBCHANDLE *)context;
    SQLFreeStmt (handle-> statement, SQL_DROP);
    if (handle-> table_name)
        mem_strfree (&handle-> table_name);
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
       mem_strfree (&column-> remark);
       mem_strfree (&column-> table_name);
       if (column-> value)
           mem_strfree ((char **)&column-> value);
       list_unlink (column);
       mem_free    (column);

}
/*  -------------------------------------------------------------------------
    Function: copy_col_def

    Synopsis: Copy a COL_DEFN structure
    -------------------------------------------------------------------------*/

COL_DEFN *
copy_col_def (COL_DEFN *col)
{
    COL_DEFN
        *new_col = NULL;

    ASSERT (col);

    new_col = mem_alloc (sizeof (COL_DEFN));
    if (new_col)
      {
        memcpy (new_col, col, sizeof (COL_DEFN));
        list_reset (new_col);
        if (col-> name)
            new_col-> name       = mem_strdup (col-> name);
        if (col-> type_name)
            new_col-> type_name  = mem_strdup (col-> type_name);
        if (col-> remark)
            new_col-> remark     = mem_strdup (col-> remark);
        if (col-> name)
            new_col-> table_name = mem_strdup (col-> table_name);
        new_col-> value = NULL;
      }
    return (new_col);
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

        if (index-> qualifier)
            mem_free (index-> qualifier);
        if (index-> owner)
            mem_free (index-> owner);
        if (index-> table_name)
            mem_free (index-> table_name);
        if (index-> index_qualifier)
            mem_free (index-> index_qualifier);
        if (index-> index_name)
            mem_free (index-> index_name);
        if (index-> column_name)
            mem_free (index-> column_name);
        if (index-> filter)
            mem_free (index-> filter);
        mem_free (index);

        index = next;
      }
   list_reset (index_head);
}


/*  -------------------------------------------------------------------------
    Function: get_all_columns_definition

    Synopsis: Get table definition.
    -------------------------------------------------------------------------*/

int
get_all_columns_definition (char *table_name)
{
    static int
        precision,
        length;
    static short
        data_type,
        scale,
        radix,
        nullable;
    SDWORD
        indic,
        indic_name,
        indic_type_name,
        indic_remark;
    COL_DEFN
        *col;

    nb_columns = 0;
    have_identity = FALSE;

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);
    return_code = SQLColumns (h_statement,
                              NULL, 0,  /*  All qualifiers                   */
                              db_schema, SQL_NTS,
                              (UCHAR FAR *)table_name, SQL_NTS,
                              NULL, 0); /*  All columns                      */
    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
      {
        SQLBindCol (h_statement, 4, SQL_C_CHAR,
                    &col_name,   STR_LEN, &indic_name);
        SQLBindCol (h_statement, 5, SQL_C_SSHORT,
                    &data_type,  0, &indic);
        SQLBindCol (h_statement, 6, SQL_C_CHAR,
                    &type_name,  STR_LEN, &indic_type_name);
        SQLBindCol (h_statement, 7, SQL_C_SLONG,
                    &precision,  0, &indic);
        SQLBindCol (h_statement, 8, SQL_C_SLONG,
                    &length,     0, &indic);
        SQLBindCol (h_statement, 9, SQL_C_SSHORT,
                    &scale,      0, &indic);
        SQLBindCol (h_statement, 10, SQL_C_SSHORT,
                    &radix,      0, &indic);
        SQLBindCol (h_statement, 11, SQL_C_SSHORT,
                    &nullable,   0, &indic);
        SQLBindCol (h_statement, 12, SQL_C_CHAR,
                    &remark,     REM_LEN, &indic_remark);
        do
          {
            memset (col_name,      0, STR_LEN);
            memset (type_name, 0, STR_LEN);
            memset (remark,    0, REM_LEN);
            precision = 0;
            length    = 0;
            data_type = 0;
            scale     = 0;
            radix     = 0;
            nullable  = 0;
            return_code = SQLFetch (h_statement);
            if (return_code == SQL_SUCCESS
            ||  return_code == SQL_SUCCESS_WITH_INFO)
              {
                col = mem_alloc (sizeof (COL_DEFN));
                if (col)
                  {
                    memset (col, 0, sizeof (COL_DEFN));
                    list_reset (col);
                    if (indic_name > 0)
                        col-> name      = mem_strdup (col_name);
                    if (indic_type_name > 0)
                        col-> type_name = mem_strdup (type_name);
                    if (indic_remark > 0)
                        col-> remark = mem_strdup (remark);
                    col-> table_name = mem_strdup (table_name);
                    col-> precision = precision;
                    col-> length    = length;
                    if (length > BUFFER_SIZE)
                        col-> length    = BUFFER_SIZE;
                    col-> data_type = data_type;
                    col-> scale     = scale;
                    col-> radix     = radix;
                    col-> nullable  = nullable;
                    if (have_identity == FALSE
                    &&  strstr (type_name, "identity") != NULL)
                        have_identity = TRUE;
                    list_relink_before (col, &columns);
                    nb_columns++;
                  }
               }
             else
             if (nb_columns == 0)
                 get_db_error_message ();

          } while (   return_code == SQL_SUCCESS
                   || return_code == SQL_SUCCESS_WITH_INFO);
      }

    if (incremental_import)
        set_field_primary_key_flag (&columns);
    return (nb_columns);
}


/*  -------------------------------------------------------------------------
    Function: get_pk_name

    Synopsis: Return primary key value.
    -------------------------------------------------------------------------*/

char *
get_pk_name (char *table_name)
{
    static char
        *feedback = NULL;
    SDWORD
        indic;

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);
    return_code = SQLPrimaryKeys (h_statement,
                              NULL, 0,  /*  All qualifiers                   */
                              db_schema, SQL_NTS,
                              (UCHAR FAR *)table, SQL_NTS);
    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
      {
        SQLBindCol (h_statement, 6, SQL_C_CHAR,
                    &pk_name,  STR_LEN, &indic);
        return_code = SQLFetch (h_statement);
        if (return_code == SQL_SUCCESS
        ||  return_code == SQL_SUCCESS_WITH_INFO)
          {
            if (indic > 0)
                feedback = pk_name;
          }
      }
    else
        get_db_error_message ();

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);
    return (feedback);
}

/*  -------------------------------------------------------------------------
    Function: get_all_index_definition

    Synopsis: Get index table definition.
    -------------------------------------------------------------------------*/

void
get_all_index_definition   (char *table)
{
    static short
        type,
        seq_in_index,
        non_unique;
    SDWORD
        indic,
        indic_qualifier,
        indic_owner,
        indic_table_name,
        indic_index_qualifier,
        indic_index_name,
        indic_filter,
        indic_column_name;
    IDX_DEFN
        *idx;
    char
        *pk_name;

    pk_name = get_pk_name (table);

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);

    return_code = SQLStatistics (h_statement,
                              NULL, 0,  /*  All qualifiers                   */
                              db_schema, SQL_NTS,
                              (UCHAR FAR *)table, SQL_NTS,
                              SQL_INDEX_ALL, SQL_QUICK);

    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
      {
        SQLBindCol (h_statement, 1, SQL_C_CHAR,
                    &qualifier,  STR_LEN, &indic_qualifier);
        SQLBindCol (h_statement, 2, SQL_C_CHAR,
                    &owner,      STR_LEN, &indic_owner);
        SQLBindCol (h_statement, 3, SQL_C_CHAR,
                    &table_name, STR_LEN, &indic_table_name);
        SQLBindCol (h_statement, 4, SQL_C_SSHORT,
                    &non_unique, 0, &indic);
        SQLBindCol (h_statement, 5, SQL_C_CHAR,
                    &index_qualifier, STR_LEN, &indic_index_qualifier);
        SQLBindCol (h_statement, 6, SQL_C_CHAR,
                    &index_name,  STR_LEN, &indic_index_name);
        SQLBindCol (h_statement, 7, SQL_C_SSHORT,
                    &type,       0, &indic);
        SQLBindCol (h_statement, 8, SQL_C_SSHORT,
                    &seq_in_index, 0, &indic);
        SQLBindCol (h_statement, 9, SQL_C_CHAR,
                    &column_name,  STR_LEN, &indic_column_name);
        SQLBindCol (h_statement, 10, SQL_C_CHAR,
                    &collation,  1, &indic);
        SQLBindCol (h_statement, 13, SQL_C_CHAR,
                    &filter,  STR_LEN, &indic_filter);

        do
          {
            memset (qualifier,       0, STR_LEN);
            memset (owner,           0, STR_LEN);
            memset (table_name,      0, STR_LEN);
            memset (index_qualifier, 0, STR_LEN);
            memset (index_name,      0, STR_LEN);
            memset (filter,          0, STR_LEN);
            memset (column_name,     0, STR_LEN);
            collation [0] = 0;
            type          = 0;
            seq_in_index  = 0;
            non_unique    = 0;

            return_code = SQLFetch (h_statement);
            if (return_code == SQL_SUCCESS
            ||  return_code == SQL_SUCCESS_WITH_INFO)
              {
                idx = mem_alloc (sizeof (IDX_DEFN));
                if (idx)
                  {
                    memset (idx, 0, sizeof (IDX_DEFN));
                    list_reset (idx);

                    if (indic_qualifier       > 0)
                        idx-> qualifier       = mem_strdup (qualifier);
                    if (indic_owner           > 0)
                        idx-> owner           = mem_strdup (owner);
                    if (indic_table_name      > 0)
                        idx-> table_name      = mem_strdup (table_name);
                    if (indic_index_qualifier > 0)
                        idx-> index_qualifier = mem_strdup (index_qualifier);
                    if (indic_index_name      > 0)
                        idx-> index_name      = mem_strdup (index_name);
                    if (indic_filter          > 0)
                        idx-> filter          = mem_strdup (filter);
                    if (indic_column_name     > 0)
                        idx-> column_name     = mem_strdup (column_name);

                    idx-> collation    = collation [0];
                    idx-> type         = type;
                    idx-> seq_in_index = seq_in_index;
                    idx-> non_unique   = non_unique;
                    if (pk_name
                    &&  idx-> index_name
                    &&  streq (pk_name, idx-> index_name))
                        idx-> primary = TRUE;
                    list_relink_before (idx, &index_list);
                  }
               }
          } while (   return_code == SQL_SUCCESS
                   || return_code == SQL_SUCCESS_WITH_INFO);
      }
    else
        get_db_error_message ();
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
    SDWORD
        indic_qualifier,
        indic_owner,
        indic_name,
        indic_type,
        indic_remark;
    TABLE_DEFN
        *table;

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);
    return_code = SQLTables (h_statement, 
                             NULL,      SQL_NTS,
                             db_schema, SQL_NTS,
                             NULL,      SQL_NTS,
                             "TABLE",   SQL_NTS);
    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
      {
        SQLBindCol (h_statement, 1, SQL_C_CHAR,
                    &qualifier,  STR_LEN, &indic_qualifier);
        SQLBindCol (h_statement, 2, SQL_C_CHAR,
                    &owner,      STR_LEN, &indic_owner);
        SQLBindCol (h_statement, 3, SQL_C_CHAR,
                    &table_name, STR_LEN, &indic_name);
        SQLBindCol (h_statement, 4, SQL_C_CHAR,
                    &type,       STR_LEN, &indic_type);
        SQLBindCol (h_statement, 5, SQL_C_CHAR,
                    &remark,     REM_LEN, &indic_remark);
        do
          {
            memset (qualifier, 0, STR_LEN);
            memset (owner,     0, STR_LEN);
            memset (table_name,0, STR_LEN);
            memset (type,      0, STR_LEN);
            memset (remark,    0, REM_LEN);
            return_code = SQLFetch (h_statement);
            if (return_code == SQL_SUCCESS
            ||  return_code == SQL_SUCCESS_WITH_INFO)
              {
                table = mem_alloc (sizeof (TABLE_DEFN));
                if (table)
                  {
                    memset (table, 0, sizeof (TABLE_DEFN));
                    list_reset (table);
                    if (indic_qualifier > 0)
                        table-> qualifier =
                           mem_strdup (qualifier);
                    if (indic_owner > 0)
                        table-> owner  = mem_strdup(owner);
                    if (indic_name > 0)
                        table-> name   = mem_strdup (table_name);
                    if (indic_type > 0)
                        table-> type   = mem_strdup (type);
                    if (indic_remark > 0)
                        table-> remark = mem_strdup (remark);
                    list_relink_before (table, &table_list);
                    nb_tables++;
                 }
              }
            else
            if (nb_tables == 0)
                get_db_error_message ();
          } while (   return_code == SQL_SUCCESS
                   || return_code == SQL_SUCCESS_WITH_INFO);
      }
    return (nb_tables);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_data_type_info

    Synopsis: Get all type of data supported by the connected ODBC driver.
    ---------------------------------------------------------------------[>]-*/

void
get_data_type_info (void)
{
    SDWORD
        indic,
        indic_type_name,
        indic_lprefix,
        indic_lsuffix,
        indic_create_params;
    int
        precision;
    short
        data_type,
        nullable,
        case_sensitive,
        searchable,
        unsign_attr;
    TYPE_DEFN
        *type;

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);
    return_code = SQLGetTypeInfo (h_statement, SQL_ALL_TYPES);
    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
      {
        SQLBindCol (h_statement,  1, SQL_C_CHAR,
                    &type_name,   STR_LEN, &indic_type_name);
        SQLBindCol (h_statement,  2, SQL_C_SSHORT,
                    &data_type,   0, &indic);
        SQLBindCol (h_statement,  3, SQL_C_SLONG,
                    &precision,   0, &indic);
        SQLBindCol (h_statement,  4, SQL_C_CHAR,
                    &prefix,      STR_LEN, &indic_lprefix);
        SQLBindCol (h_statement,  5, SQL_C_CHAR,
                    &suffix,      STR_LEN, &indic_lsuffix);
        SQLBindCol (h_statement,  6, SQL_C_CHAR,
                    &remark,      STR_LEN, &indic_create_params);
        SQLBindCol (h_statement,  7, SQL_C_SSHORT,
                    &nullable,    0, &indic);
        SQLBindCol (h_statement,  8, SQL_C_SSHORT,
                    &case_sensitive,  0, &indic);
        SQLBindCol (h_statement,  9, SQL_C_SSHORT,
                    &searchable,  0, &indic);
        SQLBindCol (h_statement,  10, SQL_C_SSHORT,
                    &unsign_attr, 0, &indic);
        do
          {
            memset (type_name, 0, STR_LEN);
            memset (prefix,    0, STR_LEN);
            memset (suffix,    0, STR_LEN);
            memset (remark,    0, STR_LEN);
            return_code = SQLFetch (h_statement);
            if (return_code == SQL_SUCCESS
            ||  return_code == SQL_SUCCESS_WITH_INFO)
              {
                type = mem_alloc (sizeof (TYPE_DEFN));
                if (type)
                  {
                    memset (type, 0, sizeof (TYPE_DEFN));
                    list_reset (type);
                    if (indic_type_name     > 0)
                        type-> type_name       = mem_strdup (type_name);
                    if (indic_lprefix       > 0)
                        type-> literal_prefix  = mem_strdup(prefix);
                    if (indic_lsuffix       > 0)
                        type-> literal_suffix  = mem_strdup (suffix);
                    if (indic_create_params > 0)
                        type-> create_params   = mem_strdup (remark);
                    type-> data_type      = data_type;
                    type-> precision      = precision;
                    type-> nullable       = nullable;
                    type-> case_sensitive = case_sensitive;
                    type-> searchable     = searchable;
                    type-> unsign_attr    = unsign_attr;

                    list_relink_before (type, &type_list);
                  }
              }
          } while (   return_code == SQL_SUCCESS
                   || return_code == SQL_SUCCESS_WITH_INFO);
      }
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

   ASSERT (table_head);

   table = (TABLE_DEFN *)table_head-> next;
   while ((void *)table != (void *)table_head)
     {
       next = table-> next;
       mem_strfree (&table-> qualifier);
       mem_strfree (&table-> owner);
       mem_strfree (&table-> name);
       mem_strfree (&table-> type);
       mem_strfree (&table-> remark);
       mem_free    (table);
       table = next;
     }
   list_reset (table_head);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_data_type_info

    Synopsis: Free data type definition list
    ---------------------------------------------------------------------[>]-*/

void
free_data_type_info (LIST *type_head)
{
    TYPE_DEFN
        *type,
        *next;

   ASSERT (type_head);

   type = (TYPE_DEFN *)type_head-> next;
   while ((void *)type != (void *)type_head)
     {
       next = type-> next;
       mem_strfree (&type-> type_name);
       mem_strfree (&type-> literal_prefix);
       mem_strfree (&type-> literal_suffix);
       mem_strfree (&type-> create_params);
       mem_free    (type);
       type = next;
     }
   list_reset (type_head);
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
   fprintf (out_file, "    updated     = \"%d\"\n",   date);
   fprintf (out_file, "    name        = \"%s\" >\n",   table_name);


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

       if (column-> remark)
           fprintf (out_file, "       remark  = \"%s\"\n", column-> remark);
       fprintf (out_file, "       nulls   = \"%c\" />\n",
                 column-> nullable == SQL_NULLABLE? 'Y': 'N');
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
                         idx->non_unique? "TRUE": "FALSE",
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

    ASSERT (list_file);

    rc = xml_load_file (&xml_item, ".", list_file, FALSE);
    if (rc != XML_NOERROR)
      {
        if (xml_item != NULL)
            xml_free (xml_item);
        return ;
      }
    ASSERT (xml_item != NULL);

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
            table_mask   = xml_get_attr (child, "name",   NULL);
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
        tmp_buffer [255],
        file_name  [50];
    FILE
        *old;
    long
        recno = 0;


    sprintf (tmp_buffer, "\"%s\"", strlwc (table_name));
    fprintf (out_file, "<export encode = \"%d\" table =%-30s", encode_data, tmp_buffer);

    sprintf (file_name, "%s.xml", strlwc (table_name));
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

    ASSERT (col_list);
    ASSERT (table);

    /* Set select statement                                                  */
    strcpy (buffer, "SELECT ");

    column = (COL_DEFN *)col_list-> next;
    while ((void *)column != (void *)col_list)
      {
        next = column-> next;
        sprintf (&strterm (buffer), "%s.%s, ",
                 column-> table_name,
                 column-> name);
        column = next;
      }
    buffer [strlen (buffer) - 2] = ' ';
    sprintf (&strterm (buffer), "FROM %s", table);
    if (where)
        sprintf (&strterm (buffer), " WHERE %s", where);
    if (order_by)
        sprintf (&strterm (buffer), " ORDER BY %s", order_by);

    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);

    return_code = SQLExecDirect (h_statement, buffer, SQL_NTS);
    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
      {
        allocate_column_value (col_list);
        bind_all_columns      (col_list);
        recno = 0;
        do
          {
            return_code = SQLFetch (h_statement);

            if (return_code == SQL_SUCCESS
            ||  return_code == SQL_SUCCESS_WITH_INFO)
                save_record_to_xml (col_list, ++recno);
            else
              {
                if (get_db_error_message () == 0)
                    fputs (message, out_file);
              }

            if (max_row > 0
            &&  recno > max_row)
                break;

          } while (   return_code == SQL_SUCCESS
                   || return_code == SQL_SUCCESS_WITH_INFO);
      }
    else
      {
        get_db_error_message ();
        fputs (message, out_file);
      }
    return (recno);
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
    char
        *buf;
    UCODE
        *ubuf = NULL;

   fprintf (out_file, "<record\nrecno=\"%ld\"\n", recno);

   column = (COL_DEFN *)col_list-> next;
   while ((void *)column != (void *)col_list)
     {
       next = column-> next;
       if (column->indic >= 0)
         {
           switch (column-> data_type)
             {
               case DATA_TYPE_CHAR:
               case DATA_TYPE_VARCHAR:
               case DATA_TYPE_LONG_VARCHAR:
                   if (column->indic < 0)
                       column->indic = 0;
                   *((char *)column-> value + column-> indic) = '\0';
                   buf = (char *)column-> value;
                   http_encode_meta (buffer, &buf, BUFFER_SIZE, FALSE);
                   fprintf (out_file, "%s=\"%s\"\n", column-> name, buffer);
                   break;
               case DATA_TYPE_NUMERIC:
               case DATA_TYPE_DECIMAL:
                   if (column-> scale == 0)
                     {
                       if (column-> precision <= 9)
                           fprintf (out_file, "%s=\"%ld\"\n", column-> name, *(long *)column-> value);
                       else
                         {
                           if (dbms_type != DBMS_TYPE_ORACLE)                      
                               fprintf (out_file, "%s=\"%I64d\"\n", column-> name, *(_int64 *)column-> value);
                           else
                               fprintf (out_file, "%s=\"%.0f\"\n",  column-> name, *(double *)column-> value);
                         }
                     }
                   else
                       fprintf (out_file, "%s=\"%f\"\n",  column-> name, *(double *)column-> value);
                   break;
               case DATA_TYPE_INTEGER:
                       fprintf (out_file, "%s=\"%ld\"\n", column-> name, *(long *)column-> value);
                   break;
               case DATA_TYPE_TINY_INT:
                   fprintf (out_file, "%s=\"%d\"\n", column-> name, *(short *)column-> value);
                   break;
               case DATA_TYPE_SMALL_INT:
                   fprintf (out_file, "%s=\"%d\"\n", column-> name, *(short *)column-> value);
                   break;
               case DATA_TYPE_FLOAT:
               case DATA_TYPE_REAL:
               case DATA_TYPE_DOUBLE:
                   fprintf (out_file, "%s=\"%f\"\n", column-> name, *(double *)column-> value);
                   break;
               case DATA_TYPE_DATE:
                   if (column->indic < 0)
                       column->indic = 0;
                   *((char *)column-> value + column-> indic) = '\0';
                   fprintf (out_file, "%s=\"%s\"\n", column-> name, (char *)column-> value);
                   break;
               case DATA_TYPE_TIME:
                   if (column->indic < 0)
                       column->indic = 0;
                   *((char *)column-> value + column-> indic) = '\0';
                   fprintf (out_file, "%s=\"%s\"\n", column-> name, (char *)column-> value);
                   break;
               case DATA_TYPE_TIME_STAMP:
                   if (column->indic < 0)
                       column->indic = 0;
                   *((char *)column-> value + column-> indic) = '\0';
                   fprintf (out_file, "%s=\"%s\"\n", column-> name, (char *)column-> value);
                   break;
               case DATA_TYPE_BINARY:
                   break;
               case DATA_TYPE_VARBINARY:
                   break;
               case DATA_TYPE_LONG_VARBINARY:
                   if (column->indic < 0)
                       column->indic = 0;
                   *((char *)column-> value + column-> indic) = '\0';
                   buf = (char *)column-> value;
                   http_encode_meta (buffer, &buf, BUFFER_SIZE, FALSE);
                   fprintf (out_file, "%s=\"%s\"\n", column-> name, buffer);
                   break;
                   break;
               case DATA_TYPE_BIG_INT:
                   break;
               case DATA_TYPE_BIT:
                   fprintf (out_file, "%s=\"%d\"\n", column-> name, *(unsigned char *)column-> value);
                   break;
               case DATA_TYPE_UNICODE_CHAR:
               case DATA_TYPE_UNICODE_VARCHAR:
               case DATA_TYPE_UNICODE_TEXT:
                   *((byte *)column-> value + column-> indic) = 0;
                    ubuf = (UCODE *)column-> value;
                    http_encode_umeta ((UCODE *)buffer, &ubuf, BUFFER_SIZE / UCODE_SIZE, FALSE);
                    buf = ucode2ascii ((UCODE *)buffer);
                    if (buf)
                      {
                        fprintf (out_file, "%s=\"%s\"\n", column-> name, buf);
                        mem_strfree (&buf);
                      }
                   break;
             }
         }
       column = next;
     }
    fprintf (out_file, "/>\n");
}

/*  -------------------------------------------------------------------------
    Function: bind_all_columns

    Synopsis: bind all columns.
    -------------------------------------------------------------------------*/

void
bind_all_columns (LIST *col_list)
{
    SQLUSMALLINT
        index = 1;
    COL_DEFN
        *column,
        *next;
SQLRETURN
   feedback;

   column = (COL_DEFN *)col_list-> next;
   while ((void *)column != (void *)col_list)
     {
       next = column-> next;
       switch (column-> data_type)
         {
           case DATA_TYPE_CHAR:
           case DATA_TYPE_VARCHAR:
           case DATA_TYPE_LONG_VARCHAR:
               feedback = SQLBindCol (h_statement, index++, SQL_C_CHAR, column-> value,
                           column-> length + 1, &column-> indic);
               if (feedback != SQL_SUCCESS)
                   get_db_error_message ();
               break;
           case DATA_TYPE_NUMERIC:
           case DATA_TYPE_DECIMAL:
               if (column-> scale == 0)
                 {
                   if (column-> precision <= 9 )
                       SQLBindCol (h_statement, index++, SQL_C_SLONG,
                                   column-> value, 0 , &column-> indic);
                   else
                     {
                       if (dbms_type != DBMS_TYPE_ORACLE)
                           SQLBindCol (h_statement, index++, SQL_C_SBIGINT,
                                       column-> value, 0 , &column-> indic);
                       else
                           SQLBindCol (h_statement, index++, SQL_C_DOUBLE,
                                       column-> value, 0 , &column-> indic);
                     }
                 }
               else
                   SQLBindCol (h_statement, index++, SQL_C_DOUBLE,
                               column-> value, 0 , &column-> indic);
               break;
           case DATA_TYPE_INTEGER:
               SQLBindCol (h_statement, index++, SQL_C_SLONG,
                           column-> value, 0 , &column-> indic);
               break;
           case DATA_TYPE_TINY_INT:
           case DATA_TYPE_SMALL_INT:
               SQLBindCol (h_statement, index++, SQL_C_SSHORT,
                           column-> value, 0 , &column-> indic);
               break;
           case DATA_TYPE_FLOAT:
           case DATA_TYPE_REAL:
           case DATA_TYPE_DOUBLE:
               SQLBindCol (h_statement, index++, SQL_C_DOUBLE,
                           column-> value, 0 , &column-> indic);
               break;
           case DATA_TYPE_DATE:
               SQLBindCol (h_statement, index++, SQL_C_CHAR, column-> value,
                           DATE_SIZE + 1, &column-> indic);
               break;
           case DATA_TYPE_TIME:
               SQLBindCol (h_statement, index++, SQL_C_CHAR, column-> value,
                           TIME_SIZE + 1, &column-> indic);
               break;
           case DATA_TYPE_TIME_STAMP:
               SQLBindCol (h_statement, index++, SQL_C_CHAR, column-> value,
                           TIMESTAMP_SIZE + 1, &column-> indic);
               break;
           case DATA_TYPE_BINARY:
               break;
           case DATA_TYPE_VARBINARY:
               break;
           case DATA_TYPE_LONG_VARBINARY:
               feedback = SQLBindCol (h_statement, index++, SQL_C_BINARY, column-> value,
                           column-> length + 1, &column-> indic);
               if (feedback != SQL_SUCCESS)
                   get_db_error_message ();
               break;
           case DATA_TYPE_BIG_INT:
               break;
           case DATA_TYPE_BIT:
               SQLBindCol (h_statement, index++, SQL_C_BIT,
                           column-> value, 0 , &column-> indic);
               break;

           case DATA_TYPE_UNICODE_CHAR:
           case DATA_TYPE_UNICODE_VARCHAR:
           case DATA_TYPE_UNICODE_TEXT:
               feedback = SQLBindCol (h_statement, index++, SQL_C_WCHAR, column-> value,
                           column-> length + UCODE_SIZE,
                           &column-> indic);
               if (feedback != SQL_SUCCESS)
                   get_db_error_message ();
               break;
           default:
               coprintf ("Error: Invalid data type for column %s\n", column-> name);
               break;
         }
       column = next;
     }
}


/*  -------------------------------------------------------------------------
    Function: allocate_column_value

    Synopsis: Allocate data buffer in col_defn structure.
    -------------------------------------------------------------------------*/

void
allocate_column_value (LIST *col_list)
{
    COL_DEFN
        *column,
        *next;

    column = (COL_DEFN *)col_list-> next;
    while ((void *)column != (void *)col_list)
      {
        next = column-> next;
        switch (column-> data_type)
          {
            case DATA_TYPE_CHAR:
            case DATA_TYPE_VARCHAR:
            case DATA_TYPE_LONG_VARCHAR:
                column-> value = (void *)mem_alloc (column-> length + 1);
                break;
            case DATA_TYPE_NUMERIC:
            case DATA_TYPE_DECIMAL:
                if (column-> scale == 0)
                  {
                    if (column-> precision <= 9)
                        column-> value = (void *)mem_alloc (sizeof (long));
                    else
                      {
                        if (dbms_type != DBMS_TYPE_ORACLE)
                            column-> value = (void *)mem_alloc (sizeof (_int64));
                        else
                            column-> value = (void *)mem_alloc (sizeof (double));
                      }
                  }
                else
                    column-> value = (void *)mem_alloc (sizeof (double));
                break;
            case DATA_TYPE_INTEGER:
                column-> value = (void *)mem_alloc (sizeof (long));
                break;
           case DATA_TYPE_TINY_INT:
           case DATA_TYPE_SMALL_INT:
               column-> value = (void *)mem_alloc (sizeof (short));
               break;
           case DATA_TYPE_FLOAT:
           case DATA_TYPE_REAL:
           case DATA_TYPE_DOUBLE:
               column-> value = (void *)mem_alloc (sizeof (double));
               break;
           case DATA_TYPE_DATE:
               column-> value = (void *)mem_alloc (DATE_SIZE + 1);
               break;
           case DATA_TYPE_TIME:
               column-> value = (void *)mem_alloc (TIME_SIZE + 1);
               break;
           case DATA_TYPE_TIME_STAMP:
               column-> value = (void *)mem_alloc (TIMESTAMP_SIZE + 1);
               break;
           case DATA_TYPE_BINARY:
               break;
           case DATA_TYPE_VARBINARY:
               break;
           case DATA_TYPE_LONG_VARBINARY:
               column-> value = (void *)mem_alloc (column-> length + 1);
               break;
           case DATA_TYPE_BIG_INT:
               column-> value = (void *)mem_alloc (sizeof (long int));
               break;
           case DATA_TYPE_BIT:
               column-> value = (void *)mem_alloc (sizeof (unsigned char));
               break;
           case DATA_TYPE_UNICODE_CHAR:
           case DATA_TYPE_UNICODE_VARCHAR:
           case DATA_TYPE_UNICODE_TEXT:
               column-> value = unicode_alloc (column-> length + 1);
               break;
         }
       column = next;
     }
}


/*  -------------------------------------------------------------------------
    Function: set_column_value

    Synopsis: Set the column value.
    -------------------------------------------------------------------------*/

void
set_column_value (COL_DEFN *column, char *value, Bool encoded_data)
{
    char
        *buf,
        *data;
    UCODE
        *uvalue,
        *udata;

    if (column-> value)
     {
       mem_free (column-> value);
       column-> value = NULL;
     }
   if (value == NULL)
     {
       column-> indic = -1;
       return;
     }
   else
       column-> indic = 0;

   switch (column-> data_type)
     {
       case DATA_TYPE_CHAR:
       case DATA_TYPE_VARCHAR:
       case DATA_TYPE_LONG_VARCHAR:
           data = value;
           http_decode_meta (value, &data, strlen (value) + 1);
           data = value;
           buf  = buffer;
           while (*data)
             {
               if (*data == '\'')
                   *buf++ = '\'';
               *buf++ = *data++;
             }
           *buf = '\0';
           if (encode_data)
             {
               buf = mem_strdup (buffer);
               if (buf)
                 {
                   data = buf;
                   http_encode_meta (buffer, &data, BUFFER_SIZE, TRUE);
                   mem_free (buf);
                 }
             }
           column-> value = mem_strdup (buffer);
           break;
       case DATA_TYPE_NUMERIC:
               column-> value = (void *)mem_alloc (column-> length + 1);

           if (column-> scale == 0)
             {
               if (column-> value)
                 {
                   if (column-> precision <= 9)
                       *((long *)column-> value) = (long)atol (value);
                   else
                     {
                       if (dbms_type != DBMS_TYPE_ORACLE)                      
                           *((_int64 *)column-> value) = (_int64)_atoi64 (value);
                       else
                           *((double *)column-> value) = (double)atof (value);
                     }
                 }
             }
           else
             {
               // XXX
               if (column-> value)
                   *((double *)column-> value) = (double)atof (value);
             }
           break;
       case DATA_TYPE_INTEGER:
               column-> value = (void *)mem_alloc (sizeof (long));
               if (column-> value)
                   *((long *)column-> value) = (long)atol (value);
           break;
       case DATA_TYPE_TINY_INT:
       case DATA_TYPE_SMALL_INT:
               column-> value = (void *)mem_alloc (sizeof (short));
               if (column-> value)
                   *((short *)column-> value) = (short)atoi (value);
           break;
       case DATA_TYPE_FLOAT:
       case DATA_TYPE_REAL:
       case DATA_TYPE_DOUBLE:
       case DATA_TYPE_DECIMAL:
           column-> value = (void *)mem_alloc (column-> length + 1);
           if (column-> value)
             {
               //XXX
               *((double *)column-> value) = (double)atof (value);
             }
           break;
       case DATA_TYPE_DATE:
           column-> value = mem_strdup (value);
           break;
       case DATA_TYPE_TIME:
           column-> value = mem_strdup (value);
           break;
       case DATA_TYPE_TIME_STAMP:
           column-> value = mem_strdup (value);
           break;
       case DATA_TYPE_BINARY:
           break;
       case DATA_TYPE_VARBINARY:
           break;
       case DATA_TYPE_LONG_VARBINARY:
           break;
       case DATA_TYPE_BIG_INT:
           break;
       case DATA_TYPE_BIT:
           *(unsigned char *)column-> value = *(unsigned char *)value;
           break;
       case DATA_TYPE_UNICODE_CHAR:
       case DATA_TYPE_UNICODE_VARCHAR:
       case DATA_TYPE_UNICODE_TEXT:
           uvalue = ascii2ucode (value);
           if (uvalue)
             {
               udata = uvalue;
               http_decode_umeta (uvalue, &udata, wcslen (uvalue) + 1);
               column-> value = mem_ustrdup ((UCODE *)uvalue);
               mem_free (uvalue);
             }
           break;
     }
}

/*  -------------------------------------------------------------------------
    Function: execute_select

    Synopsis: Execute a select request file.
    -------------------------------------------------------------------------*/

void
execute_select_request (char *select_request)
{
    XML_ITEM
        *select = NULL,
        *col    = NULL;
    char
        *table_list = NULL,
        *where      = NULL,
        *order_by   = NULL,
        *column_str = NULL;
    LIST
        select_col;
    COL_DEFN
        *column     = NULL;

    ASSERT (select_request);

    xml_load_file (&xml_item, ".", select_request, FALSE);
    if (xml_item == NULL)
        return;

    list_reset (&select_col);

    FORCHILDREN (select, xml_item)
      {
        table_list = xml_get_attr (select, "table",    NULL);
        where      = xml_get_attr (select, "where",    NULL);
        order_by   = xml_get_attr (select, "order_by", NULL);
        if (table_list)
          {
            load_col_from_table_list (table_list);
            FORCHILDREN (col, select)
              {
                column_str = xml_get_attr (col, "name", NULL);
                if (column_str)
                  {
                    column = get_select_col (&columns, column_str);
                    if (column)
                        list_relink_before (column, &select_col);
                    else
                        fprintf (out_file, "Invalid column name\n", column_str);
                  }
              }
            fputs ("<result>", out_file);
            exec_select (&select_col, table_list, where, order_by);
            fputs ("</result>", out_file);
            free_all_columns_name (&select_col);
            free_all_columns_name (&columns);
          }
        else
            fprintf (out_file, "Missing table definition\n");
      }

    xml_free (xml_item);
}


/*  -------------------------------------------------------------------------
    Function: load_col_from_table_list

    Synopsis: Load all column of table in form clause of select statement.
    -------------------------------------------------------------------------*/

void
load_col_from_table_list (char *table_list)
{
    char
        **tab,
        *buffer;
    int
        index;

    buffer = mem_strdup (table_list);
    if (buffer == NULL)
        return;
    strconvch (buffer, ',', ' ');
    tab = tok_split (buffer);
    if (tab)
      {
        for (index = 0; tab [index]; index++)
            get_all_columns_definition (tab [index]);
        tok_free (tab);
      }

    mem_free (buffer);
}


/*  -------------------------------------------------------------------------
    Function: get_select_col

    Synopsis: Search for a column definition in current list. if find, make
              a copy of this.
    -------------------------------------------------------------------------*/

COL_DEFN *
get_select_col (LIST * col_list, char *col_name)
{
    char
        *table,
        *col,
        *buffer;
    COL_DEFN
        *column,
        *next,
        *new_col = NULL;

    ASSERT (col_name);

    buffer = mem_strdup (col_name);
    if (buffer == NULL)
        return (NULL);

    /* Search if column name is 'table_name.column'                          */
    col =  strchr (buffer, '.');
    if (col)
      {
        *col++ = '\0';
        table  = buffer;
      }
    else
      {
        col   = buffer;
        table = NULL;
      }

   column = (COL_DEFN *)col_list-> next;
   while ((void *)column != (void *)col_list)
     {
       next = column-> next;
       if (lexcmp (column-> name, col) == 0
       &&  (!table || lexcmp (table, column-> table_name) == 0))
         {
           new_col = copy_col_def (column);
           next = (COL_DEFN *)col_list;
         }
       column = next;
     }
    mem_strfree (&buffer);

    return (new_col);
}


/*  -------------------------------------------------------------------------
    Function: execute_sql_file

    Synopsis: Execute SQL statements in a file. All statements are delimited
    by ';', not newline. The file can contain multiple SQL statements.
    -------------------------------------------------------------------------*/

void
execute_sql_file (char *sql_file)
{
    FILE
        *fd;
    int
        statement_nb = 0,
        read_size;
    Bool
        end_flag = FALSE;

    ASSERT (sql_file);


    fd = fopen (sql_file, "rb");
    coprintf  ("<result file=\"%s\">\n", sql_file);
    if (fd == NULL)
        fprintf (out_file, "<error value=\"Request file %s not exist\" />\n", sql_file);

    memset (buffer, 0, BUFFER_SIZE);


    while (end_flag == FALSE)
      {
        read_size = -1;
        do
          {
            read_size++;
            buffer [read_size] = getc (fd);
            if (buffer [read_size] == EOF)
                end_flag = TRUE;
            if (buffer [read_size] == '\r'
            ||  buffer [read_size] == '\n')
                read_size--;
          } while (end_flag == FALSE
            &&     buffer [read_size] != ';');

        if (read_size > 0)
          {
            buffer [read_size] = '\0';
            trim (buffer);

            SQLFreeStmt (h_statement, SQL_UNBIND);
            SQLFreeStmt (h_statement, SQL_CLOSE);

            statement_nb++;
            return_code = SQLExecDirect (h_statement, buffer, SQL_NTS);
            if (return_code == SQL_SUCCESS
            ||  return_code == SQL_SUCCESS_WITH_INFO)
                fprintf (out_file, "<statement number=\"%d\" status=\"OK\" />\n",
                          statement_nb);
            else
              {
                get_db_error_message ();
                fprintf (out_file, "<statement number=\"%d\" status=\"ERROR\" value=\"%s\" statement=\"%s\"/>\n",
                          statement_nb,
                          message,
                          buffer);

              }
          }
      }
    fputs ("</result>", out_file);

    fclose (fd);
}


/*  -------------------------------------------------------------------------
    Function: review_table

    Synopsis: Load and review table structure.
    -------------------------------------------------------------------------*/

void
review_table (char *dbml_file)
{
    XML_ITEM
        *child = NULL;
    LIST
        col_list;
    char
        *table_name = NULL;

    ASSERT (dbml_file);

    get_data_type_info ();
    get_all_table_name ();

    xml_load_file (&xml_item, ".", dbml_file, FALSE);

    if (xml_item == NULL)
        return;

    FORCHILDREN (child, xml_item)
      {
        if (streq (xml_item_name (child), "table"))
          {
            table_name = update_table_struct (child, &col_list);
            break;
          }
      }

    free_all_columns_name (&col_list);
    xml_free (xml_item);

}


/*  -------------------------------------------------------------------------
    Function: update_table_struct

    Synopsis: Update or create table.
    -------------------------------------------------------------------------*/

char *
update_table_struct  (XML_ITEM *xml_item, LIST *column_list)
{
    char
        *table_name = NULL;
    COL_DEFN
        *col        = NULL,
        *next       = NULL,
        *column     = NULL;

    table_name = get_dbml_struct (xml_item, column_list);

    if (table_name)
      {
        if (get_all_columns_definition (table_name) == 0)
          {
            if (create_table (table_name, column_list) == FALSE)
                table_name = NULL;
          }
        else
          {
            /* Remove column not in table                                    */
            col = column_list-> next;
            while ((void *)col != (void *)column_list)
              {
                next = col-> next;
                column = get_select_col (&columns, col-> name);
                if (column == NULL)
                    free_column (col);
                else
                  {
                    col-> nullable  = column-> nullable;
                    col-> data_type = column-> data_type;
                    col-> length    = column-> length;
                    col-> scale     = column-> scale;
                    col-> in_primary_key = column-> in_primary_key;
                    free_column (column);
                  }
                col = next;
              }
            /* Add new numeric column to add 0 and not nullable column       */
            col = columns.next;
            while ((void *)col != (void *)&columns)
              {
                next = col-> next;
                if (col-> nullable == FALSE
                ||  force2zero && (
                    col-> data_type == DATA_TYPE_NUMERIC
                ||  col-> data_type == DATA_TYPE_DECIMAL
                ||  col-> data_type == DATA_TYPE_INTEGER
                ||  col-> data_type == DATA_TYPE_SMALL_INT
                ||  col-> data_type == DATA_TYPE_FLOAT
                ||  col-> data_type == DATA_TYPE_REAL
                ||  col-> data_type == DATA_TYPE_DOUBLE
                ||  col-> data_type == DATA_TYPE_BIG_INT
                ||  col-> data_type == DATA_TYPE_TINY_INT
                ||  col-> data_type == DATA_TYPE_BIT)
                   )
                  {
                    column = get_select_col (column_list, col-> name);
                    if (column == NULL)
                      {
                        column = copy_col_def (col);
                        if (column)
                            list_relink_before (column, column_list);                            
                      }
                    else
                        free_column (column);    
                  }
                col = next;
              }
          }
        free_all_columns_name (&columns);
      }
    return (table_name);
}


/*  -------------------------------------------------------------------------
    Function: get_dbml_struct

    Synopsis: Create a column list from a dbml file.
    -------------------------------------------------------------------------*/

char *
get_dbml_struct (XML_ITEM *xml_item, LIST *col_list)
{
    static char
         table_name [50];
    char
         *c_value = NULL;
    short
         s_value;
    XML_ITEM
       *field = NULL;
    COL_DEFN
       *col   = NULL;

    ASSERT (xml_item);
    ASSERT (col_list);

    memset (table_name, 0, sizeof (table_name));
    list_reset (col_list);

    if (strneq (xml_item_name (xml_item), "table"))
        return (NULL);

    strcpy (table_name, xml_get_attr (xml_item, "name", ""));

    FORCHILDREN (field, xml_item)
      {
        if (streq (xml_item_name (field), "field"))
          {
            col = mem_alloc (sizeof (COL_DEFN));
            if (col)
              {
                memset (col, 0, sizeof (COL_DEFN));
                list_reset (col);
                col-> name      = mem_strdup (xml_get_attr (field, "name", ""));
                s_value = atoi (xml_get_attr (field, "size", "0"));
                col->precision = s_value;
                s_value = atoi (xml_get_attr (field, "decimal", "0"));
                col-> scale = s_value;
                c_value = xml_get_attr (field, "nulls", "N");
                if (*c_value == 'y' || *c_value == 'Y')
                    col-> nullable = TRUE;
                col-> data_type = get_odbc_type (xml_get_attr (field, "type",  NULL),
                                                 xml_get_attr (field, "ntype", NULL),
                                                 col);
                list_relink_before (col, col_list);
              }
          }
      }
    return (table_name);
}


#define SEARCH_TYPE(type)                                          \
                if (col-> type_name == NULL)                       \
                  {                                                \
                    type_name = search_type_in_list (type);        \
                    if (type_name)                                 \
                      {                                            \
                        odbc_type       = type;                    \
                        col-> data_type = type;                    \
                        col-> type_name = mem_strdup (type_name);  \
                      }                                            \
                  }

/*  -------------------------------------------------------------------------
    Function: get_odbc_type

    Synopsis: Get type of field.
    -------------------------------------------------------------------------*/

short
get_odbc_type (char *type, char *native_type, COL_DEFN *col)
{
    TYPE_DEFN
        *type_def,
        *next;
    short
        odbc_type = 9999;
    char
        *type_name;


    ASSERT (col);

    if (type == NULL)
        return (odbc_type);

    if (lexcmp (type, "textual") == 0)
      {
        if (col-> precision <= 255)
          {
            SEARCH_TYPE (DATA_TYPE_CHAR);
            SEARCH_TYPE (DATA_TYPE_VARCHAR);
          }
        else
          {
            SEARCH_TYPE (DATA_TYPE_VARCHAR);
            SEARCH_TYPE (DATA_TYPE_LONG_VARCHAR);
          }
      }
    else
    if (lexcmp (type, "numeric") == 0)
      {
        if (col-> scale > 0)
          {
            SEARCH_TYPE (DATA_TYPE_FLOAT);
            SEARCH_TYPE (DATA_TYPE_REAL);
            SEARCH_TYPE (DATA_TYPE_DOUBLE);
            SEARCH_TYPE (DATA_TYPE_DECIMAL);
          }
        else
          {
            if (col-> precision <= 5)
              {
                SEARCH_TYPE (DATA_TYPE_TINY_INT);
                SEARCH_TYPE (DATA_TYPE_SMALL_INT);
              }
            SEARCH_TYPE (DATA_TYPE_NUMERIC);
            SEARCH_TYPE (DATA_TYPE_INTEGER);
          }
      }
    else
    if (lexcmp (type, "date") == 0)
      {
        SEARCH_TYPE (DATA_TYPE_DATE);
      }
    else
    if (lexcmp (type, "time") == 0)
      {
        SEARCH_TYPE (DATA_TYPE_TIME);
      }
    else
    if (lexcmp (type, "timestamp") == 0)
      {
        SEARCH_TYPE (DATA_TYPE_TIME_STAMP);
      }
    else
    if (lexcmp (type, "unicode_textual") == 0)
      {
        if (col-> precision <= 255)
          {
            SEARCH_TYPE (DATA_TYPE_UNICODE_CHAR);
            SEARCH_TYPE (DATA_TYPE_UNICODE_VARCHAR);
          }
        else
          {
            SEARCH_TYPE (DATA_TYPE_UNICODE_VARCHAR);
            SEARCH_TYPE (DATA_TYPE_UNICODE_TEXT);
          }
      }

    if (odbc_type == 9999
    &&  native_type)
      {
        type_def = (TYPE_DEFN *)type_list.next;
        while ((void *)type_def != (void *)&type_list)
          {
            next = type_def-> next;
            if (lexcmp (type_def-> type_name, native_type) == 0)
              {
                odbc_type = type_def-> data_type;
                col-> type_name = mem_strdup (type_def-> type_name);
                next = (TYPE_DEFN *)&type_list;
              }
            type_def = next;
          }
      }
    return (odbc_type);
}


/*  -------------------------------------------------------------------------
    Function: search_type_in_list

    Synopsis: search a type item on type list for a odbc type value.
              Return NULL if not found.
    -------------------------------------------------------------------------*/

char *
search_type_in_list (short odbc_type)
{
    TYPE_DEFN
        *type,
        *next;
    char
        *feedback = NULL;
        type = (TYPE_DEFN *)type_list.next;

    while ((void *)type != (void *)&type_list)
      {
        next = type-> next;
        if (type-> data_type == odbc_type)
          {
            feedback = type-> type_name;
            next = (TYPE_DEFN *)&type_list;
          }
        type = next;
      }

    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: create_table

    Synopsis: Create table from a column list.
    -------------------------------------------------------------------------*/

Bool
create_table (char *table_name, LIST *col_list)
{
    COL_DEFN
        *col,
        *next;
    Bool
        feedback = FALSE;

    ASSERT (table_name);
    ASSERT (col_list);

    sprintf (buffer, "CREATE TABLE %s (", table_name);
    col = (COL_DEFN *)col_list-> next;
    while ((void *)col != (void *)col_list)
      {
        next = col-> next;
        sprintf (&strterm (buffer), "%s %s ", col-> name, col-> type_name);
        if (col-> data_type == DATA_TYPE_CHAR
        ||  col-> data_type == DATA_TYPE_VARCHAR)
            sprintf (&strterm (buffer), "(%d) ", col-> precision);
        else
        if (col-> data_type == DATA_TYPE_UNICODE_CHAR
        ||  col-> data_type == DATA_TYPE_UNICODE_VARCHAR)
            sprintf (&strterm (buffer), "(%d) ", col-> precision / UCODE_SIZE);
        else
        if (col-> data_type == DATA_TYPE_DECIMAL
        ||  col-> data_type == DATA_TYPE_NUMERIC)
            sprintf (&strterm (buffer), "(%d, %d) ",
                     col-> precision, col-> scale);

        if (!col-> nullable)
           strcat (&strterm (buffer), "NOT NULL");
        sprintf (&strterm (buffer), "%c",
                 ((void *)next != (void *)col_list)? ',':')');
        col = next;
      }
    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);

    return_code = SQLExecDirect (h_statement, buffer, SQL_NTS);
    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
        feedback = TRUE;
    else
      {
        get_db_error_message ();
        fputs (message, out_file);
      }
    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: import_all_tables

    Synopsis: Import all tables.
    -------------------------------------------------------------------------*/

int
import_all_tables (void)
{
    TABLE_DEFN
        *table;

    table = table_list.next;

    while ((void *)table != (void *)&table_list)
      {
        if (streq (table-> type, "TABLE"))
            if (new_import_table (table-> name) != 0)
                return -1;              /* we break on first error           */
        table = table-> next;
      }

    return 0;
}


/*  -------------------------------------------------------------------------
    Function: import_table

    Synopsis: Import a table file name. Create table if required.
    -------------------------------------------------------------------------*/

void
import_table (char *table)
{
    XML_ITEM
        *xml_item    = NULL,
        *first_child = NULL,
        *child       = NULL;
    LIST
        col_list;
    char
        file_name [256],
        *table_name = NULL;
    static int
        count = 0;
    long
        recno;
    int
        feedback;
    Bool
        data_is_encoded;


    ASSERT (table);

    sprintf (file_name, "%s.xml", table);
    fprintf (out_file, "<table name=\"%s\">\n", table);

    feedback = xml_load_file (&xml_item, ".", file_name, FALSE);

    if (xml_item == NULL)
        return;

    if (feedback != XML_NOERROR)
      {
        fprintf (out_file, "<error type = \"load import file\" description = \"%s\"/>\n",
                  xml_error ());
        xml_free (xml_item);
        return;
      }
    list_reset (&col_list);
    count = 0;
    recno = 0;
    first_child = xml_first_child (xml_item);
    if (first_child)
      {
        data_is_encoded = conv_str_bool (xml_get_attr (first_child, "encode", "0"));
        FORCHILDREN (child, first_child)
          {
            if (lexcmp (xml_item_name (child), "table") == 0)
              {
                table_name = update_table_struct (child, &col_list);
                if (table_name == NULL)
                  {
                    get_db_error_message ();
                    fprintf (out_file, "<error ROW=\"%ld\" CREATE_TABLE=\"TRUE\" MESSAGE=\"%s\" />\n",
                              recno, message);
                  }
                if (have_identity)
                  {
                    SQLFreeStmt (h_statement, SQL_UNBIND);
                    SQLFreeStmt (h_statement, SQL_CLOSE);
                    sprintf (file_name, "SET IDENTITY_INSERT %s ON", table_name);
                    return_code = SQLExecDirect (h_statement, file_name, SQL_NTS);
                    if (return_code != SQL_SUCCESS
                    &&  return_code != SQL_SUCCESS_WITH_INFO)
                        get_db_error_message ();
                  }
                SQLFreeStmt (h_statement, SQL_UNBIND);
                SQLFreeStmt (h_statement, SQL_CLOSE);
              }
            else
              {
                if (import_row (table_name, child, &col_list, data_is_encoded))
                  {
                    count++;
                    recno++;
                    if (count == AUTO_COMMIT_COUNT)
                      {
                        dbio_commit ();
                        count = 0;
                      }
                  }
                else
                  {
                    get_db_error_message ();
                    fprintf (out_file, "<error row=\"%ld\" message=\"%s\" />\n",
                              recno + 1, message);
                  }
              }
          }
        if (have_identity)
          {
            SQLFreeStmt (h_statement, SQL_UNBIND);
            SQLFreeStmt (h_statement, SQL_CLOSE);
            sprintf (file_name, "SET IDENTITY_INSERT %s OFF", table_name);
            return_code = SQLExecDirect (h_statement, file_name, SQL_NTS);
            if (return_code != SQL_SUCCESS
            &&  return_code != SQL_SUCCESS_WITH_INFO)
                get_db_error_message ();
          }
      }
    fprintf (out_file, "<nb_row value=\"%ld\" />\n", recno);
    fputs ("</table>", out_file);
    free_all_columns_name (&col_list);
    xml_free (xml_item);
    fflush (out_file);
}

Bool
    have_unicode = FALSE;
/*  -------------------------------------------------------------------------
    Function: import_row

    Synopsis: Import a row of table file name.
    -------------------------------------------------------------------------*/

Bool
import_row (char *table_name, XML_ITEM *xml_item, LIST *col_list, Bool encoded_data)
{
    COL_DEFN
        *last_update = NULL,
        *last  = NULL,
        *col   = NULL,
        *next  = NULL;
    char
        *current_pos,
        *value;
    Bool
        feedback     = FALSE;
    dbyte
        field_count;

    have_unicode = FALSE;

    col = (COL_DEFN *)col_list-> next;
    while ((void*)col != (void *)col_list)
      {
        next  = col-> next;
        value = xml_get_attr (xml_item, col-> name, NULL);
        set_column_value (col, value, encoded_data);
        if (IS_VALID_COL)
            last = col;

        if (incremental_import
        &&  col-> in_primary_key == FALSE
        &&     (IS_VALID_COL
            ||  IS_VALID_COL_NULL
            ||  IS_VALID_COL_ZERO))
                last_update = col;
        col = next;
      }

    if (incremental_import)
      {
        feedback = update_row (col_list, last_update, table_name);
        if (feedback)
            return (feedback);
      }

    sprintf (buffer, "INSERT INTO %s (", table_name);
    current_pos = buffer + strlen (buffer);
    col = (COL_DEFN *)col_list-> next;
    while ((void*)col != (void *)col_list)
      {
        next = col-> next;
        if (IS_VALID_COL)
          {
            sprintf (current_pos, "%s%c ", col-> name, (col == last)? ')': ',');
            current_pos += strlen (current_pos);
          }
        col = next;
      }
    sprintf (current_pos,"VALUES (");
    current_pos += strlen (current_pos);

    col = (COL_DEFN *)col_list-> next;
    while ((void*)col != (void *)col_list)
      {
        next = col-> next;
        
        value = value2string (col, last);
        if (*value)
          {
            sprintf (current_pos,"%s %c ", value, (col == last)? ')': ',');
            current_pos += strlen (current_pos);
          }
        col = next;
      }
    if (have_unicode == TRUE)
      {
        return_code = SQLPrepare (h_statement, buffer, SQL_NTS);
        if (return_code == SQL_SUCCESS
        ||  return_code == SQL_SUCCESS_WITH_INFO)
          {
            field_count = 1;
            col = (COL_DEFN *)col_list-> next;
            while ((void*)col != (void *)col_list)
              {
                next = col-> next;
                if (col-> value != NULL
                &&  col-> indic >= 0)
                  {
                    switch (col-> data_type)
                      {
                        case DATA_TYPE_UNICODE_CHAR:
                            return_code = SQLBindParameter (h_statement, field_count++, SQL_PARAM_INPUT,
                                              SQL_C_WCHAR,  SQL_WCHAR, col-> length / 2, 0,
                                              col-> value, 0, NULL);
                            break;
                        case DATA_TYPE_UNICODE_VARCHAR:
                        case DATA_TYPE_UNICODE_TEXT:
                            return_code = SQLBindParameter (h_statement, field_count++, SQL_PARAM_INPUT,
                                              SQL_C_WCHAR,  SQL_WVARCHAR, col-> length / 2, 0,
                                              col-> value, 0, NULL);
                            break;
                      }
                  }
                col = next;
              }
            return_code = SQLExecute (h_statement);
          }
      }
    else
        return_code = SQLExecDirect (h_statement, buffer, SQL_NTS);

    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
        feedback = TRUE;
/*    else
{
strcat (buffer, "\n");
OutputDebugString (buffer);
        get_db_error_message ();
}*/

    return (feedback);
}


static void 
set_field_primary_key_flag (LIST *col_list)
{
    COL_DEFN
        *col   = NULL,
        *next  = NULL;
    IDX_DEFN
        *idx,
        *idx_next;

   idx = (IDX_DEFN *)index_list.next;
   while ((void *)idx != (void *)&index_list)
     {
       idx_next = idx-> next;
       if (idx-> index_name 
       &&  streq (idx-> index_name, primary_key))
         {
           col = (COL_DEFN *)col_list-> next;
           while ((void*)col != (void *)col_list)
             {
               next = col-> next;
               if (lexcmp (col->name, idx->column_name) == 0)
                 {
                   col-> in_primary_key = TRUE;
                   break;
                 }
               col = next;
             }
         }
       idx = idx_next;
     }
}

Bool
update_row (LIST *col_list, COL_DEFN *last, char *table_name)
{
    Bool
        feedback = FALSE;
    char
        *separator = "",
        *value,
        *current_pos;
    COL_DEFN
        *real_last,
        *col   = NULL,
        *next  = NULL;
    dbyte
        field_count;

    have_unicode = FALSE;

    if (primary_key == NULL)
        return (FALSE);

    current_pos = buffer;
    sprintf (buffer, "UPDATE %s SET ", table_name);
    current_pos += strlen (current_pos);

    real_last = last;
    while (real_last-> in_primary_key == TRUE)
        real_last = real_last-> prev;

    col = (COL_DEFN *)col_list-> next;
    while ((void*)col != (void *)col_list)
      {
        next = col-> next;
        if (col-> in_primary_key == FALSE)
          {
            if (IS_VALID_COL)
              {
                sprintf (current_pos,"%s = %s %c ",
                     col-> name,
                     value2string (col, real_last), 
                     (col == real_last)? ' ': ',');
              }
            else
            if (IS_VALID_COL_NULL)
              {
                sprintf (current_pos,"%s = NULL %c ",
                         col-> name,
                         (col == real_last)? ' ': ',');
              }
            else
            if (IS_VALID_COL_ZERO)
              {
                sprintf (current_pos,"%s = 0 %c ",
                     col-> name,
                     (col == real_last)? ' ': ',');
              }    
        current_pos += strlen (current_pos);
          }
        col = next;
      }

   sprintf (current_pos, " WHERE ");
   current_pos += strlen (current_pos);

   col = (COL_DEFN *)col_list-> next;
   while ((void*)col != (void *)col_list)
     {
       next = col-> next;
       if (col-> in_primary_key)
         {
           value = value2wherestring (col, real_last);
           if (*value)
             {
               sprintf (current_pos,"%s%s = %s",
                                     separator,
                                     col-> name,
                                     value);
                       current_pos += strlen (current_pos);
                       separator = " AND ";
             }
          }
        col = next;
     }


   if (have_unicode == TRUE)
      {
        return_code = SQLPrepare (h_statement, buffer, SQL_NTS);
        if (return_code == SQL_SUCCESS
        ||  return_code == SQL_SUCCESS_WITH_INFO)
          {
            field_count = 1;
            col = (COL_DEFN *)col_list-> next;
            while ((void*)col != (void *)col_list)
              {
                next = col-> next;
                if (IS_VALID_COL && col-> in_primary_key == FALSE)
                  {
                    switch (col-> data_type)
                      {
                        case DATA_TYPE_UNICODE_CHAR:
                            return_code = SQLBindParameter (h_statement, field_count++, SQL_PARAM_INPUT,
                                              SQL_C_WCHAR,  SQL_WCHAR, col-> length / 2, 0,
                                              col-> value, 0, NULL);
                            break;
                        case DATA_TYPE_UNICODE_VARCHAR:
                        case DATA_TYPE_UNICODE_TEXT:
                            return_code = SQLBindParameter (h_statement, field_count++, SQL_PARAM_INPUT,
                                              SQL_C_WCHAR,  SQL_WVARCHAR, col-> length / 2, 0,
                                              col-> value, 0, NULL);
                            break;
                      }
                  }
                col = next;
              }
            col = (COL_DEFN *)col_list-> next;
            while ((void*)col != (void *)col_list)
              {
                next = col-> next;
                if (col-> in_primary_key)
                  {
                    if (col-> value != NULL
                    &&  col-> indic >= 0)
                      {
                        switch (col-> data_type)
                          {
                            case DATA_TYPE_UNICODE_CHAR:
                                return_code = SQLBindParameter (h_statement, field_count++, SQL_PARAM_INPUT,
                                                   SQL_C_WCHAR,  SQL_WCHAR, col-> length / 2, 0,
                                                   col-> value, 0, NULL);
                                break;
                            case DATA_TYPE_UNICODE_VARCHAR:
                            case DATA_TYPE_UNICODE_TEXT:
                                return_code = SQLBindParameter (h_statement, field_count++, SQL_PARAM_INPUT,
                                                  SQL_C_WCHAR,  SQL_WVARCHAR, col-> length / 2, 0,
                                                  col-> value, 0, NULL);
                                break;
                          }
                      }
                  }
                col = next;
              }
            return_code = SQLExecute (h_statement);
          }
      }
    else
        return_code = SQLExecDirect (h_statement, buffer, SQL_NTS);

    if (return_code == SQL_SUCCESS
    ||  return_code == SQL_SUCCESS_WITH_INFO)
        feedback = TRUE;
/*    else
{
strcat (buffer, "\n");
OutputDebugString (buffer);
        get_db_error_message ();
}*/
    return (feedback);
}


char *
value2string (COL_DEFN *col, COL_DEFN *last)
{
    static char
        buffer [BUFFER_SIZE + 1];

    *buffer = '\0';
    if (col-> value 
    &&  col-> indic >= 0)
      {
        switch (col-> data_type)
          {
            case DATA_TYPE_CHAR:
            case DATA_TYPE_VARCHAR:
            case DATA_TYPE_LONG_VARCHAR:
            case DATA_TYPE_DATE:
            case DATA_TYPE_TIME:
            case DATA_TYPE_TIME_STAMP:
                sprintf (buffer,"'%s'", (char *)col-> value);
                break;
            case DATA_TYPE_NUMERIC:
                if (col-> scale == 0)
                  {
                    if (col-> precision <= 9)
                        sprintf (buffer,"%ld", *(long *)col-> value);
                    else
                        sprintf (buffer,"%I64d", *(_int64 *)col-> value);
                  }
                else
                    sprintf (buffer,"%f", *(double *)col-> value);
                break;
            case DATA_TYPE_INTEGER:
                sprintf (buffer,"%ld", *(long *)col-> value);
                break;
            case DATA_TYPE_TINY_INT:
            case DATA_TYPE_SMALL_INT:
                sprintf (buffer,"%d", *(short *)col-> value);
                break;
            case DATA_TYPE_FLOAT:
            case DATA_TYPE_REAL:
            case DATA_TYPE_DOUBLE:
            case DATA_TYPE_DECIMAL:
                sprintf (buffer,"%f", *(double *)col-> value);
                break;
            case DATA_TYPE_BINARY:
            case DATA_TYPE_VARBINARY:
            case DATA_TYPE_LONG_VARBINARY:
            case DATA_TYPE_BIG_INT:
                break;
            case DATA_TYPE_BIT:
                sprintf (buffer,"%d", *(short *)col-> value);
                break;
            case DATA_TYPE_UNICODE_CHAR:
            case DATA_TYPE_UNICODE_VARCHAR:
            case DATA_TYPE_UNICODE_TEXT:
                have_unicode = TRUE;
                sprintf (buffer, "?");
                break;
          }
      }
    else
    if (IS_VALID_COL)
      {
        switch (col-> data_type)
          {
            case DATA_TYPE_CHAR:
            case DATA_TYPE_VARCHAR:
            case DATA_TYPE_LONG_VARCHAR:
            case DATA_TYPE_DATE:
            case DATA_TYPE_TIME:
            case DATA_TYPE_TIME_STAMP:
            case DATA_TYPE_UNICODE_CHAR:
            case DATA_TYPE_UNICODE_VARCHAR:
            case DATA_TYPE_UNICODE_TEXT:
                sprintf (buffer,"''");
                break;
            case DATA_TYPE_NUMERIC:
            case DATA_TYPE_DECIMAL:
            case DATA_TYPE_INTEGER:
            case DATA_TYPE_SMALL_INT:
            case DATA_TYPE_TINY_INT:
            case DATA_TYPE_FLOAT:
            case DATA_TYPE_REAL:
            case DATA_TYPE_DOUBLE:
            case DATA_TYPE_BIT:
                sprintf (buffer,"0");
                break;
            case DATA_TYPE_BINARY:
            case DATA_TYPE_VARBINARY:
            case DATA_TYPE_LONG_VARBINARY:
            case DATA_TYPE_BIG_INT:
                break;
          }
      }
    return (buffer);
}


char *
value2wherestring (COL_DEFN *col, COL_DEFN *last)
{
    static char
        *value,
        buffer [BUFFER_SIZE + 1];

    *buffer = '\0';
    if (col-> value 
    &&  col-> indic >= 0)
      {
        switch (col-> data_type)
          {
            case DATA_TYPE_DATE:
                sprintf (buffer,"{d '%s'}", (char *)col-> value);
                break;
            case DATA_TYPE_TIME:
                sprintf (buffer,"{t '%s'}", (char *)col-> value);
                break;
            case DATA_TYPE_TIME_STAMP:
                sprintf (buffer,"{ts '%s'}", (char *)col-> value);
                break;
            default:
                return (value2string (col, last));
                break;
          }
      }
    else
        return (value2string (col, last));
    return (buffer);
}


static
void print_database_summary (char *filename)
{
    TABLE_DEFN
        *table;
    XML_ITEM
        *root = NULL,
        *table_item;
    char
        *xmlname = NULL;
    int
        rec_count,
        table_count,
        rc;

    ASSERT (filename != NULL);

    printf ("Getting database tables summary...\n");

    root = xml_create ("summary", NULL);
    ASSERT (root != NULL);

    table_count = get_all_table_name ();

    table = table_list.next;
    while ((void *)table != (void *)&table_list)
      {
        if (streq (table-> type, "TABLE"))
          {
            table_item = xml_new (root, "table", NULL);
            ASSERT (table_item != NULL);

            get_all_columns_definition (table-> name);
            if (nb_columns > 0)
              {
                get_all_index_definition (table_name);
                rec_count = get_record_count (&columns, table-> name);
              }
            else
                rec_count = 0;
            free_all_columns_name (&columns);
            free_all_index_name   (&index_list);

            sprintf (buffer, "%i", rec_count);
            xml_put_attr (table_item, "name", table-> name);
            xml_put_attr (table_item, "count", buffer);
          }

        table = table-> next;
      }

    xmlname = mem_alloc (strlen(filename) + 10); /* +10: to be sure buf is long enough */
    default_extension (xmlname, filename, "xml");

    rc = xml_save_file (root, xmlname);
    if (rc != XML_NOERROR)
        fprintf (out_file, "Error while saving xml to '%s'", filename);

    xml_free (root);
    mem_free (xmlname);

}

#define TRACE(STR)   coprintf ("[TRACE %i] %s", __LINE__, STR)


static
long get_record_count (LIST *col_list, char *table)
{
    long res = 0;
    SDWORD
        indicator;
    HSTMT
        count_stmt    = NULL;          /* ODBC Statement handle             */

    ASSERT (col_list);
    ASSERT (table);

    return_code = SQLAllocStmt (h_connection, &count_stmt);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO)
      {
         printf ("Cannot create 'SELECT COUNT' statement\n");
         return -1;
      }

    sprintf (buffer, "SELECT COUNT(*) FROM %s", table);
    return_code = SQLExecDirect (count_stmt, buffer, SQL_NTS);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO)
      {
         get_db_error_message ();
         printf ("Cannot Exec: %s\n", message);
         return -1;
      }

    return_code = SQLBindCol (count_stmt, 1, SQL_C_LONG, &res, 0, &indicator);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO)
      {
         printf ("Cannot Bind Column\n");
         return -1;
      }
    return_code = SQLFetch (count_stmt);
    if (return_code != SQL_SUCCESS && return_code != SQL_SUCCESS_WITH_INFO)
      {
         get_db_error_message ();
         printf ("Cannot Fetch: %s\n", message);
         return -1;
      }

    SQLFreeStmt (count_stmt, SQL_DROP);

    return res;
}

void
drop_all_record (char *table_name)
{
    char
        sql [255];
    HSTMT
        drop_stmt    = NULL;           /* ODBC Statement handle             */

    return_code = SQLAllocStmt (h_connection, &drop_stmt);
    if (return_code != SQL_SUCCESS 
    &&  return_code != SQL_SUCCESS_WITH_INFO)
      {
         printf ("Cannot create 'DELETE ALL' statement\n");
         return;
      }

    sprintf (sql, "DELETE FROM %s", table_name);
    return_code = SQLExecDirect (drop_stmt, sql, SQL_NTS);
    if (return_code != SQL_SUCCESS 
    &&  return_code != SQL_SUCCESS_WITH_INFO)
      {
         get_db_error_message ();
         printf ("Cannot Exec: %s\n", message);
         return;
      }

    SQLFreeStmt (drop_stmt, SQL_DROP);

}

/*---------------------------------------------------------------------------*/
/* Get the table name from table list without case sensitive                 */

char *
get_real_table_name (LIST *table_list, char *table_name)
{
    char
        *feedback = NULL;
    TABLE_DEFN
        *table;

    table = table_list-> next;

    while ((void *)table != (void *)table_list)
      {
        if (lexcmp (table-> name, table_name) == 0)
          {
            feedback = table-> name;
            break;
          }
        table = table-> next;
      }

    return (feedback);
}


/*---------------------------------------------------------------------------*/
/* Get the primary key name from index list                                  */

char *
get_pk_name_from_list (char *table)
{
    char
        *backup   = NULL,
        *feedback = NULL;
    IDX_DEFN
        *idx,
        *idx_next;

    if (index_list.next == index_list.prev)
        get_all_index_definition (table);
    idx = (IDX_DEFN *)index_list.next;
    while ((void *)idx != (void *)&index_list)
      {
        idx_next = idx-> next;
        if (idx-> primary)
          {
            feedback = idx-> index_name;
            break;
          }
        else
        if (idx-> non_unique == FALSE
        &&  backup == NULL)
            backup = idx-> index_name;

        idx = idx_next;
      }

    if (feedback == NULL && backup)
        feedback = backup;

    return (feedback);
}


static int  
get_dbms_type (void)
{
    int
        feedback = 0;
    char
        dbms_name [255 + 1];
    short
        size = 0;

    SQLGetInfo (h_connection, SQL_DBMS_NAME, dbms_name, 255, &size);
    if (size > 0)
      {
        dbms_name [size] = '\0';
        strlwc (dbms_name);

        if (strstr (dbms_name, "oracle"))
            feedback = DBMS_TYPE_ORACLE;
        else
        if (strstr (dbms_name, "microsoft sql server"))
            feedback = DBMS_TYPE_MSSQL;
      }
  
    return (feedback);
}

void
print_dsn_list (void)
{
    RETCODE
        retcode;
    Bool
        feedback;

    retcode = SQLAllocEnv (&h_environ); /* Environment handle              */
    if (retcode == SQL_SUCCESS)
      {
        fprintf (out_file, "<datasources>\n");

        feedback = print_dsn (SQL_FETCH_FIRST_USER, "user");
        while (feedback)
            feedback = print_dsn (SQL_FETCH_NEXT, "user");

        feedback = print_dsn (SQL_FETCH_FIRST_SYSTEM, "system");
        while (feedback)
            feedback = print_dsn (SQL_FETCH_NEXT, "system");

        fprintf (out_file, "</datasources>\n");

        SQLFreeEnv (&h_environ);
      }
}

Bool
print_dsn (dbyte direction, char *type)
{
    Bool
        feedback = FALSE;
    char
        dsn         [256],
        description [256],
        buffer1     [512],
        buffer2     [50];
    short
        dsn_size,
        desc_size;
    RETCODE
        retcode;

    retcode = SQLDataSources (h_environ, direction, dsn, 255, &dsn_size,
                              description, 255, &desc_size);

    if (retcode == SQL_SUCCESS
    ||  retcode == SQL_SUCCESS_WITH_INFO)
      {
         sprintf (buffer2, "type = \"%s\"", type);
         sprintf (buffer1, "name = \"%s\"", dsn);
         fprintf (out_file, "   <dsn %-15s %-40s/>\n",
                            buffer2, buffer1); 
         feedback = TRUE;
      }
    return (feedback);
}

void display_usage (const char *program_name)
{
    fprintf (out_file, "%s version %s\n", program_name, VERSION);
    fprintf (out_file, "Command-Line ODBC Access Program.\n");
    fprintf (out_file, "Copyright (c) 1996-2002 iMatix - http://www.imatix.com\n\n");

    fprintf (out_file, "syntax:\n");
    fprintf (out_file, "    %s -<function> -<options>\n\n", program_name);

    fprintf (out_file, "function :\n");
    fprintf (out_file, " -e  [listfile]    Export tables listed in list file. Default is all tables\n");
    fprintf (out_file, " -i  [listfile]    Import tables listed in list file. Default is all tables\n");
    fprintf (out_file, " -ie [listfile]    Import tables listed in list file. Default is all tables\n");
    fprintf (out_file, "                   this import delete All record before inport\n");
    fprintf (out_file, " -ii [listfile]    Import tables listed in list file. Default is all tables\n");
    fprintf (out_file, "                   this import update record if exist.\n");
    fprintf (out_file, " -x  sqlfile       Execute SQL file. which can contain multiple SQL statement\n");
    fprintf (out_file, "                   delimited by ';' (not newlines).\n");
    fprintf (out_file, " -s  filename      Execute a formatted SELECT statement.\n");
    fprintf (out_file, "                   Return data as XML file.\n");
    fprintf (out_file, " -g  tablename     Get structure of table name.\n");
    fprintf (out_file, " -l  listfile      Get structure of tables listed in XML file as follows : \n");
    fprintf (out_file, "                      REQUEST (no attributes)\n");
    fprintf (out_file, "                          TABLE (NAME)\n");
    fprintf (out_file, " -r  dbmlfile      Review tables listed in DBML file to match table definition\n");
    fprintf (out_file, " -c  filename      Get record count of each tables\n");
    fprintf (out_file, "                  Returns result as XML file.\n");
    fprintf (out_file, "\noptions :\n");
    fprintf (out_file, " -d   [database]  ODBC Data Source name\n");
    fprintf (out_file, "                  if database is empty, list all data sources\n");
    fprintf (out_file, " -u   user        Connect username (if required)\n");
    fprintf (out_file, " -p   password    Connect password (if required)\n");
    fprintf (out_file, " -o   output      Specifies output file for actions. Default is stdout.\n");
    fprintf (out_file, " -m   maxrows     Max desired rows for select or export.\n");
    fprintf (out_file, " -t   maxtime     Max desired time for select, export or import(msecs)\n");
    fprintf (out_file, " -sch schema      Name of schema (or catalog) used to list all tables\n");
    fprintf (out_file, " -en              Encode data\n");
}


