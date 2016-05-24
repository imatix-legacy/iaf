/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafmysql.c
    Title:      IAF MYSQL DB Management
    Package:    IAF

    Written:    1999/08/31  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/01/09  Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Implement principal functions of iAF db agent.

    Copyright:  Copyright (c) 1991-2002 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/
#include "sfl.h"
#include "mysql.h"

/*- Definitions -------------------------------------------------------------*/
#define BUFFER_SIZE 65535

typedef int (TABLE_FUNC) (char *table);

#define VERSION                    "1.0"
#define PROGRAM_NAME               "iAFMySql"

#define DATA_TYPE_CHAR             FIELD_TYPE_CHAR
#define DATA_TYPE_NUMERIC          2
#define DATA_TYPE_DECIMAL          FIELD_TYPE_DECIMAL
#define DATA_TYPE_INTEGER          FIELD_TYPE_LONG
#define DATA_TYPE_SMALL_INT        FIELD_TYPE_SHORT
#define DATA_TYPE_FLOAT            FIELD_TYPE_FLOAT
#define DATA_TYPE_REAL             FIELD_TYPE_DOUBLE
#define DATA_TYPE_DOUBLE           FIELD_TYPE_DOUBLE
#define DATA_TYPE_DATE             FIELD_TYPE_DATE
#define DATA_TYPE_TIME             FIELD_TYPE_TIME
#define DATA_TYPE_TIME_STAMP       FIELD_TYPE_TIMESTAMP
#define DATA_TYPE_VARCHAR          FIELD_TYPE_VAR_STRING
#define DATA_TYPE_LONG_VARCHAR     FIELD_TYPE_BLOB
#define DATA_TYPE_BINARY           FIELD_TYPE_TINY_BLOB
#define DATA_TYPE_VARBINARY        FIELD_TYPE_BLOB
#define DATA_TYPE_LONG_VARBINARY   FIELD_TYPE_LONG_BLOB
#define DATA_TYPE_BIG_INT          FIELD_TYPE_LONGLONG
#define DATA_TYPE_TINY_INT         FIELD_TYPE_TINY
#define DATA_TYPE_BIT              FIELD_TYPE_TINY
#define DATA_TYPE_UNICODE_CHAR     -1
#define DATA_TYPE_UNICODE_VARCHAR  -1
#define DATA_TYPE_UNICODE_TEXT     -1


/*- Structure ---------------------------------------------------------------*/
typedef struct _table_defn
{
    struct _table_defn
         *next, *prev;                  /*  Linked list pointer              */
    char *name;                         /*  Name of table                    */
} TABLE_DEFN;

/*  Columns definition structure                                             */

typedef struct _col_defn
{
    struct _col_defn
               *next, *prev;            /*  Linked list pointer             */
    MYSQL_FIELD col;
    Bool        nullable;
    void       *value;
    int         indic;
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
    *host_name    = NULL,
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
    message      [256];
static XML_ITEM
    *xml_item = NULL;
static long
    max_row = 0;
static char
    buffer [BUFFER_SIZE + 1];

static Bool
    dropall       = FALSE,
    force2zero    = TRUE,
    have_identity = FALSE,
    import        = FALSE,
    export        = FALSE,
    encode_data   = FALSE;
static FILE
    *out_file     = NULL;
static MYSQL
    *db_handle;
static MYSQL_RES
    *result;



/*- Functions definition ----------------------------------------------------*/
void      free_resource              (void);
void      print_dbml                 (char *table);
int       get_all_columns_definition (char *table_name);
void      get_all_index_definition   (char *table_name);
int       get_all_table_name         (void);
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
short     get_db_field_type          (char *type, char *native_type, COL_DEFN *col);

static void print_database_summary (char *filename);
static long get_record_count       (LIST *col_list, char *table);


/* Include incremental parser for big xml file                               */
#define DB_MYSQL
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
                case 's': argparm    = &exec_file;    break;
                case 'm': argparm    = &max_rows;     break;
                case 't': argparm    = &max_time;     break;
                case 'r': argparm    = &dbml_file;    break;
                case 'h': help       = TRUE;          break;
                case 'n': force2zero = TRUE;          break;
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
        printf ("Argument missing - type '%s -h' for help", PROGRAM_NAME);
        free_resource ();
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        printf ("Invalid arguments - type '%s -h' for help", PROGRAM_NAME);
        free_resource ();
        exit (EXIT_FAILURE);
      }
    if (database == NULL)
      {
        printf ("A database name is required - type '%s -h' for help", PROGRAM_NAME);
        free_resource ();
        exit (EXIT_FAILURE);
      }


    if (max_rows)
        max_row = atol (max_rows);

    if (host_name == NULL)
        host_name = mem_strdup ("127.0.0.1");

