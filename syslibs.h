#ident "$Id: syslibs.h,v 2.1 1994/11/30 23:20:49 gert Exp $ Copyright (c) Gert Doering"

/* Include stdlib.h / malloc.h, depending on the O/S
 */

#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif

#if !defined( __bsdi__ ) && !defined(__FreeBSD__) && !defined(NeXT)
#include <malloc.h>
#endif
