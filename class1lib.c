#ident "$Id: class1lib.c,v 4.2 1997/12/20 17:11:53 gert Exp $ Copyright (c) Gert Doering"

/* class1lib.c
 *
 * Low-level functions to handle class 1 fax -- 
 * send a frame, receive a frame, dump frame to log file, ...
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "mgetty.h"
#include "fax_lib.h"
#include "tio.h"
#include "class1.h"

/* static variables
 *
 * only set by functions in this module and used by other functions, 
 * but have to be module-global
 */

#define F1LID 20
static char fax1_local_id[F1LID];	/* local system ID */
static int fax1_min, fax1_max;		/* min/max speed */
static int fax1_res;			/* flag for normal resolution */

int fax1_set_l_id _P2( (fd, fax_id), int fd, char * fax_id )
{
    int i,l;
    char *p;

    l = strlen( fax_id );
    if ( l > F1LID ) { l = F1LID; }

    /* bytes are transmitted in REVERSE order! */
    p = &fax1_local_id[F1LID-1];

    for ( i=0; i<l; i++ )     *(p--) = *(fax_id++);
    for (    ; i<F1LID; i++ ) *(p--) = ' ';

    return NOERROR;
}


/* set fine/normal resolution flags and min/max transmission speed
 * TODO: find out maximum speed modem can do!
 */
int fax1_set_fdcc _P4( (fd, fine, max, min),
		       int fd, int fine, int max, int min )
{
    char * p;

    lprintf( L_MESG, "fax1_set_fdcc: fine=%d, max=%d, min=%d", fine, max, min );

    fax1_max = max/2400 -1;
    fax1_min = (min>2400)? min/2400 -1: 0;
    fax1_res = fine;

    lprintf( L_MESG, "max: %d, min: %d", fax1_max, fax1_min );

    if ( fax1_max < fax1_min ||
	 fax1_max < 1 || fax1_max > 5 ||
	 fax1_min < 0 || fax1_min > 5 ) 
    {
	fax1_min = 0; fax1_max = 5;
	return ERROR;
    }

    if ( fax1_res < 0 || fax1_res > 1 ) 
    {
	fax1_res = 1;
	return ERROR;
    }

    p = mdm_get_idstring( "AT+FTH=?", 1, fd );
    lprintf( L_MESG, "modem can send HDLC headers: '%s'", p );

    p = mdm_get_idstring( "AT+FTM=?", 1, fd );
    lprintf( L_MESG, "modem can send page data: '%s'", p );

    return NOERROR;
}

int fax1_set_bor _P2( (fd, bor), int fd, int bor )
{
    /*!!! TODO - what TODO ??! */
    return NOERROR;
}

/* timeout handler
 */
static boolean fax1_got_timeout = FALSE;

RETSIGTYPE fax1_sig_alarm(SIG_HDLR_ARGS)
{
    signal( SIGALRM, fax1_sig_alarm );
    lprintf( L_WARN, "Warning: fax1: got alarm signal!" );
    fax1_got_timeout = TRUE;
}

/* receive ONE frame, put it into *framebuf
 *
 * timeout set to "timout * 1/10 seconds"
 */
