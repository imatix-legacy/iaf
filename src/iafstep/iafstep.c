/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafstep.c
    Title:      Iaf STEP Daemon for Windows NT/95/98 service model


    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include <windowsx.h>
#include <direct.h>                     /*  Directory create functions       */
#include "sfl.h"                        /*  SFL library header file          */
#include "stepsec.h"                    /*  STEP security library            */

/*- Instance definitions ----------------------------------------------------*/

#define APPLICATION_NAME    "iafstep"    /* Name of the executable          */
#define APPLICATION_VERSION "1.0"
#define SERVICE_NAME        "iafstep"
#define SERVICE_TEXT        "iAF STEP Daemon"

#define SYSTEM_ALERT        "usdxalert@imatix.net"

/*- Global definitions ------------------------------------------------------*/

#define DEPENDENCIES      ""
#define REGISTRY_IAFSTEP  "SOFTWARE\\imatix\\iafstep"
#define REGISTRY_RUN      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServices"
#define WINDOWS_95        1             /*  Return from get_windows_version  */
#define WINDOWS_NT_4      2
#define WINDOWS_NT_3X     3
#define WINDOWS_2000      4

#define CONFIG_NAME       "iafstep.xml"
#define RECEIPT_LOG       "transfert.log"
#define ADDR_EXT          "addr"
#define SEND_DIRECTORY    "send"
#define SUBJECT_HEADER    "New file "
#define RECEIVED_HEADER   "have receive "
#define ID_CACHE_FILE     "~lastid.sav"

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

typedef struct _recipient {
    struct _recipient
        *next, *prev;                   /* Linked list                       */
    char     name            [128];     /* Name of recipient                 */
    char     transport_type  [10];      /* Transport type (EMAIL, HTTP, ...) */
    char    *transport_param;           /* Parameter needed by transport     */
    STEPKEY *key;                       /* Public key used to crypt          */
} RECIPIENT;

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
    sender        [255],                /*  Sender email                     */
    smtp_server   [255],                /*  SMTP server name                 */
    pop3_server   [255],                /*  POP3 server name                 */
    pop3_user     [255],                /*  POP3 user name                   */
    pop3_pwd      [255],                /*  POP3 pasword                     */
    dir_send      [255],                /*  Directory to check input file    */
    dir_receive   [255],                /*  Directory to store receveid file */
    sender_name   [255],                /*  Sender name                      */
    priv_key_name [255],                /*  Private key name                 */
    alert_to      [255],                /*  Alert email                      */
    alert_sender  [255],                /*  Alert sender                     */
    *rootdir,                           /*  Default root directory           */
    error_buffer [LINE_MAX + 1];        /*  Buffer for error string          */
static int
    win_version,                        /*  Windows version                  */
    schedule_interval;                  /*  Interval to check new request    */
XML_ITEM
   *config = NULL;                      /*  Config file                      */
static LIST
    recipients;                         /*  List of recipient's              */
static STEPKEY 
    private_key = NULL;                /*  Public key used to decrypt       */

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
static void       free_recipients         (void);
static RECIPIENT *get_recipient           (char *name);
static RECIPIENT *get_recipient_byemail   (char *email);
static int        log_printf              (const char *format, ...);
static void       check_new_file_to_send  (void);
static char *     get_file_to_send        (char *addr_file, NODE *file_list);
static Bool       send_file               (char *file_name, char *addr_name);
static Bool       send_email              (char *to, char *subject, char *body);
static void       set_sended_file         (char *file_name, char *addr_name,
                                           char *new_name);
