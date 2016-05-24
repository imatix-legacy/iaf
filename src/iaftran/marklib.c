/*  ----------------------------------------------------------------<Prolog>-
    Name:       marklib.c
    Title:      Markup Library Functions
    Package:    Markup Translator

    Written:    1999/11/11  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/01/06  Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to read and write markup files, and
                manipulate markup data in memory as list structures.

    Copyright:  Copyright (c) 1991-2000 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"
#include "markdict.h"
#include "marklib.h"
#include "markhtml.h"
#include "markfile.h"

#define BUFFER_SIZE 10240

#define FILE_TAG_BEGIN "\"!"
#define FILE_TAG_END   "!\""

typedef struct _attr_string
{
  struct _attr_string
      *next, *prev;                     /* Linked list poiter                */
  char *begin;                          /* Begin string pointer              */
  char *end;                            /* End string pointer                */
} ATTR_STRING;


typedef struct _language_desc
{
    char
        *code,
        *name,
        *charset;
    qbyte
        codepage;
} language_desc;



#define MAX_LANG                 (30)

static language_desc language_table[MAX_LANG] = {
  {"bg"    ,"Bulgarian"          ,""              ,    0 },
  {"ca"    ,"Catalan"            ,"iso-8859-1"    , 1252 },
  {"cs"    ,"Czech"              ,""              ,    0 },
  {"da"    ,"Danske"             ,"windows-1257"  , 1257 },
  {"de"    ,"Deutsch"            ,"iso-8859-1"    , 1252 },
  {"el"    ,"Greek"              ,"windows-1253"  , 1253 },
  {"en"    ,"English"            ,"iso-8859-1"    , 1252 },
  {"en-uk" ,"English UK"         ,"iso-8859-1"    , 1252 },
  {"es"    ,"Español"            ,"iso-8859-1"    , 1252 },
  {"eu"    ,"Basque"             ,"iso-8859-1"    , 1252 },
  {"fi"    ,"Finnish"            ,"windows-1257"  , 1257 },
  {"fr"    ,"Français"           ,"iso-8859-1"    , 1252 },
  {"fr-be" ,"Français belgique"  ,"iso-8859-1"    , 1252 },
  {"fy"    ,"Frisian"            ,"iso-8859-1"    , 1252 },
  {"ga"    ,"Irish"              ,"iso-8859-1"    , 1252 },
  {"he"    ,"Hebrew"             ,"iso-8859-8"    , 1255 },
  {"it"    ,"Italiano"           ,"iso-8859-1"    , 1252 },
  {"ja"    ,"Japanese"           ,"shift_jis"     ,  932 },
  {"ko"    ,"Korean"             ,"ks_c_5601"     ,  949 },
  {"nl"    ,"Nederlands"         ,"iso-8859-1"    , 1252 },
  {"nl-be" ,"Nederlands belgie"  ,"iso-8859-1"    , 1252 },
  {"no"    ,"Norske"             ,"windows-1257"  , 1257 },
  {"pl"    ,"Polish"             ,""              ,    0 },
  {"pt"    ,"Português"          ,"iso-8859-1"    , 1252 },
  {"ro"    ,"Romanian"           ,""              ,    0 },
  {"ru"    ,"Russian"            ,""              ,    0 },
  {"sk"    ,"Slovak"             ,""              ,    0 },
  {"sl"    ,"Slovenian"          ,""              ,    0 },
  {"sv"    ,"Svenske"            ,"windows-1257"  , 1257 },
  {"zh"    ,"Chinese"            ,""              ,    0 }
};

/*- Local variables ---------------------------------------------------------*/

static char
    buffer [BUFFER_SIZE + 1];

/*- Local functions ---------------------------------------------------------*/

static int          tag_save               (FILE *markup_file, MARKUP_TAG *tag);
static int          get_non_empty_data     (MARKUP_TAG *tag, int idx, Bool before,
                                            Bool link, MARKUP_DEF *table);
static void         merge_data             (MARKUP_TAG *tag, int first, int last, Bool check_link,
                                            MARKUP_DEF *table);
static void         merge_child_value      (MARKUP_TAG *tag, char **buf, int first_order);
static Bool         check_skip_tag         (MARKUP_TAG *tag, MARKUP_DEF *table);
static Bool         check_link_tag         (MARKUP_TAG *tag, MARKUP_DEF *table);
static MARKUP_DATA *tag_first_data_in_tree (MARKUP_TAG *tag);

/* ATTR_STRING function                                                      */
static ATTR_STRING *  alloc_attr_string    (char *begin, char *end);
static void           free_attr_string     (ATTR_STRING *string);
static void           free_all_attr_string (LIST *string_list);

static LIST *         get_string_list      (char *value);
static language_desc *get_lang_desc        (const char *code);
static char *         search_tran_char     (char *string, Bool first);

/*  ---------------------------------------------------------------------[<]-
    Function: tag_new

    Synopsis: Creates and initialises a new MARKUP_TAG tag with a specified
    parent tag.  Returns the address of the created MARKUP_TAG tag or NULL
    if there was not enough memory.  Sets the new tag's name and value as
    specified; only one of these should contain a value.
    If the name is non-NULL this is a child
    node; if the value is non-NULL then this is a value node.  If the
    parent argument is non-NULL, attaches the new tag to the end of the
    parent tag list.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tag_new (
    MARKUP_TAG *parent,
    const int   id,
    const int   order,
    const char *value)
{
    MARKUP_TAG
        *tag;

    list_create (tag, sizeof (MARKUP_TAG));
    if (tag)
      {
        list_reset (&tag-> data);
        list_reset (&tag-> children);
        tag-> parent   = parent;
        tag-> tag_id   = id;
        tag-> order    = order;
        tag-> nb_items = 0;
        tag-> value    = mem_strdup (value);
        if (parent)
            list_relink_before (tag, &parent-> children);

        return (tag);
      }
    else
        return (NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_insert

    Synopsis: Creates and initialises a new MARKUP_TAG tag with a
    specified sibling.  The new tag is inserted after the specified
    sibling; to add a new child after all existing children, use tag_new.
    Returns the address of the created MARKUP_TAG tag or NULL if there was
    not enough memory.  Sets the new tag's name and value as specified;
    only one of these should contain a value, although sfltag will not
    complain if both do.  If the name is non-NULL this is a child node; if
    the value is non-NULL then this is a value node.  If the parent
    argument is non-NULL, attaches the new tag to the end of the parent
    tag list.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tag_insert (
    MARKUP_TAG *sibling,
    const int   id,
    const int   order,
    const char *value)
{
    MARKUP_TAG
        *tag;

    list_create (tag, sizeof (MARKUP_TAG));
    if (tag)
      {
        list_reset (&tag-> data);
        list_reset (&tag-> children);
        tag-> parent   = tag_parent (sibling);
        tag-> tag_id   = id;
        tag-> order    = order;
        tag-> nb_items = 0;
        tag-> value    = mem_strdup (value);
        list_relink_after (tag, sibling);

        return (tag);
      }
    else
        return (NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_modify_value

    Synopsis: Modifies an existing XML tag's value.
    ---------------------------------------------------------------------[>]-*/