int fax1_receive_frame _P4 ( (fd, carrier, timeout, framebuf),
			     int fd, int carrier, 
			     int timeout, char * framebuf)
{
    int count=0;			/* bytes in frame */
    int rc = NOERROR;			/* return code */
    char gotsync = FALSE;		/* got 0xff frame sync */
    char WasDLE = FALSE;		/* got <DLE> character */
    char * line, c;

    if ( timeout > 0 )
    {
	signal( SIGALRM, fax1_sig_alarm );
        alarm( (timeout/10)+1 );
    }

    if ( carrier > 0 )
    {
    char cmd[20];
	sprintf( cmd, "AT+FRH=%d", carrier );
	fax_send( cmd, fd );
	/*!!! DO NOT USE fax_send (FAX_COMMAND_DELAY) */

	/* wait for CONNECT/NO CARRIER */
	line = mdm_get_line( fd );
	if ( line != NULL && strcmp( line, cmd ) == 0 )
		    { line = mdm_get_line( fd ); }		/* skip echo */

	if ( line == NULL || strcmp( line, "CONNECT" ) != 0 )
	{
	    alarm(0);
	    lprintf( L_WARN, "fax1_receive_frame: no carrier (%s)", line );
	    return ERROR;
	}
    }

    lprintf( L_NOISE, "fax1_receive_frame: got:" );

    /* we have a CONNECT now - now find the first byte of the frame
     * (0xFF), and read in <DLE> shielded data up to the <DLE><ETX>
     */

    while(1)
    {
        if ( mdm_read_byte( fd, &c ) != 1 )
	{
	    lprintf( L_ERROR, "fax1_get_frame: cannot read byte, return" );
	    rc = ERROR; break;
	}

	/*!!!! ERROR statt Frame-Daten erkennen */

	lputc( L_NOISE, c );

	if ( !gotsync ) 		/* wait for preamble */
	{
	    if ( c == (char) 0xFF ) gotsync = TRUE;
	    continue;
	}

	/* got preamble, all further bytes are put into buffer */

	/* enough room? */
	if ( count >= FRAMESIZE-5 )
	{
	    lprintf( L_ERROR, "fax1_get_frame: too many octets in frame" );
	    rc = ERROR; break;
	}

	if ( WasDLE )			/* previous character was DLE */
	{
	    if ( c == DLE )		/* DLE DLE -> DLE */
		{ framebuf[count++] = DLE; }
	    else if ( c == SUB )	/* DLE SUB -> DLE DLE */
	        { framebuf[count++] = DLE; framebuf[count++] = DLE; }
	    else if ( c == ETX )	/* end of frame detected */
	        { rc = count; break; }
	    
	    WasDLE = 0;
	    continue;
	}

	/* previous character was not DLE, check for DLE now... */
	if ( c == DLE )
	{  
	    WasDLE = 1; continue;
	}

	/* all other characters are stored in buffer */
	framebuf[count++] = c;
    }

    /*!!! nur, wenn nicht schon "ERROR" !!!*/

    /* now read OK / ERROR response codes */
    line = mdm_get_line( fd );

    if ( line == NULL ||			/* timeout ... */
         strcmp( line, "ERROR" ) == 0 )		/* or FCS error */
    {
	lprintf( L_MESG, "fax1_receive_frame: dropping frame" );
        rc = ERROR;
    }

    /* turn off alarm */
    alarm(0);

    if ( rc > 0 )
    {
        fax1_dump_frame( framebuf, count );
    }

    return rc;
}


void fax1_dump_frame _P2((frame, len), unsigned char * frame, int len)
{
int fcf = frame[1];

    lprintf( L_MESG, "frame type: 0x%02x  len: %d  %s",
                      fcf, len, frame[0]&0x10? "final": "non-final" );

    if ( fcf & 0x0e ) fcf &= ~0x01;	/* clear "X" bit */

    switch( fcf )
    {
    	/* simple frames */
	case T30_CSI:
	    lprintf( L_NOISE, "CSI: '%20.20s'", &frame[2] ); break;
	case T30_CIG:
	    lprintf( L_NOISE, "CIG: '%20.20s'", &frame[2] ); break;
	case T30_TSI:
	    lprintf( L_NOISE, "TSI: '%20.20s'", &frame[2] ); break;
	case T30_NSF:
	    lprintf( L_NOISE, "NSF" ); break;
	case T30_CFR:
	    lprintf( L_NOISE, "CFR" ); break;
	case T30_FTT:
	    lprintf( L_NOISE, "FTT" ); break;
	case T30_MCF:
	    lprintf( L_NOISE, "MCF" ); break;
	case T30_RTP:
	    lprintf( L_NOISE, "RTP" ); break;
	case T30_RTN:
	    lprintf( L_NOISE, "RTN" ); break;
	case T30_DCN:
	    lprintf( L_NOISE, "DCN" ); break;

	/* complicated ones... */
        case T30_DIS:
	    lprintf( L_NOISE, "DIS:" ); 
	    if ( frame[2] & 0x40 ) lputs( L_NOISE, " V8" );
	    lputs( L_NOISE, frame[2] & 0x80 ? " 64": " 256" );

	    if ( frame[3] & 0x01 ) lputs( L_NOISE, " +FPO" );
	    if ( frame[3] & 0x02 ) lputs( L_NOISE, " RCV" );

	    switch( (frame[3] >> 2) &0x0f )
	    {
	        case 0x00: lputs( L_NOISE, " V27ter_fb" ); break;
		case 0x02: lputs( L_NOISE, " V27ter" ); break;
		case 0x01: lputs( L_NOISE, " V29" ); break;
		case 0x03: lputs( L_NOISE, " V27ter+V29" ); break;
		case 0x0b: lputs( L_NOISE, " V27ter+V29+V17" ); break;
		default:   lputs( L_NOISE, " V.???" ); break;
	    }

	    if ( frame[3] & 0x40 ) lputs( L_NOISE, " 200" );
	    if ( frame[3] & 0x80 ) lputs( L_NOISE, " 2D" );

            switch( frame[4] & 0x03 )
	    {
	        case 0x00: lputs( L_NOISE, " 215mm" ); break;
		case 0x01: lputs( L_NOISE, " 215+255+303" ); break;
		case 0x02: lputs( L_NOISE, " 215+255" ); break;
	    }
	    switch( (frame[4]>>2) & 0x03 )
	    {
	        case 0x00: lputs( L_NOISE, " A4" ); break;
		case 0x01: lputs( L_NOISE, " unlim" ); break;
		case 0x02: lputs( L_NOISE, " A4+B4" ); break;
	    }
	    switch( (frame[4]>>4) & 0x07 )
	    {
	        case 0x00: lputs( L_NOISE, " 20ms" ); break;
	        case 0x01: lputs( L_NOISE, " 40ms" ); break;
	        case 0x02: lputs( L_NOISE, " 10ms" ); break;
	        case 0x04: lputs( L_NOISE, " 5ms" ); break;
	        case 0x03: lputs( L_NOISE, " 5/10ms" ); break;
	        case 0x06: lputs( L_NOISE, " 10/20ms" ); break;
	        case 0x05: lputs( L_NOISE, " 20/40ms" ); break;
	        case 0x07: lputs( L_NOISE, " 0ms" ); break;
	    }
	    if ( ( frame[4] & 0x80 ) == 0 ) break;	/* extent bit */

	    if ( frame[5] & 0x04 ) lputs( L_NOISE, " ECM" );
	    if ( frame[5] & 0x40 ) lputs( L_NOISE, " T.6" );
	    if ( ( frame[5] & 0x80 ) == 0 ) break;	/* extent bit */

	    if ( ( frame[6] & 0x80 ) == 0 ) break;	/* extent bit */
	    /* the next bytes specify 300/400 dpi, color fax, ... */

	    break;
	case T30_DCS:
	    lprintf( L_NOISE, "DCS:" ); break;
    }
}