#if MYSQL_VERSION_ID >= 32200
    db_handle = mysql_init ((MYSQL*)0);
    if (!mysql_real_connect (db_handle, host_name, user, password, NULL, 
                             MYSQL_PORT, NULL, 0))
      {
         coprintf ("Error on connection %s", mysql_error (db_handle));
         db_handle = NULL;
      }
    else
#else
    db_handle = mysql_real_connect (NULL, host_name, user, password,
                                    MYSQL_PORT, NULL, 0);
#endif
    if (db_handle)
      {
        if (mysql_select_db (db_handle, database) != 0)
          {
            mysql_close (db_handle);
            db_handle = NULL;
          }
      }

    if (db_handle)
      {

/*            if (table)
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
*/
            if (sql_file)
                execute_sql_file (sql_file);
/*
            if (dbml_file)
                review_table (dbml_file);
*/
            if (import)
              {
                fprintf (out_file, "<import_result>\n");
                get_all_table_name ();
                if (import_list)
                    execute_table_list (import_list, new_import_table, FALSE);
                else
                    import_all_tables ();
                fprintf (out_file, "</import_result>\n");
              }

//            if (summary_file)
//                print_database_summary (summary_file);

        mysql_close (db_handle);
        db_handle = NULL;
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
    if (host_name    != NULL)
        mem_strfree (&host_name);
    if (result)
      {
        mysql_free_result (result);
        result = NULL;
      }
    if (db_handle)
      {
        mysql_close (db_handle);
        db_handle = NULL;
      }

//    free_all_columns_name (&columns);
//    free_all_index_name   (&index_list);
    free_all_table_name   (&table_list);
//    free_data_type_info   (&type_list);
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
       mem_strfree (&table-> name);
       mem_free    (table);
       table = next;
     }
   list_reset (table_head);
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
    MYSQL_ROW
        row ;

    /* load only if table list is empty                                      */
    if ((void *)table_list.next == (void *)&table_list)
      {
	    result = mysql_list_tables (db_handle, NULL);
	    while (row = mysql_fetch_row (result))
          {
            table = mem_alloc (sizeof (TABLE_DEFN));
            if (table)
              {
                memset (table, 0, sizeof (TABLE_DEFN));
                list_reset (table);
                if (row [0] && strused (row [0]))
                    table-> name   = mem_strdup (row [0]);
                list_relink_before (table, &table_list);
                nb_tables++;
              }
 	     }
        mysql_free_result (result);
        result = NULL;
      }
    return (nb_tables);
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


            statement_nb++;
            if (!mysql_query (db_handle, buffer))
                fprintf (out_file, "<statement number=\"%d\" status=\"OK\" />\n",
                          statement_nb);
            else
              {
                fprintf (out_file, "<statement number=\"%d\" status=\"ERROR\" value=\"%s\" statement=\"%s\"/>\n",
                          mysql_errno (db_handle),
                          mysql_error (db_handle),
                          buffer);

              }
          }
      }
    fputs ("</result>", out_file);

    fclose (fd);
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
            table_mask = xml_get_attr (child, "name", NULL);
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
        if (new_import_table (table-> name) != 0)
            return -1;                  /* we break on first error           */
        table = table-> next;
      }

    return 0;
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

    memset (message_code, 0, 21);
    memset (message,      0, 256);

    sprintf (message_code, "%d", mysql_errno (db_handle));
    strcpy  (message, mysql_error (db_handle));

    return (feedback);
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
                column = get_select_col (&columns, col-> col.name);
                if (column == NULL)
                    free_column (col);
                else
                  {
                    col-> nullable  = column-> nullable;
                    col-> col.type = column-> col.type;
                    col-> col.length    = column-> col.length;
                    col-> col.decimals     = column-> col.decimals;
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
                    col-> col.type == DATA_TYPE_NUMERIC
                ||  col-> col.type == DATA_TYPE_DECIMAL
                ||  col-> col.type == DATA_TYPE_INTEGER
                ||  col-> col.type == DATA_TYPE_SMALL_INT
                ||  col-> col.type == DATA_TYPE_FLOAT
                ||  col-> col.type == DATA_TYPE_REAL
                ||  col-> col.type == DATA_TYPE_DOUBLE
                ||  col-> col.type == DATA_TYPE_BIG_INT
                ||  col-> col.type == DATA_TYPE_TINY_INT
                ||  col-> col.type == DATA_TYPE_BIT)
                   )
                  {
                    column = get_select_col (column_list, col-> col.name);
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
                col-> col.name      = mem_strdup (xml_get_attr (field, "name", ""));
                s_value = atoi (xml_get_attr (field, "size", "0"));
                col-> col.max_length = s_value;
                s_value = atoi (xml_get_attr (field, "decimal", "0"));
                col-> col.decimals = s_value;
                c_value = xml_get_attr (field, "nulls", "N");
                if (*c_value == 'y' || *c_value == 'Y')
                    col-> nullable = TRUE;
                col-> col.type = get_db_field_type (xml_get_attr (field, "type",  NULL),
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
                        col-> col.type = type;                    \
                        col-> type_name = mem_strdup (type_name);  \
                      }                                            \
                  }

