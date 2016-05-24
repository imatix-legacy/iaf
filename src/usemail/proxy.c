/*  Telnet proxy for debugging purposes
 */
 
#include "sfl.h"

#define TN_CMD_EOR          239
#define TN_CMD_SE           240
#define TN_CMD_NOP          241
#define TN_CMD_DATA         242
#define TN_CMD_BRK          243
#define TN_CMD_IP           244
#define TN_CMD_AO           245
#define TN_CMD_AYT          246
#define TN_CMD_EC           247
#define TN_CMD_EL           248
#define TN_CMD_GA           249
#define TN_CMD_SB           250
#define TN_CMD_WILL         251
#define TN_CMD_WONT         252
#define TN_CMD_DO           253
#define TN_CMD_DONT         254
#define TN_CMD_IAC          255

#define TN_OPT_BINARY         0  
#define TN_OPT_ECHO           1  
#define TN_OPT_RECONNECT      2  
#define TN_OPT_SGA            3  
#define TN_OPT_AMSN           4  
#define TN_OPT_STATUS         5  
#define TN_OPT_TIMING         6  
#define TN_OPT_RCTE           7  
#define TN_OPT_LINESIZ        8  
#define TN_OPT_PAGESIZ        9  
#define TN_OPT_CRDISP        10 
#define TN_OPT_HT            11 
#define TN_OPT_HTDISP        12 
#define TN_OPT_FFDISP        13 
#define TN_OPT_VT            14 
#define TN_OPT_VTDISP        15 
#define TN_OPT_LFDISP        16 
#define TN_OPT_EXTASC        17 
#define TN_OPT_LOGOUT        18 
#define TN_OPT_BYTE          19 
#define TN_OPT_DET           20 
#define TN_OPT_SUPDUP        21 
#define TN_OPT_SUPDUPO       22 
#define TN_OPT_SENDLOC       23 
#define TN_OPT_TRMTYPE       24 
#define TN_OPT_EOR           25 
#define TN_OPT_TACACS        26 
#define TN_OPT_OUTMARK       27 
#define TN_OPT_TERMLOC       28 
#define TN_OPT_3270          29 
#define TN_OPT_X3PAD         30 
#define TN_OPT_WINSIZE       31 

#define TN_TRMTYPE_IS         0  
#define TN_TRMTYPE_SEND       1  

/* 3270 data stream orders */
#define GE      0x08
#define FF      0x0c
#define CRR     0x0d
#define SBA     0x11
#define EUA     0x12
#define ICUR    0x13
#define NL      0x15
#define EM      0x19
#define DUP     0x1c
#define SF      0x1d
#define FM      0x1e
#define SA      0x28
#define SFE     0x29
#define MF      0x2c
#define RA      0x3c
#define SUB     0x3f


static Bool cmdiac;                     /*  Are we showing an IAC sequence?  */

static char *option [] = {
    " BINARY", " ECHO", " RECONNECT", " SGA", " AMSN", " STATUS",
    " TIMING", " RCTE", " LINESIZ", " PAGESIZ", " CRDISP", " HT",
    " HTDISP", " FFDISP", " VT", " VTDISP", " LFDISP", " EXTASC",
    " LOGOUT", " BYTE", " DET", " SUPDUP", " SUPDUPO", " SENDLOC",
    " TRMTYPE", " EOR", " TACACS", " OUTMARK", " TERMLOC", " 3270",
    " X3PAD", " WINSIZE"
};

byte ebcdic_to_ascii [] = {
/*  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F            */
"\x00\x01\x02\x03\xCF\x09\xD3\x7F\xD4\xD5\xC3\x0B\x0C\x0D\x0E\x0F"  /* 00-0F */
"\x10\x11\x12\x13\xC7\x0A\x08\xC9\x18\x19\xCC\xCD\x83\x1D\xD2\x1F"  /* 10-1F */
"\x81\x82\x1C\x84\x86\x0A\x17\x1B\x89\x91\x92\x95\xA2\x05\x06\x07"  /* 20-2F */
"\xE0\xEE\x16\xE5\xD0\x1E\xEA\x04\x8A\xF6\xC6\xC2\x14\x15\xC1\x1A"  /* 30-3F */
"\x20\xA6\xE2\xE4\xE0\x90\x9F\xE2\xE7\x8B\x9B\x2E\x3C\x28\x2B\x7C"  /* 40-4F */
"\x26\xE9\xEA\xEB\xE8\xA5\xEE\xEF\xA8\x9E\x21\x24\x2A\x29\x3B\x5E"  /* 50-5F */
"\x2D\x2F\xDF\xDC\x9A\xDD\xDE\x98\x9D\xAC\xBA\x2C\x25\x5F\x3E\x3F"  /* 60-6F */
"\xD7\x88\x94\xB0\xB1\xB2\x81\xD6\xFB\x60\x3A\x23\x40\x27\x3D\x22"  /* 70-7F */
"\xF8\x61\x62\x63\x64\x65\x66\x67\x68\x69\x96\xA4\xF3\xAF\xAE\xC5"  /* 80-8F */
"\x8C\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x97\x87\xCE\x93\xF1\xFE"  /* 90-9F */
"\xC8\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xEF\xC0\xDA\x5B\xF2\xF9"  /* A0-AF */
"\xB5\xA3\xFD\xB7\xB8\xB9\xE6\xBB\xBC\xBD\x8D\xD9\xBF\x5D\xD8\xC4"  /* B0-BF */
"\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49\xCB\xF4\xF6\xE8\xF3\xED"  /* C0-CF */
"\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xA1\xFB\xFC\xF4\xA3\x8F"  /* D0-DF */
"\x5C\xE7\x53\x54\x55\x56\x57\x58\x59\x5A\xA0\x85\x8E\xE9\xE4\xD1"  /* E0-EF */
"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xB3\xF7\xF0\xFA\xA7\xFF"  /* F0-FF */
};


