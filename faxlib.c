#ident "$Id: faxlib.c,v 4.4 1997/03/31 20:56:23 gert Exp $ Copyright (c) Gert Doering"

/* faxlib.c
 *
 * Module containing generic faxmodem functions (as: send a command, wait
 * for modem responses, parse modem responses)
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "mgetty.h"
#include "policy.h"
#include "fax_lib.h"

Modem_type modem_type = Mt_class2;	/* if uninitialized, assume class2 */

char	fax_remote_id[40];		/* remote FAX id +FTSI */
char	fax_param[1000];		/* transm. parameters +FDCS */
fax_param_t	fax_par_d;		/* fax params detailed */
char	fax_hangup = 0;
int	fax_hangup_code = FHUP_UNKNOWN;	/* hangup cause +FHNG:<xxx> */
int	fax_page_tx_status = 0;		/* +FPTS:<ppm> */
boolean	fax_to_poll = FALSE;		/* there's something to poll */
boolean fax_poll_req = FALSE;		/* poll requested */

boolean	fhs_details = FALSE;		/* +FHS:xxx with detail info */
int	fhs_lc, fhs_blc, fhs_cblc, fhs_lbc;


/* wait for a given modem response string,
 * handle all the various class 2 / class 2.0 status responses
 */

/* copy fax station id, removing quote characters (dangerous for shell!) */
static void fwf_copy_remote_id _P1( (id), char * id )
{
int w = 0;
    while ( *id && w < sizeof(fax_remote_id)-1 )
    {
        if ( *id == '"' || *id == '\'' ) fax_remote_id[w++] = '_';
                                    else fax_remote_id[w++] = *id;
        id++;
    }
    fax_remote_id[w]=0;
}

static boolean fwf_timeout = FALSE;

static RETSIGTYPE fwf_sig_alarm(SIG_HDLR_ARGS)      	/* SIGALRM handler */
{
    signal( SIGALRM, fwf_sig_alarm );
    lprintf( L_WARN, "Warning: got alarm signal!" );
    fwf_timeout = TRUE;
}

