#ident "$Id: mgetty.c,v 1.98 1994/04/05 22:11:29 gert Exp $ Copyright (c) Gert Doering"
;
/* mgetty.c
 *
 * mgetty main module - initialize modem, lock, get log name, call login
 *
 * some parts of the code (lock handling, writing of the utmp entry)
 * are based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/times.h>

#ifndef sun
#include <sys/ioctl.h>
#endif

#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "mgetty.h"
#include "policy.h"
#include "tio.h"
#ifdef VOICE
#include "voclib.h"
#endif

#include "mg_utmp.h"

unsigned short portspeed = B0;	/* indicates has not yet been set */

/* warning: some modems (for example my old DISCOVERY 2400P) need
 * some delay before sending the next command after an OK, so give it here
 * as "\\dAT...".
 * 
 * To send a backslash, you have to use "\\\\" (four backslashes!) */

#ifndef NO_FAX
/* I know this is kinda ugly, being the way you have to do it in K&R,
 * but, we'll eventually have to parameterize FAX_STATION_ID anyways.
 */
#define FLID_CMD	"AT+FLID=\"%s\""
char	init_flid_cmd[50];
#endif

char *	init_chat_seq[] = { "", "\\d\\d\\d+++\\d\\d\\d\r\\dATQ0V1H0", "OK",

/* initialize the modem - defined in policy.h
 */
			    MODEM_INIT_STRING, "OK",
#ifndef NO_FAX
			    "AT+FCLASS=0", "OK",
                            "AT+FAA=1;+FBOR=0;+FCR=1", "OK",
			    init_flid_cmd, "OK",
			    /*"AT+FLID=\""FAX_STATION_ID"\"", "OK",*/
			    "AT+FDCC=1,5,0,2,0,0,0", "OK",
#endif
#ifdef DIST_RING
			    DIST_RING_INIT, "OK",
#endif
#ifdef VOICE
			    "AT+FCLASS=8", "OK",
#endif
                            NULL };

int	init_chat_timeout = 60;

chat_action_t	init_chat_actions[] = { { "ERROR", A_FAIL },
					{ "BUSY", A_FAIL },
					{ "NO CARRIER", A_FAIL },
					{ NULL,	A_FAIL } };

int	rings_wanted = 1;		/* default: one "RING" */

#ifdef DIST_RING
char *	ring_chat_seq[] = { "RING\r", NULL };
#else
char *	ring_chat_seq[] = { "RING", NULL };
#endif

chat_action_t	ring_chat_actions[] = { { "CONNECT",	A_CONN },
					{ "NO CARRIER", A_FAIL },
					{ "BUSY",	A_FAIL },
					{ "ERROR",	A_FAIL },
#ifndef NO_FAX
					{ "+FCON",	A_FAX  },
					{ "FAX",	A_FAX  },
#endif
#ifdef VOICE
					{ "VCON",       A_VCON },
#endif
#ifdef DIST_RING
					{ "RING 1",	A_RING1 },
					{ "RING 2",	A_RING2 },
					{ "RING 3",	A_RING3 },
					{ "RING 4",	A_RING4 },
					{ "RING 5",	A_RING5 },
#endif
					{ NULL,		A_FAIL } };

#ifdef VOICE
char *	answer_chat_seq[] = { "", VOICE_ATA, "VCON", NULL };
#else
char *	answer_chat_seq[] = { "", "ATA", "CONNECT", "\\c", "\n", NULL };
#endif

int	answer_chat_timeout = 80;

/* how much time may pass between two RINGs until mgetty goes into */
/* "waiting" state again */

int     ring_chat_timeout = 10;

/* the same actions are recognized while answering as are */
/* when waiting for RING, except for "CONNECT" */

chat_action_t	* answer_chat_actions = &ring_chat_actions[1];


/* private functions */
void		exit_usage();

/* prototypes for system functions (that are missing in some 
 * system header files)
 */
time_t		time _PROTO(( long * tloc ));

/* logname.c */
int getlogname _PROTO(( char * prompt, TIO * termio,
		char * buf, int maxsize ));

