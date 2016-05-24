/*  ----------------------------------------------------------------<Prolog>-
    Name:       comadmin.cpp
    Title:      Com+ Application Console Administration
    Package:    iAF

    Written:    2003/04/17  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2003/04/23  Pascal Antonnaux <pascal@imatix.com>

    Copyright:  Copyright (c) 1991-2003 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#define _WIN32_DCOM                     /* To use CoInitializeEx()           */
#include "sfl.h"
#include <objbase.h>
#include <comdef.h>
#include "comadmin.h"


#define CUR_VERSION "1.0"

#define PRG_NAME                                                            \
        "Com+ Administration " CUR_VERSION " (c) iMatix Corporation 2003"

#define EXENAME "comadmin"


#define PRG_USAGE                                                           \
    "\nSyntax:  [options...]\n"                                             \
    "Options:\n"                                                            \
    "  -list              List application or component name\n"             \
    "  -start             Start a application\n"                            \
    "  -stop              Stop a application\n"                             \
    "  -delete            Delete a application\n"                           \
    "  -create            Create a application\n"                           \
    "  -install filename  Install a new component\n"                        \
    "  -import  filename  Import a registred component\n"                   \
    "  -reg     filename  Register component into registry\n"               \
    "  -unreg   filename  Unregister component from registry\n"             \
    "  -clean   filename  Clean registry for this file\n"                   \
    "  -backup  file_name Backup Com+ registry entry to a file\n"           \
    "  -restore file_name Restore Com+ registry from a file\n"              \
    "  -a       name      Application name\n"                               \
    "  -user    name      User owner of application\n"                      \
    "  -pwd     value     Password for user owner\n"                        \
    "  -h                 Show summary of command-line options\n"           \
    "\nThe order of arguments is not important. Switches and filenames\n"   \
    "are case sensitive. See documentation for detailed information.\n"

#define BUFFER_SIZE         1024
#define DEFAULT_BACKUP_FILE "~regbackup.tmp"

typedef HRESULT (STDAPICALLTYPE *CTLREGPROC)(void) ; //see COMPOBJ.H

/*- Global Variables --------------------------------------------------------*/
typedef enum admin_command {
    DO_LIST,
    DO_CREATE,
    DO_DELETE,
    DO_START,
    DO_STOP,
    DO_BACKUP,
    DO_RESTORE,
    DO_IMPORT,
    DO_INSTALL,
    DO_REGISTER,
    DO_UNREGISTER,
    DO_CLEAN
} ADMIN_COMMAND;

static char
    error_buffer [BUFFER_SIZE + 1],
    *user_name          = NULL,         /*  User name of application owner   */
    *password           = NULL,         /*  Password for user owner          */
    *file_name          = NULL,         /*  File name (backup, restore)      */
    *application_name   = NULL;         /*  Application name                 */
static Bool
    args_ok             = TRUE;         /*  Were the arguments okay?         */
static IUnknown 
    *unknown_object     = NULL;
static ICOMAdminCatalog 
    *admin_catalog      = NULL;
static ICatalogCollection
    *catalog_collection = NULL;
static ICatalogObject
    *catalog_object     = NULL;
static HRESULT
    hr = S_OK;
ADMIN_COMMAND
    command;

/*- Local functions----------------------------------------------------------*/
static Bool  initialise_resource (void);
static void  free_resource       (void);
static void  get_main_catalog    (void);
static void  list_main_catalog   (void);
static char *get_class_id        (char *full_file_name);
static Bool  backup_registry     (char *backup_file);
static Bool  restore_registry    (char *backup_file);
static Bool  delete_application  (char *application_name);
static Bool  start_application   (char *application_name);
static Bool  stop_application    (char *application_name);
static Bool  install_application (char *name, char *user, char *password);
static Bool  install_component   (char *application, char *file_name);
static Bool  import_component    (char *application, char *file_name);
static Bool  register_dll        (char *file_name, Bool reg);
static void  clean_class_id      (char *full_file_name);

