#ident "$Id: tio.c,v 1.17 1994/03/13 01:17:01 gert Exp $ Copyright (c) 1993 Gert Doering"
;
/* tio.c
 *
 * contains routines dealing with SysV termio / POSIX termios / BSD sgtty
 *
 */

#include <sys/types.h>

#ifdef _AIX
#include <sys/ioctl.h>
#endif

#include "mgetty.h"
#include "tio.h"
#ifdef POSIX_TERMIOS
static char tio_compilation_type[]="@(#) compiled with POSIX_TERMIOS";
#endif
#ifdef SYSV_TERMIO
static char tio_compilation_type[]="@(#) compiled with SYSV_TERMIO";
#endif
#ifdef BSD_SGTTY
static char tio_compilation_type[]="@(#) compiled with BSD_SGTTY";
#endif

#ifdef SVR4
#include <sys/termiox.h>
#endif

#if defined( M_UNIX ) && defined( MAM_BUG )
#include <fcntl.h>
#endif

/* some systems do not define all flags needed later, e.g. NetBSD */

#ifdef BSD
# ifndef IUCLC
# define IUCLC 0
# endif
# ifndef TAB3
# define TAB3 OXTABS
# endif
#endif

/* baud rate table */

struct speedtab speedtab[] = {
	{ B50,	  50,	 "50"	 },
	{ B75,	  75,	 "75"	 },
	{ B110,	  110,	 "110"	 },
	{ B134,	  134,	 "134"	 },
	{ B150,	  150,	 "150"	 },
	{ B200,	  200,	 "200"	 },
	{ B300,	  300,	 "300"	 },
	{ B600,	  600,	 "600"	 },
	{ B1200,  1200,	 "1200"	 },
	{ B1800,  1800,	 "1800"	 },
	{ B2400,  2400,	 "2400"	 },
	{ B4800,  4800,	 "4800"	 },
	{ B9600,  9600,	 "9600"	 },
#ifdef	B19200
	{ B19200, 19200, "19200" },
#endif	/* B19200 */
#ifdef	B38400
	{ B38400, 38400, "38400" },
#endif	/* B38400 */
#ifdef	EXTA
	{ EXTA,	  19200, "EXTA"	 },
#endif
#ifdef	EXTB
	{ EXTB,	  38400, "EXTB"	 },
#endif
	{ 0,	  0,	 ""	 }
};


/* get current tio settings for given filedescriptor */

int tio_get _P2((fd, t), int fd, TIO *t )
{ 
#ifdef SYSV_TERMIO
    if ( ioctl( fd, TCGETA, t ) < 0 )
    {
	lprintf( L_ERROR, "TCGETA failed" ); return ERROR;
    }
#endif
#ifdef POSIX_TERMIOS
    if ( tcgetattr( fd, t ) < 0 )
    {
	lprintf( L_ERROR, "tcgetattr failed" ); return ERROR;
    }
#endif
#ifdef BSD_SGTTY
    if ( gtty( fd, t ) < 0 )
    {
	lprintf( L_ERROR, "gtty failed" ); return ERROR;
    }
#endif
    return NOERROR;
}

int tio_set _P2( (fd, t), int fd, TIO * t)	/*!! FIXME: flags, wait */
{
#ifdef sun
    int modem_lines;
#endif
#ifdef SYSV_TERMIO
    if ( ioctl( fd, TCSETA, t ) < 0 )
    {
	lprintf( L_ERROR, "ioctl TCSETA failed" ); return ERROR;
    }
#endif
#ifdef POSIX_TERMIOS
    if ( tcsetattr( fd, TCSANOW, t ) < 0 )
    {
	lprintf( L_ERROR, "tcsetattr failed" ); return ERROR;
    }
#ifdef sun
    /* On SunOS, make sure that RTS and DTR are asserted if you wanna
     * use hardware flow control
     */
    if (t->c_cflag & CRTSCTS)
    {
        /* make sure RTS is asserted!!!!!! */
        ioctl(STDIN, TIOCMGET, &modem_lines);
        modem_lines |= (TIOCM_RTS | TIOCM_DTR);
        ioctl(STDIN, TIOCMSET, &modem_lines);
    }
#endif /* sun */
#endif /* posix_termios */

#ifdef BSD_SGTTY
    if ( stty( fd, t ) < 0 )
    {
	lprintf( L_ERROR, "stty failed" ); return ERROR;
    }
#endif
    return NOERROR;
}

