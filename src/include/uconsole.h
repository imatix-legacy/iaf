/*  ----------------------------------------------------------------<Prolog>-
    Name:       uconsole.c
    Title:      Unicode Console print functions
    Package:    iMatix Application Frameworf (iAF)

    Written:    2000/05/25  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/10/03

    Copyright:  Copyright (c) 1991-2002 iMatix

    Synopsis:   Unicode console printf (from SFL)

 ------------------------------------------------------------------</Prolog>-*/

#ifndef UCONSOLE_INCLUDED               /*  Allow multiple inclusions        */
#define UCONSOLE_INCLUDED


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

void  console_usend        (CONSOLE_UFCT  *console_fct, Bool echo);
void  console_uenable      (void);
void  console_udisable     (void);
void  console_uset_mode    (int CONSOLE_MODE);
int   console_ucapture     (const char *filename, char mode);
int   couprintf            (const UCODE *format, ...);

#ifdef __cplusplus
}
#endif


#endif
