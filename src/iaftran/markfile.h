/*  ----------------------------------------------------------------<Prolog>-
    Name:       markfile.h
    Title:      Markup Translator function for non markup file :-)
    Package:    Markup Translator

    Written:    1999/12/15  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/01/26  Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to read and write Markup files, and manipulate
                Markup data in memory as list structures.  
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _MARKFILE_INCLUDED_
#define _MARKFILE_INCLUDED_

#include "marklib.h"
/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void        xlate_tagged_item (char *output, char *input, DICT_CTX *dict,
                               char *lang, long out_size, char *begin_tag);
MARKUP_TAG *tagged_file_load  (char *file_name, char *tag_begin,
                               char *tag_end);

#ifdef __cplusplus
}
#endif

#endif

