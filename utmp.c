#ident "$Id: utmp.c,v 1.1 1993/09/01 00:43:35 gert Exp $ (c) Gert Doering";
/* some parts of the code (writing of the utmp entry)
 * is based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#include <stdio.h>
#include <unistd.h>
#include <utmp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mgetty.h"

/* define some prototypes - not all supported systems have these */

#if !defined(SVR4) && !defined(linux)
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
