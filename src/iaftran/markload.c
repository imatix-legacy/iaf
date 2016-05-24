/*  ----------------------------------------------------------------<Prolog>-
    Name:       markload.c
    Title:      Markup Load file Function
    Package:    Markup Translator

    Written:    1999/11/11 Pascal Antonnaux <pascal@imatix.com>
    Revised:    2002/05/16 Pascal Antonnaux <pascal@imatix.com>

    Synopsis:   Provides functions to read and write Markup files, and manipulate
                Markup data in memory as list structures.
    Copyright:  Copyright (c) 1991-99 iMatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"
#include "markcfg.h"
#include "marklib.h"
#include "markload.d"                   /*  Include dialog data              */

#define STACK_SIZE   20

/*- Function prototypes -----------------------------------------------------*/

/*- Global variables used in this source file only --------------------------*/

static char
    *end_token     [STACK_SIZE + 1],    /*  End token                        */
    *file_name,                         /*  Current file name                */
    error_buf      [256],               /*  Error buffer                     */
    *buffer     = NULL;                 /*  Working buffer                   */
static int
    stack_pos,
    end_token_size [STACK_SIZE + 1],    /*  End Token size                   */
    tagid          [STACK_SIZE + 1],    /*  Id of current tag                */
    nbr_tag,                            /*  Number of TAG in tag table       */
    read_size,                          /*  Size of data in buffer           */
    pos,                                /*  Current position in buffer       */
    script_data_pos,                    /*  Position of script data          */
    end_script_data_pos,                /*  Position of script data          */
    saved_tag_pos,                      /*  Saved Tag position               */
    saved_pos;                          /*  Saved position                   */
static FILE
    *file;                              /*  File pointer                     */
static MARKUP_TAG
    *feedback,                          /*  Feedback for calling program     */
    *last_tag,                          /*  Last tag inserted                */
    *root_tag,                          /*  Root tag                         */
    *current_tag;                       /*  Current Tag                      */
static SYMTAB
    *sym_tag = NULL;                    /*  Symbol table of tag              */
static MARKUP_DEF
    *tag_table = NULL;                  /*  Table of tag                     */
static short
    tag_script    [STACK_SIZE + 1];     /*  Indicate if a scripting tag      */
static long
    buffer_size = 0;                    /*  Current buffer size              */

void remove_string_tag_begin (void);
void remove_string_tag_end   (void);
void load_tag_table          (MARKUP_DEF *tab, int nbr);


void
initialise_markup_resource (MARKUP_DEF *tag_tab, int nb_tag)
{
    static Bool
        have_init = FALSE;

    if (have_init == FALSE)
      {
        load_tag_table (tag_tab, nb_tag);
        have_init = TRUE;
      }
}

void
free_markup_resource (void)
{
    if (sym_tag)
      {
        sym_delete_table (sym_tag);
        sym_tag = NULL;
      }
}

/********************************   M A I N   ********************************/

MARKUP_TAG  *
markup_load (char       *fname,
             MARKUP_DEF *tag_tab,
             int         nb_tag)
{
    file_name = fname;
    tag_table = tag_tab;
    nbr_tag   = nb_tag;
#   include "markload.i"                /*  Include dialog interpreter       */
}

void
load_tag_table (MARKUP_DEF *tab, int nbr)
{
    int
        index;
    SYMBOL
        *symbol;

    ASSERT (tab);

    if (sym_tag)
      {
        sym_delete_table (sym_tag);
        sym_tag = NULL;
      }
    sym_tag = sym_create_table ();
    for (index = 0; index < nbr; index++)
      {
        symbol = sym_assume_symbol (sym_tag, tab [index].tag, "");
        if (symbol)
            symbol-> data = &tab [index];
      }

}


/*************************   INITIALISE THE PROGRAM   ************************/

MODULE initialise_the_program (void)
{
    if (file_name == NULL)
        the_next_event = error_event;
    else
      {
        current_tag    = root_tag;
        file           = NULL;
        pos            = 0;
        saved_pos      = 0;
        read_size      = 0;
        stack_pos      = 0;
        root_tag       = NULL;
        the_next_event = ok_event;
        buffer         = NULL;
      }
}


/***************************   GET EXTERNAL EVENT   **************************/

MODULE get_external_event (void)
{
}


/************************   CHECK IF NEED END OF TAG   ***********************/