/* send arbitrary frame
 */
int fax1_send_frame _P4( (fd, carrier, frame, len), 
                         int fd, int carrier, char * frame, int len )
{
char * line;
char frame_eof[] = { DLE, ETX };
static carrier_active = -1;		/* inter-frame marker */

    /* send AT+FTH=3, wait for CONNECT 
     * (but only if we've not sent an non-final frame befor!)
     */
    if ( carrier > 0 && carrier_active != carrier )
    {
    char cmd[20];
	sprintf( cmd, "AT+FTH=%d", carrier );
	fax_send( cmd, fd );		/*!!!!! NOOOO */

	/* wait for CONNECT/NO CARRIER */
	line = mdm_get_line( fd );
	if ( line != NULL && strcmp( line, cmd ) == 0 )
		    { line = mdm_get_line( fd ); }		/* skip echo */

	if ( line == NULL || strcmp( line, "CONNECT" ) != 0 )
	{
	    alarm(0);
	    lprintf( L_WARN, "fax1_send_frame: no carrier (%s)", line );
	    carrier_active=-1;
	    return ERROR;
	}

	carrier_active=carrier;
    }

    fax1_dump_frame( frame+1, len-1 );

    /* send <DLE> encoded frame data */

    /*!!! DLE CODING */
    write( fd, frame, len );

    /* end of frame: <DLE><ETX> */
    write( fd, frame_eof, 2 );

    /*!!! alarm */
    /*!!! LASAT schickt "CONNECT\r\nOK" bzw. nur "OK" (final/non-final)
     *    --> ist das normal und richtig so??!?
     *
     * Nein... - es kommt immer entweder-oder, aber nach CONNECT muss
     * man OHNE neues AT+FTH *SOFORT* weitersenden!
     */
    line = mdm_get_line( fd );
    lprintf( L_NOISE, "fax_send_frame: got '%s'", line );

    if ( frame[1] & T30_FINAL )
    {
        carrier_active = -1;		/* carrier is off */
	lprintf( L_NOISE, "carrier is off - OK='%s'", line );
    }

#if 0
    if ( line != NULL && strcmp( line, "CONNECT" ) == 0 )
    {
	line = mdm_get_line( fd );
	lprintf( L_NOISE, "fax_send_carrier: got '%s'", line );
    }
#endif

    return NOERROR;
}

/* send "disconnect now" frame 
 * Note: this is always a "final" frame
 */
int fax1_send_dcn _P1((fd), int fd )
{
    char frame[] = { 0xff, 0x13, T30_DCN };
    /*!! FIXME - set bit LSB of DCN if necessary */

    fax_hangup = TRUE;

    return fax1_send_frame( fd, 3, frame, sizeof(frame) );
}

/* send local identification (CSI, CIG or TSI) 
 * Note: "final" bit is never set, as these frames are always optional.
 */
int fax1_send_idframe _P2((fd,fcf), int fd, int fcf )
{
    unsigned char frame[F1LID+3];

    frame[0] = 0xff;
    frame[1] = 0x03;
    frame[2] = fcf;
    memcpy( &frame[3], fax1_local_id, F1LID );

    return fax1_send_frame( fd, 3, frame, sizeof(frame) );
}
