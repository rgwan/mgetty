#ident "$Id: syslibs.h,v 3.2 1996/03/04 23:08:48 gert Exp $ Copyright (c) Gert Doering"

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