MODULE check_if_need_end_of_tag (void)
{
    if (tagid [stack_pos + 1] != -1
    &&  tag_table [tagid [stack_pos + 1]].end_tag == TRUE)
        current_tag = last_tag;
}


/***************************   CLOSE MARKUP FILE   ***************************/

MODULE close_markup_file (void)
{
    if (file)
      {
        fclose (file);
        file = NULL;
      }
}


/****************************   CREATE NEW DATA   ****************************/

MODULE create_new_data (void)
{
    char
        old_val;

    old_val      = buffer [pos];
    buffer [pos] = '\0';

    tag_put_data (current_tag, &buffer [saved_pos], tag_script [stack_pos]);

    buffer [pos] = old_val;
}


/*****************************   CREATE NEW TAG   ****************************/

MODULE create_new_tag (void)
{
    char
        old_val;

    old_val      = buffer [pos];
    buffer [pos] = '\0';
    last_tag = tag_new (current_tag, tagid [stack_pos + 1], ++current_tag-> nb_items,
                        &buffer [saved_pos]);
    if (last_tag)
        tag_attach (current_tag, last_tag);
    buffer [pos] = old_val;
}


/***************************   GET CONTENT TOKEN   ***************************/

MODULE get_content_token (void)
{

    if (pos >= read_size)
      {
        if (read_size < buffer_size)
            the_next_event = end_of_file_event;
        else
            the_next_event = load_buffer_event;
        return;
      }

    saved_pos = pos;
    if (buffer [pos] == '<')
      {
        if (buffer [pos + 1] == '/')
            the_next_event = end_tag_event;
        else
            the_next_event = open_tag_event;
      }
    else
      {
        if (current_tag == root_tag)
            the_next_event = unknow_data_event;
        else
            the_next_event = data_event;
      }
    pos++;

}


/****************************   GO TO END OF TAG   ***************************/

MODULE go_to_end_of_tag (void)
{
    char
        end_char;

    if (end_token [stack_pos] == NULL)
      {
        end_token [stack_pos] = ">";
        end_token_size [stack_pos] = 1;
      }

    end_char = end_token [stack_pos] [end_token_size [stack_pos] - 1];
    while (buffer [pos] != '\0' && read_size > 0)
      {
        if (buffer [pos] == end_char)
          {
            if (lexncmp (&buffer [pos - end_token_size [stack_pos] + 1],
                         end_token [stack_pos],
                         end_token_size [stack_pos]) == 0)
              {
                pos++;
                the_next_event = end_of_tag_event;
                break;
              }
          }
        else
        if (buffer [pos] == '<')
          {
            pos++;
            the_next_event  = open_tag_event;
            break;
          }
        pos++;
        if (pos >= read_size)
            load_buffer ();
        if (read_size == 0)
          {
            raise_exception (end_of_file_event);
            break;
          }
      }
}


/****************************   GOTO END OF DATA   ***************************/

MODULE goto_end_of_data (void)
{
    int
        last_char;

    while (buffer [pos] != '<')
      {
        pos++;
        if (pos >= read_size)
          {
            load_buffer ();
            if (read_size == 0)
              {
                raise_exception (end_of_file_event);
                return;
              }
          }
      }

    /*  'pos' points to start of next tag in buffer.  We now scan backwards
     *  to search for the end of the data string, by skipping whitespace and
     *  the '&nbsp;' token if present.  We leave punctuation and numbers in
     *  the data.
     */
    last_char = pos - 1;
    while (last_char > saved_pos)
      {
        if (buffer [last_char] == ';')
          {
            if (last_char > 6
            &&  lexncmp (buffer + last_char - 5, "&nbsp;", 6) == 0)
                last_char -= 6;
            else
            if (last_char > 4
            &&  lexncmp (buffer + last_char - 3, "&lt;", 4) == 0)
                last_char -= 4;
            else
            if (last_char > 4
            &&  lexncmp (buffer + last_char - 3, "&gt;", 4) == 0)
                last_char -= 4;
            else
                break;
          }
        else
        if (buffer [last_char] <= ' ')  /*  Spaces and all control chars     */
            last_char--;
        else
        if (IS_NOT_TRANSLATED (buffer [last_char]))
            last_char--;
        else
            break;                      /*  We hit something valid           */
    }

    /*  If the entire tag is skipped, take it as it was                       */
    if (last_char > saved_pos)
        pos = last_char + 1;
}