char	* Device;			/* device to use */
char	* GettyID = "<none>";		/* Tag for gettydefs in cmd line */

boolean	toggle_dtr = TRUE;		/* lower DTR */

int	toggle_dtr_waittime = 500;	/* milliseconds while DTR is low */

int	prompt_waittime = 500;		/* milliseconds between CONNECT and */
					/* login: -prompt */

extern time_t	call_start;		/* time when we sent ATA */
					/* defined in faxrec.c */

TIO *gettermio _PROTO((char * tag, boolean first, char **prompt));

boolean	direct_line = FALSE;
boolean verbose = FALSE;

boolean virtual_ring = FALSE;
static RETSIGTYPE sig_pick_phone()		/* "simulated RING" handler */
{
    signal( SIGUSR1, sig_pick_phone );
    virtual_ring = TRUE;
}
static RETSIGTYPE sig_goodbye _P1 ( (signo), int signo )
{
    lprintf( L_AUDIT, "failed dev=%s, pid=%d, got signal %d, exiting",
	              Device, getpid(), signo );
    rmlocks();
    exit(10);
}

int main _P2((argc, argv), int argc, char ** argv)
{
        register int c, fd;
	char devname[MAXLINE+1];
	char buf[MAXLINE+1];
	TIO	tio;
	FILE *fp;
	int i;
	int cspeed;

	action_t	what_action;
	int		rings = 0;

#if defined(_3B1_) || defined(MEIBE)
	typedef ushort uid_t;
	typedef ushort gid_t;
	extern struct passwd *getpwuid(), *getpwnam();
#endif

	struct passwd *pwd;
	uid_t	uucpuid = 5;			/* typical uid for UUCP */
	gid_t	uucpgid = 0;

	char *issue = "/etc/issue";		/* default issue file */

	char * login_prompt = NULL;		/* login prompt */

	char * fax_server_file = NULL;		

#ifdef VOICE
	int answer_mode;
	voice_path_init();
#endif
	
#ifndef NO_FAX
	sprintf(init_flid_cmd, FLID_CMD, FAX_STATION_ID);
#endif

	/* startup
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	/* some systems, notable BSD 4.3, have to be told that system
	 * calls are to be interrupted by signals.
	 */
#ifdef HAVE_SIGINTERRUPT
	siginterrupt( SIGINT,  TRUE );
	siginterrupt( SIGALRM, TRUE );
	siginterrupt( SIGHUP,  TRUE );
	siginterrupt( SIGUSR1, TRUE );
#endif

	Device = "unknown";

	/* process the command line
	 */

	while ((c = getopt(argc, argv, "c:x:s:rp:n:i:S:m:")) != EOF) {
		switch (c) {
		case 'c':			/* check */
#ifdef USE_GETTYDEFS
			verbose = TRUE;
			dumpgettydefs(optarg);
			exit(0);
#else
			lprintf( L_FATAL, "gettydefs not supported\n");
			exit_usage(2);
#endif
		case 'm':
			minfreespace = atol(optarg);
			break;
		case 'x':			/* log level */
			log_level = atoi(optarg);
			break;
		case 's':			/* port speed */
			cspeed = atoi(optarg);
			for ( i = 0; speedtab[i].cbaud != 0; i++ )
			{
			    if ( speedtab[i].nspeed == cspeed )
			    {
				portspeed = speedtab[i].cbaud;
				break;
			    }
			}
			if ( speedtab[i].cbaud == 0 )
			{
			    lprintf( L_FATAL, "invalid port speed: %s", optarg);
			    exit_usage(2);
			}
			break;
		case 'r':
			direct_line = TRUE;
			break;
		case 'p':
			login_prompt = optarg;
			break;
		case 'n':			/* ring counter */
			rings_wanted = atoi( optarg );
			if ( rings_wanted == 0 ) rings_wanted = 1;
			break;
		case 'i':
			issue = optarg;		/* use different issue file */
			break;
		case 'S':
			fax_server_file = optarg;
			break;
		case '?':
			exit_usage(2);
			break;
		}
	}

	/* normal System V argument handling
	 */

	if (optind < argc)
		Device = argv[optind++];
	else {
		lprintf(L_FATAL,"no line given");
		exit_usage(2);
	}

	/* remove leading /dev/ prefix */
	if ( strncmp( Device, "/dev/", 5 ) == 0 ) Device += 5;

	/* need full name of the device */
	sprintf(devname, "/dev/%s", Device);

	/* name of the logfile is device-dependant */
	sprintf( log_path, LOG_PATH, strrchr( devname, '/' ) + 1 );

#ifdef USE_GETTYDEFS
	if (optind < argc)
		GettyID = argv[optind++];
	else {
		lprintf(L_WARN, "no gettydef tag given");
		GettyID = GETTYDEFS_DEFAULT_TAG;
	}
#endif

	lprintf(L_MESG, "check for lockfiles");

	/* deal with the lockfiles; we don't want to charge
	 * ahead if uucp, kermit or whatever else is already
	 * using the line.
	 * (Well... if we reach this point, most propably init has
	 * hung up anyway :-( )
	 */

	/* check for existing lock file(s)
	 */
	if (checklock(Device) == TRUE) {
		while (checklock(Device) == TRUE)
			(void) sleep(10);
		exit(0);
	}

	/* try to lock the line
	 */
	lprintf(L_MESG, "locking the line");

	if ( makelock(Device) == FAIL ) {
		while( checklock(Device) == TRUE)
			(void) sleep(10);
		exit(0);
	}

	/* allow uucp to access the device
	 */
	(void) chmod(devname, FILE_MODE);
	if ((pwd = getpwnam(UUCPID)) != (struct passwd *) NULL)
	{
		uucpuid = pwd->pw_uid;
		uucpgid = pwd->pw_gid;
	}
	(void) chown(devname, uucpuid, uucpgid);

	/* the line is mine now ...  */

	/* open the device; don't wait around for carrier-detect */

	if ((fd = open(devname, O_RDWR | O_NDELAY)) < 0) {
		lprintf(L_ERROR,"cannot open line");
		exit(FAIL);
	}

	/* unset O_NDELAY (otherwise waiting for characters */
	/* would be "busy waiting", eating up all cpu) */

	fcntl( fd, F_SETFL, O_RDWR);

	/* make new fd == stdin if it isn't already */

	if (fd > 0) {
		(void) close(0);
		if (dup(fd) != 0) {
			lprintf(L_ERROR,"cannot open stdin");
			exit(FAIL);
		}
	}

	/* make stdout and stderr, too */

	(void) close(1);
	(void) close(2);
	if (dup(0) != 1) {
		lprintf(L_ERROR,"cannot open stdout");
		exit(FAIL);
	}
	if (dup(0) != 2) {
		lprintf(L_ERROR,"cannot open stderr");
		exit(FAIL);
	}

	if (fd > 2)
		(void) close(fd);

	/* no buffering */

	setbuf(stdin, (char *) NULL);
	setbuf(stdout, (char *) NULL);
	setbuf(stderr, (char *) NULL);

	/* setup terminal */

	/* Currently, the tio returned here is ignored.
	   The invocation is only for the sideeffects of:
	    - loading the gettydefs file if enabled.
	    - setting portspeed appropriately, if not defaulted.
	 */

	tio = *gettermio(GettyID, TRUE, &login_prompt);

	if (toggle_dtr)
	{
		lprintf( L_MESG, "lowering DTR to reset Modem" );
		tio_toggle_dtr( STDIN, toggle_dtr_waittime );
	}

	/* initialize port */
	
	if ( tio_get( STDIN, &tio ) == ERROR )
	{
	    lprintf( L_FATAL, "cannot get TIO" );
	    exit(20);
	}
	tio_mode_sane( &tio, TRUE );
	tio_set_speed( &tio, portspeed );
	tio_default_cc( &tio );
	tio_mode_raw( &tio );
#ifdef sun
	/* sunos does not rx with RTSCTS unless carrier present */
	tio_set_flow_control( STDIN, &tio, (DATA_FLOW) & (FLOW_SOFT) );
#else
	tio_set_flow_control( STDIN, &tio, DATA_FLOW );
#endif
	if ( tio_set( STDIN, &tio ) == ERROR )
	{
	    lprintf( L_FATAL, "cannot set TIO" );
	    exit(20);
	}

	/* drain input - make sure there are no leftover "NO CARRIER"s
	 * or "ERROR"s lying around from some previous dial-out
	 */

	clean_line( STDIN, 1);

	/* handle init chat if requested
	 */
	if ( ! direct_line )
	{
	    if ( do_chat( STDIN, init_chat_seq, init_chat_actions,
			  &what_action, init_chat_timeout, TRUE ) == FAIL )
	    {
		lprintf( L_MESG, "init chat failed, exiting..." );
		rmlocks();
		exit(1);
	    }
	    /* initialize fax polling server */
	    if ( fax_server_file )
	    {
		faxpoll_server_init( fax_server_file );
	    }
	}

	/* wait .3s for line to clear (some modems send a \n after "OK",
	   this may confuse the "call-chat"-routines) */

	clean_line( STDIN, 3);

	/* remove locks, so any other process can dial-out. When waiting
	   for "RING" we check for foreign lockfiles, if there are any, we
	   give up the line - otherwise we lock it again */

	rmlocks();	

	/* with mgetty, it's possible to use a fax machine parallel to
	   the fax modem as a scanner - dial a "9" on the fax machine,
	   send mgetty a SIGUSR1, then mgetty will pick up the phone as
	   if it got a RING */

	signal( SIGUSR1, sig_pick_phone );

#if defined(linux) && defined(NO_SYSVINIT)
	/* on linux, "simple init" does not make a wtmp entry when you
	 * log so we have to do it here (otherwise, "who" won't work)
	 */
	make_utmp_wtmp( Device, UT_INIT, "uugetty" );
#endif

#ifdef VOICE
	/* With external modems, the auto-answer LED can be used
	 * to show a status flag. vgetty uses this to indicate
	 * that new messages have arrived.
	 */
	voice_message_light(&rings_wanted);
#endif /* VOICE */
	
waiting:
	/* wait for incoming characters (using select() or poll() to
	 * prevent eating away from processes dialing out)
	 */
	lprintf( L_MESG, "waiting..." );

	wait_for_input( STDIN );
	
	/* check for LOCK files, if there are none, grab line and lock it
	*/
    
	lprintf( L_NOISE, "checking lockfiles, locking the line" );

	if ( makelock(Device) == FAIL) {
	    lprintf( L_NOISE, "lock file exists (dialout)!" );

	    /* close all file descriptors -> other processes can read port */
	    close(0);
	    close(1);
	    close(2);

	    /* write a note to utmp/wtmp about dialout
	     */
	    make_utmp_wtmp( Device, UT_USER, "dialout" );

	    /* this is kind of tricky: sometimes uucico dial-outs do still
	       collide with mgetty. So, when my uucico times out, I do
	       *immediately* restart it. The double check makes sure that
	       mgetty gives me at least 5 seconds to restart uucico */

	    do {
		    /* wait for lock to disappear */
		    while (checklock(Device) == TRUE)
			sleep(10);	/*!!!!! ?? wait that long? */

		    /* wait a moment, then check for reappearing locks */
		    sleep(5);
	    }
 	    while ( checklock(Device) == TRUE );	

	    /* OK, leave & get restarted by init */
	    exit(0);
	}

	/* set to remove lockfile(s) on certain signals
	 */
	(void) signal(SIGHUP, sig_goodbye);
	(void) signal(SIGINT, sig_goodbye);
	(void) signal(SIGQUIT, sig_goodbye);
	(void) signal(SIGTERM, sig_goodbye);

#ifdef NOLOGIN_FILE
	/* check for a "nologin" file (/etc/nologin.<device>) */
	
	sprintf( buf, NOLOGIN_FILE, strrchr( devname, '/' )+1 );

	if ( access( buf, F_OK ) == 0 )
	{
	    lprintf( L_MESG, "%s exists - do not accept call!", buf );
	    clean_line( STDIN, 80 );		/* wait for ringing to stop */

	    /* and return to state waiting. If it was a data or fax
	     * call, you can now press DATA/VOICE and have the modem
	     * manually pickup the phone
	     *
	     * WARNING: if you press the button too soon, or if the
	     * modem auto-answers, this will fail. FIXME. Use do_chat()
	     * here (and count the RINGs for logging...)
	     */
	    lprintf( L_AUDIT, "rejected" );
	    rmlocks();
	    goto waiting;
	       
	    /* exit(1); */
	}
#endif
    
#ifdef VOICE
	/* check for a "answer mode" file (/etc/answer.<device>) */
	answer_mode = get_answer_mode(Device);
#endif

	/* wait for "RING", if found, send manual answer string (ATA)
	   to the modem */

	if ( ! direct_line )
	{
	    rings = 0;
	    while ( rings < rings_wanted )
	    {
		if ( do_chat( STDIN, ring_chat_seq, ring_chat_actions,
			      &what_action, ring_chat_timeout,
			      TRUE ) == FAIL 
#ifdef DIST_RING
		    && (what_action != DIST_RING_VOICE)
#endif
		   ) break;
		rings++;
	    }

	    /* timeout - the phone stopped ringing? (human picked up) */
	    if ( rings < rings_wanted && what_action == A_TIMOUT )
	    {
		lprintf( L_MESG, "phone stopped ringing" );
		rmlocks();		/* free line again */
		goto waiting;
	    }

	    /* If we got caller ID information, we check it and abort
	       if this caller isn't allowed in */

	    if (!cndlookup())
	    {
		lprintf( L_AUDIT, "denied caller dev=%s, pid=%d, caller=%s",
		    Device, getpid(), CallerId);
		clean_line( STDIN, 80 ); /* wait for ringing to stop */

		rmlocks();
		exit(1);	/* -> state "waiting" */
	    }

	    /* remember time of phone pickup */
	    call_start = time( NULL );

	    /* answer phone only, if we got all "RING"s (otherwise, the */
	    /* modem may have auto-answered (urk), the user may have */
	    /* pressed a "data/voice" button, ..., and we fall right */
	    /* through) */

	    if ( what_action != A_CONN &&
		 what_action != A_VCON &&	/* vgetty extensions */
#ifdef DIST_RING
		 (what_action < A_RING1 || what_action > A_RING5) &&
#endif
		 ( rings < rings_wanted ||
	           do_chat( STDIN, answer_chat_seq, answer_chat_actions,
			    &what_action, answer_chat_timeout, TRUE) == FAIL))
	    {
		if ( what_action == A_FAX )
		{
		    lprintf( L_MESG, "action is A_FAX, start fax receiver...");
		    faxrec( FAX_SPOOL_IN );
		    exit(1);
		}

		lprintf( L_AUDIT, 
		   "failed %s dev=%s, pid=%d, caller=%s, conn='%s', name='%s'",
		    what_action == A_TIMOUT? "timeout": "A_FAIL", 
		    Device, getpid(), CallerId, Connect, CallName );
  
		rmlocks();
		exit(1);
	    }
	}

	/* wait for line to clear (after "CONNECT" a baud rate may
           be sent by the modem, on a non-MNP-Modem the MNP-request
           string sent by a calling MNP-Modem is discarded here, too) */

	clean_line( STDIN, 3);

#ifdef VOICE
	/* Answer in voice mode. The function will return only if it
	   detects a data call, otherwise it will call exit().

	   The modem will be in voice mode when voice_answer is
	   called. If the function returns, the modem will be
	   connected in DATA mode. */

	if( ! direct_line )
	{
	    voice_answer(rings, rings_wanted,
			 answer_chat_actions,
			 answer_chat_timeout,
			 answer_mode,
			 what_action );
	}
#endif /* VOICE */

	/* honor carrier now: terminate if modem hangs up prematurely
	 */
	tio_carrier( &tio, TRUE );
	tio_set( STDIN, &tio );

	/* make utmp and wtmp entry (otherwise login won't work)
	 */
	make_utmp_wtmp( Device, UT_LOGIN, "LOGIN" );

	/* wait a little bit befor printing login: prompt (to give
	 * the other side time to get ready)
	 */
        delay( prompt_waittime );

	/* loop until a successful login is made
	 */
	for (;;)
	{
		/* set ttystate for /etc/issue ("before" setting) */
		tio = *gettermio(GettyID, TRUE, &login_prompt);
#ifdef sun
		/* we have carrier, assert data flow control */
		tio_set_flow_control( STDIN, &tio, DATA_FLOW );
#endif
		tio_set( STDIN, &tio );
		
		fputc('\r', stdout);	/* just in case */

		/* display ISSUE, if present
		 */
		if (*issue != '/') {
			fputs(issue, stdout);
			fputs("\r\n", stdout);
		} else if ((fp = fopen(issue, "r")) != (FILE *) NULL) {
			while (fgets(buf, sizeof(buf), fp) != (char *) NULL)
			{
				fputs(buf, stdout);
				fputc('\r', stdout );
			}
			fclose(fp);
		}

		/* set permissions to "rw-------" */
		(void) chmod(devname, 0600);

		/* set ttystate for login ("after"),
		 *  cr-nl mapping flags are set by getlogname()!
		 */
#ifdef USE_GETTYDEFS
		tio = *gettermio(GettyID, FALSE, &login_prompt);
		tio_set( STDIN, &tio );

		lprintf(L_NOISE, "i: %06o, o: %06o, c: %06o, l: %06o, p: %s",
		    tio.c_iflag, tio.c_oflag, tio.c_cflag, tio.c_lflag,
		    login_prompt);
#endif
		/* read a login name from tty */
		/* (if just <cr> is pressed, re-print issue file) */
		/* also adjust ICRNL / IGNCR to characters recv'd at */
		/* end of line: cr+nl -> IGNCR, cr -> ICRNL, NL -> 0 */
		/* and: cr -> ONLCR, nl -> 0 for c_oflag */

                if ( getlogname( login_prompt, &tio,
				 buf, sizeof(buf) ) == -1 ) continue;

		/* hand off to login (dispatcher) */
		login( buf );
		
		exit(FAIL);
	}
}

