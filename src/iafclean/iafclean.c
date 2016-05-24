/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafclean.c
    Title:      IAF Registry Cleaner 1.0 main module
    Package:    IAF

    Written:    2001/08/07  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2001/08/07  Pascal Antonnaux <pascal@imatix.com>

    Copyright:  Copyright (c) 1991-2001 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/
#include "sfl.h"

#define CUR_VERSION "1.0"

#define PRG_NAME                                                             \
        "IAF Registry Cleaner " CUR_VERSION " (c) iMatix Corporation 2001"

#define EXENAME "iAFCLEAN"

#define USAGE                                                                \
    "\niAfclean search and delete component declaration in registry\n"       \
    "matching pattern\n"                                                     \
    "\nSyntax: iafclean [options...] pattern\n"                              \
    "pattern: component pattern\n"                                           \
    "        you can separate different pattern by ';'\n"                    \
    "        (ex: us12;scl_)\n"                                              \
    "Options:\n"                                                             \
    "  -l     Log deleted entry to reg file (for undo)\n"                    \
    "  -h     Show summary of command-line options\n"                        \
    "\nThe order of arguments is not important. Switches and filenames\n"    \
    "are case sensitive. See documentation for detailed information.\n"

#define BUFFER_SIZE 1024

/*- Global Variables --------------------------------------------------------*/
static Bool
    log_deleted = FALSE;                /*  TRUE if deleted entry is logged  */
static char
    *pattern  = NULL,                   /*  Pattern string                   */
    **ptable  = NULL,                   /*  Pattern table                    */
    buffer     [BUFFER_SIZE + 1],       /*  Working buffer                   */
    value      [BUFFER_SIZE + 1],       /*  Buffer with value                */
    log_name [250];                     /*  Log file name                    */
static int
    *pattern_size = NULL;              /*  Table with pattern size          */

/*  -------------------------------------------------------------------------
    Function: free_all_resources

    Synopsis: Free all allocated resources.
    -------------------------------------------------------------------------*/

void
free_all_resources (void)
{
    if (pattern)
      {
        mem_free (pattern);
        pattern = NULL;
      }
    if (ptable)
      {
        tok_free (ptable);
        ptable = NULL;
      }
    if (pattern_size)
      {
        mem_free (pattern_size);
        pattern_size = NULL;
      }
}


char *
get_class_name (HKEY root_key, char *classid)
{
    static char
          value [256];
    HKEY
        key;                            /*  registry key                     */
    long
        rc;
    DWORD
        value_size;

    rc = RegOpenKeyEx (root_key, classid, 0, KEY_ALL_ACCESS, &key);
    if (rc == ERROR_SUCCESS)
      {
        value_size   = 255;
        rc = RegQueryValueEx (key, NULL, NULL, NULL, value, &value_size);
        if (rc != ERROR_SUCCESS || value_size <= 0)
            *value = 0;
        RegCloseKey (key);
      }
    return (value);
}

int
main (int argc, char *argv [])
{
    char
        *class_name,
        *file_name,                      /*  DLL file name                    */
        **argparm = NULL;               /*  Argument parameter to pick-up    */
    int
        index,                          /*  Working counter                  */
        nb_pattern,                     /*  Number of pattern                */
        argn;                           /*  Argument number                  */
    Bool
        args_ok   = TRUE;               /*  Were the arguments okay?         */
    DWORD
        key_index,                      /*  Index of registry key            */
        buf_size   = BUFFER_SIZE,       /*  Buffer size                      */
        value_size = BUFFER_SIZE;       /*  Value_size                       */
    FILETIME
        last_modif;                     /*  Time of last modification        */
    long
        rc2,
        rc;                             /*  Return code                      */
    HKEY
        key,                            /*  registry key                     */
        root_key;                       /*  root registry key                */

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
                case 'l': log_deleted = TRUE; break;
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
                pattern = mem_strdup (argv [argn]);
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
    else
    if (pattern == NULL)
      {
        coprintf ("pattern missing - type '%s -h' for help", EXENAME);
        mem_free (argparm);
        free_all_resources ();
        exit (EXIT_FAILURE);
      }

    ptable     = tok_split_ex (pattern, ';');
    nb_pattern = tok_size (ptable);
    pattern_size = (int *)mem_alloc (sizeof (int) * nb_pattern);
    for (index = 0; index < nb_pattern; index++)
        pattern_size [index] = strlen (ptable [index]);

    rc = RegOpenKeyEx (HKEY_CLASSES_ROOT, "CLSID", 0, KEY_ALL_ACCESS, &root_key);
    if (rc == ERROR_SUCCESS)
      {
        printf ("Search in classes ID...\n");
        key_index = 0;
        while (rc == ERROR_SUCCESS)
          {
           buf_size   = BUFFER_SIZE;
           rc = RegEnumKeyEx (root_key, key_index, buffer, &buf_size, NULL,
                               NULL, NULL, &last_modif);
            if (rc == ERROR_SUCCESS)
              {
                class_name = get_class_name (root_key, buffer);
                xstrcpy (value, buffer, "\\InProcServer32", NULL);
                rc2 = RegOpenKeyEx (root_key, value, 0, KEY_ALL_ACCESS, &key);
                if (rc2 == ERROR_SUCCESS)
                  {
                    value_size   = BUFFER_SIZE;
                    rc2 = RegQueryValueEx (key, NULL, NULL, NULL, value, &value_size);
                    if (rc2 == ERROR_SUCCESS && value_size > 0)
                      {
                        file_name = strip_file_path (value);
                        for (index = 0; index < nb_pattern; index++)
                          {
                            if (lexncmp (file_name, ptable [index], pattern_size [index]) == 0)
                              {
                                printf ("%s (%s) %s\n", buffer, value, class_name);
                                break;
                              }
                          }
                      }
                    RegCloseKey (key);
                  }
              }
            key_index++;
          }

        RegCloseKey (root_key);
      }

    free_all_resources ();
    mem_assert ();
    return (0);
}