# Makefile for the mgetty fax package
#
# SCCS-ID: $Id: Makefile,v 1.17 1993/11/24 20:56:27 gert Exp $ (c) Gert Doering
#
# this is the C compiler to use (on SunOS, the standard "cc" does not
# grok my code, so please use gcc there).
CC=gcc
#CC=cc
#
# For SCO Unix 3.2v4, it may be necessary to define -D__STDC__=0
# If you have problems with dial-in / dial-out on the same line (e.g.
# 'cu' telling you "CAN'T ACCESS DEVICE"), you should try -DBROKEN_SCO_324
# If the compiler barfs about my getopt() prototype, change it (mgetty.h)
#
# On SCO Unix 3.2.2, select(S) refuses to sleep less than a second,
# so use poll(S) instead. Define -DUSE_POLL
#
# Add "-DSVR4" to compile on System V Release 4 unix
#
# For Linux, you don't have to define anything (remove -DUSE_POLL!)
#
# For SunOS, you don't have to define anything either
#
# Add "-DISC" to compile on Interactive Unix
#
# For IBM AIX, you don't have to define anything, but be warned: the
# port is not fully complete. Something still breaks
#
# For SunOS, do not use hardware handshake, doesn't work yet (but I'm
# working on it!)
#
# Add "-D_3B1_ -DUSE_READ -D_NOSTDLIB_H -DSHORT_FILENAMES" to compile
# on the AT&T 3b1 machine -g.t.
#
# Add "-DMEIBE" to compile on the Panasonic (Matsushita Electric
#	industry) BE workstation
#
# For (otherwise not mentioned) systems with BSD utmp handling, define -DBSD
#
# Add "-D_NOSTDLIB_H" if you don't have <stdlib.h>
#
# Add "-DPOSIX_TERMIOS" to use POSIX termios.h, "-DSYSV_TERMIO" to use
#     the System 5 termio.h style. (Default set in tio.h)
#
# For machines without the select() call:
#     Add "-DUSE_POLL" if you don't have select, but do have poll
#     Add "-DUSE_READ" if you don't have select or poll (ugly)
#
# For older SVR systems (with a filename length restriction to 14
#     characters), define -DSHORT_FILENAMES
#
# If you don't have GNU GCC and GNU AS, remove "-pipe"!
#
CFLAGS=-Wall -g -O2 -pipe -DUSE_POLL
#CFLAGS=-g -O2 -Wall -pipe
#CFLAGS=-g -O -DSVR4
#CFLAGS=-g -O -DUSE_POLL
#CFLAGS=-Wall -g -O2 -pipe
# 3B1: You can remove _NOSTDLIB_H and USE_READ if you have the
# networking library and gcc.
#CFLAGS=-D_3B1_ -D_NOSTDLIB_H -DUSE_READ -DSHORT_FILENAMES

#
# LDFLAGS specify flags to pass to the linker. You could specify
# additional libraries here, special link flags, ...
#
# For the 3B1, add "-s -shlib"
#
# For Sequent Dynix/ptx, you have to add "-lsocket"
#
LDFLAGS=
#LDFLAGS=-s -shlib
#LDFLAGS=-lsocket

#	sed -e 's/^#ident/static char sccsid[]=/' <$< >/tmp/$<
#
# where the binaries (mgetty + sendfax) live (used for "make install")
#
BINDIR=/usr/local/bin
#
# the fax spool directory
#
FAX_SPOOL=/usr/spool/fax
FAX_SPOOL_IN=$(FAX_SPOOL)/incoming
#
# Where section 1 manual pages should be placed
#
MAN1DIR=/usr/local/man/man1
#
# A shell that understands bourne-shell syntax
#
SHELL=/bin/sh
#
# Nothing to change below this line
#
#
#
OBJS=mgetty.o logfile.o do_chat.o faxrec.o locks.o faxlib.o io.o \
     logname.o utmp.o gettydefs.o tio.o

SFAXOBJ=sendfax.o logfile.o locks.o faxlib.o faxrec.o io.o tio.o

all:	mgetty sendfax g3-tools

mgetty: $(OBJS)
	$(CC) -o mgetty $(OBJS) $(LDFLAGS)

sendfax: $(SFAXOBJ)
	$(CC) -o sendfax $(SFAXOBJ) $(LDFLAGS)

g3-tools:
	cd tools ; $(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS) -I.." "LDFLAGS=$(LDFLAGS)" all

contrib-all: 
	cd contrib ; $(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS) -I.." "LDFLAGS=$(LDFLAGS)" all

info: mgetty.info

mgetty.info: mgetty.texi
	makeinfo mgetty.texi

