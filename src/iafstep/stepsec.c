/*  ----------------------------------------------------------------<Prolog>-
    Name:       stepsec.c
    Title:      Step Name Service Library

    Written:    2000/12/22  iMatix Corporation <info@imatix.com>
    Revised:    2001/09/28

    Synopsis:   Provide all routines in STEP for security and
                certificate.

    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include "sfl.h"                        /*  SFL library header file          */
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include "stepsec.h"

/*- Definitions -------------------------------------------------------------*/

#define MAGIC_STRING        "this is a magic string"
#define MAGIC_STRING_SIZE   22

#define BEGIN_MARK          "###BEGIN###"
#define BEGIN_MARK_SIZE     11

/*- Global variables --------------------------------------------------------*/

static const char 
    rnd_seed[] = "string to make the random number generator think it has entropy";
static Bool
    have_init  = FALSE;
static char
    error_buffer [512];

/*  ---------------------------------------------------------------------[<]-
    Function: initialise_sns_library 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
initialise_crypt_library (void)
{
    if (have_init == FALSE)
      {
        ERR_load_crypto_strings    ();
        OpenSSL_add_all_algorithms ();
        RAND_seed                  (rnd_seed, sizeof (rnd_seed));
        have_init = TRUE;
        *error_buffer = '\0';
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: terminate_crypt_library 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
terminate_crypt_library (void)
{
    EVP_PBE_cleanup ();
    have_init = FALSE;
}

/*  ---------------------------------------------------------------------[<]-
    Function: create_private_certificate 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

STEPKEY 
create_private_certificate (char *file_name, int key_length, char *password)
{
    STEPKEY
        feedback = NULL;
	EVP_PKEY
        *key          = NULL;
    RSA
        *rsa          = NULL;
    BIO
        *out          = NULL;
	EVP_CIPHER
        *cipher       = NULL;

    ASSERT (have_init);

    *error_buffer = '\0';
    cipher = EVP_des_ede3_cbc ();
    key = EVP_PKEY_new ();
    if (key != NULL)
      {
        rsa = RSA_generate_key (key_length, 0x10001, NULL, NULL);
        if (rsa)
          {
            if (EVP_PKEY_assign_RSA (key, rsa))
              {
                out = BIO_new(BIO_s_file());
                if (out)
                  {
                    if (BIO_write_filename (out, file_name) > 0)
                      {
                        if (PEM_write_bio_PrivateKey (out, key, cipher,  NULL, 0, NULL, password))
                                feedback = (STEPKEY)key;
                      }
                    BIO_free (out);
                  }
              }
          }
      }
    if (feedback == NULL)
      {
        if (key)
            EVP_PKEY_free (key);
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: create_public_certificate 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

STEPKEY 
create_public_certificate (char *file_name, STEPKEY private_key, long days, char *issuer,
                           long serial)
{
    STEPKEY
        feedback = NULL;
	EVP_PKEY
        *key          = NULL;
	EVP_CIPHER
        *cipher       = NULL;
	X509
        *x509         = NULL;
    EVP_MD
        *digest       = EVP_md5();
	X509_NAME
        *subject_name         = NULL;
    BIO
        *out   = NULL;

    ASSERT (have_init);

    key = (EVP_PKEY *)private_key;
    *error_buffer = '\0';

    if (key)
      {
        digest = EVP_dss1 ();
        x509 = X509_new();

        if (x509 != NULL)
         {
            /* Set version to V3                                             */
            X509_set_version (x509, 2);
            ASN1_INTEGER_set (X509_get_serialNumber (x509), serial);
            
            /* Set certificate validity delay                                */
            X509_gmtime_adj (X509_get_notBefore (x509), 0);
            X509_gmtime_adj (X509_get_notAfter  (x509),
                                            (long)(86400 * days));
            X509_set_pubkey (x509, key);

            subject_name = X509_get_subject_name (x509);
            X509_NAME_add_entry_by_txt(subject_name,"CN",
                                       MBSTRING_ASC, issuer, -1, -1, 0);

            X509_set_issuer_name (x509, subject_name);

            X509_sign (x509, key, digest);

            out = BIO_new(BIO_s_file());
            if (out)
              {
                 if (BIO_write_filename (out, file_name) > 0)
                   {
                     PEM_write_bio_X509 (out, x509);
                     feedback = (STEPKEY)X509_get_pubkey (x509);
                   }
                 BIO_free (out);
              }
            X509_free (x509);
         }
       }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: load_public_key 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

