#ident "$Id: gettydefs.c,v 1.1 1993/10/06 00:19:42 gert Exp $ Copyright (c) 1993 Gert Doering/Chris Lewis"

/* gettydefs.c
 *
 * Read /etc/gettydefs file, and permit retrieval of individual entries.
 *
 * Code in this module by Chris Lewis
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef _NOSTDLIB_H
# include <stdlib.h>
#endif
#include <termio.h>

#include "mgetty.h"

#ifdef USE_GETTYDEFS

#ifdef _3B1_
typedef unsigned short tcflag_t;
#endif

struct modeword {
    char *name;
    tcflag_t turnon;
    tcflag_t turnoff;
    unsigned short meta;
};

#define SANE	0x01

/* input modes */

static struct modeword iflags[] = {
    { "IGNBRK", IGNBRK, IGNBRK },
    { "BRKINT", BRKINT, BRKINT, SANE },
    { "IGNPAR", IGNPAR, IGNPAR, SANE },
    { "PARMRK", PARMRK, PARMRK },
    { "INPCK", INPCK, INPCK },
    { "ISTRIP", ISTRIP, ISTRIP, SANE },
    { "INLCR", INLCR, INLCR },
    { "IGNCR", IGNCR, IGNCR },
    { "ICRNL", ICRNL, ICRNL, SANE },
    { "IUCLC", IUCLC, IUCLC },
    { "IXON", IXON, IXON, SANE },
    { "IXANY", IXANY, IXANY },
    { "IXOFF", IXOFF, IXOFF },
    { NULL }
};

/* output modes */

static struct modeword oflags[] = {
    { "OPOST", OPOST, OPOST, SANE },
    { "OLCUC", OLCUC, OLCUC },
    { "ONLCR", ONLCR, ONLCR },
    { "OCRNL", OCRNL, OCRNL },
    { "ONOCR", ONOCR, ONOCR },
    { "ONLRET", ONLRET, ONLRET },
    { "OFILL", OFILL, OFILL },
    { "OFDEL", OFDEL, OFDEL },
    { "NLDLY", NLDLY, NLDLY },
    { "NL0", NL0, NLDLY },
    { "NL1", NL1, NLDLY },
    { "CR0", CR0, CRDLY },
    { "CR1", CR1, CRDLY },
    { "CR2", CR2, CRDLY },
    { "CR3", CR3, CRDLY },
    { "TAB0", TAB0, TABDLY },
    { "TAB1", TAB1, TABDLY },
    { "TAB2", TAB2, TABDLY },
    { "TAB3", TAB3, TABDLY },
    { "BS0", BS0, BSDLY },
    { "BS1", BS1, BSDLY },
    { "VT0", VT0, VTDLY },
    { "VT1", VT1, VTDLY },
    { "FF0", FF0, FFDLY },
    { "FF1", FF1, FFDLY },
    { NULL }
};

/* control modes */

static struct modeword cflags[] = {
    { "B0", B0, CBAUD },
    { "B50", B50, CBAUD },
    { "B75", B75, CBAUD },
    { "B110", B110, CBAUD },
    { "B134", B134, CBAUD },
    { "B150", B150, CBAUD },
    { "B200", B200, CBAUD },
    { "B300", B300, CBAUD },
    { "B600", B600, CBAUD },
#ifdef B900
    { "B900", B900, CBAUD },
#endif
    { "B1200", B1200, CBAUD },
    { "B1800", B1800, CBAUD },
    { "B2400", B2400, CBAUD },
#ifdef B3600
    { "B3600", B3600, CBAUD },
#endif
    { "B4800", B4800, CBAUD },
#ifdef B7200
    { "B7200", B7200, CBAUD },
#endif
    { "B9600", B9600, CBAUD },
#ifdef B19200
    { "B19200", B19200, CBAUD },
#endif
#ifdef B38400
    { "B38400", B38400, CBAUD },
#endif
#ifdef B57600
    { "B57600", B57600, CBAUD },
#endif
#ifdef B115200
    { "B115200", B115200, CBAUD },
#endif
#ifdef B230400
    { "B230400", B230400, CBAUD },
#endif
#ifdef B230400
    { "B230400", B230400, CBAUD },
#endif
#ifdef B460800
    { "B460800", B460800, CBAUD },
#endif
    { "EXTA", EXTA, CBAUD },
    { "EXTB", EXTB, CBAUD },
    { "CS5", CS5, CS5 },
    { "CS6", CS6, CS6 },
    { "CS7", CS7, CS7, SANE },
    { "CS8", CS8, CS8 },
    { "CSTOPB", CSTOPB, CSTOPB },
    { "CREAD", CREAD, CREAD, SANE },
    { "PARENB", PARENB, PARENB, SANE },
    { "PARODD", PARODD, PARODD },
    { "HUPCL", HUPCL, HUPCL },
    { "CLOCAL", CLOCAL, CLOCAL },
/* Various handshaking defines */
#ifdef CTSCD
    { "CTSCD", CTSCD, CTSCD },
#endif
#ifdef CRTSCTS
    { "CRTSCTS", CRTSCTS, CRTSCTS },
#endif
#ifdef CRTSFL
    { "CRTSFL", CRTSFL, CRTSFL },
#endif
#ifdef RTSFLOW
    { "RTSFLOW", RTSFLOW, RTSFLOW },
    { "CTSFLOW", CTSFLOW, CTSFLOW },
#endif
#ifdef HDX
    { "HDX", HDX, HDX },
#endif
    { NULL }
};

