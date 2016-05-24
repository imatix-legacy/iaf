/*  ----------------------------------------------------------------<Prolog>-
    Name:       fcrypt.h
    Title:      Files Encryption and decryption functions
    Package:    iMatix Application Frameworf (iAF)

    Written:    2000/05/25  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/05/26

    Copyright:  Copyright (c) 1991-2000 iMatix

    Synopsis:   The encryption/decryption functions were based on routine
                found in some public news groups.
 ------------------------------------------------------------------</Prolog>-*/
#ifndef __FCRYPT_INCLUDED__
#define __FCRYPT_INCLUDED__

/*- Definitions -------------------------------------------------------------*/

#define CRYPT_TYPE_RC4    1
#define CRYPT_TYPE_DES    2
#define CRYPT_TYPE_3DES   3
#define CRYPT_TYPE_IDEA   4

/*- Function prototypes -----------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

Bool  file_encrypt (const char *file_source, const char *file_target,
                    const byte *key, short key_size, short crypt_type);

Bool  file_decrypt (const char *file_source, const char *file_target,
                    const byte *key, short key_size, short crypt_type);

Bool  buffer_encrypt (const byte *buffer, long size, const char *file_target,
                      const byte *key, short key_size, short crypt_type);

byte *buffer_decrypt (byte *buffer, long *size, const char *file_source,
                      const byte *key, short key_size, short crypt_type);

#ifdef __cplusplus
}
#endif

#endif