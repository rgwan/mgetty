/* $Id: mgetty.h,v 1.13 1993/03/23 12:33:36 gert Exp $ (c) Gert Doering */

/* stuff in logfile.c */

#define L_FATAL 0
#define L_ERROR 1
#define L_WARN 2
#define L_MESG 3
#define L_NOISE 4

extern	int	log_level;
extern	char	log_path[];
int lputc( int level, char ch );
int lputs( int level, char * s );
int lprintf();

/* various defines */

/* define here, what function to use for sleeping less than one second.
 * Chose one of the following: USE_SELECT, USE_POLL, USE_NAP
 * I recommend USE_POLL on SCO unix, and USE_SELECT on linux
 * (select() seems not be able to sleep *exactly* 500 msec on SCO!??)
 */

#ifndef linux
#define USE_POLL
#endif

#ifndef USE_SELECT
#define USE_POLL
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
void delay( int waittime );
int do_chat( char * expect_send[],
	     chat_action_t actions[], action_t * action,
             int chat_timeout_time, boolean timeout_first,
             boolean locks );
int clean_line( int tenths );

/* locks.c */

int makelock(char * name);
boolean	checklock(char * name);
int readlock(char * name);
sig_t	rmlocks();

extern	char	*lock;

/* fax stuff */
void faxrec( void );

/* how long should I wait for a string from modem */
#define FAX_RESPONSE_TIMEOUT	120
/* how much time may pass while receiving a fax without getting data */
#define	FAX_PAGE_TIMEOUT	60

/********* system prototypes **************/
char * mktemp( char * template );

#ifndef linux
extern int	getopt( int, char **, char * );
#endif
extern int	optind;
extern char *	optarg;

/* system specific stuff */
#ifdef ISC
#define fileno(p)	(p)->_file
# ifndef O_NDELAY
# define O_NDELAY O_NONBLOCK
# endif
#endif

/* hardware handshake flags - use what is available */
/* warning: this will work only, if mgetty.h is included *after* termio.h
 * (so, if termio.h was not included, CS8 is not defined, and
 * HARDWARE_HANSHAKE will be undefined, giving an error)
 */

#ifdef CS8

#ifdef CRTSCTS
#define HARDWARE_HANDSHAKE CRTSCTS
#else
#ifdef RTSFLOW
#define HARDWARE_HANDSHAKE RTSFLOW | CTSFLOW
#else
#define HARDWARE_HANDSHAKE 0
#endif
#endif

#endif

#include "policy.h"