/* line discipline */
static struct modeword lflags[] =  {
    { "ISIG", ISIG, ISIG, SANE },
    { "ICANON", ICANON, ICANON, SANE },
    { "XCASE", XCASE, XCASE },
    { "ECHO", ECHO, ECHO, SANE },
    { "ECHOE", ECHOE, ECHOE },
    { "ECHOK", ECHOK, ECHOK, SANE },
    { "ECHONL", ECHONL, ECHONL },
    { "NOFLSH", NOFLSH, NOFLSH },
    { NULL }
};

static struct gdentry *gdentries[GDENTRYMAX];
static struct gdentry **cur = gdentries;

static struct modeword *
findmode _P2 ((modes, tok), struct modeword *modes, register char *tok)
{
    for( ; modes->name; modes++)
	if (strcmp(modes->name, tok) == 0)
	    return(modes);
    return(0);
}

static void
metaset _P3((tc, modes, key), tcflag_t *tc, struct modeword *modes, int key)
{
    for ( ; modes->name; modes++)
	if (modes->meta&key)
	    *tc = (*tc &= ~ modes->turnoff) | modes->turnon;
}

static void
parsetermio _P2((ti, str), struct tio *ti, char *str)
{
    register char *p;
    struct modeword *m;
    tcflag_t *flag;
    int metakey;

    while (p = strtok(str, " \t")) {

	str = NULL;

	metakey = 0;

	if (strcmp(p, "SANE") == 0)
	    metakey = SANE;
	
	if (metakey) {
	    tcflag_t on, off;
	    metaset(&ti->c_lflag, lflags, metakey);
	    metaset(&ti->c_oflag, oflags, metakey);
	    metaset(&ti->c_iflag, iflags, metakey);
	    metaset(&ti->c_cflag, cflags, metakey);
	    continue;
	}

	if      ((m = findmode(lflags, p)) != NULL)
	    flag = &ti->c_lflag;
	else if ((m = findmode(oflags, p)) != NULL)
	    flag = &ti->c_oflag;
	else if ((m = findmode(iflags, p)) != NULL)
	    flag = &ti->c_iflag;
	else if ((m = findmode(cflags, p)) != NULL)
	    flag = &ti->c_cflag;
	if (m)
	    *flag = (*flag & ~ m->turnoff) | m->turnon;
	else
	    lprintf(L_WARN, "Can't parse %s", p);
    }

}

static char *
mydup _P1 ((s), register char *s)
{
    register char *p = (char *) malloc(strlen(s) + 1);
    if (!p) {
	lprintf(L_ERROR, "mydup can't malloc");
	exit(1);
    }
    strcpy(p, s);
    return(p);
}

static char *
stripblanks _P1 ((s), register char *s)
{
    register char *p;
    while(*s && isspace(*s)) s++;
    p = s;
    while(*p && !isspace(*p)) p++;
    *p = '\0';
    return(s);
}

#define	GETTYBUFSIZE	(10*BUFSIZ)

int
getentry _P3((entry, elen, f), char *entry, int elen, FILE *f) {
    char buf[BUFSIZ*2];
    register char *p;

    entry[0] = '\0';

    do {
	if (!fgets(buf, sizeof(buf), f))
	    return(0);
	for (p = buf; isspace(*p); p++);
    } while(*p == '#' || *p == '\n');

    p = strchr(buf, '\n');
    if (p)
	*p = '\0';
    strcat(entry, buf);

    while (1) {
	if (!fgets(buf, sizeof(buf), f))
	    break;
	p = strchr(buf, '\n');
	if (p)
	    *p = '\0';
	for (p = buf; isspace(*p); p++);
	if (!*p)
	    break;
	strcat(entry, " ");
	strcat(entry, p);
    }
    fprintf(stderr, "entry is: %s\n", entry);
    return(1);
}

/*
 * loads all of the entries from the gettydefs file
 * returns 0 if it fails.
 */
