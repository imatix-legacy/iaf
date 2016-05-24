/*  ----------------------------------------------------------------<Prolog>-
    Name:       fcrypt.c
    Title:      Files Encryption and decryption functions
    Package:    iMatix Application Frameworf (iAF)

    Written:    2000/05/25  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/05/26

    Copyright:  Copyright (c) 1991-2000 iMatix

    Synopsis:   The encryption/decryption functions were based on routine
                found in some public news groups.
 ------------------------------------------------------------------</Prolog>-*/
#include "sfl.h"
// #include "sflrc4.h"
// #include "sfldes.h"
// #include "sflidea.h"
// #include "sflsha.h"
#include "fcrypt.h"

#define BLOCK_SIZE 1024

#define ROUND8(val, new) {           \
    if ((val / 8) * 8 != val)        \
        new =  ((val / 8) + 1) * 8;  \
    else                             \
        new = val;                   \
}

/*  ---------------------------------------------------------------------[<]-
    Function: file_encrypt

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

Bool
file_encrypt (const char *file_source, const char *file_target,
              const byte *key, short key_size, short crypt_type)
{
    byte
        *buffer = NULL;
    long
        size;
    Bool
        feedback = FALSE;
    FILE
        *f_source = NULL;

    f_source = fopen (file_source, "rb");
    if (f_source)
      {  
        fseek (f_source, 0L, SEEK_END);
        size = ftell (f_source);
        fseek (f_source, 0L, SEEK_SET);
        buffer = (byte *)mem_alloc (size);
        if (buffer)
          {
            fread (buffer, 1, size, f_source);
            fclose (f_source);
            f_source = NULL;
            feedback = buffer_encrypt (buffer, size, file_target, key, key_size,
                                       crypt_type);
            mem_free (buffer);
          }
      }

    if (f_source)
        fclose (f_source);

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: file_decrypt

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

Bool
file_decrypt (const char *file_source, const char *file_target,
              const byte *key, short key_size, short crypt_type)
{
    byte
        *buffer;
    long
        size = 0;
    Bool
        feedback = FALSE;
    FILE
        *file;

    buffer = buffer_decrypt (NULL, &size, file_source, key, key_size, crypt_type);
    if (buffer && size > 0)
      {
         file = fopen (file_target, "wb");
         if (file)
           {
             if (fwrite (buffer, 1, size, file) > 0)
                 feedback = TRUE;
             fclose (file);
           }
         mem_free (buffer);
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: buffer_encrypt

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

Bool
buffer_encrypt (const byte *buffer, long size, const char *file_target,
                const byte *key, short key_size, short crypt_type)
{
    KEY_RC4
        key_rc4;
    des_keys
        key_des1,
        key_des2,
        key_des3;
    idea_key_schedule
        key_idea;
    Bool
        feedback = FALSE;
    byte
        *sha_key;
    short
        end_size,
        new_size;
    byte
        *des_data,
        *des_end,
        *end,
        *data,
        *block = NULL;
    FILE
        *f_target;

    f_target = fopen (file_target, "wb");
    if (f_target)
      {
        /* Set Crypt Key value                                               */
        sha_key = sha1 (key, key_size, NULL);
        switch (crypt_type)
          {
            case CRYPT_TYPE_DES:
                des_key ((des_cblock *)sha_key, &key_des1);
                break;
            case CRYPT_TYPE_3DES:
                des_key ((des_cblock *)sha_key,       &key_des1);
                des_key ((des_cblock *)&sha_key [8],  &key_des2);
                des_key ((des_cblock *)&sha_key [12], &key_des3);
                break;
            case CRYPT_TYPE_IDEA:
                memset (&key_idea, 0, sizeof (key_idea));
                set_encrypt_key_idea (sha_key, &key_idea);
                break;
            default:                        /* Default is RC4                */
                rc4_expand_key (&key_rc4, sha_key, SHA_DIGEST_SIZE);
                break;
          }
        block = (byte *)mem_alloc (BLOCK_SIZE + 10);
        if (block)
          {
            data = (byte *)buffer;
            end  = (byte *)buffer + size;

            feedback = TRUE;
            /* Write uncrompressed and decrypted file size                   */
            fwrite (&size, sizeof (long), 1, f_target);
            while (data <= end)
              {
                /* Compress data                                             */
                memset (block, 0, BLOCK_SIZE + 10);
                new_size = ((end - data) > BLOCK_SIZE)? BLOCK_SIZE: end - data;
                new_size = compress_block (data, block, (word)new_size);

                switch (crypt_type)
                  {
                    case CRYPT_TYPE_DES:
                        des_data = block;
                        ROUND8 (new_size, end_size);
                        des_end  = block + end_size;
                        while (des_data < des_end)
                          {
                            crypt_des ((des_cblock *)des_data,
                                       (des_cblock *)des_data,
                                        &key_des1, TRUE);
                            des_data += 8;
                          }
                        break;
                    case CRYPT_TYPE_3DES:
                        des_data = block;
                        ROUND8 (new_size, end_size);
                        des_end  = block + end_size;
                        while (des_data < des_end)
                          {
                            crypt_des3 ((des_cblock *)des_data,
                                        (des_cblock *)des_data,
                                         &key_des1, &key_des2,
                                         &key_des3, TRUE);
                            des_data += 8;
                          }
                        break;
                    case CRYPT_TYPE_IDEA:
                        des_data = block;
                        ROUND8 (new_size, end_size);
                        des_end  = block + end_size;
                        while (des_data < des_end)
                          {
                            crypt_idea_ecb (des_data, des_data, &key_idea);
                            des_data += 8;
                          }
                        break;
                    default:            /* Default is RC4                    */
                        rc4_crypt (&key_rc4, block, (const word)new_size);
                        break;
                  }
                /* Write Block size                                          */
                if (crypt_type == CRYPT_TYPE_DES
                ||  crypt_type == CRYPT_TYPE_3DES
                ||  crypt_type == CRYPT_TYPE_IDEA)
                  {
                    fwrite (&end_size, sizeof (short), 1, f_target);
                    fwrite (&new_size, sizeof (short), 1, f_target);
                    fwrite (block, 1,  end_size, f_target);
                  }
                else
                  {
                    /* Write block data                                      */
                    fwrite (&new_size, sizeof (short), 1, f_target);
                    fwrite (block, 1, new_size, f_target);
                  }
                data += BLOCK_SIZE;
              }
            mem_free (block);
          }      
        fclose (f_target);
      }
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: buffer_decrypt

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

