#ident "$Id: tio.c,v 1.6 1993/11/12 15:21:15 gert Exp $ Copyright (c) 1993 Gert Doering";

/* tio.c
 *
 * contains routines dealing with SysV termio / POSIX termios / BSD sgtty
 *
 */

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

/* some systems do not define all flags needed later, e.g. NetBSD */

#ifdef BSD
# ifndef IUCLC
# define IUCLC 0
# endif
# ifndef TAB3
# define TAB3 OXTABS
# endif
#endif

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
#endif
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
#ifdef SVR4
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
    TIO t, save_t;

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
    
    return tio_set( fd, &save_t );
}
