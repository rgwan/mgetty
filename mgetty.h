/* $Id: mgetty.h,v 1.31 1993/10/19 22:25:06 gert Exp $ Copyright (c) Gert Doering */

/* ANSI vs. non-ANSI support */
#ifdef __STDC__
#define	_PROTO(x)	x
#define _P0(x)		(x)
#define _P1(x,a1)	(a1)
#define _P2(x,a1,a2)	(a1,a2)
#define _P3(x,a1,a2,a3)	(a1,a2,a3)
#define _P4(x,a1,a2,a3,a4)	(a1,a2,a3,a4)
#define _P7(x,a1,a2,a3,a4,a5,a6,a7)	(a1,a2,a3,a4,a5,a6,a7)
#define _P8(x,a1,a2,a3,a4,a5,a6,a7,a8)	(a1,a2,a3,a4,a5,a6,a7,a8)
#else
#define _PROTO(x)	()
#define _P0(x)		()
#define _P1(x,a1)	x a1;
#define _P2(x,a1,a2)	x a1;a2;
#define _P3(x,a1,a2,a3)	x a1;a2;a3;
#define _P4(x,a1,a2,a3,a4)	x a1;a2;a3;a4;
#define _P7(x,a1,a2,a3,a4,a5,a6,a7)	x a1;a2;a3;a4;a5;a6;a7;
#define _P8(x,a1,a2,a3,a4,a5,a6,a7,a8)	x a1;a2;a3;a4;a5;a6;a7;a8;
#define const
#endif

/* some generic, useful defines */

#ifndef ERROR
#define	ERROR	-1
#define NOERROR	0
#endif

#define TRUE (1==1)
#define FALSE (1==0)
#define FAIL	-1
#define SUCCESS	0

/* stuff in logfile.c */

#define L_FATAL 0
#define L_ERROR 1
#define L_WARN 2
#define L_MESG 3
#define L_NOISE 4
#define L_JUNK 5

extern	int	log_level;
extern	char	log_path[];
int lputc _PROTO(( int level, char ch ));
int lputs _PROTO(( int level, char * s ));
int lprintf _PROTO(());

/* various defines */

/* define here what function to use for polling for characters
 * Chose one of the following: USE_SELECT, USE_POLL, USE_READ
 * I recommend USE_SELECT on all machines that have it, except SCO Unix,
 * since the tv_usec timer is not exact at all on SCO.
 * If your System has the "nap(S)" call, you can use this instead of
 * select(S) or poll(S) for sleeping less than one second.
 */

#if !defined(USE_POLL) && !defined(USE_READ)
#define USE_SELECT
#endif

typedef	void	sig_t;
typedef	char	boolean;

#define MAXLINE 256		/* max. # chars in a line */
#define MAXPATH MAXLINE
#define STDIN	0

typedef	enum	{ A_FAIL, A_FAX, A_VCON, A_CONN } action_t;
typedef struct	chat_actions {
			char * expect;
			action_t action; } chat_action_t ;

/* do_chat.c */
int	do_chat _PROTO(( int filedesc, char * expect_send[],
	     	 chat_action_t actions[], action_t * action,
		 int chat_timeout_time, boolean timeout_first,
		 boolean locks, boolean virtual_rings ));
int	clean_line _PROTO(( int filedesc, int tenths ));

/* io.c */
boolean	check_for_input _PROTO (( int fd ));
void    wait_for_input  _PROTO (( int fd ));
void	delay _PROTO(( int waittime ));

/* locks.c */
int	makelock _PROTO((char * device));
boolean	checklock _PROTO((char * device));
sig_t	rmlocks _PROTO (());
  
/* fax stuff */
void	faxrec _PROTO(( char * spool_dir ));

/* utmp.c */
void	make_utmp_wtmp _PROTO(( char * line, boolean login_process ));
int	get_current_users _PROTO(( void ));

/* how long should I wait for a string from modem */
#define FAX_RESPONSE_TIMEOUT	120
/* how much time may pass while receiving a fax without getting data */
#define	FAX_PAGE_TIMEOUT	60

/* gettydefs.c */

#ifdef USE_GETTYDEFS
struct tio {
    unsigned short c_iflag;
    unsigned short c_oflag;
    unsigned short c_cflag;
    unsigned short c_lflag;
};

struct gdentry {
    char *tag;
    struct tio before;
    struct tio after;
    char *prompt;
    char *nexttag;
};

/* number of gettydefs entries to make */
#define GDENTRYMAX 200

int		loadgettydefs _PROTO(void);
struct gdentry	*getgettydef _PROTO((char *s));
#endif

/********* system prototypes **************/
extern char * mktemp _PROTO(( char * template ));

#if  !defined(linux) && !defined(SVR4) && !defined(__hpux)
extern int	getopt _PROTO(( int, char **, char * ));
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
