/*  ----------------------------------------------------------------<Prolog>-
    Name:       marklib.h
    Title:      Markup Library Functions
    Package:    Markup Translator

    Written:    1999/11/11 Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/01/06 Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to read and write Markup files,
                and manipulate Markup data in memory as list structures.
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _MARKLIB_INCLUDED_
#define _MARKLIB_INCLUDED_

#include "markdict.h"

/* -------------------------------------------------------------------------
    An Markup tree is built as the following recursive structure:

                   .---------.    .----------.
                 .-:  Data   :<-->:   0..n   :  Data are not sorted.
    .----------. : :  Head   :    :   datas  :
    :   TAG    :-' `---------'    `----------'
    :   node   :-. .---------.    .----------.
    `----------' : :  Child/ :<-->:   0..n   :  Each child node is the root
                 `-:  Value  :    : children :  of its own tree of nodes.
                   `---------'    `----------'
   ------------------------------------------------------------------------- */

/*- Structure definitions -------------------------------------------------- */

typedef struct _MARKUP_TAG {
    struct _MARKUP_TAG
        *next,                          /*  Next tag in list                 */
        *prev,                          /*  Previous tag in list             */
        *parent;                        /*  Parent if this is a child        */
    int
        order,                          /*  Display order in parent TAG      */
        nb_items,                       /*  Number of data and children      */
        tag_id;                         /*  Id of tag                        */
    char
        *value;                         /*  Value node, allocated string     */
    LIST
        data,                           /*  List of data, 0 or more          */
        children;                       /*  List of children, 0 or more      */
} MARKUP_TAG;

typedef struct _MARKUP_LINK {
    struct _MARKUP_LINK
        *next,                          /*  Next link in list                */
        *prev;                          /*  Previous link in list            */
    struct _MARKUP_DATA
        *parent;                        /*  Parent tag of this data          */
    char
        *value;                         /*  Value of link                    */
    int
        order;                          /*  Order of this link               */
} MARKUP_LINK;

typedef struct _MARKUP_DATA {
    struct _MARKUP_DATA
        *next,                          /*  Next data in list                */
        *prev;                          /*  Previous data in list            */
    struct _MARKUP_TAG
        *parent;                        /*  Parent tag of this data          */
    int
         nbr_link,                      /*  Number of link in this data      */
         script,                        /*  IF scripting data, client or ser.*/
         dico_id,                       /*  ID of dictionary item            */
         order;                         /*  Display order in parent TAG      */
    char
        *value;                         /*  data value, may be null          */
    LIST
        link;                           /*  List of link                     */
} MARKUP_DATA;

typedef struct _markup_def {
  int   id;                             /*  Id of tag                        */
  char *tag;                            /*  Tag name                         */
  Bool  end_tag;                        /*  End of tag required              */
  char *end_token;                      /*  Token for end of tag             */
  short script;                         /*  If Scripting tag (client or ser.)*/
  Bool  skip;                           /*  Skip tag, stay in data           */
  Bool  link;                           /*  Link markup                      */
  Bool  parse_string;                   /*  Add attribute in data tree       */
} MARKUP_DEF;


#define CLIENT_SCRIPT    1
#define SERVER_SCRIPT    2

#define TRANSLATION_MODE_STATIC 1
#define TRANSLATION_MODE_FORMAT 2

/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/*  Markup tag functions                                                     */
MARKUP_TAG  *tag_new             (MARKUP_TAG *parent,
                                  const int   id,
                                  const int   order,
                                  const char *value);
MARKUP_TAG  *tag_insert          (MARKUP_TAG *sibling,
                                  const int   id,
                                  const int   order,
                                  const char *value);
void         tag_modify_value    (MARKUP_TAG *tag,
                                  const char *value);
int          tag_id              (MARKUP_TAG *tag);
int          tag_order           (MARKUP_TAG *tag);
char        *tag_value           (MARKUP_TAG *tag);
void         tag_free            (MARKUP_TAG *tag);

void         tag_del_skipped     (MARKUP_TAG *tag, MARKUP_DEF *table);
void         tag_remove_link     (MARKUP_TAG *tag, MARKUP_DEF *table);
void         tag_add_string_attr (MARKUP_TAG *tag, MARKUP_DEF *table);

