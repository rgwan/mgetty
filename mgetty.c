#ident "$Id: mgetty.c,v 1.128 1994/08/20 23:57:54 gert Exp $ Copyright (c) Gert Doering"

/* mgetty.c
 *
 * mgetty main module - initialize modem, lock, get log name, call login
 *
 * some parts of the code (lock handling, writing of the utmp entry)
 * are based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#include <stdio.h>
#include "syslibs.h"
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/times.h>

#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "mgetty.h"
#include "policy.h"
#include "tio.h"
#include "fax_lib.h"
#ifdef VOICE
#include "voclib.h"
#endif

#include "mg_utmp.h"

#include "version.h"		/* for logging the mgetty release number */


#ifndef MODEM_CHECK_TIME
# define MODEM_CHECK_TIME -1	/* no check */
#endif

unsigned short portspeed = B0;	/* indicates has not yet been set */

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
					{ "+FCON",	A_FAX  },
					{ "+FCO\r",	A_FAX  },
					{ "FAX",	A_FAX  },
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

#ifdef ELINK
char *	answer_chat_seq[] = { "", "AT\\\\OA", "CONNECT", "\\c", "\n", NULL };
#else
char *	answer_chat_seq[] = { "", "ATA", "CONNECT", "\\c", "\n", NULL };
#endif /* !ELINK */

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
char	* DevID;			/* device name withouth '/'s */
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

#ifdef MGETTY_PID_FILE
/* create a file with the process ID of the mgetty currently
 * active on a given device in it.
 */
static char pid_file_name[ MAXPATH ];
static void make_pid_file _P0( void )
{
    FILE * fp;

    sprintf( pid_file_name, MGETTY_PID_FILE, DevID );

    fp = fopen( pid_file_name, "w" );
    if ( fp == NULL )
	lprintf( L_ERROR, "can't create pid file %s", pid_file_name );
    else
    {
	fprintf( fp, "%d\n", (int) getpid() ); fclose( fp );
    }
}
#endif
    

enum { St_unknown,
       St_go_to_jail,			/* reset after unwanted call */
       St_waiting,			/* wait for activity on tty */
       St_check_modem,			/* check if modem is alive */
       St_wait_for_RINGs,		/* wait for <n> RINGs before ATA */
       St_answer_phone,			/* ATA, wait for CONNECT/+FCO(N) */
       St_nologin,			/* no login allowed, wait for
					   RINGing to stop */
       St_dialout,			/* parallel dialout, wait for
					   lockfile to disappear */
       St_get_login,			/* prompt "login:", call login() */
       St_incoming_fax			/* +FCON detected */
   } mgetty_state = St_unknown;
       
