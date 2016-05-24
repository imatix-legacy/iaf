/*  ----------------------------------------------------------------<Prolog>-
    Name:       markfile.c
    Title:      Markup Translator function for non markup file :-)
    Package:    Markup Translator

    Written:    1999/12/15  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/01/06  Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to read and write Markup files, and manipulate
                Markup data in memory as list structures.
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"
#include "marklib.h"
#include "markfile.h"

#define BUFFER_SIZE 2048

/*- Local variables ---------------------------------------------------------*/

static char
    buffer [BUFFER_SIZE + 1];

/*  ---------------------------------------------------------------------[<]-
    Function: xlate_tagged_item

    Synopsis: Translate a buffer with taged item with value in requested
              language.
    ---------------------------------------------------------------------[>]-*/

//void
//xlate_tagged_item (char *output,
//                   char *input,
//                   DICT_CTX *dict,
//                   char *lang,
//                   long out_size,
//                   char *begin_tag)
//{
//    int
//        tag_length;
//    char
//        *in,
//        *out,
//        *end,
//        *value,
//        *tag = NULL;
//    long
//        id;

//    ASSERT (output);
//    ASSERT (input);
//    ASSERT (begin_tag);

//    tag_length = strlen (begin_tag);
//    in  = input;
//    out = output;
//    end = output + out_size;
//    while (*in)
//      {
//        /* If tag of translation found                                       */
//        if (lexncmp (in, begin_tag, tag_length) == 0)
//          {
//            in += tag_length;
//            tag = in;
//            while (*in && *in != ')')
//                in++;
//            *in++ = '\0';
//            id = atol (tag);
//            value = get_dictionary_value (id, lang, dict, TRUE);
//            if (value)
//              {
//                while (*value && out < end)
//                    *out++ = *value++;
//              }
//          }
//        else
//            *out++ = *in++;
//      };
//}

/*  ---------------------------------------------------------------------[<]-
    Function: tagged_file_load

    Synopsis: Load a file whitout markup. Only block begining with 'tag_begin'
    (ex: "!) and ending with 'tag_end (ex: !") is stored in data tag.
    Rem: Block is one line maximum. tag_begin and tag_end must be different.
    ---------------------------------------------------------------------[>]-*/

MARKUP_TAG *
tagged_file_load (char *file_name, char *tag_begin, char *tag_end)
{
    MARKUP_TAG
        *root_tag = NULL;
    FILE
        *file;
    char
        val,
        *begin,
        *end,
        *block;
    int
        tag_begin_len;
    long
        line = 0;

    ASSERT (file_name);
    ASSERT (tag_begin);
    ASSERT (tag_end);

    file = fopen (file_name, "rb");
    if (file)
      {
        root_tag = tag_new (NULL, 0, 0, "");
        tag_begin_len = strlen (tag_begin);
        while (fgets (buffer, BUFFER_SIZE, file) != NULL)
          {
            block = buffer;
            line++;
            while (block)
              {
                begin = txtfind (block, tag_begin);
                if (begin)
                  {
                    begin += tag_begin_len;
                    end = txtfind (begin, tag_end);
                    if (end)
                      {
                        begin--;
                        val    = *begin;
                        *begin = '\0';
                        tag_new (root_tag, 0, ++root_tag->nb_items, block);
                        *begin = val;
                        begin++;
                        val    = *end;
                        *end   = '\0';
                        tag_put_data (root_tag, begin, SERVER_SCRIPT);
                        *end   = val;
                        block  = end + 1;
                      }
                    else
                      {
                        coprintf ("Missing end of tag line %ld", line);
                        tag_new (root_tag, 0, ++root_tag->nb_items, block);
                        block = NULL;
                      }
                  }
                else
                  {
                    tag_new (root_tag, 0, ++root_tag->nb_items, block);
                    block = NULL;
                  }
              }
          }
        fclose (file);
      }
    return (root_tag);
}
