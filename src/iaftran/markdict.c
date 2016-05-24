/*  ----------------------------------------------------------------<Prolog>-
    Name:       markdict.c
    Title:      Markup Dictionary Functions
    Package:    Markup Translator

    Written:    1999/11/12 Pascal Antonnaux <pascal@imatix.com>
    Revised:    2001/05/09 Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to Load, save and update dictionnary.

    Copyright:  Copyright (c) 1991-2000 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"
#include "markdict.h"
#include "marklib.h"


/*- Structure of XML dictionary ---------------------------------------------

  <dictionary
    name = "dictionary name"
    path = "path of source"
    mask = "file mask for parsing"
    recursive = "1"
    format = "format used for dynamic translation"
    script_format = "format used in script zone in dynamic translation"
  >
    <field  id = "1">
        <data value = "Select" lang = "EN" default = "1" />
        <data value = "Sélectionner" lang = "FR" />
        <usage value = "xxxx.htm"/>
    </field>
    <field id = "2">
        <data value = "Select <a1>this</a>" lang = "EN" default = "1" />
        <usage value = "xxxx.htm"/>
        <link  id = "1" value = "<A HREF=&quot;/images/this.jpg&quot;>"/>
      .
      .
      .
    </field>
  </dictionary>


  ---------------------------------------------------------------------------*/

/*- Definitions -------------------------------------------------------------*/

#define BUFFER_SIZE     10240           /*  Largest size of one term         */

#define LANG_IS_DEFAULT   0x01
#define LANG_IS_UNICODE   0x02
#define LANG_IS_TECHNICAL 0x04
#define LANG_MULTI_VALUE  0x08
#define LANG_UNIQUE_VALUE 0x10

#define OBJTYPE_LANG    0x01
#define OBJTYPE_USAGE   0x02
#define OBJTYPE_SCREEN  0x03
#define OBJTYPE_LINK    0x04

/*- Local functions ---------------------------------------------------------*/

static char   *get_dict_key       (const char *value);
static void    add_language       (DICT_CTX *dict, char *language);
static void    save_langvalue2xml (FILE *file, LANG_VALUE *value);
static void    add_langvalue      (char *val, long id, char *language,
                                   Bool is_default, Bool is_technical,
                                   Bool multi, Bool unique, long ext_id,
                                   char *screen_name, DICT_CTX *dict);
static char   *get_xml_usage      (XML_ITEM *field_item);
static char   *get_bin_usage      (long field_id, DICT_CTX *dict);
static void    dump_lang_value_rec (FILE *file, LANG_VALUE *value);

/*- Global variables --------------------------------------------------------*/

long
    lang_value_deleted = 0,
    usage_deleted      = 0,
    screen_deleted     = 0,
    link_deleted       = 0;

static char
    buffer [BUFFER_SIZE + 2];           /*  Current line w/space for EOL     */

static char *
get_dict_key   (const char *value)
{
    char
        *buf,
        *val;

    val = (char *)value;
    buf = buffer;

    while (*val)
      {
        if (!(*val == '\r'
        ||    *val == '\n'
        ||    *val == '\t'))
            *buf++ = *val;
        val++;

        /* Remove multiple space                                             */
        if (*val == ' '
        && (   *(val - 1) == ' '
            || (     buf      != buffer
                && *(buf - 1) == ' '))
           )
          {
            while (*val && *val == ' ')
                val++;
          }
      }
    *buf = '\0';
    return (buffer);
}

static void
add_langvalue (char *val, long id, char *language, Bool is_default, Bool is_technical,
               Bool multi, Bool unique, long ext_id, char *screen_name, DICT_CTX *dict)
{
    LANG_VALUE
        *value;
    SYMBOL
        *symbol;
    char
        unique_key [4000],
        *key,
        key_buffer [20];

   value = mem_alloc (sizeof (LANG_VALUE));
   if (value)
     {
       memset (value, 0, sizeof (LANG_VALUE));
       value-> field_id = id;
       if (language && lexcmp (language, "ja") == 0)
           value-> is_unicode = TRUE;
       value-> value    = mem_strdup (val);

       if (value-> value)
            value-> length = strlen (value-> value);

       if (language)
           strcpy (value-> language, language);
       value-> is_default   = is_default;
       value-> is_technical = is_technical;
       value-> multi_value  = multi;
       value-> unique_value = unique;
       value-> ext_id       = ext_id;
       value-> used         = TRUE;

       if (value-> is_default)
         {
           sprintf (key_buffer, "%ld", value-> field_id);
           if (unique == TRUE)
             {
               sprintf (unique_key, "%s_%s",
                        screen_name, get_dict_key (value-> value));
               key = unique_key;
             }
           else
               key = get_dict_key (value-> value);
           symbol = sym_assume_symbol (dict-> def_value, key, key_buffer);
           if (symbol)
               symbol-> data = value;
           symbol = sym_assume_symbol (dict-> translated, key_buffer, value-> value);
         }
       else
         {
           sprintf (key_buffer, "%ld_%s", value-> field_id, value-> language);
           symbol = sym_assume_symbol (dict-> translated, key_buffer, value-> value);
         }
       if (symbol)
           symbol-> data = value;
       if (id > dict-> max_id)
           dict-> max_id = id;
    }
}


static int
sym_sort_by_id (const void *sym1, const void *sym2)
{
    long
        id1,
        id2;
    int 
        compare = 0;

    id1 = atol ((*(SYMBOL **) sym1)-> value);
    id2 = atol ((*(SYMBOL **) sym2)-> value);

    if (id1 > id2)
        compare = 1;
    else
    if (id1 < id2)
        compare = -1;
        
    return (compare);
}


/*  ---------------------------------------------------------------------[<]-
    Function: load_xml_dictionary

    Synopsis: Load dictionary file into a XML tree and create symbol table
              to search reference.
    ---------------------------------------------------------------------[>]-*/