/*  TAG tree mainipulation                                                   */
void         tag_attach          (MARKUP_TAG *parent, MARKUP_TAG *tag);
void         tag_detach          (MARKUP_TAG *tag);

/*  TAG family navigation                                                    */
MARKUP_TAG  *tag_first_child     (MARKUP_TAG  *tag);
MARKUP_TAG  *tag_next_sibling    (MARKUP_TAG  *tag);
MARKUP_TAG  *tag_parent          (MARKUP_TAG  *tag);
MARKUP_TAG  *tag_get             (MARKUP_TAG  *tag, int order);

/*  TAG data functions                                                       */
int          tag_put_data        (MARKUP_TAG  *tag,
                                  const char  *value, const short script);
int          tag_insert_data     (MARKUP_TAG  *item,  const int   order,
                                  const char  *value, const short script);
char        *tag_data_value      (MARKUP_DATA *tag);
int          tag_data_order      (MARKUP_DATA *data);
void         tag_free_data       (MARKUP_DATA *data);
Bool         tag_data_is_empty   (MARKUP_DATA *data);


/*  TAG data navigation  */
MARKUP_DATA *tag_first_data      (MARKUP_TAG  *tag);
MARKUP_DATA *tag_next_data       (MARKUP_DATA *data);
MARKUP_DATA *tag_get_data        (MARKUP_TAG  *tag, const int order);

/*  TAG link function                                                        */
int          tag_new_link        (MARKUP_DATA *data, char *value);
void         tag_free_link       (MARKUP_LINK *link);

/*  TAG link navigation                                                      */
MARKUP_LINK *tag_first_link      (MARKUP_DATA  *data);
MARKUP_LINK *tag_next_link       (MARKUP_LINK  *link);
MARKUP_LINK *tag_get_link        (MARKUP_DATA  *data, const int order);
/*  Macros to treat all children and all data                                */

#define FORTAG(child,tag)    for (child  = tag_first_child (tag);           \
                                        child != NULL;                      \
                                        child  = tag_next_sibling (child))


#define FORDATA(data,tag)    for (data  = tag_first_data (tag);             \
                                  data != NULL;                             \
                                  data  = tag_next_data (data))             \
                                  if (tag_data_value (data))

#define FORLINK(link, data)  for (link  = tag_first_link (data);            \
                                  link != NULL;                             \
                                  link  = tag_next_link (link))

/* Markup functions */

int         markup_save           (MARKUP_TAG *tag,  const char *filename);
void        update_dictionary     (DICT_CTX   *dict, MARKUP_TAG *main_tag,
                                   char *default_lang, char *file_name);
MARKUP_TAG *parse_markup_file     (char *file_name, char *path, DICT_CTX *dict,
                                   char *language, IAFTRAN_CONFIG *cfg);
void        parse_markup_dir      (char *file_name, char *main_path,
                                   DICT_CTX *dict, Bool recursive,
                                   char *language, IAFTRAN_CONFIG *cfg);

void        translate_markup      (MARKUP_TAG *tag, DICT_CTX *dict,
                                   char *language, int trans_mode,
                                   char *trans_format, char *script_format,
                                   char *screen_name, Bool multibyte);
MARKUP_TAG *translate_markup_file (char *file_name, char *path, DICT_CTX *dict,
                                   char *language, char *path_dest,
                                   int   trans_mode, char *trans_format,
                                   char *script_format, Bool multibyte,
                                   IAFTRAN_CONFIG *cfg);
void        translate_markup_dir  (char *file_name, char *main_path,
                                   DICT_CTX *dict, Bool recursive,
                                   char *language, char *path_dest,
                                   int trans_mode, char *trans_format,
                                   char *script_format, Bool multibyte,
                                   IAFTRAN_CONFIG *cfg);

qbyte       get_lang_codepage     (const char *code);
char       *get_lang_charset      (const char *code);
char       *get_lang_name         (const char *code);



#ifdef __cplusplus
}
#endif

#endif

