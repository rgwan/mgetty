/* 
* From owner-mgetty Wed Jun 28 02:08:30 1995
* Return-Path: <owner-mgetty>
* Received: by greenie.muc.de (/\==/\ Smail3.1.24.1 #24.2)
* 	id <m0sQkgA-0001nCC@greenie.muc.de>; Wed, 28 Jun 95 02:08 MEST
* Return-Path: <l-mgetty-owner@muc.de>
* Received: by greenie.muc.de (/\==/\ Smail3.1.24.1 #24.2)
* 	id <m0sQkg6-0002jPC@greenie.muc.de>; Wed, 28 Jun 95 02:08 MEST
* Received: from greenie.muc.de ([193.174.4.62]) by colin.muc.de with SMTP id <25543-2>; Wed, 28 Jun 1995 02:08:14 +0200
* Received: by greenie.muc.de (/\==/\ Smail3.1.24.1 #24.2)
* 	id <m0sQkXo-0001nCC@greenie.muc.de>; Wed, 28 Jun 95 01:59 MEST
* Received: from GATEWAY by greenie.muc.de with netnews
* 	for mgetty@muc.de (mgetty@muc.de)
* To: mgetty@muc.de
* Date: Sat, 24 Jun 1995 21:26:42 +0200
* From: fdc@cliwe.ping.de (Frank D. Cringle)
* Message-ID: <vt68lv7cb1.fsf_-_@cliwe.ping.de>
* Organization: calls for speculation 
* References: <m0sNnpJ-0001v0C@greenie.muc.de>
* Subject: Save the forests (was: Conversion ascii->g3)
* Status: RO
* 
* gert@greenie.muc.de (Gert Doering) writes:
* >Erlend Schei wrote:
* >> Any recommendations (perhaps som script-gurus who know how to make
* >> gslp skip the white stuff at the end of the last page)?
* >
* >I'd like to see that too. I don't think it's easy, you'll have to hack the
* >dfaxhigh (or faxg3) driver, I think.
* 
* Here is a nasty little hack that will chop the end off a g3 file.  It
* is nasty 'cos it only works for single-page 1d-encoded files with
* msbfirst bit order.  But that is what mgetty uses at present, so what
* the hell.
* 
* It is rather cool in the way that it decides what to chop.  All
* identical lines at the end of the page are candidates and any more
* than 10 (by default) are discarded.  It does not matter whether they
* are white, black or striped.
* 
* I leave it to a competent shell programmer to integrate it into the
* faxspool script.
* 
* ----------------------------------------------------------------
*/
/* g3hack.c - hack identical lines off the end of a fax
 *
 * This program is in the public domain.  If it does not work or
 * causes you any sort of grief, blame the public, not me.
 *
 * fdc@cliwe.ping.de, 1995-06-24
 *
 * $Id: g3hack.c,v 1.1 1998/10/07 13:55:31 gert Exp $
 */

#include <stdio.h>
#include <stdlib.h>

extern int getopt();
extern char *optarg;
extern int optind, opterr;

char *usage = "usage: %s <-n count> <-h size> -o <outputfile>\n\n\
Copy a g3-(1d)-fax file from stdin to stdout and delete any\n\
   more than `count' identical trailing lines (default 10).\n\
Optionally skip `size'-byte header.\n\
Optionally named outputfile (else stdout).\n";

#define nxtbit()	((imask>>=1) ? ((ibits&imask)!=0) :		\
			 ((ibits=getchar()) == EOF) ? -1 :		\
			 (((imask=0x80)&ibits)!=0))
#define putbit(b)							\
    do {								\
	if (b)								\
	    obits |= omask;						\
	if ((omask >>= 1) == 0) {					\
	    this->line[this->length>>3] = obits;			\
	    omask = 0x80;						\
	    obits = 0;							\
	}								\
	this->length++;							\
	if (this->length >= BUFSIZ<<3) {				\
	    fputs("g3hack: unreasonably long line\n", stderr);		\
	    exit(1);							\
	}								\
    } while (0)

static void
copy(int nlines)
{
    int ibits, imask = 0;	/* input bits and mask */
    int obits = 0;		/* output bits */
    int omask = 0x80;		/* output mask */
    int zeros = 0;		/* number of consecutive zero bits */
    int thisempty = 1;		/* empty line (so far) */
    int empties = 0;		/* number of consecutive EOLs */
    int identcount = 0;		/* number of consecutive identical lines */
    struct {
	char line[BUFSIZ];
	int length;
    } lines[2], *prev, *this, *temp;

    this = &lines[0];
    prev = &lines[1];
    this->length = prev->length = 0;
    while (!ferror(stdout)) {
	int bit = nxtbit();
	if (bit == -1)
	    break;		/* end of file */
	putbit(bit);
	if (bit == 0) {
	    zeros++;
	    continue;
	}
	if (zeros < 11) {	/* not eol and not empty */
	    zeros = 0;
	    thisempty = 0;
	    for ( ; empties; empties--)
		fwrite("\0\1", 1, 2, stdout);
	    continue;
	}
	/* at end of line */
	zeros = 0;
	omask = 0x80;
	obits = 0;
	if (thisempty) {
	    empties++;
	    if (empties >= 6)
		break;
	    this->length = 0;
	    continue;
	}
	thisempty = 1;
	/* at end of non-empty line */
	identcount++;
	this->length = (this->length+7)&~7;
	this->line[(this->length-1)>>3] = 1; /* byte-align the eol */
	if (this->length == prev->length &&
	    memcmp(this->line, prev->line, this->length>>3) == 0) {
	    this->length = 0;
	    continue;
	}
	/* at end of non-matching line */
	for ( ; identcount; identcount--)
	    fwrite(prev->line, 1, prev->length>>3, stdout);
	temp = prev;
	prev = this;
	this = temp;
	this->length = 0;
    }
    if (identcount > nlines)
	identcount = nlines;
    for ( ; !ferror(stdout) && identcount; identcount--)
	    fwrite(prev->line, 1, prev->length>>3, stdout);
    for ( ; !ferror(stdout) && empties; empties--)
	fwrite("\0\1", 1, 2, stdout);
    if (ferror(stdout)) {
	fputs("write error\n", stderr);
	exit(1);
    }
}

int
main(int argc, char **argv)
{
    int c;
    int header = 0;
    int nlines = 10;

    opterr = 0;
    while ((c = getopt(argc, argv, "h:n:o:")) != EOF)
	switch (c) {
	case 'h':
	    header = atoi(optarg);
	    break;
	case 'n':
	    nlines = atoi(optarg);
	    break;
	case 'o':
	    if (freopen(optarg, "w", stdout) == NULL) {
		perror(optarg);
		exit(1);
	    }
	    break;
	case '?':
	    fprintf(stderr, usage, argv[0]);
	    exit(1);
	}
    while (header--)
	putchar(getchar());
    copy(nlines);
    exit(0);
}

/*
* -- 
* Frank Cringle                      | fdc@cliwe.ping.de
* voice + fax                        | +49 2304 467101
*/
