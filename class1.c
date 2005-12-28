#ident "$Id: class1.c,v 4.4 2005/12/28 21:53:08 gert Exp $ Copyright (c) Gert Doering"

/* class1.c
 *
 * High-level functions to handle class 1 fax -- 
 * state machines for fax phase A, B, C, D. Error recovery.
 *
 * Uses library functions in class1lib.c, faxlib.c and modem.c
 *
 * $Log: class1.c,v $
 * Revision 4.4  2005/12/28 21:53:08  gert
 * fix some compiler warnings and typos
 * adapt to changed fax1_send_idframe()
 * add CVS header
 * add (very rough and incomplete) fax class 1 implementation, consisting
 *  - fax1_receive() (to be called from faxrec.c)
 *  - fax1_receive_tcf() (handle TCF training frame + responses)
 *  - fax1_receive_page() (setup page reception, hand to fax_get_page_data())
 *
 */

#ifdef CLASS1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "mgetty.h"
#include "fax_lib.h"
#include "tio.h"
#include "class1.h"

enum T30_phases { Phase_A, Phase_B, Phase_C, Phase_D, Phase_E } fax1_phase;

int fax1_dial_and_phase_AB _P2( (dial_cmd,fd),  char * dial_cmd, int fd )
{
char * p;			/* modem response */
uch framebuf[FRAMESIZE];
int first;

    /* send dial command */
    if ( fax_send( dial_cmd, fd ) == ERROR )
    {
	fax_hangup = TRUE; fax_hangup_code = FHUP_ERROR;
	return ERROR;
    }

    /* wait for ERROR/NO CARRIER/CONNECT */
    signal( SIGALRM, fax1_sig_alarm );
    alarm(FAX_RESPONSE_TIMEOUT);

    while( !fax_hangup )
    {
        p = mdm_get_line ( fd );

	if ( p == NULL )
	    { lprintf( L_ERROR, "fax1_dial: hard error dialing out" );
	      fax_hangup = TRUE; fax_hangup_code = FHUP_ERROR; break; }

	lprintf( L_NOISE, "fax1_dial: string '%s'", p );

	if ( strcmp( p, "ERROR" ) == 0 ||
	     strcmp( p, "NO CARRIER" ) == 0 )
	    { fax_hangup = TRUE; fax_hangup_code = FHUP_ERROR; break; }

	if ( strcmp( p, "NO DIALTONE" ) == 0 ||
	     strcmp( p, "NO DIAL TONE" ) == 0 )
	    { fax_hangup = TRUE; fax_hangup_code = FHUP_NODIAL; break; }

	if ( strcmp( p, "BUSY" ) == 0 )
	    { fax_hangup = TRUE; fax_hangup_code = FHUP_BUSY; break; }

        if ( strcmp( p, "CONNECT" ) == 0 )		/* gotcha! */
	    { break; }
    }

    alarm(0);
    if ( fax_hangup ) return ERROR;

    /* now start fax negotiation (receive CSI, DIS, send DCS)
     * read all incoming frames until FINAL bit is set
     */
    first=TRUE;
    do
    {
	if ( fax1_receive_frame( fd, first? 0:3, 30, framebuf ) == ERROR )
	{
	    /*!!!! try 3 times! (flow diagram from T.30 / T30_T1 timeout) */
	    fax_hangup = TRUE; fax_hangup_code = 11; return ERROR;
	}
	switch ( framebuf[1] )		/* FCF */
	{
	    case T30_CSI: fax1_copy_id( framebuf ); break;
	    case T30_NSF: break;
	    case T30_DIS: fax1_parse_dis( framebuf ); break;
	    default:
	        lprintf( L_WARN, "unexpected frame type 0x%02x", framebuf[1] );
	}
	first=FALSE;
    }
    while( ( framebuf[0] & T30_FINAL ) == 0 );

    /* send local id frame (TSI) */
    fax1_send_idframe( fd, T30_TSI|0x01, T30_CAR_V21 );

    /* send DCS */
    if ( fax1_send_dcs( fd, 14400 ) == ERROR )
    {
        fax_hangup = TRUE; fax_hangup_code = 10; return ERROR;
    }

    fax1_phase = Phase_B;			/* Phase A done */

    return NOERROR;
}