static void
show_char (FILE *logfile, byte ch)
{
    char
        single_char [3] = "'?",
        unprintable [6],
        *code;
        
    switch (ch)
     {
        case TN_CMD_EOR:           
            code = " EOR"; break;
        case TN_CMD_SE:           
            code = " SE"; break;
        case TN_CMD_NOP:          
            code = " NOP"; break;
        case TN_CMD_DATA:         
            code = " DATA"; break;
        case TN_CMD_BRK:          
            code = " BRK"; break;
        case TN_CMD_IP:           
            code = " IP"; break;
        case TN_CMD_AO:           
            code = " AO"; break;
        case TN_CMD_AYT:          
            code = " AYT"; break;
        case TN_CMD_EC:           
            code = " EC"; break;
        case TN_CMD_EL:           
            code = " EL"; break;
        case TN_CMD_GA:           
            code = " GA"; break;
        case TN_CMD_SB:           
            code = " SB"; break;
        case TN_CMD_DO:
            code = " DO"; break; 
        case TN_CMD_DONT:
            code = " DONT"; break; 
        case TN_CMD_WILL:
            code = " WILL"; break; 
        case TN_CMD_WONT:
            code = " WONT"; break;
        case TN_CMD_IAC:
            code = " IAC"; cmdiac = TRUE; break;
        case 8:
            code = " BKSPC"; break;
        case 9:
            code = " TAB"; break;
        case 10:
            code = " LF"; break;
        case 13:
            code = " CR"; break;
        case 14:
            code = " SIN"; break;
        case 15:
            code = " SOUT"; break;
        case 27:
            code = " ESC"; break;
        case 127:
            code = " BKSP"; break;
        default:
            if (ch < 32 && cmdiac)
                code = option [ch];
            else
            if (isprint (ch))
              {
                single_char [1] = ch;
                code = single_char;
              }
            else
              {
                sprintf (unprintable, "(%d)", ch);
                code = unprintable;
              }  
            break;
      }
    fprintf (logfile, code);
}


/*-------------------------------------------------------------------*/
/* Dump the contents of a buffer, formatted in a way that makes a    */
/* 3270 data stream easier to interpret.                             */
/*-------------------------------------------------------------------*/
void dump3270 (FILE *logfile, byte *dat, int len)
{
  char *cptr ;
  byte c ;
  char cc[3] ;
  int i ;
  int n, m ;

  n = 0 ;
  m = 0 ;
  cptr = dat ;
  do
    {
      fprintf(logfile, " ") ;
      for(i=1; i <= 32; i++)
        {                   /* 0 indicates all */
          if (*dat=='\0' && len==0)
            break ;
          if (n==len && len>0)
            break ;
          c = *dat++ ;
          n++ ;
          fprintf(logfile, "%.2X",c) ;          
          if ((i%4)==0)
              fprintf(logfile, " ") ;
        }
      fprintf(logfile, "\n ") ;

      for(i = 1; i <= 32; i++)
        {
          if (*cptr=='\0' && len==0)
            break ;
          if (m==len && len>0)
            break ;
          c = *cptr++ ;
          m++ ;
          if (c <= 0x3f || c==0xff)
            {
              switch (c)
                {
                  case GE:   strcpy(cc,"GE") ;
                    break ;
                  case FF:   strcpy(cc,"FF") ;
                    break ;
                  case CRR:  strcpy(cc,"CR") ;
                    break ;
                  case SBA:  strcpy(cc,"SB") ;
                    break ;
                  case EUA:  strcpy(cc,"EU") ;
                    break ;
                  case ICUR: strcpy(cc,"IC") ;
                    break ;
                  case NL:   strcpy(cc,"NL") ;
                    break ;
                  case EM:   strcpy(cc,"EM") ;
                    break ;
                  case DUP:  strcpy(cc,"DU") ;
                    break ;
                  case SF:   strcpy(cc,"SF") ;
                    break ;
                  case FM:   strcpy(cc,"FM") ;
                    break ;
                  case SA:   strcpy(cc,"SA") ;
                    break ;
                  case SFE:  strcpy(cc,"SX") ;
                    break ;
                  case MF:   strcpy(cc,"MF") ;
                    break ;
                  case RA:   strcpy(cc,"RA") ;
                    break ;
                  case SUB:  strcpy(cc,"SU") ;
                    break ;
                  default:   strcpy(cc,". ") ;
                    break ;
                }
              fprintf(logfile, "%s",cc) ;
            }
          else
              fprintf(logfile, "%c ", ebcdic_to_ascii [c]) ;
          if ((i%4)==0)
            fprintf(logfile, " ") ;
        }
      fprintf(logfile, "\n") ;
    }
  while( ((*dat!='\0') && (len==0)) ||
         ((m!=len) && (len>20))
       ) ;
}