DICT_CTX *
load_xml_dictionary (const char *name)
{
    XML_ITEM
        *root_item = NULL,
        *main_item,
        *item,
        *field_item;
    XML_ATTR
        *attr;
    DICT_CTX
        *root;
    char
        *screen_name,
        *language = NULL,
        *key,
        *id_val = "",
        *item_name,
        *attr_name,
        *ptr;
    long
        id;
    Bool
        technical,
        multi_value,
        unique_value,
        default_item;

    root = mem_alloc (sizeof (DICT_CTX));
    if (root)
      {
        memset (root, 0, sizeof (DICT_CTX));

        root-> def_value  = sym_create_table ();
        root-> screen     = sym_create_table ();
        root-> screen_id  = sym_create_table ();
        root-> usage      = sym_create_table ();
        root-> translated = sym_create_table ();

        if (xml_load (&root_item, NULL, name) != 0)
           {
             coprintf ("Error on load %s: %s", name, xml_error ());
             if (root_item)
                 xml_free (root_item);
             return (root);
           }


        main_item = xml_first_child (root_item);

        /* Get Field item                                                    */
        FORCHILDREN (field_item, main_item)
          {
            /* Get field value                                               */
                FORATTRIBUTES (attr, field_item)
                  {
                    attr_name = xml_attr_name  (attr);
                    ptr       = xml_attr_value (attr);
                    if (attr_name
                    &&  ptr
                    &&  lexcmp (attr_name, "id") == 0)
                      {
                        id_val  = ptr;
                        id      = atol (ptr);
                        if (id > root-> max_id)
                            root-> max_id = id;
                      }
                  }
                /* Get data item                                             */
                FORCHILDREN (item, field_item)
                  {
                    item_name = xml_item_name (item);
                    if (lexcmp (item_name, "data") == 0)
                      {
                        key          = NULL;
                        default_item = FALSE;
                        technical    = FALSE;
                        multi_value  = FALSE;
                        unique_value = FALSE;
                        FORATTRIBUTES (attr, item)
                          {
                            attr_name = xml_attr_name  (attr);
                            ptr       = xml_attr_value (attr);
                            if (attr_name &&  ptr)
                              {
                                if (lexcmp (attr_name, "value") == 0)
                                    key = ptr;
                                else
                                if (lexcmp (attr_name, "lang") == 0)
                                    language = ptr;
                                else
                                if (lexcmp (attr_name, "default") == 0
                                &&  ptr [0] == '1')
                                    default_item = TRUE;
                                else
                                if (lexcmp (attr_name, "technical") == 0
                                &&  ptr [0] == '1')
                                    technical = TRUE;
                                else
                                if (lexcmp (attr_name, "multi") == 0
                                &&  ptr [0] == '1')
                                    multi_value = TRUE;
                                else
                                if (lexcmp (attr_name, "unique") == 0
                                &&  ptr [0] == '1')
                                    unique_value = TRUE;
                              }
                          }

                        if (key && strused (key))
                          {
                            if (unique_value == TRUE)
                                screen_name = get_xml_usage (field_item);
                            else
                                screen_name = NULL;
                            add_langvalue (key, id, language, default_item, technical,
                                           multi_value, unique_value, 0, screen_name,
                                           root);
                          }
                        if (language)
                            add_language   (root, language);
                      }
                    else
                    if (lexcmp (item_name, "usage") == 0)
                      {
                        key = xml_get_attr (item, "value", NULL);
                        if (key)
                            update_usage (key, id, 0, root);
                      }
                    else
                    if (lexcmp (item_name, "link") == 0)
                      {
                        key    = xml_get_attr (item, "value", NULL);
                        id_val = xml_get_attr (item, "id",    NULL);
                        if (key && id_val)
                            update_item_link (key, atol(id_val), id, root);
                      }
                  }
          }
        xml_free (root_item);
      }
    return (root);
}


/*  ---------------------------------------------------------------------[<]-
    Function: load_bin_dictionary

    Synopsis: Load binary dictionary file and create symbol table
              to search reference.
    ---------------------------------------------------------------------[>]-*/
DICT_CTX *
load_bin_dictionary (const char *name)
{
    DICT_CTX
        *root = NULL;
    FILE
        *file = NULL;
    char
        unique_key [200],
        *key,
        key_buffer [20];
    SYMBOL
        *symbol;
    byte
        object_type;
    VALUE_LINK
        *link;
    SCREEN
        *screen;
    USAGE
        *usage;
    LANG_VALUE
        *value;

    root = mem_alloc (sizeof (DICT_CTX));
    if (root)
      {
        memset (root, 0, sizeof (DICT_CTX));
        root-> def_value  = sym_create_table ();
        root-> screen     = sym_create_table ();
        root-> screen_id  = sym_create_table ();
        root-> usage      = sym_create_table ();
        root-> translated = sym_create_table ();

        if (name && *name)
            file = fopen (name, "rb");
        if (file)
          {
            while (fread (&object_type, 1, 1, file) > 0)
              {
                switch (object_type)
                  {
                    case OBJTYPE_LANG:
                        value = read_lang_value_rec (file);
                        if (value)
                          {
                            add_language   (root, value-> language);
                            if (value-> is_default)
                              {
                                if (value->unique_value == TRUE)
                                  {
                                    sprintf (unique_key, "%s_%s",
                                             get_bin_usage (value-> field_id, root),
                                             get_dict_key (value-> value));
                                    key = unique_key;
                                  }
                                else
                                    key = get_dict_key (value-> value);
                                sprintf (key_buffer, "%ld", value-> field_id);
                                symbol = sym_assume_symbol (root-> def_value, key, key_buffer);
                                if (symbol)
                                    symbol-> data = value;
                                symbol = sym_assume_symbol (root-> translated, key_buffer, value-> value);
                              }
                            else
                              {
                                sprintf (key_buffer, "%ld_%s", value-> field_id, value-> language);
                                symbol = sym_assume_symbol (root-> translated, key_buffer, value-> value);
                              }
                            if (symbol)
                                symbol-> data = value;
                            if (value-> field_id > root-> max_id)
                                root-> max_id = value-> field_id;
                          }
                        break;
                    case OBJTYPE_USAGE:
                        usage = read_usage_rec (file);
                        if (usage)
                          {
                            sprintf (key_buffer, "%ld_%ld", usage-> lang_value_id, usage-> screen_id);
                            symbol = sym_assume_symbol (root-> usage, key_buffer, "");
                            if (symbol)
                                symbol-> data = usage;
                            
                          }
                        break;
                    case OBJTYPE_SCREEN:
                        screen = read_screen_rec (file);
                        if (screen)
                          {
                            sprintf (key_buffer, "%ld", screen-> id);
                            symbol = sym_assume_symbol (root-> screen, screen-> value, key_buffer);
                            if (symbol)
                                symbol-> data = screen;
                            symbol = sym_assume_symbol (root-> screen_id, key_buffer, screen-> value);
                            if (symbol)
                                symbol-> data = screen;
                            if (screen-> id > root-> max_screenid)
                                root-> max_screenid = screen-> id;
                          }
                        break;
                    case OBJTYPE_LINK:
                        link = read_value_link_rec (file);
                        if (link)
                          {
                            sprintf (key_buffer, "%ld_%d", link->lang_value_id, link-> index);
                            symbol = sym_assume_symbol (root-> link, key_buffer, link-> value);
                            if (symbol)
                                symbol-> data = link;
                            if (link-> index > root-> max_link)
                                root-> max_link = link-> index;
                          }
                        break;
                    default:
                        break;
                  }
              }
            fclose (file);
          }
      }

    return (root);
}