/* fax1_send_page
 *
 * send a page of G3 data
 * - if phase is "B", include sending of TCF and possibly 
 *   baud rate stepdown and repeated transmission of DCS.
 * - if phase is "C", directly send page data
 */

int fax1_send_page _P5( (g3_file, bytes_sent, tio, ppm, fd),
		        char * g3_file, int * bytes_sent, TIO * tio,
		        Post_page_messages ppm, int fd )
{
uch framebuf[FRAMESIZE];
char * line;
char cmd[40];
char dleetx[] = { DLE, ETX };
char rtc[] = { 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x08 };
int g3fd, r, w, rx;
#define CHUNK 512
char buf[CHUNK], wbuf[CHUNK];

    /* if we're in T.30 phase B, send training frame (TCF) now...
     * don't forget delay (75ms +/- 20ms)!
     */
    if ( fax1_phase == Phase_B )
    {
        char train[150];
	int i, num;

	sprintf( cmd, "AT+FTS=8;+FTM=%d", dcs_btp->c_long );
	fax_send( cmd, fd );

	line = mdm_get_line( fd );
	if ( line != NULL && strcmp( line, cmd ) == 0 )
		line = mdm_get_line( fd );

	if ( line == NULL || strcmp( line, "CONNECT" ) != 0 )
	{
	    lprintf( L_ERROR, "fax1_send_page: unexpected response 1: '%s'", line );
	    fax_hangup = TRUE; fax_hangup_code = 20; return ERROR;
	}

	/* send data for training (1.5s worth) */

	num = (dcs_btp->speed/8)*1.5;
	lprintf( L_NOISE, "fax1_send_page: send %d bytes training (TCF)", num );
	memset( train, 0, sizeof(train));

	for( i=0; i<num; i+=sizeof(train))
		write( fd, train, sizeof(train) );
	write( fd, dleetx, 2 );

	line = mdm_get_line( fd );
	if ( line == NULL || strcmp( line, "OK" ) != 0 )
	{
	    lprintf( L_ERROR, "fax1_send_page: unexpected response 2: '%s'", line );
	    fax_hangup = TRUE; fax_hangup_code = 20; return ERROR;
	}

	/* receive frame - FTT or CFR */
	/*!!! return code! */
	fax1_receive_frame( fd, 3, 30, framebuf );

	if ( ( framebuf[0] & T30_FINAL ) == 0 ||
	     framebuf[1] != T30_CFR )
	{
	    lprintf( L_ERROR, "fax1_receive_frame: failed to train" );
	    /*!!! try 3 times! */
	    fax_hangup = TRUE; fax_hangup_code = 27;
	    return ERROR;
	}

	/* phase B done, go to phase C */
	fax1_phase = Phase_C;
    }

    if ( fax1_phase != Phase_C )
    {
        lprintf( L_ERROR, "fax1_send_page: internal error: not Phase C" );
	fax_hangup = TRUE; fax_hangup_code = FHUP_ERROR;
	return ERROR;
    }

    r=0;w=0;
    g3fd = open( g3_file, O_RDONLY );
    if ( g3fd < 0 )
    {
        lprintf( L_ERROR, "fax1_send_page: can't open '%s'", g3_file );
	/*!!! do something smart here... */
	fax_hangup = TRUE; fax_hangup_code = FHUP_ERROR;
	fax1_send_dcn( fd );
	return ERROR;
    }

    /* Phase C: send page data with high-speed carrier
     */
    sprintf( cmd, "AT+FTM=%d", dcs_btp->c_short );
    fax_send( cmd, fd );

    line = mdm_get_line( fd );
    if ( line != NULL && strcmp( line, cmd ) == 0 )
	    line = mdm_get_line( fd );

    if ( line == NULL || strcmp( line, "CONNECT" ) != 0 )
    {
	lprintf( L_ERROR, "fax1_send_page: unexpected response 3: '%s'", line );
	fax_hangup = TRUE; fax_hangup_code = 40; return ERROR;
    }

    lprintf( L_NOISE, "send page data" );

    /* read page data from file, invert byte order, 
     * insert padding bits (if scan line time > 0), 
     * at end-of-file, add RTC
     */
    /*!!!! padding, one-line-at-a-time, watch out for sizeof(wbuf)*/
    /*!!!! digifax header!*/
    rx=0; r=0; w=0;
    do
    {
        if ( rx >= r )			/* buffer empty, read more */
	{
	    r = read( g3fd, buf, CHUNK );
	    if ( r < 0 )
	    {
	    	lprintf( L_ERROR, "fax1_send_page: error reading '%s'", g3_file );
		break;
	    }
	    if ( r == 0 ) break;
	    lprintf( L_JUNK, "read %d", r );
	    rx = 0;
	}
	wbuf[w] = buf[rx++];
	if ( wbuf[w] == DLE ) wbuf[++w] = DLE;
	w++;

	/*!! zero-counting, bitpadding! */
	if ( w >= sizeof(wbuf)-2 )
	{
	    if ( w != write( fd, wbuf, w ) )
	    {
	        lprintf( L_ERROR, "fax1_send_page: can't write %d bytes", w );
		break;
	    }
	    lprintf( L_JUNK, "write %d", w );
	    w=0;
	}
    }
    while(r>0);
    close(g3fd);

    /*!!! ERROR HANDLING!! */
    /*!!! PARANOIA: alarm()!! */
    /* end of page: RTC */
    write( fd, rtc, sizeof(rtc) );
    /* end of data: DLE ETX */
    write( fd, dleetx, 2 );

    line = mdm_get_line( fd );
    if ( line == NULL || strcmp( line, "OK" ) != 0 )
    {
	lprintf( L_ERROR, "fax1_send_page: unexpected response 3a: '%s'", line );
	fax_hangup = TRUE; fax_hangup_code = 40; return ERROR;
    }


    /* now send end-of-page frame (MPS/EOM/EOP) and get pps */

    fax1_phase = Phase_D;
    lprintf( L_MESG, "page data sent, sending end-of-page frame (C->D)" );
    sprintf( cmd, "AT+FTS=8;+FTH=3" );
    fax_send( cmd, fd );

    line = mdm_get_line( fd );
    if ( line != NULL && strcmp( line, cmd ) == 0 )
	    line = mdm_get_line( fd );

    if ( line == NULL || strcmp( line, "CONNECT" ) != 0 )
    {
        if ( strcmp( line, "OK" ) == 0 ) goto tryanyway;
	lprintf( L_ERROR, "fax1_send_page: unexpected response 4: '%s'", line );
	fax_hangup = TRUE; fax_hangup_code = 50; return ERROR;
    }

    /* some modems seemingly can't handle AT+FTS=8;+FTH=3 (returning 
     * "OK" instead of "CONNECT"), so send AT+FTH=3 again for those.
     */
tryanyway:

    framebuf[0] = 0xff;
    framebuf[1] = 0x03 | T30_FINAL;
    switch( ppm )
    {
        case pp_eom: framebuf[2] = T30_EOM | fax1_dis; break;
	case pp_eop: framebuf[2] = T30_EOP | fax1_dis; break;
	case pp_mps: framebuf[2] = T30_MPS | fax1_dis; break;
	default: 
	   lprintf( L_WARN, "fax1_send_page: canthappen(1) - PRI not supported" );
    }

    fax1_send_frame( fd, strcmp(line, "OK")==0? 3:0 , framebuf, 3 );

    /* get MPS/RTP/RTN code */
    fax1_receive_frame( fd, 3, 30, framebuf );

    /*!!! T.30 flow chart... */

    switch( framebuf[1] )
    {
        case T30_MCF:		/* page good */
		fax_page_tx_status = 1; break;
	case T30_RTN:		/* retrain / negative */
		fax_page_tx_status = 2; fax1_phase = Phase_B; break;
	case T30_RTP:		/* retrain / positive */
		fax_page_tx_status = 3; fax1_phase = Phase_B; break;
	case T30_PIN:		/* procedure interrupt */
		fax_page_tx_status = 4; break;
	case T30_PIP:
		fax_page_tx_status = 5; break;
	default:
		lprintf( L_ERROR, "fax1_transmit_page: unexpected frame" );
		fax_hangup = TRUE; fax_hangup_code = 53; 
		fax1_send_dcn(fd); break;
    }

    fax_hangup = TRUE; fax_hangup_code = 50;
    return ERROR;
}

