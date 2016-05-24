/*  ----------------------------------------------------------------<Prolog>-
    Name:       dbpars.c
    Title:      XML serialisation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/08  iMatix SFL project team <sfl@imatix.com>
    Revised:    2002/07/22

    Synopsis:   Loads XML file into memory, to/from disk files.

    Copyright:  Copyright (c) 1991-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/


void init_charmaps (void);
int new_import_table (char *table_name);


struct
{
    char
        file_name [256];
    char
        *table_name;
    LIST
        col_list;
    long
        recno;
    Bool
        db_error;
    Bool
        encoded_data;
} db_globals;



Bool table_management (XML_ITEM *parent, XML_ITEM *child)
{
    Bool res = TRUE;

    /* parent is not used */
    ASSERT (child);

    db_globals.table_name = update_table_struct (child, &db_globals.col_list);
    if (db_globals.table_name == NULL)
      {
        res = FALSE;
        db_globals.db_error = TRUE;
        get_db_error_message ();
        fprintf (out_file, "<error ROW=\"%ld\" CREATE_TABLE=\"TRUE\" MESSAGE=\"%s\" />\n",
                    db_globals.recno, message);
      }
#ifdef DB_ODBC
    if (have_identity)
      {
        SQLFreeStmt (h_statement, SQL_UNBIND);
        SQLFreeStmt (h_statement, SQL_CLOSE);
        sprintf (db_globals.file_name, "SET IDENTITY_INSERT %s ON", db_globals.table_name);
        return_code = SQLExecDirect (h_statement, db_globals.file_name, SQL_NTS);
        if (return_code != SQL_SUCCESS
        &&  return_code != SQL_SUCCESS_WITH_INFO)
          {
            get_db_error_message ();
            db_globals.db_error = TRUE;
            res = FALSE;
          }
      }
    SQLFreeStmt (h_statement, SQL_UNBIND);
    SQLFreeStmt (h_statement, SQL_CLOSE);
#endif

    xml_free (child);

    return res;
}


Bool record_management (XML_ITEM *parent, XML_ITEM *child)
{
    Bool res = TRUE;

    /* parent is not used */
    ASSERT (child);

    if (import_row (db_globals.table_name, child, &db_globals.col_list, db_globals.encoded_data))
        db_globals.recno++;
    else
      {
        get_db_error_message ();
        fprintf (out_file, "<error row=\"%ld\" message=\"%s\" />\n",
                    db_globals.recno + 1, message);
        res = FALSE;
        db_globals.db_error = TRUE;
      }

    xml_free (child);

    return res;
}

/* these constants must be used as 'mode' parameter in parse_XXX functions */
#define MODE_ONCE                (1)
#define MODE_ONCE_OR_MORE        (2)
#define MODE_ZERO_OR_MORE        (3)
#define MODE_OPTIONAL            (4)



#define NO_ERROR_MSG                      (-1)
#define CANNOT_FIND_FILE                    0
#define CANNOT_OPEN_FILE                    1
#define ERROR_CLOSING_FILE                  2
#define EXPECTED_EOF                        3
#define UNEXPECTED_EOF                      4
#define UNEXPECTED_CHAR                     5
#define EXPECTED_STRING                     6
#define OUT_OF_MEMORY                       7

const char * error_messages[] =
  {
    /* static messages */
/*  0 */       "Could not find XML file."
/*  1 */     , "Could not open XML file."
/*  2 */     , "Error closing file."
/*  3 */     , "End of file expected."
/*  4 */     , "Unexpected end of file."
/*  5 */     , "Unexpected character."
/*  6 */     , "Expected string."
/*  7 */     , "Out of memory"
    /* messages from xml spec decription */
  };

/* it is necessary to have an even XML_BUFFER_SIZE and to have
 * HALF_BUFFER defined as the exact half of XML_BUFFER_SIZE
 * I didn't want to use (XML_BUFFER_SIZE / 2) to avoid expensive divisions
 * However, perhaps compilers solve this
 */

#define XML_BUFFER_SIZE          (0x100000)  /* 1 Mb */
#define HALF_XML_BUFFER_SIZE     (0x80000)   /* 500 Kb */
#define TAB_EXPANSION        8
#define STACK_DEPTH          500
#define ERROR_MSG_SIZE       512


typedef struct _XML_CONTEXT
{
    long
            pos;                    /* pos relatively to start of the file.*/
    int
            line;

} XML_CONTEXT;


typedef struct _XML_BUFFER
{
    char
        *cache,
        *cache_last_ptr,
        *cache_cur_ptr;
    int
        stack_top,
        stack_depth,
        cache_size;
    long
        cache_offset;              /* tells which byte (relatively to start of file)
                                      is loaded in cache[0] */
    XML_CONTEXT
        error_ctxt,
        *stack;
    long
        file_size;
    char
        *error_msg;
    int
        error_code;
    const char
        *filename;
    FILE
        *fd;
} XML_BUFFER;

#define cur_ctxt(BUF)        ((BUF)->stack [(BUF)-> stack_top])


#define get_next_char(BUF, RES)                                            \
    ASSERT (BUF);                                                          \
    if ( !BUF-> cache_cur_ptr                                              \
            || (BUF-> cache_cur_ptr == BUF-> cache_last_ptr) )             \
        handle_page_fault (BUF);                                           \
    /* if cache_cur_ptr is valid, it must be pointing to the cache, or */  \
    /* to the char after last one (INVARIANT) */                           \
    ASSERT (BUF-> cache_cur_ptr                                            \
        ? (BUF-> cache_cur_ptr >= BUF-> cache)                             \
              && (BUF-> cache_cur_ptr < BUF-> cache_last_ptr)              \
        : TRUE);                                                           \
                                                                           \
    if (BUF-> cache_cur_ptr)                                               \
      {                                                                    \
        cur_ctxt(BUF).pos++;                                               \
        if (*BUF-> cache_cur_ptr == '\r')                                  \
            cur_ctxt(BUF).line++;                                          \
        RES = *BUF-> cache_cur_ptr++;                                      \
      }                                                                    \
    else                                                                   \
        RES = 0;



#define clear_error(BUF)                                                   \
  {                                                                        \
    BUF-> error_code = NO_ERROR;                                           \
    BUF-> error_ctxt.pos = -1;                                             \
  }

