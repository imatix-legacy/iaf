/*  ----------------------------------------------------------------<Prolog>-
    Name:       markdict.h
    Title:      Markup Dictionary Functions
    Package:    Markup Translator

    Written:    1999/11/12 Pascal Antonnaux <pascal@imatix.com>
    Revised:    2001/05/31 Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to Load, save and update disctionnary.

    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _MARKDICT_INCLUDED_
#define _MARKDICT_INCLUDED_

#define MAX_LANGUAGE  20
/*- Structure definitions -------------------------------------------------- */

typedef struct _lang_value {
    long  field_id;                      /* Terms ID                          */
    char  language [6];                  /* Language code                     */
    char *value;                         /* Translated value                  */
    Bool  is_default;                    /* TRUE if default language value    */
    Bool  is_unicode;                    /* TRUE if value is unicode data     */
    Bool  is_technical;                  /* TRUE if don't require tranlation  */
    Bool  multi_value;                   /* Indicate multi instance of the    */
                                         /* same value in dictionary (for dif.*/
                                         /* translation of same value         */
    Bool  unique_value;                  /* Value only for one screen         */
    long  ext_id;                        /* External ID                       */
    short length;                        /* Translated value length           */
} LANG_VALUE;

typedef struct _usage {
    long lang_value_id;                 /* Language value ID                 */
    long screen_id;                     /* Screen ID                         */
    long ext_id;                        /* External ID                       */
} USAGE;

typedef struct _screen {
    long id;                            /* Screen ID                         */
    char *value;                        /* Screen Name                       */
    long ext_id;                        /* External ID                       */
} SCREEN;

typedef struct _value_link {
    long  lang_value_id;                /* Language value ID                 */
    byte  index;                        /* Index of link                     */
    char *value;                        /* Link value                        */
    long ext_id;                        /* External ID                       */
} VALUE_LINK;

typedef struct _dict_ctx {
    SYMTAB   *def_value;
    SYMTAB   *translated;
    SYMTAB   *screen;
    SYMTAB   *screen_id;
    SYMTAB   *usage;
    SYMTAB   *link;

    char     *language [MAX_LANGUAGE];  /* All language in dictionary        */
    long      max_id;                   /* Maximum id value in dictionary    */
    long      max_screenid;             /* Maximum screen id value           */
    short     max_link;                 /* Maximum link index                */
    Bool      recursive;                /* True if want parsing sub directory*/
} DICT_CTX;

/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

LANG_VALUE *read_lang_value_rec  (FILE *file);
long        write_lang_value_rec (FILE *file, LANG_VALUE *value);
void        free_lang_value      (LANG_VALUE *value);
USAGE      *read_usage_rec       (FILE *file);
long        write_usage_rec      (FILE *file, USAGE *value);
void        free_usage           (USAGE *usage);
SCREEN     *read_screen_rec      (FILE *file);
long        write_screen_rec     (FILE *file, SCREEN *value);
void        free_screen          (SCREEN *screen);
VALUE_LINK *read_value_link_rec  (FILE *file);
long        write_value_link_rec (FILE *file, VALUE_LINK *value);
void        free_value_link      (VALUE_LINK *value_link);



DICT_CTX *load_bin_dictionary     (const char *name);
DICT_CTX *load_xml_dictionary     (const char *name);
void      save_bin_dictionary     (const char *name, DICT_CTX *dict);
void      save_xml_dictionary     (const char *name, DICT_CTX *dict);
void      free_dictionary         (DICT_CTX *dict);
long      add_dictionary_item     (char *value, char *language, DICT_CTX *dict);
long      add_dictionary_item_ext (UCODE *value, char *language, long ext_id,
                                   Bool is_technical, Bool multi, Bool unique,
                                   long id, char *screen_name, DICT_CTX *dict);
long      get_dictionary_item     (char *value, char *screen_name, DICT_CTX *dict);
char     *get_dictionary_value    (long id, char *language, DICT_CTX *dict,
                                   Bool resolve_link, Bool multibyte_result);
char     *get_simple_value        (long id, char *language, DICT_CTX *dict,
                                   char *default_value);
void      update_usage            (char *file_name, long field_id, long ext_id, 
                                   DICT_CTX* dict);
void      update_item_link        (char *value, long id, long field_id, DICT_CTX *dict);
long      update_screen           (char *screen_name, long ext_id, DICT_CTX *dict);
char *    get_item_link           (long id, long field_id, DICT_CTX *dict);
int       populate_dictionary     (DICT_CTX *dict, char *language,
                                   char *source_lang);

#ifdef __cplusplus
}
#endif

#endif
