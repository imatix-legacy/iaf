/*  ----------------------------------------------------------------<Prolog>-
    Name:       emaildb.h
    Title:      UltraSource EMail daemon Database Access

    Written:    2000/07/14  Pascal Antonnaux <pacal@imatix.com>
    Revised:    2000/07/14

    Synopsis:   Get data from email log and email queue

    This program is copyright (c) 1991-2000 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/
#ifndef _EMAILDB_INCLUDED__
#define _EMAILDB_INCLUDED__

/*- Record structure --------------------------------------------------------*/

typedef struct
{
    long   usid;
    long   uscustomerid;
    long   ususergroupid;
    long   usorderid;
    char   uscontext     [  50 + 1];
    long   ussender;
    char   usrecipients  [  50 + 1];
    char   ussubject     [ 150 + 1];
    char   usbody        [8000 + 1];
    short  usstatus;
    char   usmessage     [8000 + 1];
    double ussentat;
    double uscreatedts;
} USEMAILLOG;

typedef struct
{
    long   usid;
    double ussendat;
    long   usemaillogid;
    double usrevisedts;
} USEMAILQUEUE;

/*- Function declaration ----------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

Bool  emaildb_connect       (char *name, char *user, char *pwd);
void  emaildb_disconnect    (void);
char *emaildb_error_message (void);
int   emaillog_update       (USEMAILLOG   *record);
int   emaillog_get          (USEMAILLOG   *record);
long  emailqueue_get_all    (double timestamp, USEMAILQUEUE **record);
int   emailqueue_delete     (USEMAILQUEUE *record);


#ifdef __cplusplus
}
#endif

#endif