/**************************   INITIALISE TAG TREE   **************************/

MODULE initialise_tag_tree (void)
{
    root_tag = tag_new (NULL, 0, 0, "");
    if (root_tag == NULL)
        raise_exception (error_event);
    else
        current_tag = root_tag;
    feedback = root_tag;
}


/***************************   MOVE TO PARENT TAG   **************************/

MODULE move_to_parent_tag (void)
{
    if (current_tag != root_tag)
        current_tag = current_tag-> parent;
    else
        current_tag = root_tag;
}


/****************************   OPEN MARKUP FILE   ***************************/

MODULE open_markup_file (void)
{
    file = fopen (file_name, "rb");
    if (file == NULL)
      {
        sprintf (error_buf, "Error: I can't open file %s", file_name);
        raise_exception (error_event);
        return;
      }

    /* Get file size                                                         */
    fseek (file, 0, SEEK_END);
    buffer_size = ftell (file);
    fseek (file, 0, SEEK_SET);

    buffer = mem_alloc (buffer_size + 1);
    if (buffer)
      {
        memset (buffer, 0, buffer_size + 1);
        read_size = fread (buffer, 1, buffer_size, file);
        if (read_size == 0)
            raise_exception (end_of_file_event);
        else
          {
            while (buffer [pos] != '<' && pos < read_size)
                pos++;     
          }
      }
    else
      {
        sprintf (error_buf, "Error: I can't alloc buffer with size %ld",
                             buffer_size);
        raise_exception (error_event);
      }
}


/******************************   SIGNAL ERROR   *****************************/

MODULE signal_error (void)
{
    coprintf ("%s\n", error_buf);
}


/******************************   LOAD BUFFER   ******************************/

MODULE load_buffer (void)
{
    read_size = 0;
/*    memset (buffer, 0, buffer_size + 1);
    pos       = 0;
    saved_pos = 0;*/
}


/*************************   TERMINATE THE PROGRAM    ************************/

MODULE terminate_the_program (void)
{
    if (file)
      {
        fclose (file);
        file  = NULL;
      }
    if (buffer)
      {
         mem_free (buffer);
         buffer = NULL;
      }
    feedback  = root_tag;               /*  No errors so far                 */
    the_next_event = terminate_event;
}

/***************************   GET TAG ATTRIBUTE   ***************************/

MODULE get_tag_attribute (void)
{
    if (end_token [stack_pos] == NULL)
        the_next_event = unknow_tag_event;
    else
    if (tag_script [stack_pos] > 0)
        the_next_event = script_event;
    else
    if (stack_pos > 1)
        the_next_event = invalid_tag_event;
    else
        the_next_event = normal_event;

}


/*****************************   GET TAG VALUE   *****************************/

MODULE get_tag_value (void)
{
    char
         tmp [50],
         tag [250];
    int
         size,
         begin;
    SYMBOL
         *symbol;

    if (pos >= read_size)
        load_buffer ();

    if (read_size == 0)
      {
        raise_exception (end_of_file_event);
        return;
      }
    if (buffer [pos] == '/')
        pos++;

    begin         = pos;
    size          = 0;
    saved_tag_pos = pos;

    while (buffer [pos] != '>'
    &&     buffer [pos] != ' '
    &&     buffer [pos] != '@'
    &&     buffer [pos] != '\r'
    &&     buffer [pos] != '\n')
      {
        pos++;
        if (pos >= read_size)
          {
            strncpy (tmp, &buffer [begin], pos - begin);
            tmp [pos - begin] = '\0';
            load_buffer ();
            if (read_size == 0)
              {
                raise_exception (end_of_file_event);
                return;
              }
          }
      }
    if (begin < pos)
      {
        size = pos - begin;
        strncpy (tag, &buffer [begin], size);
        tag [size] = '\0';
      }
    else
      {
        strcpy (tag, tmp);
        size = strlen (tmp);
        strncpy (&tag [size], &buffer [0], pos);
        tag [size + pos] = '\0';
      }

    strlwc (tag);

    tagid [stack_pos] = -1;
    symbol = sym_lookup_symbol (sym_tag, tag);
    if (symbol)
      {
        tagid          [stack_pos] = ((MARKUP_DEF *)symbol-> data)-> id;
        end_token      [stack_pos] = ((MARKUP_DEF *)symbol-> data)-> end_token;
        tag_script     [stack_pos] = ((MARKUP_DEF *)symbol-> data)-> script;
        end_token_size [stack_pos] = ((MARKUP_DEF *)symbol-> data)-> endtag_length;
      }
}


