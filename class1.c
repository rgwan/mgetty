#ident "$Id: class1.c,v 4.1 1997/12/20 17:11:52 gert Exp $ Copyright (c) Gert Doering"

/* class1.c
 *
 * High-level functions to handle class 1 fax -- 
 * state machines for fax phase A, B, C, D. Error recovery.
 *
 * Usese library functions in class1lib.c, faxlib.c and modem.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "mgetty.h"
#include "fax_lib.h"
#include "tio.h"
#include "class1.h"

enum T30_phases { Phase_A, Phase_B, Phase_C, Phase_D, Phase_E } fax1_phase;

int fax1_dial_and_phase_AB _P2( (dial_cmd,fd),  char * dial_cmd, int fd )
{
char * p;			/* modem response */
char framebuf[FRAMESIZE];

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

    /* now start fax negotiation (receive CSI, DIS, send DCS) */
    fax1_receive_frame( fd, 0, 30, &framebuf );

    /* read further frames until FINAL bit is set */
    while( ( framebuf[0] & T30_FINAL ) == 0 )
	fax1_receive_frame( fd, 3, 30, &framebuf );

    /* send local id frame (TSI) */
    fax1_send_idframe( fd, T30_TSI|0x01 );

    /* send DCS */
    /*!!!! use values from AT+FTH=? and received DIS! */
    framebuf[0] = 0xff;
    framebuf[1] = 0x03 | T30_FINAL;
    framebuf[2] = 0x01 | T30_DCS;		/*!!! received_dis | ... */
    framebuf[3] = 0;				/* bits 1-8 */
    framebuf[4] = 0 | 0 | 0x04 | 0x40 | 0;	/* bits 9-16 */
    framebuf[5] = 0 | 0x04 | 0x70 | 0;		/* bits 17-24 */
    fax1_send_frame( fd, 3, framebuf, 6 );

    /* send DCN - hang up */
    /* fax1_send_dcn( fd ); */
    
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
unsigned char framebuf[FRAMESIZE];
char * line;
char cmd[40];
char dleetx[] = { DLE, ETX };

    /* if we're in T.30 phase B, send training frame (TCF) now...
     * don't forget delay (75ms +/- 20ms)!
     */
    if ( fax1_phase == Phase_B )
    {
        char train[100];
	int i;

	sprintf( cmd, "AT+FTS=8;+FTM=96" );		/*!!! PROPER CARRIER */
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
	/*!!! move to class1lib.c */
	memset( train, 0, sizeof(train));

	/*!!! PROPER number of bytes for carrier! */
	for( i=0; i<18; i++)
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
	fax1_receive_frame( fd, 3, 30, &framebuf );

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

    /* Phase C: send page data with high-speed carrier
     */

    /*!!! user PROPER carrier */
    sprintf( cmd, "AT+FTM=96" );		/*!!! PROPER CARRIER */
    fax_send( cmd, fd );

    line = mdm_get_line( fd );
    if ( line != NULL && strcmp( line, cmd ) == 0 )
	    line = mdm_get_line( fd );

    if ( line == NULL || strcmp( line, "CONNECT" ) != 0 )
    {
	lprintf( L_ERROR, "fax1_send_page: unexpected response 3: '%s'", line );
	fax_hangup = TRUE; fax_hangup_code = 40; return ERROR;
    }


    /* end of page: DLE ETX */
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
	lprintf( L_ERROR, "fax1_send_page: unexpected response 4: '%s'", line );
	fax_hangup = TRUE; fax_hangup_code = 50; return ERROR;
    }

    framebuf[0] = 0xff;
    framebuf[1] = 0x03 | T30_FINAL;
    framebuf[2] = 0x01 | T30_EOP;		/*!!! receive_dis */
    fax1_send_frame( fd, 0, framebuf, 6 );

    /* get MPS/RTP/RTN code */
    fax1_receive_frame( fd, 3, 30, framebuf );

    /*!! say goodbye */
    fax1_send_dcn( fd );

    fax_hangup = TRUE; fax_hangup_code = 50;
    return ERROR;
}