int fax1_receive _P6( (fd, pagenum, dirlist, uid, gid, mode ),
		       int fd, int * pagenum, char * dirlist,
		       int uid, int gid, int mode)
{
int rc;
uch frame[FRAMESIZE];
char * p;
Post_page_messages ppm = pp_mps;

    /* after ATA/CONNECT, first AT+FTH=3 is implicit -> send frames
       right away*/

    /* with +FAE=1, the sequence seems to be
     *  RING
     *     ATA
     *  FAX
     *  CONNECT
     */

    /* TODO: reasonable timeout! */
    alarm(5);
    p = mdm_get_line( STDIN );
    alarm(0);

    if ( p == NULL || strcmp( p, "CONNECT" ) != 0 )
    {
	lprintf( L_WARN, "fax1_receive: initial CONNECT not seen" );
	return ERROR;
    }

    *pagenum = 0;

    /* send local ID frame (CSI) - non-final */
    fax1_send_idframe( STDIN, T30_CSI, T30_CAR_SAME );

    /* send DSI = Digital Identification Signal - local capabilities */
    frame[0] = 0xff;
    frame[1] = 0x03 | T30_FINAL;
    frame[2] = T30_DIS;
    frame[3] = 0x00;		/* bits 1..8: group 1/2 - unwanted */
    frame[4] = 0x00 |		/* bit 9: can transmit */
	       0x02 |           /* bit 10: can receive */
	       0x2c |		/* bit 11..14: receive rates - TODO!! */
	       0xc0;		/* bit 15: can fine, bit 16: can 2D */
    frame[5] = 0x08 |		/* bits 17..20 = 215mm width, unlim. length */
               0x70 |		/* bits 21..23 = 0ms scan time */
	       0x00;		/* bit 24: extend bit - final */

    rc = fax1_send_frame( STDIN, T30_CAR_SAME, frame, 6 );

    /* now see what the other side has to say... */
wait_for_dcs:

    do
    {
        if ( fax1_receive_frame( STDIN, T30_CAR_V21, 30, frame ) == ERROR )
	{
	    fax_hangup = TRUE; fax_hangup_code = 70; return ERROR;
	    /* TODO: need to re-try initial handshake 3 times! */
	}
	switch( frame[1] & 0xfe )		/* FCF, ignoring X-bit */
	{
	    case T30_TSI: fax1_copy_id( frame ); break;
	    case T30_NSF: break;
	    case T30_DCS: lprintf( L_MESG, "DCS!! TODO!!" ); break;
	    default:
		lprintf( L_WARN, "unexpected frame type 0x%02x", frame[1] );
	}
    }
    while( ( frame[0] & T30_FINAL ) == 0 );

    /* parse&print negotiated values */

    /* receive 1.5s training sequence */
    rc = fax1_receive_tcf( STDIN, 96 );

    if ( rc < 0 ) return ERROR;

    /* TCF bad? send FTT (failure to train), wait for next DCS */
    frame[0] = 0xff;
    frame[1] = 0x03 | T30_FINAL;

    if ( rc == 0 )
    {
	frame[2] = T30_FTT;
	rc = fax1_send_frame( STDIN, T30_CAR_V21, frame, 3 );
	goto wait_for_dcs;
	/* TODO!!! break endless loop */
    }

    /* TCF good, send CFR frame (confirmation to receive) */
    frame[2] = T30_CFR;
    rc = fax1_send_frame( STDIN, T30_CAR_V21, frame, 3 );

receive_next_page:

    /* start page reception & get page data */
    fax1_receive_page( STDIN, 96, pagenum, dirlist, uid, gid, mode );

    /* switch back to low-speed carrier, get (PRI-)EOM/MPS/EOP code */

    /* TODO: T.30 flow chart: will we ever hit non-final frames here?
     * what to do on error?
     */
    do
    {
        if ( fax1_receive_frame( STDIN, T30_CAR_V21, 30, frame ) == ERROR )
	{
	    fax_hangup = TRUE; fax_hangup_code = 100; return ERROR;
	    /* TODO: need to re-try post-page handshake 3 times! */
	}
	switch( frame[1] & 0xfe & ~T30_PRI )	/* FCF, ignoring X-bit */
	{
	    case T30_MPS: 
	        lprintf( L_MESG, "MPS: end of page" ); ppm = pp_mps;
	        break;
	    case T30_EOM: 
		lprintf( L_MESG, "EOM: back to phase B" ); ppm = pp_eom;
		break;
	    case T30_EOP: 
		lprintf( L_MESG, "EOP: end of transmission" ); 
		ppm = pp_eop;
		break;
	    default:
		lprintf( L_WARN, "unexpected frame type 0x%02x", frame[1] );
	}
    }
    while( ( frame[0] & T30_FINAL ) == 0 );

    /* send back page good/bad return code */
    frame[0] = 0xff;
    frame[1] = 0x03 | T30_FINAL;
    frame[2] = T30_MCF;
    rc = fax1_send_frame( STDIN, T30_CAR_V21, frame, 3 );

    /* go back to phase B (EOM), go to next page (MPS), done (EOP) */
    if ( ppm == pp_eom )
	{ lprintf( L_ERROR, "EOM: not implemented (TODO!)" ); return ERROR; }
    if ( ppm == pp_mps )
	{ goto receive_next_page; }

    /* EOP - get goodbye frame (DCN) from remote end, hang up */
    fax1_receive_frame( STDIN, T30_CAR_V21, 30, frame );

    if ( (frame[1] & 0xfe) != T30_DCN )
    {
	lprintf( L_WARN, "fax1_receive: unexpected frame 0x%02x 0x%02x after EOP", frame[0], frame[1] );
    }

    return NOERROR;
}

