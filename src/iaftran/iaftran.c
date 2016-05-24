/*  ----------------------------------------------------------------<Prolog>-
    Name:       iaftran.c
    Title:      IAF Markup Translator 1.0 main module
    Package:    Markup Translator

    Written:    1999/11/11  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/05/03  Pascal Antonnaux <pascal@imatix.com>

    Copyright:  Copyright (c) 1991-2001 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"
#include "markdict.h"
#include "marklib.h"
#include "markhtml.h"

#define CUR_VERSION "1.6a"

#define PRG_NAME                                                             \
        "IAF Markup Translator " CUR_VERSION " (c) iMatix Corporation 2002"
#define EXENAME "iAFTRAN"

#define USAGE                                                               \
    "\nSyntax: iaftran [options...] source\n"                               \
    "Source: file pattern or directory (ex: direct\\*.htm)\n"               \
    "        you can separate different file pattern by ';'\n"              \
    "        (ex: *.asp;*.htm;*.php)\n"                                     \
    "Options:\n"                                                            \
    "  -t directory     Target directory for transleted file\n"             \
    "  -d dictionary    Dictionary name (default: dict.tdb)\n"              \
    "  -r               Recursive mode on\n"                                \
    "  -l  language     Default language code (default: xx)\n"              \
    "  -l2 language     Target language for language copy\n"                \
    "  -x               Translate mode (default is create dictionary)\n"    \
    "  -p               Copy value for one language to other language\n"    \
    "  -mn              translate mode value: 1 = static\n"                 \
    "                                         2 = format usage\n"           \
    "  -f  \"format\"     Format to translate, use %d for term id\n"        \
    "                   ex: ""<%%=Translate (%d, UserLanguage)%%>""\n"      \
    "  -fs \"format\"     Format to translate server script\n"              \
    "                   ex: ""Translate (%d, UserLanguage)""\n"             \
    "  -u               make Unicode to multibyte in translation\n"         \
    "  -e               Remove unused value from dictionary\n"              \
    "  -h               Show summary of command-line options\n"             \
    "\nThe order of arguments is not important. Switches and filenames\n"   \
    "are case sensitive. See documentation for detailed information.\n"

#define CONFIG_NAME    "iaftran.cfg"

/*- Global Variables --------------------------------------------------------*/

static char
    *language_code     = NULL,
    *source_dir        = NULL,
    *target_dir        = NULL,
    *language          = NULL,
    *language2         = NULL,
    *trans_format      = NULL,
    *script_format     = NULL,
    *dictionary        = NULL;
static Bool
    multibyte_trans    = FALSE,
    args_ok            = TRUE,          /*  Were the arguments okay?         */
    recursive_mode     = FALSE,
    translate_mode     = FALSE,
    remove_mode        = FALSE,
    populate_mode      = FALSE;
static int
    translate_mode_val = 0;


/*  -------------------------------------------------------------------------
    Function: free_all_resources

    Synopsis: Free all allocated resources.
    -------------------------------------------------------------------------*/

void
free_all_resources (void)
{
    if (source_dir)
        mem_strfree (&source_dir);
    if (target_dir)
        mem_strfree (&target_dir);
    if (language)
        mem_strfree (&language);
    if (language2)
        mem_strfree (&language2);
    if (trans_format)
        mem_strfree (&trans_format);
    if (script_format)
        mem_strfree (&script_format);
    if (dictionary)
        mem_strfree (&dictionary);
}


