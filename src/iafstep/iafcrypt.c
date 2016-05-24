/*  ----------------------------------------------------------------<Prolog>-
    Name:       iafcrypt.c
    Title:      Crypt/decrypt file

    Written:    2002/01/22  iMatix Corporation <info@imatix.com>
    Revised:    2002/01/22

    Synopsis:   Crypt/decrypt file using private/public key

    This program is copyright (c) 1991-2002 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include "sfl.h"                        /*  SFL library header file          */
#include "stepsec.h"

/*- Definitions -------------------------------------------------------------*/

#define APPLICATION_NAME "iAFCrypt"
#define CUR_VERSION "1.0"

#define PROGRAM_NAME                                                          \
    APPLICATION_NAME " v" CUR_VERSION " (c) 1991-2002 iMatix\n"

#define CRYPT_FILE_PREFIX "sec_"
#define PREFIX_SIZE       4

#define BUFFER_SIZE       65535

#define RANDOM_KEY_SIZE   128

#define USAGE                                                                 \
    "Syntax: " APPLICATION_NAME " [options...] input_file_name\n"             \
    "Options:\n"                                                              \
    "  -pu certificate Certificate with public  key\n"                        \
    "  -pr certificate Certificate with private key\n"                        \
    "  -o filename     Output file name (default is " CRYPT_FILE_PREFIX       \
                       "+ input file name )\n"                                \
    "  -d              decrypt file (default is crypt)\n"                     \
    "  -h              Show summary of command-line options\n"                \
    "\nThe order of arguments is not important. Switches and filenames\n"     \
    "are case sensitive. See documentation for detailed information.\n"


#define FREE_RESOURCE                                                         \
    if (private_key) free_key (private_key);                                  \
    if (public_key)  free_key (public_key);                                   \
    if (f_input)     fclose   (f_input);                                      \
    if (f_output)    fclose   (f_output);                                     \
    if (buffer)      mem_free (buffer);                                       \
    terminate_crypt_library ();

/*---------------------------------------------------------------------------*/