int
main (int argc, char *argv [])
{
    char
        **argparm = NULL;               /*  Argument parameter to pick-up    */
    int
        argn;                           /*  Argument number                  */

    if (argc == 1)
      {
        printf ("%s%s", PRG_NAME, PRG_USAGE);
        free_resource ();
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
                case 'a': argparm = &application_name;   break;
                case 'p': argparm = &password;           break;
                case 'l': command = DO_LIST;             break;
                case 'd': command = DO_DELETE;           break;
                case 'b':
                    command = DO_BACKUP;
                    argparm = &file_name;
                    break;
                case 'r':
                    if (lexcmp (&argv [argn][1], "reg") == 0)
                      {
                        command = DO_REGISTER;
                        argparm = &file_name;
                      }
                    else
                    if (lexcmp (&argv [argn][1], "restore") == 0)
                      {
                        command = DO_RESTORE;
                        argparm = &file_name;
                      }
                    else
                        args_ok = FALSE;
                    break;
                case 'u':
                    if (lexcmp (&argv [argn][1], "unreg") == 0)
                      {
                        command = DO_UNREGISTER;
                        argparm = &file_name;
                      }
                    else
                    if (lexcmp (&argv [argn][1], "user") == 0)
                        argparm = &user_name;
                    else
                        args_ok = FALSE;
                    break;
                case 's':
                    if (lexcmp (&argv [argn][1], "start") == 0)
                        command = DO_START;
                    else
                    if (lexcmp (&argv [argn][1], "stop") == 0)
                        command = DO_STOP;
                    else
                        args_ok = FALSE;
                    break;
                case 'i':
                    if (lexcmp (&argv [argn][1], "install") == 0)
                      {
                        command = DO_INSTALL;
                        argparm = &file_name;
                      }
                    else
                    if (lexcmp (&argv [argn][1], "import") == 0)
                      {
                        command = DO_IMPORT;
                        argparm = &file_name;
                      }
                    else
                        args_ok = FALSE;
                    break;
                case 'c':
                    if (lexcmp (&argv [argn][1], "clean") == 0)
                      {
                        command = DO_CLEAN;
                        argparm = &file_name;
                      }
                    else
                    if (lexcmp (&argv [argn][1], "create") == 0)
                        command = DO_CREATE;
                    else
                        args_ok = FALSE;
                    break;
                case 'h':
                    printf ("%s%s", PRG_NAME, PRG_USAGE);
                    free_resource ();
                    exit (EXIT_SUCCESS);
                /*  Anything else is an error                                */
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
        printf ("Argument missing - type '%s -h' for help\n", EXENAME);
        mem_free (argparm);
        free_resource ();
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        printf ("Invalid arguments - type '%s -h' for help\n", EXENAME);
        free_resource ();
        exit (EXIT_FAILURE);
      }

    if ((command == DO_CREATE
    ||   command == DO_DELETE 
    ||   command == DO_START
    ||   command == DO_INSTALL
    ||   command == DO_IMPORT
    ||   command == DO_STOP)
    &&   application_name == NULL)
      {
        printf ("Missing application name - type '%s -h' for help\n", EXENAME);
        free_resource ();
        exit (EXIT_FAILURE);
      }

    *error_buffer = 0;

    if (command != DO_REGISTER 
    &&  command != DO_UNREGISTER
    &&  command != DO_CLEAN)
      {
        if (initialise_resource () == FALSE)
          {
             printf ("Error: impossible to find comadmin component\n%s\n",
                     error_buffer);
             free_resource ();
             exit (EXIT_FAILURE);
          }
      }

    switch (command)
      {
        case DO_LIST:
            list_main_catalog ();
            break;
        case DO_START:
            printf ("%s start %s...\n", EXENAME, application_name);
            if (start_application (application_name))
                printf ("   Started\n");
            else
                printf ("   Fail: %s\n", error_buffer);
            break;
        case DO_STOP:
            printf ("%s stop %s...\n", EXENAME, application_name);
            if (stop_application (application_name))
                printf ("  application are shutdown\n");
            else
                printf ("   Fail: %s\n", error_buffer);
            break;
        case DO_CREATE:
            printf ("%s create application %s...\n", EXENAME, application_name);
            delete_application (application_name);
            if (install_application (application_name, user_name, password))
                printf ("  installed\n");
            else
                printf ("   Fail: %s\n", error_buffer);
            break;
        case DO_DELETE:
            printf ("%s delete %s...\n", EXENAME, application_name);
            if (delete_application (application_name))
                printf ("  deleted\n");
            else
                printf ("   Fail: %s\n", error_buffer);
            break;
        case DO_BACKUP:
                printf ("%s  Backup com+ registry to %s...\n",
                        EXENAME, file_name);
            if (backup_registry (file_name))
                printf ("  OK\n");
            else
                printf ("   Fail: %s\n", error_buffer);
            break;
        case DO_RESTORE:
                printf ("%s  Restore com+ registry from %s...\n",
                        EXENAME, file_name);
            if (restore_registry (file_name))
                printf ("  OK\n");
            else
                printf ("   Fail: %s\n", error_buffer);
            break;
        case DO_INSTALL:
            printf ("%s  Install component %s...\t\t\t", EXENAME, file_name);
            if (install_component (application_name, file_name))
                printf ("OK\n");
            else
                printf ("\n  Error: %s\n", error_buffer);
            break;
            break;
        case DO_IMPORT:
            printf ("%s  Import component %s...\t\t\t", EXENAME, file_name);
            if (import_component (application_name, file_name))
                printf ("OK\n");
            else
                printf ("\n  Error: %s\n", error_buffer);
            break;
        case DO_REGISTER:
            printf ("%s  Register component %s...\t\t\t", EXENAME, file_name);
            if (register_dll (file_name, TRUE))
                printf ("OK\n");
            else
                printf ("\n  Error: %s\n", error_buffer);
            break;
        case DO_UNREGISTER:
            printf ("%s  Unregister component %s...\t\t\t", EXENAME, file_name);
            if (register_dll (file_name, FALSE))
                printf ("OK\n");
            else
                printf ("\n  Error: %s\n", error_buffer);
            break;
        case DO_CLEAN:
            printf ("%s  clean registry for component %s...", EXENAME, file_name);
            clean_class_id (file_name);
            break;
      }
    free_resource ();

    mem_assert ();
    return (0);
}

