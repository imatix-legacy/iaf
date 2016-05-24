/*  ----------------------------------------------------------------<Prolog>-
    Name:       markcfg.c
    Title:      Markup Configuration Functions
    Package:    Markup Translator

    Written:    2001/03/12 Sergio Mazzoleni <sergio@imatix.net>
    Revised:    2001/03/12 Sergio Mazzoleni <sergio@imatix.net>

    Synopsis:   Provides functions to read Markup configuration
                and to abstract the XML config layer
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"

/* IN stands for Iten Name */
/* AN stands for Attribute Name */
static const char
  *IN_ITEM_CONFIG             = "item_config",
  *IN_FILE_CONFIG             = "file_config",
  *IN_CONTENT                 = "content",
  *IN_ATTR                    = "attr",
  *AN_PATTERN                 = "pattern",
  *AN_NAME                    = "name";


IAFTRAN_CONFIG *iaftran_load_config (const char *path, const char *file_name)
{
    IAFTRAN_CONFIG *res = NULL;
    IAFTRAN_CONFIG *root= NULL;
    int rc;

    rc = xml_load_file (&root, path, file_name, FALSE);
    if (rc == XML_NOERROR)
      {
        res = xml_first_child (root);
        xml_detach (res);
        xml_free (root);
      }

    return res;
}

FILE_CONFIG *iaftran_get_file_config (
    IAFTRAN_CONFIG *config,
    const char *file_name
  )
{
    ITEM_CONFIG *res = NULL;
    ITEM_CONFIG *file_config = NULL;
    const char *pattern = NULL;

    if ((config == NULL) || (file_name == NULL))
        return NULL;

    FORCHILDREN (file_config, config)
      {
        ASSERT (streq(xml_item_name(file_config), IN_FILE_CONFIG));

        pattern = xml_get_attr (file_config, AN_PATTERN, NULL);
        ASSERT (pattern != NULL);           /* bad config file */

        if (file_matches (file_name, pattern))
          {
            res = file_config;
            break;
          }
      }

    return res;
}

ITEM_CONFIG *iaftran_get_item_config (
    FILE_CONFIG *file_config,
    XML_ITEM    *xml_item
  )
{
    const char *item_name = NULL;
    const char *config_item_name = NULL;
    ITEM_CONFIG *item_config = NULL;
    ITEM_CONFIG *res = NULL;

    if (!file_config || !xml_item)
        return NULL;

    item_name = xml_item_name (xml_item);
    ASSERT (item_name != NULL);

    FORCHILDREN (item_config, file_config)
        if (streq (xml_item_name(item_config), IN_ITEM_CONFIG ))
          {
            config_item_name = xml_get_attr (item_config, AN_NAME, NULL);

            if ((config_item_name != NULL) && (streq (config_item_name, item_name)))
              {
                res = item_config;
                break;
              }
          }

    return res;
}

Bool iaftran_must_handle_attr (
    ITEM_CONFIG *item_config,
    const char  *attr_name
  )
{
    const char *child_value;
    const char *child_type;
    XML_ITEM   *child;
    Bool        res = FALSE;

    if (!item_config || !attr_name)
        return FALSE;

    FORCHILDREN (child, item_config)
      {
        child_type = xml_item_name (child);  /* ATTR or CONTENT */

        if (streq (child_type, IN_ATTR))
          {
            child_value = xml_get_attr (child, AN_NAME, NULL);
            ASSERT (child_value);         /* bad config file ? */

            if (streq (child_value, attr_name))
              {
                res = TRUE;
                break;
              }
          }
      }

    return res;
}


Bool iaftran_must_handle_content (ITEM_CONFIG *item_config)
{
    const char *child_type;
    XML_ITEM   *child;
    Bool        res = FALSE;

    if (!item_config)
        return FALSE;

    FORCHILDREN (child, item_config)
      {
        child_type = xml_item_name (child);  /* ATTR or CONTENT */

        if (streq (child_type, IN_CONTENT))
          {
            res = TRUE;
            break;
          }
      }

    return res;
}


