#ident "$Id: mgetty.c,v 1.62 1993/11/12 15:19:16 gert Exp $ Copyright (c) Gert Doering";
/* some parts of the code (lock handling, writing of the utmp entry)
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
#include "tio.h"
#include "policy.h"

struct	speedtab {
	ushort	cbaud;		/* baud rate */
	int	nspeed;		/* speed in numeric format */
	char	*speed;		/* speed in display format */
} speedtab[] = {
	{ B50,	  50,	 "50"	 },
	{ B75,	  75,	 "75"	 },
	{ B110,	  110,	 "110"	 },
	{ B134,	  134,	 "134"	 },
	{ B150,	  150,	 "150"	 },
	{ B200,	  200,	 "200"	 },
	{ B300,	  300,	 "300"	 },
	{ B600,	  600,	 "600"	 },
	{ B1200,  1200,	 "1200"	 },
	{ B1800,  1800,	 "1800"	 },
	{ B2400,  2400,	 "2400"	 },
	{ B4800,  4800,	 "4800"	 },
	{ B9600,  9600,	 "9600"	 },
#ifdef	B19200
	{ B19200, 19200, "19200" },
#endif	/* B19200 */
#ifdef	B38400
	{ B38400, 38400, "38400" },
#endif	/* B38400 */
	{ EXTA,	  19200, "EXTA"	 },
	{ EXTB,	  38400, "EXTB"	 },
	{ 0,	  0,	 ""	 }
};

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

char *	init_chat_seq[] = { "", "\\d\\d\\d+++\\d\\d\\d\r\\dATQ0H0", "OK",

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
                            NULL };

int	init_chat_timeout = 60;

chat_action_t	init_chat_actions[] = { { "ERROR", A_FAIL },
					{ "BUSY", A_FAIL },
					{ "NO CARRIER", A_FAIL },
					{ NULL,	A_FAIL } };

int	rings_wanted = 1;		/* default: one "RING" */
char *	ring_chat_seq[] = { "RING", NULL };
chat_action_t	ring_chat_actions[] = { { "CONNECT",	A_CONN },
					{ "NO CARRIER", A_FAIL },
					{ "BUSY",	A_FAIL },
					{ "ERROR",	A_FAIL },
#ifndef NO_FAX
					{ "+FCON",	A_FAX  },
					{ "FAX",	A_FAX  },
#endif
					{ NULL,		A_FAIL } };

char *	answer_chat_seq[] = { "", "ATA", "CONNECT", "\\c", "\n", NULL };
int	answer_chat_timeout = 80;

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

#ifndef sun
boolean	toggle_dtr = TRUE;		/* lower DTR */
#else
boolean toggle_dtr = FALSE;             /* doesn't work on SunOS! */
#endif

int	toggle_dtr_waittime = 500;	/* milliseconds while DTR is low */

int	prompt_waittime = 500;		/* milliseconds between CONNECT and */
					/* login: -prompt */

TIO *gettermio _PROTO((char * tag, boolean first, char **prompt));

boolean	direct_line = FALSE;
boolean verbose = FALSE;