int fax_wait_for _P2( (s, fd),
		      char * s, int fd )
{
char * line;
int  ix;

    lprintf( L_MESG, "fax_wait_for(%s)", s );

    if ( fax_hangup )
    {
	lputs( L_MESG, ": already hangup!" );
	return ERROR;
    }

    fwf_timeout = FALSE;
    signal( SIGALRM, fwf_sig_alarm );

    alarm( FAX_RESPONSE_TIMEOUT );

    do
    {
	line = mdm_get_line( fd );

	if ( line == NULL )
	{
	    alarm( 0 ); signal( SIGALRM, SIG_DFL );
	    if ( fwf_timeout ) fax_hangup_code = FHUP_TIMEOUT;
	    return ERROR;
	}
	
	lprintf( L_NOISE, "fax_wait_for: string '%s'", line );

	/* find ":" character (or end-of-string) */
	for ( ix=0; line[ix] != 0; ix++ )
	    if ( line[ix] == ':' ) { ix++; break; }
	if ( line[ix] == ' ' ) ix++;

	if ( strncmp( line, "+FTSI:", 6 ) == 0 ||
	     strncmp( line, "+FCSI:", 6 ) == 0 ||
	     strncmp( line, "+FCIG:", 6 ) == 0 ||
	     strncmp( line, "+FTI:", 5 ) == 0 ||
	     strncmp( line, "+FCI:", 5 ) == 0 ||
	     strncmp( line, "+FPI:", 5 ) == 0 )
	{
	    lprintf( L_MESG, "fax_id: '%s'", line );
	    fwf_copy_remote_id( &line[ix] );
	}

	else if ( strncmp( line, "+FDCS:", 6 ) == 0 ||
		  strncmp( line, "+FCS:", 5 ) == 0 )
	{
	    lprintf( L_MESG, "transmission par.: '%s'", line );
	    strcpy( fax_param, line );
	    if ( sscanf( &fax_param[ix], "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
			 &fax_par_d.vr, &fax_par_d.br, &fax_par_d.wd,
			 &fax_par_d.ln, &fax_par_d.df, &fax_par_d.ec,
			 &fax_par_d.bf, &fax_par_d.st ) != 8 )
	    {
		lprintf( L_WARN, "cannot evaluate +FCS-Code!" );
		fax_par_d.vr = 0;
	    }
	}

	else if ( strncmp( line, "+FHNG:", 6 ) == 0 ||
		  strncmp( line, "+FHS:", 5 ) == 0 )
	{
	    /* hangup. First, set flag to indicate +FHNG: was read.
	     * The SIGHUP signal catcher will check this, and not exit.
	     * Next, reset the action for SIGHUP, to be ignore, so we
	     * (and child processes) are not interrupted while we cleanup.
	     * If the wait_for string is not "OK", return immediately,
	     * since that is all that the modem can send after +FHNG
	     */
	    fax_hangup = 1; /* set this as soon as possible */
	    /* change action for SIGHUP signals to be ignore */
#ifdef SIG_ERR
	    if ( signal( SIGHUP, SIG_IGN ) == SIG_ERR )
	    {
		lprintf( L_WARN, "fax_wait_for: cannot reset SIGHUP handler." );
	    }
#else
	    signal( SIGHUP, SIG_IGN );
#endif
	    lprintf( L_MESG, "connection hangup: '%s'", line );
	    sscanf( &line[ix], "%d", &fax_hangup_code );

	    lprintf( L_NOISE,"(%s)", fax_strerror( fax_hangup_code ));

	    if ( strcmp( s, "OK" ) != 0 ) break;
	}

	else if ( strncmp( line, "+FPTS:", 6 ) == 0 ||
		  strncmp( line, "+FPS:", 5 ) == 0 )
	{
	    /* page transmit status
	     * store into global variable (read by sendfax.c)
	     */
	    lprintf( L_MESG, "page status: %s", line );
	    sscanf( &line[ix], "%d", &fax_page_tx_status );

	    /* evaluate line count, bad line count, ... for reception */
	    fhs_lc = 9999; fhs_blc = fhs_cblc = fhs_lbc = 0;
	    fhs_details = FALSE;

	    if ( line[ix+1] == ',' &&		/* +FPS:s,lc,blc */
		 sscanf( &line[ix+2],
			 (modem_type==Mt_class2_0)?"%x,%x,%x,%x"
			                          :"%d,%d,%d,%d",
		         &fhs_lc, &fhs_blc, &fhs_cblc, &fhs_lbc ) >= 2 )
	    {
		lprintf( L_NOISE, "%d lines received, %d lines bad, %d bytes lost", fhs_lc, fhs_blc, fhs_lbc );
		fhs_details = TRUE;
	    }
	}

	else if ( strcmp( line, "+FPOLL" ) == 0 ||
		  strcmp( line, "+FPO" ) == 0 )
	{
	    /* the other side is telling us that it has a document that
	     * we can poll (with AT+FDR)
	     */
	    lprintf( L_MESG, "got +FPO -> will do polling" );
	    fax_to_poll = TRUE;
	}

	else if ( strncmp( line, "+FDTC:", 6 ) == 0 ||
		  strncmp( line, "+FTC:", 5 ) == 0 )
	{
	    /* we sent a +FLPL=1, and the other side wants to poll
	     * that document now (send it with AT+FDT)
	     */
	    lprintf( L_MESG, "got +FTC -> will send polled document" );
	    fax_poll_req = TRUE;
	    
	    /* we're waiting for a CONNECT here, in response to a
	     * AT+FDR command, but only an OK will come. So, change
	     * expect string to "OK"
	     */
	    lprintf( L_MESG, "fax_wait_for('OK')" );
	    s = "OK";
	}
	
	else
	if ( strcmp( line, "ERROR" ) == 0 ||
	     strcmp( line, "NO CARRIER" ) == 0 ||
	     strcmp( line, "BUSY" ) == 0 ||
	     strcmp( line, "NO DIALTONE" ) == 0 ||
	     strcmp( line, "NO DIAL TONE" ) == 0 )
	{
#if 0		/* not needed right now (fax_send_ppm), problem with USR!! */
	    if ( modem_type == Mt_class2_0 )		/* valid response */
	    {						/* in class 2.0! */
		if ( strcmp( line, "ERROR" ) == 0 )
		{
		    lprintf( L_MESG, "ERROR response" );
		    alarm(0);
		    return NOERROR;			/* !C2 */
		}
	    }
#endif

	    /* in class 2, one of the above codes means total failure */
	    
	    lprintf( L_MESG, "ABORTING: line='%s'", line );
	    fax_hangup = 1;
	    
	    if ( strcmp(line, "BUSY") == 0 ) fax_hangup_code = FHUP_BUSY;
	    else if (strcmp(line, "NO DIALTONE") == 0 ||
		     strcmp(line, "NO DIAL TONE") == 0)
	                                     fax_hangup_code = FHUP_NODIAL;
	    else                             fax_hangup_code = FHUP_ERROR;
	    
	    alarm( 0 ); signal( SIGALRM, SIG_DFL );
	    return ERROR;
	}

    }
    while ( strncmp( s, line, strlen(s) ) != 0 );
    lputs( L_MESG, "** found **" );

    alarm( 0 );
    signal( SIGALRM, SIG_DFL );

    if ( fax_hangup && fax_hangup_code != 0 ) return ERROR;

    return NOERROR;
}

