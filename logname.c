#ident "$Id: logname.c,v 1.14 1993/11/05 22:36:52 gert Exp $ Copyright (c) Gert Doering"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#ifndef sun
#include <sys/ioctl.h>
#endif

#ifndef ENOENT
#include <errno.h>
#endif

#include "mgetty.h"
#include "policy.h"
#include "tio.h"

#ifndef SYSTEM
#include <sys/utsname.h>
#endif

/* Linux apparently does not define these constants */
#ifdef linux
#define CQUIT	034		/* FS,  ^\ */
#define CEOF	004		/* EOT, ^D */
#define CKILL	025		/* NAK, ^U */
#define CERASE	010		/* BS,  ^H */
#define CINTR	0177		/* DEL, ^? */
#endif

char * ln_escape_prompt _P1( (ep), char * ep )
{
#define MAX_PROMPT_LENGTH 140
    char * p;
    int    i;

    p = malloc( MAX_PROMPT_LENGTH );
    if ( p == NULL ) return NULL;

    i = 0;
    
    while ( *ep != 0 && i < MAX_PROMPT_LENGTH-4 )
    {
	if ( *ep == '@' )		/* system name */
	{
#ifdef SYSTEM
	    if ( sizeof( SYSTEM ) + i > MAX_PROMPT_LENGTH ) break;
	    i += sprintf( &p[i], "%s", SYSTEM );
#else
	    struct utsname un;
	    uname( &un );
	    if ( strlen( un.nodename ) +1 +i > MAX_PROMPT_LENGTH ) break;
	    i += sprintf( &p[i], "%s", un.nodename );
#endif
	}
	else if ( *ep != '\\' ) p[i++] = *ep;
	else		/* *ep == '\\' */
	{
	    ep++;
	    switch ( *ep )
	    {
	      case 'n': p[i++] = '\n'; break;
	      case 'r': p[i++] = '\r'; break;
	      case 'g': p[i++] = '\007'; break;
	      case 'b': p[i++] = '\010'; break;
	      case 'v': p[i++] = '\013'; break;
	      case 'f': p[i++] = '\f'; break;
	      case 't': p[i++] = '\t'; break;
	      case 'D':			/* fallthrough */
	      case 'T':
		if ( i + 30 > MAX_PROMPT_LENGTH )
		{
		    i += sprintf( &p[i], "(x)" ); break;
		}
		else
		{
		    time_t ti = time( NULL );
		    struct tm * tm = localtime( &ti );

		    if ( tm == NULL ) break;

		    if ( *ep == 'D' )
		        i += sprintf( &p[i], "%d/%d/%d", tm->tm_mon+1,
				     tm->tm_mday, tm->tm_year + 1900 );
		    else
		        i += sprintf( &p[i], "%d:%d:%d", tm->tm_hour,
				     tm->tm_min, tm->tm_sec );
		}
		break;
	      default:		/* not a recognized string */
		if ( isdigit( *ep ) )		/* "\01234" */
		{
		    char * help;
		    p[i++] = strtol( ep, &help, 0 );
		    ep = help-1;
		}
		else p[i++] = *ep;
	    }			/* end switch( char after '\\' ) */
	}			/* end if ( char is '\\' ) */
	ep++;
    }				/* end while ( char to copy, p not full ) */

    p[i] = 0;			/* terminate string */
    
    if ( *ep != 0 )
    {
	lprintf( L_WARN, "buffer overrun - input prompt too long" );
    }

    return p;
}


static int timeouts = 0;
#ifdef MAX_LOGIN_TIME
static RETSIGTYPE getlog_timeout()
{
    signal( SIGALRM, getlog_timeout );

    lprintf( L_WARN, "getlogname: timeout\n" );
    timeouts++;
    if ( timeouts == 1 )
    {
	printf( "\r\n\07\r\nHey! Please login now. You have one minute left\r\n" );
	alarm(60);
    }
    else
    {
	printf( "\r\n\07\r\nYour login time (%d minutes) ran out. Goodbye.\r\n",
		 MAX_LOGIN_TIME / 60 );
	sleep(3);
    }
}
#endif

int getlogname _P4( (prompt, tio, buf, maxsize),
		    char * prompt, TIO * tio, char * buf,
		    int maxsize )
{
int i;
char ch;
TIO tio_save;
char * final_prompt;

    /* read character by character! */
    tio_save = *tio;
    tio_mode_cbreak( tio );
    tio_set( STDIN, tio );

#ifdef MAX_LOGIN_TIME
    signal( SIGALRM, getlog_timeout );
    alarm( MAX_LOGIN_TIME );
#endif

    final_prompt = ln_escape_prompt( prompt );

newlogin:
    printf( "\r\n" );
    printf( "%s ", final_prompt );

    i = 0;
    lprintf( L_NOISE, "getlogname, read:" );
    do
    {
	if ( read( STDIN, &ch, 1 ) != 1 )
	{
	     if ( errno != EINTR || timeouts != 1 ) exit(0);	/* HUP / ctrl-D */
	     ch = CKILL;		/* timeout (1) -> clear input */
	}

	ch = ch & 0x7f;					/* strip to 7 bit */
	lputc( L_NOISE, ch );				/* logging */

	if ( ch == CQUIT ) exit(0);
	if ( ch == CEOF )
	{
            if ( i == 0 ) exit(0); else continue;
	}

	if ( ch == CKILL ) goto newlogin;

	/* since some OSes use backspace and others DEL -> accept both */

	if ( ch == CERASE || ch == CINTR )
	{
	    if ( i > 0 )
	    {
		fputs( " \b", stdout );
		i--;
	    }
            else
		fputc( ' ', stdout );
	}
	else
	{
	    if ( i < maxsize )
		buf[i++] = ch;
	    else
		fputs( "\b \b", stdout );
	}
    }
    while ( ch != '\n' && ch != '\r' );

    alarm(0);
    free( final_prompt );

    buf[--i] = 0;

    *tio = tio_save;
    if ( ch == '\n' )
    {
	tio_map_cr( tio, FALSE );
	putc( '\r', stdout );
    }
    else
    {
	tio_map_cr( tio, TRUE );
	putc( '\n', stdout );
	lprintf( L_NOISE, "input finished with '\\r', setting ICRNL ONLCR" );
    }

    tio_set( STDIN, tio );

    if ( i == 0 ) return -1;
    else return 0;
}
