#ident "@(#)cnd.c	$Id: cnd.c,v 1.3 1994/02/16 14:03:40 gert Exp $ Copyright (c) 1993 Gert Doering/Chris Lewis"
;
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#ifndef _NOSTDLIB_H
# include <stdlib.h>
#endif
#include <malloc.h>

#include "policy.h"
#include "mgetty.h"

char *Connect = "";
char *CallerId = "none";
char *CallTime = "";
char *CallName = "";

struct cndtable {
    char *string;
    char **variable;
};

struct cndtable cndtable[] =
{
    {"RING"},			/* speedups */
    {"OK"},			/* speedups */
    {"CALLER NAME: ",		&CallName},
    {"CALLER NUMBER: ",		&CallerId},
    {"CONNECT ",		&Connect},
    {"TIME: ",			&CallTime},
    {"REASON FOR NO CALLER NUMBER: ",	&CallerId},
    {"REASON FOR NO CALLER NAME: ",	&CallName},
    {NULL}
};
    

void
cndfind _P1((str), char *str)
{
    struct cndtable *cp;
    register int len;
    register char *p;

    while (*str && isspace(*str)) str++;
    p = str + strlen(str) - 1;
    while(p >= str && isspace(*p))
	*p-- = '\0';

    lprintf(L_JUNK, "CND: %s", str);

    for (cp = cndtable; cp->string; cp++)
    {
	len = strlen(cp->string);
	if (strncmp(cp->string, str, len) == 0)
	{
	    if (!cp->variable)
		return;
	    *(cp->variable) = malloc(strlen(str) - len + 1);
	    (void) strcpy(*(cp->variable), str+len);
	    lprintf(L_JUNK, "CND: found: %s", *(cp->variable));
	    return;
	}
    }
}

int
cndlookup _P0 (void)
{
    int match = 1;
#ifdef CNDFILE
    FILE *cndfile;
    char buf[BUFSIZ];
    if (!(cndfile = fopen(CNDFILE, "r")))
	return(1);
    while (fgets(buf, sizeof(buf), cndfile)) {
	register char *p = buf, *p2;
	while(isspace(*p)) p++;
	if (*p == '#' || *p == '\n')
	    continue;
	while(p2 = strtok(p, " \t\n,")) {
	    match = (*p2 != '!');

	    if (!match)
		p2++;

	    if (strcmp(p2, "all") == 0)
		goto leave;
	    if (strncmp(p2, CallerId, strlen(p2)) == 0)
		goto leave;

	    lprintf(L_JUNK, "CND: number: %s", p2);
	    p = NULL;
	}
    }
    match = 1;
  leave:
    fclose(cndfile);
#endif
    return(match);
}
