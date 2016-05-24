/*  ----------------------------------------------------------------<Prolog>-
    Name:       searchidx.c
    Title:      Function to index search item

    Written:    2001/08/21  iMatix Corporation <info@imatix.com>
    Revised:    2001/08/23

    Synopsis:   

    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include "sfl.h"
#include "sflsearch.h"

#define EXENAME "iafsearch"

#define CUR_VERSION "1.0"

#define PRG_NAME                                                             \
        "iAF Search Index Maker " CUR_VERSION " (c) iMatix Corporation 2001"

#define USAGE                                                                \
     "\nSyntax: iafsearch [option]\n"                                        \
     "Option:\n"                                                             \
     "  -i   Input file name  (default is 'search.dat')\n"                   \
     "  -o   Output file name (default is 'dbsearch.xml')\n"                 \
     "  -d   Debug mode, don't remove temporary file\n"                      \
     "  -t   Show time taken for processing\n"                               \
     "  -h  Show summary of command-line options\n"                          \
     "\nThe order of arguments is not important. Switches and filenames\n"   \
     "are case sensitive. See documentation for detailed information.\n"

#define TMP_FILE  "searchtmp.dat"
#define SORT_FILE "searchsort.dat"

int
main (int argc, char *argv [])
{
    clock_t
        t1,
        t2;
    Bool
        args_ok = TRUE,
        timer   = FALSE,
        debug   = FALSE;
    char
        *input_file  = "search.dat",
        *output_file = "dbsearch.xml",
        **argparm    = NULL;            /*  Argument parameter to pick-up    */
    int
        argn;                           /*  Argument number                  */

    for (argn = 1; argn < argc; argn++)
      {
        /*  If argparm is set, we have to collect an argument parameter      */
        if (argparm)
          {
            if (*argv [argn] != '-')    /*  Parameter can't start with '-'   */
              {
                *argparm = argv [argn];
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
                case 'i': argparm = &input_file;   break;
                case 'o': argparm = &output_file;  break;
                case 'd': debug   = TRUE;          break;
                case 't': timer   = TRUE;          break;
                case 'h':
                    printf ("%s%s", PRG_NAME, USAGE);
                    exit (EXIT_SUCCESS);
                default:
                    args_ok = FALSE;
              }
          }
        else
          {
            args_ok = FALSE;
            break;
          }
      }

    /*  Verify parameters are valid                                          */
    if (argparm)
      {
        coprintf ("Argument missing - type '%s -h' for help", EXENAME);
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        coprintf ("Invalid arguments - type '%s -h' for help", EXENAME);
        exit (EXIT_FAILURE);
      }

    if (!file_exists (input_file))
      {
        printf ("Error: file %s not found\n", input_file);
        exit (EXIT_FAILURE);
      }

    t1 = clock ();

    printf ("Parsing search data... ", input_file);
    make_search_file (input_file, TMP_FILE);
    printf ("OK\n");
    
    printf ("Sorting search tokens... ");
    sort_file (TMP_FILE, SORT_FILE);
    if (debug == FALSE)
        file_delete (TMP_FILE);
    printf ("OK\n");

    printf ("Compressing search tokens... ");
    save_xml_search (SORT_FILE, output_file);
    if (debug == FALSE)
        file_delete (SORT_FILE);
    printf ("OK\n");

    if (timer) 
      {
        t2 = clock ();
        printf ("Execution time %g seconds\n", (double)(t2 - t1) / CLOCKS_PER_SEC); 
      }
    mem_assert ();
    return (0);
}
