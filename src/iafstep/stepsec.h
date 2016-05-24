/*  ----------------------------------------------------------------<Prolog>-
    Name:       stepsec.h
    Title:      Step Security Library

    Written:    2000/12/22  iMatix Corporation <info@imatix.com>
    Revised:    2002/01/22

    Synopsis:   Provide all routines in STEP for security and
                certificate.

    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/
#ifndef _STEPSEC_INCLUDED__
#define _STEPSEC_INCLUDED__

/*- Declarations ------------------------------------------------------------*/

typedef void * STEPKEY;

#define PRIVATE_KEY_PASSWORD   "axF9d_#&m*;fspEz$e(^qsf45e(7-g5s&s57sj"


/*- Function declaration ----------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void    initialise_crypt_library   (void);
void    terminate_crypt_library    (void);
STEPKEY create_private_certificate (char *file_name, int key_length,
                                    char *password);
STEPKEY create_public_certificate  (char *file_name, STEPKEY private_key, long days,
                                    char *issuer, long serial);
STEPKEY load_public_key            (char *file_name);
STEPKEY load_private_key           (char *file_name, char *password);
void    dump_public_key            (char *file_name);
Bool    free_key                   (STEPKEY key);

byte   *encrypt_buffer             (byte *buffer, long buffer_size, long *return_size,
                                    STEPKEY public_key, STEPKEY private_key);
byte   *decrypt_buffer             (byte *buffer, long buffer_size, long *return_size,
                                    STEPKEY public_key, STEPKEY private_key);
char   *encrypt_email_body         (char *body, STEPKEY pub_key, STEPKEY priv_key,
                                    char *sender);
char   *decrypt_email_body         (char *body, STEPKEY pub_key, STEPKEY priv_key);
char   *get_sender_in_body         (char *body);

char   *stepsec_error              (void);
int     set_random_key             (byte *buffer, int key_size);

#ifdef __cplusplus
}
#endif
#endif