/* (re-) initialize all global/static variables set by faxlib.c
 * necessary if fax state machine runs multiple times, e.g. in vgetty
 */
void faxlib_init _P0( void )
{
    fax_hangup = 0;
    fax_hangup_code = FHUP_UNKNOWN;
    fax_page_tx_status = 0;
    fax_to_poll = fax_poll_req = FALSE;
    fhs_details = FALSE;
    fax_remote_id[0] = '\0';
}

/* send a command string to the modem, terminated with the
 * MODEM_CMD_SUFFIX character / string from policy.h
 */

int fax_send _P2( (send, fd),
		  char * send, int fd )
{
#ifdef FAX_COMMAND_DELAY
    delay(FAX_COMMAND_DELAY);
#endif

    lprintf( L_MESG, "fax_send: '%s'", send );

    if ( write( fd, send, strlen( send ) ) != strlen( send ) ||
	 write( fd, MODEM_CMD_SUFFIX, sizeof(MODEM_CMD_SUFFIX)-1 ) !=
	        ( sizeof(MODEM_CMD_SUFFIX)-1 ) )
    {
	lprintf( L_ERROR, "fax_send: cannot write" );
	return ERROR;
    }

    return NOERROR;
}

/* simple send / expect sequence, but pass "expect"ing through
 * fax_wait_for() to handle all the class 2 fax responses
 */

int fax_command _P3( (send, expect, fd),
		     char * send, char * expect, int fd )
{
    if ( fax_send( send, fd ) == ERROR ) return ERROR;
    return fax_wait_for( expect, fd );
}

/* Couple of routines to set this and that fax parameter, using class 2
 * or 2.0 commands, according to the setting of "modem_type"
 */

/* set local fax id */

int fax_set_l_id _P2( (fd, fax_id), int fd, char * fax_id )
{
    char flid[60];

    if ( modem_type == Mt_class2_0 )
        sprintf( flid, "AT+FLI=\"%.40s\"",  fax_id );
    else
        sprintf( flid, "AT+FLID=\"%.40s\"", fax_id );
    
    if ( mdm_command( flid, fd ) == FAIL )
    {
	lprintf( L_MESG, "cannot set local fax id. Huh?" );
	return ERROR;
    }
    return NOERROR;
}