static void
add_language   (DICT_CTX *dict, char *language)
{
    register short
         index;
    Bool
         found = FALSE;

    for (index = 0;
         index < MAX_LANGUAGE && dict-> language [index] != NULL;
         index++)
      {
         if (lexcmp (dict-> language [index], language) == 0)
           {
             found = TRUE;
             break;
           }
      }
    if (found == FALSE)
        dict-> language [index] = language;
}

/*  ---------------------------------------------------------------------[<]-
    Function: save_bin_dictionary

    Synopsis: Save dictionary into a binary file
    ---------------------------------------------------------------------[>]-*/

void
save_bin_dictionary (const char *name, DICT_CTX *dict)
{
    FILE
        *file;
    SYMBOL
        *symbol;

    file = fopen (name, "wb");
    if (file)
      {
        if (dict-> screen)
          {
            for (symbol = dict-> screen-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     write_screen_rec (file, (SCREEN *)symbol-> data);
               }
          }
        if (dict-> usage)
          {
            for (symbol = dict-> usage-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     write_usage_rec (file, (USAGE *)symbol-> data);
               }
          }
        if (dict-> translated)
          {
            for (symbol = dict-> translated-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     write_lang_value_rec (file, (LANG_VALUE *)symbol-> data);
               }
          }
        if (dict-> link)
          {
            for (symbol = dict-> link-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     write_value_link_rec (file, (VALUE_LINK *)symbol-> data);
               }
          }
        fclose (file);
      }

}

/*  ---------------------------------------------------------------------[<]-
    Function: save_xml_dictionary

    Synopsis: Save dictionary into a XML file.
    ---------------------------------------------------------------------[>]-*/

void
save_xml_dictionary (const char *name, DICT_CTX *dict)
{
    FILE
        *file;
    LANG_VALUE
        *value;
    VALUE_LINK
        *link;
    SYMBOL
        *symbol,
        *tmp_symbol,
        *translation,
        *sym_usage,
        *sym_link;
    SYMTAB
        *tmp_table;
    long
        id;
    int
        index;
    char
        key_buffer [20],
        *screen_name;

    file = fopen (name, "w");
    if (file)
      {
        fprintf (file, "<?xml version='1.0'?>\n");
        fprintf (file, "<dictionary>\n");

        sym_sort_table (dict-> def_value, sym_sort_by_id);
        sym_sort_table (dict-> usage, NULL);
        for (symbol = dict-> def_value-> symbols; symbol; symbol = symbol-> next)
          {
            if (symbol-> data)
              {
                value = (LANG_VALUE *)symbol-> data;
                id = value-> field_id;
                fprintf (file, "    <field id = \"%ld\">\n", value-> field_id);
                save_langvalue2xml (file, value);
                for (index = 0;
                     index < MAX_LANGUAGE && dict-> language [index] != NULL;
                     index++)
                  {
                    sprintf (key_buffer, "%d_%s", id, dict-> language [index]);
                    translation = sym_lookup_symbol (dict-> translated, key_buffer);
                    if (translation)
                        save_langvalue2xml (file, (LANG_VALUE *)translation-> data);
                  }
                if (dict-> max_link > 0)
                  {
                    for (index = 1; index <= dict-> max_link; index++)
                      {
                        sprintf (key_buffer, "%ld_%d", id, index);
                        sym_link = sym_lookup_symbol (dict-> link, key_buffer);
                        if (sym_link)
                          {
                            link = (VALUE_LINK *)sym_link-> data;
                            if (link)
                              {
                                screen_name = link-> value;
                                http_encode_meta (buffer, &screen_name, BUFFER_SIZE, TRUE);
                                fprintf (file, "        <link id = \"%d\" value = \"%s\"/>\n",
                                               link-> index, buffer);
                              }
                          }
                      }
                  }
                /* Save usage in screen name order                           */                
                tmp_table = sym_create_table ();
                for (index = 1; index <= dict-> max_screenid; index++)
                  {
                    sprintf (key_buffer, "%ld_%d", id, index);
                    sym_usage = sym_lookup_symbol (dict-> usage, key_buffer);
                    if (sym_usage)
                      {
                        sprintf (key_buffer, "%d", index);
                        screen_name = sym_get_value (dict-> screen_id, key_buffer, NULL);
                        if (screen_name)
                            sym_assume_symbol (tmp_table, screen_name, "");
                      }
                  }
                sym_sort_table (tmp_table, NULL);
                for (tmp_symbol = tmp_table-> symbols; tmp_symbol;
                     tmp_symbol = tmp_symbol-> next)
                    fprintf (file, "        <usage value = \"%s\"/>\n",
                                           tmp_symbol-> name);               
                sym_delete_table (tmp_table);
                fprintf (file, "    </field>\n");
              }
          }

        fprintf (file, "</dictionary>\n");
        fclose (file);
      }
}