/************************   GO TO END OF SCRIPT TAG   ************************/

MODULE go_to_end_of_script_tag (void)
{
    if (end_token [stack_pos] == NULL)
      {
        end_token [stack_pos] = ">";
        end_token_size [stack_pos] = 1;
      }

    while (buffer [pos] != '\0' && read_size > 0)
      {
         pos++;
         if (pos >= read_size)
            load_buffer ();
         if (read_size == 0)
           {
             raise_exception (end_of_file_event);
             return;
           }
         if (buffer [pos] == end_token [stack_pos][0])
           {
             if (lexncmp (&buffer [pos], end_token [stack_pos],
                          end_token_size [stack_pos]) == 0)
               {
                 pos += end_token_size [stack_pos];
                 break;
               }
           }
         if (buffer [pos]     == '"'
         &&  buffer [pos + 1] == '!'
         &&  buffer [pos + 2] != '"')
           {
             remove_string_tag_begin ();
             raise_exception (script_data_event);
             break;
           }
      }
    the_next_event = end_of_tag_event;
}

void 
remove_string_tag_begin (void)
{
    int
        quot_pos;
    char
        old_val;

    quot_pos = pos;

    pos++;

    while (buffer [pos] != '\0')
      {
        if (!IS_NOT_TRANSLATED (buffer [pos]))
            break;
        if (buffer [pos] == '<')
            break;
        if (buffer [pos]     == '!'
        &&  buffer [pos + 1] == '"')
            break;
        if (lexncmp (&buffer [pos], "&nbsp;", 6) == 0)
            pos += 5;
        if (lexncmp (&buffer [pos], "&lt;", 4) == 0)
            pos += 3;
        if (lexncmp (&buffer [pos], "&gt;", 4) == 0)
            pos += 3;
        pos++;
        if (pos >= read_size)
          {
            raise_exception (last_tag_event);
            return;
          }
      }

    if (pos == quot_pos + 2)
        buffer [pos - 1] = '\0';
    else
      {
        old_val      = buffer [pos];
        buffer [pos] = '\0';
        memmove (&buffer [quot_pos + 1], &buffer [quot_pos + 2], pos - quot_pos - 1);
        buffer [pos] = old_val;
      }
}

void 
remove_string_tag_end (void)
{
    int
        begin_pos,
        quot_pos;

    quot_pos   = end_script_data_pos;
    begin_pos  = pos;
    saved_pos  = pos + 1;

    if (pos < quot_pos - 1)
        memmove (&buffer [pos + 1], &buffer [pos], quot_pos - pos - 1);

    pos = end_script_data_pos;
    pos++;
}

/*******************************   STACK TAG   *******************************/

MODULE stack_tag (void)
{
    stack_pos++;
    end_token      [stack_pos] = NULL;
    end_token_size [stack_pos] = 0;
    tagid          [stack_pos] = -1;
    tag_script     [stack_pos] = 0;
}


/******************************   UNSTACK TAG   ******************************/

MODULE unstack_tag (void)
{
    if (stack_pos > 0)
        stack_pos--;

    if (stack_pos == 0)
        the_next_event = end_event;
    else
        the_next_event = normal_event;
}


/******************************   REMOVE CRLF   ******************************/

MODULE remove_crlf (void)
{
    while (buffer [pos] != '\0')
      {
        if (!IS_NOT_TRANSLATED (buffer [pos]))
            break;
        if (buffer [pos] == '<')
            break;
        if (lexncmp (&buffer [pos], "&nbsp;", 6) == 0)
            pos += 5;
        pos++;
        if (pos >= read_size)
          {
            raise_exception (last_tag_event);
            return;
          }
      }
}


/*************************   CHECK IF NEED END TAG   *************************/

MODULE check_if_need_end_tag (void)
{
    if (tagid [stack_pos]!= -1
    &&  tag_table [tagid [stack_pos]].end_tag == TRUE)
        the_next_event  = yes_event;
    else
        the_next_event  = no_event;

}


