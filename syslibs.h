#ident "$Id: syslibs.h,v 4.1 1997/01/12 14:53:46 gert Exp $ Copyright (c) Gert Doering"

/* Include stdlib.h / malloc.h, depending on the O/S
 */

#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif

#if !defined( __bsdi__ ) && !defined(__FreeBSD__) && !defined(NeXT)
#include <malloc.h>
#endif

#ifdef NEXTSGTTY		/* NeXT, not POSIX subsystem */
# include <libc.h>
#endif
