#ident "$Id: mgetty-launchd.c,v 1.2 2012/02/22 11:49:49 gert Exp $ Copyright (c) Gert Doering"

/* mgetty-launchd.c
 *
 * init replacement to start, re-start (and possibly rate-limit restarting of)
 * the mgetty process for a single tty
 *
 * this is mainly useful in cluster environments where mgetty activity
 * needs to be controlled by some sort of cluster start/stop script
 * (starting/stopping launchd's) and init cannot be used - like: IBM HACMP
 * and IBM SRC resource manager.
 *
 * $Log: mgetty-launchd.c,v $
 * Revision 1.2  2012/02/22 11:49:49  gert
 * use mgetty path from $SBINDIR in Makefile (-DSBINDIR=...)
 *
 * Revision 1.1  2012/02/22 11:46:39  gert
 * init replacement to launch mgetty in cluster environments (e.g.
 * IBM HACMP) where the mgetty process must run only on a single node
 * (e.g. because the "tty" is connected via tcp/ip to a terminal server)
 *
 */

#include <stdio.h>
#include "syslibs.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

#include "version.h"
#include "mgetty.h"
#include "policy.h"
#include "mg_utmp.h"

#include "config.h"

char	* Device;			/* device to use */
char	* DevID;			/* device name withouth '/'s */

pid_t	mgetty_pid;			/* child process, 0 if no child */

char * mgetty_version = VERSION_LONG;

/* signal handler
 *
 * SIGHUP and SIGTERM are caught, relayed to mgetty, and then exit()
 */

static RETSIGTYPE sig_goodbye _P1 ( (signo), int signo )
{
    if ( mgetty_pid != 0 )
    {
	lprintf( L_AUDIT, "launchd: dev=%s, pid=%d, got signal %d, signal mgetty pid %d, exit",
			  Device, getpid(), signo, mgetty_pid );
    	delay(100); 
	kill(mgetty_pid, signo);
    }
    else
	lprintf( L_AUDIT, "launchd: dev=%s, pid=%d, got signal %d, exiting",
			  Device, getpid(), signo );

    exit(2);
}
    
int main _P2((argc, argv), int argc, char ** argv)
{
    int i;
    char buf[MAXPATH];
    time_t last_start, now;

    /* startup: initialize all signal handlers *NOW*
     */
    (void) signal(SIGHUP, sig_goodbye);
    (void) signal(SIGINT, sig_goodbye);
    (void) signal(SIGQUIT, sig_goodbye);
    (void) signal(SIGTERM, sig_goodbye);

#ifdef HAVE_SIGINTERRUPT
    /* some systems, notable BSD 4.3, have to be told that system
     * calls are not to be automatically restarted after those signals.
     */
    siginterrupt( SIGINT,  TRUE );
    siginterrupt( SIGALRM, TRUE );
    siginterrupt( SIGHUP,  TRUE );
    siginterrupt( SIGUSR1, TRUE );
    siginterrupt( SIGUSR2, TRUE );
#endif

    Device = "unknown";

    /* process the command line
     * 
     * we do not actually parse any option, just take the LAST argument
     * and use it for the logfile naming
     */
    
    if (argc > 1)
        Device = argv[argc-1];
    else {
	lprintf(L_FATAL,"no line given");
	fprintf( stderr, "%s: must specify mgetty options including device name!",argv[0] );
	exit(2);
    }

    /* remove leading /dev/ prefix */
    if ( strncmp( Device, "/dev/", 5 ) == 0 ) Device += 5;

    /* Device ID = Device name without "/dev/", all '/' converted to '-' */
    DevID = strdup( Device );
    for ( i=0; DevID[i] != 0; i++ )
        if ( DevID[i] == '/' ) DevID[i] = '-';
		  
    /* name of the logfile is device-dependant */
    sprintf( buf, LOG_PATH, DevID );
    log_init_paths( argv[0], buf, &Device[strlen(Device)-3] );
    log_set_llevel( 9 );

    lprintf( L_MESG, "mgetty_launchd: %s", mgetty_version);
    lprintf( L_NOISE, "%s compiled at %s, %s", __FILE__, __DATE__, __TIME__ );
    lprintf( L_NOISE, "user id: %d, pid: %d, parent pid: %d", 
		      getuid(), getpid(), getppid());

    /* main loop
     *
     * fork()
     *   child:
     *      setup utmp: INIT_PROCESS
     *      exec mgetty()
     * wait for process
     * put DEAD_PROCESS in utmp/wtmp
     *
     * delay restarting if mgetty fails too frequently
     */

     while(TRUE)
     {
	last_start = time(NULL);

	mgetty_pid = fork();
	if ( mgetty_pid < 0 )
	{
	    lprintf( L_FATAL, "fork() failed" );
	    sleep(10);					/* limit looping */
	    exit(1);
	}

	if ( mgetty_pid == 0 )				/* child */
	{
	    lprintf( L_NOISE, "launchd: child, pid=%d", (int) getpid() );
	    log_close();

	    argv[0] = "mgetty";
	    execv( SBINDIR "/mgetty", argv );

	    lprintf( L_FATAL, "exec() failed" );
	    exit(1);
	}
	else						/* parent */
	{
	    pid_t p;
	    int status;

	    p = wait( &status );

	    if ( p<0 )
		{ lprintf( L_FATAL, "launchd: wait() returned error" ); 
		  exit(2); }
	    
	    mgetty_pid = 0;				/* no child */
	    lprintf( L_NOISE, "launchd: child finished, pid=%d, status=%04x",
				(int) p, status );

	    if ( status != 0 )	/* rate limit on error */
	    {
		now = time(NULL);
		if ( now - last_start < 15 )
		{
		    errno = EINTR;
		    lprintf( L_ERROR, "launchd: mgetty restarting too fast, delaying 60s" );
		    sleep(60); 		/* TODO: more dynamic handling */
		}
	    }

	    /* TODO: DEAD_PROCESS wtmp handling */
	}
     }
     /* NOT REACHED */
     return 0;
}

