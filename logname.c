#ident "$Id: logname.c,v 1.29 1994/02/20 09:53:58 gert Exp $ Copyright (c) Gert Doering"
;
#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>
#include <string.h>
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

/* ln_escape_prompt()
 *
 * substitute all "known" escapes, e.g. "\n" and "\t", plus some
 * private extentions (@, \D and \T for system name, date, and time)
 *
 * The caller has to free() the string returned
 *
 * If the resulting string would be too long, it's silently truncated.
 */

int strappnd _P2((s1,s2), char * s1, char * s2 )
{
    strcpy( s1, s2 );
    return strlen( s1 );
}

char * ln_escape_prompt _P1( (ep), char * ep )
{
#define MAX_PROMPT_LENGTH 140
    char * p;
    int    i;
    extern char * Device;		/* mgetty.c */

    p = malloc( MAX_PROMPT_LENGTH );
    if ( p == NULL ) return NULL;

    i = 0;
    
    while ( *ep != 0 && i < MAX_PROMPT_LENGTH-4 )
    {
	if ( *ep == '@' )		/* system name */
	{
#ifdef SYSTEM
	    if ( sizeof( SYSTEM ) + i > MAX_PROMPT_LENGTH ) break;
	    i += strappnd( &p[i], SYSTEM );
#else
	    struct utsname un;
	    uname( &un );
	    if ( strlen( un.nodename ) +1 +i > MAX_PROMPT_LENGTH ) break;
	    i += strappnd( &p[i], un.nodename );
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
	      case 'P':					/* port name */
	      case 'L':					/* tty line */
		{
		    if ( i + strlen(Device) +1 > MAX_PROMPT_LENGTH ) break;
		    i += strappnd( &p[i], Device );
		    break;
		}
	      case 'C':					/* ctime */
		{
		    time_t ti = time(NULL);
		    char * h = ctime( &ti );
		    if ( strlen(h) +1 +i > MAX_PROMPT_LENGTH ) break;
		    i += strappnd( &p[i], h ) -1;
		    break;
		}
	      case 'I':
		{
		    if ( strlen(Connect) +1 +i > MAX_PROMPT_LENGTH ) break;
		    i += strappnd( &p[i], Connect);
		    break;
		}
	      case 'N':					/* numer of */
	      case 'U':					/* users */
		{
		    sprintf( &p[i], "%d", get_current_users() );
		    i = strlen(p);
		    break;
		}
	      case 'S':					/* port speed */
		{					/* ugly, I know. */
		    TIO temp_t;
		    tio_get( 0, &temp_t );
		    sprintf( &p[i], "%d", tio_get_speed( &temp_t ) );
		    i = strlen(p);
		}
		break;
	      case 'D':			/* fallthrough */
	      case 'T':
		if ( i + 30 > MAX_PROMPT_LENGTH )
		{
		    i += strappnd( &p[i], "(x)" ); break;
		}
		else
		{
		    time_t ti = time( NULL );
		    struct tm * tm = localtime( &ti );

		    if ( tm == NULL ) break;

		    if ( *ep == 'D' )
		        sprintf( &p[i], "%d/%d/%d", tm->tm_mon+1,
				 tm->tm_mday, tm->tm_year + 1900 );
		    else
		        sprintf( &p[i], "%02d:%02d:%02d", tm->tm_hour,
				 tm->tm_min, tm->tm_sec );
		    i = strlen(p);
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


/* set_env_var( var, string )
 *
 * create an environment entry "VAR=string"
 */
void set_env_var _P2( (var,string), char * var, char * string )
{
    char * v;
    v = malloc( strlen(var) + strlen(string) + 2 );
    if ( v == NULL )
        lprintf( L_ERROR, "set_env_var: cannot malloc" );
    else
    {
	sprintf( v, "%s=%s", var, string );
	lprintf( L_NOISE, "setenv: '%s'", v );
	if ( putenv( v ) != 0 )
	    lprintf( L_ERROR, "putenv failed" );
    }
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

/* getlogname()
 *
 * read the login name into "char buf[]", maximum length "maxsize".
 * depending on the key that the input was finished (\r vs. \n), mapping
 * of cr->nl is set in "TIO * tio" (with tio_set_cr())
 *
 * If ENV_TTYPROMPT is set, do not read anything
 */

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

    final_prompt = ln_escape_prompt( prompt );

#ifdef ENV_TTYPROMPT
    printf( "\r\n%s", final_prompt );
    tio_mode_sane( tio, FALSE );
    tio_map_cr( tio, TRUE );
    tio_set( STDIN, tio );
    buf[0] = 0;
    set_env_var( "TTYPROMPT", final_prompt );
    free( final_prompt );
    return 0;
#else			/* !ENV_TTYPROMPT */

#ifdef MAX_LOGIN_TIME
    signal( SIGALRM, getlog_timeout );
    alarm( MAX_LOGIN_TIME );
#endif

newlogin:
#ifdef FIDO
    printf( "**EMSI_REQA77E\r\021              \r" );
#endif

    printf( "\r\n%s", final_prompt );

    i = 0;
    lprintf( L_NOISE, "getlogname, read:" );
    do
    {
	if ( read( STDIN, &ch, 1 ) != 1 )
	{
	     if ( errno != EINTR || timeouts != 1 ) exit(0);	/* HUP / ctrl-D */
	     ch = CKILL;		/* timeout (1) -> clear input */
	}

	lputc( L_NOISE, ch );				/* logging */

#ifdef FIDO
	if ( ch == (char) TSYNC )
	{
	    strcpy( buf, "\377tsync" ); i=6; ch='\r'; 
	}
	else if ( ch == (char) YOOHOO )
	{
	    strcpy( buf, "\377yoohoo" ); i=7; ch='\r';
	}
#endif

	ch = ch & 0x7f;					/* strip to 7 bit */

	if ( ch == CQUIT ) exit(0);
	if ( ch == CEOF )
	{
            if ( i == 0 ) exit(0); else continue;
	}

	if ( ch == CKILL ) goto newlogin;

	/* ignore XON/XOFF characters */
	
	if ( ch == CSTART || ch == CSTOP ) continue;

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
	    if ( i >= maxsize ||			/* buffer full */
		 ( ch == ' ' && i == 0 ) )		/* or leading ' ' */
		fputs( "\b \b", stdout );		/* -> ignore */
	    else
		buf[i++] = ch;
#ifdef FIDO
	    if ( i >= 10 && strncmp( &buf[i-10], "**EMSI_INQ", 10 ) == 0 )
	    {
		strcpy( buf, "\377**EMSI_INQ" ); i=11; break;
	    }
#endif
	}
    }
    while ( ch != '\n' && ch != '\r' );

#ifdef FIDO
    if ( strncmp( buf, "\377**EMSI_INQ", 11 ) == 0 )
    {				/* read up to final \r */
	while ( ch != '\r' )
	{
	    if ( read( STDIN, &ch, 1 ) != 1 )
	    {
		lprintf( L_ERROR, "logname/FIDO: read error" );
		exit(0);
	    }
	    if ( i < maxsize) buf[i++] = ch;
	}
    }
#endif
	
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
#endif			/* !ENV_TTYPROMPT */
}