#define set_error(BUF, ERROR_CODE)                                         \
  {                                                                        \
    if (cur_ctxt(BUF).pos >= BUF-> error_ctxt.pos)                         \
      {                                                                    \
        memcpy (&BUF-> error_ctxt,&cur_ctxt(BUF), sizeof(XML_CONTEXT));    \
        BUF-> error_code = ERROR_CODE;                                     \
      }                                                                    \
  }


/***************************************************************************/
/* these macros are used to TRY parsing something.                         */
/* the TRY encapsulation allows a roll back while parsing.                 */
/* meaning that buffer context is saved before parsing function            */
/* is called. If the parse function succeed, saved context is dropped      */
/* otherwise, saved context is restored                                    */
/*                                                                         */
/* to use TRY mechanism within a function :                                */
/* - as first statement of the function : ENABLE_TRY(buf)                  */
/*   where buf is an XML_BUFFER (not NULL)                                 */
/*                                                                         */
/* Bool f (XML_BUFFER *buf, blah blah)                                     */
/* {                                                                       */
/*    ENABLE_TRY                                                           */
/*    ...                                                                  */
/*                                                                         */
/*    TRY (parse_XXX);                                                     */
/*    if (TRY_SUCCEED)                                                     */
/*       ...                                                               */
/*    // or                                                                */
/*    if (TRY_FAILED)                                                      */
/*       ...                                                               */
/* }                                                                       */
/*                                                                         */
/* TRY_HANDLER is quite the same, except a handler is applied if parsing   */
/* function succeed                                                        */
/***************************************************************************/
#define ENABLE_TRY                                                        \
    Bool                                                                  \
            try_res;


void save_context (XML_BUFFER *buf)
{
    ASSERT (buf);

    ASSERT (buf-> stack_top >= 0);
    ASSERT (buf-> stack_top < buf-> stack_depth);
    if (buf-> stack_top == buf-> stack_depth - 1)
      {
        /* our static stack is full. We realloc */
        buf-> stack_depth += STACK_DEPTH;
        buf-> stack = (XML_CONTEXT *)
                            mem_realloc (buf-> stack, buf-> stack_depth);
        if (!buf-> stack)
          {
            set_error (buf, OUT_OF_MEMORY);
            return;
          }
      }

    ASSERT (sizeof (XML_CONTEXT) == 8);
    memcpy (&buf-> stack [buf-> stack_top + 1],
            &cur_ctxt(buf),
//             8 );
            sizeof(XML_CONTEXT));
    buf-> stack_top++;
}


void restore_context (XML_BUFFER *buf)
{
    ASSERT (buf);
    buf-> stack_top--;

    /* we must update or invalidate cache information */
    if (buf-> cache_cur_ptr)
      {
        /* cache still has revelant information */
        ASSERT (cur_ctxt(buf).pos >= 0);
        ASSERT (cur_ctxt(buf).pos < buf-> file_size);
        ASSERT (cur_ctxt(buf).pos
                      < buf-> cache_offset + buf-> cache_size);
        if ( cur_ctxt(buf).pos >= buf-> cache_offset)
            /* cur pos is still in the cache */
            buf-> cache_cur_ptr =
                    &buf-> cache [cur_ctxt(buf).pos - buf-> cache_offset];
        else
            buf-> cache_cur_ptr = NULL;
      }
}


void drop_context (XML_BUFFER *buf)
{
    ASSERT (buf);
    ASSERT (buf-> stack_top > 0);
    buf-> stack_top--;
    memcpy (&cur_ctxt(buf),
            &buf-> stack[buf-> stack_top + 1],
            sizeof (XML_CONTEXT));
}



#define _save_context                                                          \
{                                                                             \
    ASSERT (buf);                                                             \
                                                                              \
    ASSERT (buf-> stack_top >= 0);                                            \
    ASSERT (buf-> stack_top < buf-> stack_depth);                             \
    if (buf-> stack_top == buf-> stack_depth - 1)                             \
      {                                                                       \
        /* our static stack is full. We realloc */                            \
        buf-> stack_depth += STACK_DEPTH;                                     \
        buf-> stack = (XML_CONTEXT *)                                         \
                            mem_realloc (buf-> stack, buf-> stack_depth);     \
        if (!buf-> stack)                                                     \
            ASSERT (FALSE);   /* XXX TODO : exhausted memory management */    \
      }                                                                       \
                                                                              \
    memcpy (&buf-> stack [buf-> stack_top + 1],                               \
            &cur_ctxt(buf)                ,                                   \
            sizeof(XML_CONTEXT));                                             \
    buf-> stack_top++;                                                        \
}


#define _restore_context                                                       \
{                                                                             \
    ASSERT (buf);                                                             \
    buf-> stack_top--;                                                        \
                                                                              \
    /* we must update or invalidate cache information */                      \
    if (buf-> cache_cur_ptr)                                                  \
      {                                                                       \
        /* cache still has revelant information */                            \
        ASSERT (cur_ctxt(buf).pos >= 0);                                      \
        ASSERT (cur_ctxt(buf).pos < buf-> file_size);                         \
        ASSERT (cur_ctxt(buf).pos                                             \
                      < buf-> cache_offset + buf-> cache_size);               \
        if ( cur_ctxt(buf).pos >= buf-> cache_offset)                         \
            /* cur pos is still in the cache */                               \
            buf-> cache_cur_ptr =                                             \
                    &buf-> cache [cur_ctxt(buf).pos - buf-> cache_offset];    \
        else                                                                  \
            buf-> cache_cur_ptr = NULL;                                       \
      }                                                                       \
}


#define _drop_context                                                          \
{                                                                             \
    ASSERT (buf);                                                             \
    ASSERT (buf-> stack_top > 0);                                             \
    buf-> stack_top--;                                                        \
    memcpy (&cur_ctxt(buf),                                                   \
            &buf-> stack[buf-> stack_top + 1],                                \
            sizeof (XML_CONTEXT));                                            \
}



/***************************************************************************/
/* - PARSE_FCT must be a complete function call (with arguments).          */
/*   That function must return a boolean (TRUE on success)                 */
/* - HANDLER must be the name of a void function, excpecting 2 arguments:  */
/*   a pointer to a struct to fill and a string                            */
/* - STRUCT_TO_FILL is the actual struct var we'll give the handler on     */
/*   success                                                               */
/* - DECODE is a boolean informing whether the string to handle (on        */
/*   success) must be decoded for HTTP meta characters                     */
/***************************************************************************/

