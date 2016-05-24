/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafcert.c
    Title:      Secure certificate generator

    Written:    2000/11/30  iMatix Corporation <info@imatix.com>
    Revised:    2001/09/25

    Synopsis:   Generate Certificate for PKI (Public Key infrastructure)

    This program is copyright (c) 1991-2000 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include "sfl.h"                        /*  SFL library header file          */
#include "stepsec.h"

/*- Definitions -------------------------------------------------------------*/

#define APPLICATION_NAME "iAFCert"
#define CUR_VERSION "1.0b"

#define PROGRAM_NAME                                                          \
    APPLICATION_NAME " v" CUR_VERSION " (c) 1991-2000 iMatix\n"


#define USAGE                                                                 \
    "Syntax: " APPLICATION_NAME " [options...]\n"                             \
    "Options:\n"                                                              \
    "  -pu [private_cert] Certificate with public key\n"                      \
    "  -pr                Certificate with private key\n"                     \
    "  -all               Generate public and private key\n"                  \
    "  -o filename        Output file name (default is 'certificate')\n"      \
    "                     Add '_priv.cert' for private certificate\n"         \
    "                     and '_pub.cert'  for public certificate\n"          \
    "  -d                 dump public key certificate (name specified by -o)\n"\
    "  -h                 Show summary of command-line options\n"             \
    "\nThe order of arguments is not important. Switches and filenames\n"     \
    "are case sensitive. See documentation for detailed information.\n"

#define PRIVATE_KEY_LENGTH           1024
#define CERTIFICATE_ISSUER_NAME      "iMatix Corporation"
#define CERTIFICATE_DAYS             365 * 2 /* Set days to 2 years          */
#define CERTIFICATE_SERIAL           20001204

/*- Local function declaration ----------------------------------------------*/

void  test_rsa_crypt (char *cert_source, char *cert_target);

/*---------------------------------------------------------------------------*/