int
main (int argc, char *argv [])
{
    DICT_CTX
        *dict;
    char
        full_name [256],
        main_path [256],
        *source,
        *full_dir,
        *cur_dir = NULL,
        **argparm = NULL;               /*  Argument parameter to pick-up    */
    int
        argn;                           /*  Argument number                  */
    IAFTRAN_CONFIG
        *iaftran_config = NULL;

    if (argc == 1)
      {
        printf ("%s%s", PRG_NAME, USAGE);
        free_all_resources ();
        exit (EXIT_SUCCESS);
       }
    for (argn = 1; argn < argc; argn++)
      {
        /*  If argparm is set, we have to collect an argument parameter      */
        if (argparm)
          {
            if (*argv [argn] != '-')    /*  Parameter can't start with '-'   */
              {
                *argparm = mem_strdup (argv [argn]);
                argparm = NULL;
              }
            else
              {
                args_ok = FALSE;
                break;
              }
          }
        else
        if (*argv [argn] == '-')
          {
            switch (argv [argn][1])
              {
                /*  These switches take a parameter                          */
                case 't': argparm = &target_dir;   break;
                case 'd': argparm = &dictionary;   break;
                case 'l':
                    if (argv [argn][2] == '2')
                        argparm = &language2;
                    else
                        argparm = &language;
                    break;
                case 'f':
                     if (argv [argn][2] == 's')
                         argparm = &script_format;
                     else
                         argparm = &trans_format;
                     break;
                case 'r': recursive_mode  = TRUE;   break;
                case 'x': translate_mode  = TRUE;   break;
                case 'p': populate_mode   = TRUE;   break;
                case 'u': multibyte_trans = TRUE;   break;
                case 'e': remove_mode     = TRUE;   break;
                case 'm':
                    translate_mode_val = argv [argn][2] - '0';
                    break;
                case 'h':
                    printf ("%s%s", PRG_NAME, USAGE);
                    free_all_resources ();
                    exit (EXIT_SUCCESS);

                /*  Anything else is an error                                */
                default:
                    args_ok = FALSE;
              }
          }
        else
          {
            if (argn == argc - 1)
                source_dir = mem_strdup (argv [argn]);
            else
                args_ok = FALSE;
            break;
          }
      }

    /*  Verify parameters are valid                                          */
    if (argparm)
      {
        coprintf ("Argument missing - type '%s -h' for help", EXENAME);
        mem_free (argparm);
        free_all_resources ();
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        coprintf ("Invalid arguments - type '%s -h' for help", EXENAME);
        free_all_resources ();
        exit (EXIT_FAILURE);
      }
    if (translate_mode)
      {
        if (target_dir == NULL)
          {
            printf ("%s Error: Target directory is required in translate mode\n",
                    EXENAME);
            free_all_resources ();
            exit (EXIT_FAILURE);
          }
        if (translate_mode_val == TRANSLATION_MODE_FORMAT
        &&  trans_format       == NULL)
          {
            printf ("%s Error: missing translate format\n",
                    EXENAME);
            free_all_resources ();
            exit (EXIT_FAILURE);
          }
      }
    if (source_dir == NULL && populate_mode == FALSE)
      {
        printf ("%s Error: missing source file name or pattern\n",
                 EXENAME);
        free_all_resources ();
        exit (EXIT_FAILURE);
      }
    if (populate_mode)
      {
        if (language2 == NULL)
          {
            printf ("%s Error: missing target language\n", EXENAME);
            free_all_resources ();
            exit (EXIT_FAILURE);
          }
      }
    if (dictionary == NULL)
        dictionary = mem_strdup ("dict.tbd");
    if (language   == NULL)
        language   = mem_strdup ("xx");
    if (translate_mode_val == 0)
        translate_mode_val = TRANSLATION_MODE_STATIC;

    /* Check if dictionary exist                                             */
    if (populate_mode == TRUE || translate_mode == TRUE)
      {
        if (file_exists (dictionary) == FALSE)
          {
            printf ("%s Error: dictionary %s don't exist\n",
                    EXENAME, dictionary);
            free_all_resources ();
            exit (EXIT_FAILURE);
          }
      }
    dict = load_bin_dictionary (dictionary);

    if (populate_mode == TRUE)
      {
        populate_dictionary (dict, language2, language);
        save_bin_dictionary (dictionary, dict);
      }
    else
      {
        /* Load config file                                                  */
        iaftran_config = iaftran_load_config (".", CONFIG_NAME);
        html_init ();
        /* Get file pattern with full directory and main path                */
        cur_dir  = get_curdir ();
        source = strtok (source_dir, ";");
        while (source)
          {
            full_dir = locate_path (cur_dir, source);
            strcpy  (full_name, clean_path (full_dir));
            mem_free (full_dir);
            strcpy (main_path, strip_file_name (full_name));

            if (translate_mode == FALSE)
                parse_markup_dir     (full_name,
                                      main_path,
                                      dict,
                                      recursive_mode,
                                      language,
                                      iaftran_config);
            else
                translate_markup_dir (full_name,
                                      main_path,
                                      dict,
                                      recursive_mode,
                                      language,
                                      target_dir,
                                      translate_mode_val,
                                      trans_format,
                                      script_format,
                                      multibyte_trans,
                                      iaftran_config);

            source = strtok (NULL, ";");
          }

        if (remove_mode == TRUE)
          {
            remove_unused_from_dictionary (dict);
            if (lang_value_deleted)
                coprintf ("%ld Value deleted", lang_value_deleted);
            if (usage_deleted)
                coprintf ("%ld usage deleted", usage_deleted);
            if (screen_deleted)
                coprintf ("%ld screen deleted", screen_deleted);
            if (link_deleted)
                coprintf ("%ld link deleted", link_deleted);
          }
        if (translate_mode == FALSE)
            save_bin_dictionary (dictionary, dict);

        mem_free (cur_dir);

        html_free ();
        if (iaftran_config)
            iaftran_free (iaftran_config);
      }

    free_dictionary (dict);
    free_all_resources ();
    mem_assert ();
    return (0);
}

