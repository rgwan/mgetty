#ident "$Id: mgetty.c,v 1.49 1993/10/05 13:47:00 gert Exp $ Copyright (c) Gert Doering";
/* some parts of the code (lock handling, writing of the utmp entry)
 * are based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
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

#ifdef USE_SELECT
#if defined (linux) || defined (sun) || defined (__hpux)
#include <sys/time.h>
#else
#include <sys/select.h>
#endif
#else
#include <stropts.h>
#include <poll.h>
#endif	/* USE_SELECT */

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

unsigned short portspeed = DEFAULT_PORTSPEED;

/* warning: some modems (for example my old DISCOVERY 2400P) need
 * some delay before sending the next command after an OK, so give it here
 * as "\\dAT...".
 * 
 * To send a backslash, you have to use "\\\\" (four backslashes!) */

char *	init_chat_seq[] = { "", "\\d\\d\\d+++\\d\\d\\d\r\\dATQ0H0", "OK",

/* initialize the modem - defined in policy.h
 */
			    MODEM_INIT_STRING, "OK",
#ifndef NO_FAX
                            "AT+FAA=1;+FBOR=0;+FCR=1", "OK",
			    "AT+FLID=\""FAX_STATION_ID"\"", "OK",
			    "AT+FDCC=1,5,0,2,0,0,0", "OK",
#endif
                            NULL };

char *	init_chat_abort[] = { "ERROR", "BUSY", NULL };
int	init_chat_timeout = 60;

chat_action_t	init_chat_actions[] = { { "ERROR", A_FAIL },
					{ "BUSY", A_FAIL },
					{ "NO CARRIER", A_FAIL },
					{ NULL,	A_FAIL } };

char *	call_chat_seq[] = { "RING", "ATA", "CONNECT", "\\c", "\n", NULL };
char *	call_chat_abort[] = { "NO CARRIER", "BUSY", "ERROR", NULL };
int	call_chat_timeout = 60;

chat_action_t	call_chat_actions[] = { { "NO CARRIER", A_FAIL },
					{ "BUSY",	A_FAIL },
					{ "ERROR", A_FAIL },
#ifndef NO_FAX
					{ "+FCON", A_FAX },
#endif
					{ NULL, A_FAIL } };


/* private functions */
void		exit_usage();

/* prototypes for system functions (that are missing in some 
 * system header files)
 */
time_t		time( long * tloc );

/* logname.c */
int getlogname( char * prompt, struct termio * termio,
		char * buf, int maxsize );

char	* Device;			/* device to use */

boolean	toggle_dtr = TRUE;		/* lower DTR */
int	toggle_dtr_waittime = 500;	/* milliseconds while DTR is low */

int	prompt_waittime = 500;		/* milliseconds between CONNECT and */
					/* login: -prompt */

boolean	direct_line = FALSE;

boolean virtual_ring = FALSE;
static void sig_pick_phone()		/* "simulated RING" handler */
{
    signal( SIGUSR1, sig_pick_phone );
    virtual_ring = TRUE;
}

