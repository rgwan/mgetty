#ident "$Id: utmp.c,v 1.3 1993/09/13 21:03:09 gert Exp $ (c) Gert Doering";
/* some parts of the code (writing of the utmp entry)
 * is based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#ifndef sun
#include <stdio.h>
#include <unistd.h>
#include <utmp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef ENOENT
#include <errno.h>
#endif

#endif

#include "mgetty.h"

#ifdef sun
/* on SunOS, the getty process does not have to care for the utmp
 * entries, login and init do all the worko
 * Anyway, we have to _read_ it to get the number of users logged in.
 */
void make_utmp_wtmp( char * line, boolean login_process ) {}
int get_current_users( void ) { return 0; }	/*! FIXME */
#else

/* define some prototypes - not all supported systems have these */
#if !defined(SVR4) && !defined(linux) && !defined(__hpux)
struct	utmp	*getutent();
struct	utmp	*pututline(struct utmp * utmp);
void		setutent(void);
void		endutent(void);
#endif

void make_utmp_wtmp( char * line, boolean login_process )
{
struct utmp *utmp;
pid_t	pid;
time_t	clock;
struct stat	st;
FILE *	fp;

    pid = getpid();
    while ((utmp = getutent()) != (struct utmp *) NULL)
    {
	if (utmp->ut_type == INIT_PROCESS && utmp->ut_pid == pid)
	{
	    lprintf(L_NOISE, "utmp + wtmp entry made");

	    strcpy(utmp->ut_line, line );

	    (void) time(&clock);
	    utmp->ut_time = clock;

	    if ( login_process )
	    {		/* show login process in utmp */
		strcpy(utmp->ut_user, "LOGIN");
		utmp->ut_type = LOGIN_PROCESS;
	    }
	    else
	    {		/* still waiting for call */
		strcpy(utmp->ut_user, "uugetty" );
		utmp->ut_type = INIT_PROCESS;
	    }

	    pututline(utmp);

	    /* write same record to end of wtmp
	     * if wtmp file exists
	     */
	    if (stat(WTMP_FILE, &st) && errno == ENOENT)
		    break;
	    if ((fp = fopen(WTMP_FILE, "a")) != (FILE *) NULL)
	    {
		(void) fseek(fp, 0L, SEEK_END);
		(void) fwrite((char *)utmp,sizeof(*utmp),1,fp);
		(void) fclose(fp);
	    }
	}
    }
    endutent();
}

int get_current_users(void)
{
struct utmp * utmp;
int Nusers;

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
    return Nusers;
}
#endif		/* !sun */
