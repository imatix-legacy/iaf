/*  ----------------------------------------------------------------<Prolog>-
    Name:       markcfg.h
    Title:      Markup Configuration Functions
    Package:    Markup Translator

    Written:    2001/03/12 Sergio Mazzoleni <sergio@imatix.net>
    Revised:    2001/03/20 Sergio Mazzoleni <sergio@imatix.net>

    Synopsis:   Provides functions to read Markup configuration
                and to abstract the XML config layer
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _MARKCFG_INCLUDED_
#define _MARKCFG_INCLUDED_

typedef XML_ITEM IAFTRAN_CONFIG;
typedef XML_ITEM FILE_CONFIG;
typedef XML_ITEM ITEM_CONFIG;

#define iaftran_free   xml_free

/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void            config_filter               (XML_ITEM *item, IAFTRAN_CONFIG *config);
ITEM_CONFIG    *get_item_config             (XML_ITEM *item);
IAFTRAN_CONFIG *iaftran_load_config         (const char *path, const char *file_name);
FILE_CONFIG    *iaftran_get_file_config     (IAFTRAN_CONFIG *config,
                                             const char *file_name);
ITEM_CONFIG    *iaftran_get_item_config     (FILE_CONFIG *config, XML_ITEM *item);
Bool            iaftran_must_handle_attr    (ITEM_CONFIG *config, const char *attr_name);
Bool            iaftran_must_handle_content (ITEM_CONFIG *config);

#ifdef __cplusplus
}
#endif

#endif
