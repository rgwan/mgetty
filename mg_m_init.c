#ident "$Id: mg_m_init.c,v 1.1 1994/04/27 00:26:13 gert Exp $ Copyright (c) Gert Doering"
;
/* mg_m_init.c - part of mgetty+sendfax
 *
 * Initialize (fax-) modem for use with mgetty
 */

#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>
#include <string.h>
#include <signal.h>

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


/* mini-chat: send command, wait for OK, return ERROR/NOERROR */

int mi_command _P2( (fd, cmd), int fd, char * cmd )
{
    action_t what_action;
    static char * chat[] = { "", NULL, "OK", NULL };
    
    chat[1] = cmd;
    
    if ( do_chat( fd, chat, init_chat_actions,
		  &what_action, init_chat_timeout, TRUE ) == SUCCESS )
    {
	return SUCCESS;
    }
    return FAIL;
}

/* initialize data section */

int mg_init_data _P1( (fd), int fd )
{
    action_t what_action;
    
    if ( do_chat( fd, init_chat_seq, init_chat_actions,
		 &what_action, init_chat_timeout, TRUE ) == FAIL )
    {
	lprintf( L_MESG, "init chat failed, exiting..." );
	return FAIL;
    }
    return SUCCESS;
}

/* initialize fax section */

int mg_init_fax _P2( (fd, fax_id), int fd, char * fax_id )
{
    static char flid[60];

    /* find out whether this beast is a fax modem... */

    if ( mi_command( fd, "AT+FCLASS=2.0" ) == SUCCESS )
    {
	lprintf( L_MESG, "great! The modem can do Class 2.0 - but it's not yet supported :-( ");
    }

    if ( mi_command( fd, "AT+FCLASS=2" ) == FAIL )
    {
	lprintf( L_NOISE, "no class 2 faxmodem, no faxing available" );
	return FAIL;
    }
    else
    {
	/* even if we know that it's a class 2 modem, set it to
	 * +FCLASS=0: there are some weird modems out there that won't
	 * properly auto-detect fax/data when in +FCLASS=2 mode...
	 */
	if ( mi_command( fd, "AT+FCLASS=0" ) == FAIL )
	{
	    lprintf( L_MESG, "weird: cannot set class 0" );
	}
    }

    /* now, set various flags and modem settings. Failures are logged,
       but ignored - after all, either the modem works or not, we'll see it
       when answering the phone ... */
    
    /* set adaptive answering, bit order, receiver on */

    if ( mi_command( fd, "AT+FAA=1;+FBOR=0;+FCR=1" ) == FAIL )
    {
	lprintf( L_MESG, "cannot set reception flags" );
    }

    /* local fax station id */

    sprintf( flid, "AT+FLID=\"%.40s\"", fax_id );
    if ( mi_command( fd, flid ) == FAIL )
    {
	lprintf( L_MESG, "cannot set local fax id. Huh?" );
    }

    /* capabilities */

    if ( mi_command( fd, "AT+FDCC=1,5,0,2,0,0,0,0" ) == FAIL &&
	 mi_command( fd, "AT+FDCC=1,3,0,2,0,0,0,0" ) == FAIL )
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
    else if ( mi_command( fd, "AT+FLPL=1" ) == FAIL )
    {
	lprintf( L_WARN, "faxpoll_server_init: no polling available" );
    }
    else
    {
	lprintf( L_NOISE, "faxpoll_server_init: waiting for poll" );
	faxpoll_server_file = f;
    }
}