int
loadgettydefs _P0(void) {
    FILE *gd = fopen(GETTYDEFS, "r");
    char buf[GETTYBUFSIZE];
    char tbuf[5];
    int oct;
    register char *p, *p2;
    char *tag, *prompt, *nexttag, *before, *after;

    if (!gd) {
	lprintf(L_WARN, "Can't open %s\n", GETTYDEFS);
	return(0);
    }

    while(getentry(buf, sizeof(buf), gd)) {
	
	p = buf;

	tag = strtok(p, "#");
	if (!tag)
	    continue;
	tag = stripblanks(tag);
	tag = mydup(tag);

	before = strtok(NULL, "#");
	if (!before)
	    continue;

	after = strtok(NULL, "#");
	if (!after)
	    continue;

	prompt = strtok(NULL, "#");
	if (!prompt)
	    continue;

	for (p = p2 = prompt; *p; p++,p2++)
	    if (*p == '\\') {
		switch (*(p+1)) {
		    case 'n': *p2 = '\n'; break;
		    case 'r': *p2 = '\r'; break;
		    case 'g': *p2 = '\007'; break;
		    case 'f': *p2 = '\f'; break;
		    case '0': case '1': case '2': case '3':
		    case '4': case '5': case '6': case '7':
			strncpy(tbuf, p+1, 3);
			tbuf[3] = '\0';
			p += 2;
			*p2 = strtol(tbuf, (char *) NULL, 0);
			break;
		    default: *p2 = *(p+1); break;
		}
		p++;
	    } else
		*p2 = *p;
	*p2 = '\0';

	prompt = mydup(prompt);

	nexttag = strtok(NULL, "#");
	if (!nexttag)
	    continue;

	p = strchr(nexttag, '\n');
	if (p)
	    *p = '\0';

	nexttag = stripblanks(nexttag);
	nexttag = mydup(nexttag);

#ifdef 0
	printf("tag: %s\nbefore: %s\nafter: %s\nprompt: %s\nnexttag: %s\n\n",
	    tag, before, after, prompt, nexttag);
#endif

	if (cur - gdentries > GDENTRYMAX-1) {
	    lprintf(L_WARN,
		"Too many gettydefs entries, skipping extra - increase GDENTRYMAX");
	    return(1);
	}
	
	*cur = (struct gdentry *) malloc(sizeof(struct gdentry));
	if (!*cur) {
	    lprintf(L_ERROR, "Can't malloc a gdentry");
	    exit(1);
	}

	(*cur)->tag = tag;
	(*cur)->prompt = prompt;
	(*cur)->nexttag = nexttag;
	parsetermio(&(*cur)->before, before);
	parsetermio(&(*cur)->after, after);
	lprintf(L_NOISE, "Processed `%s' gettydefs entry", tag);
	cur++;
    }
    fclose(gd);
    return(1);
}

struct gdentry *
getgettydef _P1 ((s), register char *s)
{
    for (cur = gdentries; *cur; cur++)
	if (strcmp((*cur)->tag, s) == 0)
	    return(*cur);
    if (gdentries[0]->tag) {
	lprintf(L_WARN, "getgettydef(%s) entry not found using %s",
	    s, gdentries[0]->tag);
	return(gdentries[0]);
    }
    lprintf(L_WARN, "getgettydef(%s) no entry found", s);
    return((struct gdentry *) NULL);
}

#ifdef DEBUGGETTY

void
dumpflag _P3((type, modes, flag),
	     char *type,
	     struct modeword *modes, tcflag_t flag)
{
    printf("%s:", type);
    for(; modes->name; modes++)
	if ((flag&modes->turnoff) == modes->turnon)
	    printf(" %s", modes->name);
    putchar('\n');
}

void
dump _P2((ti, s), struct tio *ti, char *s)
{
    printf("%s:", s);
    dumpflag("iflags", iflags, ti->c_iflag);
    dumpflag("oflags", oflags, ti->c_oflag);
    dumpflag("cflags", cflags, ti->c_cflag);
    dumpflag("lflags", lflags, ti->c_lflag);
    putchar('\n');
}

#include <varargs.h>
int lprintf( level, format, va_alist )
int level;
const char * format;
va_dcl
{
char    ws[200];
va_list pvar;
    va_start( pvar );
    vsprintf( ws, format, pvar );
    va_end( pvar );
    fprintf(stderr, "%d: %s\n", level, ws);
}

void 
spew _P1 ((gd), struct gdentry *gd)
{
    printf("tag: `%s'\nprompt: `%s'\nnexttag: `%s'\n",
	gd->tag, gd->prompt, gd->nexttag);
    dump(&gd->before, "before");
    dump(&gd->after, "after");
    printf("\n");
}

int
main _P2((argc, argv), int argc, char **argv) {
    struct gdentry *p;
    if (! loadgettydefs()) {
	fprintf(stderr, "Couldn't read %s\n", GETTYDEFS);
	exit(1);
    }
    printf("loaded entries:\n");
    for (cur = gdentries; *cur; cur++)
	spew(*cur);

}
#endif
#endif
