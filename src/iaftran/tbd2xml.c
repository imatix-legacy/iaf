/*  ----------------------------------------------------------------<Prolog>-
    Name:       tbd2xml.c
    Title:      Convert TML dictionary to binary version
    Package:    Markup Translator

    Written:    2001/05/10  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2001/05/10  Pascal Antonnaux <pascal@imatix.com>

    Copyright:  Copyright (c) 1991-2001 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"
#include "markdict.h"
#include "marklib.h"

#define CUR_VERSION "1.0"

#define PRG_NAME                                                             \
        "TBD (Translation Binary Dictionary) convert " CUR_VERSION " (c) iMatix Corporation 2001"

#define USAGE                                                               \
    "Syntax: tbd2xml [option] source \n"                                    \
    "Source: dictionary name without extension\n"                           \
    "Options:\n"                                                            \
    "  -x     Convert xml dictionary to binary\n"                           \
    "  -h     Show summary of command-line options\n"                       \
    "\nThe order of arguments is not important. Switches and filenames\n"   \
    "are case sensitive. See documentation for detailed information.\n"

int
main (int argc, char *argv[])
{
    char
        *source,
        xml_file [256],
        bin_file [256];
    Bool
        xml_convert = FALSE;
    DICT_CTX
        *dict = NULL;

    if (argc < 2 || lexcmp (argv [1], "-h") == 0)
      {
         puts (PRG_NAME);
         puts (USAGE);
         exit (1);
      }

    if (argc == 3)
      {
        xml_convert = TRUE;
        source = argv [2];
      }
    else
        source = argv [1];

    sprintf (xml_file, "%s.xml", source);
    sprintf (bin_file, "%s.tbd", source);

    if (xml_convert == TRUE)
      {
        if (file_exists (xml_file))
          {        
            dict = load_xml_dictionary (xml_file);
            if (dict)
              {
                printf ("  Writing to %s...\n", bin_file);
                save_bin_dictionary (bin_file, dict);
              }
          }
        else
            printf ("Error: input file %s missing", xml_file);
      }
    else
      {
        if (file_exists (bin_file))
          {        
            dict = load_bin_dictionary (bin_file);
            if (dict)
              {
                printf ("  Writing to %s...\n", xml_file);
                save_xml_dictionary (xml_file, dict);
              }
          }
        else
            printf ("Error: input file %s missing", bin_file);
      }

    if (dict)
        free_dictionary (dict);

    mem_assert ();
    return (0);
}