static void
save_langvalue2xml (FILE *file, LANG_VALUE *value)
{
    char
        *quot,
        *string;
    int
        length = 0;

    string = value-> value;

    if (string)
      {
         http_encode_meta (buffer, &string, BUFFER_SIZE, TRUE);
         /* Encode only quot character not encoded by http_encode_meta       */
         string = buffer;                
         while ((quot = strchr (string, '"')) != NULL)
           {
             if (length == 0)
                 length = strlen (buffer);
             memmove (quot + 6, quot + 1, length - (quot - buffer));
             *quot++ = '&';
             *quot++ = 'q';
             *quot++ = 'u';
             *quot++ = 'o';
             *quot++ = 't';
             *quot++ = ';';
             length += 5;
             string  = quot;
           }

         fprintf (file, "        <data lang = \"%s\" value = \"%s\"%s%s%s%s/>\n",
                                 value-> language,
                                 buffer,
                                 value->is_default?   " default = \"1\"": "",
                                 value->is_technical? " technical = \"1\"": "",
                                 value->multi_value?  " multi = \"1\"": "",
                                 value->unique_value? " unique = \"1\"": ""
                  );
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: free_dictionary

    Synopsis: Free all allocated resource used by dictionary.
    ---------------------------------------------------------------------[>]-*/
void
free_dictionary (DICT_CTX *dict)
{
    SYMBOL
        *symbol;

    if (dict)
      {

        if (dict-> def_value)
          {
            sym_delete_table (dict-> def_value);
            dict-> def_value = NULL;
          }

        if (dict-> translated)
          {
            for (symbol = dict-> translated-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     free_lang_value ((LANG_VALUE *)symbol-> data);
               }
            sym_delete_table (dict-> translated);
            dict-> translated = NULL;
          }

        if (dict-> usage)
          {
            for (symbol = dict-> usage-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     free_usage ((USAGE *)symbol-> data);
               }
            sym_delete_table (dict-> usage);
            dict-> usage = NULL;
          }

        if (dict-> screen)
          {
            for (symbol = dict-> screen-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     free_screen ((SCREEN *)symbol-> data);
               }
            sym_delete_table (dict-> screen);
            dict-> screen = NULL;
          }
        if (dict-> screen_id)
          {
            sym_delete_table (dict-> screen_id);
            dict-> screen_id = NULL;
          }
        if (dict-> link)
          {
            for (symbol = dict-> link-> symbols; symbol; symbol = symbol-> next)
               {
                 if (symbol-> data)
                     free_value_link ((VALUE_LINK *)symbol-> data);
               }
            sym_delete_table (dict-> link);
            dict-> link = NULL;
          }
        mem_free (dict);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: add_dictionary_item

    Synopsis: Add a new dictionary item.
    ---------------------------------------------------------------------[>]-*/

long
add_dictionary_item (char     *value,
                     char     *language,
                     DICT_CTX *dict)
{
    long
        new_id = 0;
    Bool
        is_default = FALSE;

    new_id = ++dict-> max_id;
    if (language && lexcmp (language, "xx") == 0)
        is_default = TRUE;

    add_langvalue (value, new_id, language, is_default, FALSE, FALSE, FALSE,
                   0, NULL, dict);
    return (new_id);
}


/*  ---------------------------------------------------------------------[<]-
    Function: add_dictionary_item_ext

    Synopsis: Add a new dictionary item.
    ---------------------------------------------------------------------[>]-*/

long
add_dictionary_item_ext (UCODE *value, char *language, long ext_id,
                          Bool is_technical, Bool multi, Bool unique,
                          long id, char *screen_name, DICT_CTX *dict)
{
    long
        new_id = 0;
    Bool
        is_default = FALSE;
    char
        *new_value;
    UCODE
        *in;


    if (id > 0)
      {
        new_id = id;
        if (new_id > dict-> max_id)
            dict-> max_id = new_id;
      }
    else
        new_id = ++dict-> max_id;

    if (language && lexcmp (language, "xx") == 0)
        is_default = TRUE;
    if (language && lexcmp (language, "ja") == 0)
      {
        in = value;
        http_encode_umeta ((UCODE *)buffer, &in, BUFFER_SIZE/2, TRUE);
        new_value = ucode2ascii ((UCODE *)buffer);
      }
    else
        new_value = ucode2ascii (value);

    if (unique == TRUE && screen_name == NULL)
        screen_name = get_bin_usage  (new_id, dict);

    if (new_value 
    &&  (   ( lexcmp (language, "xx") == 0  
             && get_dictionary_item (new_value, screen_name, dict) == -1)
         || get_simple_value (new_id, language, dict, NULL) == NULL)
       )
      {
        add_langvalue (new_value, new_id, language, is_default, is_technical,
                       multi, unique, ext_id, screen_name, dict);
      }
    if (new_value)
        mem_free (new_value);
    return (new_id);
}

/*  ---------------------------------------------------------------------[<]-
    Function: get_dictionary_item

    Synopsis: Get a dictionary item. Return the ID of item or -1 if not found
    ---------------------------------------------------------------------[>]-*/

long
get_dictionary_item (char *value, char *screen_name, DICT_CTX *dict)
{
    long
        item_id = -1;
    SYMBOL
        *symbol;
    char
        *key,
        unique_key [4000];

    ASSERT (dict);
    ASSERT (dict-> def_value);

    key = get_dict_key (value);
    symbol = sym_lookup_symbol (dict-> def_value, key);
    if (symbol)
      {
        if (symbol-> value && *symbol-> value)
            item_id = atol (symbol-> value);
        /* If multi value, check if need unique value                        */
        if (((LANG_VALUE *)symbol-> data)-> multi_value == TRUE
            && screen_name
           )
          {
            sprintf (unique_key, "%s_%s", screen_name, key);
            symbol = sym_lookup_symbol (dict-> def_value, unique_key);
            if (symbol && symbol-> value && *symbol-> value)
                item_id = atol (symbol-> value);
          }
      }
    if (symbol && symbol-> data)
        ((LANG_VALUE *)symbol-> data)-> used = TRUE;

    return (item_id);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_dictionary_value

    Synopsis: Get a dictionary value for a ID and a language.
              If value is not defined in specified language,
              return the default dictionary value.
              if multibyte_result is set, returned result is
              converted to unicode, using codepage relative to language
              result MUST BE FREE'D by mem_free !!!
    ---------------------------------------------------------------------[>]-*/

char *
get_dictionary_value (long id, char *language, DICT_CTX *dict,
                      Bool resolve_link, Bool multibyte_result)
{
    char
        *source,
        *dest,
        *link_tag,
        *link,
        *feedback = NULL;
    SYMBOL
        *symbol = NULL;
    long
        link_id;
    qbyte
        codepage;
    char
        key_buffer [20];
    UCODE 
        *uvalue = NULL,
        *tmp    = NULL;

    if (id >= 0
    &&  language
    &&  dict
    &&  dict-> translated)
      {
        sprintf (key_buffer, "%ld_%s", id, language);
        symbol = sym_lookup_symbol (dict-> translated, key_buffer);
        if (symbol == NULL)
          {
            sprintf (key_buffer, "%ld", id);
            symbol= sym_lookup_symbol (dict-> translated, key_buffer);
          }
        if (symbol && symbol-> value)
          {
            if (resolve_link)
              {
                link_tag = txtfind (symbol-> value, "<a");
                if (link_tag == NULL)
                    feedback = mem_strdup (symbol-> value);
                else
                  {
                    source = symbol-> value;
                    dest   = buffer;
                    while (link_tag)
                    {
                      while (*source && source < link_tag)
                          *dest++ = *source++;
                      link_tag += 2;
                      source   += 2;
                      link_id = atol (link_tag);
                      while (*source && *source != '>')
                          source++;
                      source++;
                      if (link_id > 0)
                        {
                          link = get_item_link (link_id, id, dict);
                          if (link)
                            {
                              while (*link)
                                  *dest++ = *link++;
                            }
                        }
                      link_tag = txtfind (source, "<a");
                    }
                    while (*source)
                        *dest++ = *source++;
                    *dest = '\0';
                    feedback = mem_strdup (buffer);
                  }
              }
            else
                feedback = mem_strdup (symbol-> value);
          }

        if (feedback && multibyte_result)
          {
            /* caller want a multibyte result. We decode unicode meta characters
             * and encode it to multibyte */
            codepage = get_lang_codepage (language);

            uvalue = ascii2ucode_ex (feedback, codepage);
            if (uvalue != NULL)
              {
                source = feedback;
                tmp = uvalue;
                http_decode_umeta (uvalue, &tmp, wcslen(uvalue)+1);
                feedback = ucode2ascii_ex (uvalue, codepage);
                mem_free (uvalue);
                mem_free (source);
              }
          }
      }


    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: get_simple_value

    Synopsis: Get a dictionary value for a ID and a language.
              If value is not defined in specified language,
              return the default value.
    ---------------------------------------------------------------------[>]-*/

char *
get_simple_value (long id, char *language, DICT_CTX *dict, char *default_value)
{
    char
        key_buffer [20],
        *feedback = NULL;
    SYMBOL
        *symbol = NULL;

    if (id >= 0
    &&  language
    &&  dict
    &&  dict-> translated)
      {
        sprintf (key_buffer, "%ld_%s", id, language);
        symbol = sym_lookup_symbol (dict-> translated, key_buffer);
        if (symbol && symbol-> value)
            feedback = symbol-> value;
        else
            feedback = default_value;
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: update_usage

    Synopsis: Update usage file name for a dictionary item.
    ---------------------------------------------------------------------[>]-*/

void
update_usage (char *file_name, long field_id, long ext_id, DICT_CTX* dict)
{
    long
        screen_id = 0;
    SYMBOL
        *symbol;
    USAGE
        *usage;
    char
        key_buffer [20];

 
    screen_id = update_screen (file_name, 0, dict);
    sprintf (key_buffer, "%ld_%ld", field_id, screen_id);
    symbol = sym_lookup_symbol (dict-> usage, key_buffer);
    if (symbol == NULL)
      {
        usage = mem_alloc (sizeof (USAGE));
        if (usage)
          {
            memset (usage, 0, sizeof (USAGE));
            usage-> lang_value_id = field_id;
            usage-> screen_id     = screen_id;
            usage-> ext_id        = ext_id;
            usage-> used          = TRUE;
            symbol = sym_assume_symbol (dict-> usage, key_buffer, "");
            if (symbol)
                symbol-> data = usage;
          }
      }
    else
    if (symbol-> data)
        ((USAGE *)symbol-> data)-> used = TRUE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: update_screen

    Synopsis: Update screen list. Create a new screen record if screen name
    not exist. Return ID of the screen.
    ---------------------------------------------------------------------[>]-*/

long
update_screen (char *screen_name, long ext_id, DICT_CTX *dict)
{
    long
        screen_id = 0;
    SCREEN
        *screen;
    SYMBOL
        *symbol;
    char
        key_buffer [20];

    symbol = sym_lookup_symbol (dict-> screen, screen_name);
    if (symbol)
      {
        if (symbol-> data)
          {
            screen_id = ((SCREEN *)symbol-> data)-> id;
            if (ext_id > 0)
                ((SCREEN *)symbol-> data)-> ext_id = ext_id;
            ((SCREEN *)symbol-> data)-> used = TRUE;
          }
      }
    else
      {
         screen = mem_alloc (sizeof (SCREEN));
         if (screen)
           {
             memset (screen, 0, sizeof (SCREEN));

             screen-> id     = ++dict-> max_screenid;
             screen-> value  = mem_strdup (screen_name);
             screen-> ext_id = ext_id;
             screen-> used   = TRUE;

             sprintf (key_buffer, "%ld", screen-> id);
             symbol = sym_assume_symbol (dict-> screen, screen-> value, key_buffer);
             if (symbol)
                 symbol-> data = screen;
             symbol = sym_assume_symbol (dict-> screen_id, key_buffer, screen-> value);
             if (symbol)
                 symbol-> data = screen;
             screen_id = screen-> id;
           }
      }
    return (screen_id);
}

/*  ---------------------------------------------------------------------[<]-
    Function: update_item_link

    Synopsis: Update link value for a dictionary item.
    ---------------------------------------------------------------------[>]-*/

void
update_item_link (char *link_value, long index, long field_id, DICT_CTX *dict)
{
    char
        key_buffer [20];
    VALUE_LINK
        *link;
    SYMBOL
        *symbol;

    sprintf (key_buffer, "%ld_%ld", field_id, index);
    symbol = sym_lookup_symbol (dict-> link, key_buffer);
    if (symbol)
      {
        if (symbol-> data)
          {
            link = (VALUE_LINK *)symbol-> data;
            if (strcmp (link-> value, link_value) != 0)
                mem_strfree (&link-> value);
            if (link-> value == NULL)
                link-> value = mem_strdup (link_value);
            link-> used = TRUE;
          }
      }
    else
      {
        link = mem_alloc (sizeof (VALUE_LINK));
        if (link)
          {
            memset (link, 0, sizeof (VALUE_LINK));
            link-> index         = (byte)index;
            link-> lang_value_id = field_id;
            link-> value         = mem_strdup (link_value);
            link-> used          = TRUE;
            symbol = sym_assume_symbol (dict-> link, key_buffer, link-> value);
            if (symbol)
                symbol-> data = link;
          }
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_item_link

    Synopsis: Get value for a link id.
    ---------------------------------------------------------------------[>]-*/

char *
get_item_link (long id, long field_id, DICT_CTX *dict)
{
    char
        key_buffer [20];

    sprintf (key_buffer, "%ld_%ld", field_id, id);
    return (sym_get_value (dict-> link, key_buffer, NULL));
}


/*  ---------------------------------------------------------------------[<]-
    Function: populate_dictionary

    Synopsis: Create new entries for each term in specified language, where
    entries do not already exist.  Copies the value for the default language
    if defined, else gives the term the value "".  Returns number of terms
    translated.
    ---------------------------------------------------------------------[>]-*/

int
populate_dictionary (DICT_CTX *dict, char *language, char *source_lang)
{
    char
        key_buffer [20],
        *value,
        *source = "xx";
    long
        index;
    SYMBOL
        *symbol;

    ASSERT (dict);
    ASSERT (language);

    if (source_lang)
        source = source_lang;

    for (index = 1; index <= dict-> max_id; index++)
      {
        sprintf (key_buffer, "%ld_%s", index, language);
        symbol = sym_lookup_symbol (dict-> translated, key_buffer);
        if (symbol == NULL)
          {
            value = get_dictionary_value (index, source_lang, dict, FALSE, TRUE);
            if (value == NULL)
                value = get_dictionary_value (index, "", dict, FALSE, TRUE);
            if (value == NULL)
                value = mem_strdup ("");
            add_langvalue (value, index, language, FALSE, FALSE, FALSE, FALSE,
                           0, NULL, dict);
            mem_free (value);
          }
      }

    return (0);
}

/*  ---------------------------------------------------------------------[<]-
    Function: remove_unused_from_dictionary

    Synopsis: Remove all unused object from dictionary
    ---------------------------------------------------------------------[>]-*/

void
remove_unused_from_dictionary (DICT_CTX *dict)
{
    SYMBOL
        *translation,
        *next,
        *symbol;
    long
        id,
        index;
    char
        key_buffer [20];

#define DELETE_TRANSLATION                                             \
    translation = sym_lookup_symbol (dict-> translated, key_buffer);   \
    if (translation) {                                                 \
        if (translation-> data)                                        \
            free_lang_value ((LANG_VALUE *)translation-> data);        \
        sym_delete_symbol (dict-> translated, translation);            \
    }

    if (dict)
      {
        if (dict-> def_value)
          {
            symbol = dict-> def_value-> symbols;
            while (symbol)
               {
                 if (symbol-> data
                 &&  ((LANG_VALUE *)symbol-> data)-> used == FALSE)
                   {
                     lang_value_deleted++;
                     id = symbol-> value? atol (symbol-> value): 0;
                     sprintf (key_buffer, "%d", id);
                     DELETE_TRANSLATION;
                     for (index = 0;
                          index < MAX_LANGUAGE && dict-> language [index] != NULL;
                          index++)
                      {
                        sprintf (key_buffer, "%d_%s", id, dict-> language [index]);
                        DELETE_TRANSLATION;
                      }
                     next = symbol-> next;
                     sym_delete_symbol (dict-> def_value, symbol);
                     symbol = next;
                   }
                 else
                     symbol = symbol-> next;
               }
          }

        if (dict-> usage)
          {
            symbol = dict-> usage-> symbols;
            while (symbol)
               {
                 if (symbol-> data
                 &&  ((USAGE *)symbol-> data)-> used == FALSE)
                   {
                     usage_deleted++;
                     free_usage ((USAGE *)symbol-> data);
                     next = symbol-> next;
                     sym_delete_symbol (dict-> usage, symbol);
                     symbol = next;
                   }
                 else
                     symbol = symbol-> next;
               }
          }

        if (dict-> screen)
          {
            symbol = dict-> screen-> symbols;
            while (symbol)
               {
                 if (symbol-> data
                 &&  ((SCREEN *)symbol-> data)-> used == FALSE)
                   {
                     screen_deleted++;
                     free_screen ((SCREEN *)symbol-> data);
                     next = symbol-> next;
                     sym_delete_symbol (dict-> screen, symbol);
                     symbol = next;
                   }
                 else
                     symbol = symbol-> next;
               }
          }
        if (dict-> link)
          {
            symbol = dict-> link-> symbols;
            while (symbol)
               {
                 if (symbol-> data
                 &&  ((VALUE_LINK *)symbol-> data)-> used == FALSE)
                   {
                     link_deleted++;
                     free_value_link ((VALUE_LINK *)symbol-> data);
                     next = symbol-> next;
                     sym_delete_symbol (dict-> link, symbol);
                     symbol = next;
                   }
                 else
                     symbol = symbol-> next;
               }
          }
    }
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_xml_usage

    Synopsis: Return the first screen name for one item
    ---------------------------------------------------------------------[>]-*/

static char *
get_xml_usage (XML_ITEM *field_item)
{
    char
        *feedback = NULL;
    XML_ITEM
        *item;

    FORCHILDREN (item, field_item)
      {
        if (lexcmp (xml_item_name (item), "usage") == 0)
          {
            feedback = xml_get_attr (item, "value", NULL);
            break;
          }
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: get_bin_usage

    Synopsis: Return the first screen name for one item
    ---------------------------------------------------------------------[>]-*/

static char *
get_bin_usage  (long field_id, DICT_CTX *dict)
{
    char
        key_buffer [20],
        *feedback = NULL;
    int
        index;
    SYMBOL
        *sym_usage;

    for (index = 1; index <= dict-> max_screenid; index++)
      {
        sprintf (key_buffer, "%ld_%d", field_id, index);
        sym_usage = sym_lookup_symbol (dict-> usage, key_buffer);
        if (sym_usage)
          {
            sprintf (key_buffer, "%d", index);
            feedback = sym_get_value (dict-> screen_id, key_buffer, NULL);
            break;
          }
       }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: read_lang_value_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

LANG_VALUE *
read_lang_value_rec  (FILE *file)
{
    LANG_VALUE
        *value = NULL;
    long
        alloc_size;
    byte
        status;
    size_t
        read_size;
    UCODE
        *ustring;

    ASSERT (file);

    value = mem_alloc (sizeof (LANG_VALUE));
    if (value)
      {    
        memset (value, 0, sizeof (LANG_VALUE));
        read_size = fread (&value-> field_id,    sizeof (value-> field_id), 1, file);
        if (read_size)
          {
            read_size = fread (value-> language, 5,                         1, file);
            strcrop (value-> language);
          }
        if (read_size)
            read_size = fread (&value-> ext_id,  sizeof (value-> ext_id),   1, file);
        if (read_size)
            read_size = fread (&status,          1,                         1, file);
        if (status & LANG_IS_DEFAULT)
            value-> is_default = TRUE;
        if (status & LANG_IS_UNICODE)
            value-> is_unicode = TRUE;
        if (status & LANG_IS_TECHNICAL)
            value-> is_technical = TRUE;
        if (status & LANG_MULTI_VALUE)
            value-> multi_value = TRUE;
        if (status & LANG_UNIQUE_VALUE)
            value-> unique_value = TRUE;
        if (read_size)
            read_size = fread (&value-> length,  sizeof (value-> length),   1, file);
        if (read_size && value-> length > 0)
          {
            alloc_size = value-> length + 1;
            if (value-> is_unicode)
                alloc_size++;
            value-> value = mem_alloc (alloc_size);
            if (value-> value)
              {
                read_size = fread (value-> value, 1, value-> length, file);
                memset (&value-> value [read_size], 0, alloc_size - read_size);
                value-> length = read_size;
                if (value-> is_unicode)
                  {
                    value-> length /= UCODE_SIZE;   
                    ustring = (UCODE *)value-> value;
                    http_encode_umeta ((UCODE *)buffer, &ustring, BUFFER_SIZE/2, TRUE);
                    mem_free (value-> value);
                    value-> value = ucode2ascii ((UCODE *)buffer);
                  }
              }
          }
      }
    if (read_size <= 0)
      {
        if (value)
          {
            if (value-> value)
                mem_free (value-> value);
            mem_free (value);
          }
        value = 0;
      }
    return (value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: write_lang_value_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

long        
write_lang_value_rec (FILE *file, LANG_VALUE *value)
{
    long
        size = 0;
    byte
        obj_type = OBJTYPE_LANG,
        status = 0;
    UCODE
        *old,
        *ustring;

    ASSERT (file);
    ASSERT (value);

    size += fwrite (&obj_type, 1, 1, file);
    size += fwrite (&value-> field_id, sizeof (value-> field_id), 1, file);
    strcrop (value-> language);
    size += fwrite (value-> language,  5,                         1, file);
    size += fwrite (&value-> ext_id,   sizeof (value-> ext_id),   1, file);
    if (value-> is_default)
        status |= LANG_IS_DEFAULT;
    if (value-> is_technical)
        status |= LANG_IS_TECHNICAL;
    if (value-> multi_value)
        status |= LANG_MULTI_VALUE;
    if (value-> unique_value)
        status |= LANG_UNIQUE_VALUE;
    if (value-> is_unicode)
      {
        status |= LANG_IS_UNICODE;

        ustring = (UCODE *)ascii2ucode (value-> value);
        old = ustring;
        http_decode_umeta ((UCODE *)buffer, &ustring, wcslen (ustring) + 1);
        value-> length = wcslen ((UCODE *)buffer) * UCODE_SIZE;
        if (old)
            mem_free (old);
       }
    size += fwrite (&status,           1,                         1, file);

    size += fwrite (&value-> length,   sizeof (value-> length),   1, file);
    if (value-> length && value-> value)
      {
        if (value-> is_unicode)
            size += fwrite (buffer, value-> length,               1, file);
        else
            size += fwrite (value-> value, value-> length,        1, file);
      }
    return (size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_lang_value

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void        
free_lang_value      (LANG_VALUE *value)
{
    if (value)
      {
        if (value-> value)
            mem_free (value-> value);
        mem_free (value);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: read_usage_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

USAGE *
read_usage_rec       (FILE *file)
{
    USAGE
        *usage = NULL;
    size_t
        read_size;

    ASSERT (file);

    usage = mem_alloc (sizeof (USAGE));
    if (usage)
      {    
        memset (usage, 0, sizeof (USAGE));
        read_size = fread (&usage-> lang_value_id, sizeof (usage-> lang_value_id), 1, file);
        if (read_size)
            read_size = fread (&usage-> screen_id, sizeof (usage-> screen_id),     1, file);
        if (read_size)
            read_size = fread (&usage-> ext_id,    sizeof (usage-> ext_id),        1, file);
      }
    if (read_size <= 0 && usage)
      {
        mem_free (usage);
        usage = NULL;
      }

    return (usage);
}


/*  ---------------------------------------------------------------------[<]-
    Function: write_usage_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

long
write_usage_rec      (FILE *file, USAGE *value)
{
    long
        size = 0;
    byte
        obj_type = OBJTYPE_USAGE,
        status = 0;

    ASSERT (file);
    ASSERT (value);

    size += fwrite (&obj_type, 1, 1, file);
    size += fwrite (&value-> lang_value_id, sizeof (value-> lang_value_id), 1, file);
    size += fwrite (&value-> screen_id,     sizeof (value-> screen_id),     1, file);
    size += fwrite (&value-> ext_id,        sizeof (value-> ext_id),        1, file);
    return (size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_usage

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
free_usage (USAGE *usage)
{
    if (usage)
      {
        mem_free (usage);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: read_screen_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

SCREEN *
read_screen_rec (FILE *file)
{
    SCREEN
        *screen = NULL;
    byte
        value_size = 0;
    size_t
        read_size;

    ASSERT (file);

    screen = mem_alloc (sizeof (SCREEN));
    if (screen)
      {
        memset (screen, 0, sizeof (SCREEN));
        read_size = fread (&screen-> id,         sizeof (screen-> id),     1, file);
        if (read_size)
            read_size = fread (&screen-> ext_id, sizeof (screen-> ext_id), 1, file);
        if (read_size)
            read_size = fread (&value_size, 1, 1, file);
        if (read_size && value_size > 0)
          {
            screen-> value = (char *)mem_alloc (value_size + 1);
            if (screen-> value)
              {            
                read_size = fread (screen-> value, 1, value_size, file);
                screen-> value [read_size] = '\0';
              }
          }       
      }
    if (read_size <= 0 && screen)
      {
        if (screen-> value)
            mem_free (screen-> value);
        mem_free (screen);
        screen = NULL;
      }
    return (screen);
}


/*  ---------------------------------------------------------------------[<]-
    Function: write_screen_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

long
write_screen_rec     (FILE *file, SCREEN *screen)
{
    long
        size = 0;
    byte
        obj_type = OBJTYPE_SCREEN,
        value_size = 0;

    ASSERT (file);
    ASSERT (screen);

    if (screen-> value)
        value_size = (byte)strlen (screen-> value);

    size += fwrite (&obj_type, 1, 1, file);
    size += fwrite (&screen-> id,     sizeof (screen-> id),     1, file);
    size += fwrite (&screen-> ext_id, sizeof (screen-> ext_id), 1, file);
    size += fwrite (&value_size,      1,                        1, file);
    if (value_size > 0)
        size += fwrite (screen-> value, value_size, 1, file);

    return (size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_screen

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
free_screen (SCREEN *screen)
{
   if (screen)
     {
       if (screen-> value)
           mem_free (screen-> value);
       mem_free (screen);
     }
}


/*  ---------------------------------------------------------------------[<]-
    Function: read_value_link_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

VALUE_LINK *
read_value_link_rec  (FILE *file)
{
    VALUE_LINK
        *link = NULL;
    short
        value_size = 0;
    size_t
        read_size;

    ASSERT (file);

    link = mem_alloc (sizeof (VALUE_LINK));
    if (link)
      {
        memset (link, 0, sizeof (VALUE_LINK));
        read_size = fread (&link-> lang_value_id, sizeof (link-> lang_value_id), 1, file);
        if (read_size)
            read_size = fread (&link-> index,     sizeof (link-> index),  1, file);
        if (read_size)
            read_size = fread (&link-> ext_id,    sizeof (link-> ext_id), 1, file);
        if (read_size)
            read_size = fread (&value_size,       sizeof (value_size),    1, file);
        if (read_size && value_size > 0)
          {
            link-> value = (char *)mem_alloc (value_size + 1);
            if (link-> value)
              {            
                read_size = fread (link-> value, 1, value_size, file);
                link-> value [read_size] = '\0';
              }
          }       
      }
    if (read_size <= 0 && link)
      {
        if (link-> value)
            mem_free (link-> value);
        mem_free (link);
        link = NULL;
      }
    return (link);
}


/*  ---------------------------------------------------------------------[<]-
    Function: write_value_link_rec

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

long
write_value_link_rec (FILE *file, VALUE_LINK *link)
{
    long
        size = 0;
    short
        value_size = 0;
    byte
        obj_type = OBJTYPE_LINK;

    if (link-> value)
        value_size = (byte)strlen (link-> value);

    size += fwrite (&obj_type, 1, 1, file);
    size += fwrite (&link-> lang_value_id, sizeof (link-> lang_value_id), 1, file);
    size += fwrite (&link-> index,         sizeof (link-> index),         1, file);
    size += fwrite (&link-> ext_id,        sizeof (link-> ext_id),        1, file);
    size += fwrite (&value_size,           sizeof (value_size),           1, file);
    if (value_size > 0)
        size += fwrite (link-> value, value_size, 1, file);

    return (size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_value_link

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
free_value_link      (VALUE_LINK *value_link)
{
    if (value_link)
      {
        if (value_link-> value)
            mem_free (value_link-> value);
        mem_free (value_link);
      }
}


static void
dump_lang_value_rec (FILE *file, LANG_VALUE *value)
{
    long
        size = 0;
    byte
        obj_type = OBJTYPE_LANG,
        status = 0;
 
    ASSERT (file);
    ASSERT (value);

    fprintf (file, "%6ld %5s %s (%s%s%s)\n",
                   value-> field_id,
                   value-> language,
                   value-> value,
                   value-> is_technical? "T": "",
                   value-> multi_value?  "M": "",
                   value-> unique_value? "U": "");
}