int
main (int argc, char *argv [])
{
    int
        argn;                           /*  Argument number                  */
    Bool
        crypt         = TRUE,           /* Crypt of decrypt file             */
        args_ok       = TRUE;           /*  Were the arguments okay?         */
    char
        output_filename  [256],
        filename         [256],
        private_filename [256],
        public_filename  [256],
        *argparm;                       /*  Argument parameter to pick-up    */
	STEPKEY
        private_key   = NULL,
        public_key    = NULL;
    FILE
        *f_input  = NULL,
        *f_output = NULL;
    byte
        random_buffer [RANDOM_KEY_SIZE + 1],
        *buffer   = NULL,
        *r_buffer = NULL;
    long
        size,
        read_size,
        crypt_size;
    KEY_RC4
        key_rc4;

    clock_t
        start_time;

    if (argc == 1)
      {
        puts (PROGRAM_NAME);
        puts (USAGE);
        exit (EXIT_SUCCESS);
      }

    *filename         = '\0';
    *private_filename = '\0';
    *public_filename  = '\0';
    *output_filename  = '\0';

    argparm = NULL;                     /*  Argument parameter to pick-up    */
    for (argn = 1; argn < argc; argn++)
      {
        /*  If argparm is set, we have to collect an argument parameter      */
        if (argparm)
          {
            if (*argv [argn] != '-')    /*  Parameter can't start with '-'   */
              {
                strcpy (argparm, argv [argn]);
                argparm = NULL;
              }
            else
              {
                args_ok  = FALSE;
                break;
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
                        argparm = (char *)public_filename;
                    else
                        argparm = (char *)private_filename;
                    break;
                case 'o':
                    argparm = (char *)output_filename;
                    break;

                case 'd':
                    crypt = FALSE;
                    break;
                case 'h':
                    puts (PROGRAM_NAME);
                    puts (USAGE);
                    exit (EXIT_SUCCESS);
                    break;
                /*  Anything else is an error                                */
                default:
                    args_ok = FALSE;
              }
          }
        else
        if (argn == argc - 1)
            strcpy (filename, argv [argn]);
        else
          {
            args_ok = FALSE;
            break;
          }
      }
    if (argparm)
      {
        printf ("Argument missing - type '%s -h' for help\n", APPLICATION_NAME);
        exit (EXIT_FAILURE);
      }
    else
    if (!args_ok)
      {
        printf ("Invalid arguments - type '%s -h' for help", APPLICATION_NAME);
        exit (EXIT_FAILURE);
      }

    if (*filename == '\0')
      {
        printf ("Argument missing - Missing file name\ntype '%s -h' for help\n",
                APPLICATION_NAME);
        exit (EXIT_FAILURE);
      }

    if (*private_filename == '\0')
      {
        printf ("Argument missing - private certificate file name\ntype '%s -h' for help\n",
                APPLICATION_NAME);
        exit (EXIT_FAILURE);
      }

    if (*public_filename == '\0')
      {
        printf ("Argument missing - public certificate file name\ntype '%s -h' for help\n",
                APPLICATION_NAME);
        exit (EXIT_FAILURE);
      }

    if (*output_filename == '\0')
      {
        if (crypt)
            xstrcpy (output_filename, CRYPT_FILE_PREFIX, filename, NULL);
        else
          {
            if (lexncmp (filename, CRYPT_FILE_PREFIX, PREFIX_SIZE) == 0)
                strcpy (output_filename, filename + PREFIX_SIZE);
            else
                xstrcpy (output_filename, CRYPT_FILE_PREFIX, filename, NULL);

          }
      }
    /* Set default value                                                     */

    start_time = clock ();
    initialise_crypt_library ();


    /* Load keys                                                             */
    private_key = load_private_key (private_filename, PRIVATE_KEY_PASSWORD);
    if (private_key == NULL)
      {
        printf ("Error on loading private key %s\n", private_filename);
        FREE_RESOURCE;
        exit (EXIT_FAILURE);
      }
      
    public_key  = load_public_key  (public_filename);
    if (public_key == NULL)
      {
        printf ("Error on loading public key %s\n", public_filename);
        FREE_RESOURCE;
        exit (EXIT_FAILURE);
      }

    /* Open files                                                            */

    f_input = fopen (filename, "rb");
    if (f_input == NULL)
      {
        printf ("Error on open input file %s\n", filename);
        FREE_RESOURCE;
        exit (EXIT_FAILURE);
      }
     
    f_output = fopen (output_filename, "wb");
    if (f_output == NULL)
      {
        printf ("Error on open output file %s\n", output_filename);
        FREE_RESOURCE;
        exit (EXIT_FAILURE);
      }

    buffer = mem_alloc (BUFFER_SIZE + 1);
    if (buffer == NULL)
      {
        printf ("Error in memory allocation\n");
        FREE_RESOURCE;
        exit (EXIT_FAILURE);
      }


    printf ("%scrypt file %s to %s\n", crypt? "En": "De", filename, output_filename);
    if (crypt)
      {
        /* Save the key to decrypt                                           */
        set_random_key (random_buffer, RANDOM_KEY_SIZE);
        r_buffer = encrypt_buffer (random_buffer, RANDOM_KEY_SIZE, &crypt_size,
                                   public_key, private_key);
        if (r_buffer)
          {
            size = htonl (crypt_size);
            fwrite (&size,    1, 4, f_output);
            fwrite (r_buffer, 1, crypt_size, f_output);
            mem_free (r_buffer);
          }
        rc4_expand_key (&key_rc4, random_buffer, RANDOM_KEY_SIZE);

        /* Read and encrypt file                                             */
        read_size = fread (buffer, 1, BUFFER_SIZE, f_input);
        while (read_size > 0)
          {
            rc4_crypt (&key_rc4, buffer, (const word)read_size);
            size = htonl (read_size);
            fwrite (&size,  1, 4,         f_output);
            fwrite (buffer, 1, read_size, f_output);
            read_size = fread (buffer, 1, BUFFER_SIZE, f_input);
          }
      }
    else
      {
        /* read the key to decrypt                                           */
        read_size = fread (&size, 1, 4, f_input);
        if (read_size > 0)
          {
            read_size = ntohl (size);
            read_size = fread (buffer, 1, read_size, f_input);
            if (read_size)
                r_buffer = decrypt_buffer (buffer, read_size, &crypt_size,
                                           public_key, private_key);
            if (r_buffer)
              {
                memcpy (random_buffer, r_buffer, crypt_size);
                mem_free (r_buffer);
                r_buffer = NULL;
              }

            rc4_expand_key (&key_rc4, random_buffer, (const word)crypt_size);

            read_size = fread (&size, 1, 4, f_input);
            while (read_size > 0)
              {
                read_size = ntohl (size);
                read_size = fread (buffer, 1, read_size, f_input);
                if (read_size)
                    rc4_crypt (&key_rc4, buffer, (const word)read_size);
                fwrite (buffer, 1, read_size, f_output);
                read_size = fread (&size, 1, 4, f_input);
              }
         }
      }

    printf ("Time to execute: %.2f\n", ((double)(clock () - start_time)) / CLOCKS_PER_SEC);
    FREE_RESOURCE;
    mem_assert ();
    return (0);
}