/*
 *	exit_usage() - exit with usage display
 */

void exit_usage _P1((code), int code )
{
#ifdef USE_GETTYDEFS
    lprintf( L_FATAL, "Usage: mgetty [-x debug] [-s speed] [-r] line [gettydefentry]" );
#else
    lprintf( L_FATAL, "Usage: mgetty [-x debug] [-s speed] [-r] line" );
#endif
    exit(code);
}

TIO *
gettermio _P3 ((id, first, prompt), char *id, boolean first, char **prompt) {

    static TIO termio;
    char *rp;

#ifdef USE_GETTYDEFS
    static loaded = 0;
    GDE *gdp;
#endif

    /* default setting */
    tio_get( STDIN, &termio );		/* init flow ctrl, C_CC flags */
    tio_mode_sane( &termio, FALSE );
    rp = LOGIN_PROMPT;

#ifdef USE_GETTYDEFS

    if (!loaded) {
	if (!loadgettydefs(GETTYDEFS)) {
	    lprintf(L_WARN, "Couldn't load gettydefs - using defaults");
	}
	loaded = 1;
    }
    if ( (gdp = getgettydef(id)) != NULL )
    {
	lprintf(L_NOISE, "Using %s gettydefs entry, \"%s\"", gdp->tag,
		first? "before" : "after" );
	if (first) {
	    if ( portspeed == B0 )	/* no "-s" arg, use gettydefs */
	        portspeed = ( gdp->before.c_cflag ) & CBAUD;
	} else {
	    termio = gdp->after;
	}
	rp = gdp->prompt;
    }

#endif
    if (portspeed == B0)
	portspeed = DEFAULT_PORTSPEED;

    tio_set_speed( &termio, portspeed );

    if (prompt && !*prompt) *prompt = rp;

    return( &termio );
}
