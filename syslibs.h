#ident "$Id: syslibs.h,v 3.1 1995/08/30 12:40:59 gert Exp $ Copyright (c) Gert Doering"

/* Include stdlib.h / malloc.h, depending on the O/S
 */

#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif

#if !defined( __bsdi__ ) && !defined(__FreeBSD__) && !defined(NeXT)
#include <malloc.h>
#endif