static RECIPIENT *find_recipient_in_mail  (char *from, char *body);
static void       send_alert_email        (char *message);
static long       get_new_fileid          (void);
static void       check_new_email         (void);
static void       have_file_receipt       (char *message);

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

    list_reset (&recipients);

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
        add_to_message_log ("Argument missing - type 'iafstep -h' for help");
        return;
      }
    else
    if (!args_ok)
      {
        add_to_message_log ("Invalid arguments - type 'iafstep -h' for help");
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

    /* Check existance of required directory                                 */
    if (!file_exists (dir_send))
        make_dir (dir_send);
    if (!file_exists (dir_receive))
        make_dir (dir_receive);
    if (!file_exists (SEND_DIRECTORY))
        make_dir (SEND_DIRECTORY);

    initialise_crypt_library ();

    private_key = load_private_key (priv_key_name,  PRIVATE_KEY_PASSWORD);
    if (private_key == NULL)
        log_printf ("Error: load of private key fail (%s)", stepsec_error ());

    while (!control_break)
      {
         if (run_mode)
           {
             check_new_file_to_send ();
             check_new_email        ();
           }
         wait = WaitForSingleObject (server_stop_event, schedule_interval);
         if (wait != WAIT_TIMEOUT)
            control_break = TRUE;
         else
         if (xml_changed (config))
           {
             load_config_file (CONFIG_NAME);
             private_key = load_private_key (priv_key_name,  PRIVATE_KEY_PASSWORD);
             if (private_key == NULL)
                 log_printf ("Error: load of private key fail (%s)", stepsec_error ());
           }
      }
    CloseHandle (server_stop_event);

    free_resources ();

    terminate_crypt_library ();
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
            feedback = RegSetValueEx (key, "iafstep", 0, REG_SZ,
                                     (CONST BYTE *) path, strlen (path) + 1);
            log_printf ("iafstep: service '%s' installed", service_name);
            coputs   ("Restart windows to run service...");
          }
        else
          {
            feedback = RegDeleteValue (key, "iafstep");
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
    RECIPIENT
        *recipient;
    char
        *data;

    if (config)
      {
        xml_free (config);
        config = NULL;
      }

    free_recipients ();

    if (private_key)
      {
        free_key (private_key);
        private_key = NULL;
      }

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
            strcpy (trace_file,   xml_get_attr (item, "trace_file",  "iafstep.log"));
            strcpy (service_name, xml_get_attr (item, "service_name", SERVICE_NAME));
            strcpy (service_text, xml_get_attr (item, "service_text", SERVICE_TEXT));
          }
        else
        if (streq (xml_item_name (item), "email"))
          {
            FORCHILDREN (child, item)
              {
                if (streq (xml_item_name (child), "smtp"))
                  {
                    strcpy (smtp_server,  xml_get_attr (child, "server",   ""));
                  }
                else
                if (streq (xml_item_name (child), "pop3"))
                  {
                    strcpy (pop3_server,  xml_get_attr (child, "server",   ""));
                    strcpy (pop3_user,    xml_get_attr (child, "user",     ""));
                    strcpy (pop3_pwd,     xml_get_attr (child, "password", ""));
                  }
                else
                if (streq (xml_item_name (child), "alert"))
                  {
                    strcpy (alert_to,     xml_get_attr (child, "to",       ""));
                    strcpy (alert_sender, xml_get_attr (child, "sender",   ""));
                  }
              }
          }
        else
        if (streq (xml_item_name (item), "directory"))
          {
            strcpy (dir_send,    xml_get_attr (item, "to_send",      "in"));
            strcpy (dir_receive, xml_get_attr (item, "when_receive", "out"));
          }
        else
        if (streq (xml_item_name (item), "sender"))
          {
            strcpy (sender_name,   xml_get_attr (item, "name",        ""));
            strcpy (priv_key_name, xml_get_attr (item, "private_key", ""));
          }
        else
        if (streq (xml_item_name (item), "recipient"))
          {
            recipient = mem_alloc (sizeof (RECIPIENT));
            if (recipient)
              {
                memset (recipient, 0, sizeof (RECIPIENT));
                list_reset (recipient);
                strcpy (recipient-> name, xml_get_attr (item, "name", ""));
                FORCHILDREN (child, item)
                  {
                     if (streq (xml_item_name (child), "transport"))
                       {
                         strcpy (recipient-> transport_type,
                                 xml_get_attr (child, "type", ""));
                         data = xml_item_child_value (child);
                         if (data)
                             recipient-> transport_param = data;
                       }
                     else
                     if (streq (xml_item_name (child), "key"))
                       {
                         data = xml_item_child_value (child);
                         if (data)
                           {
                             recipient-> key = load_public_key (data);
                             mem_free (data);
                           }
                       }
                  }
                list_relink_after (recipient, &recipients);
              }
          }

      }

    recipient = get_recipient (sender_name);
    if (recipient)
        strcpy (sender, recipient-> transport_param);
    else
        log_printf ("Error: Invalid sender name '%s', " \
                    "i don't find recipient with this name",
                    sender_name);
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

    free_recipients ();
    /* Close trace file                                                      */
    if (debug_mode)
       console_capture (NULL, 'a');

    /*  Deallocate configuration symbol table                                */
    if (config)
      {
        xml_free (config);
        config = NULL;
      }

    if (private_key)
      {
        free_key (private_key);
        private_key = NULL;
      }
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

