#ident "$Id: mg_m_init.c,v 1.8 1994/08/04 17:22:46 gert Exp $ Copyright (c) Gert Doering"
;
/* mg_m_init.c - part of mgetty+sendfax
 *
 * Initialize (fax-) modem for use with mgetty
 */

#include <stdio.h>
#include "syslibs.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#ifndef sunos4
#include <sys/ioctl.h>
#endif

#include "mgetty.h"
#include "tio.h"
#include "policy.h"
#include "fax_lib.h"
#ifdef VOICE
#include "voclib.h"
#endif

chat_action_t	init_chat_actions[] = { { "ERROR", A_FAIL },
					{ "BUSY", A_FAIL },
					{ "NO CARRIER", A_FAIL },
					{ NULL, A_FAIL } };

/* warning: some modems (for example my old DISCOVERY 2400P) need
 * some delay before sending the next command after an OK, so give it here
 * as "\\dAT...".
 * 
 * To send a backslash, you have to use "\\\\" (four backslashes!) */

static char *	init_chat_seq[] = { "",
			    "\\d\\d\\d+++\\d\\d\\d\r\\dATQ0V1H0", "OK",

/* initialize the modem - defined in policy.h
 */
			    MODEM_INIT_STRING, "OK",
#ifdef DIST_RING
			    DIST_RING_INIT, "OK",
#endif
#ifdef VOICE
			    "AT+FCLASS=8", "OK",
#endif
                            NULL };

static int init_chat_timeout = 20;

/* initialize data section */

int mg_init_data _P1( (fd), int fd )
{
    action_t what_action;
    
    if ( do_chat( fd, init_chat_seq, init_chat_actions,
		 &what_action, init_chat_timeout, TRUE ) == FAIL )
    {
	errno = ( what_action == A_TIMOUT )? EINTR: EINVAL;
	lprintf( L_ERROR, "init chat failed, exiting..." );
	return FAIL;
    }
    return SUCCESS;
}


/* initialization stuff for fax */

/* initialize fax section */

int mg_init_fax _P3( (fd, mclass, fax_id),
		      int fd, char * mclass, char * fax_id )
{
    /* find out whether this beast is a fax modem... */

    modem_type = fax_get_modem_type( fd, mclass );
    
    if ( modem_type == Mt_data )
    {
	lprintf( L_NOISE, "no class 2/2.0 faxmodem, no faxing available" );
	return FAIL;
    }
    
    if ( modem_type == Mt_class2_0 )
    {
	lprintf( L_WARN, "WARNING: using preliminary class 2.0 code");

	/* set adaptive answering, bit order, receiver on */
	
	if ( mdm_command( "AT+FAA=1;+FCR=1", fd ) == FAIL )
	{
	    lprintf( L_MESG, "cannot set reception flags" );
	}
	if ( fax_set_bor( fd, 1 ) == FAIL )
	{
	    lprintf( L_MESG, "cannot set bit order, trying +BOR=0" );
	    fax_set_bor( fd, 0 );
	}

	/* report everything except NSF */
	mdm_command( "AT+FNR=1,1,1,0", fd );
    }

    if ( modem_type == Mt_class2 )
    {
	/* even if we know that it's a class 2 modem, set it to
	 * +FCLASS=0: there are some weird modems out there that won't
	 * properly auto-detect fax/data when in +FCLASS=2 mode...
	 */
	if ( mdm_command( "AT+FCLASS=0", fd ) == FAIL )
	{
	    lprintf( L_MESG, "weird: cannot set class 0" );
	}

	/* now, set various flags and modem settings. Failures are logged,
	   but ignored - after all, either the modem works or not, we'll
	   see it when answering the phone ... */
    
	/* set adaptive answering, bit order, receiver on */

	if ( mdm_command( "AT+FAA=1;+FCR=1", fd ) == FAIL )
	{
	    lprintf( L_MESG, "cannot set reception flags" );
	}
	if ( fax_set_bor( fd, 0 ) == FAIL )
	{
	    lprintf( L_MESG, "cannot set bit order. Huh?" );
	}
    }

    /* common part for class 2 and class 2.0 */

    /* local fax station id */
    fax_set_l_id( fd, fax_id );

    /* capabilities */

    if ( fax_set_fdcc( fd, 1, 14400, 0 ) == FAIL )
    {
	lprintf( L_MESG, "huh? Cannot set +FDCC parameters" );
    }
    
    return SUCCESS;
}


    
/* initialize fax poll server functions (if possible) */
   
extern char * faxpoll_server_file;		/* in faxrec.c */

void faxpoll_server_init _P2( (fd,f), int fd, char * f )
{
    faxpoll_server_file = NULL;
    if ( access( f, R_OK ) != 0 )
    {
	lprintf( L_ERROR, "cannot access/read '%s'", f );
    }
    else if ( mdm_command( modem_type == Mt_class2_0? "AT+FLP=1":"AT+FLPL=1",
			   fd ) == FAIL)
    {
	lprintf( L_WARN, "faxpoll_server_init: no polling available" );
    }
    else
    {
	faxpoll_server_file = f;
	lprintf( L_NOISE, "faxpoll_server_init: OK, waiting for poll" );
    }
}


