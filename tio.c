#ident "$Id: tio.c,v 1.1 1993/10/19 22:28:45 gert Exp $ Copyright (c) 1993 Gert Doering";

/* tio.c
 *
 * contains routines dealing with SysV termio / POSIX termios / BSD sgtty
 *
 */

#include "mgetty.h"
#include "tio.h"
#ifdef POSIX_TERMIOS
static char tio_compilation_type[]="@(#) compiled with POSIX_TERMIOS";
#elif defined SYSV_TERMIO
static char tio_compilation_type[]="@(#) compiled with SYSV_TERMIO";
#else
static char tio_compilation_type[]="@(#) compiled with BSD_SGTTY";
#endif

int tio_get _P2((fd, t), int fd, TIO *t )
{ 
#ifdef SYSV_TERMIO
    if ( ioctl( fd, TCGETA, t ) < 0 )
    {
	lprintf( L_ERROR, "TCGETA failed" ); return ERROR;
    }
#elif defined(POSIX_TERMIOS)
    if ( tcgetattr( fd, t ) < 0 )
    {
	lprintf( L_ERROR, "tcgetattr failed" ); return ERROR;
    }
#else
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
#elif defined(POSIX_TERMIOS)
    if ( tcsetattr( fd, TCSANOW, t ) < 0 )
    {
	lprintf( L_ERROR, "tcsetattr failed" ); return ERROR;
    }
#else
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
#elif defined( POSIX_TERMIOS )
    cfsetospeed( speed );
    cfsetispeed( speed );
#else
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
    t->c_cflag&= ~(CSIZE | CSTOPB | PARENB | PARODD | CLOCAL | LOBLK );
    t->c_cflag|= CS8 | CREAD | HUPCL | ( local? CLOCAL:0 );
    t->c_lflag = ECHOK | ECHOE | ECHO | ISIG | ICANON;

    t->c_line  = 0;
    t->c_cc[VEOF] = 0x04;
#if defined(POSIX_TERMIOS) || !defined(linux)
    t->c_cc[VEOL] = 0;
#endif

#if !defined(VSWTCH) && defined(VSWTC)
#define VSWTCH VSWTC
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
#error not implemented yet
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
#error not implemented yet
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
 * *only* CTSFLOW is set (Uwe S. Obst)
 */
#ifdef BROKEN_SCO_324
# undef HARDWARE_HANDSHAKE
# define HARDWARE_HANDSHAKE CTSFLOW
#endif

int tio_set_flow_control _P2( (t, type), TIO * t, int type )
{
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
#error not yet implemented
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

    tio_set_flow_control( &t, type );

    return tio_set( fd, &t );
}

int tio_toggle_dtr _P2( (fd, msec_wait), int fd, int msec_wait )
{
    TIO t, save_t;

    if ( tio_get( fd, &t ) == ERROR ) return ERROR;

    save_t = t;
    
#if defined( SYSV_TERMIO )
    t.c_cflag = ( t.c_cflag & ~CBAUD ) | B0;		/* speed = 0 */
#elif defined( POSIX_TERMIOS )
    cfsetospeed( t, B0 );
    cfsetispeed( t, B0 );
#else
    t.sg_ispeed = t.sg_ospeed = B0
#endif

    tio_set( fd, &t );
    delay( msec_wait );
    
    return tio_set( fd, &save_t );
}
