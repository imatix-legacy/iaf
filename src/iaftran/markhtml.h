/*  ----------------------------------------------------------------<Prolog>-
    name:       markhtml.h
    title:      Markup Definition for html version 4.0
    package:    Markup Translator

    Generated:  2002/05/16 14:53:30 from markstat.gsl

    copyright:  Copyright (C) 1991-99 Imatix Corporation
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _markhtml_included                /*  allow multiple inclusions        */
#define _markhtml_included

#define MARKUP_HTML__                   0
#define MARKUP_HTML____FML              1
#define MARKUP_HTML____                 2
#define MARKUP_HTML_A                   3
#define MARKUP_HTML_ABBR                4
#define MARKUP_HTML_ACRONYM             5
#define MARKUP_HTML_ADDRESS             6
#define MARKUP_HTML_APPLET              7
#define MARKUP_HTML_AREA                8
#define MARKUP_HTML_B                   9
#define MARKUP_HTML_BASE               10
#define MARKUP_HTML_BASEFONT           11
#define MARKUP_HTML_BDO                12
#define MARKUP_HTML_BIG                13
#define MARKUP_HTML_BLOCKQUOTE         14
#define MARKUP_HTML_BODY               15
#define MARKUP_HTML_BR                 16
#define MARKUP_HTML_BUTTON             17
#define MARKUP_HTML_CAPTION            18
#define MARKUP_HTML_CENTER             19
#define MARKUP_HTML_CITE               20
#define MARKUP_HTML_CODE               21
#define MARKUP_HTML_COL                22
#define MARKUP_HTML_COLGROUP           23
#define MARKUP_HTML_DD                 24
#define MARKUP_HTML_DEL                25
#define MARKUP_HTML_DFN                26
#define MARKUP_HTML_DIR                27
#define MARKUP_HTML_DIV                28
#define MARKUP_HTML_DL                 29
#define MARKUP_HTML_DT                 30
#define MARKUP_HTML_EM                 31
#define MARKUP_HTML_FIELDSET           32
#define MARKUP_HTML_FONT               33
#define MARKUP_HTML_FORM               34
#define MARKUP_HTML_FRAME              35
#define MARKUP_HTML_FRAMESET           36
#define MARKUP_HTML_H1                 37
#define MARKUP_HTML_H2                 38
#define MARKUP_HTML_H3                 39
#define MARKUP_HTML_H4                 40
#define MARKUP_HTML_H5                 41
#define MARKUP_HTML_H6                 42
#define MARKUP_HTML_HEAD               43
#define MARKUP_HTML_HR                 44
#define MARKUP_HTML_HTML               45
#define MARKUP_HTML_I                  46
#define MARKUP_HTML_IFRAME             47
#define MARKUP_HTML_IMG                48
#define MARKUP_HTML_INPUT              49
#define MARKUP_HTML_INS                50
#define MARKUP_HTML_ISINDEX            51
#define MARKUP_HTML_KBD                52
#define MARKUP_HTML_LABEL              53
#define MARKUP_HTML_LEGEND             54
#define MARKUP_HTML_LI                 55
#define MARKUP_HTML_LINK               56
#define MARKUP_HTML_MAP                57
#define MARKUP_HTML_MENU               58
#define MARKUP_HTML_META               59
#define MARKUP_HTML_NOFRAMES           60
#define MARKUP_HTML_NOSCRIPT           61
#define MARKUP_HTML_OBJECT             62
#define MARKUP_HTML_OL                 63
#define MARKUP_HTML_OPTGROUP           64
#define MARKUP_HTML_OPTION             65
#define MARKUP_HTML_P                  66
#define MARKUP_HTML_PARAM              67
#define MARKUP_HTML_PRE                68
#define MARKUP_HTML_Q                  69
#define MARKUP_HTML_S                  70
#define MARKUP_HTML_SAMP               71
#define MARKUP_HTML_SCRIPT             72
#define MARKUP_HTML_SELECT             73
#define MARKUP_HTML_SMALL              74
#define MARKUP_HTML_SPAN               75
#define MARKUP_HTML_STRIKE             76
#define MARKUP_HTML_STRONG             77
#define MARKUP_HTML_STYLE              78
#define MARKUP_HTML_SUB                79
#define MARKUP_HTML_SUP                80
#define MARKUP_HTML_TABLE              81
#define MARKUP_HTML_TBODY              82
#define MARKUP_HTML_TD                 83
#define MARKUP_HTML_TEXTAREA           84
#define MARKUP_HTML_TFOOT              85
#define MARKUP_HTML_TH                 86
#define MARKUP_HTML_THEAD              87
#define MARKUP_HTML_TITLE              88
#define MARKUP_HTML_TR                 89
#define MARKUP_HTML_TT                 90
#define MARKUP_HTML_U                  91
#define MARKUP_HTML_UL                 92
#define MARKUP_HTML_VAR                93
#define HTML_NB_TAG 94

/*- Function prototypes ---------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

MARKUP_TAG *html_load                        (char *file_name);
void        html_del_skipped                 (MARKUP_TAG *tag);
void        html_remove_link                 (MARKUP_TAG *tag);
void        html_add_string_attr (MARKUP_TAG *tag);
void        html_init                        (void);
void        html_free                        (void);

#ifdef __cplusplus
}
#endif
#endif
