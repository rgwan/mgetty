static char sccsid[]="$Id: do_chat.c,v 1.3 1993/03/11 11:09:58 gert Exp $ (c) Gert Doering";
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termio.h>
#include <signal.h>
#ifdef linux
#include <sys/ioctl.h>
#endif

#include "mgetty.h"

#ifdef USE_POLL
#include <poll.h>
int poll( struct pollfd fds[], unsigned long nfds, int timeout );
#endif

void delay( int waittime )		/* wait waittime milliseconds */
{
#ifdef USE_POLL
struct pollfd sdummy;
    poll( &sdummy, 0, waittime );
#else
#ifdef USE_NAP
    nap( (long) waittime );
#else				/* neither poll nor nap available */
    if ( waittime < 1000 ) waittime = 1000;	/* round up */
    sleep( waittime / 1000);
#endif
#endif
}
    
boolean chat_has_timeout;
void chat_timeout( void )
{
    chat_has_timeout = TRUE;
}

int do_chat( char * expect_send[],
	     chat_action_t actions[], action_t * action,
             int chat_timeout_time, boolean timeout_first,
             boolean locks )
{
#define BUFFERSIZE 500
char	buffer[BUFFERSIZE];
int	i,cnt;
int	retcode = SUCCESS;
int	str;
char	*p;
int	h;
boolean	nocr;
struct termio	termio, save_termio;

    (void) ioctl(STDIN, TCGETA, &termio);
    save_termio = termio;
    termio.c_iflag &= ~ICRNL;
    termio.c_lflag &= ~ICANON;
    termio.c_cc[VMIN] = 1;
    termio.c_cc[VTIME] = 0;
    (void) ioctl(STDIN, TCSETA, &termio);

    signal( SIGALRM, chat_timeout );

    str=0;
    while ( expect_send[str] != NULL )
    {
	/* expect a string (expect_send[str] or abort[]) */
	i = 0;

	if ( strlen( expect_send[str] ) != 0 )
	{
	    lprintf( L_MESG, "waiting for ``%s''", expect_send[str] );

	    lprintf( L_NOISE, "got: " );

	    chat_has_timeout = FALSE;

	    /* set alarm timer. for the first string, the timer is only
	       set if the flag "timeout_first" is true */

	    if ( str != 0 || timeout_first )
		alarm( chat_timeout_time );
	    else
		alarm( 0 );

	    do
	    {
		cnt = read( STDIN, &buffer[i], 1 );

		if ( chat_has_timeout )		/* timeout */
		{
		    lprintf( L_ERROR,"timeout in chat skript, waiting for `%s'", expect_send[str] );
		    retcode = FAIL;
		    break;
		}
		if ( cnt > 0 )
		{
		    lputc( L_NOISE, buffer[i] );
		
		    /* this is a UUgetty - so look for lock files on the
		       first character we read - the data could be for
		       someone else. If there's no lockfile, the data is
		       probably a "RING", so we lock the line */

		    if ( locks )
		    {
			lprintf( L_NOISE, "checking lockfiles, locking the line" );
			if ( makelock(lock) == FAIL) {
			    lprintf( L_NOISE, "lock file exists!" );
			    /* close stdin -> other processes can read port */
			    close(0);
			    close(1);
			    close(2);
check_further:
			    while (checklock(lock) == TRUE)
			        sleep(10);	/*!!!!! ?? wait that long? */
			    sleep(5);
			    if ( checklock(lock) == TRUE )
				goto check_further;

			    exit(0);
			}
			locks = FALSE;

			/* now: set alarm timer (my ZyXEL sometimes sends
                           spurious "cr"s to the host, so we will not
			   leave the line locked forever waiting for the
                           next character (not) to come) */
		        alarm( chat_timeout_time );
		    }		/* end if (locks) */

		}		/* end if (character read) */
		i += cnt;
		if ( i>BUFFERSIZE-5 )
		{
		    memcpy( &buffer[0], &buffer[BUFFERSIZE/2], i-BUFFERSIZE/2+1 );
		    i-=BUFFERSIZE/2;
		}

		/* look for the "expect"-string */
		cnt = strlen( expect_send[str] );
		if ( i >= cnt &&
		     memcmp( &buffer[i-cnt], expect_send[str], cnt ) == 0 )
		{
		    lputs( L_MESG, " ** found **" );
		    break;
		}

		/* look for one of the "abort"-strings */
		for ( h=0; actions[h].expect != NULL; h ++ )
		{
		    cnt = strlen( actions[h].expect );
		    if ( i>=cnt && 
			 memcmp( &buffer[i-cnt], actions[h].expect, cnt ) == 0 )
		    {
			lprintf( L_MESG,"found action string: ``%s''",
			                actions[h].expect );
			*action = actions[h].action;
			retcode = FAIL;
			break;
		    }
		}
	    }
	    while ( i<BUFFERSIZE && retcode != FAIL );

	    /* disable timeout alarm clock */
	    alarm(0);

	    /* found abort string or timeout? */
	    if ( retcode == FAIL ) break;

	}		/* end if (strlen(expect_send[str] != 0) */

	str++;
	/* end of list? */

	if ( expect_send[str] == NULL ) break;

	/* send a string, translate esc-sequences */

	lprintf( L_MESG, "send: " );
	p = expect_send[str];
	nocr = FALSE;
	while ( *p != 0 ) 
	{
	    if ( *p == '\\' )
	    {
		switch ( *(++p) )
		{
		    case 'd': lputs( L_MESG, "\\d"); delay(500); break;
		    case 'c': nocr = TRUE; break;
		    default:
			fputc( *p, stdout );
			lputc( L_MESG, *p );
		}
	    }
	    else
	    {
		fputc( *p, stdout );
		lputc( L_MESG, *p );
	    }
	    p++;
	}
	if ( ! nocr ) fputc( '\n', stdout );

	str++;

    }			/* end while ( expect_send[str] != NULL ) */

    /* reset terminal settings */
    (void) ioctl(STDIN, TCSETAF, &save_termio);

    return retcode;
}

int clean_line( int waittime )
{
struct termio	termio, save_termio;
char	buffer[2];

    lprintf( L_NOISE, "waiting for line to clear, read: " );

    /* set terminal timeout to "waittime" tenth of a second */
    (void) ioctl(STDIN, TCGETA, &termio);
    save_termio = termio;
    termio.c_lflag &= ~ICANON;
    termio.c_cc[VMIN] = 0;
    termio.c_cc[VTIME] = waittime;
    (void) ioctl(STDIN, TCSETA, &termio);

    /* read everything that comes from modem until a timeout occurs */
    while ( read( STDIN, buffer, 1 ) > 0 ) 
        lputc( L_NOISE, buffer[0] );

    /* reset terminal settings */
    (void) ioctl(STDIN, TCSETAF, &save_termio);
    
    return 0;
}
