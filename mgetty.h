#ident "$Id: mgetty.h,v 1.65 1994/09/04 23:17:52 gert Exp $ Copyright (c) Gert Doering"

/* mgetty.h
 *
 * contains most of the constants and prototypes necessary for
 * mgetty+sendfax (except some fax constants, they are in fax_lib.h)
 */

#include "ugly.h"

/* some generic, useful defines */

#ifndef ERROR
#define	ERROR	-1
#define NOERROR	0
#endif

#ifndef TRUE
#define TRUE (1==1)
#define FALSE (1==0)
#endif

#define FAIL	-1
#define SUCCESS	0

/* defines for FIDO mailers */

#define TSYNC	0xae
#define YOOHOO	0xf1

/* stuff in logfile.c */

#define L_FATAL 0
#define L_ERROR 1
#define L_AUDIT 2
#define L_WARN 3
#define L_MESG 4
#define L_NOISE 5
#define L_JUNK 6

void log_init_paths _PROTO(( char * program, char * path, char * infix ));
void log_set_llevel _PROTO(( int level ));
int lputc _PROTO(( int level, char ch ));
int lputs _PROTO(( int level, char * s ));
int lprintf _PROTO(());

/* various defines */

/* bsd stuff */
#if defined(__BSD_NET2__) || defined(__386BSD__) || \
    defined(__NetBSD__)   || defined(__FreeBSD__)
# define BSD
#endif

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

typedef	void	RETSIGTYPE;
typedef	char	boolean;

/* the cpp directive "sun" isn't useful at all, since is defined on
 * SunOS 4, Solaris 2, and even Solaris x86...
 * So, you have to define -Dsunos4, -Dsolaris2, or -Dsolaris86.
 * Otherwise: barf!
 */
#ifdef sun
# if !defined( sunos4 ) && !defined( solaris2 ) && !defined( solaris86 )
#  error "Please define -Dsunos4 or -Dsolaris2 or -Dsolaris86"
# endif
#endif

#ifdef solaris2
# define SVR4
# define SVR42
# ifndef sun
#  define sun
# endif
#endif

/* assume that all BSD systems have the siginterrupt() function
 */
#if ( defined(BSD) || defined(sunos4) ) && !defined(NO_SIGINTERRUPT)
#define HAVE_SIGINTERRUPT
#endif

#define MAXLINE 1024		/* max. # chars in a line */
#define MAXPATH MAXLINE
#define STDIN	0

typedef enum {
	A_TIMOUT, A_FAIL, A_FAX, A_VCON, A_CONN,
	A_RING1, A_RING2, A_RING3, A_RING4, A_RING5
} action_t;

typedef struct	chat_actions {
			char * expect;
			action_t action; } chat_action_t ;

/* do_chat.c */
int	do_chat _PROTO(( int filedesc, char * expect_send[],
	     	 chat_action_t actions[], action_t * action,
		 int chat_timeout_time, boolean timeout_first ));
int	clean_line _PROTO(( int filedesc, int tenths ));

/* goodies.c */
char * get_basename _PROTO(( char * ));
char * mydup _PROTO(( char *s ));
char * get_ps_args _PROTO(( int pid ));

/* io.c */
boolean	check_for_input _PROTO (( int fd ));
boolean wait_for_input  _PROTO (( int fd, int seconds ));
void	delay _PROTO(( int waittime ));

/* locks.c */
#define	NO_LOCK	0	/* returned by checklock() if no lock found */
int		makelock _PROTO((char * device));
int		makelock_file _PROTO(( char * lockname ));
int		checklock _PROTO((char * device));
RETSIGTYPE	rmlocks _PROTO (());
  
/* fax stuff */
void	faxrec _PROTO(( char * spool_dir ));
char *	fax_strerror _PROTO(( int fax_hangup_code ));

/* initialization stuff: mg_m_init.c */
int	mg_init_data  _PROTO(( int fd ));
int	mg_init_fax   _PROTO(( int fd, char * mclass, char * fax_id ));
int 	mg_init_voice _PROTO(( int fd ));
void	faxpoll_server_init _PROTO(( int fd, char * fax_server_file ));
int	mg_open_device _PROTO(( char * devname, boolean blocking ));
int	mg_init_device _PROTO(( int fd, boolean toggle_dtr,
			        int toggle_dtr_waittime,
			        unsigned short portspeed ));
int	mg_get_ctty _PROTO(( int fd, char * devname ));
int	mg_drop_ctty _PROTO(( int fd ));

/* logname.c */
char *	ln_escape_prompt _PROTO(( char * prompt ));

/* login stuff */
void login _PROTO(( char * user ));

/* how long should I wait for a string from modem */
#define FAX_RESPONSE_TIMEOUT	120
/* how much time may pass while receiving a fax without getting data */
#define	FAX_PAGE_TIMEOUT	60

/* cnd.c */

extern char *Connect;
extern char *CallerId;
extern char *CallTime;
extern char *CallName;

void cndfind _PROTO((char *str));
int cndlookup _PROTO((void));

/* disk statistics retrieval in getdisk.c */

struct mountinfo {
    long	mi_bsize;	/* fundamental block size */
    long	mi_blocks;	/* number of blocks in file system */
    long	mi_bfree;	/* number of free blocks in file system */
    long	mi_bavail;	/* blocks available to non-super user */
    long	mi_files;	/* number of file nodes in file system */
    long	mi_ffree;	/* number of free nodes in fs */
};

typedef struct mountinfo	mntinf;

extern long minfreespace;

int checkspace _PROTO((char *path));
int getdiskstats _PROTO ((char *path, mntinf *mi));

/********* system prototypes **************/
extern char * mktemp _PROTO(( char * template ));

#if  !defined(linux) && !defined(SVR4) && !defined(__hpux) && \
     !defined(BSD) && !defined(M_UNIX) && !defined(_AIX)
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