/*  -------------------------------------------------------------------------
    Function: initialise_resource

    Synopsis: Initialise all resources.
    -------------------------------------------------------------------------*/

static Bool
initialise_resource (void)
{

    hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);
    if (FAILED (hr))
      {
        sprintf(error_buffer, "CoInitializeEx failed: Error # %#x", hr);
        return (FALSE);
      }

    hr = CoCreateInstance  (CLSID_COMAdminCatalog,
                            NULL, 
                            CLSCTX_INPROC_SERVER,
                            IID_IUnknown,
                            (void**)&unknown_object);
    if (FAILED (hr))
      {
        unknown_object = NULL;
        sprintf(error_buffer, "CoCreateInstance failed: Error # %#x", hr);
        return(FALSE);
      }

    hr = unknown_object-> QueryInterface (IID_ICOMAdminCatalog, 
                                          (void**)&admin_catalog);
    if (FAILED (hr))
      {
        unknown_object-> Release();  
        unknown_object = NULL;
        sprintf(error_buffer, "ICOMAdminCatalog not supported: Error # %#x",
                hr);
        return (FALSE);
      }

    return (TRUE);
}


/*  -------------------------------------------------------------------------
    Function: free_resource

    Synopsis: Free all resources.
    -------------------------------------------------------------------------*/

static void
free_resource (void)
{
    if (application_name)
        mem_strfree (&application_name);

    if (file_name)
        mem_strfree (&file_name);

    if (user_name)
        mem_strfree (&user_name);

    if (password)
        mem_strfree (&password);

    if (unknown_object)
      {
        unknown_object-> Release ();  
        unknown_object = NULL;
      }
    if (admin_catalog)
      {
        admin_catalog-> Release ();
        admin_catalog = NULL;
      }
    if (catalog_collection)
      {
        catalog_collection-> Release ();
        catalog_collection = NULL;
      }

    CoUninitialize ();
}


/*  -------------------------------------------------------------------------
    Function: get_main_catalog 

    Synopsis: Get the main applications collection
    -------------------------------------------------------------------------*/

static void 
get_main_catalog (void)
{
    hr = admin_catalog-> GetCollection (L"Applications",
                                        (IDispatch **)&catalog_collection);
    if (FAILED (hr))
        return;

    catalog_collection-> Populate ();
}


/*  -------------------------------------------------------------------------
    Function: list_main_catalog 

    Synopsis: Display the main applications collection
    -------------------------------------------------------------------------*/