/* set speed, do not touch the other flags */
int tio_set_speed _P2( (t, speed ), TIO *t, int speed )
{
#ifdef SYSV_TERMIO
    t->c_cflag = ( t->c_cflag & ~CBAUD) | speed;
#endif
#ifdef POSIX_TERMIOS
    cfsetospeed( t, speed );
    cfsetispeed( t, speed );
#endif
#ifdef BSD_SGTTY
    t->sg_ispeed = t->sg_ospeed = B0;
#endif
    return NOERROR;
}

/* get port speed. Return integer value, not symbolic constant */
int tio_get_speed _P1( (t), TIO *t )
{
#ifdef SYSV_TERMIO
    ushort cbaud = t->c_cflag & CBAUD;
#endif
#ifdef POSIX_TERMIOS
    speed_t cbaud = cfgetospeed( t );
#endif
#ifdef BSD_SGTTY
    int cbaud = t->sg_ospeed;
#endif
    struct speedtab * p;

    for ( p=speedtab; p->nspeed != 0; p++ )
    {
	if ( p->cbaud == cbaud ) break;
    }
    return p->nspeed;
}
 

/* set "raw" mode: do not process input or output, do not buffer */
/* do not touch cflags or xon/xoff flow control flags */

void tio_mode_raw _P1( (t), TIO * t )
{
#if defined(SYSV_TERMIO) || defined( POSIX_TERMIOS)
    t->c_iflag &= ( IXON | IXOFF | IXANY );	/* clear all flags except */
						/* xon / xoff handshake */
    t->c_oflag  = 0;				/* no output processing */
    t->c_lflag  = 0;				/* no signals, no echo */

    t->c_cc[VMIN]  = 1;				/* disable line buffering */
    t->c_cc[VTIME] = 0;
#else
    t->sg_flags = RAW;
#endif
}

/* "cbreak" mode - do not process input in lines, but process ctrl
 * characters, echo characters back, ...
 * warning: must be based on raw / sane mode
 */
void tio_mode_cbreak _P1( (t), TIO * t )
{
#if defined(SYSV_TERMIO) || defined(POSIX_TERMIOS)
    t->c_oflag = 0;
    t->c_iflag &= ~( IGNCR | ICRNL | INLCR | IUCLC );
    t->c_lflag &= ~( ICANON );
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME]= 0;
#else
    t->sg_flags |= CBREAK;
#endif
}

/* set "sane" mode, usable for login, ...
 * unlike the other tio_mode_* functions, this function initializes
 * all flags, and should be called before calling any other function
 */
void tio_mode_sane _P2( (t, local), TIO * t, int local )
{
#if defined(SYSV_TERMIO) || defined( POSIX_TERMIOS )
    t->c_iflag = BRKINT | IGNPAR | IXON | IXANY;
    t->c_oflag = OPOST | TAB3;
    /* be careful, only touch "known" flags */
    t->c_cflag&= ~(CSIZE | CSTOPB | PARENB | PARODD | CLOCAL
#ifdef LOBLK
		   | LOBLK
#endif
		   );
    t->c_cflag|= CS8 | CREAD | HUPCL | ( local? CLOCAL:0 );
    t->c_lflag = ECHOK | ECHOE | ECHO | ISIG | ICANON;

#if !defined(POSIX_TERMIOS)
    t->c_line  = 0;
#endif

    /* initialize the most important c_cc's here */
    t->c_cc[VEOF] = 0x04;
#if defined(VEOL) && VEOL < TIONCC
    t->c_cc[VEOL] = 0;
#endif

#ifdef VSWTCH
    t->c_cc[VSWTCH] = 0;
#endif

#else
    t->sg_flags = ECHO | EVENP | ODDP;
    t->sg_erase = 0x7f;            /* erase character */
    t->sg_kill  = 0x25;            /* kill character, ^u */
#endif
}

/* tio_default_cc( TIO )
 *
 * initialize all c_cc fields (for POSIX and SYSV) to proper start
 * values (normally, the serial driver should do this, but there are
 * numerous systems where some of the more esoteric (VDSUSP...) flags
 * are plain wrong (e.g. set to "m" or so)
 *
 * do /not/ initialize VERASE and VINTR, since some systems use
 * ^H / DEL here, others DEL / ^C.
 */
