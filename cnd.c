#ident "@(#)cnd.c	$Id: cnd.c,v 1.9 1994/11/21 21:21:20 gert Exp $ Copyright (c) 1993 Gert Doering/Chris Lewis"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include "syslibs.h"

#include "policy.h"
#include "mgetty.h"

char *Connect = "";
char *CallerId = "none";
char *CallTime = "";
char *CallName = "";
/* the next few are for Rockwell */
char *CallDate = "";
char *CallMsg1 = "";
char *CallMsg2 = "";

/* those are for Rockwell "CONNECT" messages */
static char * cnd_carrier = "";
static char * cnd_protocol= "";

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

    /* those are for rockwell-based modems insisting on a multi-line
       message "CARRIER ... / PROTOCOL ... / CONNECT */
    {"CARRIER ",		&cnd_carrier},
    {"PROTOCOL: ",		&cnd_protocol},

    /* those are for Rockwell Caller ID */
    {"DATE = ",                 &CallDate},
    {"TIME = ",			&CallTime},
    {"NMBR = ",			&CallerId},
    {"NAME = ",			&CallName},
    {"MESG = ",			&CallMsg1},
    {"MESG = ",			&CallMsg2},

    {NULL}
};
    

void
cndfind _P1((str), char *str)
{
    struct cndtable *cp;
    register int len;
    register char *p;

    /* strip off blanks */
    
    while (*str && isspace(*str)) str++;
    p = str + strlen(str) - 1;
    while(p >= str && isspace(*p))
	*p-- = '\0';

    lprintf(L_JUNK, "CND: %s", str);

    /* The ELINK 301 ISDN modem can send us the caller ID if it is
       asked for it with AT\O. The CID will simply get sent on a single
       line consisting only of digits. So, if we get a line starting
       with a digit, let's assume that it's the CID...
     */
#ifdef ELINK
    if ( isdigit(*str) )
    {
	CallerId = p = strdup(str);
	while( isdigit(*p) ) p++;
	*p = 0;
    }
#endif

    for (cp = cndtable; cp->string; cp++)
    {
	len = strlen(cp->string);
	if (strncmp(cp->string, str, len) == 0)
	{
	    if (!cp->variable)
		return;

	    /* special case: Rockwell sends *two* MESG=... lines */
	    if (cp->variable == &CallMsg1 && CallMsg1[0] != 0)
		continue;

	    /* special case for CONNECT on Rockwell-Based modems */
	    if ( ( cnd_carrier[0] != 0 || cnd_protocol[0] != 0 ) &&
		 strncmp( str, "CONNECT ", 8 ) == 0 )
	    {
		*(cp->variable) = malloc( strlen(str) - len +
		                  strlen( cnd_carrier ) +
				  strlen( cnd_protocol ) + 5 );
		sprintf( *(cp->variable), "%s/%s %s",
			 str+len, cnd_carrier, cnd_protocol );
	    }
	    else	/* normal case */
	    {
		*(cp->variable) = malloc(strlen(str) - len + 1);
		(void) strcpy(*(cp->variable), str+len);
	    }
	    lprintf(L_JUNK, "CND: found: %s", *(cp->variable));
	    return;
	}
    }
}

/* process Rockwell-style caller ID. Weird */

void process_rockwell_mesg _P0 (void)
{
    int length = 0;
    int loop;
    char *p;

  /* In Canada, Bell Canada has come up with a fairly
     odd method of encoding the caller_id into MESG fields.
     With Supra caller ID (Rockwell), these come out as follows:

     MESG = 030735353531323132

     The first two bytes seem to mean nothing. The second
     two bytes are a hex number representing the phone number
     length. The phone number begins with the digit 3, then
     each digit of the phone number. Each digit of the phone
     number is preceeded by the character '3'.

     NB: I'm not sure whether this is Bell's or Rockwell's folly. I'd
     prefer to blaim Rockwell. gert
   */

    if ( CallMsg1[0] == 0) return;

    if ( (CallMsg1[0] != '0') || (CallMsg1[1] != '3')) return;

    /* Get the length of the number */
    CallMsg1[4] = '\0';
    sscanf( &CallMsg1[2], "%x", &length);

    lprintf(L_JUNK, "CND: number length: %d",length);
      
    /* Allocate space for the new number */
    p = CallerId = malloc(length + 1);
    
    /* Get the phone number only and put it into CallerId */
    for (loop = 5; loop <= (3 + length*2); loop += 2)
    {
	*p = CallMsg1[loop];
	p++;
    }  
    *p = 0;
      
    lprintf(L_JUNK, "CND: caller ID: %s", CallerId);
}

/* lookup Caller ID in CNDFILE, decide upon answering or not */

int cndlookup _P0 (void)
{
    int match = 1;
#ifdef CNDFILE
    FILE *cndfile;
    char buf[BUFSIZ];
    if (!(cndfile = fopen(CNDFILE, "r")))
	return(1);

    process_rockwell_mesg();

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