int main( int argc, char ** argv)
{
	register int c, fd;
	char devname[MAXLINE+1];
	char buf[MAXLINE+1];
	struct termio termio;
	FILE *fp;
	int Nusers;
	int i;
	int cspeed;

	action_t	what_action;

#ifdef USE_SELECT
	fd_set	readfds;
#else	/* use poll */
	struct	pollfd fds;
#endif
	int	slct;


	struct passwd *pwd;
	uid_t	uucpuid = 5;			/* typical uid for UUCP */
	gid_t	uucpgid = 0;

	char *issue = "/etc/issue";		/* default issue file */

	char *login = "/bin/login";		/* default login program */

	char * login_prompt = LOGIN_PROMPT;	/* default login prompt */

	/* startup
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	Device = "unknown";

	/* process the command line
	 */

	while ((c = getopt(argc, argv, "x:s:rp:")) != EOF) {
		switch (c) {
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
	if ( strcmp( Device, "/dev/" ) == 0 ) Device += 5;

	/* need full name of the device */
	sprintf(devname, "/dev/%s", Device);

	/* name of the logfile is device-dependant */
	sprintf( log_path, LOG_PATH, Device );

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
	if ((pwd = getpwnam(UUCPID)) != (struct passwd *) NULL) {
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

	if (toggle_dtr)
	{
		lprintf( L_MESG, "lowering DTR to reset Modem" );
		(void) ioctl(STDIN, TCGETA, &termio);
		termio.c_cflag &= ~CBAUD;	/* keep all but CBAUD bits */
		termio.c_cflag |= B0;		/* set speed == 0 */
		(void) ioctl(STDIN, TCSETAF, &termio);
		delay(toggle_dtr_waittime);
	}

	termio.c_iflag = ICRNL | IXANY;	/*!!!!!! ICRNL??? */
	termio.c_oflag = OPOST | ONLCR;

        /* we want to set hardware (RTS+CTS) flow control here.
	 * see the mass of #ifdefs in mgetty.h what is defined
	 * (and change it according to your OS)
	 */
	termio.c_cflag = portspeed | CS8 | CREAD | HUPCL | CLOCAL |
			 HARDWARE_HANDSHAKE;

	termio.c_lflag = 0;		/* echo off, signals off! */
	termio.c_line = 0;
	termio.c_cc[VMIN] = 1;
	termio.c_cc[VTIME]= 0;
	ioctl (STDIN, TCSETAF, &termio);

	/* drain input - make sure there are no leftover "NO CARRIER"s
	 * or "ERROR"s lying around from some previous dial-out
	 */

	clean_line( STDIN, 1);

	/* handle init chat if requested
	 */
	if ( ! direct_line )
	  if ( do_chat( STDIN, init_chat_seq, init_chat_actions, &what_action,
                        init_chat_timeout, TRUE, FALSE, FALSE ) == FAIL )
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

	/* wait for incoming characters.
	   I use select() instead of a blocking read(), since select()
	   does *not* eat up characters and thus prevents collisions
	   with dial-outs
	   On Systems without select(S), poll(S) can be used. */

	lprintf( L_MESG, "waiting..." );

#ifdef USE_SELECT

	FD_ZERO( &readfds );
	FD_SET( STDIN, &readfds );
	slct = select( 1, &readfds, NULL, NULL, NULL );
	lprintf( L_NOISE, "select returned %d", slct );

#else	/* use poll */

	fds.fd = 1;
	fds.events = POLLIN;
	fds.revents= 0;
	slct = poll( &fds, 1, -1 );
	lprintf( L_NOISE, "poll returned %d", slct );
#endif

	/* check for LOCK files, if there are none, grab line and lock it
	*/

	lprintf( L_NOISE, "checking lockfiles, locking the line" );

	if ( makelock(Device) == FAIL) {
	    lprintf( L_NOISE, "lock file exists!" );

	    /* close stdin -> other processes can read port */
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

	log_level++; /*FIXME: remove this - for debugging only !!!!!!!!!!!*/
	if ( ! direct_line )
	  if ( do_chat( STDIN, call_chat_seq, call_chat_actions, &what_action,
                        call_chat_timeout, TRUE, FALSE, virtual_ring ) == FAIL)
	{
	    if ( what_action == A_FAX )
	    {
		lprintf( L_MESG, "action is A_FAX, start fax receiver...");
		faxrec( FAX_SPOOL_IN );
		lprintf( L_MESG, "fax receiver finished, exiting...");
		exit(1);
	    }

	    lprintf( L_MESG, "action != A_FAX; chat failed, exiting..." );
	    rmlocks();
	    exit(1);
	}
	log_level--;

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
	for (;;) {

		/* set Nusers value to number of users currently logged in
		 */
		Nusers = get_current_users();
		/* lprintf(L_NOISE, "Nusers=%d", Nusers); */

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

		/* set ttystate for login prompt (no mapping CR->NL)*/
		(void) ioctl(STDIN, TCGETA, &termio);
		termio.c_iflag = BRKINT | IGNPAR | IXON | IXANY;
		termio.c_oflag = OPOST | TAB3;
		termio.c_cflag = CS8 | portspeed | CREAD | HUPCL |
			 	 HARDWARE_HANDSHAKE;
		termio.c_lflag = ECHOK | ECHOE | ECHO | ISIG | ICANON;
		termio.c_line = 0;

		(void) ioctl(STDIN, TCSETAW, &termio);

		/* set permissions to "rw-------" */
		(void) chmod(devname, 0600);

		/* read a login name from tty */
		/* (if just <cr> is pressed, re-print issue file) */
		/* also adjust ICRNL / IGNCR to characters recv'd at */
		/* end of line: cr+nl -> IGNCR, cr -> ICRNL, NL -> 0 */
		/* and: cr -> ONLCR, nl -> 0 for c_oflag */

                if ( getlogname( login_prompt, &termio,
				 buf, sizeof(buf) ) == -1 ) continue;

		/* set sane state for login */
		termio.c_lflag = ECHOK | ECHOE | ECHO | ISIG | ICANON;

		termio.c_cc[VEOF] = 0x04;	/* ^D */
#ifndef linux
		termio.c_cc[VEOL] = 0;		/* ^J */
#endif
#ifdef linux
#define VSWTCH VSWTC
#endif
#ifndef _3B1_
		termio.c_cc[VSWTCH] = 0;
#endif

		(void) ioctl(STDIN, TCSETAW, &termio);

		lprintf( L_MESG, "device=%s, pid=%d, calling 'login %s'...\n", Device, getpid(), buf );

		/* hand off to login, (can be a shell script!) */

		(void) execl(login, "login", buf, (char *) NULL);
		(void) execl("/bin/sh", "sh", "-c",
				login, buf, (char *) NULL);
		lprintf(L_FATAL, "cannot execute %s", login);
		exit(FAIL);
	}
}

/*
 *	exit_usage() - exit with usage display
 */

void exit_usage( int code )
{
    lprintf( L_FATAL, "Usage: mgetty [-x debug] [-s speed] [-r] line" );
    exit(code);
}

