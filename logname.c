#ident "$Id: logname.c,v 1.12 1993/10/08 01:30:49 gert Exp $ Copyright (c) Gert Doering"
#include <stdio.h>
#include <termio.h>
#include <unistd.h>
#include <signal.h>
#ifndef sun
#include <sys/ioctl.h>
#endif

#ifndef ENOENT
#include <errno.h>
#endif

#include "mgetty.h"
#include "policy.h"

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

static int timeouts = 0;
#ifdef MAX_LOGIN_TIME
static sig_t getlog_timeout()
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

int getlogname _P4( (prompt, termio, buf, maxsize),
		    char * prompt, struct termio * termio, char * buf,
		    int maxsize )
{
int i;
char ch;

	/* read character by character! */
    termio->c_lflag &= ~ICANON;
    termio->c_cc[VMIN] = 1;
    termio->c_cc[VTIME] = 0;
    ioctl(STDIN, TCSETAW, termio);

#ifdef MAX_LOGIN_TIME
    signal( SIGALRM, getlog_timeout );
    alarm( MAX_LOGIN_TIME );
#endif

newlogin:
    printf( "\r\n" );
#ifdef SYSTEM
    printf( prompt, SYSTEM );
#else
    {
    struct utsname un;
	uname( &un );
	printf( prompt, un.nodename );
    }
#endif
    printf( " " );

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

    buf[--i] = 0;

    if ( ch == '\n' )
    {
	putc( '\r', stdout );
    }
    else
    {
	termio->c_iflag |= ICRNL;
	termio->c_oflag |= ONLCR;
	putc( '\n', stdout );
	lprintf( L_NOISE, "input finished with '\\r', setting ICRNL ONLCR" );
    }

    if ( i == 0 ) return -1;
    else return 0;
}
