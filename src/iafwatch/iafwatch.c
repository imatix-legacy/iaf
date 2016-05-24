/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafwatch.c
    Title:      Iaf STEP Daemon for Windows NT/95/98 service model


    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include <windowsx.h>
#include <direct.h>                     /*  Directory create functions       */
#include "sfl.h"                        /*  SFL library header file          */

/*- Instance definitions ----------------------------------------------------*/

#define APPLICATION_NAME    "iafwatch"    /* Name of the executable          */
#define APPLICATION_VERSION "1.0"
#define SERVICE_NAME        "iafwatch"
#define SERVICE_TEXT        "iAF Watch Daemon"


/*- Global definitions ------------------------------------------------------*/

#define DEPENDENCIES      ""
#define REGISTRY_IAFSTEP  "SOFTWARE\\imatix\\iafwatch"
#define REGISTRY_RUN      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices"
#define WINDOWS_95        1             /*  Return from get_windows_version  */
#define WINDOWS_NT_4      2
#define WINDOWS_NT_3X     3
#define WINDOWS_2000      4

#define CONFIG_NAME       "iafwatch.xml"

#define USAGE                                                                 \
    APPLICATION_NAME "server version " APPLICATION_VERSION ": Windows service model\n" \
    "Syntax: " APPLICATION_NAME " [options...]\n"                             \
    "Options:\n"                                                              \
    "  -i               Install Windows service\n"                            \
    "  -u               Uninstall Windows service\n"                          \
    "  -d               Run as DOS console program\n"                         \
    "  -h               Show summary of command-line options.\n"              \
    "\nThe order of arguments is not important. Switches and filenames\n"     \
    "are case sensitive. See documentation for detailed information."


#define BUFFER_SIZE            4096

typedef struct _process_item {
    struct _process_item
        *next, *prev;
    char *name;
} PROCESS_ITEM;

/*- Global variables --------------------------------------------------------*/

static SERVICE_STATUS
    service_status;                     /*  current status of the service    */
static SERVICE_STATUS_HANDLE
    service_status_handle;              /*  Service status handle            */
static HANDLE
    server_stop_event = NULL;           /*  Handle for server stop event     */
static DWORD
    error_code = 0;                     /*  Last error code                  */
static BOOL
    run_mode      = TRUE,               /*  TRUE if check request in database*/
    debug_mode    = FALSE,              /*  TRUE if debug mode               */
    console_mode  = FALSE,              /*  TRUE if console mode             */
    control_break = FALSE;              /*  TRUE if control break            */
static char
    service_name  [255],                /*  Service name                     */
    service_text  [255],                /*  Service description              */
    trace_file    [255],                /*  Trace file name                  */
    http_url      [1024],
    http_agent    [1024],
    http_cookie   [1024],
    http_timeout  [128],
    *rootdir,                           /*  Default root directory           */
    error_buffer [LINE_MAX + 1];        /*  Buffer for error string          */
static int
    win_version,                        /*  Windows version                  */
    schedule_interval;                  /*  Interval to check new request    */
XML_ITEM
   *config = NULL;                      /*  Config file                      */
LIST
   process_list;
/*- Function prototypes -----------------------------------------------------*/

void WINAPI       service_main            (int argc, char **argv);
void              service_start           (int argc, char **argv);
void              service_stop            (void);
void WINAPI       service_control         (DWORD control_code);
void              install_service         (void);
void              remove_service          (void);
void              console_service         (int argc, char **argv);
BOOL WINAPI       control_handler         (DWORD control_type);
static char *     get_last_error_text     (char *buffer, int size);
static int        get_windows_version     (void);
static long       set_win95_service       (BOOL add);
static BOOL       report_status           (DWORD state, DWORD exit_code,
                                           DWORD wait_hint);
static void       add_to_message_log      (char *message);
static void       load_config_file        (char *file_name);
static void       free_resources          (void);
static void       hide_window             (void);
static int        log_printf              (const char *format, ...);
static void       free_process_list       (void);
static void       watch_url               (void);
static void       execute_process_list    (void);

/*  ---------------------------------------------------------------------[<]-
    Function: main

    Synopsis: Main service function
    ---------------------------------------------------------------------[>]-*/