void tio_default_cc _P1( (t), TIO *t )
{
#ifdef BSD_SGTTY
    t->sg_erase = 0x7f;            /* erase character */
    t->sg_kill  = 0x25;            /* kill character, ^u */

#else /* posix or sysv */
    t->c_cc[VQUIT]  = CQUIT;
    t->c_cc[VKILL]  = CKILL;
    t->c_cc[VEOF]   = CEOF;
#if defined(VEOL) && VEOL < TIONCC
    t->c_cc[VEOL] = CEOL;
#endif
#if defined(VSTART) && VSTART < TIONCC
    t->c_cc[VSTART] = CSTART;
#endif
#if defined(VSTOP) && VSTOP < TIONCC
    t->c_cc[VSTOP] = CSTOP;
#endif
#if defined(VSUSP) && VSUSP < TIONCC
    t->c_cc[VSUSP] = CSUSP;
#endif
#if defined(VSWTCH) && VSWTCH < TIONCC
    t->c_cc[VSWTCH] = CSWTCH;
#endif
    /* the following are for SVR4.2 (and higher) */
#if defined(VDSUSP) && VDSUSP < TIONCC
    t->c_cc[VDSUSP] = CDSUSP;
#endif
#if defined(VREPRINT) && VREPRINT < TIONCC
    t->c_cc[VREPRINT] = CRPRNT;
#endif
#if defined(VDISCARD) && VDISCARD < TIONCC
    t->c_cc[VDISCARD] = CFLUSH;
#endif
#if defined(VWERASE) && VWERASE < TIONCC
    t->c_cc[VWERASE] = CWERASE;
#endif
#if defined(VLNEXT) && VLNEXT < TIONCC
    t->c_cc[VLNEXT] = CLNEXT;
#endif

#endif /* bsd <-> posix + sysv */
}


void tio_map_cr _P2( (t, perform_mapping), TIO * t, int
		    perform_mapping )
{
#if defined(SYSV_TERMIO) || defined(POSIX_TERMIOS)
    if ( perform_mapping )
    {
	t->c_iflag |= ICRNL;
	t->c_oflag |= ONLCR;
    }
    else
    {
	t->c_iflag &= ~ICRNL;
	t->c_oflag &= ~ONLCR;
    }
#else
#include "not implemented yet"
#endif
}

/* tio_carrier()
 * specify whether the port should be carrier-sensitive or not
 */

void tio_carrier _P2( (t, carrier_sensitive), TIO *t, int carrier_sensitive )
{
#if defined(SYSV_TERMIO) || defined(POSIX_TERMIOS)
    if ( carrier_sensitive )
    {
	t->c_cflag &= ~CLOCAL;
    }
    else
    {
	t->c_cflag |= CLOCAL;
    }
#else
#include "not implemented yet"
#endif
}

/* set handshake */
 
/* hardware handshake flags - use what is available on local system
 */

#ifdef CRTSCTS
# define HARDWARE_HANDSHAKE CRTSCTS		/* linux, SunOS */
#else
# ifdef CRTSFL
#  define HARDWARE_HANDSHAKE CRTSFL		/* SCO 3.2v4.2 */
# else
#  ifdef RTSFLOW
#   define HARDWARE_HANDSHAKE RTSFLOW | CTSFLOW	/* SCO 3.2v2 */
#  else
#   ifdef CTSCD
#    define HARDWARE_HANDSHAKE CTSCD		/* AT&T 3b1? */
#   else
#    define HARDWARE_HANDSHAKE 0		/* nothing there... */
#   endif
#  endif
# endif
#endif

/* Dial-Out parallel to a Dial-in on SCO 3.2v4.0 does only work if
 * *only* CTSFLOW is set (Uwe S. Fuerst)
 */
#ifdef BROKEN_SCO_324
# undef HARDWARE_HANDSHAKE
# define HARDWARE_HANDSHAKE CTSFLOW
#endif

/* tio_set_flow_control
 *
 * set flow control according to the <type> parameter. It can be any
 * combination of
 *   FLOW_XON_IN - use Xon/Xoff on incoming data
 *   FLOW_XON_OUT- respect Xon/Xoff on outgoing data
 *   FLOW_HARD   - use RTS/respect CTS line for hardware handshake
 * (not every combination will work on every system)
 *
 * WARNING: for most systems, this function will not touch the tty
 *          settings, only modify the TIO structure. On some systems,
 *          you have to use extra ioctl()s [notably SVR4 and AIX] to
 *          modify hardware flow control settings. On those systems,
 *          these calls are done immediately.
 */