void
tag_modify_value  (MARKUP_TAG *tag, const char *value)
{
    ASSERT (tag);

    if (!tag-> value)
        tag-> value = mem_strdup (value);
    else
        if (! value || (strneq (value, tag-> value)))
          {
            mem_free (tag-> value);
            tag-> value = mem_strdup (value);
          }
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_id

    Synopsis: Extracts the ID of a specified tag.
    ---------------------------------------------------------------------[>]-*/

int
tag_id (MARKUP_TAG *tag)
{
    ASSERT (tag);

    return tag-> tag_id;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_order

    Synopsis: Extracts the order of a specified tag.
    ---------------------------------------------------------------------[>]-*/

int
tag_order (MARKUP_TAG *tag)
{
    ASSERT (tag);

    return tag-> order;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_value

    Synopsis: Extracts the value from a value node.  These are recognised
    by their name being NULL.  The returned string should NOT be modified.
    To manipulate it, first make a copy first.
    ---------------------------------------------------------------------[>]-*/

char *
tag_value (MARKUP_TAG *tag)
{
    ASSERT (tag);

    return tag-> value;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_free

    Synopsis: Frees all memory used by an MARKUP_TAG tag and its children.
    ---------------------------------------------------------------------[>]-*/

void
tag_free (MARKUP_TAG *tag)
{
    ASSERT (tag);

    /*  Free data nodes for the tag                                    */
    while (!list_empty (&tag-> data))
        tag_free_data (tag-> data.next);

    /*  Free child nodes for the tag                                        */
    while (!list_empty (&tag-> children))
        tag_free (tag-> children.next);

    /*  Now free this tag itself                                            */
    list_unlink (tag);                 /*  Unlink from its parent list      */
    if (tag-> value)
        mem_free (tag-> value);
    mem_free (tag);                    /*  And free the tag itself         */
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_attach

    Synopsis: Attaches an XML tag as the child of a given parent.  If the
    tag is already attached to a parent, it is first removed.
    ---------------------------------------------------------------------[>]-*/

void
tag_attach (
    MARKUP_TAG *parent,
    MARKUP_TAG *tag)
{
    if (tag-> parent)
        tag_detach (tag);

    tag-> parent = parent;
    if (parent)
        list_relink_before (tag, &parent-> children);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_detach

    Synopsis: Removes an XML tag from the tree.
    ---------------------------------------------------------------------[>]-*/

void
tag_detach (
    MARKUP_TAG *tag)
{
    tag-> parent = NULL;
    list_unlink (tag);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_first_child

    Synopsis: Returns the first child node of the specified tag, or NULL
    if there are none.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tag_first_child (MARKUP_TAG *tag)
{
    ASSERT (tag);

    if (!list_empty (&tag-> children))
        return tag-> children. next;
    else
        return NULL;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_next_sibling

    Synopsis: Returns the next sibling of the specified tag, or NULL if there
    if are none.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tag_next_sibling (MARKUP_TAG *tag)
{
    ASSERT (tag);

    if ((LIST *) tag-> next != & tag-> parent-> children)
        return tag-> next;
    else
        return NULL;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_parent

    Synopsis: Returns the parent of the specified tag, or NULL if this is
    the root tag.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tag_parent (MARKUP_TAG *tag)
{
    ASSERT (tag);

    return (tag-> parent);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_parent

    Synopsis: Returns the tag fof the specified order, or NULL.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tag_get (MARKUP_TAG *tag, int order)
{
    MARKUP_TAG
        *feedback = NULL,
        *child;

    ASSERT (tag);

    for (child  = tag_first_child (tag);
         child != NULL;
         child  = tag_next_sibling (child))
      {
        if (tag_order (child) == order)
          {
            feedback = child;
            break;
          }
      }

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: tag_put_data

    Synopsis: Sets, modifies, or deletes an data for the
    specified tag.
    ---------------------------------------------------------------------[>]-*/

int
tag_put_data (
    MARKUP_TAG   *item,
    const char   *value,
    const short   script)
{
    int
        feedback = 0;
    MARKUP_DATA
        *data;

    ASSERT (item);

    if (value)                      /*  Value specified - update data    */
      {
         list_create (data, sizeof (MARKUP_DATA));
         if (data)
           {
              data-> order    = ++item-> nb_items;
              data-> value    = mem_strdup (value);
              data-> parent   = item;
              data-> dico_id  = 0;
              data-> script   = script;
              data-> nbr_link = 0;
              list_reset (&data-> link);

              list_relink_before (data, &item-> data);
              feedback = 1;
           }
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_insert_data

    Synopsis: Sets, modifies, or deletes an data for the
    specified tag.
    ---------------------------------------------------------------------[>]-*/

int
tag_insert_data (
    MARKUP_TAG   *item,
    const int     order,
    const char   *value,
    const short   script)
{
    int
        feedback = 0;
    MARKUP_DATA
        *data;

    ASSERT (item);

    if (value)                      /*  Value specified - update data    */
      {
         list_create (data, sizeof (MARKUP_DATA));
         if (data)
           {
              data-> order    = order;
              data-> value    = mem_strdup (value);
              data-> parent   = item;
              data-> dico_id  = 0;
              data-> script   = script;
              data-> nbr_link = 0;
              list_reset (&data-> link);

              list_relink_before (data, &item-> data);
              feedback = 1;
           }
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_data_value

    Synopsis: Extracts the value of a specified tag data.  The returned string
    should NOT be modified.  To manipulate it, first make a copy first.
    ---------------------------------------------------------------------[>]-*/

char *
tag_data_value (MARKUP_DATA *data)
{
    ASSERT (data);

    return data-> value;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_data_value

    Synopsis: Extracts the order of a specified tag data.
    ---------------------------------------------------------------------[>]-*/

int
tag_data_order (MARKUP_DATA *data)
{
    ASSERT (data);

    return data-> order;
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_data_is_empty

    Synopsis: Check if one significant character is in data value.
    ---------------------------------------------------------------------[>]-*/

Bool
tag_data_is_empty (MARKUP_DATA *data)
{
    Bool
        feedback = TRUE;
    char
        *val;

    ASSERT (data);

    if (data-> value)
      {
        val = data-> value;
        http_decode_meta (buffer, &val, BUFFER_SIZE);

        for (val = buffer; *val; val++)
          {
            if (isalpha (*val))
              {
                feedback = FALSE;
                break;
              }
          }
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_free_data

    Synopsis: Frees all memory used by an MARKUP_DATA node.
    ---------------------------------------------------------------------[>]-*/

void
tag_free_data (
    MARKUP_DATA *data)
{
    ASSERT (data);

    list_unlink (data);

    /*  Free link nodes for the data                                         */
    while (!list_empty (&data-> link))
        tag_free_link (data-> link.next);

    if (data-> value)
        mem_free (data-> value);
    mem_free (data);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_first_data

    Synopsis: Returns the first data of a specified TAG item, or NULL
    if there are none.
    ---------------------------------------------------------------------[>]-*/

MARKUP_DATA *
tag_first_data (MARKUP_TAG *item)
{
    ASSERT (item);

    if (!list_empty (&item-> data))
        return item-> data. next;
    else
        return (NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_next_data

    Synopsis: Returns the next data following the specified data,
    or NULL if there are none.
    ---------------------------------------------------------------------[>]-*/

MARKUP_DATA *
tag_next_data (MARKUP_DATA *data)
{
    ASSERT (data);

    if ((LIST *) data-> next != & data-> parent-> data)
        return (data-> next);
    else
        return (NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_get_data

    Synopsis: Returns the data for the specified,
    or NULL if there are none.
    ---------------------------------------------------------------------[>]-*/

MARKUP_DATA *
tag_get_data (MARKUP_TAG *tag, const int order)
{
    MARKUP_DATA
        *feedback = NULL,
        *data;

    ASSERT (tag);

    FORDATA (data, tag)
      {
        if (tag_data_order (data) == order)
          {
            feedback = data;
            break;
          }
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_new_link

    Synopsis: Add a new link value in list in markup tag.
    ---------------------------------------------------------------------[>]-*/

int
tag_new_link (MARKUP_DATA *data, char *value)
{
    int
        feedback = 0;

    MARKUP_LINK
        *link;

    ASSERT (data);

    if (value)                      /*  Value specified - update data    */
      {
         list_create (link, sizeof (MARKUP_LINK));
         if (link)
           {
              link-> order    = ++data-> nbr_link;
              link-> value    = mem_strdup (value);
              link-> parent   = data;
              list_relink_before (link, &data-> link);
              feedback = 1;
           }
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: tag_free_link

    Synopsis: Frees all memory used by an MARKUP_LINK node.
    ---------------------------------------------------------------------[>]-*/

void
tag_free_link (MARKUP_LINK *link)
{
    ASSERT (link);

    list_unlink (link);

    if (link-> value)
        mem_free (link-> value);

    mem_free (link);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_first_link

    Synopsis: Get first link value of data item.
    ---------------------------------------------------------------------[>]-*/

MARKUP_LINK *
tag_first_link (MARKUP_DATA  *data)
{
    ASSERT (data);

    if (!list_empty (&data-> link))
        return (data-> link. next);
    else
        return (NULL);
}

/*  ---------------------------------------------------------------------[<]-
    Function: tag_next_link

    Synopsis: Get next link value.
    ---------------------------------------------------------------------[>]-*/

MARKUP_LINK *
tag_next_link (MARKUP_LINK  *link)
{
    ASSERT (link);

    if (link-> parent
    &&  (LIST *) link-> next != & link-> parent-> link)
        return (link-> next);
    else
        return (NULL);
}

/*  ---------------------------------------------------------------------[<]-
    Function: tag_get_link

    Synopsis: Get link for the specified order.
    ---------------------------------------------------------------------[>]-*/

MARKUP_LINK *
tag_get_link (MARKUP_DATA *data, const int order)
{
    MARKUP_LINK
        *feedback = NULL,
        *link;

    ASSERT (data);

    FORLINK (link, data)
      {
        if (link-> order == order)
          {
            feedback = link;
            break;
          }
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: markup_save

    Synopsis: Saves an TAG tree to the specified file.  Returns the number
    of tags saved, or -1 if there was an error.
    ---------------------------------------------------------------------[>]-*/

int
markup_save (
    MARKUP_TAG   *tag,
    const char   *filename)
{
    FILE
        *tagfile;                       /*  XML output stream                */
    int
        count;                          /*  How many symbols did we save?    */

    ASSERT (tag);
    ASSERT (filename);

    if ((tagfile = file_open (filename, 'w')) == NULL)
        return (-1);                    /*  No permission to write file      */

    /*  Output TAG root                                                      */
    count = tag_save (tagfile, tag);

    file_close (tagfile);
    return (count);
}

static int
tag_save (FILE *markup_file, MARKUP_TAG *tag)
{
    int
        index,
        count = 1;                      /*  Count 1 for current item         */
    MARKUP_TAG
        *child = NULL;
    MARKUP_DATA
        *data;

    if (tag-> value)
        fprintf (markup_file, "%s", tag-> value);

    for (index = 1; index <= tag-> nb_items; index++)
      {
        data  = tag_get_data (tag, index);
        if (data)
          {
            if (data-> value)
                fprintf (markup_file, "%s", data-> value);
            count++;
          }
        else
          {
            child = tag_get (tag, index);
            if (child)
                count += tag_save (markup_file, child);
          }
      }

    return (count);
}

/*  ---------------------------------------------------------------------[<]-
    Function: update_dictionary

    Synopsis: Update dictionary with TAG MARKUP.
    ---------------------------------------------------------------------[>]-*/

void
update_dictionary (DICT_CTX *dict, MARKUP_TAG *tag, char *default_lang,
                   char *file_name)
{
    int
        index;
    MARKUP_TAG
        *child = NULL;
    MARKUP_LINK
        *link;
    MARKUP_DATA
        *data;
    long
        item_id;

    if (dict && tag)
      {
        for (index = 1; index <= tag-> nb_items; index++)
          {
            data = tag_get_data (tag, index);
            if (data)
              {
                if (!tag_data_is_empty (data))
                  {
                    item_id = get_dictionary_item (data-> value, file_name, dict);
                    if (item_id < 0)
                        item_id = add_dictionary_item (data-> value,
                                                       default_lang, dict);
                    if (item_id > 0)
                      {
                        update_usage (file_name, item_id, 0, dict);
                        FORLINK (link, data)
                            update_item_link (link-> value, link-> order, item_id, dict);
                      }
                  }
              }
            else
              {
                child = tag_get (tag, index);
                if (child)
                    update_dictionary (dict, child, default_lang, file_name);
              }
          }
      }
}

static ATTR_STRING *
alloc_attr_string (char *begin, char *end)
{
    ATTR_STRING
        *feedback = NULL;

    feedback = mem_alloc (sizeof (ATTR_STRING));
    if (feedback)
      {
        memset (feedback, 0, sizeof (ATTR_STRING));
        list_reset (feedback);
        feedback-> begin = begin;
        feedback-> end   = end;
      }
    return (feedback);
}

static void
free_attr_string (ATTR_STRING *string)
{
    ASSERT (string);

    list_unlink (string);
    mem_free (string);
}

static void
free_all_attr_string (LIST *string_list)
{
    while (!list_empty (string_list))
        free_attr_string (string_list-> next);
    mem_free (string_list);
}

/*  ---------------------------------------------------------------------[<]-
    Function: tag_add_string_attr

    Synopsis: Add in data tree all string attribute for tag with flag
              parse_string .
    ---------------------------------------------------------------------[>]-*/

void
tag_add_string_attr (MARKUP_TAG *tag, MARKUP_DEF *table)
{
    MARKUP_TAG
        *child;
    MARKUP_DATA
        *data;
    LIST
        *list;
    ATTR_STRING
        *first,
        *last,
        *next,
        *attr_string;
    char
        *last_char,
        *value,
        val;
    int
        index,
        nbr_string;

    if ((table + tag-> tag_id)-> parse_string == TRUE
    && tag-> value [0] == '<')          /*  PH 2000/01/21                    */
      {
        list = get_string_list (tag-> value);
        if (list)
          {
            first = (ATTR_STRING *)list-> next;
            last  = (ATTR_STRING *)list-> prev;
            last_char = &tag-> value [strlen (tag-> value) - 1];
            /* Count number of string                                       */
            nbr_string = 0;
            attr_string = (ATTR_STRING *)list-> next;
            while ((void *)attr_string != (void *)list)
              {
                next = attr_string-> next;
                nbr_string++;
                attr_string = next;
              }
            nbr_string += nbr_string - 1;
            if (last-> end != last_char)
                nbr_string++;
            /* Move position of next tag and data in tag                     */
            for (index = tag-> nb_items; index > 0; index--)
              {
                child = tag_get (tag, index);
                if (child)
                    child-> order += nbr_string;
                else
                  {
                     data  = tag_get_data (tag, index);
                     if (data)
                         data-> order += nbr_string;
                  }
              }
            tag-> nb_items += nbr_string;
            /* Create new tag and data structure                             */
            value = tag-> value;
            val = *first-> begin;
            *first-> begin = '\0';
            tag-> value = mem_strdup (value);
            *first-> begin = val;
            index = 1;
            attr_string = (ATTR_STRING *)list-> next;
            while ((void *)attr_string != (void *)list)
              {
                next = attr_string-> next;
                val = *attr_string-> end;
                *attr_string-> end = '\0';
                tag_insert_data (tag, index++, attr_string-> begin, FALSE);
                *attr_string-> end = val;
                if ((void *)next != (void *)list)
                  {
                    val = *next-> begin;
                    *next-> begin = '\0';
                    tag_new (tag, tag-> tag_id, index++, attr_string-> end);
                    *next-> begin = val;
                  }
                attr_string = next;
              }
            if (last-> end != last_char)
                tag_new (tag, tag-> tag_id, index++, last-> end);

            free_all_attr_string (list);
            mem_free (value);
          }
      }
    FORTAG (child, tag)
        tag_add_string_attr (child, table);
}


/*  Return a list of quoted strings found within the string value.
 *  If a string contains no letters, it is ignored.  Any non-alpha
 *  characters, or '&nbsp;' at the start or end of the string is ignored.
 */

static LIST *
get_string_list (char *value)
{
    LIST
        *list = NULL;                   /*  Returned list of strings         */
    ATTR_STRING
        *attr_string;
    char
        old_val,                        /*  Old value of a char              */
        *script_tag,                    /*  Pointer to script tag            */
        *string_start,                  /*  First char in string             */
        *string_top,                    /*  Char after string, usually "     */
        *quote_posn,                    /*  Scan for quote in string         */
        *char_ptr;                      /*  Scan through string itself       */

    quote_posn = strchr (value, '"');
    while (quote_posn)
      {
        string_start = quote_posn + 1;
        quote_posn   = strchr (string_start, '"');
        if (quote_posn)
          {
            /*  Drop trailing whitespace                                     */
            string_top = quote_posn;
            while (string_top > string_start && !isalnum (string_top [-1]))
              {
                string_top--;
                if ((string_top - string_start > 5)
                &&  memcmp (string_top - 5, "&nbsp;", 5) == 0)
                    string_top -= 5;
              }
            /*  Find first letter in string                                  */
            while (strprefixed (string_start, "&nbsp;"))
                string_start += 6;
            for (char_ptr = string_start; char_ptr < string_top; char_ptr++)
                if (isalnum (*char_ptr))
                    break;

            /*  Reject empty and non-text strings                            */
            if (char_ptr != string_top)
              {
                /* Reject string with ASP script                             */
                old_val = *string_top;
                *string_top = '\0';
                script_tag  = strstr (string_start, "<%");
                *string_top = old_val;

                if (script_tag == NULL)
                  {
                    /*  Allocate list if necessary (first string)            */
                    if (list == NULL)
                      {
                        list = mem_alloc (sizeof (LIST));
                        list_reset (list);
                      }
                    attr_string = alloc_attr_string (string_start, string_top);
                    if (attr_string)
                        list_relink_after (list, attr_string);
                  }
              }
            quote_posn = strchr (quote_posn + 1, '"');
          }
      }
    return (list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_del_skipped

    Synopsis: Remove all tag to skip (ex in HTML: B, I, U EM).
    ---------------------------------------------------------------------[>]-*/

void
tag_del_skipped   (MARKUP_TAG *tag, MARKUP_DEF *table)
{
    int
        first_data,
        last_data,
        index;
    MARKUP_TAG
        *child = NULL;

    ASSERT (tag);
    ASSERT (table);

    for (index = 1; index <= tag-> nb_items; index++)
      {
        child = tag_get (tag, index);
        if (child)
          {
            if (child-> tag_id >= 0
            &&  table [child-> tag_id].skip == TRUE)
              {
                first_data = get_non_empty_data (tag, index, TRUE,  FALSE, table);
                last_data  = get_non_empty_data (tag, index, FALSE, FALSE, table);
                if (first_data != last_data)
                  {
                    merge_data (tag, first_data, last_data, FALSE, table);
                    index = first_data + 1;
                  }
              }
            else
            if (child-> nb_items > 0)
                tag_del_skipped   (child, table);
          }
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: tag_remove_link

    Synopsis: Remove all link tag and insert tag into data.
    ---------------------------------------------------------------------[>]-*/

void
tag_remove_link (MARKUP_TAG *tag, MARKUP_DEF *table)
{
    int
        first_data,
        last_data,
        index;
    MARKUP_TAG
        *child = NULL;

    ASSERT (tag);
    ASSERT (table);

    for (index = 1; index <= tag-> nb_items; index++)
      {
        child = tag_get (tag, index);
        if (child)
          {
            if (child-> tag_id >= 0
            &&  (table + child-> tag_id)-> link == TRUE)
              {
                first_data = get_non_empty_data (tag, index, TRUE,  TRUE, table);
                last_data  = get_non_empty_data (tag, index, FALSE, TRUE, table);
                if (first_data != last_data)
                  {
                    merge_data (tag, first_data, last_data, TRUE, table);
                    index = first_data + 1;
                  }
              }
            else
            if (child-> nb_items > 0)
                tag_remove_link (child, table);
          }
      }
}

static int
get_non_empty_data (MARKUP_TAG *tag, int idx, Bool before,  Bool check_link,
                    MARKUP_DEF *table)
{
    MARKUP_DATA
        *data;
    MARKUP_TAG
        *child = NULL;
    int
        nb_items,
        feedback = idx,
        index;

    nb_items = tag-> nb_items;
    index = idx;
    while (index > 0
    &&     index <= nb_items
    &&     feedback == idx)
      {
        data  = tag_get_data (tag, index);
        if (data)
          {
            if (tag_data_is_empty (data) == FALSE)
                feedback = index;
          }
        else
          {
            child = tag_get (tag, index);
            if (child)
              {
                if (check_link == TRUE)
                  {
                    if (child-> tag_id >= 0
                    &&  (table + child-> tag_id)-> skip == FALSE
                    &&  (table + child-> tag_id)-> link == FALSE)
                        break;
                    else
                    if (check_link_tag (child, table) == FALSE)
                        break;
                    else
                    if (tag_first_data_in_tree (child) == NULL)
                        break;
                  }
                else
                  {
                    if (child-> tag_id >= 0
                    &&  (table + child-> tag_id)-> skip == FALSE)
                        break;
                    else
                    if (check_skip_tag (child, table) == FALSE)
                        break;
                    else
                    if (tag_first_data_in_tree (child) == NULL)
                        break;
                 }
              }
          }
        if (before)
            index--;
        else
            index++;
      }

    return (feedback);
}

static void
merge_data (MARKUP_TAG *tag, int first, int last, Bool check_link,
            MARKUP_DEF *table)
{
    MARKUP_DATA
        *first_data = NULL,
        *data;
    MARKUP_TAG
        *child = NULL;
    int
        count,
        index;
    char
        temp [20],
        *value,
        *buf;

    buf   = buffer;
    count = last - first;

    for (index = first; index <= last; index++)
      {
        data  = tag_get_data (tag, index);
        if (data)
          {
            value = data-> value;
            while (*value)
                *buf++ = *value++;
            if (index == first)
                first_data = data;
            else
                tag_free_data (data);
          }
        else
          {
            child = tag_get (tag, index);
            if (child)
              {
                if (index != first)
                  {
                    if (check_link
                    &&  child-> tag_id >= 0
                    &&  (table + child-> tag_id)-> link == TRUE
                    &&   first_data)
                      {
                        tag_new_link (first_data, child-> value);
                        sprintf (temp, "<a%d>", first_data-> nbr_link);
                        value = temp;
                      }
                    else
                        value = child-> value;
                    while (*value)
                        *buf++ = *value++;
                    merge_child_value (child, &buf, 1);
                    tag_free (child);
                  }
                else
                  {
                    first_data = tag_first_data_in_tree (child);
                    merge_child_value (child, &buf, first_data->order);
                    /*  Free child nodes for the tag                         */
                    while (!list_empty (&first_data->parent-> children))
                        tag_free (first_data->parent-> children.next);
                  }
              }
          }
      }
    *buf = '\0';

    for (index = last + 1; index <= tag-> nb_items; index++)
      {
        data  = tag_get_data (tag, index);
        if (data)
            data-> order -= count;
        else
          {
            child = tag_get (tag, index);
            if (child)
                child-> order -= count;
          }
      }

    if (first_data)
      {
        if (first_data-> value)
            mem_strfree (&first_data-> value);
        first_data-> value = mem_strdup (buffer);
        tag-> nb_items -= count;
      }

}

static MARKUP_DATA *
tag_first_data_in_tree (MARKUP_TAG *tag)
{
    MARKUP_DATA
        *data = NULL;
    MARKUP_TAG
        *child;

    data = tag_first_data (tag);
    if (data == NULL)
      {
        child = tag_first_child (tag);
        if (child)
            data = tag_first_data_in_tree (child);
      }
    return (data);
}


static void
merge_child_value  (MARKUP_TAG *tag, char **buf, int first_order)
{
    MARKUP_DATA
        *data;
    MARKUP_TAG
        *child;
    char
        *value;
    int
        index;

    for (index = first_order; index <= tag-> nb_items; index++)
      {
        data = tag_get_data (tag, index);
        if (data)
          {
            value = data-> value;
            while (*value)
                *(*buf)++ = *value++;
          }
        else
          {
            child = tag_get (tag, index);
            if (child)
              {
                value = child-> value;
                while (*value)
                    *(*buf)++ = *value++;
                merge_child_value (child, buf, 1);
              }
          }
      }
}

/*  -------------------------------------------------------------------------
    Function: check_skip_tag

    Synopsis: Check if all tag in tree is skiped tag.
    -------------------------------------------------------------------------*/

static Bool
check_skip_tag (MARKUP_TAG *tag, MARKUP_DEF *table)
{
    Bool
        feedback = TRUE;
    MARKUP_TAG
        *child;

    for (child  = tag_first_child (tag);
         child != NULL && feedback == TRUE;
         child  = tag_next_sibling (child))
      {
        if (child-> tag_id >= 0
        &&  (table + child-> tag_id)-> skip == FALSE)
          {
            feedback = FALSE;
            break;
          }
        else
            feedback = check_skip_tag (child, table);
      }
    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: check_link_tag

    Synopsis: Check if all tag in tree is link tag.
    -------------------------------------------------------------------------*/

static Bool
check_link_tag (MARKUP_TAG *tag, MARKUP_DEF *table)
{
    Bool
        feedback = TRUE;
    MARKUP_TAG
        *child;

    for (child  = tag_first_child (tag);
         child != NULL && feedback == TRUE;
         child  = tag_next_sibling (child))
      {
        if (child-> tag_id >= 0
        &&  (table + child-> tag_id)-> link == FALSE
        &&  (table + child-> tag_id)-> skip == FALSE)
          {
            feedback = FALSE;
            break;
          }
        else
            feedback = check_link_tag (child, table);
      }
    return (feedback);
}



/*  -------------------------------------------------------------------------
    Function: search_tran_char

    Synopsis: Search the begin or the end of string to made translation (bypass
              non alpha numeric character)
    -------------------------------------------------------------------------*/

static char *
search_tran_char     (char *string, Bool first)
{
    char
        *end,
        *feedback = NULL;

    if (first)
      {
        feedback = string;
        end      = feedback + strlen (string);

        while (feedback < end && IS_NOT_TRANSLATED (*feedback))
            feedback++;
      }
    else
      {
        end      = string;
        feedback = string + strlen (string) - 1;

        while (feedback > end && IS_NOT_TRANSLATED (*feedback))
            feedback--;
        feedback++;
      }
    return (feedback);
}

static void 
add_string_to_dict (
          DICT_CTX  *dict,
    const char      *string,
    const char      *language,
    const char      *src_filename
  )
{
    int
        item_id;
    char
        old,
        *begin,
        *end;
  
    begin = search_tran_char ((char *)string, TRUE);
    end   = search_tran_char ((char *)string, FALSE);
    old   = *end;
    *end  = '\0';

    item_id = get_dictionary_item (begin, (char*)src_filename, dict);
    if (item_id < 0)
      item_id = add_dictionary_item (begin, (char*)language, dict);

    if (item_id > 0)
          update_usage ((char*)src_filename, item_id, 0, dict);

    *end = old;
}


static void update_dict_from_xml_item (
    DICT_CTX    *dict,
    XML_ITEM    *item,
    FILE_CONFIG *file_config,
    const char  *language,
    const char  *src_filename
  )
{
    XML_ATTR *attr;
    XML_ITEM *child;
    ITEM_CONFIG *item_config;

    item_config = iaftran_get_item_config (file_config, item);

    if (item_config != NULL)
      {
        FORATTRIBUTES (attr, item)
            if (iaftran_must_handle_attr (item_config, xml_attr_name(attr)))
                add_string_to_dict (
                    dict,
                    xml_attr_value (attr),
                    language,
                    src_filename
                  );

        if (iaftran_must_handle_content (item_config))
            FORVALUES (child, item)
                add_string_to_dict (
                    dict,
                    xml_item_value (child),
                    language,
                    src_filename
                  );
      }

    FORCHILDREN (child, item)
        update_dict_from_xml_item (
            dict,
            child,
            file_config,
            language,
            src_filename
          );
}

static void
update_dict_from_xml_file (
    DICT_CTX    *dict,
    const char  *path,
    const char  *file_name,
    FILE_CONFIG *file_config,
    const char  *language)
{
    XML_ITEM 
        *root = NULL;
    int
        rc;
    char
        *short_file_name;

    rc = xml_load_file (&root, path, file_name, FALSE);
    if (rc == XML_NOERROR)
      {
        if (path && *path)
          {
            short_file_name = (char *)file_name + strlen (path);
            if (*short_file_name == PATHEND)
                short_file_name++;
          }
        else
            short_file_name = (char *)file_name;
        update_dict_from_xml_item(dict, root, file_config, language, short_file_name);
      }
    else
        coprintf ("Error in file %s: %s", file_name, xml_error ());

    if (root)
      xml_free (root);
}


/*  -------------------------------------------------------------------------
    Function: parse_markup_file

    Synopsis: Parse Markup file and update dictionary.
              file_name must be in full path.
    -------------------------------------------------------------------------*/

MARKUP_TAG *
parse_markup_file (char *file_name, char *path, DICT_CTX *dict, char *language,
                   IAFTRAN_CONFIG *cfg)
{
    FILE_CONFIG *
        file_config = NULL;
    MARKUP_TAG
        *tag = NULL;
    int
        path_length = 0;
    char
        *ext,
        *short_file_name;

    if (path)
        path_length = strlen (path);


    if ((file_config = iaftran_get_file_config (cfg, file_name)) != NULL)
      {
        /* we handle this file as an xml file, we'll handle only attributes
         * and contents specified in iaftran config file */
        coprintf ("Parsing %s(*)...", file_name);
        update_dict_from_xml_file (dict, path, file_name, file_config, language);
        tag = NULL;                     /* return value */
      }
    else
      {
        coprintf ("Parsing %s...", file_name);
        ext = strrchr (file_name, '.');
        if (ext)
          {
            ext++;
            if (lexcmp (ext, "htm")   == 0
            ||  lexcmp (ext, "asp")   == 0
            ||  lexcmp (ext, "html")  == 0
            ||  lexcmp (ext, "shtml") == 0
            ||  lexcmp (ext, "shtm")  == 0
            ||  lexcmp (ext, "php")   == 0
               )
              {
                tag = html_load (file_name);
                if (tag)
                  {
                     html_del_skipped     (tag);
                     html_remove_link     (tag);
                     html_add_string_attr (tag);
                  }
              }
            else
            if (lexcmp (ext, "c")   == 0
            ||  lexcmp (ext, "cpp") == 0
            ||  lexcmp (ext, "pl")  == 0
            ||  lexcmp (ext, "h")   == 0
            ||  lexcmp (ext, "hpp") == 0
               )
                tag = tagged_file_load (file_name, FILE_TAG_BEGIN, FILE_TAG_END);
          }
        if (tag && dict)
          {
            short_file_name = file_name + path_length;
            if (*short_file_name == PATHEND)
                short_file_name++;
            update_dictionary (dict, tag, language, short_file_name);
          }
      }

    return tag;
}


/*  -------------------------------------------------------------------------
    Function: parse_markup_dir

    Synopsis: Parse directory and update dictionary.
    -------------------------------------------------------------------------*/

void
parse_markup_dir (char *file_name, char *main_path, DICT_CTX *dict,
                  Bool recursive, char *language, IAFTRAN_CONFIG *cfg)
{
    DIRST
        dir;
    char
        *full_dir;
    char
        sub_dir [256],
        fname   [256],
        path    [256];
    MARKUP_TAG
        *tag;

    strcpy (fname, file_name);
    strcpy (path,  file_name);

    strcpy (path, strip_file_name (file_name));
    strip_file_path (fname);

    if (open_dir (&dir, path))
    do
      {
        if ((dir.file_attrs & ATTR_HIDDEN) != 0)
            ;   /*  Do nothing                                               */
        else
        if ((dir.file_attrs & ATTR_SUBDIR) != 0
        &&   recursive)
          {
            full_dir = locate_path (path, dir.file_name);
            sprintf (sub_dir, "%s%s", full_dir, fname);
            strcpy  (sub_dir, clean_path (sub_dir));
            parse_markup_dir (sub_dir, main_path, dict, TRUE, language, cfg);
            mem_free (full_dir);
          }
        else
        if (file_matches (dir.file_name, fname))
          {
            sprintf (sub_dir, "%s/%s", path, dir.file_name);
            strcpy  (sub_dir, clean_path (sub_dir));
            tag = parse_markup_file (sub_dir, main_path, dict, language, cfg);
            if (tag)
                tag_free (tag);
          }
      }
    while (read_dir (&dir));
    close_dir (&dir);
}

/*  ---------------------------------------------------------------------[<]-
    Function: translate_markup

    Synopsis: Translate a markup tree.
    ---------------------------------------------------------------------[>]-*/

void
translate_markup  (
    MARKUP_TAG *tag,
    DICT_CTX   *dict,
    char       *language,
    int         trans_mode,
    char       *trans_format,
    char       *script_format,
    char       *screen_name,
    Bool        multibyte)
{
    MARKUP_DATA
        *data;
    MARKUP_TAG
        *child;
    char
        *value;
    long
        id;

    FORDATA (data, tag)
      {
        id = get_dictionary_item (data-> value, screen_name, dict);
        if (id >= 0)
          {
            switch (trans_mode)
              {
                case TRANSLATION_MODE_STATIC:
                    /* Translate to multibyte only client side script        */
                    if (data-> script == 1)
                        value = get_dictionary_value (id, language, dict, TRUE, multibyte);
                    else
                        value = get_dictionary_value (id, language, dict, TRUE, FALSE);
                    if (value)
                      {
                        mem_strfree (&data-> value);
                        data-> value = mem_strdup (value);
                        mem_free (value);
                      }
                    break;
                case TRANSLATION_MODE_FORMAT:
                    if (data-> script != SERVER_SCRIPT
                    ||  script_format == NULL)
                      {
                        sprintf (buffer , trans_format, id);
                        mem_strfree (&data-> value);
                        data-> value = mem_strdup (buffer);
                      }
                    else
                      {
                        sprintf (buffer , script_format, id);
                        mem_strfree (&data-> value);
                        data-> value = mem_strdup (buffer);
                        child = tag_get (tag, data-> order - 1);
                        if (child)
                          {
                            value = &child-> value [strlen (child-> value) - 1];
                            while (value > child-> value && *value != '"')
                                value--;
                            *value = '\0';
                          }
                        child = tag_get (tag, data-> order + 1);
                        if (child)
                          {
                            value = child-> value + 1;
                            while (*value  && *value != '"')
                                value++;
                            value = mem_strdup (value);
                            mem_strfree (&child-> value);
                            child-> value = value;
                          }

                      }
                    break;
              }
          }
      }
    FORTAG (child, tag)
        translate_markup (child, dict, language, trans_mode, trans_format,
                          script_format, screen_name, multibyte);
}


static char *
translate_string (
    DICT_CTX    *dict,
    char        *string,
    char        *language,
    char        *screen_name,
    Bool         multibyte
  )
{
    int
        id;
    char
        *translation,
        *feedback = NULL,
        old,
        *begin,
        *end;
  
    begin = search_tran_char ((char *)string, TRUE);
    end   = search_tran_char ((char *)string, FALSE);
    old   = *end;
    *end  = '\0';

    ASSERT (string != NULL);

    if (*string == '\0')
        return (feedback);

    id = get_dictionary_item (string, screen_name, dict);
    if (id >= 0)
        /* case TRANSLATION_MODE_STATIC: */
        translation = get_dictionary_value (id, language, dict, FALSE, multibyte);
    else
        translation = mem_strdup (string);

    *end  = old;

    if (translation)
      {
        old   = *begin;
        *begin = '\0';
        feedback = xstrcpy (NULL, string, translation, end, NULL);
        *begin = old;
        mem_free (translation);
      }
    return (feedback);
}


static void 
translate_xml_item (
    DICT_CTX    *dict,
    XML_ITEM    *item,
    FILE_CONFIG *file_config,
    char        *language,
    Bool         multibyte,
    char        *screen_name
  )
{
    XML_ITEM 
        *child;
    XML_ATTR 
        *attr;
    char 
        *attr_value,
        *trans;
    ITEM_CONFIG 
        *item_config;
    int 
        rc;

    ASSERT (item != NULL);

    item_config = iaftran_get_item_config (file_config, item);
    if (item_config != NULL)
      {
        FORATTRIBUTES (attr, item)
          {
            if (iaftran_must_handle_attr (item_config, xml_attr_name(attr)))
              {
                attr_value = xml_attr_value (attr);
                ASSERT (attr_value != NULL);
                trans = translate_string (dict, attr_value, language, screen_name,
                                          multibyte);
                if (trans)
                  {
                    rc = xml_put_attr (item, xml_attr_name(attr), trans);
                    ASSERT (rc == 0);       /* we replace, we don't create */
                    mem_free (trans);
                  }
              }
          }

        if (iaftran_must_handle_content (item_config))
            FORVALUES (child, item)
              {
                trans = translate_string (
                    dict,
                    xml_item_value (child),
                    language,
                    screen_name,
                    multibyte
                  );
                if (trans)
                  {
                    xml_modify_value  (child, trans);
                    mem_free (trans);
                  }
              }
      }

    FORCHILDREN (child, item)
      {
        translate_xml_item (
            dict,
            child,
            file_config,
            language,
            multibyte,
            screen_name
          );
      }
}


static void 
translate_xml_file (
    DICT_CTX    *dict,
    char        *src_filename,
    char        *path_dest,
    FILE_CONFIG *file_config,
    char        *language,
    Bool         multibyte,
    char        *path
  )
{
    XML_ITEM
        *root = NULL;
    char
        *short_name = NULL;
    int
        rc;

    rc = xml_load_file (&root, NULL, src_filename, FALSE);
    
    if (rc == XML_NOERROR)
      {
        short_name = src_filename + strlen (path);
        if (*short_name == PATHEND)
            short_name++;

        translate_xml_item (dict, root, file_config, language, multibyte,
                            short_name);

        xml_save_file (
            xml_first_child (root),
            file_where ('s', path_dest, short_name, NULL)
          );
      }
    xml_free (root);
}



/*  ---------------------------------------------------------------------[<]-
    Function: translate_markup_file

    Synopsis: Translate a markup file.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
translate_markup_file (
    char     *file_name,
    char     *path,
    DICT_CTX *dict,
    char     *language,
    char     *path_dest,
    int       trans_mode,
    char     *trans_format,
    char     *script_format,
    Bool      multibyte,
    IAFTRAN_CONFIG *cfg)
{
    FILE_CONFIG 
        *file_config = NULL;
    MARKUP_TAG
        *tag = NULL;
    int
        path_length = 0;
    char
        *ext,
        *short_file_name;

    if (path)
        path_length = strlen (path);

    if ((file_config = iaftran_get_file_config (cfg, file_name)) != NULL)
      {
        /* we handle this file as an xml file, we'll handle only attributes
         * and contents specified in iaftran config file */
        coprintf ("Translate %s(*)...", file_name);
        translate_xml_file (
            dict,
            file_name,
            path_dest,
            file_config,
            language,
            FALSE,                      /* Don't convert to multibyte xml    */
            path
          );
        tag = NULL;                     /* return value                      */
      }
    else
      {
        coprintf ("Translate %s...", file_name);
        ext = strrchr (file_name, '.');
        if (ext)
          {
            ext++;
            if (lexcmp (ext, "htm")   == 0
            ||  lexcmp (ext, "asp")   == 0
            ||  lexcmp (ext, "html")  == 0
            ||  lexcmp (ext, "shtml") == 0
            ||  lexcmp (ext, "shtm")  == 0
            ||  lexcmp (ext, "php")   == 0
               )
              {
                tag = html_load (file_name);
                if (tag)
                  {
                     html_del_skipped     (tag);
                     html_remove_link     (tag);
                     html_add_string_attr (tag);
                  }
              }
            else
            if (lexcmp (ext, "c")   == 0
            ||  lexcmp (ext, "cpp") == 0
            ||  lexcmp (ext, "pl")  == 0
            ||  lexcmp (ext, "h")   == 0
            ||  lexcmp (ext, "hpp") == 0
               )
                tag = tagged_file_load (file_name, FILE_TAG_BEGIN, FILE_TAG_END);
          }
        if (tag && dict)
          {
            /* Make destination file name                                        */
            short_file_name = file_name + path_length;
            if (*short_file_name == PATHEND)
                short_file_name++;

            translate_markup (tag, dict, language, trans_mode, trans_format,
                              script_format, short_file_name, multibyte);

            sprintf (buffer, "%s%c%s", path_dest, PATHEND, short_file_name);
            strcpy (buffer, clean_path (buffer));
            markup_save (tag, buffer);
          }
      }

    return (tag);
}


/*  ---------------------------------------------------------------------[<]-
    Function: translate_markup_dir

    Synopsis: Translate a markup directory.
    ---------------------------------------------------------------------[>]-*/

void
translate_markup_dir  (
    char     *file_name,
    char     *main_path,
    DICT_CTX *dict,
    Bool      recursive,
    char     *language,
    char     *path_dest,
    int       trans_mode,
    char     *trans_format,
    char     *script_format,
    Bool      multibyte,
    IAFTRAN_CONFIG *cfg
    )
{
    DIRST
        dir;
    char
        *full_dir;
    char
        sub_dir [256],
        fname   [256],
        path    [256];
    MARKUP_TAG
        *tag;

    strcpy (fname, file_name);
    strcpy (path,  file_name);

    strcpy (path, strip_file_name (file_name));
    strip_file_path (fname);

    if (open_dir (&dir, path))
    do
      {
        if ((dir.file_attrs & ATTR_HIDDEN) != 0)
            ;   /*  Do nothing                                               */
        else
        if ((dir.file_attrs & ATTR_SUBDIR) != 0
        &&   recursive)
          {
            full_dir = locate_path (path, dir.file_name);
            sprintf (sub_dir, "%s%s", full_dir, fname);
            strcpy  (sub_dir, clean_path (sub_dir));
            translate_markup_dir (sub_dir, main_path, dict, TRUE, language,
                                  path_dest, trans_mode, trans_format,
                                  script_format, multibyte, cfg);
            mem_free (full_dir);
          }
        else
        if (file_matches (dir.file_name, fname))
          {
            sprintf (sub_dir, "%s/%s", path, dir.file_name);
            strcpy  (sub_dir, clean_path (sub_dir));
            tag = translate_markup_file (sub_dir, main_path, dict, language,
                                         path_dest, trans_mode, trans_format,
                                         script_format, multibyte, cfg);
            if (tag)
                tag_free (tag);
          }
      }
    while (read_dir (&dir));
    close_dir (&dir);
}

/*  -------------------------------------------------------------------------
    Function: get_lang_desc

    Synopsis: code is usually a string (2 or 5 char : XX or XX-YY)
    -------------------------------------------------------------------------*/

static language_desc *
get_lang_desc (const char *code)
{
    language_desc
        *res = NULL;
    int
        index;

    for (index = 0; index < MAX_LANG; index++)
      {
        if (lexcmp (code, language_table [index].code) == 0)
          {
            res = &language_table [index];
            break;
          }
      }

    return (res);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_lang_name

    Synopsis: return the language description
    ---------------------------------------------------------------------[>]-*/

char *
get_lang_name (const char *code)
{
    char
        *res = NULL;
    language_desc
        *lang;

    lang = get_lang_desc (code);
    if (lang)
        res = lang-> name;

    return (res);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_lang_charset

    Synopsis: return the character set for a language code
    ---------------------------------------------------------------------[>]-*/

char *
get_lang_charset (const char *code)
{
    char
       *res = NULL;
    language_desc
       *lang;

    lang = get_lang_desc (code);
    if (lang)
        res = lang-> charset;

    return (res);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_lang_codepage

    Synopsis: return the codepage for a language code
    ---------------------------------------------------------------------[>]-*/

qbyte
get_lang_codepage (const char *code)
{
    qbyte
        res = 1252;                     /* Default codepage is 1252          */
    language_desc
        *lang;

    lang = get_lang_desc (code);
    if (lang)
        res = lang-> codepage;

    return (res);
}