int
main (int argc, char **argv)
{
    static char
        buffer [LINE_MAX];
    int
        argn;                           /*  Argument number                  */
    char
        *p_char;

    SERVICE_TABLE_ENTRY 
        dispatch_table [] = {
        { NULL, (LPSERVICE_MAIN_FUNCTION) service_main },
        { NULL, NULL }
    };

    /*  Change to the correct working directory                              */
    GetModuleFileName (NULL, buffer, LINE_MAX);
    if ((p_char = strrchr (buffer, '\\')) != NULL)
        *p_char = '\0';
    SetCurrentDirectory (buffer);

    list_reset (&process_list);

    /*  Load configuration data, if any, into the config table               */
    if (file_exists (CONFIG_NAME))
        load_config_file (CONFIG_NAME);
    else
      {
        printf ("Error: config file %s is missing\n", CONFIG_NAME);
        exit (EXIT_FAILURE);
      }

    dispatch_table [0].lpServiceName = service_name;
    win_version = get_windows_version ();

    for (argn = 1; argn < argc; argn++)
      {
        if (*argv [argn] == '-')
          {
            switch (argv [argn][1])
              {
                case 'h':
                    puts (USAGE);
                    return (0);
                /*  These switches take a parameter                          */
                case 'i':
                    if (win_version == WINDOWS_95)
                        set_win95_service (TRUE);
                    else
                        install_service ();
                    return (0);
                case 'u':
                    if (win_version == WINDOWS_95)
                        set_win95_service (FALSE);
                    else
                        remove_service ();
                    return (0);
                case 'd':
                    console_mode  = TRUE;
                    console_service (argc, argv);
                    return (0);
              }
          }
      }
    if (debug_mode)
        log_printf ("Start service... (%s version %s)",
                  APPLICATION_NAME,
                  APPLICATION_VERSION);

    if (win_version == WINDOWS_95)
      {
        hide_window ();
        console_mode = TRUE;
        console_service (argc, argv);
      }
    else
    if (win_version == WINDOWS_NT_3X
    ||  win_version == WINDOWS_NT_4
    ||  win_version == WINDOWS_2000)
      {
        printf ("\n%s: initialising service dispatcher...", APPLICATION_NAME);
        if (!StartServiceCtrlDispatcher (dispatch_table))
            add_to_message_log ("StartServiceCtrlDispatcher failed");
      }
        return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: service_main

    Synopsis: This routine performs the service initialization and then calls
              the user defined service_start() routine to perform majority
              of the work.
    ---------------------------------------------------------------------[>]-*/

void WINAPI
service_main (int argc, char **argv)
{
    /* Register our service control handler:                                 */
    service_status_handle 
        = RegisterServiceCtrlHandler (service_name, service_control);
    if (!service_status_handle)
        return;

    service_status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    service_status.dwServiceSpecificExitCode = 0;

    /* report the status to the service control manager.                     */
    if (report_status (
            SERVICE_START_PENDING,      /*  Service state                    */
            NO_ERROR,                   /*  Exit code                        */
            3000))                      /*  Wait Hint                        */
        service_start (argc, argv);

    /* Try to report the stopped status to the service control manager.      */
    if (service_status_handle)
        report_status (SERVICE_STOPPED, error_code, 0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: service_start

    Synopsis: Main routine for xitami web server. The service stops when
    server_stop_event is signaled.
    ---------------------------------------------------------------------[>]-*/

void
service_start (int argc, char **argv)
{
    int
        argn;                           /*  Argument number                  */
    Bool
        args_ok     = TRUE,             /*  Were the arguments okay?         */
        quite_mode  = FALSE;            /*  -q means suppress all output     */
    char
        **argparm = NULL;               /*  Argument parameter to pick-up    */
    DWORD
        wait;

    if (debug_mode)
        log_printf ("Check argument value (nb = %d)", argc);

    argparm = NULL;
    for (argn = 1; argn < argc; argn++)
      {
        /*  If argparm is set, we have to collect an argument parameter      */
        if (argparm)
          {
            if (*argv [argn] != '-')    /*  Parameter can't start with '-'   */
              {
                free (*argparm);
                *argparm = strdupl (argv [argn]);
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
                /*  These switches have an immediate effect                  */
                case 'q':
                    quite_mode = TRUE;
                    break;
                /*  Used only for service                                    */
                case 'i':
                case 'd':
                case 'u':
                case 'v':
                case 'h':
                    break;
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

    /*  If there was a missing parameter or an argument error, quit          */
    if (argparm)
      {
        add_to_message_log ("Argument missing - type 'iafwatch -h' for help");
        return;
      }
    else
    if (!args_ok)
      {
        add_to_message_log ("Invalid arguments - type 'iafwatch -h' for help");
        return;
      }
    /* Service initialization                                                */

    /* Report the status to the service control manager.                     */
    if (!report_status (
            SERVICE_START_PENDING,      /* Service state                     */
            NO_ERROR,                   /* Exit code                         */
            3000))                      /* wait hint                         */
        return;

    /* Create the event object. The control handler function signals         */
    /* this event when it receives the "stop" control code.                  */
    server_stop_event = CreateEvent (
                            NULL,       /* no security attributes            */
                            TRUE,       /* manual reset event                */
                            FALSE,      /* not-signalled                     */
                            NULL);      /* no name                           */

    if (server_stop_event == NULL)
        return;

    /* report the status to the service control manager.                     */
    if (!report_status (
            SERVICE_START_PENDING,      /* Service state                     */
            NO_ERROR,                   /* Exit code                         */
            3000))                      /* wait hint                         */
      {
        CloseHandle (server_stop_event);
        return;
      }
    if (quite_mode)
      {
        fclose (stdout);                /*  Kill standard output             */
        fclose (stderr);                /*   and standard error              */
      }
    /*  Report the status to the service control manager.                    */
    if (!report_status (SERVICE_RUNNING, NO_ERROR, 0))                
      {
        CloseHandle (server_stop_event);
        return;
      }


    while (!control_break)
      {
         if (run_mode)
             watch_url ();

         wait = WaitForSingleObject (server_stop_event, schedule_interval);
         if (wait != WAIT_TIMEOUT)
            control_break = TRUE;
         else
         if (xml_changed (config))
             load_config_file (CONFIG_NAME);
      }
    CloseHandle (server_stop_event);

    free_resources ();
}


/*  ---------------------------------------------------------------------[<]-
    Function: service_stop

    Synopsis: Stops the service. If a service_stop procedure is going to
    take longer than 3 seconds to execute, it should spawn a thread to
    execute the stop code, and return.  Otherwise, ServiceControlManager
    will believe that the service has stopped responding.
    ---------------------------------------------------------------------[>]-*/

void
service_stop (void)
{
    if (server_stop_event)
        SetEvent (server_stop_event);
}


/*  ---------------------------------------------------------------------[<]-
    Function: service_control

    Synopsis: This function is called by the service control manager whenever
              ControlService() is called on this service.
    ---------------------------------------------------------------------[>]-*/

void WINAPI
service_control (DWORD control_code)
{
    /* Handle the requested control code.                                    */
    switch (control_code)
      {
        case SERVICE_CONTROL_STOP:      /*  Stop the service                 */
            service_status.dwCurrentState = SERVICE_STOP_PENDING;
            service_stop ();
            break;

        case SERVICE_CONTROL_INTERROGATE:/* Update the service status        */
            break;
        default:                        /*  Invalid control code             */
            break;
      }
    report_status (service_status.dwCurrentState, NO_ERROR, 0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: report_status

    Synopsis: Sets the current status of the service and
              reports it to the Service Control Manager.
    ---------------------------------------------------------------------[>]-*/

static BOOL
report_status (DWORD state, DWORD exit_code, DWORD wait_hint)
{
    static DWORD
        check_point = 1;
    BOOL
       result       = TRUE;

    /*  when debugging we don't report to the SCM                            */
    if (!console_mode)
      {
        if (state == SERVICE_START_PENDING)
            service_status.dwControlsAccepted = 0;
        else
            service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        service_status.dwCurrentState  = state;
        service_status.dwWin32ExitCode = exit_code;
        service_status.dwWaitHint      = wait_hint;

        if (state == SERVICE_RUNNING
        ||  state == SERVICE_STOPPED)
            service_status.dwCheckPoint = 0;
        else
            service_status.dwCheckPoint = check_point++;


        /* Report the status of the service to the service control manager.  */
        result = SetServiceStatus (service_status_handle, &service_status);
        if (!result)
            add_to_message_log ("SetServiceStatus");
      }
    return result;
}


/*  ---------------------------------------------------------------------[<]-
    Function: add_to_message_log

    Synopsis: Allows any thread to log an error message.
    ---------------------------------------------------------------------[>]-*/

static void
add_to_message_log (char *message)
{
    static char
        *strings       [2],
        message_buffer [LINE_MAX + 1];
    HANDLE
        event_source_handle;

    if (!console_mode)
      {
        error_code = GetLastError();

        /* Use event logging to log the error.                               */
        event_source_handle = RegisterEventSource (NULL, service_name);

        sprintf (message_buffer, "%s error: %d", service_name, error_code);
        strings [0] = message_buffer;
        strings [1] = message;

        if (event_source_handle)
          {
            ReportEvent (event_source_handle,/* handle of event source       */
                EVENTLOG_ERROR_TYPE,         /* event type                   */
                0,                           /* event category               */
                0,                           /* event ID                     */
                NULL,                        /* current user's SID           */
                2,                           /* strings in variable strings  */
                0,                           /* no bytes of raw data         */
                strings,                     /* array of error strings       */
                NULL);                       /* no raw data                  */

            DeregisterEventSource (event_source_handle);
        }
    }
}


/*  ---------------------------------------------------------------------[<]-
    Function: install_service

    Synopsis: Installs the service
    ---------------------------------------------------------------------[>]-*/

void
install_service (void)
{
    SC_HANDLE
        service,
        manager;
    static char
        path [512];

    if (GetModuleFileName( NULL, path, 512 ) == 0)
      {
        printf ("%s: cannot install '%s': %s\n",
                 APPLICATION_NAME, service_name, 
                 get_last_error_text (error_buffer, LINE_MAX));
        return;
      }

    manager = OpenSCManager(
                    NULL,                      /* machine  (NULL == local)   */
                    NULL,                      /* database (NULL == default) */
                    SC_MANAGER_ALL_ACCESS      /* access required            */
                );
    if (manager)
      {
        service = CreateService (
                    manager,                   /* SCManager database         */
                    service_name,              /* Short name for service     */
                    service_text,              /* Name to display            */
                    SERVICE_ALL_ACCESS,        /* desired access             */
                    SERVICE_WIN32_OWN_PROCESS, /* service type               */
                    SERVICE_AUTO_START,        /* start type                 */
                    SERVICE_ERROR_NORMAL,      /* error control type         */
                    path,                      /* service's binary           */
                    NULL,                      /* no load ordering group     */
                    NULL,                      /* no tag identifier          */
                    DEPENDENCIES,              /* dependencies               */
                    NULL,                      /* LocalSystem account        */
                    NULL);                     /* no password                */

        if (service)
          {
            printf ("%s: service '%s' installed\n",
                     APPLICATION_NAME, service_name);
            CloseServiceHandle (service);
          }
        else
            printf ("%s: CreateService '%s' failed: %s\n", 
                    APPLICATION_NAME, service_name,
                    get_last_error_text (error_buffer, LINE_MAX));

        CloseServiceHandle (manager);
      }
    else
        printf ("%s: OpenSCManager failed: %s\n",
                APPLICATION_NAME, 
                get_last_error_text (error_buffer, LINE_MAX));
}


/*  ---------------------------------------------------------------------[<]-
    Function: remove_service

    Synopsis: Stops and removes the service
    ---------------------------------------------------------------------[>]-*/

void
remove_service (void)
{
    SC_HANDLE
        service,
        manager;

    manager = OpenSCManager(
                        NULL,                  /* machine (NULL == local)    */
                        NULL,                  /* database (NULL == default) */
                        SC_MANAGER_ALL_ACCESS  /* access required            */
                        );
    if (manager)
      {
        service = OpenService (manager, service_name, SERVICE_ALL_ACCESS);
        if (service)
          {
            /*  Try to stop the service                                      */
            if (ControlService (service, SERVICE_CONTROL_STOP, 
                &service_status))
              {
                printf ("%s: stopping service '%s'...",
                         APPLICATION_NAME, service_name);
                sleep (1);

                while (QueryServiceStatus (service, &service_status))
                    if (service_status.dwCurrentState == SERVICE_STOP_PENDING)
                      {
                        printf (".");
                        sleep  (1);
                      }
                    else
                        break;

                if (service_status.dwCurrentState == SERVICE_STOPPED)
                    printf (" Ok\n");
                else
                    printf (" Failed\n");
              }

            /*  Now remove the service                                       */
            if (DeleteService (service))
                printf ("%s: service '%s' removed\n",
                         APPLICATION_NAME, service_name);
            else
                printf ("%s: DeleteService '%s' failed: %s\n",
                         APPLICATION_NAME, service_name,
                         get_last_error_text (error_buffer, LINE_MAX));

            CloseServiceHandle (service);
          }
        else
            printf ("%s: OpenService '%s' failed: %s\n",
                     APPLICATION_NAME, service_name,
                     get_last_error_text (error_buffer, LINE_MAX));

        CloseServiceHandle (manager);
    }
    else
        printf ("%s: OpenSCManager failed: %s\n",
                 APPLICATION_NAME,
                 get_last_error_text (error_buffer, LINE_MAX));
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_service

    Synopsis: Runs the service as a console application
    ---------------------------------------------------------------------[>]-*/

void
console_service (int argc, char ** argv)
{
    printf ("%s %s: starting in console mode\n",
            APPLICATION_NAME, APPLICATION_VERSION);
    SetConsoleCtrlHandler (control_handler, TRUE);
    service_start (argc, argv);
    control_break = FALSE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: control_handler

    Synopsis: Handled console control events
    ---------------------------------------------------------------------[>]-*/

BOOL WINAPI
control_handler (DWORD control_type)
{
    switch (control_type)
      {
        /* Use Ctrl+C or Ctrl+Break to simulate SERVICE_CONTROL_STOP in      */
        /* console mode                                                      */
        case CTRL_BREAK_EVENT:
        case CTRL_C_EVENT:
            service_stop ();
            printf ("%s: stopping service\n", APPLICATION_NAME);
            control_break = TRUE;
            return (TRUE);

      }
    return (FALSE);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_last_error_text

    Synopsis: copies error message text to string
    ---------------------------------------------------------------------[>]-*/

static char *
get_last_error_text (char *buffer, int size)
{
    DWORD
        return_code;
    char
        *temp = NULL;

    return_code = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM     |
                                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                 NULL,
                                 GetLastError (),
                                 LANG_NEUTRAL,
                                 (LPTSTR) &temp,
                                 0,
                                 NULL );

    /*  Supplied buffer is not long enough                                    */
    if (return_code == 0 || ((long) size < (long) return_code + 14))
        buffer [0] = '\0';
    else
      {
        temp [lstrlen (temp) - 2] = '\0'; /*remove cr and newline character   */
        sprintf (buffer, "%s (0x%x)", temp, GetLastError ());
      }
    if (temp)
        LocalFree ((HLOCAL) temp);

    return (buffer);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_windows_version

    Synopsis: Return the windows version
    <TABLE>
    WINDOWS_95       Windows 95 or later
    WINDOWS_NT_3X    Windows NT 3.x
    WINDOWS_NT_4     Windows NT 4.0
    WINDOWS_2000     Windows 2000
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

static int
get_windows_version (void)
{
    static int
        version = 0;
    OSVERSIONINFO
        version_info;

    version_info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (GetVersionEx (&version_info))
      {
        if (version_info.dwMajorVersion < 4)
            version = WINDOWS_NT_3X;
        else
        if (version_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
          {
            if (version_info.dwMajorVersion = 4)
                version = WINDOWS_NT_4;
            else
                version = WINDOWS_2000;
          }
        else
            version = WINDOWS_95;
      }
    return (version);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_win95_service

    Synopsis: Add or remove from the windows registry the value to run
              the web server on startup.
    ---------------------------------------------------------------------[>]-*/

static long
set_win95_service (BOOL add)
{
    HKEY
        key;
    DWORD
        disp;
    long
        feedback;
    static char
        path [LINE_MAX + 1];

    feedback = RegCreateKeyEx (HKEY_LOCAL_MACHINE, REGISTRY_RUN, 
        0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &disp);

    if (feedback == ERROR_SUCCESS)
      {
        if (add)
          {
            GetModuleFileName (NULL, path, LINE_MAX);
            feedback = RegSetValueEx (key, "iafwatch", 0, REG_SZ,
                                     (CONST BYTE *) path, strlen (path) + 1);
            log_printf ("iafwatch: service '%s' installed", service_name);
            coputs   ("Restart windows to run service...");
          }
        else
          {
            feedback = RegDeleteValue (key, "iafwatch");
            log_printf ("Updater: service '%s' uninstalled", service_name);
          }
        RegCloseKey (key);
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: load_config_file

    Synopsis: Load all configuration value from config file.
    ---------------------------------------------------------------------[>]-*/

static void
load_config_file (char *file_name)
{
    XML_ITEM
        *root,
        *child = NULL,
        *item  = NULL;
    XML_ATTR
        *attr  = NULL;
    PROCESS_ITEM
        *process_item;

    if (config)
      {
        xml_free (config);
        config = NULL;
      }


    free_process_list ();

    xml_load_file (&config, ".", file_name, FALSE);
    if (config == NULL)
      {
        log_printf ("Error on load config file!");
        return;
      }

    root = xml_first_child (config);
    FORCHILDREN (item, root)
      {
        if (streq (xml_item_name (item), "main"))
          {
            run_mode    = (Bool)atoi (xml_get_attr (item, "run",   "1"));
            debug_mode  = (Bool)atoi (xml_get_attr (item, "debug", "0"));
            schedule_interval = atoi (xml_get_attr (item, "schedule", "10"));
            schedule_interval *= 1000;
            strcpy (trace_file,   xml_get_attr (item, "trace_file",  "iafwatch.log"));
            strcpy (service_name, xml_get_attr (item, "service_name", SERVICE_NAME));
            strcpy (service_text, xml_get_attr (item, "service_text", SERVICE_TEXT));
          }
        else
        if (streq (xml_item_name (item), "http"))
          {
            strcpy (http_url,     xml_get_attr (item, "url",     ""));
            strcpy (http_agent,   xml_get_attr (item, "agent",   ""));
            strcpy (http_cookie,  xml_get_attr (item, "cookie",  ""));
            strcpy (http_timeout, xml_get_attr (item, "timeout", ""));
          }
        else
        if (streq (xml_item_name (item), "launch_if_fail"))
          {
            FORCHILDREN (child, item)
              {
                process_item = mem_alloc (sizeof (PROCESS_ITEM));
                if (process_item)
                  {
                    memset (process_item, 0, sizeof (PROCESS_ITEM));
                    list_reset (process_item);
                    process_item-> name = mem_strdup (xml_get_attr (child, "value", ""));
                  }
                list_relink_before (process_item, &process_list);
              }
          }
      }

} 

/*  ---------------------------------------------------------------------[<]-
    Function: free_resource

    Synopsis: Free all allocated resources
    ---------------------------------------------------------------------[>]-*/

static void  
free_resources (void)
{
    FILE
        *trace_f = NULL;

    /* Close trace file                                                      */
    if (debug_mode)
       console_capture (NULL, 'a');

    /*  Deallocate configuration symbol table                                */
    if (config)
      {
        xml_free (config);
        config = NULL;
      }

    free_process_list ();

    /*  Check that all memory was cleanly released                           */
    if (mem_used ())
      {
        add_to_message_log ("Memory leak error, see 'memtrace.lst'");
        trace_f = fopen ("memtrace.lst", "w");
        if (trace_f)
          {
            mem_display (trace_f);
            fclose (trace_f);
          }
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: hide_window

    Synopsis: Hidden console window.
    ---------------------------------------------------------------------[>]-*/

static void
hide_window (void)
{
    char
        title [255];
    HWND
        win;
    GetConsoleTitle  (title, 254);
    win = FindWindow (NULL,title);
    if (win)
        SetWindowPos (win, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
}


/*  -------------------------------------------------------------------------
 *  date_str
 *
 *  Returns the current date formatted as: "yyyy/mm/dd".
 *
 *  Safety:  NOT thread safe (shared variables), buffer safe.
 */

static char *
date_str (void)
{
    static char
        formatted_date [11];
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    snprintf (formatted_date, sizeof (formatted_date),
	                      "%4d/%02d/%02d",
                              time_struct-> tm_year + 1900,
                              time_struct-> tm_mon  + 1,
                              time_struct-> tm_mday);
                            
    return (formatted_date);
}


/*  -------------------------------------------------------------------------
 *  time_str
 *
 *  Returns the current time formatted as: "hh:mm:ss".
 *
 *  Safety:  NOT thread safe (shared variables), buffer safe.
 */

static char *
time_str (void)
{
    static char
        formatted_time [9];
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    snprintf (formatted_time, sizeof (formatted_time),
	                      "%02d:%02d:%02d",
                              time_struct-> tm_hour,
                              time_struct-> tm_min,
                              time_struct-> tm_sec);
    return (formatted_time);
}


/*  ---------------------------------------------------------------------[<]-
    Function: log_printf

    Synopsis: As printf() but sends output to the current console.  This is
    by default the stderr device, unless you used console_send() to direct
    console output to some function.  A newline is added automatically.

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/

static int
log_printf (const char *format, ...)
{
    va_list
        argptr;                         /*  Argument list pointer            */
    int
        fmtsize = 0;                    /*  Size of formatted line           */
    char
        *formatted = NULL,              /*  Formatted line                   */
        *prefixed = NULL;               /*  Prefixed formatted line          */
    FILE
        *console_file;

/*  Maximum size of a line we'll print using log_printf                        */
#define CONSOLE_LINE_MAX  16000
#define CONSOLE_ECHO_STREAM   stderr    /*  Stream to echo console output to */

    console_file = file_open (trace_file, 'a');
    if (console_file)
      {
        formatted = mem_alloc (CONSOLE_LINE_MAX + 1);
        if (!formatted)
            return (0);
        va_start (argptr, format);      /*  Start variable args processing   */
        vsnprintf (formatted, CONSOLE_LINE_MAX, format, argptr);
        va_end (argptr);                /*  End variable args processing     */
        xstrcpy_debug ();
        prefixed = xstrcpy (NULL, date_str (), " ", time_str (), ": ",
                                    formatted, NULL);
        file_write (console_file, prefixed? prefixed: formatted);
        fflush (console_file);
        fprintf (CONSOLE_ECHO_STREAM, "%s", prefixed? prefixed: formatted);
        fprintf (CONSOLE_ECHO_STREAM, "\n");
        fflush  (CONSOLE_ECHO_STREAM);

        if (prefixed)
          {
            fmtsize = strlen (prefixed);
            mem_free (prefixed);
          }
        else
            fmtsize = strlen (formatted);

        mem_free (formatted);

        file_close (console_file);
      }
    return (fmtsize);
}

static void
free_process_list (void)
{
    PROCESS_ITEM
        *current,
        *next;

    current = (PROCESS_ITEM *)process_list.next;
    while (current != (PROCESS_ITEM *)&process_list)
      {
        next = current-> next;
        if (current-> name)
            mem_free (current-> name);
        mem_free (current);
        current = next;
      }
}


static void
watch_url (void)
{
    char
        *out    = "outfile.txt",
        *status = "status.txt",
        buffer [10240];
    FILE
       *file;

    sprintf (buffer, "curl -f -m %s -b \"%s\" -A \"%s\" %s",
                    http_timeout,
                    http_cookie,
                    http_agent,
                    http_url);
    if (file_exists (status))
        file_delete (status);

    if (process_create (
        buffer,                         /*  Command to execute               */
        NULL,                           /*  Arguments for command            */
        NULL,                           /*  Work in current directory        */
        NULL, out, status,              /*  redirect stdin/out/err           */
        NULL,                           /*  User current environment         */
        TRUE)                           /*  And wait until it's finished     */
        == 0)
      {
        log_printf ("cannot execute '%s': %s", buffer,
                     strerror (errno));
      }
    else
      {
        if (file_exists (status))
          {
            file = fopen (status, "rt");
            if (file)
              {
                while (fgets (buffer, sizeof (buffer) - 1, file))
                  {
                    if (*buffer 
                    &&  strstr (buffer, "curl: ")
                    &&  get_file_size (out) == 0)
                      {
                        log_printf ("Server don't respond, restart (%s)",
                                     buffer);
                        execute_process_list ();
                        break;
                      }
                  }
                fclose (file);
              }
          }
      }
}


static void
execute_process_list (void)
{
    PROCESS_ITEM
        *current,
        *next;

    current = (PROCESS_ITEM *)process_list.next;
    while (current != (PROCESS_ITEM *)&process_list)
      {
        next = current-> next;
        if (current-> name
        &&  current-> name)
          {
            if (process_create (
                current-> name,         /*  Command to execute               */
                NULL,                   /*  Arguments for command            */
                NULL,                   /*  Work in current directory        */
                NULL, NULL, NULL,       /*  Don't redirect stdin/out/err     */
                NULL,                   /*  User current environment         */
                TRUE)                   /*  And wait until it's finished     */
                == 0)
              {
                log_printf ("cannot execute '%s': %s", current-> name,
                            strerror (errno));
                break;
              }
          }
        current = next;
      }
}
