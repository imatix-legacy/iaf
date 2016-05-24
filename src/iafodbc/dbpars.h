#ifndef XML_PARSER
#define XML_PARSER

#define XML_NOERROR         0           /*  No errors                        */
#define XML_FILEERROR       1           /*  Error in file i/o                */
#define XML_LOADERROR       2           /*  Error loading XML                */
void init_charmaps (void);

void new_import_table (char *table_name);


#endif /* XML_PARSER */