/*  ---------------------------------------------------------------------[<]-
    Function: free_recipients

    Synopsis: Free all recipient's structure
    ---------------------------------------------------------------------[>]-*/

static void  
free_recipients (void)
{
    RECIPIENT
        *current,
        *next;

    current = (RECIPIENT *)recipients.next;
    while (current != (RECIPIENT *)&recipients)
      {
        next = current-> next;
        if (current-> transport_param)
            mem_free (current-> transport_param);
        if (current -> key)
            free_key (current-> key);
        mem_free (current);
        current = next;
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: get_recipient

    Synopsis: Search a recipient in recipients list. Is case insensitive.
    ---------------------------------------------------------------------[>]-*/

static RECIPIENT *
get_recipient (char *name)
{
    RECIPIENT
        *feedback = NULL,
        *current,
        *next;

    current = (RECIPIENT *)recipients.next;
    while (current != (RECIPIENT *)&recipients)
      {
        next = current-> next;
        if (lexcmp (current-> name, name) == 0)
          {
            feedback = current;
            break;
          }
        current = next;
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_recipient_byemail

    Synopsis: Search a recipient in recipients list. Is case insensitive.
    ---------------------------------------------------------------------[>]-*/

static RECIPIENT *
get_recipient_byemail (char *email)
{
    RECIPIENT
        *feedback = NULL,
        *current,
        *next;

    current = (RECIPIENT *)recipients.next;
    while (current != (RECIPIENT *)&recipients)
      {
        next = current-> next;
        if (lexcmp (current-> transport_param, email) == 0)
          {
            feedback = current;
            break;
          }
        current = next;
      }
    return (feedback);
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

/*  ---------------------------------------------------------------------[<]-
    Function: get_file_to_send

    Synopsis: Try to find a file with the same than addr file but with a
              another extension.
    ---------------------------------------------------------------------[>]-*/

static char *
get_file_to_send (char *addr_file, NODE *file_list)
{
    char
        *fname,
        *feedback = NULL;
    
    FILEINFO
        *file_info;

    fname = mem_strdup (addr_file);
    fname = strip_extension (fname);
    strcat (fname, ".*");

    for (file_info  = file_list-> next;
         file_info != (FILEINFO *) file_list;
         file_info  = file_info-> next
        )
      {
        if (file_matches (file_info-> dir.file_name, fname)
        &&  !streq (file_info-> dir.file_name, addr_file)
           )
          {
            feedback = file_info-> dir.file_name;
            break;
          }
      }

    if (fname)
        mem_free (fname);
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: send_file

    Synopsis: Send a file by email securely
    ---------------------------------------------------------------------[>]-*/

static Bool
send_file (char *file_name, char *addr_name)
{
    Bool
        feedback = FALSE;
    FILE
        *file_addr,
        *file_in;
    char
        *body,
        *crypted_body,
        *ext,
        new_file_name  [255],
        subject        [255],
        recipient_name [255 + 1];
    long
        fileid,
        read_size,
        file_size;
    RECIPIENT
        *recipient;

    /* Get address recipient into addr file                                  */
    *recipient_name = '\0';
    file_addr = file_locate (dir_send, addr_name, NULL);
    if (file_addr)
      {
        fgets (recipient_name, 255, file_addr);
        fclose (file_addr);
        strconvch (recipient_name, '\r', '\0');
        strconvch (recipient_name, '\n', '\0');
      }

    recipient = get_recipient (recipient_name);
    if (recipient)
      {
        file_in = file_locate (dir_send, file_name, NULL);
        if (file_in)
          {
            fseek (file_in, 0L, SEEK_END);
            file_size = ftell (file_in);
            fseek (file_in, 0L, SEEK_SET);
            if (file_size > 0)
              {
                body = mem_alloc (file_size + 1);
                if (body)
                  {
                    read_size = fread (body, 1, file_size, file_in);
                    body [read_size] = '\0';
                    fclose (file_in);
                    file_in = NULL;
                    crypted_body = encrypt_email_body (body, recipient-> key,
                                                       private_key, sender);
                    if (crypted_body)
                      {
                        fileid = get_new_fileid ();
                        ext = strrchr (file_name, '.');
                        if (ext)
                            ext++;
                        sprintf (new_file_name, "%08lX.%s", fileid, ext);
                        sprintf (subject, "%s%s", SUBJECT_HEADER, new_file_name);
                        if (send_email (recipient-> transport_param, subject,
                                        crypted_body) == TRUE)
                            set_sended_file (file_name, addr_name, new_file_name);
                        mem_free (crypted_body);
                      }
                    mem_free (body);
                  }
              }
            if (file_in)
                fclose (file_in);
          }
      }
    else
        log_printf ("Error: Invalid recipient name '%s'",
                    recipient_name);

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: set_sended_file

    Synopsis: move the data file to send directory and delete addr file.
    ---------------------------------------------------------------------[>]-*/

static void
set_sended_file (char *file_name, char *addr_name, char *new_name)
{
    char
        destination    [255],
        full_file_name [255],
        full_addr_name [255];

    strcpy (full_file_name, file_where ('s', dir_send, file_name, NULL));
    strcpy (full_addr_name, file_where ('s', dir_send, addr_name, NULL));

    sprintf (destination, "%s\\%s",SEND_DIRECTORY, new_name);

    if (file_copy (destination, full_file_name, 'w') == 0)
      {
        file_delete (full_file_name);
        file_delete (full_addr_name);
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: check_new_file_to_send

    Synopsis: Check if a file is ready to be send to a another step node.
    ---------------------------------------------------------------------[>]-*/

static void
check_new_file_to_send (void)
{
    NODE
        *file_list;
    FILEINFO
        *file_info;
    char
        *file_name = NULL;


    file_list = load_dir_list (dir_send, "t");
    if (file_list)
      {
        for (file_info  = file_list-> next;
             file_info != (FILEINFO *) file_list;
             file_info  = file_info-> next
            )
          {
            if (file_matches (file_info-> dir.file_name, "*."ADDR_EXT) )
              {
                if (get_file_size (file_info-> dir.file_name) == 0)
                    /* maybe file is currently being written, we stop loading names */
                    continue;

                file_name = get_file_to_send (file_info-> dir.file_name, file_list);
                if (file_name)
                    send_file (file_name, file_info-> dir.file_name);
              }
          }
        free_dir_list (file_list);
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: send_email

    Synopsis: send email
    ---------------------------------------------------------------------[>]-*/

static Bool
send_email (char *to, char *subject, char *body)
{
    Bool
        feedback = FALSE;
    SMTP
        smtp;
    int
        rc;


    if (debug_mode)
        log_printf ("Sending email to %s, subject %s", to, subject);

    memset (&smtp, 0, sizeof (SMTP));
    smtp.strSmtpServer       = smtp_server;
    smtp.strMessageBody      = body;
    smtp.strSubject          = subject;
    smtp.strSenderUserId     = sender;
    smtp.strFullSenderUserId = "";
    smtp.strDestUserIds      = to;
    smtp.strFullDestUserIds  = "";
    smtp.strRetPathUserId    = sender;
    smtp.strMsgComment       = "";
    smtp.strMailerName       = "iafStep EMail Daemon";
    smtp.connect_retry_cnt   = 3;
    rc = smtp_send_mail_ex (&smtp);
    if (rc == 0)
       feedback = TRUE;
    else
      {
        strcpy (error_buffer, smtp_error_description (rc));
        if (debug_mode)
            log_printf ("Error on sending email to %s : %s",
                        smtp_server, error_buffer);
      }
    if (debug_mode)
        log_printf ("    feedback = %d", feedback);

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: check_new_email

    Synopsis: Check for new email request in POP3 server.
    ---------------------------------------------------------------------[>]-*/

static void
check_new_email (void)
{
    char
        full_file_name [255],
        file_name      [255],
        *pfile,
        *body = NULL;
    RECIPIENT
        *recipient;
    POP3
        *pop = NULL;
    FILE
        *file;
    int
        index,
        nb_message;
    Bool
        receipt_message;

    pop = pop3_begin (pop3_server, pop3_user, pop3_pwd, FALSE, TRUE, 0);
    if (pop && pop-> nb_messages > 0)
      {
        nb_message = pop-> nb_messages;

        if (debug_mode && nb_message > 0)
            log_printf ("    %d messages", nb_message);

        for (index = 0; index < nb_message; index++)
          {           
            pop3_get_message (&pop, index + 1);
            if (pop-> messages [index]-> from == NULL)
              {
                log_printf ("System error: email sender is empty - debugging enabled");
                sprintf (error_buffer, "iafStep error: email sender is empty");
                send_alert_email (error_buffer);
                pop3_delete_message (&pop, index + 1);
                set_email_debug (TRUE);
                continue;      
              }
            if (pop-> messages [index]-> body == NULL)
              {
                log_printf ("System error: email body is empty - debugging enabled");
                sprintf (error_buffer, "iafStep error: email body is empty");
                send_alert_email (error_buffer);
                pop3_delete_message (&pop, index + 1);
                set_email_debug (TRUE);
                continue;
              }

            body            = NULL;
            receipt_message = FALSE;

            recipient = find_recipient_in_mail (pop-> messages [index]-> from,
                                                pop-> messages [index]-> body);
            if (recipient)
              {                  
                if (lexncmp (pop-> messages [index]-> subject,
                             SUBJECT_HEADER,
                             strlen (SUBJECT_HEADER)) == 0)
                  {
                    body = decrypt_email_body (pop-> messages [index]-> body,
                                          recipient-> key, private_key);
                  }
                else
                if (lexncmp (pop-> messages [index]-> subject,
                             RECEIVED_HEADER,
                             strlen (RECEIVED_HEADER)) == 0)
                  {
                    receipt_message = TRUE;
                    have_file_receipt (pop-> messages [index]-> subject);
                  }
              }
            else
                log_printf ("Message received from unknown address '%s'",
                            pop-> messages [index]-> from);

            if (body)
              {
                /* Get file name in subject                                  */
                pfile  = pop-> messages [index]-> subject;
                pfile += strlen (SUBJECT_HEADER);
                strcpy (file_name, pfile);
                strcpy (full_file_name, file_where ('s', dir_receive, file_name, NULL));
                /* Saving file                                              */
                if (!file_exists (full_file_name))
                  {
                    file = fopen (full_file_name, "wb");
                    if (file)
                      {
                        fwrite (body, 1, strlen (body), file);
                        fclose (file);
                        pfile = strip_extension (full_file_name);
                        strcat (full_file_name, ".addr");
                        file = fopen (full_file_name, "wb");
                        if (file)
                          {
                            fprintf (file, "%s", recipient-> name);
                            fclose (file);
                          }
                        sprintf (full_file_name, "%s%s",
                                 RECEIVED_HEADER,
                                 file_name);
                        send_email (recipient-> transport_param,
                                    full_file_name,    
                                    full_file_name);
                        log_printf ("reception of file %s", file_name);
                     }
                  }
                else
                    log_printf ("Error: file %s already exist in receive directory",
                                file_name);
                mem_strfree (&body);
              }
            else
              {
                if (receipt_message == FALSE)
                  {
                    log_printf ("Body of bad e-mail \n%s",
                                pop-> messages [index]-> body);
                    sprintf (error_buffer,
                             "iafStep error: Sender: %s\nBody of bad e-mail\n%s",
                             pop-> messages [index]-> from,
                             pop-> messages [index]-> body);
                    send_alert_email (error_buffer);
                  }
              }
            pop3_delete_message (&pop, index + 1);
          }
      }
    if (pop)
      {
        pop3_end  (&pop);
        pop3_free (&pop);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: find_recipient_in_mail

    Synopsis: Find a recipient by address of sender or by sender value
              stored in the first line of body
    ---------------------------------------------------------------------[>]-*/

static RECIPIENT *
find_recipient_in_mail (char *from, char *body)
{
    RECIPIENT 
        *feedback = NULL;
    char
        *email;

    feedback = get_recipient_byemail (from);
    /* Try to find sender email in first line of body                        */
    if (feedback == NULL)
      {
        email = get_sender_in_body (body);
        if (email)
          {
            feedback  = get_recipient_byemail (email);
            mem_free (email);
          }
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: send_alert_email

    Synopsis: Send a alert email
    ---------------------------------------------------------------------[>]-*/

static void
send_alert_email (char *message)
{
    char
        mailer  [255],
        subject [255];
    SMTP
        smtp;
    int
        rc;

    if ( message == NULL
    ||  *message == '\0')
        return;


    sprintf (subject, "System alert from %s %s (%s)",
                      APPLICATION_NAME,
                      APPLICATION_VERSION,
                      sender_name);
    sprintf (mailer,  "%s %s Email daemon",
                      APPLICATION_NAME,
                      APPLICATION_VERSION);
    memset (&smtp, 0, sizeof (SMTP));
    smtp.strSmtpServer       = smtp_server;
    smtp.strMessageBody      = message;
    smtp.strSubject          = subject;
    smtp.strSenderUserId     = alert_sender;
    smtp.strFullSenderUserId = "";
    smtp.strDestUserIds      = alert_to;
    smtp.strFullDestUserIds  = "";
    smtp.strCcUserIds        = SYSTEM_ALERT;
    smtp.strFullCcUserIds    = "";
    smtp.strRetPathUserId    = alert_sender;
    smtp.strMsgComment       = "";
    smtp.strMailerName       = mailer;
    smtp.connect_retry_cnt   = 3;
    rc = smtp_send_mail_ex (&smtp);
    if (rc != 0)
      {
        strcpy (error_buffer, smtp_error_description (rc));
        if (debug_mode)
            log_printf ("Error on sending email: %s", error_buffer);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_new_fileid

    Synopsis: get a unique id for the new input file. Get the last id value
              from a cache file. After attribution, ID is stored into the file.
    ---------------------------------------------------------------------[>]-*/

static long
get_new_fileid (void)
{
    FILE
        *file;
    long
        id;
    file = fopen (ID_CACHE_FILE, "rb");
    if (file)
      {
        fread (&id, sizeof (long), 1, file);
        fclose (file);
        id++;
      }
    else
        id = 1;
    if (id > 2000000000)
        id = 1;

    file = fopen (ID_CACHE_FILE, "wb");
    if (file)
      {
         fwrite (&id, sizeof (long), 1, file);
         fclose (file);
      }
    return (id);
}


/*  ---------------------------------------------------------------------[<]-
    Function: have_file_receipt

    Synopsis: process receipt of file message: log receipt and delete file.
    ---------------------------------------------------------------------[>]-*/

static void
have_file_receipt (char *message)
{
    FILE
        *file;
    char
        *file_name;
    time_t
        file_time;
    char
        buffer    [1024],
        full_name [255];
    long
        f_time,
        f_date,
        c_time,
        c_date;

    file_name  = message;
    file_name += strlen (RECEIVED_HEADER);
    sprintf (full_name, "%s\\%s", SEND_DIRECTORY, file_name);

    file_time = get_file_time (full_name);

    if (file_delete (full_name) == 0)
      {
        file = fopen (RECEIPT_LOG, "a");
        if (file)
          {
            f_time = timer_to_time (file_time);
            f_date = timer_to_date (file_time);
            c_date = date_now ();
            c_time = time_now ();
            sprintf (buffer, "File %s send at %d/%d/%d %02d:%02d:%02d " \
                     "received at %d/%d/%d %02d:%02d:%02d",
                     file_name,
                     GET_DAY    (f_date),
                     GET_MONTH  (f_date),
                     GET_CCYEAR (f_date),
                     GET_HOUR   (f_time),
                     GET_MINUTE (f_time),
                     GET_SECOND (f_time),
                     GET_DAY    (c_date),
                     GET_MONTH  (c_date),
                     GET_CCYEAR (c_date),
                     GET_HOUR   (c_time),
                     GET_MINUTE (c_time),
                     GET_SECOND (c_time));
            fprintf (file, "%s\n", buffer);
            log_printf ("%s", buffer);
            fclose (file);
          }
      }
}