boolean virtual_ring = FALSE;
static RETSIGTYPE sig_pick_phone()		/* "simulated RING" handler */
{
    signal( SIGUSR1, sig_pick_phone );
    virtual_ring = TRUE;
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
	int		rings;

#if defined(_3B1_) || defined(MEIBE)
	typedef ushort uid_t;
	typedef ushort gid_t;
	extern struct passwd *getpwuid(), *getpwnam();
#endif

	struct passwd *pwd;
	uid_t	uucpuid = 5;			/* typical uid for UUCP */
	gid_t	uucpgid = 0;

	char *issue = "/etc/issue";		/* default issue file */

	char *login = "/bin/login";		/* default login program */

	char * login_prompt = NULL;	/* default login prompt */

#ifndef NO_FAX
	sprintf(init_flid_cmd, FLID_CMD, FAX_STATION_ID);
#endif

	/* startup
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	Device = "unknown";

	/* process the command line
	 */

	while ((c = getopt(argc, argv, "c:x:s:rp:n:")) != EOF) {
		switch (c) {
		case 'c':
#ifdef USE_GETTYDEFS
			verbose = TRUE;
			dumpgettydefs(optarg);
			exit(0);
#else
			lprintf( L_FATAL, "gettydefs not supported\n");
			exit_usage(2);
#endif
		case 'x':
			log_level = atoi(optarg);
			break;
		case 's':
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
		case 'n':
			rings_wanted = atoi( optarg );
			if ( rings_wanted == 0 ) rings_wanted = 1;
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
	sprintf( log_path, LOG_PATH, Device );

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

	/* unset O_NDELAY (otherwise waiting for characters */
	/* would be "busy waiting", eating up all cpu) */

	fcntl(STDIN, F_SETFL, O_RDWR);

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
	tio_mode_raw( &tio );
	tio_set_flow_control( STDIN, &tio, DATA_FLOW );
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
	  if ( do_chat( STDIN, init_chat_seq, init_chat_actions, &what_action,
                        init_chat_timeout, TRUE ) == FAIL )
	{
	    lprintf( L_MESG, "init chat failed, exiting..." );
	    rmlocks();
	    exit(1);
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

#ifdef linux
	/* on linux, "init" does not make a wtmp entry when you log out,
	 * so we have to do it here (otherwise, "who" won't work)
	 */
	make_utmp_wtmp( Device, FALSE );
#endif

	/* wait for incoming characters (using select() or poll() to
	 * prevent eating away from processes dialing out)
	 */
	lprintf( L_MESG, "waiting..." );

	wait_for_input( STDIN );
	
	/* check for LOCK files, if there are none, grab line and lock it
	*/
    
	lprintf( L_NOISE, "checking lockfiles, locking the line" );

	if ( makelock(Device) == FAIL) {
	    lprintf( L_NOISE, "lock file exists!" );

	    /* close all file descriptors -> other processes can read port */
	    close(0);
	    close(1);
	    close(2);

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

	/* check for a "nologin" file (/etc/nologin.<device>) */

	sprintf( buf, "/etc/nologin.%s", Device );

	if ( access( buf, F_OK ) == 0 )
	{
	    lprintf( L_MESG, "%s exists - do not accept call!", buf );
	    clean_line( STDIN, 80 );		/* wait for ringing to stop */
	    exit(1);
	}

	/* wait for "RING", if found, send manual answer string (ATA)
	   to the modem */

	if ( ! direct_line )
	{
	    rings = 0;
	    while ( rings < rings_wanted )
	    {
		if ( do_chat( STDIN, ring_chat_seq, ring_chat_actions,
			      &what_action, answer_chat_timeout,
			      TRUE ) == FAIL ) break;
		rings++;
	    }

	    /* answer phone only, if we got all "RING"s (otherwise, the */
	    /* modem may have auto-answered (urk), the user may have */
	    /* pressed a "data/voice" button, ..., and we fall right */
	    /* through) */

	    log_level++; /*FIXME!!: remove this - for debugging only */
	    
	    if ( what_action != A_CONN &&
		 ( rings < rings_wanted ||
	           do_chat( STDIN, answer_chat_seq, answer_chat_actions,
			    &what_action, answer_chat_timeout, TRUE) == FAIL))
	    {
		if ( what_action == A_FAX )
		{
		    lprintf( L_MESG, "action is A_FAX, start fax receiver...");
		    faxrec( FAX_SPOOL_IN );
		    lprintf( L_MESG, "fax receiver finished, exiting...");
		    exit(1);
		}

		lprintf( L_MESG, "chat failed (timeout or A_FAIL), exiting..." );
		rmlocks();
		exit(1);
	    }
	    log_level--;
	}

	/* wait for line to clear (after "CONNECT" a baud rate may
           be sent by the modem, on a non-MNP-Modem the MNP-request
           string sent by a calling MNP-Modem is discarded here, too) */

	clean_line( STDIN, 3);

	/* set to remove lockfile(s) on certain signals
	 */
	(void) signal(SIGHUP, rmlocks);
	(void) signal(SIGINT, rmlocks);
	(void) signal(SIGQUIT, rmlocks);
	(void) signal(SIGTERM, rmlocks);

	/* make utmp and wtmp entry (otherwise login won't work)
	 */
	make_utmp_wtmp( Device, TRUE );

        delay( prompt_waittime );
	/* loop until a successful login is made
	 */
	for (;;)
	{
		/* set ttystate for /etc/issue ("before" setting) */
		tio = *gettermio(GettyID, TRUE, &login_prompt);
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

		lprintf( L_MESG, "device=%s, pid=%d, calling 'login %s'...\n", Device, getpid(), buf );

		/* hand off to login, (can be a shell script!) */
		(void) execl(login, "login", buf[0]? buf: NULL, NULL);
		(void) execl("/bin/sh", "sh", "-c",
				login, buf, (char *) NULL);
		lprintf(L_FATAL, "cannot execute %s", login);
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