STEPKEY 
load_public_key (char *file_name)
{
    STEPKEY
        feedback = NULL;
    EVP_PKEY
        *key  = NULL;
    X509
        *x509 = NULL;
    BIO
        *in   = NULL;

    *error_buffer = '\0';
    in = BIO_new (BIO_s_file ());
    if (in)
      {
        if (BIO_read_filename (in, file_name) > 0)
          {
            x509 = PEM_read_bio_X509 (in, NULL, NULL, NULL);
            if (x509)
              {
                key = X509_get_pubkey (x509);
                if (key != NULL)
                    feedback = key;
                else
                    sprintf (error_buffer, "Load of public key %s fail: %s",
                              file_name,
                              ERR_error_string (ERR_get_error(), NULL));
                X509_free (x509);
              }
            else
                sprintf (error_buffer, "Load of public certificate %s fail: %s",
                          file_name,
                          ERR_error_string (ERR_get_error(), NULL));
          }
        else
            sprintf (error_buffer, "Imposible to load public certificate %s",
                      file_name);
        BIO_free (in);
      }

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: load_private_key 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

STEPKEY 
load_private_key (char *file_name, char *password)
{
    EVP_PKEY
        *key  = NULL;
    BIO
        *in   = NULL;
    STEPKEY
        feedback = NULL;

    *error_buffer = '\0';
    in = BIO_new (BIO_s_file ());
    if (in)
      {
        if (BIO_read_filename (in, file_name) > 0)
          {
            key = PEM_read_bio_PrivateKey (in, NULL, NULL, password);
            if (key != NULL)
                feedback = key;
            else
                sprintf (error_buffer, "Load of private key %s fail: %s",
                          file_name,
                          ERR_error_string (ERR_get_error(), NULL));
          }
        else
            sprintf (error_buffer, "Impossible to load private certificate %s",
                       file_name);
        BIO_free (in);
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: free_key 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

Bool   
free_key (STEPKEY key)
{
    Bool
        feedback = FALSE;

    *error_buffer = '\0';
    if (key)
      {
        EVP_PKEY_free ((EVP_PKEY *)key);
        feedback = TRUE;
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_key 

    Synopsis: Display content of public key
    ---------------------------------------------------------------------[>]-*/

void
dump_public_key (char *file_name)
{
    X509
        *x509 = NULL;
    BIO
        *out  = NULL,
        *in   = NULL;

    ASSERT (have_init);

    *error_buffer = '\0';
    in = BIO_new (BIO_s_file ());
    if (in)
      {
        if (BIO_read_filename (in, file_name) > 0)
          {
            x509 = PEM_read_bio_X509 (in, NULL, NULL, NULL);
            if (x509)
              {
                out = BIO_new_fp (stdout, BIO_NOCLOSE);
                X509_print (out, x509);
                BIO_free (out);
                X509_free  (x509);
              }
            else
                sprintf (error_buffer, "Load of public certificate %s fail: %s",
                          file_name,
                          ERR_error_string (ERR_get_error(), NULL));
          }
        else
            sprintf (error_buffer, "Imposible to load public certificate %s",
                      file_name);
        BIO_free (in);
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: encrypt_buffer 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

byte *
encrypt_buffer (byte *buffer, long buffer_size, long *return_size,
                STEPKEY public_key, STEPKEY private_key)
{
    byte
        magic_buffer [256],
        *source_block,
        *block,
        *feedback = NULL;
    int
        position,
        encrypt_size,
        source_size,
        block_size;
    long
        feedback_size;

    ASSERT (buffer);

    *error_buffer = '\0';
    if (buffer_size <= 0)
        buffer_size = strlen (buffer) + 1;

    block_size    = RSA_size (((EVP_PKEY *)public_key)-> pkey.rsa);
    feedback_size = ((int)(buffer_size / (block_size - 42)) + 2) * block_size;
    feedback      = mem_alloc (feedback_size + 1);

    if (feedback)
      {
        memset (feedback, 0, feedback_size);

        /* Save the magic string                                             */
        memset (magic_buffer, 0, sizeof (magic_buffer));
        /* Add hash of non crypted data                                      */
        sha1 (buffer, buffer_size, magic_buffer);
        feedback_size = SHA_DIGEST_SIZE;

        /* Add magic string                                                  */
        strcpy (&magic_buffer [feedback_size], MAGIC_STRING);
        feedback_size += MAGIC_STRING_SIZE + 1;

        /* Add timestamp value                                               */
        feedback_size += sprintf (&magic_buffer [feedback_size], "%f",
                                  gmtimestamp_now ());

        RSA_private_encrypt (feedback_size, magic_buffer, feedback,
                             ((EVP_PKEY *)private_key)-> pkey.rsa,
                             RSA_PKCS1_PADDING);

        block        = feedback + block_size;
        source_block = buffer;
        position     = 0;
        *return_size = block_size;
        while (position < buffer_size)
          {
            source_size = ((buffer_size - position) > block_size - 42)? block_size - 42: 
                                                                   buffer_size - position;
            encrypt_size = RSA_public_encrypt (source_size, source_block, block,
                                               ((EVP_PKEY *)public_key)-> pkey.rsa,
                                               RSA_PKCS1_OAEP_PADDING);
            if (encrypt_size < 0)
              {
                sprintf (error_buffer, "In encryption %s\n", ERR_error_string (ERR_get_error(), NULL));
                mem_free (feedback);
                feedback = NULL;
                break;
              }
            block        += encrypt_size;
            *return_size += encrypt_size;
            position     += source_size;
            source_block += source_size;
          }
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: decrypt_buffer 

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

byte *
decrypt_buffer (byte *buffer, long buffer_size, long *return_size,
                STEPKEY public_key, STEPKEY private_key)
{
    byte
        *hash,
        *source_block,
        *block,
        *feedback = NULL;
    int
        position,
        decrypt_size,
        source_size,
        block_size;
    byte
        magic_string [256];

    *error_buffer = '\0';
    block_size = RSA_size (((EVP_PKEY *)private_key)-> pkey.rsa);

    /* Check magic string                                                    */
    memset (magic_string, 0, sizeof (magic_string));
    decrypt_size = RSA_public_decrypt (block_size, buffer, magic_string,
                                       ((EVP_PKEY *)public_key)-> pkey.rsa,
                                       RSA_PKCS1_PADDING);
    if (decrypt_size < 0)
      {
        sprintf (error_buffer, "Invalid recipient public key in decrypt");
        return (NULL);
      }

    if (strcmp (&magic_string [SHA_DIGEST_SIZE], MAGIC_STRING) == 0)
      {
        feedback = mem_alloc (buffer_size - block_size + 1);
        if (feedback)
          {
            memset (feedback, 0, buffer_size - block_size + 1);
            block        = feedback;
            source_block = buffer + block_size;
            position     = block_size;
            *return_size = 0;
            while (position < buffer_size)
              {
                source_size = ((buffer_size - position) > block_size)?
                              block_size: buffer_size - position;
                decrypt_size = RSA_private_decrypt (source_size,
                                                    source_block,
                                                    block,
                                                    ((EVP_PKEY *)private_key)-> pkey.rsa,
                                                    RSA_PKCS1_OAEP_PADDING);
                block        += decrypt_size;
                *return_size += decrypt_size;
                position     += source_size;
                source_block += source_size;
              }

            /* Verify hash value                                             */
            hash = sha1 (feedback, *return_size, NULL);
            if (memcmp (hash, magic_string, SHA_DIGEST_SIZE) != 0)
              {
                sprintf (error_buffer, "Invalid hash value");
                mem_free (feedback);
                feedback = NULL;
                *return_size = 0;
              }
          }
      }
    else
        sprintf (error_buffer, "Invalid magic string in decrypt");
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: encrypt_email_body

    Synopsis: Encrypt body of email. the buffer returned must freed by mem_free
    function.
    ---------------------------------------------------------------------[>]-*/

char *
encrypt_email_body (char *body, STEPKEY pub_key, STEPKEY priv_key, char *sender)
{
    char
        size_buffer [255],
        *feedback = NULL;
    byte
        *encrypted_body = NULL;
    long
        line_size,
        position,
        feedback_size,
        return_size;

    encrypted_body = encrypt_buffer (body, 0, &return_size, pub_key, priv_key);
    if (encrypted_body)
      {
        feedback_size = (long)(return_size * 1.4);
        feedback_size = ((feedback_size / 70) + 2) * 72;
        feedback = mem_alloc (feedback_size + 12 + strlen (sender) + BEGIN_MARK_SIZE);
        if (feedback)
          {
            memset (feedback, 0, feedback_size);
            feedback_size = encode_base64 (encrypted_body, feedback, return_size);
            /* Cut message in line of 70 characters                          */
            position = 70;
            while (position < feedback_size)
              {
                memmove (&feedback [position + 2], &feedback [position],
                         feedback_size - position);
                feedback [position++] = '\r';
                feedback [position++] = '\n';
                feedback_size += 2;
                position      += 70;
              }
            sprintf (size_buffer, "%s%d|%s\n", BEGIN_MARK, feedback_size, sender);
            line_size = strlen (size_buffer);
            memmove (&feedback [line_size], feedback, feedback_size + 1);
            strncpy (feedback, size_buffer, line_size);
          }
        mem_free (encrypted_body);
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: decrypt_email

    Synopsis: Decrypt body of email. the buffer returned must freed by mem_free
    function.
    ---------------------------------------------------------------------[>]-*/

char *
decrypt_email_body (char *body, STEPKEY pub_key, STEPKEY priv_key)
{
    char
        *cur_pos,
        *new_pos,
        *end,
        *encoded_body,
        *body_begin,
        *feedback = NULL;
    long
        crypted_size,
        return_size,
        body_size;
    int
        nb_invalid;
    byte
        *crypted_buffer;
#define IS_VALIDCHAR(c)   ((c >= 'A' && c <= 'Z') \
                        || (c >= 'a' && c <= 'z') \
                        || (c >= '0' && c <= '9') \
                        || (c == '+' || c == '/' || c == '='))

    cur_pos = strstr (body, BEGIN_MARK);
    if (cur_pos)
      {
        cur_pos += BEGIN_MARK_SIZE;
        encoded_body = mem_strdup (cur_pos); 
      }
    else
        encoded_body = mem_strdup (body);

    if (encoded_body)
      {
        body_begin    = strchr (encoded_body, '\n');
        *body_begin++ = '\0';
        while (body_begin 
           && !IS_VALIDCHAR (*body_begin))
            body_begin++;

        body_size = atol (encoded_body);
        if (body_size == 0)
          {
            mem_free (encoded_body);
            return (feedback);
          }
        /* Count invalid characters                                          */
        nb_invalid = 0;
        cur_pos = body_begin;
        while (cur_pos && *cur_pos)
          {
            if ( *cur_pos != '\r'
              && *cur_pos != '\n'
              && !IS_VALIDCHAR (*cur_pos) )
                nb_invalid++;
            cur_pos++;
          }
        body_size += nb_invalid;
        *(body_begin + body_size) = '\0';

        /* Remove CR LR from message body                               */
        end      = body_begin + body_size + 1;
        cur_pos  = body_begin;
        new_pos  = body_begin;
        while (cur_pos < end)
         {
           if (*cur_pos == '\r'
           ||  *cur_pos == '\n'
           ||  !IS_VALIDCHAR (*cur_pos))
               cur_pos++;
           else  
               *new_pos++ = *cur_pos++;
         }
        *new_pos = '\0';
        body_size = strlen (body_begin);
        crypted_buffer = mem_alloc (body_size);
        if (crypted_buffer)
          {
            crypted_size = decode_base64  (body_begin,   crypted_buffer,
                                           body_size);
            feedback     = decrypt_buffer (crypted_buffer, crypted_size,
                                           &return_size,   pub_key, priv_key);
            mem_free (crypted_buffer);
          }
        mem_free (encoded_body);
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_sender_in_body

    Synopsis: find the sender value stored in first line of body.
    ---------------------------------------------------------------------[>]-*/

char *
get_sender_in_body (char *body)
{
    char
        *feedback = NULL,
        old_value,
        *begin,
        *end;

    begin = strstr (body, BEGIN_MARK);
    if (begin)
        begin = strchr (begin, '|');
    if (begin)
      {
        begin++;
        end = strchr (begin, '\r');
        if (end == NULL)
            end = strchr (begin, '\n');
        if (end)
          {
            old_value = *end;
            *end      = '\0';
            feedback = mem_strdup (begin);
            *end      = old_value;
            if (feedback)
                strcrop (feedback);
          }
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: stepsec_error

    Synopsis: return the last error description
    ---------------------------------------------------------------------[>]-*/

char *
stepsec_error (void)
{
    return (error_buffer);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_random_key

    Synopsis: generate a random key value. return the size of the key.
    ---------------------------------------------------------------------[>]-*/

int 
set_random_key (byte *buffer, int key_size)
{
    return (RAND_bytes (buffer, key_size));
}
