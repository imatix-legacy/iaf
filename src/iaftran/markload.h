/*  ----------------------------------------------------------------<Prolog>-
    Name:       markload.h
    Title:      Markup Load file Function
    Package:    Markup Translator

    Written:    1999/11/11 Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/05/13 Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to read and write Markup files, and manipulate
                Markup data in memory as list structures.  
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _MARKLOAD_INCLUDED
#define _MARKLOAD_INCLUDED


/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void        initialise_markup_resource (MARKUP_DEF *tag_tab, int nb_tag);
void        free_markup_resource       (void);
MARKUP_TAG *markup_load                (char       *fname,
                                        MARKUP_DEF *tag_tab,
                                        int         nb_tag);

#ifdef __cplusplus
}
#endif

#endif
