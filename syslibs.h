#ident "$Id: syslibs.h,v 1.2 1994/08/08 12:34:37 gert Exp $ Copyright (c) Gert Doering"

/* Include stdlib.h / malloc.h, depending on the O/S
 */

#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif

#if !defined( __bsdi__ ) && !defined(__FreeBSD__)
#include <malloc.h>
#endif
