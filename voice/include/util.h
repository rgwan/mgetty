/*
 * util.h
 *
 * Some useful defines.
 *
 */

#ifdef MAIN
char *util_h = "$Id: util.h,v 1.1 1997/12/16 11:49:21 marc Exp $";
#endif

/*
 * Generic constants
 */

#undef TRUE
#undef FALSE

#define TRUE (0==0)
#define FALSE (0==1)

#undef OK
#undef FAIL

#define OK (0)
#define FAIL (-1)
#define UNKNOWN_EVENT (-2)

#define INTERRUPTED (0x4d00)

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

/*
 * Special modem control characters
 */

#undef ETX
#undef NL
#undef CR
#undef DLE
#undef XON
#undef XOFF
#undef DC4
#undef CAN

#define ETX  (0x03)
#define NL   (0x0a)
#define CR   (0x0d)
#define DLE  (0x10)
#define XON  (0x11)
#define XOFF (0x13)
#define DC4  (0x14)
#define CAN  (0x18)

/*
 * Check, that the system we are running on has proper bit sizes and
 * does proper handling of right bit shift operations
 */

extern void check_system (void);

/*
 * Useful path concatenation function
 */

extern void make_path (char *result, char *path, char *name);
