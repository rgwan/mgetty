#ident "$Id: xmodem.c,v 1.1 1999/03/31 21:03:22 gert Exp $"

/* xmodem.c
 *
 * XModem-primitives for uploading and downloading from an already 
 * open file descriptor (no terminal handling etc)
 *
 * warning: these functions use alarm() and mess up SIGALRM handlers
 *
 * $Log: xmodem.c,v $
 * Revision 1.1  1999/03/31 21:03:22  gert
 * XModem primitives - first cut, downloading works
 *
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#ifndef ENOENT
# include <errno.h>
#endif

#include "mgetty.h"

/* defines for the XModem protocol */
#define ACK	0x06			/* Acknowledge */
#define NAK	0x15			/* Not Acknowledge */
#define CAN	0x18			/* CANcel transmission (ctrl-X) */
#define SOH	0x01			/* start of block, 128byte */
#define STX	0x02			/* start of block, 1024byte */
#define EOT	0x04			/* end of transmission */

static char c_ack = ACK, c_nak = NAK;	/* to be used for write() */

static int x_errcnt=0;			/* error counter */
static int x_eotcnt=0;			/* EOT counter */
       int xmodem_blk=0;		/* current block number */

/* message function is specified by a pointer to a (char *, ...) function */
typedef void (*msg_func_t)(char *,...);

/* semi-globals */
static int x_fd;			/* file descriptor to "modem" */
static msg_func_t x_msg_func;		/* message function for logging */

/* helper for x_timeout handling */
static int x_timeout=0;
static void x_sigalarm(int s)
{ 
    signal(SIGALRM, x_sigalarm); 
    x_timeout=1;
}

/* initialize XModem receiver
 */
int xmodem_rcv_init _P3((fd, msg_func, buf),
			int fd, msg_func_t msg_func, char *buf)
{
    x_fd = fd;
    x_msg_func = msg_func;
    signal(SIGALRM, x_sigalarm);

    xmodem_blk = 1;
    x_errcnt = x_eotcnt = 0;

    x_nak();
    return xmodem_rcv_block(buf);
}

/* get next block
 */
int xmodem_rcv_block _P1((buf), unsigned char *buf)
{
unsigned char ch;
int got_can=0;
int len, i, chks;

again:
    alarm(10); 

    lprintf( L_NOISE, "xmodem_rcv_block, got:" );

    do
    {
	if ( read( x_fd, &ch, 1 ) < 1 ) { goto send_nak; }
	lputc( L_NOISE, ch );

	if ( ch == CAN ) { if ( ++got_can > 1 ) { x_errcnt=10; return -1;}}
		    else { got_can=0; }

	if ( ch == EOT ) 
	{ 
	    x_eotcnt++;
	    if ( x_eotcnt < 2 )		/* false alarm? NAK! */
	    	goto send_nak;
	    return 0;			/* EOF */
	}
    }
    while( ch != SOH && ch != STX );

    len = ( ch == SOH )? 128: 1024;
    lprintf( L_NOISE, "got SOH/STX, block size=%d", len );

    if ( read( x_fd, &ch, 1 ) < 1 || ch != (xmodem_blk & 0xff) ) 
	{ fprintf( stderr, "Block # %d doesn't match %d\n", ch, xmodem_blk); 
	  goto send_nak; }
    if ( read( x_fd, &ch, 1 ) < 1 || ch != 255-(xmodem_blk & 0xff) ) 
	{ fprintf( stderr, "!Block # %d doesn't match %d\n", ch, xmodem_blk); 
	  goto send_nak; }
    
    lprintf( L_NOISE, "block number %d ok, got:", xmodem_blk );
    chks=0;

    for( i=0; i<len; i++ )
    {
	if ( read( x_fd, &buf[i], 1 ) < 1 ) { return -1; }
	chks += buf[i];
    }

    if ( read( x_fd, &ch, 1 ) < 1 || ch != (chks &0xff)) 
	{ fprintf( stderr, "Chksum %02x doesn't match %02x\n", ch, chks); 
	  goto send_nak; }

    alarm(0);
    
    lprintf( L_NOISE, "Block %d len %d ok, send ACK\n", xmodem_blk, len );
    write( x_fd, &c_ack, 1 );
    xmodem_blk++;
    return len;

send_nak:
    x_nak();
    x_errcnt++;
    if ( x_errcnt<10 ) goto again;

    alarm(0);
    return -1;
}

static int x_nak(void)
{
    char ch;

    lprintf( L_JUNK, "x_nak: drain queue, send NAK, got:" );

    /* drain input queue - make sure we're not in the middle of a block
     */
    x_timeout=0;
    while(!x_timeout)
    { 
	alarm(1);
	if ( read( x_fd, &ch, 1 ) != 1 && errno != EINTR ) 
		{ alarm(0); return -1; }
	lputc( L_JUNK, ch );
    }

    /* send NAK */
    return write( x_fd, &c_nak, 1 );
}