# README PROBLEMS
DISTRIB=README.1st THANKS TODO Makefile ChangeLog policy.h-dist \
        mgetty.c mgetty.h do_chat.c logfile.c logname.c locks.c \
	faxrec.c faxlib.c fax_lib.h sendfax.c io.c utmp.c tio.c tio.h \
        gettydefs.c \
	pbmtog3.p1 \
	fax/faxcvt fax/printfax fax/faxspool fax/faxrunq fax/faxq \
	fax/cour24i.pbm \
	tools/Makefile tools/g3cat.c tools/g3topbm.c tools/g3.c tools/g3.h \
	contrib/README contrib/scrts.c contrib/pbmscale.c contrib/g3toxwd.c \
	contrib/3b1 contrib/faxin.c contrib/Makefile contrib/lp-fax \
	contrib/ecu320.p1 contrib/faxiobe.sh contrib/sun.readme \
	doc/Makefile doc/mgetty.texi \
	doc/g3topbm.1 doc/g3cat.1 doc/sendfax.1 \
	doc/faxspool.1 doc/faxrunq.1

noident: policy.h
	for file in $(DISTRIB) policy.h ; do \
	    echo "$$file..."; \
	    case $$file in \
	      *.c) \
		mv -f $$file tmp-noident; \
		sed -e "s/^#ident/static char sccsid[] =/" <tmp-noident >$$file; \
		;; \
	      *.h) \
		mv -f $$file tmp-noident; \
		f=`basename $$file .h`; \
		sed -e "s/^#ident/static char sccs_$$f[] =/" <tmp-noident >$$file; \
		;; \
	    esac; \
	done
	$(MAKE) all

policy.h-dist: policy.h
	@rm -f policy.h-dist
	cp policy.h policy.h-dist
	@chmod u+w policy.h-dist

tar:	$(DISTRIB)
	tar cvf mgetty016.tar $(DISTRIB)
	gzip -f -9 mgetty016.tar

shar1:	$(DISTRIB)
	shar -M -l 40 -o mgetty.sh $(DISTRIB)

shar:	$(DISTRIB)
	shar -M $(DISTRIB) >mgetty016.sh

policy.h:
	@echo
	@echo "You have to create your local policy.h first."
	@echo "Copy policy.h-dist and edit it."
	@echo
	@exit 1

clean:
	rm -f *.o
	cd tools ; $(MAKE) clean
	cd contrib ; $(MAKE) clean
	cd doc ; $(MAKE) clean

fullclean:
	rm -f *.o mgetty sendfax testgetty
	cd tools ; $(MAKE) fullclean
	cd contrib ; $(MAKE) fullclean
	cd doc ; $(MAKE) fullclean

install: mgetty sendfax g3-tools
	-mv -f $(BINDIR)/mgetty $(BINDIR)/mgetty.old
	-mv -f $(BINDIR)/sendfax $(BINDIR)/sendfax.old
	-for i in faxrunq faxq faxspool; do mv $(BINDIR)/$$i $(BINDIR)/$$i.old ; done
	cp mgetty sendfax tools/g3cat fax/faxspool fax/faxq fax/faxrunq $(BINDIR)/
	chmod 700 $(BINDIR)/mgetty $(BINDIR)/sendfax
	chmod 700 $(BINDIR)/faxrunq
	chmod 755 $(BINDIR)/faxspool $(BINDIR)/g3cat
	test -d $(FAX_SPOOL) || mkdir $(FAX_SPOOL)
	test -d $(FAX_SPOOL_IN) || mkdir $(FAX_SPOOL_IN)
	test -d $(FAX_SPOOL)/outgoing || mkdir $(FAX_SPOOL)/outgoing
	cd doc ; $(MAKE) install MAN1DIR=$(MAN1DIR)

######## anything below this line was generated by gcc -MM *.c
config.o : config.c mgetty.h config.h 
do_chat.o : do_chat.c mgetty.h tio.h 
faxlib.o : faxlib.c mgetty.h fax_lib.h 
faxrec.o : faxrec.c mgetty.h policy.h fax_lib.h tio.h 
gettydefs.o : gettydefs.c mgetty.h policy.h 
io.o : io.c mgetty.h 
locks.o : locks.c mgetty.h policy.h 
logfile.o : logfile.c mgetty.h policy.h 
logname.o : logname.c mgetty.h policy.h tio.h 
mgetty.o : mgetty.c mgetty.h policy.h tio.h 
sendfax.o : sendfax.c mgetty.h policy.h fax_lib.h tio.h 
tio.o : tio.c mgetty.h tio.h 
utmp.o : utmp.c mgetty.h 
