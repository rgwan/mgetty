#ident "$Id: syslibs.h,v 1.3 1994/09/28 17:35:51 gert Exp $ Copyright (c) Gert Doering"

/* Include stdlib.h / malloc.h, depending on the O/S
 */

#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif

#if !defined( __bsdi__ ) && !defined(__FreeBSD__) && !defined(NeXT)
#include <malloc.h>
#endif