int fax1_receive_tcf _P2((fd,carrier), int fd, int carrier)
{
int rc, count, notnull;
char c, *p;
boolean wasDLE=FALSE;

    /* TODO: proper timeout settings as per T.30 */
    alarm(10);

    lprintf( L_NOISE, "fax1_r_tcf: carrier=%d", carrier );
    rc = fax1_init_FRM( fd, carrier );
    if ( rc == ERROR ) return 0;		/* re-try */

    count = notnull = 0;

    lprintf( L_JUNK, "fax1_r_tcf: got: " );
    while(1)
    {
	if ( mdm_read_byte( fd, &c ) != 1 ) 
	{
	    lprintf( L_ERROR, "fax1_r_tcf: cannot read byte, return" );
	    return -1;
	}
	if ( c != 0 ) lputc( L_JUNK, c );

	if ( wasDLE ) 
	{
	    if ( c == ETX ) break;
	    wasDLE = 0;
	}
	if ( c == DLE ) { wasDLE = 1; continue; }

	count++;
	if ( c != 0 ) notnull++;
    }

    /* read post-frame "NO CARRIER" message */
    p = mdm_get_line( fd );
    alarm(0);

    if ( notnull > count/10 ) 
    {
	lprintf( L_NOISE, "TCF: %d bytes, %d non-null, retry", count, notnull );
	return 0;				/* try again */
    }

    lprintf( L_NOISE, "TCF: %d bytes, %d non-null, OK", count, notnull );
    return 1;					/* acceptable, go ahead */
}

int fax1_receive_page _P7( (fd,carrier,pagenum,dirlist,uid,gid,mode), 
				int fd, int carrier,
				int * pagenum, char * dirlist,
				int uid, int gid, int mode )
{
int rc;
char *p;

char directory[MAXPATH];

    fax_find_directory( dirlist, directory, sizeof(directory) );

    /* TODO: proper timeout settings as per T.30 */
    alarm(10);

    lprintf( L_NOISE, "fax1_rp: carrier=%d", carrier );
    rc = fax1_init_FRM( fd, carrier );
    if ( rc == ERROR ) return ERROR;

    /* now get page data (common function for class 1 and class 2) */
    /* TODO: reset alarm handler! */
    rc = fax_get_page_data( STDIN, ++(*pagenum), directory, uid, gid, mode );

    /* read post-frame "NO CARRIER" message */
    p = mdm_get_line( fd );
    alarm(0);

    /* TODO: copy quality checking */

    return NOERROR;
}

#endif /* CLASS 1 */
