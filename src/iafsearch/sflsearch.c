/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflsearch.c
    Title:      Function to sort search indexation result

    Written:    2001/08/21  iMatix Corporation <info@imatix.com>
    Revised:    2001/08/22

    Synopsis:   

    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include "sfl.h"
#include "sflsearch.h"

/*- Declaration -------------------------------------------------------------*/

#define RECORD_LINE_SIZE 102
#define INDEX_SIZE       82

#define FIELD_NAME_SIZE  40
#define FIELD_VALUE_SIZE 40
#define FIELD_TYPE_SIZE  2
#define FIELD_TYPE_POS   80
#define FIELD_ID_POS     91

#define FIELD_TYPE_NUM_TXT "3"

typedef struct {
    char  field_name [FIELD_NAME_SIZE  + 1];
    char  txt_value  [FIELD_VALUE_SIZE + 1];
    char  value_type [FIELD_TYPE_SIZE  + 1];
    long  numeric_value;
    long  record_id;
} SEARCH_REC;

typedef struct {
    char *index;
    long rec_position;
} REC_INDEX;

#define CR  13
#define LF  10

/*- Local function declaration ----------------------------------------------*/
void          get_index_value     (FILE *file, REC_INDEX *index);
int           sort_index          (const void *index1, const void *index2);
void          save_indexed_record (FILE *ft, FILE *file, REC_INDEX *index);
SEARCH_TOKEN *alloc_token         (char *value, short type, short operation,
                                   short query);
void          save_xml_field      (FILE *file, char *xml, char *id);

/*  ---------------------------------------------------------------------[<]-
    Function: get_nbr_record 

    Synopsis: Return the number of record.
    ---------------------------------------------------------------------[>]-*/

