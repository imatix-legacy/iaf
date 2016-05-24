/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflsearch.h
    Title:      Function to sort search indexation result

    Written:    2001/08/21  iMatix Corporation <info@imatix.com>
    Revised:    2001/08/22

    Synopsis:   

    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#ifndef _SFLSEARCH_INCLUDED
#define _SFLSEARCH_INCLUDED

/*- Definition --------------------------------------------------------------*/

#define SEARCH_TOKEN_TYPE_SOUNDEX 1
#define SEARCH_TOKEN_TYPE_PREFIX  2
#define SEARCH_TOKEN_TYPE_NUMBER  3

#define PREFIX_SIZE               2

#define SEARCH_OPERATION_OR       1
#define SEARCH_OPERATION_AND      2
#define SEARCH_OPERATION_NAND     3
#define SEARCH_OPERATION_RP       4     /* Part of range                     */

#define SEARCH_QUERY_EQ           1
#define SEARCH_QUERY_GE           2
#define SEARCH_QUERY_LE           3
#define SEARCH_QUERY_RANGE        4

#define SEARCH_QUERY_CHAR         '>'

typedef struct _search_token{
    struct _search_token
           *next, *prev;
    char  *value;
    short  type;
    short  operation;
    short  query;
} SEARCH_TOKEN;

/*- Function declaration ----------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

long   sort_file        (char *source, char *target);
int    search_split     (const char *string, LIST *list, Bool search_mode);
void   free_token_list  (LIST *list);
void   dump_token_list  (LIST *list);
void   save_token_list  (FILE *file, LIST *list, char *field_name, char *id);
void   make_search_file (char *input, char *output);
void   save_xml_search  (char *input, char *output);

#ifdef __cplusplus
}
#endif

#endif