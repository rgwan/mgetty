#ident "$Id: logname.c,v 1.1 1993/02/13 16:32:09 gert Exp $ (c) Gert Doering"
#include <stdio.h>
#include <termio.h>

#include "mgetty.h"

int getlogname( struct termio * termio, char * buf, int maxsize )
{
int i;
char ch;

	/* read character by character! */
    termio->c_lflag &= ~ICANON;
    termio->c_cc[VMIN] = 1;
    termio->c_cc[VTIME] = 0;
    ioctl(STDIN, TCSETAW, termio);

newlogin:
    printf( "\r\n%s!login: ", SYSTEM );
    i = 0;
    lprintf( L_NOISE, "getlogname, read:" );
    do
    {
	if ( read( STDIN, &ch, 1 ) != 1 ) exit(0);	/* HUP / ctrl-D */
	ch = ch & 0x7f;					/* strip to 7 bit */
	lputc( L_NOISE, ch );					/* logging */

	if ( ch == CQUIT ) exit(0);
	if ( ch == CEOF )
	{
            if ( i == 0 ) exit(0); else continue;
	}

	if ( ch == CKILL ) goto newlogin;

	if ( ch == CERASE )
	{
	    if ( i > 0 )
	    {
		fputs( " \b", stdout );
		i--;
	    }
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
    }

    if ( i == 0 ) return -1;
    else return 0;
}
