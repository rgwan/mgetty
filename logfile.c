#ident "$Id: logfile.c,v 1.11 1993/09/21 15:17:58 gert Exp $ (c) Gert Doering"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <varargs.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include "mgetty.h"

int log_level = LOG_LEVEL;	/* set default log level threshold (jcp) */

static FILE * log_fp;
static boolean mail_logfile = FALSE;
char log_path[ MAXPATH ];

extern int atexit( void (*)(void) );

/* Interactive Unix is a little bit braindead - does not have atexit(),
 * and does not define externals for sys_nerr and sys_errlist,
 * nevertheless I've been told that they exist - so I declare them myself.
 *
 * Strange enough, SVR4 and SunOS seems to be simiarily stupid.
 * Weeeeellll... there was something about POSIX and strerror...
 */
#if defined(ISC) || defined(SVR4) || defined(sun) || defined(__hpux) || \
    defined(3B1)
# define atexit( dummy )
# ifdef 3B1
extern int errno;
# endif

extern int sys_nerr;
extern char *sys_errlist[];
#endif

void logmail( void )
{
char	ws[MAXPATH+100];
char	buf[512];
int	l;
FILE *	pipe_fp;
int	log_fd;

    if ( mail_logfile )
    {
	lprintf( L_MESG, "mailing logfile to %s...", ADMIN );

	sprintf( ws, "%s %s", MAILER, ADMIN );
	pipe_fp = popen( ws, "w" );
	if ( pipe_fp == NULL )
	{
	    lprintf( L_ERROR, "cannot open pipe to %s", MAILER );
	    /* FIXME: write to console - last resort */
	    fprintf( stderr, "cannot open pipe to %s", MAILER );
	    return;
	}

	fprintf( pipe_fp, "Subject: fatal error in logfile\n" );
	fprintf( pipe_fp, "To: %s\n", ADMIN );
	fprintf( pipe_fp, "From: root (Fax Getty)\n" );
	fprintf( pipe_fp, "\n"
	                  "A fatal error has occured! The logfile follows\n" );
	log_fd = open( log_path, O_RDONLY );
	if ( log_fd == -1 )
	{
	    fprintf( pipe_fp, "The logfile '%s' cannot be opened (errno=%d)\n",
		     log_path, errno );
	}
	else
	{
	    do
	    {
	        l = read( log_fd, buf, sizeof( buf ) );
		fwrite( buf, l, 1, pipe_fp );
	    }
	    while( l == sizeof( buf ) );
	    fprintf( pipe_fp, "\n------ logfile ends here -----\n" );
	}
	close( log_fd );
	pclose( pipe_fp );
    }
}

int lputc( int level, char ch )
{
    if ( log_fp != NULL && level <= log_level )
    {
	if ( isprint(ch) ) fputc( ch, log_fp );
		      else fprintf( log_fp, "[%02x]", (unsigned char) ch );
	fflush( log_fp );
    }
    return 0;
}

int lputs( int level, char * s )
{
int retcode = 0;
    if ( log_fp != NULL && level <= log_level )
    {
	retcode = fputs( s, log_fp );
	fflush( log_fp );
    }
    return retcode;
}

int lprintf( level, format, va_alist )
int level;
const char * format;
va_dcl
{
char    ws[200];
time_t  ti;
struct tm *tm;
va_list pvar;
int     errnr;

    va_start( pvar );

    errnr = errno;

    if ( log_fp == NULL )
    {
        if ( log_path[0] == 0 )
	    sprintf( log_path, LOG_PATH, "unknown" );
	log_fp = fopen( log_path, "a" );
	if ( log_fp == NULL )
	{

	    sprintf(ws, "cannot open logfile %s", log_path);
	    perror(ws);
	    exit(10);
	}
	/* make sure that the logfile is not accidently stdin, -out or -err
	 */
	if ( fileno( log_fp ) < 3 )
	{
	int fd;
	    if ( ( fd = fcntl( fileno( log_fp ), F_DUPFD, 3 ) ) > 2 )
	    {
		fclose( log_fp );
		log_fp = fdopen( fd, "a" );
	    }
	}
	fprintf( log_fp, "\n--" );
    }

    vsprintf( ws, format, pvar );
    va_end( pvar );

    ti = time(NULL); tm = localtime(&ti);
    if ( level <= log_level )
    {
	if ( level != L_ERROR && level != L_FATAL )
	{
	    fprintf(log_fp, "\n%d.%d./%02d:%02d:%02d  %s", tm->tm_mday, tm->tm_mon+1,
			     tm->tm_hour, tm->tm_min, tm->tm_sec, ws );
	    fflush(log_fp);
	}
	else
	{
	    fprintf(log_fp, "\n%d.%d./%02d:%02d:%02d  %s: %s", tm->tm_mday, tm->tm_mon+1,
			     tm->tm_hour, tm->tm_min, tm->tm_sec, ws,
			     ( errno <= sys_nerr ) ? sys_errlist[errno]:
			     "<error not in list>" );
	    fflush(log_fp);
	    if ( level == L_FATAL )
	    {
	    FILE * cons_fp;
		if ( ( cons_fp = fopen( CONSOLE, "w" ) ) != NULL )
		{
		    fprintf( cons_fp, "\nmgetty FATAL: %s\n", ws );
		    fclose( cons_fp );
		}
		else	/* last resort */
		{
		    mail_logfile = TRUE;
		    atexit( logmail );
		}
	    }
	}
    }
    return 0;
}