/**************************   GOTO SCRIPT END TAG   **************************/

MODULE goto_script_end_tag (void)
{
    char
        *tag_name;
    int
        tag_size;

    if (tagid [stack_pos]!= -1)
        tag_name =  tag_table [tagid [stack_pos]].tag;
    else
        return;

    tag_size = tag_table [tagid [stack_pos]].tag_length;
    while (buffer [pos] != '\0')
      {
        if (buffer [pos]     == '<'
        &&  buffer [pos + 1] == '/')
          {
            if (lexncmp (&buffer [pos + 2], tag_name, tag_size) == 0)
              {
                while (buffer [pos] != '>')
                  {
                    pos++;
                    if (pos >= read_size)
                        load_buffer ();
                    if (read_size == 0)
                      {
                        raise_exception (end_of_file_event);
                        return;
                      }
                  }
                pos++;
                break;
              }
          }
         if (buffer [pos]     == '"'
         &&  buffer [pos + 1] == '!')
           {
             remove_string_tag_begin ();
             raise_exception (script_data_event);
             return;
           }
        pos++;
        if (pos >= read_size)
            load_buffer ();
        if (read_size == 0)
          {
            raise_exception (end_of_file_event);
            return;
          }
      }

    if (stack_pos > 0)
        stack_pos--;

}


/************************   SEARCH FOR SCRIPTING TAG   ***********************/

MODULE search_for_scripting_tag (void)
{
    int
        index;

    pos = saved_tag_pos;

    the_next_event = not_found_event;

    if (buffer [pos] == '<')
        pos++;

    for (index = 0; index < nbr_tag; index++)
      {
        if (lexncmp (&buffer [pos], tag_table [index].tag,
                     (tag_table + index)-> tag_length) == 0)
          {
            tagid          [stack_pos] = tag_table [index].id;
            end_token      [stack_pos] = tag_table [index].end_token;
            tag_script     [stack_pos] = tag_table [index].script;
            end_token_size [stack_pos] = tag_table [index].endtag_length;
            the_next_event = found_event;
            break;
          }
      }
}


/***************************   RETURN BEFORE TAG   ***************************/

MODULE return_before_tag (void)
{
  pos = saved_tag_pos - 1;
  if (buffer [pos] == '/')
      pos--;

  the_next_event = end_of_tag_event;
}


/***************************   CREATE SCRIPT DATA   **************************/

MODULE create_script_data (void)
{
    char
        old_val;

    old_val      = buffer [pos];
    buffer [pos] = '\0';

    tag_put_data (current_tag, &buffer [script_data_pos], tag_script [stack_pos]);

    buffer [pos] = old_val;

    remove_string_tag_end ();
    pos++;
}


/************************   GO TO END OF SCRIPT DATA   ***********************/

MODULE go_to_end_of_script_data (void)
{
   int
       last_char;

   script_data_pos = pos;

   while (buffer [pos]     != '!'
   &&     buffer [pos + 1] != '"')
     {
       pos++;
       if (pos >= read_size)
          load_buffer ();
       if (read_size == 0)
         {
           raise_exception (end_of_file_event);
           return;
         }
     }

    end_script_data_pos = pos + 1;

    last_char = pos - 1;
    while (last_char > script_data_pos)
      {
        if (buffer [last_char] == ';')
          {
            if (last_char > 6
            &&  lexncmp (buffer + last_char - 5, "&nbsp;", 6) == 0)
                last_char -= 6;
            else
            if (last_char > 4
            &&  lexncmp (buffer + last_char - 3, "&lt;", 4) == 0)
                last_char -= 4;
            else
            if (last_char > 4
            &&  lexncmp (buffer + last_char - 3, "&gt;", 4) == 0)
                last_char -= 4;
            else
                break;
          }
        else
        if (IS_NOT_TRANSLATED (buffer [last_char]))
            last_char--;
        else
            break;                      /*  We hit something valid           */
    }

    /*  If the entire tag is skipped, take it as it was                       */
    if (last_char > script_data_pos)
        pos = last_char + 1;

}


/*****************************   RESTORE STACK   *****************************/

MODULE restore_stack (void)
{
    stack_pos++;
}


/****************************   GOTO END OF FILE   ***************************/

MODULE goto_end_of_file (void)
{
    pos = read_size;
}

