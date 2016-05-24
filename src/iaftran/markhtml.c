/*  ----------------------------------------------------------------<Prolog>-
    name:       markhtml.c
    title:      Markup Definition for html version 4.0
    package:    Markup Translator

    Gerered By GSLGEN schema markstat on 2002/05/16 14:53:30

    copyright:  Copyright (C) 1991-99 Imatix Corporation
 ------------------------------------------------------------------</Prolog>-*/
#include "sfl.h"
#include "markcfg.h"
#include "marklib.h"
#include "markload.h"
#include "markhtml.h"


MARKUP_DEF markup_html_tag [HTML_NB_TAG] = {
 {MARKUP_HTML__         , "%"         , FALSE,  "%>", SERVER_SCRIPT, FALSE, FALSE, FALSE, 1 , 2}, /* Asp Script               */
 {MARKUP_HTML____FML    , "!--fml"    , FALSE, "-->", CLIENT_SCRIPT, FALSE, FALSE, TRUE , 6 , 3}, /* Fml Tag In Imatix Studio Screen*/
 {MARKUP_HTML____       , "!--"       , FALSE, "-->", CLIENT_SCRIPT, FALSE, FALSE, FALSE, 3 , 3}, /* Comment                  */
 {MARKUP_HTML_A         , "a"         , TRUE ,   ">", FALSE, FALSE, TRUE , FALSE, 1 , 1}, /* Anchor                   */
 {MARKUP_HTML_ABBR      , "abbr"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Abbreviated Form (E.G., Www, Http, Etc.)*/
 {MARKUP_HTML_ACRONYM   , "acronym"   , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 7 , 1}, /*                          */
 {MARKUP_HTML_ADDRESS   , "address"   , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 7 , 1}, /* Information On Author    */
 {MARKUP_HTML_APPLET    , "applet"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Java Applet              */
 {MARKUP_HTML_AREA      , "area"      , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Client-Side Image Map Area */
 {MARKUP_HTML_B         , "b"         , TRUE ,   ">", FALSE, TRUE , FALSE, FALSE, 1 , 1}, /* Bold Text Style          */
 {MARKUP_HTML_BASE      , "base"      , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Document Base Uri        */
 {MARKUP_HTML_BASEFONT  , "basefont"  , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Base Font Size           */
 {MARKUP_HTML_BDO       , "bdo"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* I18N Bidi Over-Ride      */
 {MARKUP_HTML_BIG       , "big"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Large Text Style         */
 {MARKUP_HTML_BLOCKQUOTE, "blockquote", TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 10, 1}, /* Long Quotation           */
 {MARKUP_HTML_BODY      , "body"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Document Body            */
 {MARKUP_HTML_BR        , "br"        , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Forced Line Break        */
 {MARKUP_HTML_BUTTON    , "button"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Push Button              */
 {MARKUP_HTML_CAPTION   , "caption"   , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 7 , 1}, /* Table Caption            */
 {MARKUP_HTML_CENTER    , "center"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Shorthand For Div Align=Center */
 {MARKUP_HTML_CITE      , "cite"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Citation                 */
 {MARKUP_HTML_CODE      , "code"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Computer Code Fragment   */
 {MARKUP_HTML_COL       , "col"       , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Table Column             */
 {MARKUP_HTML_COLGROUP  , "colgroup"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Table Column Group       */
 {MARKUP_HTML_DD        , "dd"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Definition Description   */
 {MARKUP_HTML_DEL       , "del"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Deleted Text             */
 {MARKUP_HTML_DFN       , "dfn"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Instance Definition      */
 {MARKUP_HTML_DIR       , "dir"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Directory List           */
 {MARKUP_HTML_DIV       , "div"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Generic Language/Style Container */
 {MARKUP_HTML_DL        , "dl"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Definition List          */
 {MARKUP_HTML_DT        , "dt"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Definition Term          */
 {MARKUP_HTML_EM        , "em"        , TRUE ,   ">", FALSE, TRUE , FALSE, FALSE, 2 , 1}, /* Emphasis                 */
 {MARKUP_HTML_FIELDSET  , "fieldset"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Form Control Group       */
 {MARKUP_HTML_FONT      , "font"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Local Change To Font     */
 {MARKUP_HTML_FORM      , "form"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Interactive Form         */
 {MARKUP_HTML_FRAME     , "frame"     , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Subwindow                */
 {MARKUP_HTML_FRAMESET  , "frameset"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Window Subdivision       */
 {MARKUP_HTML_H1        , "h1"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Heading                  */
 {MARKUP_HTML_H2        , "h2"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Heading                  */
 {MARKUP_HTML_H3        , "h3"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Heading                  */
 {MARKUP_HTML_H4        , "h4"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Heading                  */
 {MARKUP_HTML_H5        , "h5"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Heading                  */
 {MARKUP_HTML_H6        , "h6"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Heading                  */
 {MARKUP_HTML_HEAD      , "head"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Document Head            */
 {MARKUP_HTML_HR        , "hr"        , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Horizontal Rule          */
 {MARKUP_HTML_HTML      , "html"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Document Root Element    */
 {MARKUP_HTML_I         , "i"         , TRUE ,   ">", FALSE, TRUE , FALSE, FALSE, 1 , 1}, /* Italic Text Style        */
 {MARKUP_HTML_IFRAME    , "iframe"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Inline Subwindow         */
 {MARKUP_HTML_IMG       , "img"       , FALSE,   ">", FALSE, FALSE, FALSE, TRUE , 3 , 1}, /* Embedded Image           */
 {MARKUP_HTML_INPUT     , "input"     , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Form Control             */
 {MARKUP_HTML_INS       , "ins"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Inserted Text            */
 {MARKUP_HTML_ISINDEX   , "isindex"   , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 7 , 1}, /* Single Line Prompt       */
 {MARKUP_HTML_KBD       , "kbd"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Text To Be Entered By The User*/
 {MARKUP_HTML_LABEL     , "label"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Form Field Label Text    */
 {MARKUP_HTML_LEGEND    , "legend"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Fieldset Legend          */
 {MARKUP_HTML_LI        , "li"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* List Item                */
 {MARKUP_HTML_LINK      , "link"      , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* A Media-Independent Link */
 {MARKUP_HTML_MAP       , "map"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Client-Side Image Map    */
 {MARKUP_HTML_MENU      , "menu"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Menu List                */
 {MARKUP_HTML_META      , "meta"      , FALSE,   ">", FALSE, FALSE, FALSE, TRUE , 4 , 1}, /* Generic Metainformation  */
 {MARKUP_HTML_NOFRAMES  , "noframes"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Alternate Content Container For Non Frame-Based Rendering */
 {MARKUP_HTML_NOSCRIPT  , "noscript"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Alternate Content Container For Non Script-Based Rendering */
 {MARKUP_HTML_OBJECT    , "object"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Generic Embedded Object  */
 {MARKUP_HTML_OL        , "ol"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Ordered List             */
 {MARKUP_HTML_OPTGROUP  , "optgroup"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Option Group             */
 {MARKUP_HTML_OPTION    , "option"    , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Selectable Choice        */
 {MARKUP_HTML_P         , "p"         , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 1 , 1}, /* Paragraph                */
 {MARKUP_HTML_PARAM     , "param"     , FALSE,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Named Property Value     */
 {MARKUP_HTML_PRE       , "pre"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Preformatted Text        */
 {MARKUP_HTML_Q         , "q"         , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 1 , 1}, /* Short Inline Quotation   */
 {MARKUP_HTML_S         , "s"         , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 1 , 1}, /* Strike-Through Text Style*/
 {MARKUP_HTML_SAMP      , "samp"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Sample Program Output, Scripts, Etc.*/
 {MARKUP_HTML_SCRIPT    , "script"    , TRUE ,   ">", CLIENT_SCRIPT, FALSE, FALSE, FALSE, 6 , 1}, /* Script Statements        */
 {MARKUP_HTML_SELECT    , "select"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Option Selector          */
 {MARKUP_HTML_SMALL     , "small"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Small Text Style         */
 {MARKUP_HTML_SPAN      , "span"      , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 4 , 1}, /* Generic Language/Style Container */
 {MARKUP_HTML_STRIKE    , "strike"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Strike-Through Text      */
 {MARKUP_HTML_STRONG    , "strong"    , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 6 , 1}, /* Strong Emphasis          */
 {MARKUP_HTML_STYLE     , "style"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Style Info               */
 {MARKUP_HTML_SUB       , "sub"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Subscript                */
 {MARKUP_HTML_SUP       , "sup"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}, /* Superscript              */
 {MARKUP_HTML_TABLE     , "table"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* 160;                     */
 {MARKUP_HTML_TBODY     , "tbody"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Table Body               */
 {MARKUP_HTML_TD        , "td"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Table Data Cell          */
 {MARKUP_HTML_TEXTAREA  , "textarea"  , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 8 , 1}, /* Multi-Line Text Field    */
 {MARKUP_HTML_TFOOT     , "tfoot"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Table Footer             */
 {MARKUP_HTML_TH        , "th"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Table Header Cell        */
 {MARKUP_HTML_THEAD     , "thead"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Table Header             */
 {MARKUP_HTML_TITLE     , "title"     , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 5 , 1}, /* Document Title           */
 {MARKUP_HTML_TR        , "tr"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Table Row                */
 {MARKUP_HTML_TT        , "tt"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Teletype Or Monospaced Text Style*/
 {MARKUP_HTML_U         , "u"         , TRUE ,   ">", FALSE, TRUE , FALSE, FALSE, 1 , 1}, /* Underlined Text Style    */
 {MARKUP_HTML_UL        , "ul"        , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 2 , 1}, /* Unordered List           */
 {MARKUP_HTML_VAR       , "var"       , TRUE ,   ">", FALSE, FALSE, FALSE, FALSE, 3 , 1}  /* Instance Of A Variable Or Program Argument*/
};

/*  -------------------------------------------------------------------------
    Function: html_load

    Synopsis: Load html file.
    -------------------------------------------------------------------------*/

MARKUP_TAG *
html_load (char *file_name)
{
    return (markup_load (file_name, (MARKUP_DEF *)markup_html_tag,
                         HTML_NB_TAG));
}


/*  -------------------------------------------------------------------------
    Function: html_del_skipped

    Synopsis: Delete all html skipped tag.
    -------------------------------------------------------------------------*/

void
html_del_skipped (MARKUP_TAG *tag)
{
    tag_del_skipped   (tag, (MARKUP_DEF *)markup_html_tag);
}

/*  -------------------------------------------------------------------------
    Function: html_remove_link

    Synopsis: Replace all html link tag.
    -------------------------------------------------------------------------*/

void
html_remove_link (MARKUP_TAG *tag)
{
    tag_remove_link (tag, (MARKUP_DEF *)markup_html_tag);
}

/*  -------------------------------------------------------------------------
    Function: html_add_string_attr

    Synopsis: Replace all html link tag.
    -------------------------------------------------------------------------*/

void
html_add_string_attr (MARKUP_TAG *tag)
{
    tag_add_string_attr (tag, (MARKUP_DEF *)markup_html_tag);
}

/*  -------------------------------------------------------------------------
    Function: html_init

    Synopsis: Initialise html parser resource.
    -------------------------------------------------------------------------*/

void
html_init (void)
{
    initialise_markup_resource ((MARKUP_DEF *)markup_html_tag,
                                HTML_NB_TAG);
}

/*  -------------------------------------------------------------------------
    Function: html_free

    Synopsis: Free html parser resource.
    -------------------------------------------------------------------------*/

void
html_free (void)
{
    free_markup_resource ();
}