#define TRY_HANDLER(PARSE_FCT, HANDLER, STRUCT_TO_FILL, DECODE, ERROR_CODE) \
    save_context (buf);                                                   \
    try_res = PARSE_FCT;                                                  \
    if (try_res)                                                          \
      {                                                                   \
        try_res = HANDLER (STRUCT_TO_FILL, get_string(buf, DECODE));      \
        if (!try_res)                                                     \
            set_error (buf, ERROR_CODE)                                   \
      }                                                                   \
    if (try_res)                                                          \
      {                                                                   \
        if (cur_ctxt(buf).pos > buf-> error_ctxt.pos)                     \
            clear_error (buf);                                            \
        drop_context (buf);                                               \
      }                                                                   \
    else                                                                  \
        restore_context (buf);


#define TRY(PARSE_FCT)                                                    \
    save_context (buf);                                                   \
    try_res = PARSE_FCT;                                                  \
    if (try_res)                                                          \
      {                                                                   \
        drop_context (buf);                                               \
        if (cur_ctxt(buf).pos > buf-> error_ctxt.pos)                     \
            clear_error (buf);                                            \
      }                                                                   \
    else                                                                  \
        restore_context (buf);


#define TRY_IS(CMAP_ENTRY)                                                \
    save_context (buf);                                                   \
    get_next_char (buf, try_ch);                                          \
    try_res = is_##CMAP_ENTRY (try_ch);                                   \
    if (try_res)                                                          \
      {                                                                   \
        drop_context (buf);                                               \
        if (cur_ctxt(buf).pos > buf-> error_ctxt.pos)                     \
            clear_error (buf);                                            \
      }                                                                   \
    else                                                                  \
      {                                                                   \
        set_error (buf, UNEXPECTED_CHAR);                                 \
        restore_context (buf);                                            \
      }


#define TRY_SUCCEED     (try_res)

#define TRY_FAILED      (!try_res)


/*  Character classification tables and macros                               */

static byte
// static word
    cmap [256];                         /*  Character classification tables  */


/* TODO : we could define set with other sets (recursively)
and define masks.
Ex : name_char = letter ou digit

#define CMAP_NAME_CHAR         CMAP_LETTER + CMAP_DIGIT

Main advantage is that a set can be described by subsets,
and a subset can be used to describe several sets.
---> with only ONE test, we can figure out whether in set !!!
*/

#define CMAP_WHITE_SPACE    (1)
#define CMAP_FIRST_NAME_CHAR  (2)
#define CMAP_NAME_CHAR      (4)

/*  Macros for character mapping:    */
#define is_white_space(ch)   (cmap [(byte) (ch)] & CMAP_WHITE_SPACE)
#define is_first_name_char(ch)  (cmap [(byte) (ch)] & CMAP_FIRST_NAME_CHAR)
#define is_name_char(ch)     (cmap [(byte) (ch)] & CMAP_NAME_CHAR)

/***************************************************************************/
/*                          FUNCTION DECLARATIONS                          */
/***************************************************************************/

/* CMAP functions                                                          */

