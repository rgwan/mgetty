/* $Id: mgetty.h,v 1.26 1993/09/13 21:03:07 gert Exp $ (c) Gert Doering */

/* stuff in logfile.c */

#define L_FATAL 0
#define L_ERROR 1
#define L_WARN 2
#define L_MESG 3
#define L_NOISE 4
#define L_JUNK 5

extern	int	log_level;
extern	char	log_path[];
int lputc( int level, char ch );
int lputs( int level, char * s );
int lprintf();

/* various defines */

/* define here what function to use for polling for characters
 * Chose one of the following: USE_SELECT, USE_POLL
 * I recommend USE_SELECT on all machines that have it, except SCO Unix,
 * since the tv_usec timer is not exact at all on SCO.
 * If your System has the "nap(S)" call, you can use this instead of
 * select(S) or poll(S) for sleeping less than one second.
 */

#ifndef USE_POLL
#define USE_SELECT
#endif

#define TRUE (1==1)
#define FALSE (1==0)
#define FAIL	-1
#define SUCCESS	0

typedef	void	sig_t;
typedef	char	boolean;

#define MAXLINE 256		/* max. # chars in a line */
#define MAXPATH MAXLINE
#ifdef OLD_STUFF
#define STDIN	fileno(stdin)
#else
#define STDIN	0
#endif

typedef	enum	{ A_FAIL, A_FAX } action_t;
typedef struct	chat_actions {
			char * expect;
			action_t action; } chat_action_t ;

/* do_chat.c */
int	do_chat( char * expect_send[],
	     	 chat_action_t actions[], action_t * action,
		 int chat_timeout_time, boolean timeout_first,
		 boolean locks, boolean virtual_rings );
int	clean_line( int tenths );

/* io.c */
boolean	check_for_input( int fd );
void	delay( int waittime );

/* locks.c */
int	makelock(char * device);
boolean	checklock(char * device);
sig_t	rmlocks();
  
/* fax stuff */
void faxrec( char * spool_dir );

/* utmp.c */
void make_utmp_wtmp( char * line, boolean login_process );
int get_current_users( void );

/* how long should I wait for a string from modem */
#define FAX_RESPONSE_TIMEOUT	120
/* how much time may pass while receiving a fax without getting data */
#define	FAX_PAGE_TIMEOUT	60

/********* system prototypes **************/
char * mktemp( char * template );

#if  !defined(linux) && !defined(SVR4) && !defined(__hpux)
extern int	getopt( int, char **, char * );
#endif
extern int	optind;
extern char *	optarg;

/* system specific stuff */
#ifdef ISC
#define fileno(p)	(p)->_file
# ifndef O_NDELAY
#  define O_NDELAY O_NONBLOCK
# endif
#endif

/* hardware handshake flags - use what is available */
/* warning: this will work only, if mgetty.h is included *after* termio.h
 * (so, if termio.h was not included, CS8 is not defined, and
 * HARDWARE_HANSHAKE will be undefined, giving an error)
 */

#ifdef CS8

#ifdef CRTSCTS
# define HARDWARE_HANDSHAKE CRTSCTS
#else
# ifdef CRTSFL
#  define HARDWARE_HANDSHAKE CRTSFL
# else
#  ifdef RTSFLOW
#   define HARDWARE_HANDSHAKE RTSFLOW | CTSFLOW
#  else
#   ifdef CTSCD
#    define HARDWARE_HANDSHAKE CTSCD
#   else
#    define HARDWARE_HANDSHAKE 0
#   endif
#  endif
# endif
#endif

/* I've got reports telling me that dial-in / dial-out on SCO 3.2.4
 * does only work reliably if *only* CTSFLOW is set
 */
#ifdef BROKEN_SCO_324
#undef HARDWARE_HANDSHAKE
#define HARDWARE_HANDSHAKE CTSFLOW
#endif

#endif			/* cs8 */

#include "policy.h"