/* set resolution, minimum and maximum bit rate */
int fax_set_fdcc _P4( (fd, fine, max, min),
		      int fd, int fine, int max, int min )
{
    char buf[50];

#ifndef FAX_USRobotics
    sprintf( buf, "AT%s=%d,%d,0,2,0,0,0,0",
	     (modem_type == Mt_class2_0) ? "+FCC" : "+FDCC",
	     fine, (max/2400) -1 );
#else /* FAX_USRobotics */
    /* some versions of the USR firmware got this wrong, so don't set speed
     */
    sprintf( buf, "AT+FCC=%d", fine );
#endif
    
    if ( mdm_command( buf, fd ) == ERROR )
    {
	if ( max > 9600 )
	    return fax_set_fdcc( fd, fine, 9600, min );
	else
	    return ERROR;
    }

    if ( min >= 2400 )
    {
	if ( modem_type == Mt_class2_0 )
	    sprintf( buf, "AT+FMS=%d", (min/2400) -1 );
	else
	    sprintf( buf, "AT+FMINSP=%d", (min/2400) -1 );

	if ( mdm_command( buf, fd ) == ERROR )
	{
	    lprintf( L_WARN, "+FMINSP command failed, ignoring" );
	}
    }
    return NOERROR;
}

/* set modem flow control (for fax mode only)
 *
 * right now, this works only for class 2.0 faxing. Class 2 has
 * no idea of a common flow control command.
 * If hw_flow is set, use RTS/CTS, otherwise, use Xon/Xoff.
 */

int fax_set_flowcontrol _P2( (fd, hw_flow), int fd, int hw_flow )
{
    if ( modem_type == Mt_class2_0 )
    {
	if ( hw_flow )
	{
	    if ( mdm_command( "AT+FLO=2", fd ) == NOERROR ) return NOERROR;
	    lprintf( L_WARN, "modem doesn't like +FLO=2; using Xon/Xoff" );
	}
	return mdm_command( "AT+FLO=1", fd );
    }
    return NOERROR;
}


/* byte swap table used for sending (yeah. Because Rockwell screwed
 * up *that* completely in class 2, we have to have different tables
 * for sending and receiving. Bah.)
 */
unsigned char fax_send_swaptable[256];

/* set up bit swap table */

static 
void fax_init_swaptable _P2( (direct, byte_tab),
			      int direct, unsigned char byte_tab[] )
{
int i;
    if ( direct ) for ( i=0; i<256; i++ ) byte_tab[i] = i;
    else
      for ( i=0; i<256; i++ )
	     byte_tab[i] = ( ((i & 0x01) << 7) | ((i & 0x02) << 5) |
			     ((i & 0x04) << 3) | ((i & 0x08) << 1) |
			     ((i & 0x10) >> 1) | ((i & 0x20) >> 3) |
			     ((i & 0x40) >> 5) | ((i & 0x80) >> 7) );
}

/* set modem bit order, and initialize bit swap table accordingly */

int faxmodem_bit_order = 0;

int fax_set_bor _P2( (fd, bor), int fd, int bor )
{
    char buf[20];

    faxmodem_bit_order = bor;

    fax_init_swaptable( faxmodem_bit_order & 1, fax_send_swaptable );
    
    if ( modem_type == Mt_class2_0 )
        sprintf( buf, "AT+FBO=%d", bor );
    else
        sprintf( buf, "AT+FBOR=%d", bor );

    return mdm_command( buf, fd );
}


/* find out the type of modem connected
 *
 * controlled by the "mclass" parameter ("auto", "cls2", "c2.0", "data")
 */

Modem_type fax_get_modem_type _P2( (fd, mclass), int fd, char * mclass )
{
    /* data modem? unknown mclass? handle as "auto" (for sendfax) */
    if ( strcmp( mclass, "cls2" ) != 0 &&
	 strcmp( mclass, "c2.0" ) != 0 )
    {
	mclass = "auto";
    }
    
    /* first of all, check for 2.0 */
    if ( strcmp( mclass, "auto" ) == 0 ||
	 strcmp( mclass, "c2.0" ) == 0 )
    {
	if ( mdm_command( "AT+FCLASS=2.0", fd ) == SUCCESS )
	{
	    return Mt_class2_0;
	}
    }

    /* not a 2.0 modem (or not allowed to check),
       simply *try* class 2, nothing to loose */

    if ( mdm_command( "AT+FCLASS=2", fd ) == SUCCESS )
    {
	return Mt_class2;
    }

    /* failed. Assume data modem */

    return Mt_data;
}			/* end fax_get_modem_type() */