static void 
list_main_catalog (void)
{
    long
        index,
        count;
    VARIANT
        name;

    get_main_catalog ();
    if (catalog_collection)
      {
        catalog_collection-> get_Count (&count);

        if (count > 0)
          {
            printf ("<com>\n");
            memset (&name, 0, sizeof (VARIANT));
            for (index = 0; index < count; index++)
              {
                hr = catalog_collection-> get_Item (index, (IDispatch **)&catalog_object);
                if (FAILED (hr))
                    break;

                catalog_object-> get_Name (&name);
                if (name.vt == VT_BSTR)
                    wprintf (L"    <application name = \"%s\" />\n", name.bstrVal);
                catalog_object-> Release ();
                catalog_object = NULL;
              }
            printf ("</com>\n");
          }
      }
}


/*  -------------------------------------------------------------------------
    Function: list_main_catalog 

    Synopsis: Display the main applications collection
    -------------------------------------------------------------------------*/

static Bool  
delete_application  (char *application_name)
{
    long
        nb_change,
        index,
        count;
    Bool
        feedback = FALSE;
    UCODE
        *application = NULL;
    VARIANT
        name;

    get_main_catalog ();

    if (application_name == NULL
    ||  *application_name == 0)
        return (feedback);

    application = ascii2ucode (application_name);
    if (catalog_collection
    &&  application)
      {
        catalog_collection-> get_Count (&count);
        if (count > 0)
          {
            memset (&name, 0, sizeof (VARIANT));
            for (index = 0; index < count; index++)
              {
                hr = catalog_collection-> get_Item (index, (IDispatch **)&catalog_object);
                if (FAILED (hr))
                    break;

                catalog_object-> get_Name (&name);
                catalog_object-> Release ();
                catalog_object = NULL;

                if (name.vt == VT_BSTR
                &&  wcscmp (application, name.bstrVal) == 0)
                  {
                    hr = catalog_collection->Remove (index);
                    if (hr == S_OK)
                      {
                        hr = catalog_collection-> SaveChanges (&nb_change);
                        if (hr == S_OK)
                            feedback = TRUE;
                        else
                            sprintf (error_buffer, "Can't save change");
                      }
                    else
                        sprintf (error_buffer, "Can't remove application");
                    break;
                  }
              }
            if (index == count && feedback == FALSE)
                sprintf (error_buffer, "Can't find application");

          }
      }

    if (application)
        mem_free (application);

    return (feedback);
}

/*  -------------------------------------------------------------------------
    Function: backup_registry 

    Synopsis: Backup registry entry for COM+ application
    -------------------------------------------------------------------------*/

