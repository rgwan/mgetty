#ident "$Id: xmodem.c,v 1.2 1999/04/05 20:43:15 gert Exp $"

/* xmodem.c
 *
 * XModem-primitives for uploading and downloading from an already 
 * open file descriptor (no terminal handling etc)
 *
 * warning: these functions use alarm() and mess up SIGALRM handlers
 *
 * $Log: xmodem.c,v $
 * Revision 1.2  1999/04/05 20:43:15  gert
 * - add ELSA-EOT (NAKing EOT doesn't work, recognize 0d/0a/OK/0d/0a
 * - add "all-in-one" download function
 * - fix timeout bug
 *
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
int got_can=0, ok_seq_cnt=0;
int len, i, chks;
static char ok_seq[] = { 0x0d, 0x0a, 'O', 'K', 0x0d, 0x0a };

again:
    alarm(10); 

    lprintf( L_NOISE, "xmodem_rcv_block, got:" );

    do
    {
	if ( read( x_fd, &ch, 1 ) < 1 ) { goto send_nak; }
	lputc( L_NOISE, ch );

	/* if we get two CANs in sequence, give up
	 */
	if ( ch == CAN ) 
	    { lprintf( L_NOISE, "<CAN>" );
	      if ( ++got_can > 1 ) { x_errcnt=10; return -1;}}
	else 
	    { got_can=0; }

	/* the XModem protocol description suggests that the first EOT
	 * should be NAKed, and the second one should be accepted - 
	 * problem with that is, the ELSA doesn't send a second EOT,
	 * so we accept \r\nOK\r\n as "second EOT"
	 */
	if ( ch == EOT ) 
	{ 
	    x_eotcnt++;
	    if ( x_eotcnt < 2 )		/* false alarm? NAK! */
	    	goto send_nak;
	    return 0;			/* EOF */
	}

	if ( ch == ok_seq[ok_seq_cnt] )
	{
	    ok_seq_cnt++;
	    lputs( L_JUNK, "<s>" );
	    if ( ok_seq_cnt >= 6 )
	    	{ lprintf( L_NOISE, "ELSA-EOT" ); return 0; }
	}
	else ok_seq_cnt= ( ch == ok_seq[0] );
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
    
    lprintf( L_NOISE, "expected seq.# %d ok, got:", xmodem_blk );
    chks=0;

    for( i=0; i<len; i++ )
    {
	if ( read( x_fd, &buf[i], 1 ) < 1 ) { goto send_nak; }
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
    lprintf( L_MESG, "too many NAKs, giving up" );
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

int xmodem_rcv_file _P3(( modem_fd, out_fd, msg_func),
			 int modem_fd, int out_fd, msg_func_t msg_func )
{
int s;
char buf[1030];

    s = xmodem_rcv_init( modem_fd, msg_func, buf );

    if ( s <= 0 ) 
    {
        lprintf( L_ERROR, "XModem startup failed" );
        fprintf( stderr, "XModem startup failed\n" );
	close(out_fd);
	return -1;
    }

    do
    {
    	if ( write( out_fd, buf, s ) != s ) 
	{
	    lprintf( L_ERROR, "can't write %d bytes to file: %s",
	    		s, strerror(errno) );
	    fprintf( stderr, "can't write %d bytes to file: %s\n",
	    		s, strerror(errno) );
	    close(out_fd);
	    return -1;
	}
	printf( "block #%d\r", xmodem_blk ); fflush( stdout );

        s = xmodem_rcv_block( buf );
    }
    while( s > 0 );

    if ( s < 0 )
    {
    	lprintf( L_ERROR, "can't receive expected block" );
    	fprintf( stderr, "can't receive expected block\n" );
	return -1;
    }

    close(out_fd);

    return 0;
}