int main _P2((argc, argv), int argc, char ** argv)
{
    register int c;
    
    char devname[MAXLINE+1];		/* full device name (with /dev/) */
    char buf[MAXLINE+1];
    TIO	tio;
    FILE *fp;
    int i;
    int cspeed;
    
    action_t	what_action;
    int		rings = 0;
#ifdef NO_FAX
    boolean	data_only = TRUE;
#else
    boolean	data_only = FALSE;
#endif
    char	* modem_class = DEFAULT_MODEMTYPE;	/* policy.h */
    boolean	autobauding = FALSE;
    
#if defined(_3B1_) || defined(MEIBE)
    typedef ushort uid_t;
    typedef ushort gid_t;
#endif
#if defined(_3B1_) || defined(MEIBE) || defined(sysV68)
    extern struct passwd *getpwuid(), *getpwnam();
#endif

    struct passwd *pwd;
    uid_t	uucpuid = 5;			/* typical uid for UUCP */
    gid_t	uucpgid = 0;

    char *issue = "/etc/issue";		/* default issue file */
    
    char * login_prompt = NULL;		/* login prompt */

    char * fax_server_file = NULL;		/* -S <file> */
    char * fax_station_id = FAX_STATION_ID;	/* -I <id> */

    boolean	blocking_open = FALSE;	/* open device in blocking mode */
    
#ifdef VOICE
    boolean	use_voice_mode = TRUE;
    
    voice_path_init();
#endif
	
    /* startup
     */
    (void) signal(SIGHUP, SIG_IGN);
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

    /* some magic done by the command's name */
    if ( strcmp( get_basename( argv[0] ), "getty" ) == 0 )
    {
	blocking_open = TRUE;
	direct_line = TRUE;
    }

    while ((c = getopt(argc, argv, "c:x:s:rp:n:i:DC:S:m:I:ba")) != EOF)
    {
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
	    log_set_llevel( atoi(optarg) );
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
	  case 'D':			/* switch off fax */
	    data_only = TRUE;
	    break;
	  case 'C':
	    modem_class = optarg;
	    if ( strcmp( modem_class, "data" ) == 0 ) data_only = TRUE;
	    break;
	  case 'S':
	    fax_server_file = optarg;
	    break;
	  case 'I':
	    fax_station_id = optarg; break;
	  case 'b':			/* open port in blocking mode */
	    blocking_open = TRUE; break;
	  case 'a':			/* autobauding */
	    autobauding = TRUE; break;
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
    sprintf( devname, "/dev/%s", Device);

    /* Device ID = Device name without "/dev/", all '/' converted to '-' */
    DevID = mydup( Device );
    for ( i=0; DevID[i] != 0; i++ )
        if ( DevID[i] == '/' ) DevID[i] = '-';
		  
    /* name of the logfile is device-dependant */
    sprintf( buf, LOG_PATH, DevID );
    log_init_paths( argv[0], buf, &Device[strlen(Device)-3] );
    lprintf( L_NOISE, "mgetty: %s", mgetty_version );
	    
#ifdef USE_GETTYDEFS
    if (optind < argc)
        GettyID = argv[optind++];
    else {
	lprintf(L_WARN, "no gettydef tag given");
	GettyID = GETTYDEFS_DEFAULT_TAG;
    }
#endif

#ifdef MGETTY_PID_FILE
    make_pid_file();
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
    if (checklock(Device) != NO_LOCK)
    {
	while (checklock(Device) != NO_LOCK) sleep(10);
	exit(0);
    }

    /* try to lock the line
     */
    lprintf(L_MESG, "locking the line");

    if ( makelock(Device) == FAIL )
    {
	while( checklock(Device) != NO_LOCK ) sleep(10);
	exit(0);
    }

    /* the line is mine now ...  */

    /* allow uucp to access the device
     */
    if ((pwd = getpwnam(UUCPID)) != (struct passwd *) NULL)
    {
	uucpuid = pwd->pw_uid;
	uucpgid = pwd->pw_gid;
    }
    (void) chown(devname, uucpuid, uucpgid);
    (void) chmod(devname, FILE_MODE);

    /* open the device; don't wait around for carrier-detect */
    if ( mg_open_device( devname, blocking_open ) == ERROR ) /* mg_m_init.c */
    {
	lprintf( L_FATAL, "open device %s failed, exiting", devname );
	exit( FAIL );
    }
    
    /* setup terminal */

    /* Currently, the tio returned here is ignored.
       The invocation is only for the sideeffects of:
       - loading the gettydefs file if enabled.
       - setting portspeed appropriately, if not defaulted.
       */

    tio = *gettermio(GettyID, TRUE, &login_prompt);

    if ( mg_init_device( STDIN, toggle_dtr, toggle_dtr_waittime,
			 portspeed ) == ERROR )
    {
	lprintf( L_FATAL, "cannot initialize device, exiting" );
	exit( 20 );
    }
    
    /* drain input - make sure there are no leftover "NO CARRIER"s
     * or "ERROR"s lying around from some previous dial-out
     */

    clean_line( STDIN, 1);

    /* do modem initialization, normal stuff first, then fax
     */
    if ( ! direct_line )
    {
	if ( mg_init_data( STDIN ) == FAIL )
	{
	    rmlocks();
	    exit(1);
	}
	/* initialize ``normal'' fax functions */
	if ( ( ! data_only ) &&
	     mg_init_fax( STDIN, modem_class, fax_station_id ) == SUCCESS )
	{
	    /* initialize fax polling server (only if faxmodem) */
	    if ( fax_server_file )
	    {
		faxpoll_server_init( STDIN, fax_server_file );
	    }
	}
#ifdef VOICE
	if ( mg_init_voice( STDIN ) == FAIL )
	{
	    use_voice_mode = FALSE;
	} else {
	    use_voice_mode = TRUE;
	}
#endif
    }

    /* wait .3s for line to clear (some modems send a \n after "OK",
       this may confuse the "call-chat"-routines) */

    clean_line( STDIN, 3);

    /* remove locks, so any other process can dial-out. When waiting
       for "RING" we check for foreign lockfiles, if there are any, we
       give up the line - otherwise we lock it again */

    rmlocks();	

    /* sometimes it may be desired to have mgetty pick up the phone even
       if it didn't RING often enough (because you accidently picked it up
       manually...) or if it didn't RING at all (because you have a fax
       machine directly attached to the modem...), so send mgetty a signal
       SIGUSR1 and it will behave as if a RING was seen
       */
    
    signal( SIGUSR1, sig_pick_phone );

#if ( defined(linux) && defined(NO_SYSVINIT) ) || defined(sysV68)
    /* on linux, "simple init" does not make a wtmp entry when you
     * log so we have to do it here (otherwise, "who" won't work) */
    make_utmp_wtmp( Device, UT_INIT, "uugetty", NULL );
#endif

#ifdef VOICE
    if ( use_voice_mode ) {
	/* With external modems, the auto-answer LED can be used
	 * to show a status flag. vgetty uses this to indicate
	 * that new messages have arrived.
	 */
	voice_message_light();
    }
#endif /* VOICE */

    /* set to remove lockfile(s) on certain signals (SIGHUP is ignored)
     */
    (void) signal(SIGINT, sig_goodbye);
    (void) signal(SIGQUIT, sig_goodbye);
    (void) signal(SIGTERM, sig_goodbye);

    /* sleep... waiting for activity */
    mgetty_state = St_waiting;

    while ( mgetty_state != St_get_login )
    {
	switch (mgetty_state)	/* state machine */
	{
	  case St_go_to_jail:
	    /* after a rejected call (caller ID, not enough RINGs,
	     * /etc/nologin file), do some cleanups, and go back to
	     * field one: St_waiting
	     */
	    rmlocks();
	    mgetty_state = St_waiting;
	    break;
	    
	  case St_waiting:
	    /* wait for incoming characters (using select() or poll() to
	     * prevent eating away from processes dialing out)
	     */
	    lprintf( L_MESG, "waiting..." );

	    /* ignore accidential sighup, caused by dialout or such
	     */
	    signal( SIGHUP, SIG_IGN );
	    
	    /* here's mgetty's magic. Wait with select() or something
	     * similar non-destructive for activity on the line.
	     * If called with "-b" or as "getty", the blocking has
	     * already happened in the open() call.
	     */
	    if ( ! blocking_open )
	    {
		if ( ! wait_for_input( STDIN, MODEM_CHECK_TIME*1000 ) &&
		     ! direct_line )
		{
		    /* no activity - is the modem alive or dead? */
		    mgetty_state = St_check_modem;
		    break;
		}
	    }
	
	    /* check for LOCK files, if there are none, grab line and lock it
	     */
    
	    lprintf( L_NOISE, "checking lockfiles, locking the line" );

	    if ( makelock(Device) == FAIL)
	    {
		lprintf( L_NOISE, "lock file exists (dialout)!" );
		mgetty_state = St_dialout;
		break;
	    }

	    /* now: honour SIGHUP
	     */
	    signal(SIGHUP, sig_goodbye );

	    rings = 0;
	    
	    /* check, whether /etc/nologin.<device> exists. If yes, do not
	       answer the phone. Instead, wait for ringing to stop. */
#ifdef NOLOGIN_FILE
	    sprintf( buf, NOLOGIN_FILE, DevID );

	    if ( access( buf, F_OK ) == 0 )
	    {
		lprintf( L_MESG, "%s exists - do not accept call!", buf );
		mgetty_state = St_nologin;
		break;
	    }
#endif
	    mgetty_state = St_wait_for_RINGs;
	    break;


	  case St_check_modem:
	    /* some modems have the nasty habit of just dying after some
	       time... so, mgetty regularily checks with AT...OK whether
	       the modem is still alive */
	    lprintf( L_MESG, "checking if modem is still alive" );

	    if ( makelock( Device ) == FAIL )
	    {
		mgetty_state = St_dialout; break;
	    }

	    /* try twice */
	    if ( mdm_command( "AT", STDIN ) == SUCCESS ||
		 mdm_command( "AT", STDIN ) == SUCCESS )
	    {
		mgetty_state = St_go_to_jail; break;
	    }

	    lprintf( L_FATAL, "modem on %s doesn't react!", devname );

	    /* give up */
	    exit( 30 );

	    break;
		
	  case St_nologin:
#ifdef NOLOGIN_FILE
	    /* if a "/etc/nologin.<device>" file exists, wait for RINGing
	       to stop, but count RINGs (in case the user removes the
	       nologin file while the phone is RINGing), and if the modem
	       auto-answers, handle it properly */
	    
	    sprintf( buf, NOLOGIN_FILE, DevID );

	    /* while phone is ringing... */
	    
	    while ( do_chat( STDIN, ring_chat_seq, ring_chat_actions,
			      &what_action, 10, TRUE ) == SUCCESS )
	    {
		rings++;
		if ( access( buf, F_OK ) != 0 ||	/* removed? */
		     virtual_ring == TRUE )		/* SIGUSR1? */
		{
		    mgetty_state = St_wait_for_RINGs;	/* -> accept */
		    break;
		}
	    }

	    /* did nologin file disappear? */
	    if ( mgetty_state != St_nologin ) break;

	    /* phone stopped ringing (do_chat() != SUCCESS) */
	    switch( what_action )
	    {
	      case A_TIMOUT:	/* stopped ringing */
		lprintf( L_AUDIT, "rejected, rings=%d", rings );
		mgetty_state = St_go_to_jail;
		break;
	      case A_CONN:	/* CONNECT */
		clean_line( STDIN, 5 );
		printf( "\r\n\r\nSorry, no login allowed\r\n" );
		printf( "\r\nGoodbye...\r\n\r\n" );
		sleep(5); exit(20); break;
	      case A_FAX:	/* +FCON */
		mgetty_state = St_incoming_fax; break;
	      default:
		lprintf( L_MESG, "unexpected action: %d", what_action );
		exit(20);
	    }
#endif
	    break;


	  case St_dialout:
	    /* the line is locked, a parallel dialout is in process */

	    /* close all file descriptors -> other processes can read port */
	    close(0);
	    close(1);
	    close(2);

	    /* write a note to utmp/wtmp about dialout
	     * (don't do this on two-user-license systems!)
	     */
#ifndef USER_LIMIT
	    i = checklock( Device );		/* !! FIXME, ugly */
	    make_utmp_wtmp( Device, UT_USER, "dialout", get_ps_args(i) );
#endif

	    /* this is kind of tricky: sometimes uucico dial-outs do still
	       collide with mgetty. So, when my uucico times out, I do
	       *immediately* restart it. The double check makes sure that
	       mgetty gives me at least 5 seconds to restart uucico */

	    do {
		/* wait for lock to disappear */
		while ( checklock(Device) != NO_LOCK ) sleep(10);

		/* wait a moment, then check for reappearing locks */
		sleep(5);
	    }
 	    while ( checklock(Device) != NO_LOCK );	

	    /* OK, leave & get restarted by init */
	    exit(0);
	    break;


	  case St_wait_for_RINGs:
	    /* Wait until the proper number of RING strings have been
	       seen. In case the modem auto-answers (yuck!) or someone
	       hits DATA/VOICE, we'll accept CONNECT, +FCON, ... also. */
	       
	    if ( direct_line )		/* no RING needed */
	    {
		mgetty_state = St_get_login;
		break;
	    }
	    
#ifdef VOICE
	    if ( use_voice_mode ) {
		/* check how many RINGs we're supposed to wait for */
		voice_rings(&rings_wanted);
	    }
#endif /* VOICE */

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

	    /* enough rings? */
	    if ( rings >= rings_wanted
#ifdef DIST_RING
		|| ( what_action >= A_RING1 && what_action <= A_RING5 )
#endif
		)
	    {
		mgetty_state = St_answer_phone; break;
	    }

	    /* not enough rings, timeout or action? */

	    switch( what_action )
	    {
	      case A_TIMOUT:		/* stopped ringing */
		if ( rings == 0 )	/* no ring *AT ALL* */
		{
		    lprintf( L_WARN, "huh? Junk on the line?" );
		    rmlocks();		/* line is free again */
		    exit(0);		/* let init restart mgetty */
		}
		lprintf( L_MESG, "phone stopped ringing (rings=%d)", rings );
		mgetty_state = St_go_to_jail;
		break;
	      case A_CONN:		/* CONNECT */
		mgetty_state = St_get_login; break;
	      case A_FAX:		/* +FCON */
		mgetty_state = St_incoming_fax; break;
#ifdef VOICE
	      case A_VCON:
		voice_button(rings);
		use_voice_mode = FALSE;
		mgetty_state = St_answer_phone;
		break;
#endif
	      case A_FAIL:
		lprintf( L_AUDIT, "failed A_FAIL dev=%s, pid=%d, caller=%s",
			          Device, getpid(), CallerId );
		exit(20);
	      default:
		lprintf( L_MESG, "unexpected action: %d", what_action );
		exit(20);
	    }
	    break;


	  case St_answer_phone:
	    /* Answer an incoming call, after the desired number of
	       RINGs. If we have caller ID information, and checking
	       it is desired, do it now, and possibly reject call if
	       not allowed in */
	    
	    if ( !cndlookup() )
	    {
		lprintf( L_AUDIT, "denied caller dev=%s, pid=%d, caller=%s",
			 Device, getpid(), CallerId);
		clean_line( STDIN, 80 ); /* wait for ringing to stop */

		mgetty_state = St_go_to_jail;
		break;
	    }

	    /* from here, there's no way back. Either the call will succeed
	       and mgetty will exec() something else, or it will fail and
	       mgetty will exit(). */
	    
	    /* get line as ctty: hangup will come through
	     */
	    mg_get_ctty( STDIN, devname );
		
	    /* remember time of phone pickup */
	    call_start = time( NULL );

#ifdef VOICE
	    if ( use_voice_mode ) {
		/* Answer in voice mode. The function will return only if it
		   detects a data call, otherwise it will call exit(). */
		
		/* The modem will be in voice mode when voice_answer is
		   called. If the function returns, the modem is ready
		   to be connected in DATA mode with ATA. */
		
		voice_answer(rings, rings_wanted, what_action );
	    }
#endif /* VOICE */

	    if ( do_chat( STDIN, answer_chat_seq, answer_chat_actions,
			 &what_action, answer_chat_timeout, TRUE) == FAIL )
	    {	
		if ( what_action == A_FAX )
		{
		    mgetty_state = St_incoming_fax;
		    break;
		}

		lprintf( L_AUDIT, 
		   "failed %s dev=%s, pid=%d, caller=%s, conn='%s', name='%s'",
		    what_action == A_TIMOUT? "timeout": "A_FAIL", 
		    Device, getpid(), CallerId, Connect, CallName );
  
		rmlocks();
		exit(1);
	    }

	    /* some (old) modems require the host to change port speed
	     * to the speed returned in the CONNECT string, usually
	     * CONNECT 2400 / 1200 / "" (meaning 300)
	     */
	    if ( autobauding )
	    {
		if ( strlen( Connect ) == 0 )	/* "CONNECT\r" */
		    cspeed = 300;
		else
		    cspeed = atoi(Connect);

		lprintf( L_MESG, "autobauding: switch to %d bps", cspeed );
		
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
		    lprintf( L_ERROR, "autobauding: cannot parse 'CONNECT %s'",
			               Connect );
		}
		else
		{
		    tio_set_speed( &tio, portspeed );
		    tio_set( STDIN, &tio );
		}
	    }
	    
	    mgetty_state = St_get_login;
	    break;
	    
	  case St_incoming_fax:
	    /* incoming fax, receive it (->faxrec.c) */

	    lprintf( L_MESG, "start fax receiver..." );
	    faxrec( FAX_SPOOL_IN );
	    rmlocks();
	    exit( 0 );
	    break;
	    
	  default:
	    /* unknown machine state */
	    
	    lprintf( L_WARN, "unknown state: %s", mgetty_state );
	    exit( 33 );
	}		/* end switch( mgetty_state ) */
    }			/* end while( state != St_get_login ) */

    /* this is "state St_get_login". Not included in switch/case,
       because it doesn't branch to other states. It may loop for
       a while, but it will never return
       */

    /* wait for line to clear (after "CONNECT" a baud rate may
       be sent by the modem, on a non-MNP-Modem the MNP-request
       string sent by a calling MNP-Modem is discarded here, too) */
    
    clean_line( STDIN, 3);

    /* honor carrier now: terminate if modem hangs up prematurely
     */
    tio_get( STDIN, &tio );
    tio_carrier( &tio, TRUE );
    tio_set( STDIN, &tio );
    
    /* make utmp and wtmp entry (otherwise login won't work)
     */
    make_utmp_wtmp( Device, UT_LOGIN, "LOGIN", Connect );

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
#ifdef sunos4
	/* we have carrier, assert data flow control */
	tio_set_flow_control( STDIN, &tio, DATA_FLOW );
#endif
	tio_set( STDIN, &tio );
		
	fputc('\r', stdout);	/* just in case */

	/* display ISSUE, if present
	 */
	if (*issue != '/')
	{
	    printf( "%s\r\n", ln_escape_prompt( issue ) );
	}
	else if ((fp = fopen(issue, "r")) != (FILE *) NULL)
	{
	    while (fgets(buf, sizeof(buf), fp) != (char *) NULL)
	    {
		char * p = ln_escape_prompt( buf );
		if ( p != NULL ) fputs( p, stdout );
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
	/* read a login name from tty
	   (if just <cr> is pressed, re-print issue file)

	   also adjust ICRNL / IGNCR to characters recv'd at end of line:
	   cr+nl -> IGNCR, cr -> ICRNL, NL -> 0/ and: cr -> ONLCR, nl -> 0
	   for c_oflag */

	if ( getlogname( login_prompt, &tio,
			 buf, sizeof(buf) ) == -1 ) continue;

	/* remove PID file (mgetty is due to exec() login) */
#ifdef MGETTY_PID_FILE
	(void) unlink( pid_file_name );
#endif
	
	/* hand off to login (dispatcher) */
	login( buf );

	/* doesn't return, if it does, something broke */
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
