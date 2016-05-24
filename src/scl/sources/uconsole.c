/*  ----------------------------------------------------------------<Prolog>-
    Name:       uconsole.c
    Title:      Unicode Console print functions
    Package:    iMatix Application Frameworf (iAF)

    Written:    2000/05/25  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/10/03

    Copyright:  Copyright (c) 1991-2002 iMatix

    Synopsis:   Unicode console printf (from SFL)

 ------------------------------------------------------------------</Prolog>-*/
#include "sfl.h"
#include "uconsole.h"

/*  Maximum size of a line we'll print using coprintf                        */
#define CONSOLE_LINE_MAX  256000

static Bool
    console_active = TRUE,              /*  Allow console output?            */
    console_echo   = TRUE;              /*  Copy to console echo stream      */
#define CONSOLE_ECHO_STREAM   stderr    /*  Stream to echo console output to */

static CONSOLE_FCT
    *console_fct = NULL;                /*  Redirector function              */
static int
    console_mode = CONSOLE_PLAIN;       /*  Output display mode              */
static FILE
    *console_file = NULL;               /*  Capture file, if any             */

static char  *date_str  (void);
static char  *time_str  (void);

#if (defined (DOES_UNICODE_PRINTF))
static CONSOLE_UFCT
    *console_ufct = NULL;               /*  Redirector function              */
static UFILE
    *console_ufile = NULL;              /*  Unicode capture file, if any     */

static UCODE *date_ustr (void);
static UCODE *time_ustr (void);
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: console_usend

    Synopsis: Redirects console output to a specified CONSOLE_FCT function.
    If the specified address is NULL, the default handling will be reinstated.
    This is independent of any console capturing in progress.  If the echo
    argument is TRUE, console output is also sent to the echo stream (stderr).

    Unicode version

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/
void
console_usend (CONSOLE_UFCT *new_console_fct, Bool echo)
{
    console_ufct  = new_console_fct;
    console_echo = echo;                /*  Copy to console echo stream      */
}

/*  ---------------------------------------------------------------------[<]-
    Function: console_enable

    Synopsis: Enables console output.  Use together with console_disable()
    to stop and start console output.

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/

void
console_uenable (void)
{
    console_active = TRUE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_disable

    Synopsis: Disables console output. Use together with console_enable()
    to stop and start console output.

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/

void
console_udisable (void)
{
    console_active = FALSE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_set_mode

    Synopsis: Sets console display mode; the argument can be one of:
    <TABLE>
    CONSOLE_PLAIN       Output text exactly as specified
    CONSOLE_DATETIME    Prefix text by "yy/mm/dd hh:mm:ss "
    CONSOLE_TIME        Prefix text by "hh:mm:ss "
    </TABLE>
    The default is plain output.

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/

void
console_uset_mode (int mode)
{
    ASSERT (mode == CONSOLE_PLAIN
         || mode == CONSOLE_DATETIME
         || mode == CONSOLE_TIME);

    console_mode = mode;
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_ucapture

    Synopsis: Starts capturing console output to the specified file.  If the
    mode is 'w', creates an empty capture file.  If the mode is 'a', appends
    to any existing data.  Returns 0 if okay, -1 if there was an error - in
    this case you can test the value of errno.  If the filename is NULL or
    an empty string, closes any current capture file.

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/

int
console_ucapture (const char *filename, char mode)
{
    if (console_ufile)
      {
        unicode_close (console_ufile);
        console_ufile = NULL;
      }
    if (filename && *filename)
      {
        ASSERT (mode == 'w' || mode == 'a');
        console_ufile = unicode_open (filename, mode);
        if (console_ufile == NULL)
            return (-1);
        else if (ftell (console_ufile-> file) == 0)
            unicode_set_format (console_ufile, UNICODE_FORMAT_UTF8, BYTE_ORDER);
      }
    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: couprintf

    Synopsis: As printf() but sends output to the current console.  This is
    by default the stderr device, unless you used console_send() to direct
    console output to some function.  A newline is added automatically.

    Unicode version.

    Safety:  NOT thread safe (shared variables), buffer safe.
    ---------------------------------------------------------------------[>]-*/

int
couprintf (const UCODE *format, ...)
{
    va_list
        argptr;                         /*  Argument list pointer            */
    int
        fmtsize = 0;                    /*  Size of formatted line           */
    UCODE
        *line,
        *formatted = NULL,              /*  Formatted line                   */
        *prefixed = NULL;               /*  Prefixed formatted line          */

    if (console_active)
      {
        formatted = unicode_alloc (CONSOLE_LINE_MAX + 1 );
        if (!formatted)
            return (0);
        va_start (argptr, format);      /*  Start variable args processing   */
        _vsnwprintf (formatted, CONSOLE_LINE_MAX - 1, format, argptr);
        va_end (argptr);                /*  End variable args processing     */

        wcscat (formatted, L"\n");
        switch (console_mode)
          {
            case CONSOLE_DATETIME:
                prefixed = unicode_alloc (wcslen (formatted) + 25);
                if (prefixed)
                  {
                    wcscpy (prefixed, date_ustr ());
                    wcscat (prefixed, L" ");
                    wcscat (prefixed, time_ustr ());
                    wcscat (prefixed, L": ");
                    wcscat (prefixed, formatted);
                  }
                break;
            case CONSOLE_TIME:
                prefixed = unicode_alloc (wcslen (formatted) + 25);
                  {
                    wcscpy (prefixed, time_ustr ());
                    wcscat (prefixed, L": ");
                    wcscat (prefixed, formatted);
                  }
                break;
          }
        line    = prefixed? prefixed: formatted;
        fmtsize = wcslen (line);

        if (console_ufile)
          {
            unicode_write (line, fmtsize, console_ufile);
            fflush (console_ufile-> file);
          }
        if (console_ufct)
            (console_ufct) (line);

        if (console_echo)
          {
            fwprintf (CONSOLE_ECHO_STREAM, L"%s", line);
            fwprintf (CONSOLE_ECHO_STREAM, L"\n");
            fflush  (CONSOLE_ECHO_STREAM);
          }
        if (prefixed)
            mem_free (prefixed);

        mem_free (formatted);
      }
    return (fmtsize);
}

/*  -------------------------------------------------------------------------
 *  date_str
 *
 *  Returns the current date formatted as: "yyyy/mm/dd".
 *
 *  Safety:  NOT thread safe (shared variables), buffer safe.
 */

static UCODE *
date_ustr (void)
{
    static UCODE
        formatted_date [11];
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    swprintf (formatted_date, L"%4d/%02d/%02d",
                              time_struct-> tm_year + 1900,
                              time_struct-> tm_mon  + 1,
                              time_struct-> tm_mday);
                            
    return (formatted_date);
}


/*  -------------------------------------------------------------------------
 *  time_ustr
 *
 *  Returns the current time formatted as: "hh:mm:ss".
 *
 *  Safety:  NOT thread safe (shared variables), buffer safe.
 */

static UCODE *
time_ustr (void)
{
    static UCODE
        formatted_time [9];
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    swprintf (formatted_time, L"%02d:%02d:%02d",
                              time_struct-> tm_hour,
                              time_struct-> tm_min,
                              time_struct-> tm_sec);
    return (formatted_time);
}