byte *
buffer_decrypt (byte *buf, long *size, const char *file_source,
                const byte *key, short key_size, short crypt_type)
{
    KEY_RC4
        key_rc4;
    des_keys
        key_des1,
        key_des2,
        key_des3;
    idea_key_schedule
        key_idea;
    FILE
        *f_source;
    byte
        *sha_key,
        *des_data,
        *des_end,
        *block,
        *p_buf,
        *buffer = NULL;
    short
        end_size,
        block_size,
        new_size;
    long
        file_size;

    buffer = buf;

    f_source = fopen (file_source, "rb");
    if (f_source)
      {
        fread (&file_size, sizeof (long), 1, f_source);
        if (buffer == NULL)
            buffer = (byte *)mem_alloc (file_size);
        *size = file_size;

        if (buffer)
          {
            /* Set decrypt Key value                                         */
            sha_key = sha1 (key, key_size, NULL);
            switch (crypt_type)
              {
                case CRYPT_TYPE_DES:
                    des_key ((des_cblock *)sha_key, &key_des1);
                    break;
                case CRYPT_TYPE_3DES:
                    des_key ((des_cblock *)sha_key,       &key_des1);
                    des_key ((des_cblock *)&sha_key [8],  &key_des2);
                    des_key ((des_cblock *)&sha_key [12], &key_des3);
                    break;
                case CRYPT_TYPE_IDEA:
                    memset (&key_idea, 0, sizeof (key_idea));
                    set_decrypt_key_idea (sha_key, &key_idea);
                    break;
                default:                /* Default is RC4                    */
                    rc4_expand_key (&key_rc4, sha_key, SHA_DIGEST_SIZE);
                    break;
              }
            block = (byte *)mem_alloc (BLOCK_SIZE * 2);
            if (block)
              {
                p_buf = buffer;
                while (fread (&block_size, sizeof (short), 1, f_source) > 0)
                  {
                    memset (block, 0, BLOCK_SIZE);

                    if (crypt_type == CRYPT_TYPE_DES
                    ||  crypt_type == CRYPT_TYPE_3DES
                    ||  crypt_type == CRYPT_TYPE_IDEA)
                        fread (&end_size, sizeof (short), 1, f_source);
                    else
                        end_size = block_size;

                    fread (block, 1, block_size, f_source);

                    switch (crypt_type)
                      {
                        case CRYPT_TYPE_DES:
                            des_data = block;
                            des_end  = block + block_size;
                            while (des_data <= des_end)
                              {
                                crypt_des ((des_cblock *)des_data,
                                           (des_cblock *)des_data,
                                            &key_des1, FALSE);
                                des_data += 8;
                              }
                            break;
                        case CRYPT_TYPE_3DES:
                            des_data = block;
                            des_end  = block + block_size;
                            while (des_data <= des_end)
                              {
                                crypt_des3 ((des_cblock *)des_data,
                                            (des_cblock *)des_data,
                                             &key_des1, &key_des2,
                                             &key_des3, FALSE);
                                des_data += 8;
                              }
                            break;
                        case CRYPT_TYPE_IDEA:
                            des_data = block;
                            des_end  = block + block_size;
                            while (des_data <= des_end)
                              {
                                crypt_idea_ecb (des_data, des_data, &key_idea);
                                des_data += 8;
                              }
                            break;
                        default:            /* Default is RC4                    */
                            rc4_crypt (&key_rc4, block, (const word)block_size);
                            break;
                      }
                    new_size = expand_block (block, p_buf, end_size);
                    p_buf += new_size;
                  }
                mem_free (block);
              }
          }
        fclose (f_source);
      }       
    return (buffer);
}