/* open device (non-blocking / blocking)
 *
 * open with O_NOCTTY, to avoid preventing dial-out processes from
 * getting the line as controlling tty
 */


int mg_open_device _P2 ( (devname, blocking),
		         char * devname, boolean blocking )
{
    int fd;

    if ( ! blocking )
    {
	fd = open(devname, O_RDWR | O_NDELAY | O_NOCTTY );
	if ( fd < 0 )
	{
	    lprintf( L_FATAL, "cannot open line" );
	    return ERROR;
	}

	/* unset O_NDELAY (otherwise waiting for characters */
	/* would be "busy waiting", eating up all cpu) */
	
	fcntl( fd, F_SETFL, O_RDWR);
    }
    else		/* blocking open */
    {
	fd = open( devname, O_RDWR | O_NOCTTY );
	if ( fd < 0)
	{
	    lprintf( L_FATAL, "cannot open line" );
	    return ERROR;
	}
    }

    /* make new fd == stdin if it isn't already */

    if (fd > 0)
    {
	(void) close(0);
	if (dup(fd) != 0)
	{
	    lprintf( L_FATAL, "cannot open stdin" );
	    return ERROR;
	}
    }

    /* make stdout and stderr, too */

    (void) close(1);
    (void) close(2);
    
    if (dup(0) != 1)
    {
	lprintf( L_FATAL, "cannot open stdout"); return ERROR;
    }
    if (dup(0) != 2)
    {
	lprintf( L_FATAL, "cannot open stderr"); return ERROR;
    }

    if ( fd > 2 ) (void) close(fd);

    /* switch off stdio buffering */

    setbuf(stdin, (char *) NULL);
    setbuf(stdout, (char *) NULL);
    setbuf(stderr, (char *) NULL);

    return NOERROR;
}

/* init device: toggle DTR (if requested), set TIO values */

int mg_init_device _P4( (fd, toggle_dtr, toggle_dtr_waittime, portspeed ),
		       int fd,
		       boolean toggle_dtr, int toggle_dtr_waittime,
		       unsigned short portspeed )
{
    TIO tio;
    
    if (toggle_dtr)
    {
	lprintf( L_MESG, "lowering DTR to reset Modem" );
	tio_toggle_dtr( fd, toggle_dtr_waittime );
    }

    /* initialize port */
	
    if ( tio_get( fd, &tio ) == ERROR )
    {
	lprintf( L_FATAL, "cannot get TIO" );
	return ERROR;
    }
    
    tio_mode_sane( &tio, TRUE );	/* initialize all flags */
    tio_set_speed( &tio, portspeed );	/* set bit rate */
    tio_default_cc( &tio );		/* init c_cc[] array */
    tio_mode_raw( &tio );

#ifdef sunos4
    /* SunOS does not rx with RTSCTS unless carrier present */
    tio_set_flow_control( STDIN, &tio, (DATA_FLOW) & (FLOW_SOFT) );
#else
    tio_set_flow_control( STDIN, &tio, DATA_FLOW );
#endif
    
    if ( tio_set( STDIN, &tio ) == ERROR )
    {
	lprintf( L_FATAL, "cannot set TIO" );
	return ERROR;
    }
    return NOERROR;
}

/* get a given tty as controlling tty
 *
 * on many systems, this works with ioctl( TIOCSCTTY ), on some
 * others, you have to reopen the device
 */

int mg_get_ctty _P2( (fd, devname), int fd, char * devname )
{
    /* BSD systems, Linux */
#if defined( TIOCSCTTY )
    if ( setsid() == -1 )
    {
	lprintf( L_ERROR, "cannot make myself session leader (setsid)" );
    }
    if ( ioctl( fd, TIOCSCTTY, NULL ) != 0 )
    {
	lprintf( L_ERROR, "cannot set controlling tty (ioctl)" );
	return ERROR;
    }
#else
    /* SVR3 and earlier */
    fd = open( devname, O_RDWR | O_NDELAY );

    if ( fd == -1 )
    {
        lprintf( L_ERROR, "cannot set controlling tty (open)" );
	return ERROR;
    }

    fcntl( fd, F_SETFL, O_RDWR);		/* unset O_NDELAY */
    close( fd );
#endif						/* !def TIOCSCTTY */

    return NOERROR;
}

/* get rid of a controlling tty.
 *
 * difficult, non-portable, *ugly*.
 */

int mg_drop_ctty _P1( (fd), int fd )
{
#ifdef BSD
    setpgrp( 0, getpid() );
#else
    setpgrp();
#endif
    
    if ( setsid() == -1
#ifdef TIOCNOTTY
	|| ioctl( fd, TIOCNOTTY, NULL )
#endif
	)
    {
	lprintf( L_ERROR, "can't get rid of controlling tty" );
	return ERROR;
    }
    return NOERROR;
}