/*  -------------------------------------------------------------------------
    Function: get_db_field_type

    Synopsis: Get type of field.
    -------------------------------------------------------------------------*/

short
get_db_field_type (char *type, char *native_type, COL_DEFN *col)
{
    short
        odbc_type = 9999;
    char
        *type_name;


    ASSERT (col);

    if (type == NULL)
        return (odbc_type);

    if (lexcmp (type, "textual") == 0)
      {
        if (col-> col.max_length <= 255)
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
        if (col-> col.decimals > 0)
          {
            SEARCH_TYPE (DATA_TYPE_FLOAT);
            SEARCH_TYPE (DATA_TYPE_REAL);
            SEARCH_TYPE (DATA_TYPE_DOUBLE);
            SEARCH_TYPE (DATA_TYPE_DECIMAL);
          }
        else
          {
            if (col-> col.max_length <= 5)
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
        if (col-> col.max_length <= 255)
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
                odbc_type = type_def-> col.type;
                col-> type_name = mem_strdup (type_def-> type_name);
                next = (TYPE_DEFN *)&type_list;
              }
            type_def = next;
          }
      }
    return (odbc_type);
}


void display_usage (const char *program_name)
{
    fprintf (out_file, "%s version %s\n", program_name, VERSION);
    fprintf (out_file, "Command-Line MySql Access Program.\n");
    fprintf (out_file, "Copyright (c) 1996-2002 iMatix - http://www.imatix.com\n\n");

    fprintf (out_file, "syntax:\n");
    fprintf (out_file, "    %s -<function> -<options>\n\n", program_name);

    fprintf (out_file, "function :\n");
    fprintf (out_file, " -e  [listfile]    Export tables listed in list file. Default is all tables\n");
    fprintf (out_file, " -i  [listfile]    Import tables listed in list file. Default is all tables\n");
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
    fprintf (out_file, " -r  dbmlfile      Review tables listed in DBML file to match table definition\n");
    fprintf (out_file, " -c  filename      Get record count of each tables\n");
    fprintf (out_file, "                  Returns result as XML file.\n");
    fprintf (out_file, "\noptions :\n");
    fprintf (out_file, " -d database      ODBC Data Source name\n");
    fprintf (out_file, " -u user          Connect username (if required)\n");
    fprintf (out_file, " -p password      Connect password (if required)\n");
    fprintf (out_file, " -o output        Specifies output file for actions. Default is stdout.\n");
    fprintf (out_file, " -m maxrows       Max desired rows for select or export.\n");
    fprintf (out_file, " -t maxtime       Max desired time for select, export or import(msecs)\n");
    fprintf (out_file, " -en              Encode data\n");
    fprintf (out_file, " -host host_name  Connect to this host name\n");
    fprintf (out_file, " -p port          Port value\n");
}