static Bool  
backup_registry (char *backup_file)
{
    Bool
        feedback = FALSE;
    UCODE
        *file_name;

    if (backup_file 
    && *backup_file
    &&  admin_catalog)
      {
        file_name = ascii2ucode (backup_file);
        if (file_name)
          {
            hr = admin_catalog-> BackupREGDB (file_name);
            if (hr == S_OK)
                feedback = TRUE;
            else
                sprintf (error_buffer, "BackupREGDB error %x", hr);
            mem_free (file_name);
          }       
      }

    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: restore_registry 

    Synopsis: restore registry entry for COM+ application
    -------------------------------------------------------------------------*/

static Bool  
restore_registry (char *backup_file)
{
    Bool
        feedback = FALSE;
    UCODE
        *file_name;

    if (backup_file 
    && *backup_file
    &&  admin_catalog)
      {
        file_name = ascii2ucode (backup_file);
        if (file_name)
          {
            hr = admin_catalog-> RestoreREGDB (file_name);
            if (hr == S_OK)
                feedback = TRUE;
            else
                sprintf (error_buffer, "RestoreREGDB error %x", hr);
            mem_free (file_name);
          }       
      }

    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: start_application 

    Synopsis: Start a com+ application
    -------------------------------------------------------------------------*/

static Bool
start_application (char *application_name)
{
    Bool
        feedback = FALSE;
    UCODE
        *application = NULL;
    if (application_name)
      {
        application = ascii2ucode (application_name);
        if (application)
          {
            if (admin_catalog)
              {
                hr = admin_catalog-> StartApplication (application);
                if (hr == S_OK)
                    feedback = TRUE;
                else
                    sprintf (error_buffer, "StartApplication return %x", hr);
              }
            mem_free (application);
          }
      }
    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: stop_application 

    Synopsis: Stop a com+ application
    -------------------------------------------------------------------------*/

static Bool
stop_application (char *application_name)
{
    Bool
        feedback = FALSE;
    UCODE
        *application = NULL;
    if (application_name)
      {
        application = ascii2ucode (application_name);
        if (application)
          {
            if (admin_catalog)
              {
                hr = admin_catalog-> ShutdownApplication (application);
                if (hr == S_OK)
                    feedback = TRUE;
                else
                    sprintf (error_buffer, "ShutdownApplication return %x", hr);
              }
            mem_free (application);
          }
      }
    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: install_application 

    Synopsis: Install a new com+ application
    -------------------------------------------------------------------------*/

static Bool  
install_application (char *name, char *user, char *password)
{
    Bool
        feedback = FALSE;
    UCODE
         *application = NULL,
         *user_name   = NULL,
         *pwd         = NULL;
    long
        nb_change;
    _variant_t
        value;

    if (name == NULL || *name == '\0')
        return (feedback);
 
    application = ascii2ucode (name);

    if (user && *user)
        user_name = ascii2ucode (user);
    if (password && *password)
        pwd = ascii2ucode (password);

    if (admin_catalog)
      {
         get_main_catalog ();
         if (catalog_collection)
           {
             hr = catalog_collection-> Add ((IDispatch **)&catalog_object);
             if (hr == S_OK)
               {
                 value = application;
                 catalog_object-> put_Value (L"Name", value);

                 if (user_name)
                   {
                     value = user_name;
                     catalog_object-> put_Value (L"Identity", value);
                   }
                 if (pwd)
                   {
                     value = pwd;
                     catalog_object-> put_Value (L"Password", value);
                   }
                 catalog_object-> Release ();

                 hr = catalog_collection-> SaveChanges (&nb_change);
                 if (hr == S_OK)
                     feedback = TRUE;
                 else
                     sprintf (error_buffer, "Can't save change, error %x", hr);
               }
             else
                 sprintf (error_buffer, "Can't add a new application");
           }
         else
             sprintf (error_buffer, "Missing catalog collection");

      }

    if (application)
        mem_free (application);
    if (user_name)
        mem_free (user_name);
    if (pwd)
        mem_free (pwd);

    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: install_component

    Synopsis: Install a new component to a application
    -------------------------------------------------------------------------*/

static Bool
install_component (char *application_name, char *file_name)
{
    Bool
        feedback = FALSE;
    UCODE
        *u_clsid,
        *application = NULL;
    char
        *file_full_path,
        *clsid;


    if (application_name == NULL
    ||  *application_name == 0)
        return (feedback);

    file_full_path = file_where ('r', ".", file_name, NULL);
    if (file_full_path != NULL
    &&  file_exists (file_full_path))
      {
        application = ascii2ucode (application_name);
        clsid = get_class_id (file_name);
        if (clsid && *clsid)
          {
            u_clsid = ascii2ucode (clsid);
            if (u_clsid)
              {
                hr = admin_catalog-> InstallComponent (application, u_clsid, NULL, NULL);
                if (hr == S_OK)
                    feedback = TRUE;
                else
                    sprintf (error_buffer, "Error when install component: %x", hr);
                            
                mem_free (u_clsid);
              }
          }
        else
            sprintf (error_buffer, "Can't find registration for this file");
                  
      }
    else
        sprintf (error_buffer, "Can't locate file or don't exist");

    if (application)
        mem_free (application);

    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: import_component  

    Synopsis: Import a already registred component into application.
    -------------------------------------------------------------------------*/

static Bool
import_component (char *application_name, char *file_name)
{
    Bool
        feedback = FALSE;
    UCODE
        *u_clsid,
        *application = NULL;
    char
        *file_full_path,
        *clsid;


    if (application_name == NULL
    ||  *application_name == 0)
        return (feedback);

    file_full_path = file_where ('r', ".", file_name, NULL);
    if (file_full_path != NULL
    &&  file_exists (file_full_path))
      {
        application = ascii2ucode (application_name);
        clsid = get_class_id (file_full_path);
        if (clsid && *clsid)
          {
            u_clsid = ascii2ucode (clsid);
            if (u_clsid)
              {
                hr = admin_catalog-> ImportComponent (application, u_clsid);
                if (hr == S_OK)
                    feedback = TRUE;
                else
                    sprintf (error_buffer, "Error when import component: %x", hr);
                            
                mem_free (u_clsid);
              }
          }
        else
            sprintf (error_buffer, "Can't find registration for this file");
                  
      }
    else
        sprintf (error_buffer, "Can't locate file or don't exist");

    if (application)
        mem_free (application);

    return (feedback);
}

/*  -------------------------------------------------------------------------
    Function: get_class_id 

    Synopsis: Get the class ID in registry for a dll file. (require full path).
    -------------------------------------------------------------------------*/

static char *
get_class_id (char *full_file_name)
{
    static char
        clsid_name [BUFFER_SIZE + 1],
        buffer     [BUFFER_SIZE + 1],
        feedback   [BUFFER_SIZE + 1];
    HKEY
        key_clsid,
        key_x;
    long
        size,
        result;
    DWORD
        index;
    *feedback = '\0';
        
/*
    HKEY_CLASSES_ROOT
	    CLSID
		    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}	
			    Implemented Categories
			    InprocServer32
			    ProgID
                Programmable
                TypeLib
                VERSION

    We search in all clsid entry in registry for a InprocServer32 with
    file name value.
*/
    result = ::RegOpenKey (HKEY_CLASSES_ROOT, "CLSID", &key_clsid);
    if (result != ERROR_SUCCESS)
        return (feedback);
    
    index = 0;
    /* Enum all entries under CLSID                                          */
    while (::RegEnumKey (key_clsid, index++, clsid_name, BUFFER_SIZE + 1)
            == ERROR_SUCCESS)
      {
        /* Open the CLSID key                                                */
        result = ::RegOpenKey (key_clsid, clsid_name, &key_x) ;
        if (result != ERROR_SUCCESS)
            continue;

        /* Get full path of component in inprocserver value                  */
        size = BUFFER_SIZE + 1;
        result = ::RegQueryValue (key_x, "InprocServer32", buffer, &size) ;
        ::RegCloseKey (key_x) ;
        if (result == ERROR_SUCCESS)
          {
            if (buffer 
            && *buffer
            &&  lexcmp (buffer, full_file_name) == 0)
              {
                strcpy (feedback, clsid_name);
                break;
              }
          }
      }
    ::RegCloseKey (key_clsid); 

    return (feedback);
}


/*  -------------------------------------------------------------------------
    Function: delete_reg_key 

    Synopsis: Recursive function to delete registry key and child.
    -------------------------------------------------------------------------*/

void
delete_reg_key (HKEY key, char *sub_key)
{
  int
      index;   
  long
      result;
  char
      sub_key_name [BUFFER_SIZE + 1];
  HKEY
      local_key;

  /* Will remove sub key first                                               */
  result = ::RegOpenKeyEx (key, sub_key, 0, KEY_ALL_ACCESS, &local_key);
  if (result != ERROR_SUCCESS)
      return;

    index = 0;
    /* Enum all entries under CLSID                                          */
    while (::RegEnumKey (local_key, index++, sub_key_name, BUFFER_SIZE + 1)
            == ERROR_SUCCESS)
      {
        result = ::RegDeleteKey (local_key, sub_key_name);
        if (result != 0)
            delete_reg_key (local_key, sub_key_name);
      }
  ::RegCloseKey (local_key); 
  result = ::RegDeleteKey (key, sub_key);
}


/*  -------------------------------------------------------------------------
    Function: clean_class_id 

    Synopsis: Search reference of component file in registry and delete key
    when found.
    -------------------------------------------------------------------------*/

static void
clean_class_id (char *file_name)
{
    static char
        *full_file_name,
        clsid_name   [BUFFER_SIZE + 1],
        version_name [BUFFER_SIZE + 1],
        buffer       [BUFFER_SIZE + 1];
    HKEY
        key_clsid,
        key_y,
        key_z,
        key_x;
    long
        size,
        result;
    DWORD
        sub_index,
        index;

    /* Search in CLASSES_ROOT\CLSID                                          */
    result = ::RegOpenKeyEx (HKEY_CLASSES_ROOT, "CLSID", NULL, KEY_ALL_ACCESS,
                             &key_clsid);
    if (result != ERROR_SUCCESS)
        return;
    
    full_file_name = file_where ('r', ".", file_name, NULL);
    index = 0;
    /* Enum all entries under CLSID                                          */
    while (::RegEnumKey (key_clsid, index++, clsid_name, BUFFER_SIZE + 1)
            == ERROR_SUCCESS)
      {
        /* Open the CLSID key                                                */
        result = ::RegOpenKey (key_clsid, clsid_name, &key_x) ;
        if (result != ERROR_SUCCESS)
            continue;

        /* Get full path of component in inprocserver value                  */
        size = BUFFER_SIZE + 1;
        result = ::RegQueryValue (key_x, "InprocServer32", buffer, &size) ;
        ::RegCloseKey (key_x) ;
        if (result == ERROR_SUCCESS)
          {
            if (buffer 
            && *buffer
            &&  lexcmp (buffer, full_file_name) == 0)
                delete_reg_key (key_clsid, clsid_name);
          }
      }
    ::RegCloseKey (key_clsid); 


    /* Search in CLASSES_ROOT\TypeLib                                        */
    result = ::RegOpenKeyEx (HKEY_CLASSES_ROOT, "TypeLib", NULL,
                             KEY_ALL_ACCESS,  &key_clsid);
    if (result != ERROR_SUCCESS)
        return;

    index = 0;
    while (::RegEnumKey (key_clsid, index++, clsid_name, BUFFER_SIZE + 1)
            == ERROR_SUCCESS)
      {
        /* Open the CLSID key                                                */
        result = ::RegOpenKey (key_clsid, clsid_name, &key_x) ;
        if (result != ERROR_SUCCESS)
            continue;

        sub_index = 0;
        while (::RegEnumKey (key_x, sub_index++, version_name, BUFFER_SIZE + 1)
            == ERROR_SUCCESS)
          {
            result = ::RegOpenKey (key_x, version_name, &key_y) ;
            if (result != ERROR_SUCCESS)
                continue;

            result = ::RegOpenKey (key_y, "0", &key_z) ;
            if (result != ERROR_SUCCESS)
              {
                ::RegCloseKey (key_y);
                continue;
              }
            /* Get full path of component in inprocserver value                  */
            size = BUFFER_SIZE + 1;
            result = ::RegQueryValue (key_z, "win32", buffer, &size) ;
            ::RegCloseKey (key_y);
            ::RegCloseKey (key_z);
            if (result == ERROR_SUCCESS)
              {
                if (buffer 
                && *buffer
                &&  lexcmp (buffer, full_file_name) == 0)
                    delete_reg_key (key_clsid, clsid_name);
              }
         }
       ::RegCloseKey (key_x);
      }
    ::RegCloseKey (key_clsid); 
}
/*  -------------------------------------------------------------------------
    Function: register_dll 

    Synopsis: Register or unregister DLL
    -------------------------------------------------------------------------*/

static Bool
register_dll (char *file_name, Bool reg)
{
    char
        *file_full_path;
    HINSTANCE
        module;
    CTLREGPROC
        reg_function;
    Bool
        feedback = FALSE;

    if (file_name)
      {
        file_full_path = file_where ('r', ".", file_name, NULL);
        if (file_full_path != NULL
        &&  file_exists (file_full_path))
          {
            module = ::LoadLibrary (file_full_path);
            if (module)
              {
                reg_function = (CTLREGPROC)::GetProcAddress (module,
                                             reg? "DllRegisterServer":
                                                  "DllUnregisterServer") ;
                if (reg_function != NULL)
                  {
			        hr = reg_function ();
			        feedback = (hr == NOERROR); 
                    if (feedback == FALSE)
                        sprintf ("%sregister function fail: %x", 
                                  reg? "": "un", hr);
                  }		
                else
                    sprintf (error_buffer, "Can't find self %sregister function",
                                           reg? "": "un");
                ::FreeLibrary(module);
              }
            else
                sprintf (error_buffer, "Can't load library");
          }
        else
            sprintf (error_buffer, "Can't locate file or don't exist");
      }
    return (feedback);
}