long
get_nbr_record (const char *file_name)
{
    long
        feedback = 0;
    long
        size;                           /* Size of file                      */

    size = get_file_size (file_name);
    if (size > 0)
        feedback = size / RECORD_LINE_SIZE;

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: sort_file

    Synopsis: Sort the file 'source' into file 'target'
    ---------------------------------------------------------------------[>]-*/

long 
sort_file (char *source, char *target)
{
    long
        size,
        index,
        feedback = 0;
    REC_INDEX
        *index_table;
    FILE
        *ft,
        *file;

    size = get_nbr_record (source);

    if (size <= 0)
        return (feedback);

    index_table = mem_alloc (sizeof (REC_INDEX) * size);
    if (index_table)
      {
        file = fopen (source, "rb");
        if (file)
          {
            for (index = 0; index < size; index++)
                get_index_value (file, &index_table [index]);

            qsort ((void *)index_table, (size_t)size, sizeof (REC_INDEX), sort_index);
            ft = fopen (target, "wb");
            if (ft)
              {
                for (index = 0; index < size; index++)
                    save_indexed_record (ft, file, &index_table [index]);
                fclose (ft);
              }
            fclose (file);
          }
        for (index = 0; index < size; index++)
            mem_strfree (&index_table [index].index);
        mem_free (index_table);
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: search_split

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

int
search_split (const char *string, LIST *list, Bool search_mode)
{
    int
        feedback = 0;
    char
        prefix [PREFIX_SIZE + 1],
        *begin,
        *bufptr,
        *target,
        *sound,
        *buffer;
    SEARCH_TOKEN
        *token;
    short
        operation,
        query,
        type;

    buffer = mem_alloc ((long)((double)(strlen (string) + 1) * 2));
    if (buffer == NULL)
        return (feedback);

    bufptr    = (char *)string;
    target    = buffer;
    operation = SEARCH_OPERATION_OR;
    query     = SEARCH_QUERY_EQ;
    begin     = NULL;
    while (*bufptr)
      {
        if (*bufptr == '+')
          {
            if (search_mode)
                operation = SEARCH_OPERATION_AND;
          }
        else
        if (*bufptr == '-')
          {
            if (search_mode)
                operation = SEARCH_OPERATION_NAND;
          }
        else
        if (*bufptr == SEARCH_QUERY_CHAR)
          {
            if (search_mode
            &&  *(bufptr + 1)
            &&  (   isalpha (*(bufptr + 1)) 
                 || isdigit (*(bufptr + 1)))
                )
                query = SEARCH_QUERY_LE;
          }
        if (isalpha (*bufptr))
          {
             begin = target;
             while (*bufptr && isalpha (*bufptr))
                 *target++ = toupper (*bufptr++);
             if (*bufptr == SEARCH_QUERY_CHAR)
               {
                 if (search_mode)
                   {                 
                     if (*(bufptr + 1)
                     &&  isalpha (*(bufptr + 1)))
                         query = SEARCH_QUERY_RANGE;
                     else
                         query = SEARCH_QUERY_GE;
                   }
                 bufptr++;
               }
             bufptr--;
             *target++ = '\0';              
             type = SEARCH_TOKEN_TYPE_SOUNDEX;
          }
        else
        if (isdigit (*bufptr))
          {
             begin = target;
             while (*bufptr 
             &&     ( isdigit (*bufptr)
                     || *bufptr == '.'
                   ))
                 *target++ = toupper (*bufptr++);
             if (*bufptr == SEARCH_QUERY_CHAR)
               {
                 if (search_mode)
                   {                 
                     if (*(bufptr + 1)
                     &&  isdigit (*(bufptr + 1)))
                         query = SEARCH_QUERY_RANGE;
                     else
                         query = SEARCH_QUERY_GE;
                   }
                 bufptr++;
               }
             bufptr--;
             *target++ = '\0'; 
             type = SEARCH_TOKEN_TYPE_NUMBER;
          }
        if (begin)
          {
            if (type == SEARCH_TOKEN_TYPE_SOUNDEX)
              {
                if (!search_mode 
                || (   query == SEARCH_QUERY_EQ
                    && operation != SEARCH_OPERATION_RP))
                  {
                    /* Save soundex token                                    */
                    type = SEARCH_TOKEN_TYPE_SOUNDEX;
                    sound = soundex (begin);
                    token = alloc_token (sound, type, operation, query); 
                    list_relink_after (list, token);                     
                    feedback++;
                    operation = SEARCH_OPERATION_AND;
                  }
                /* Save prefix token                                         */
                strncpy (prefix, begin, PREFIX_SIZE);
                prefix [PREFIX_SIZE] = '\0';
                type = SEARCH_TOKEN_TYPE_PREFIX;
                token = alloc_token (prefix, type, operation, query); 
                list_relink_after (list, token);                     
                feedback++;

              }
            else
              {
                token = alloc_token (begin, type, operation, query); 
                list_relink_after (list, token);
                feedback++;
              }
            begin = NULL;
            if (query == SEARCH_QUERY_RANGE)
                operation = SEARCH_OPERATION_RP;
            else
                operation = SEARCH_OPERATION_OR;
            query     = SEARCH_QUERY_EQ;
           
          }
        bufptr++;
      }       
    mem_free (buffer);
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: free_token_list

    Synopsis: Free all allocated search token
    ---------------------------------------------------------------------[>]-*/

void
free_token_list (LIST *list)
{
    SEARCH_TOKEN
        *next,
        *token;

    token = list-> next;
    while ((void *)token != (void *)list)
      {
        next = token-> next;
        if (token-> value)
            mem_free (token-> value);
        mem_free (token);
        token = next;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: dump_token_list

    Synopsis: dump search token list
    ---------------------------------------------------------------------[>]-*/

void
dump_token_list (LIST *list)
{
    char
        buffer [256];

    SEARCH_TOKEN
        *token;

    token = list-> next;
    while ((void *)token != (void *)list)
      {
        sprintf (buffer, "    %-30s", token-> value);
        switch (token-> type)
          {
            case SEARCH_TOKEN_TYPE_SOUNDEX:
                strcat (buffer, "S ");
                break;
            case SEARCH_TOKEN_TYPE_PREFIX:
                strcat (buffer, "P ");
                break;
            case SEARCH_TOKEN_TYPE_NUMBER:
                strcat (buffer, "N ");
                break;
          }
        switch (token-> operation)
          {
            case SEARCH_OPERATION_OR:
                strcat (buffer, "OR   ");
                break;
            case SEARCH_OPERATION_AND:
                strcat (buffer, "AND  ");
                break;
            case SEARCH_OPERATION_NAND:
                strcat (buffer, "NAND ");
                break;
            case SEARCH_OPERATION_RP:
                strcat (buffer, "RP   ");
                break;
          }
        switch (token-> query)
          {
            case SEARCH_QUERY_EQ:
                strcat (buffer, "E");
                break;
            case SEARCH_QUERY_GE:
                strcat (buffer, "G");
                break;
            case SEARCH_QUERY_LE:
                strcat (buffer, "L");
                break;
            case SEARCH_QUERY_RANGE:
                strcat (buffer, "R");
                break;
          }
        coputs (buffer);
        token = token-> next;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: save_token_list

    Synopsis: save list of search token to a file.
    ---------------------------------------------------------------------[>]-*/

void
save_token_list (FILE *file, LIST *list, char *field_name, char *id)
{
    SEARCH_TOKEN
        *token;

    strconvch (id, '\r', '\0');
    strconvch (id, '\n', '\0');

    token = list-> next;
    while ((void *)token != (void *)list)
      {
        fprintf (file, "%-40.40s%-40.40s%-2d%-9.9s%-9.9s\r\n",
                       field_name,
                       token-> value,
                       token-> type,
                       token-> type == SEARCH_TOKEN_TYPE_NUMBER?
                           token-> value: "",
                       id);
        token = token-> next;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: make_search_file 

    Synopsis: Read file with field value and save token for search.
    ---------------------------------------------------------------------[>]-*/

void
make_search_file (char *input, char *output)
{
    FILE
        *fs,
        *ft;
    char        
        field_name  [41],
        field_value [512],
        id          [10],
        *ptr,
        xml         [10240];
    int
        type,
        value;
    LIST
        list;
    Bool
        have_xml;
    
    fs = fopen (input, "rb");
    if (fs)
      {
        ft = fopen (output, "wb");
        if (ft)
          {
            while ((value = fgetc (fs)) != EOF)
              {
                if (value == 1)         /* Text field value                  */
                  {
                    ptr  = field_name;
                    type = 1;
                    while ((value = fgetc (fs)) != EOF
                    &&      type > 0)
                      {
                        *ptr++ = value;
                        if (value == 0 || value == CR || value == LF)
                          {
                            switch (type)
                              {
                                case 1:
                                    ptr = field_value;
                                    break;
                                case 2:
                                    ptr = id;
                                    break;
                                case 3:
                                    *(--ptr) = '\0';
                                    type = -1;   /* End of record            */
                                    break;
                              }
                            type++;
                          }
                      }
                    list_reset (&list);
                    search_split (field_value, &list, FALSE);
                    save_token_list (ft, &list, field_name, id);
                    free_token_list (&list);
                  }
                else
                if (value == 2)         /* XML field value                   */
                  {
                    have_xml = FALSE;
                    ptr  = xml;
                    while ((value = fgetc (fs)) != EOF)
                      {
                        *ptr++ = value;
                        if (value == 0)
                          {
                            ptr = id;
                            have_xml = TRUE;
                          }
                        else
                        if (have_xml && (value == CR || value == LF))
                            break;
                      }
                    *ptr = '\0';
                    save_xml_field (ft, xml, id);
                  }
                while (value != EOF && value != LF)
                    value = fgetc (fs);
              }
            fclose (ft);
          }
        fclose (fs);
      }

}


/*  ---------------------------------------------------------------------[<]-
    Function: save_xml_search

    Synopsis: Save search result with bitstring
    ---------------------------------------------------------------------[>]-*/

void
save_xml_search  (char *input, char *output)
{
    FILE
        *fs,
        *ft;
    char
        *bit_string,
        field_name  [FIELD_NAME_SIZE  + 1],
        field_value [FIELD_VALUE_SIZE + 1],
        field_type  [FIELD_TYPE_SIZE  + 1],
        record      [RECORD_LINE_SIZE + 1],
        last_record [RECORD_LINE_SIZE + 1];
    long
        record_count = 1,
        id;
    BITS
        *bits = NULL;

    fs = fopen (input, "rb");
    if (fs == NULL)
        return;

    ft = fopen (output, "wt");
    if (ft == NULL)
      {
        fclose (fs);
        return;
      }
    /* Save file header                                                      */
    fputs ("<export><table\n", ft);
    fputs ("    description = \"\"\n", ft);
    fputs ("    created     = \"20010815\"\n", ft);
    fputs ("    updated     = \"20010815\"\n", ft);
    fputs ("    name        = \"dbsearch\" >\n", ft);
    fputs ("<field name    = \"dbid\"\n", ft);
    fputs ("       type    = \"NUMERIC\"\n", ft);
    fputs ("       ntype   = \"numeric() identity\"\n", ft);
    fputs ("       size    = \"9\"\n", ft);
    fputs ("       decimal = \"0\"\n", ft);
    fputs ("       nulls   = \"N\" />\n", ft);
    fputs ("<field name    = \"dbfieldname\"\n", ft);
    fputs ("       type    = \"TEXTUAL\"\n", ft);
    fputs ("       ntype   = \"varchar\"\n", ft);
    fputs ("       size    = \"40\"\n", ft);
    fputs ("       nulls   = \"Y\" />\n", ft);
    fputs ("<field name    = \"dbtxtvalue\"\n", ft);
    fputs ("       type    = \"TEXTUAL\"\n", ft);
    fputs ("       ntype   = \"varchar\"\n", ft);
    fputs ("       size    = \"40\"\n", ft);
    fputs ("       nulls   = \"Y\" />\n", ft);
    fputs ("<field name    = \"dbnumvalue\"\n", ft);
    fputs ("       type    = \"NUMERIC\"\n", ft);
    fputs ("       ntype   = \"int\"\n", ft);
    fputs ("       size    = \"10\"\n", ft);
    fputs ("       decimal = \"0\"\n", ft);
    fputs ("       nulls   = \"Y\" />\n", ft);
    fputs ("<field name    = \"dbtxtvaltyp\"\n", ft);
    fputs ("       type    = \"NUMERIC\"\n", ft);
    fputs ("       ntype   = \"int\"\n", ft);
    fputs ("       size    = \"10\"\n", ft);
    fputs ("       decimal = \"0\"\n", ft);
    fputs ("       nulls   = \"Y\" />\n", ft);
    fputs ("<field name    = \"dbidstring\"\n", ft);
    fputs ("       type    = \"TEXTUAL\"\n", ft);
    fputs ("       ntype   = \"text\"\n", ft);
    fputs ("       size    = \"65534\"\n", ft);
    fputs ("       nulls   = \"Y\" />\n", ft);
    fputs ("<field name    = \"dbidcount\"\n", ft);
    fputs ("       type    = \"NUMERIC\"\n", ft);
    fputs ("       ntype   = \"int\"\n", ft);
    fputs ("       size    = \"10\"\n", ft);
    fputs ("       decimal = \"0\"\n", ft);
    fputs ("       nulls   = \"Y\" />\n", ft);
    fputs (" <key name=\"primary_index\" duplicates=\"FALSE\" code=\"A\" >\n", ft);
    fputs ("  <field name=\"dbid\" sort=\"ASCENDING\" />\n", ft);
    fputs ("</key>\n", ft);
    fputs ("<key name=\"bynumvalue\" duplicates=\"TRUE\" code=\"B\" >\n", ft);
    fputs ("  <field name=\"dbfieldname\" sort=\"ASCENDING\" />\n", ft);
    fputs ("  <field name=\"dbnumvalue\" sort=\"ASCENDING\" />\n", ft);
    fputs ("</key>\n", ft);
    fputs ("<key name=\"bytxtvalue\" duplicates=\"TRUE\" code=\"C\" >\n", ft);
    fputs ("  <field name=\"dbfieldname\" sort=\"ASCENDING\" />\n", ft);
    fputs ("  <field name=\"dbtxtvalue\" sort=\"ASCENDING\" />\n", ft);
    fputs ("  <field name=\"dbtxtvaltyp\" sort=\"ASCENDING\" />\n", ft);
    fputs ("</key>\n", ft);
    fputs ("</table>\n", ft);

    memset (last_record, 0, RECORD_LINE_SIZE + 1);
    memset (record,      0, RECORD_LINE_SIZE + 1);
    while (fread (record, 1, RECORD_LINE_SIZE, fs) == RECORD_LINE_SIZE)
      {
        /* Index change, save to file                                        */
        if (strncmp (record, last_record, INDEX_SIZE) != 0)
          {
             if (*last_record)
               {
                 fprintf (ft, "<record\nrecno=\"%ld\"\ndbid=\"%ld\"\n",
                          record_count, record_count);
                 fprintf (ft, "dbfieldname=\"%s\"\ndbtxtvalue=\"%s\"\n",
                          field_name, field_value);
                 if (streq (field_type, FIELD_TYPE_NUM_TXT))
                     fprintf (ft, "dbnumvalue=\"%s\"\n", field_value);
                 fprintf (ft, "dbtxtvaltyp=\"%s\"\n", field_type);
                 bit_string = bits_save (bits);
                 if (bit_string)
                   {
                     fprintf (ft, "dbidstring=\"%s\"\n",  bit_string);
                     mem_free (bit_string);
                   }
/* Not used */
/*                 fprintf (ft, "dbidcount=\"%ld\"\n",  bits_set_count (bits));*/
                 fprintf (ft, "/>\n");                 
                 record_count++;
               }
             strcpy (last_record, record);

             strncpy (field_name, record, FIELD_NAME_SIZE);
             field_name [FIELD_NAME_SIZE] = '\0';
             strcrop (field_name);

             strncpy (field_value, &record [FIELD_NAME_SIZE],
                                   FIELD_VALUE_SIZE);
             field_value [FIELD_VALUE_SIZE] = '\0';
             strcrop (field_value);

             strncpy (field_type, &record [FIELD_TYPE_POS],
                                   FIELD_TYPE_SIZE);
             field_type [FIELD_TYPE_SIZE] = '\0';
             strcrop (field_type);
             if (bits)
                 bits_destroy (bits);
             bits = bits_create ();               
          }
        /* Get ID value and save in bitstring                                */
        id = atol (&record [FIELD_ID_POS]);
        if (bits)
            bits_set (bits, id);
      }

    /* Save last record                                                      */
    fprintf (ft, "<record\nrecno=\"%ld\"\ndbid=\"%ld\"\n",
                 record_count, record_count);
    fprintf (ft, "dbfieldname=\"%s\"\ndbtxtvalue=\"%s\"\n",
                 field_name, field_value);
    if (streq (field_type, FIELD_TYPE_NUM_TXT))
        fprintf (ft, "dbnumvalue=\"%s\"\n", field_value);
    fprintf (ft, "dbtxtvaltyp=\"%s\"\n", field_type);
    bit_string = bits_save (bits);
    if (bit_string)
      {
        fprintf (ft, "dbidstring=\"%s\"\n",  bit_string);
        mem_free (bit_string);
      }
/* Not used */
/*  fprintf (ft, "dbidcount=""%ld""\n",  bits_set_count (bits));*/
    fprintf (ft, "/>\n");                 
    if (bits)
        bits_destroy (bits);
    fputs ("</export>\n", ft);
    fclose (ft);
    fclose (fs);
}


void
save_xml_field (FILE *file, char *xml, char *id)
{
    LIST
        list;
    char
        *attr_value;
    XML_ITEM
        *root = NULL,
        *item;
    XML_ATTR
        *attr;
    
    if (xml_load_string (&root, xml, FALSE) == XML_NOERROR)
      {
        item = xml_first_child (root);
        if (item)
          {
            FORATTRIBUTES (attr, item)
              {
                attr_value = xml_attr_value (attr);
                if (attr_value && *attr_value)
                  {
                    list_reset (&list);
                    search_split (attr_value, &list, FALSE);
                    save_token_list (file, &list, xml_attr_name (attr), id);
                    free_token_list (&list);
                  }
              }
          }
      }
    else
        printf ("Error on load xml\n%s\n%s\n", xml, xml_error ());
    if (root)
        xml_free (root);
}


SEARCH_TOKEN *
alloc_token (char *value, short type, short operation, short query)
{
    SEARCH_TOKEN
        *token;
    token = mem_alloc (sizeof (SEARCH_TOKEN));
    if (token)
      {
        list_reset (token);
         token-> value      = mem_strdup (value);
         token-> type       = type;
         token-> operation  = operation;
         token-> query      = query;
      }
    return (token);
}


void 
get_index_value (FILE *file, REC_INDEX *index)
{
    char
        *source,
        *target,
        buffer [RECORD_LINE_SIZE + 1];
    int
        field;
#define FIELD_NAME 1
#define FIELD_TEXT 2
#define FIELD_TYPE 3

    index-> rec_position = ftell (file);

    fread (buffer, 1, RECORD_LINE_SIZE, file);

    source = buffer;
    target = buffer;
    field  = FIELD_NAME;
    while (*source)
      {
        if (*source == ' '
        ||  (field == FIELD_NAME && source - buffer >= FIELD_NAME_SIZE)
        ||  (field == FIELD_TEXT && source - buffer >= FIELD_TYPE_POS)
        ||  (field == FIELD_TYPE && source - buffer >= INDEX_SIZE))
          {
            switch (field)
              {
                case FIELD_NAME:
                    *target++ = '|';
                    source    = &buffer [FIELD_NAME_SIZE];
                    field     = FIELD_TEXT;
                    break;
                case FIELD_TEXT:
                    *target++ = '|';
                    source    = &buffer [FIELD_TYPE_POS];
                    field     = FIELD_TYPE;
                    break;
                case FIELD_TYPE:
                    *target   = '\0';
                    source    = target;
                    break;
              }
          }
        else
            *target++ = *source++;
      }
    index-> index = mem_strdup (buffer);
}

int
sort_index  (const void *index1, const void *index2)
{
    return (strcmp (((REC_INDEX *)index1)-> index,
                    ((REC_INDEX *)index2)-> index));
}

void 
save_indexed_record (FILE *ft, FILE *file, REC_INDEX *index)
{
    char
        buffer [RECORD_LINE_SIZE + 1];

    fseek (file, index-> rec_position, SEEK_SET);
    fread  (buffer, 1, RECORD_LINE_SIZE, file);
    fwrite (buffer, 1, RECORD_LINE_SIZE, ft);
}
