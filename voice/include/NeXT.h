/*
 * NeXT_args.h
 *
 * Ugly hack to get varargs to work on NeXTstep.
 *
 */

#ifdef MAIN
char *NeXT_args_h = "$Id: NeXT.h,v 1.1 1997/12/16 11:49:11 marc Exp $";
#endif

/*
 * On NeXTstep, we have to use this *UGLY* way to cheat varargs/stdarg
 */

#if defined(NeXT) && !defined(NEXTSGTTY)
# define va_alist a1,a2,a3,a4,a5,a6,a7,a8,a9
# define va_dcl long a1,a2,a3,a4,a5,a6,a7,a8,a9;
# define vsprintf(buf,fmt,v) sprintf((buf),(fmt),a1,a2,a3,a4,a5,a6,a7,a8,a9)
# define va_list int
# define va_start(v)
# define va_end(v)
#endif