int main (int argc, char *argv [])
{
    sock_t
        master_handle = 0,
        client_handle = 0,
        server_handle = 0;
    byte
        buffer [1024];
    int
        char_nbr,
        rc;
    fd_set
        read_set,                       /*  Sockets to check for input       */
        error_set;
    struct timeval
        timeout;                        /*  Timeout for select()             */
    FILE
        *logfile;
    Bool
        tn3270 = FALSE,
        trace;                          /*  Trace input only?                */
    char
        *hostname,
        *port;

    hostname = "sundev";
    port     = "1023";
    if (argc > 1)
        if (streq (argv [1], "-trace"))
            trace = TRUE;
        else
          {
            hostname = argv [1];
            port = "23";
          }
    else
        trace = FALSE;
        
    sock_init ();
    ip_nonblock = FALSE;
    logfile = fopen ("tnproxy.log", "w");

    if (trace)
        printf ("tnproxy ready in trace mode\n");
    else
        printf ("tnproxy ready in proxy mode\n");

    master_handle = passive_TCP ("23", 1);
    client_handle = accept_socket (master_handle);
    printf ("Have client connection\n");

    if (!trace)
      {
        printf ("Making telnet server connection... ");
        server_handle = connect_TCP (hostname, port);
        if (server_handle == -1)
          {
            printf ("server connect failed: %s\n", sockmsg ());
            exit (1);
          }
        else
            printf ("Ok\n");
      }
    FOREVER
      {
        cmdiac = FALSE;
        timeout.tv_sec  = 1;                
        timeout.tv_usec = 0;  
        memset (&read_set,  0, sizeof (fd_set));
        memset (&error_set, 0, sizeof (fd_set));
        FD_SET (client_handle, &error_set);
        FD_SET (client_handle, &read_set);
        if (!trace)
          {
            FD_SET (server_handle, &error_set);
            FD_SET (server_handle, &read_set);
          }
        rc = sock_select (max (server_handle, client_handle) + 1,
                          &read_set, NULL, &error_set, &timeout);
        if (rc == SOCKET_ERROR)             /*  Error from socket call           */
          {
            printf ("Error on select()");
            break;
          }
        else
        if (rc == 0)                        /*  No input or output activity      */
            ;
        else
        if (FD_ISSET (server_handle, &error_set))
          {
            printf ("Error on server handle");
            break;
          }
        else
        if (FD_ISSET (client_handle, &error_set))
          {
            printf ("Error on client handle");
            break;
          }
        else
        if (FD_ISSET (server_handle, &read_set))
          {
            fprintf (logfile, "----------------- From Server -----------------\n");
            if ((rc = read_TCP (server_handle, buffer, 1024)) < 1)
              {
                printf ("Server broke connection\n");
                break;
              }
            if (memchr (buffer, TN_CMD_WILL, rc)
            &&  memchr (buffer, TN_OPT_BINARY, rc))
                tn3270 = TRUE;            

            if (tn3270 && buffer [0] != TN_CMD_IAC)
                dump3270 (logfile, buffer, rc);
            else
              {
                for (char_nbr = 0; char_nbr < rc; char_nbr++)
                    show_char (logfile, buffer [char_nbr]);
                fprintf (logfile, "\n");
              }
            write_TCP (client_handle, buffer, rc);
          }
        else
        if (FD_ISSET (client_handle, &read_set))
          {
            fprintf (logfile, "----------------- From Client -----------------\n");
            if ((rc = read_TCP (client_handle, buffer, 1024)) < 1)
              {
                printf ("Client broke connection\n");
                break;
              }
            if (tn3270 && buffer [0] != TN_CMD_IAC)
                dump3270 (logfile, buffer, rc);
            else
              {
                for (char_nbr = 0; char_nbr < rc; char_nbr++)
                    show_char (logfile, buffer [char_nbr]);
                fprintf (logfile, "\n");
              }
            write_TCP (server_handle, buffer, rc);
          }
      }
    close (server_handle);
    close (client_handle);
    close (master_handle);

    sock_term ();
    return (0);
}

