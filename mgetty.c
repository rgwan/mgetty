#ident "$Id: mgetty.c,v 1.10 1993/03/08 19:14:40 gert Exp $ (c) Gert Doering";
/* some parts of the code are loosely based on the 
 * "getty kit 2.0" by Paul Sutcliffe, Jr., paul@devon.lns.pa.us
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/times.h>
#ifndef linux
#include <sys/select.h>
#endif
#include <sys/stat.h>
#include <signal.h>
#include <utmp.h>
#include <fcntl.h>

#include "mgetty.h"
int getlogname( struct termio * termio, char * buf, int maxsize );

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

unsigned short portspeed = B38400;

/* warning: some modems (for example my old DISCOVERY 2400P) need
   some delay before sending the next command after an OK, so give it here
   as "\\dAT..." */

char *	init_chat_seq[] = { "", "\r\\d\\d\\d+++\\d\\d\\d\r\\dATQ0H0", "OK",
			    /*"\\dAT&V", "OK\n", !!!weg*/
			    "ATS0=0E1&K4&D3&N0", "OK",
#ifndef NO_FAX
                            "AT+FAA=1;+FBOR=0;+FCR=1;+FLID=\""FAX_STATION_ID"\"", "OK",
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



extern	int	errno;

sig_t		timeout();
int		tputc( char c );
void		exit_usage();
struct	utmp	*getutent();
void		pututline(struct utmp * utmp);
void		setutent(void);
void		endutent(void);

int		getopt( int, char **, char * );

time_t		time( long * tloc );

char	* Device;			/* device to use */

boolean	toggle_dtr = TRUE;		/* lower DTR */
int	toggle_dtr_waittime = 500;	/* milliseconds while DTR is low */

int	prompt_waittime = 500;		/* milliseconds between CONNECT and */
					/* login: -prompt */

int main( int argc, char ** argv)
{
	register int c, fd;
	char devname[MAXLINE+1];
	char buf[MAXLINE+1];
	struct termio termio;
	FILE *fp;
	struct utmp *utmp;
	struct stat st;
	int Nusers;

	action_t	what_action;

	time_t	clock;

	fd_set	readfds;
	int	slct;


	struct passwd *pwd;
	uid_t	uucpuid = 0;
	gid_t	uucpgid = 0;

	int pid;

	char *issue = "/etc/issue";		/* default issue file */

	char *login = "/bin/login";		/* default login program */

	extern int optind;
	extern char *optarg;
	extern int expect();

	/* startup
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);

	Device = "unknown";
	toggle_dtr = TRUE;

	/* process the command line
	 */

	while ((c = getopt(argc, argv, "x:")) != EOF) {
		switch (c) {
		case 'x':
			log_level = atoi(optarg);
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

	/* need full name of the device */
	sprintf(devname, "/dev/%s", Device);

	/* name of the logfile is device-dependant */
	sprintf( log_path, "/tmp/log_m_%s", Device );

	lprintf(L_MESG, "check for lockfiles");

	/* deal with the lockfiles; we don't want to charge
	 * ahead if uucp, kermit or something is already
	 * using the line.
	 */

	/* name the lock file(s)
	 */
	(void) sprintf(buf, LOCK, Device);
	lock = strdup(buf);

	/* check for existing lock file(s)
	 */
	if (checklock(lock) == TRUE) {
		while (checklock(lock) == TRUE)
			(void) sleep(10);
		exit(0);
	}
	lprintf(L_MESG, "locking the line");

	/* try to lock the line
	 */
	if (makelock(lock) == FAIL) {
		while (checklock(lock) == TRUE)
			(void) sleep(10);
		exit(0);
	}

	/* allow uucp to access the device
	 */
	(void) chmod(devname, 0666);
	if ((pwd = getpwuid(UUCPID)) != (struct passwd *) NULL) {
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

	if (toggle_dtr) {
		lprintf( L_MESG, "lowering DTR to reset Modem" );
		(void) ioctl(STDIN, TCGETA, &termio);
		termio.c_cflag &= ~CBAUD;	/* keep all but CBAUD bits */
		termio.c_cflag |= B0;		/* set speed == 0 */
		(void) ioctl(STDIN, TCSETAF, &termio);
		delay(toggle_dtr_waittime);
	}

	termio.c_iflag = ICRNL | IXANY;	/*!!!!!! ICRNL??? */
	termio.c_oflag = OPOST | ONLCR;
#ifdef linux
	termio.c_cflag = portspeed | CS8 | CREAD | HUPCL | CLOCAL |
                         CRTSCTS;
#else
	termio.c_cflag = portspeed | CS8 | CREAD | HUPCL | CLOCAL |
                         RTSFLOW | CTSFLOW;
#endif
	termio.c_lflag = ISIG;		/* no echo for chat with modem! */
	termio.c_line = 0;
	termio.c_cc[VMIN] = 1;
	termio.c_cc[VTIME]= 0;
	ioctl (STDIN, TCSETAF, &termio);

	/* handle init chat if requested
	 */
	if ( do_chat( init_chat_seq, init_chat_actions, &what_action,
                      init_chat_timeout, TRUE, FALSE ) == FAIL )
	{
	    lprintf( L_MESG, "init chat failed, exiting..." );
	    rmlocks();
	    exit(1);
	}

	/* wait .3s for line to clear (some modems send a \n after "OK",
	   this may confuse the "call-chat"-routines) */

	clean_line(3);

	/* remove locks, so any other process can dial-out. When waiting
	   for "RING" we check for foreign lockfiles, if there are any, we
	   give up the line - otherwise we lock it again */

	rmlocks();	

	/* wait for incoming characters.
	   I use select() instead of a blocking read(), since select()
	   does *not* eat up characters and thus prevents collisions
	   with dial-outs */

	lprintf( L_MESG, "waiting..." );
	FD_ZERO( &readfds );
	FD_SET( STDIN, &readfds );
	slct = select( 1, &readfds, NULL, NULL, NULL );
	lprintf( L_NOISE, "select returned %d", slct );

	/* check for LOCK files, if there are none, grab line and lock it
	*/

	lprintf( L_NOISE, "checking lockfiles, locking the line" );

	if ( makelock(lock) == FAIL) {
	    lprintf( L_NOISE, "lock file exists!" );

	    /* close stdin -> other processes can read port */
	    close(0);
	    close(1);
	    close(2);

	    /* this is kind of tricky: sometimes uucico dial-outs do still
	       collide with mgetty. So, when uucico times out, I do
	       *immediately* restart it. The double look makes sure that
	       mgetty gives me *at least* 5 seconds to restart uucico */

	    do {
		    /* wait for lock to disappear */
		    while (checklock(lock) == TRUE)
			sleep(10);	/*!!!!! ?? wait that long? */

		    /* wait a moment, then check for reappearing locks */
		    sleep(5);
	    }
	    while ( checklock(lock) == TRUE );	

	    /* OK, leave & get restarted by init */
	    exit(0);
	}

	/* check for a "nologin" file (/etc/nologin.<device>) */

	sprintf( buf, "/etc/nologin.%s", Device );

	if ( access( buf, F_OK ) == 0 )
	{
	    lprintf( L_MESG, "%s exists - do not accept call!", buf );
	    clean_line( 80 );		/* wait for last ring */
	    exit(1);
	}

	/* wait for "RING" (and check for LOCK-Files while reading the
           first string), if "RING" found and no lock-file, lock the line
           and send manual answer string to the modem */

	log_level++; /*FIXME: remove this !!!!!!!!!!!*/
	if ( do_chat( call_chat_seq, call_chat_actions, &what_action,
                      call_chat_timeout, TRUE, FALSE ) == FAIL )
	{
	    if ( what_action == A_FAX )
	    {
		lprintf( L_MESG, "action is A_FAX, start fax receiver...");
		termio.c_iflag = 0;		/* do NOT process input! */
		termio.c_lflag &= ~ISIG;	/* disable signals! */
		ioctl (STDIN, TCSETAF, &termio);
		faxrec();
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
           string sent by a calling MNP-Modem is discarded here, too */

	clean_line(3);

	/* set to remove lockfile(s) on certain signals
	 */
	(void) signal(SIGHUP, rmlocks);
	(void) signal(SIGINT, rmlocks);
	(void) signal(SIGQUIT, rmlocks);
	(void) signal(SIGTERM, rmlocks);

	pid = getpid();
	while ((utmp = getutent()) != (struct utmp *) NULL) {
		if (utmp->ut_type == INIT_PROCESS && utmp->ut_pid == pid) {

			lprintf(L_NOISE, "utmp entry made");
			/* show login process in utmp
			 */
			strcpy(utmp->ut_line, Device);
			strcpy(utmp->ut_user, "LOGIN");
			utmp->ut_pid = pid;
			utmp->ut_type = LOGIN_PROCESS;
			(void) time(&clock);
			utmp->ut_time = clock;
			pututline(utmp);

			/* write same record to end of wtmp
			 * if wtmp file exists
			 */
			if (stat(WTMP_FILE, &st) && errno == ENOENT)
				break;
			if ((fp = fopen(WTMP_FILE, "a")) != (FILE *) NULL) {
				(void) fseek(fp, 0L, 2);
				(void) fwrite((char *)utmp,sizeof(*utmp),1,fp);
				(void) fclose(fp);
			}
		}
	}
	endutent();

        delay( prompt_waittime );
	/* loop until a successful login is made
	 */
	for (;;) {

		/* set Nusers value to number of users
		 */
		Nusers = 0;
		setutent();
		while ((utmp = getutent()) != (struct utmp *) NULL) {
		    if (utmp->ut_type == USER_PROCESS)
		    {
			Nusers++;
			/*lprintf(L_NOISE, "utmp entry (%s)", utmp->ut_name); */
		    }
		}
		endutent();
		/*lprintf(L_NOISE, "Nusers=%d", Nusers);*/

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
		termio.c_cflag = CS8 | portspeed | CREAD | HUPCL;
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

                if ( getlogname(&termio, buf, sizeof(buf) ) == -1 ) continue;

		/* set sane state for login */
		termio.c_lflag = ECHOK | ECHOE | ECHO | ISIG | ICANON;

		termio.c_cc[VEOF] = 0x04;	/* ^D */
#ifndef linux
		termio.c_cc[VEOL] = 0;		/* ^J */
#endif
#ifdef linux
#define VSWTCH VSWTC
#endif
		termio.c_cc[VSWTCH] = 0;

		(void) ioctl(STDIN, TCSETAW, &termio);

		lprintf( L_MESG, "pid=%d, calling 'login %s'...\n", pid, buf );

		/* hand off to login, (can be a shell script!) */

		/* do NOT handle "login ZyXEL" as special case for
		 * zyxel fax mode any more - since all fax stuff is
		 * class 2 now, fax calles are handled differently
		 */

		(void) execl(login, "login", buf, (char *) NULL);
		(void) execl("/bin/sh", "sh", "-c",
				login, buf, (char *) NULL);
		lprintf(L_FATAL, "cannot execute %s", login);
		exit(FAIL);

	}
}


/*
**	timeout() - handles SIGALRM
*/

sig_t
timeout()
{
	struct termio termio;

	/* say bye-bye
	 */
	(void) fprintf(stdout, "\nTimed out after %d seconds.\n", -1);
	(void) fputs("Bye Bye.\n", stdout);

	/* force a hangup
	 */
	(void) ioctl(STDIN, TCGETA, &termio);
	termio.c_cflag &= ~CBAUD;
	termio.c_cflag |= B0;
	(void) ioctl(STDIN, TCSETAF, &termio);

	exit(1);
}

/*
**	exit_usage() - exit with usage display
*/

void
exit_usage(code)
int code;
{
	FILE *fp;

	if ((fp = fopen(CONSOLE, "w")) != (FILE *) NULL) {
		(void) fprintf(fp, "Usage: mgetty [-x debug] line\n");
		(void) fclose(fp);
	}
	exit(code);
}