int
main (int argc, char *argv [])
{
    int
        argn;                           /*  Argument number                  */
    Bool
        dump          = FALSE,
        testcrypt     = FALSE,
        args_ok       = TRUE,           /*  Were the arguments okay?         */
        private_cert  = FALSE,          /*  Generate private certificate?    */
        public_cert   = FALSE;          /*  Public certificate               */
    char
        *private_certname,
        *filename     = NULL,
        *private_name = NULL,
        **argparm,                      /*  Argument parameter to pick-up    */
        private_filename [256],
        public_filename  [256];
	STEPKEY
        private_key   = NULL,
        public_key    = NULL;


    if (argc == 1)
      {
        puts (PROGRAM_NAME);
        puts (USAGE);
        exit (EXIT_SUCCESS);
      }

    argparm = NULL;                     /*  Argument parameter to pick-up    */
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
                if (*argparm == private_name)
                  {
                    *argparm = mem_strdup ("certificate_priv.cert");
                    argparm  = NULL;
                    argn--;
                  }
                else
                  {
                    args_ok  = FALSE;
                    break;
                  }
              }
          }
        else
        if (*argv [argn] == '-')
          {
            switch (argv [argn][1])
              {
                /*  These switches take a parameter                          */
                case 'p':
                    if (argv [argn][2] ==  'u')
                      {
                        public_cert  = TRUE;
                        argparm = &private_name;
                      }
                    else
                        private_cert  = TRUE;
                    break;
                case 'o':
                    argparm = &filename;         break;
                    break;

                /*  These switches have an immediate effect                  */
                case 'a':
                    private_cert = TRUE;
                    public_cert  = TRUE;
                    break;
                case 'd':
                    dump = TRUE;
                    break;
                case 'h':
                    puts (PROGRAM_NAME);
                    puts (USAGE);
                    exit (EXIT_SUCCESS);
                    break;
                case 't':
                    testcrypt = TRUE;
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
    if (argparm)
      {
        puts ("Argument missing - type 'iafcert -h' for help");
        if (filename)
            mem_strfree (&filename);
        if (private_name)
            mem_strfree (&private_name);
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        puts ("Invalid arguments - type 'iafcert -h' for help");
        if (filename)
            mem_strfree (&filename);
        if (private_name)
            mem_strfree (&private_name);
        exit (EXIT_FAILURE);
      }

    /* Set default value                                                     */
    if (filename == NULL)
        filename = mem_strdup ("certificate");
    if (public_cert == FALSE && private_cert == FALSE)
        public_cert = TRUE;

    initialise_crypt_library ();

    xstrcpy (private_filename, filename, "_priv.cert", NULL);
    xstrcpy (public_filename,  filename, "_pub.cert",  NULL);

    if (testcrypt)
      {
        private_cert = FALSE;
        public_cert  = FALSE;
        test_rsa_crypt ("source", "target");
      }
 
    if (dump)
      {
        private_cert = FALSE;
        public_cert  = FALSE;
        dump_public_key (public_filename);
      }
    if (private_cert)
      {
        private_key = create_private_certificate (private_filename, PRIVATE_KEY_LENGTH,
                                          PRIVATE_KEY_PASSWORD);
        if (private_key)
            printf ("Private certificate %s has been created with a key length of %d\n",
                     private_filename, PRIVATE_KEY_LENGTH);
        else
            printf ("Private key generation error: %s\n", stepsec_error ());
      }

    if (public_cert)
      {
        private_certname = private_name? private_name: private_filename;
        /* Load private key if required                                      */
        if (private_key == NULL)
            private_key = load_private_key (private_certname, PRIVATE_KEY_PASSWORD); 
        if (private_key)
          {
             public_key = create_public_certificate (public_filename,
                                                     private_key,
                                                     CERTIFICATE_DAYS,
                                                     CERTIFICATE_ISSUER_NAME,
                                                     CERTIFICATE_SERIAL);
             if (public_key)
                 printf ("Public certificate %s has been created\n",
                         public_filename);
         }
       else
           printf ("Error when read private certificate file %s\n", private_certname);
      }

    if (private_key)
        free_key (private_key);
    if (public_key)
        free_key (public_key);

    terminate_crypt_library ();

    if (filename)
        mem_strfree (&filename);
    if (private_name)
        mem_strfree (&private_name);
    mem_assert ();
    return (0);
}


/*---------------------------------------------------------------------------
  Function: test_rsa_crypt

  Synopsis: Test encryption/decryption with private and public certificate with
            RSA algorithme.
  ---------------------------------------------------------------------------*/

void 
test_rsa_crypt (char *cert_source, char *cert_target)
{
    char
        source_priv [100],
        source_pub  [100],
        target_priv [100],
        target_pub  [100];
	STEPKEY
        source_priv_key  = NULL,
        source_pub_key   = NULL,
        target_priv_key  = NULL,
        target_pub_key   = NULL;
    byte
        *decrypted        = NULL,
        *encrypted        = NULL;
    char
        *buffer = "This is a test message to test public key system, with RSA algotithm" \
                  "with 'magic string'system: the encrypted string contain a string encrypted" \
                  "with the private key of the sender. When you decript with the public key of the" \
                  " sender, you can check the magic string. simple but efficient :-)" ;
    long
        return_size;

    xstrcpy (source_priv, cert_source, "_priv.cert", NULL);
    xstrcpy (source_pub,  cert_source, "_pub.cert",  NULL);
    xstrcpy (target_priv, cert_target, "_priv.cert", NULL);
    xstrcpy (target_pub,  cert_target, "_pub.cert",  NULL);

    
    source_priv_key = load_private_key (source_priv, PRIVATE_KEY_PASSWORD);
    source_pub_key  = load_public_key  (source_pub);
    target_priv_key = load_private_key (target_priv, PRIVATE_KEY_PASSWORD);
    target_pub_key  = load_public_key  (target_pub);
        
    encrypted = encrypt_buffer ((byte *)buffer, 0, &return_size,
                                 target_pub_key, source_priv_key);

    if (encrypted)
        decrypted = decrypt_buffer (encrypted, return_size, &return_size,
                                     source_pub_key, target_priv_key);

    if (decrypted && strcmp (decrypted, buffer) == 0)
        printf ("Encryption test ok\n");

    /* Free allocated resource                                               */
    if (encrypted)
        mem_free (encrypted);
    if (decrypted)
        mem_free (decrypted);
    if (source_priv_key)
        free_key (source_priv_key);
    if (source_pub_key)
        free_key (source_pub_key);
    if (target_priv_key)
        free_key (target_priv_key);
    if (target_pub_key)
        free_key (target_pub_key);
}