int tio_set_flow_control _P3( (fd, t, type), int fd, TIO * t, int type )
{
#ifdef SVR4
    struct termiox tix;
#endif
    
#if defined( SYSV_TERMIO ) || defined( POSIX_TERMIOS )
    t->c_cflag &= ~HARDWARE_HANDSHAKE;
    t->c_iflag &= ~( IXON | IXOFF | IXANY );

    if ( type & FLOW_HARD )
			t->c_cflag |= HARDWARE_HANDSHAKE;
    if ( type & FLOW_XON_IN )
			t->c_iflag |= IXOFF;
    if ( type & FLOW_XON_OUT )
			t->c_iflag |= IXON | IXANY;
#else
#include "not yet implemented"
#endif
    /* SVR4 came up with a new method of setting h/w flow control */
    /* unfortunately, it's broken in 4.2! */
#if defined(SVR4) && !defined(SVR42)
    if (ioctl(fd, TCGETX, &tix) < 0)
    {
	lprintf( L_ERROR, "ioctl TCGETX" ); return ERROR;
    }
    if ( type & FLOW_HARD )
        tix.x_hflag |= (RTSXOFF | CTSXON);
    else
        tix.x_hflag &= ~(RTSXOFF | CTSXON);
    
    if ( ioctl(fd, TCSETX, &tix) < 0 )
    {
	lprintf( L_ERROR, "ioctl TCSETX" ); return ERROR;
    }
#endif
    /* AIX has yet another method to set hardware flow control (yuck!) */
#ifdef _AIX
    if ( ioctl( fd, ( type & FLOW_HARD ) ? TXADDCD : TXDELCD, "rts" ) < 0 )
    {
	lprintf( L_ERROR, "ioctl TXADDCD/TXDELCD" ); return ERROR;
    }
#ifdef DEBUG
    {	union txname t; int i;
	lprintf( L_NOISE, "control disciplines:");
	for ( i=1; ; i++ ) {
	    t.tx_which = i;
	    if ( ioctl( fd, TXGETCD, &t ) ) {
		lprintf( L_FATAL, "TXGETCD error" ); break;
	    }
	    if ( t.tx_name == NULL || !t.tx_name[0] ) break;
	    lputc( L_NOISE, ' ');
	    lputs( L_NOISE, t.tx_name );
	}
    }
#endif			/* DEBUG */
#endif			/* _AIX */
    return NOERROR;
}

/* for convenience - do not have to get termio settings before, but
 * it's slower (two system calls more)
 */
int tio_set_flow_control2 _P2( (fd, type), int fd, int type )
{
TIO t;

    if ( tio_get( fd, &t ) == ERROR ) return ERROR;

    tio_set_flow_control( fd, &t, type );

    return tio_set( fd, &t );
}

int tio_toggle_dtr _P2( (fd, msec_wait), int fd, int msec_wait )
{
    /* On SVR4.2, lowering DTR by setting the port speed to zero will
     * bring the port to some strange state where every ioctl() later
     * on simply fails - so use special "modem control" ioctl()s to
     * lower and raise DTR
     */
#if defined(SVR4) && defined(TIOCMBIS)		/* SVR4 special */
    int mdm_ctrl = TIOCM_DTR;

    if ( ioctl( fd, TIOCMBIC, (char *) mctl ) < 0 )
    {
	lprintf( L_ERROR, "TIOCMBIC failed" ); return ERROR;
    }
    delay( msec_wait );
    if ( ioctl( fd, TIOCMBIS, (char *) mctl ) < 0 )
    {
	lprintf( L_ERROR, "TIOCMBIS failed" ); return ERROR;
    }
#else						/* !SVR4 */

    /* The "standard" way of doing things - via speed = B0
     */
    TIO t, save_t;
#ifdef sun
    int modem_lines;
#endif
    int result;

    if ( tio_get( fd, &t ) == ERROR ) return ERROR;

    save_t = t;
    
#ifdef SYSV_TERMIO 
    t.c_cflag = ( t.c_cflag & ~CBAUD ) | B0;		/* speed = 0 */
#endif
#ifdef POSIX_TERMIOS
    cfsetospeed( &t, B0 );
    cfsetispeed( &t, B0 );
#endif
#ifdef BSD_SGTTY
    t.sg_ispeed = t.sg_ospeed = B0
#endif

    tio_set( fd, &t );
    delay( msec_wait );
    
#ifdef sun
    /* on SunOS, if you hangup via B0, the DTR line will *stay* low.
     * So: enable it manually again.
     */
    ioctl(STDIN, TIOCMGET, &modem_lines);
    modem_lines |= (TIOCM_RTS | TIOCM_DTR);
    ioctl(STDIN, TIOCMSET, &modem_lines);
#endif
    result = tio_set( fd, &save_t );
    
#if defined(M_UNIX) && defined(MAM_BUG)
    /* some SCO Unix variants apparently forget to raise DTR again
     * after lowering it. Reopening the port fixes it. Crude, but works.
     */
    close( open( "/dev/tty", O_RDONLY | O_NDELAY ) );
#endif
    
    return result;
#endif					/* !SVR4 */
}