/* parse functions                                                        */
Bool    parse_export_doc (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_table (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_record (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_white_spaces (XML_BUFFER *buf, int mode);
Bool    parse_attr (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_field (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_key (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_name (XML_BUFFER *buf, int mode);
Bool    parse_eq (XML_BUFFER *buf, int mode);
Bool    parse_att_value (XML_BUFFER *buf, int mode, XML_ATTR *parent_struct);

/* parse_group functions                                                  */
Bool    parse_group_1 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_2 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_3 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_4 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_5 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_6 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_7 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_8 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct);
Bool    parse_group_9 (XML_BUFFER *buf, int mode);

/* general parse functions                                                */
//#define parse_one_char(BUF, CH)    (CH == get_next_char(BUF))
Bool    parse_one_char          (XML_BUFFER *buf, const char ch);
Bool    parse_constant_string   (XML_BUFFER *buf, const char *str);
Bool    parse_any_before_char   (XML_BUFFER *buf, const char ch, Bool empty_allowed);
Bool    parse_any_before_string (XML_BUFFER *buf, const char *str);

/* XML_BUFFER manipulation function                                       */
XML_BUFFER    *buf_init_from_file   (const char *fullpath, char *error_msg);
XML_BUFFER    *buf_init_from_string (const char *xmlstring);
char          *get_string           (XML_BUFFER *buf, Bool decode_http);
void           buf_dispose          (XML_BUFFER **buf);
void           handle_page_fault    (XML_BUFFER *buf);
char          *build_error_message  (XML_BUFFER *buf);





/***************************************************************************/
/*                          FUNCTION DEFINITIONS                           */
/***************************************************************************/

/*  -------------------------------------------------------------------------
 *  build_charmap
 *
 *  Encode character string and flag into character map table.  Flag should
 *  be a 1-bit value from 1 to 128 (character map is 8 bits wide).
 */

static void
build_charmap (byte flag, char *string)
{
    for (; *string; string++)
        cmap [(byte) *string] |= flag;
}

static void
build_charmap_for_range (byte flag, byte min, byte max)
{
    int
            ch, intmin, intmax;

    ASSERT (min <= max);

    /* cast to cover case : min == 0 && max == 255 */
    intmin = min;
    intmax = max;

    for (ch = min; ch <= max; ch++)
      {
        ASSERT (ch <= 255);
        cmap [(byte) ch] |= flag;
      }
}

static void
build_charmap_for_except (byte flag, char *string)
{
    for (; *string; string++)
        cmap [(byte) *string] &= ~flag;
}


void
init_charmaps (void)
{
    memset (cmap, 0, sizeof (cmap));    /*  Clear all bitmaps                */

    build_charmap (CMAP_WHITE_SPACE, " \n\r\t");

    build_charmap (CMAP_FIRST_NAME_CHAR, "_:");
    build_charmap_for_range (CMAP_FIRST_NAME_CHAR, 'a', 'z');
    build_charmap_for_range (CMAP_FIRST_NAME_CHAR, 'A', 'Z');

    build_charmap (CMAP_NAME_CHAR, "-_.:");
    build_charmap_for_range (CMAP_NAME_CHAR, 'a', 'z');
    build_charmap_for_range (CMAP_NAME_CHAR, 'A', 'Z');
    build_charmap_for_range (CMAP_NAME_CHAR, '0', '9');
}


/***************************************************************************/
int new_import_table (char *table)
{
    XML_ITEM
        *root;
    XML_BUFFER
        *buf;
    char
        *table_name,
        row_buf   [50],
        error_msg [1024];
    int
        res = -1;
    Bool
        parsed;

    ASSERT (table);

    sprintf (db_globals.file_name, "%s.xml", table);

    if (!file_exists (db_globals.file_name))
      {
        sprintf (error_msg, "name=\"%s\"", strlwc (table));
        fprintf (out_file,
                "<table %-25s warning=\"1\" message=\"file is missing\" />\n",
                error_msg);
        return XML_NOERROR;
      }

    if (dropall)
        drop_all_record (table);

#ifdef DB_ODBC
    if (incremental_import)
      {
        table_name = get_real_table_name (&table_list, table);
        if (table_name)
          {
            primary_key = get_pk_name (table_name);
            if (primary_key == NULL)
              {
                primary_key = get_pk_name_from_list (table_name);

                if (primary_key == NULL)
                  {
                    fprintf (out_file,
                        "<table name=\"%s\" warning=\"1\" message=\"%s: missing primary key\" />\n",
                        strlwc (table), table);
                    return XML_NOERROR;
                  }
              }
          }
      }
#endif
    buf = buf_init_from_file (db_globals.file_name, error_msg);

    if (buf)
      {
        list_reset (&db_globals.col_list);

        root = alloc_xml_item ();

        init_charmaps ();
        db_globals.recno = 0;
        db_globals.db_error = FALSE;

        parsed = parse_export_doc (buf, MODE_ONCE, root);

        if (parsed )
          {
            res = 0;
#ifdef DB_ODBC
            if (have_identity)
              {
                SQLFreeStmt (h_statement, SQL_UNBIND);
                SQLFreeStmt (h_statement, SQL_CLOSE);
                sprintf (db_globals.file_name, "SET IDENTITY_INSERT %s OFF", db_globals.table_name);
                return_code = SQLExecDirect (h_statement, db_globals.file_name, SQL_NTS);
                if (return_code != SQL_SUCCESS
                &&  return_code != SQL_SUCCESS_WITH_INFO)
                  {
                    get_db_error_message ();
                    res = -1;
                  }
              }
#endif
          }
        else
        if (!db_globals.db_error)
            printf ("xml file is not well-formed");

        buf_dispose (&buf);

        sprintf (error_msg, "name=\"%s\"", strlwc (table));
        sprintf (row_buf,   "rows=\"%ld\"", db_globals.recno);
        fprintf (out_file, "<table %-25s %-11s/>\n", error_msg, row_buf);

        free_all_columns_name (&db_globals.col_list);
        xml_free (root);
        fflush (out_file);
      }
    else
      {
        fprintf (out_file, "<table name=\"%s\" error=\"1\" row=\"%ld\" message=\"%s\" />\n",
                           strlwc (table),
                           db_globals.recno + 1,
                           error_msg);
      }

    free_all_columns_name (&columns);
    free_all_index_name   (&index_list);

    return res;
}


/***************************************************************************/
Bool parse_export_doc (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_constant_string (buf, "<export>"));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_table (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_group_1 (buf, MODE_ZERO_OR_MORE, parent_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_constant_string (buf, "</export>"));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_export_doc(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
Bool parse_table (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    XML_ITEM
            *child_struct = NULL;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            child_struct = alloc_xml_item();
            if (!child_struct)
              {
                /* XXX TODO : management of an error message - exhausted memory */
                /* If this happen, we can assume it is THE actual error */
                res = FALSE;
                break;
              }
            res = FALSE;
            TRY (parse_one_char (buf, '<'));
            if (TRY_FAILED)
                break;
            TRY_HANDLER (parse_constant_string (buf, "table"),
                         set_xml_item_name,
                         child_struct,
                         FALSE,
                         NO_ERROR_MSG);
            if (TRY_FAILED)
                break;
            TRY (parse_group_2 (buf, MODE_ONCE_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_one_char (buf, '>'));
            if (TRY_FAILED)
                break;
            TRY (parse_group_3 (buf, MODE_ONCE_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_group_4 (buf, MODE_ZERO_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_constant_string (buf, "</table>"));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_table(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    if (mode == MODE_ONCE)
      {
        ASSERT (child_struct);
        if (res)
          {
            res = table_management (parent_struct, child_struct);
            if (!res)
                set_error (buf, NO_ERROR_MSG)
          }
        else
            xml_free (child_struct);
      }

    return res;
}


/***************************************************************************/
Bool parse_record (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    XML_ITEM
            *child_struct = NULL;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            child_struct = alloc_xml_item();
            if (!child_struct)
              {
                /* XXX TODO : management of an error message - exhausted memory */
                /* If this happen, we can assume it is THE actual error */
                res = FALSE;
                break;
              }
            res = FALSE;
            TRY (parse_one_char (buf, '<'));
            if (TRY_FAILED)
                break;
            TRY_HANDLER (parse_constant_string (buf, "record"),
                         set_xml_item_name,
                         child_struct,
                         FALSE,
                         NO_ERROR_MSG);
            if (TRY_FAILED)
                break;
            TRY (parse_group_5 (buf, MODE_ONCE_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_constant_string (buf, "/>"));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_record(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    if (mode == MODE_ONCE)
      {
        ASSERT (child_struct);
        if (res)
          {
            res = record_management (parent_struct, child_struct);
            if (!res)
                set_error (buf, NO_ERROR_MSG)
          }
        else
            xml_free (child_struct);
      }

    return res;
}


/***************************************************************************/
Bool parse_white_spaces (XML_BUFFER *buf, int mode)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    char
            try_ch;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY_IS (white_space);
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_white_spaces(buf, MODE_ONCE));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
Bool parse_attr (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    XML_ATTR
            *child_struct = NULL;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            child_struct = alloc_xml_attr();
            if (!child_struct)
              {
                /* XXX TODO : management of an error message - exhausted memory */
                /* If this happen, we can assume it is THE actual error */
                res = FALSE;
                break;
              }
            res = FALSE;
            TRY_HANDLER (parse_name (buf, MODE_ONCE),
                         set_xml_attr_name,
                         child_struct,
                         FALSE,
                         NO_ERROR_MSG);
            if (TRY_FAILED)
                break;
            TRY (parse_eq (buf, MODE_ONCE));
            if (TRY_FAILED)
                break;
            TRY (parse_att_value (buf, MODE_ONCE, child_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_attr(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    if (mode == MODE_ONCE)
      {
        ASSERT (child_struct);
        if (res)
          {
            res = link_xml_attr (parent_struct, child_struct);
            if (!res)
                set_error (buf, NO_ERROR_MSG)
          }
        else
            xml_free_attr (child_struct);
      }

    return res;
}


/***************************************************************************/
Bool parse_field (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    XML_ITEM
            *child_struct = NULL;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            child_struct = alloc_xml_item();
            if (!child_struct)
              {
                /* XXX TODO : management of an error message - exhausted memory */
                /* If this happen, we can assume it is THE actual error */
                res = FALSE;
                break;
              }
            res = FALSE;
            TRY (parse_one_char (buf, '<'));
            if (TRY_FAILED)
                break;
            TRY_HANDLER (parse_constant_string (buf, "field"),
                         set_xml_item_name,
                         child_struct,
                         FALSE,
                         NO_ERROR_MSG);
            if (TRY_FAILED)
                break;
            TRY (parse_group_6 (buf, MODE_ONCE_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_constant_string (buf, "/>"));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_field(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    if (mode == MODE_ONCE)
      {
        ASSERT (child_struct);
        if (res)
          {
            res = link_xml_child (parent_struct, child_struct);
            if (!res)
                set_error (buf, NO_ERROR_MSG)
          }
        else
            xml_free (child_struct);
      }

    return res;
}


/***************************************************************************/
Bool parse_key (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    XML_ITEM
            *child_struct = NULL;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            child_struct = alloc_xml_item();
            if (!child_struct)
              {
                /* XXX TODO : management of an error message - exhausted memory */
                /* If this happen, we can assume it is THE actual error */
                res = FALSE;
                break;
              }
            res = FALSE;
            TRY (parse_one_char (buf, '<'));
            if (TRY_FAILED)
                break;
            TRY_HANDLER (parse_constant_string (buf, "key"),
                         set_xml_item_name,
                         child_struct,
                         FALSE,
                         NO_ERROR_MSG);
            if (TRY_FAILED)
                break;
            TRY (parse_group_7 (buf, MODE_ONCE_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_one_char (buf, '>'));
            if (TRY_FAILED)
                break;
            TRY (parse_group_8 (buf, MODE_ONCE_OR_MORE, child_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_constant_string (buf, "</key>"));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_key(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    if (mode == MODE_ONCE)
      {
        ASSERT (child_struct);
        if (res)
          {
            res = link_xml_child (parent_struct, child_struct);
            if (!res)
                set_error (buf, NO_ERROR_MSG)
          }
        else
            xml_free (child_struct);
      }

    return res;
}


/***************************************************************************/
Bool parse_name (XML_BUFFER *buf, int mode)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;
    char
            try_ch;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY_IS (first_name_char);
            if (TRY_FAILED)
                break;
            TRY (parse_group_9 (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_name(buf, MODE_ONCE));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
Bool parse_eq (XML_BUFFER *buf, int mode)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_one_char (buf, '='));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_eq(buf, MODE_ONCE));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
Bool parse_att_value (XML_BUFFER *buf, int mode, XML_ATTR *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    ASSERT (parent_struct);

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_one_char (buf, '\"'));
            if (TRY_FAILED)
                break;
            TRY_HANDLER (parse_any_before_char (buf, '\"', TRUE),
                         set_xml_attr_value,
                         parent_struct,
                         FALSE,
                         NO_ERROR_MSG);
            if (TRY_FAILED)
                break;
            TRY (parse_one_char (buf, '\"'));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_att_value(buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_1 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_record (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_1 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_2 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ONCE_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_attr (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_2 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_3 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_field (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_3 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_4 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ZERO_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_key (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_4 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_5 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ONCE_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_attr (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_5 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_6 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ONCE_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_attr (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_6 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_7 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ONCE_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_attr (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_7 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_8 (XML_BUFFER *buf, int mode, XML_ITEM *parent_struct)
{
    ENABLE_TRY
    int
            hit;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY (parse_white_spaces (buf, MODE_ONCE_OR_MORE));
            if (TRY_FAILED)
                break;
            TRY (parse_field (buf, MODE_ONCE, parent_struct));
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_8 (buf, MODE_ONCE, parent_struct));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


/***************************************************************************/
/* UNION */
Bool parse_group_9 (XML_BUFFER *buf, int mode)
{
    ENABLE_TRY
    int
            hit;
    char
            try_ch;
    Bool
            res;

    switch (mode)
      {
        case MODE_ONCE:
            res = FALSE;
            TRY_IS (name_char);
            if (TRY_FAILED)
                break;
            res = TRUE;
            break;


        default:
            ASSERT ( (mode == MODE_ZERO_OR_MORE)
                  || (mode == MODE_ONCE_OR_MORE)
                  || (mode == MODE_OPTIONAL) );
            hit = 0;
            FOREVER
              {
                TRY(parse_group_9 (buf, MODE_ONCE));
                if (TRY_FAILED)
                    break;

                hit++;

                if (mode == MODE_OPTIONAL)
                    break;
              }
            res = (mode == MODE_ONCE_OR_MORE)
                        ? (hit >= 1)
                        : TRUE;
            break;
      }

    return res;
}


Bool parse_one_char(XML_BUFFER *buf, const char ch)
{
    char
        buf_ch;

    ASSERT (buf);

    get_next_char (buf, buf_ch);

    if (ch == buf_ch)
        return TRUE;
    else
      {
        if (cur_ctxt(buf).pos > buf-> error_ctxt.pos)
            set_error (buf, UNEXPECTED_CHAR);
        return FALSE;
      }
}

Bool parse_constant_string (XML_BUFFER *buf, const char * str)
{
    const char
            *ch;
    char
        buf_ch;

    ASSERT (buf);
    ASSERT (str);

    for (ch = str; *ch; ch++)
      {
        get_next_char (buf, buf_ch);
        if ( buf_ch != *ch)
          {
            set_error (buf, UNEXPECTED_CHAR);
            return FALSE;
          }
      }

    return TRUE;
}


/* returns TRUE if at least one char has been found before 'ch' */
Bool parse_any_before_char (XML_BUFFER *buf, const char ch, Bool allow_empty)
{
    char
        *from,
        *to,
        *found = NULL;
    int
        new_pos,
        new_line;

    ASSERT (buf);
    ASSERT (ch != 0);

    ASSERT (buf-> cache_cur_ptr
                  ? buf-> cache_cur_ptr >= buf-> cache
                  : TRUE);
    ASSERT (buf-> cache_cur_ptr <= buf-> cache_last_ptr);
    if ( !buf-> cache_cur_ptr
            || (buf-> cache_cur_ptr == buf-> cache_last_ptr) )
      {
        handle_page_fault (buf);
        if (!buf-> cache_cur_ptr)
          {
            set_error (buf, UNEXPECTED_EOF);
            return FALSE;
          }
      }

    new_line = cur_ctxt(buf).line;

    /* here, the cache is valid and not consumed */
    FOREVER
      {
        ASSERT (buf-> cache_last_ptr > buf-> cache_cur_ptr);
        found = memchr (buf-> cache_cur_ptr,
                        ch,
                        buf-> cache_last_ptr - buf-> cache_cur_ptr);

        to = found
                ? found
                : buf-> cache_last_ptr;
        ASSERT (buf-> cache_cur_ptr <= to);
        for (from=buf-> cache_cur_ptr; from != to; from++)
            if (*from == '\r')
                new_line++;

        if ( found )
          {
            buf-> cache_cur_ptr = found;
            ASSERT (buf-> cache_cur_ptr >= buf-> cache);
            new_pos = buf-> cache_offset + (buf-> cache_cur_ptr - buf-> cache);
            ASSERT (new_pos >= cur_ctxt(buf).pos);
            if (new_pos == cur_ctxt(buf).pos)
              {
                /* buffer was already pointing to the searched char */
                if (allow_empty)
                    return TRUE;
                else
                  {
                    set_error (buf, UNEXPECTED_CHAR);
                    return FALSE;
                  }
              }
            else
              {
                cur_ctxt(buf).pos  = new_pos;
                cur_ctxt(buf).line = new_line;
                return TRUE;
              }
          }

        /* not in cache */
        buf-> cache_cur_ptr = buf-> cache_last_ptr;  /* causes page fault */
        handle_page_fault (buf);
        if (!buf-> cache_cur_ptr)
          {
            /* we reached EOF or an error occured */
            set_error (buf, UNEXPECTED_EOF);
            return FALSE;
          }
      }
}


Bool parse_any_before_string (XML_BUFFER *buf, const char *str)
{
    char
        *from,
        *to,
        *found;
    int
        length,
        new_pos,
        new_line;

    ASSERT (buf);
    ASSERT (str);
    length = strlen (str);
    ASSERT (length > 0);
    ASSERT (length < HALF_XML_BUFFER_SIZE);  /* XXX needs discussion */

    ASSERT (buf-> cache_cur_ptr
                  ? buf-> cache_cur_ptr >= buf-> cache
                  : TRUE);
    ASSERT (buf-> cache_cur_ptr <= buf-> cache_last_ptr);
    if ( !buf-> cache_cur_ptr
            || (buf-> cache_cur_ptr == buf-> cache_last_ptr) )
      {
        handle_page_fault (buf);
        if (!buf-> cache_cur_ptr)
          {
            set_error (buf, UNEXPECTED_EOF);
            return FALSE;
          }
      }

    new_line = cur_ctxt(buf).line;

    /* here, the cache is valid and not consumed */
    FOREVER
      {
        ASSERT (buf-> cache_last_ptr > buf-> cache_cur_ptr);
        ASSERT (buf-> cache[XML_BUFFER_SIZE] == 0);  /* cache must be terminated
                                                  * like a string */
        found = strstr (buf-> cache_cur_ptr, str);

        to = found
                ? found
                : buf-> cache_last_ptr - length;
        ASSERT (buf-> cache_cur_ptr <= to);
        for (from=buf-> cache_cur_ptr; from != to; from++)
            if (*from == '\r')
                new_line++;

        if ( found )
          {
            buf-> cache_cur_ptr = found;
            ASSERT (buf-> cache_cur_ptr >= buf-> cache);

            new_pos = buf-> cache_offset + (buf-> cache_cur_ptr - buf-> cache);
            ASSERT (new_pos >= cur_ctxt(buf).pos);
            if (new_pos == cur_ctxt(buf).pos)
              {
                set_error (buf, UNEXPECTED_CHAR);
                return FALSE;
              }
            else
              {
                cur_ctxt(buf).pos  = new_pos;
                cur_ctxt(buf).line = new_line;
                return TRUE;
              }
          }

        /* not in cache */
        buf-> cache_cur_ptr = buf-> cache_last_ptr;  /* causes page fault */
        handle_page_fault (buf);
        if (!buf-> cache_cur_ptr)
          {
            /* we reached EOF or an error occured */
            set_error (buf, UNEXPECTED_EOF);
            return FALSE;
          }
        ASSERT (buf-> cache_cur_ptr - buf-> cache == HALF_XML_BUFFER_SIZE);
        buf-> cache_cur_ptr -= length;  /* insure we won't skip str */
      }
}

XML_BUFFER *buf_init_from_file (const char *fullpath, char *error_msg)
{
    #define DISPOSE_AND_RETURN_NULL                                         \
      {                                                                     \
        if (file)                                                           \
            fclose (file);                                                  \
        if (res)                                                            \
          {                                                                 \
            if (res-> cache)                                                \
                mem_free (res-> cache);                                     \
            mem_free (res);                                                 \
          }                                                                 \
        return NULL;                                                        \
      }

    FILE
            *file = NULL;
    XML_BUFFER
            *res = NULL;
    Bool
            error = FALSE;

    ASSERT (fullpath);

    file = file_open(fullpath, 'r');
    if (!file)
      {
        if (error_msg)
            sprintf (error_msg, "Could not open XML file : %s", fullpath);
        DISPOSE_AND_RETURN_NULL
      }
    res = (XML_BUFFER *) mem_alloc (sizeof(XML_BUFFER));
    if (!res)
      {
        if (error_msg)
            strcpy (error_msg, "Not enough memory on the heap");
        DISPOSE_AND_RETURN_NULL
      }

    res-> file_size = get_file_size (fullpath);
    if (res-> file_size <= 0)
        DISPOSE_AND_RETURN_NULL

    res-> cache = (char*) mem_alloc (XML_BUFFER_SIZE+1);
    if (!res-> cache)
      {
        if (error_msg)
            strcpy (error_msg, "Not enough memory on the heap");
        DISPOSE_AND_RETURN_NULL
      }

    res-> cache[XML_BUFFER_SIZE] = 0;  /* for the cache to be a string */
    res->fd = file;
    res-> cache_cur_ptr  = NULL;
    res-> cache_last_ptr = NULL;
    res-> stack          = (XML_CONTEXT *) mem_alloc (STACK_DEPTH * sizeof(XML_CONTEXT));
    res-> stack_depth    = STACK_DEPTH;
    res-> stack_top      = 0;
    cur_ctxt(res).pos    = 0;
    cur_ctxt(res).line   = 0;
    res-> cache_offset   = 0;
    res-> error_ctxt.pos  = -1;
    res-> error_ctxt.line = -1;
    res-> filename  = fullpath;
    res-> error_msg = error_msg;
    res-> error_code = NO_ERROR_MSG;
    if (error_msg)
        error_msg[0];

    return res;

    #undef DISPOSE_AND_RETURN_NULL
}

XML_BUFFER *buf_init_from_string (const char *xmlstring)
{
    #define DISPOSE_AND_RETURN_NULL                                         \
      {                                                                     \
        if (res)                                                            \
          {                                                                 \
            if (res-> cache)                                                \
                mem_free (res-> cache);                                     \
            mem_free (res);                                                 \
          }                                                                 \
        return NULL;                                                        \
      }

    XML_BUFFER
            *res = NULL;
    Bool
            error = FALSE;

    if (!xmlstring || (strlen (xmlstring) == 0))
        return NULL;


    res = (XML_BUFFER *) mem_alloc (sizeof(XML_BUFFER));
    if (!res)
        return NULL;

    /* NO use of strdupl for compatibility (strfree VS mem_free when disposing) */
    res-> cache = (char*) mem_alloc (strlen(xmlstring) + 1);
    if (!res-> cache)
        DISPOSE_AND_RETURN_NULL
    strcpy (res-> cache, xmlstring);

    res-> cache_size = strlen (xmlstring);
    res-> file_size = res-> cache_size;
    res->fd = NULL;
    res-> cache_cur_ptr  = res-> cache;
    res-> cache_last_ptr = &res-> cache[res-> cache_size];
    res-> stack          = (XML_CONTEXT *) mem_alloc (STACK_DEPTH * sizeof(XML_CONTEXT));
    res-> stack_depth    = STACK_DEPTH;
    res-> stack_top      = 0;
    cur_ctxt(res).pos    = 0;
    cur_ctxt(res).line   = 0;
    res-> cache_offset   = 0;
    res-> error_ctxt.pos  = -1;
    res-> error_ctxt.line = -1;
    res-> filename  = NULL;
    res-> error_msg = NULL;
    res-> error_code = NO_ERROR_MSG;

    return res;

    #undef DISPOSE_AND_RETURN_NULL
}


char *get_string (XML_BUFFER *buf, Bool decode_http)
{
    char
        *res = NULL,
        *src,
        *cur,
        *dst;
    int
        offset,
        tab_count,
        size;
    char
        saved;

    ASSERT (buf);
    ASSERT (buf-> stack);
    ASSERT (buf-> stack_top > 0);      /* at least one context saved in stack */
    ASSERT (buf-> stack[buf-> stack_top - 1].pos <= cur_ctxt(buf).pos);
    ASSERT (buf-> cache_cur_ptr);


    src = buf-> cache_cur_ptr - (cur_ctxt(buf).pos - buf-> stack[buf-> stack_top-1].pos);
    saved = *buf-> cache_cur_ptr;
    *buf-> cache_cur_ptr = 0;

    /* 'src' points to the string that will be copied for result.
     *
     * the copy could be different, because
     * 1) CR-LF ----> CR
     * 2) TAB are expanded to space chars.
     *
     * For tab expansion, management is quite heavy, because we have to
     * find the start of the line in order to know how a TAB will be expanded.
     * We'll avoid the TAB expansion management if no TAB characters are
     * present in 'src'  */


    /* For backward compatibility, we don't change original string when buf is
       initialized from string, i.e. that tabs are not expanded, and CR-LFs
       don't change */
    if (!buf-> filename)
      {
        res = (char *) mem_alloc (strlen (src) + 1);
        if (res)
            strcpy (res, src);
        *buf-> cache_cur_ptr = saved;
        return res;
      }

    tab_count = 0;
    for (cur=src; *cur; cur++)
        if (*cur == '\t')
            tab_count++;

    if (tab_count > 0)
      {
        /* 1. We search start of line or TAB before src */
        cur = src;
        offset = 0;
        if (*cur != '\n')
          {
            FOREVER
              {
                cur--;
                if (cur < buf-> cache)
                  {
                    /* we're outside the cache.*/
                    if (buf-> cache_offset == 0)
                        /* we were on the first line of the file */
                        break;
                    else
                      {
                        /* this is a very bad case, but it should be rare */
                        // TODO : reloading cache management !!!
                        ASSERT (FALSE);
                      }
                  }
                else
                  {
                    if ( (*cur == '\t') || (*cur == '\n') )
                        /* we found a char from which we can compute TAB expansion */
                        break;
                    else
                      offset++;
                  }
              }
          }


        /* 2. we allocate the string. We use the src size + tab_count,
         * to be sure the string will be long enough.
         * Note : some extra memory may be allocated, but if we want to compute the
         * exact size, we need an extra pass
         * ONE tab will need TAB_EXPANSION - 1 extra char for space extension
         * ( -1 because the tab char will be REPLACED by a space ) */
        size = strlen(src) + (tab_count * (TAB_EXPANSION-1));
        res = (char *) mem_alloc (size + 1);
        ASSERT (res);       // TO DO : error management
        dst = res;

        /* we copy the source to result, expanding TAB to spaces,
         * and ignoring CR... */
        for (cur=src; *cur; cur++)
          {
            if (*cur != '\r')
              {
                if (*cur == '\t')
                  {
                    memset (dst, ' ', TAB_EXPANSION - (offset%TAB_EXPANSION));
                    dst += TAB_EXPANSION - (offset%TAB_EXPANSION);
                    offset = 0;
                  }
                else
                  {
                    *dst++ = *cur;
                    if (*cur == '\n')
                        offset = 0;
                    else
                        offset++;
                  }
              }
          }
        *dst = 0;
      }
    else
      {
        res = (char *) mem_alloc (strlen (src) + 1);
        ASSERT (res);       // TO DO : error management
        dst = res;

        for (cur=src; *cur; cur++)
            if (*cur != '\r')
                *dst++ = *cur;
        *dst = 0;

        ASSERT (dst >= res);
        ASSERT (strlen(res) == (size_t)(dst - res));
      }

    if (decode_http)
      {
        /* Using directly 'res' as argument is not safe because 'res' is
         * modified by http_decode_meta. */
        src = res;
        http_decode_meta (src, &src, dst-src+1);
      }

    *buf-> cache_cur_ptr = saved;

    return res;
}




void buf_dispose (XML_BUFFER **buf)
{
    ASSERT (buf);

    if (*buf)
      {
        if ((*buf)-> fd)
          {
            if (fclose ((*buf)-> fd) && (*buf)-> error_msg)
                strcpy ((*buf)-> error_msg, "Error closing file");
          }
        if ((*buf)-> stack)
            mem_free ((*buf)-> stack);

        mem_free ( (*buf)-> cache);
        mem_free (*buf);
        *buf = NULL;
      }
}

void handle_page_fault (XML_BUFFER *buf)
{
    long
        offset;
    char
        *half;

    ASSERT (buf);

    if (!buf->filename)
      {
        /*  buffer had been initialized from string
            --> no cache management */
        buf-> cache_cur_ptr = NULL;
        return;
      }


    ASSERT (buf-> fd);

    if (buf-> cache_cur_ptr)
      {
        /* we check whether the entire file has been handled */
        if (buf-> cache_offset + buf-> cache_size == buf-> file_size)
          {
            /* EOF has been reached */
            buf-> cache_cur_ptr = NULL;
            return ;
          }

        /* we keep half of the cache */
        ASSERT (buf-> cache_last_ptr);
        ASSERT (buf-> cache_cur_ptr == buf-> cache_last_ptr);
        /* copy 2nd half of cache on 1st half */
        half = &buf-> cache[HALF_XML_BUFFER_SIZE];
        memcpy (buf-> cache, half, HALF_XML_BUFFER_SIZE);
        /* read 2nd half from file */
        buf-> cache_size = HALF_XML_BUFFER_SIZE
                        + fread (&buf-> cache[HALF_XML_BUFFER_SIZE],
                                 1,
                                 HALF_XML_BUFFER_SIZE,
                                 buf-> fd);

        buf-> cache_cur_ptr = &buf-> cache[HALF_XML_BUFFER_SIZE];
        buf-> cache_last_ptr = &buf-> cache[buf-> cache_size];
        buf-> cache_last_ptr[0] = 0;
        buf-> cache_offset += HALF_XML_BUFFER_SIZE;
      }
    else
      {
        /* cache is not valid at all. */
        ASSERT (cur_ctxt(buf).pos <= buf-> file_size);

        if (cur_ctxt(buf).pos == buf-> file_size)  /* End of file */
          {
            buf-> cache_cur_ptr = NULL;
            return;
          }

        offset = cur_ctxt(buf).pos - HALF_XML_BUFFER_SIZE;
        offset = max (offset, 0);
        fseek (buf-> fd, offset, SEEK_SET);

        buf-> cache_size = fread (buf-> cache, 1, XML_BUFFER_SIZE, buf-> fd);

        buf-> cache_last_ptr = &buf-> cache[buf-> cache_size];
        buf-> cache_last_ptr[0] = 0;
        buf-> cache_offset = offset;
        buf-> cache_cur_ptr = buf-> cache + (cur_ctxt(buf).pos - offset);
      }

    if ( buf-> cache_size < XML_BUFFER_SIZE )
      {
        ASSERT (buf-> cache_offset + buf-> cache_size <= buf-> file_size);
        if (buf-> cache_offset + buf-> cache_size < buf-> file_size)
            /* fread failed for some unknown reason (not EOF)
             * --> we invalidate the cache */
            buf-> cache_cur_ptr = NULL;
      }
}

char *build_error_message  (XML_BUFFER *buf)
{
    char
        *start,
        *end,
        saved;
    int
        length;

    ASSERT (buf);
    ASSERT (buf-> error_code != NO_ERROR_MSG);

    if (!buf-> error_msg)
        return NULL;

    buf-> error_msg[0] = 0;

    ASSERT (buf-> fd);

    /* we force the cache to be reloaded with current error line */
    buf-> error_ctxt.pos --;  /* XXX due to get_next_char who increments cache_cur_ptr */
    memcpy ( &cur_ctxt(buf), &buf-> error_ctxt, sizeof (XML_CONTEXT));
    buf-> cache_cur_ptr = NULL;
    handle_page_fault (buf);

    if (buf-> cache_cur_ptr)
      {
        /* we search the start of the line containing the error */
        ASSERT (buf-> cache_cur_ptr >= buf-> cache);
        start = buf-> cache_cur_ptr - 1;
        while ( (start >= buf-> cache)
                    && (*start != '\r')
                    && (*start != '\n') )
            start--;

        start++;
        ASSERT (start >= buf-> cache);
        ASSERT (start < buf-> cache_last_ptr);

        end = buf-> cache_cur_ptr;
        while ( (end < buf-> cache_last_ptr)
                && (*end != '\r')
                && (*end != '\n') )
            end++;

        saved = *end;
        *end = 0;
        /* XXX todo : error code management */
        sprintf (buf-> error_msg, "%s (%i): %s\n",
                    buf-> filename,
                    cur_ctxt(buf).line + 1,
                    start );
        *end = saved;
      }

    length = strlen (buf-> error_msg);
    ASSERT (length >= 0);
    sprintf (&buf-> error_msg[length], error_messages[buf-> error_code]);

    /* XXXXX  just for debug purpoises */
    if (buf-> cache_cur_ptr)
      {
        length = strlen (buf-> error_msg);
        sprintf (&buf-> error_msg[length], "\nFAULTY CHAR    : %c", *buf-> cache_cur_ptr);
        length = strlen (buf-> error_msg);
        saved = *end;
        *end = 0;
        sprintf (&buf-> error_msg[length], "\nFAULTY STRING : %s", buf-> cache_cur_ptr);
        *end = saved;
      }

    return buf-> error_msg;
}


