# Makefile for the mgetty fax package
#
# SCCS-ID: $Id: Makefile,v 1.19 1994/04/24 12:53:59 gert Exp $ (c) Gert Doering
#
# this is the C compiler to use (on SunOS, the standard "cc" does not
# grok my code, so please use gcc there).
CC=gcc
#CC=cc
#
#### C Compiler Flags ####
#
# For SCO Unix 3.2v4, it may be necessary to define -D__STDC__=0
# If you have problems with dial-in / dial-out on the same line (e.g.
# 'cu' telling you "CAN'T ACCESS DEVICE"), you should try -DBROKEN_SCO_324
# If the compiler barfs about my getopt() prototype, change it (mgetty.h)
# If the linker cannot find "syslog()" (compiled with SYSLOG defined),
# link "-lsocket".
#
# On SCO Unix 3.2.2, select(S) refuses to sleep less than a second,
# so use poll(S) instead. Define -DUSE_POLL
#
# For Systems with a "login uid" (man setluid()), define -DSECUREWARE
# (All SCO Systems have this). Maybe you've to link some special library
# for it, on SCO it's "-lprot_s".
#
#
# Add "-DSVR4" to compile on System V Release 4 unix
# For SVR4.2, add "-DSVR4 -DSVR42" to the CFLAGS line.
#
# For Linux, you don't have to define anything
#
# For SunOS, you don't have to define anything either.
#   But be warned, Hardware handshake (and serial operations in general)
#   work a lot more reliably with patch 100513-04 (jumbo tty patch)!
#
# Add "-DISC" to compile on Interactive Unix. For posixized ISC 4.0 you
# may have to add -D_POSIX_SOURCE
#
# For IBM AIX, you don't have to define anything, but be warned: the
# port is not fully complete. Something still breaks. Use -DUSE_POLL!
#
# Add "-D_3B1_ -DUSE_READ -D_NOSTDLIB_H -DSHORT_FILENAMES" to compile
# on the AT&T 3b1 machine -g.t.
#
# Add "-DMEIBE" to compile on the Panasonic (Matsushita Electric
#	industry) BE workstation
#
# When compiling on HP/UX, make sure that the compiler defines _HPUX_SOURCE, 
#     if it doesn't, add "-D_HPUX_SOURCE" to the CFLAGS line.
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
# For Systems that default to non-interruptible system calls (symptom:
# timeouts in do_chat() don't work reliably) but do have siginterrupt(),
# define -DHAVE_SIGINTERRUPT. This is the default if BSD is defined.
#
# If you don't have *both* GNU CC and GNU AS, remove "-pipe"
#
# Disk statistics:
#
# The following macros select one of 5 different variants of partition
# information grabbing.  AIX, Linux, 3b1 and SVR4 systems will figure
# this out themselves.  You can test whether you got this right by
# running "make testdisk".  If it fails, consult getdisk.c for
# further instructions.
#
#	    BSDSTATFS     - BSD/hp-ux/SunOS/Dynix/vrios
#			    2-parameter statfs()
#	    ULTRIXSTATFS  - Ultrix wacko statfs
#	    SVR4	  - SVR4 statvfs()
#	    SVR3	  - SVR3/88k/Apollo/SCO 4-parameter statfs()
#	    USTAT	  - ustat(), no statfs etc.
#
#CFLAGS=-Wall -g -O2 -pipe -DSECUREWARE -DUSE_POLL
CFLAGS=-g -O2 -Wall -pipe
#CFLAGS=-g -O -DSVR4
#CFLAGS=-g -O -DSVR4 -DSVR42
#CFLAGS=-g -O -DUSE_POLL
#CFLAGS=-Wall -g -O2 -pipe
# 3B1: You can remove _NOSTDLIB_H and USE_READ if you have the
# networking library and gcc.
#CFLAGS=-D_3B1_ -D_NOSTDLIB_H -DUSE_READ -DSHORT_FILENAMES

#
# LDFLAGS specify flags to pass to the linker. You could specify
# additional libraries here, special link flags, ...
#
# To use the "setluid()" function on SCO, link "-lprot", and to use
# the "syslog()" function, link "-lsocket".
#
# For the 3B1, add "-s -shlib"
#
# For ISC, add "-linet -lpt" (and -lcposix, if necessary)
#
# For Sequent Dynix/ptx, you have to add "-lsocket"
#
LDFLAGS=
#LDFLAGS=-lprot -lsocket
#LDFLAGS=-s -shlib
#LDFLAGS=-lsocket
#
#
# the following things are mainly used for ``make install''
#
#
# program to use for installing files
#
# if your systems doesn't have one, use the shell script that I provide
# in "inst.sh" (taken from X11R5)
INSTALL=install -c -o bin -g bin
#
# where the mgetty + sendfax binaries live (used for "make install")
#
SBINDIR=/usr/local/sbin
#
# where the user executable binaries live
#
BINDIR=/usr/local/bin
#
# where the config files go to (check vs. policy.h!)
#
LIBDIR=/usr/local/lib/mgetty+sendfax
#
# the fax spool directory
#
FAX_SPOOL=/usr/spool/fax
FAX_SPOOL_IN=$(FAX_SPOOL)/incoming
#
# Where section 1 manual pages should be placed
MAN1DIR=/usr/local/man/man1
#
# Where section 4 manual pages (mgettydefs.4) should be placed
MAN4DIR=/usr/local/man/man4
#
# Section 5 man pages (faxqueue.5)
MAN5DIR=/usr/local/man/man5
#
# Section 8 man pages (sendfax.8)
MAN8DIR=/usr/local/man/man8
#
# Where the GNU Info-Files are located
#
INFODIR=/usr/local/info
#
#
# A shell that understands bourne-shell syntax
#
SHELL=/bin/sh
#
# If you have problems with the awk-programs in the fax/ shell scripts,
# try using "nawk" or "gawk" (or whatever works...) here
#
AWK=awk

#
# for mgetty, that's it. If you want to use Klaus Weidner's voice
# extentions, go ahead (don't forget to read the manual!)
#
#
# All the files needed will be put here,
# except for binaries, which are put into BINDIR (defined above)

VOICE_DIR=/usr/spool/voice

# ZYXEL_R610 should be set to 1 if you have a ZyXEL with
# a ROM release >= 6.10 and to 0 for older ROMs. This only
# affects the conversion adpcm<=>pvf.

ZYXEL_R610=1

# Add -DNO_STRSTR to CFLAGS if you don't have strstr().

# create hard/soft links, add `-s' if you want symlinks
LN=ln -s
#LN=/usr/5bin/ln

#
# Nothing to change below this line
#
VS=20
#
#
OBJS=mgetty.o logfile.o do_chat.o locks.o utmp.o logname.o login.o \
     faxrec.o faxsend.o faxlib.o faxhng.o \
     io.o gettydefs.o tio.o config.o cnd.o getdisk.o

SFAXOBJ=sendfax.o logfile.o locks.o faxlib.o faxsend.o faxrec.o \
     io.o tio.o faxhng.o cnd.o getdisk.o

all:	mgetty sendfax g3-tools fax-stuff

mgetty: $(OBJS)
	$(CC) -o mgetty $(OBJS) $(LDFLAGS)

sendfax: $(SFAXOBJ)
	$(CC) -o sendfax $(SFAXOBJ) $(LDFLAGS)

g3-tools:
	cd tools ; $(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS) -I.." "LDFLAGS=$(LDFLAGS)" all

fax-stuff:
	cd fax ; $(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS) -I.." "LDFLAGS=$(LDFLAGS)" all

contrib-all: 
	cd contrib ; $(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS) -I.." "LDFLAGS=$(LDFLAGS)" all

getdisk: getdisk.c
	$(CC) $(CFLAGS) -DTESTDISK getdisk.c -o getdisk

testdisk:	getdisk
	./getdisk / .

# README PROBLEMS
DISTRIB=README.1st THANKS TODO FTP inittab.aix inst.sh \
	Makefile ChangeLog policy.h-dist login.config dialin.config \
        mgetty.c mgetty.h ugly.h do_chat.c logfile.c logname.c locks.c \
	faxrec.c faxsend.c faxlib.c fax_lib.h sendfax.c io.c \
	tio.c tio.h gettydefs.c config.c config.h login.c faxhng.c \
	cnd.c getdisk.c mksed.c utmp.c mg_utmp.h \
	patches/pbmtog3.p1 patches/taylor104.p1 \
	patches/ecu320.p1 patches/pnmtops.p1 patches/dip337.p1 \
	fax/faxspool.in fax/faxrunq.in fax/faxq.in fax/faxrm.in \
	fax/faxcvt fax/printfax \
	fax/cour24i.pbm fax/Makefile fax/etc-magic fax/faxheader \
	tools/Makefile tools/g3cat.c tools/g3topbm.c tools/g3.c tools/g3.h \
	tools/pbmtog3.c tools/run_tbl.c \
	contrib/README contrib/scrts.c contrib/pbmscale.c contrib/g3toxwd.c \
	contrib/3b1 contrib/faxin.c contrib/Makefile contrib/lp-fax \
	contrib/faxiobe.sh contrib/sun.readme contrib/xforms.note \
	contrib/gslp-iso.p contrib/dvi-fax \
	contrib/new_fax.lj contrib/new_fax.mail contrib/new_fax.pbm \
	contrib/log_diags \
	dialog/playback.sh dialog/faxv \
	doc/Makefile doc/modems.db doc/mgetty.texi \
	doc/g3topbm.1 doc/g3cat.1 doc/sendfax.8 doc/faxspool.1 \
	doc/faxrunq.1 doc/faxq.1 doc/faxrm.1 doc/mgettydefs.4 \
	doc/fhng-codes doc/pbmtog3.1 doc/faxqueue.5 \
	voice/MANIFEST \
	voice/CHANGES voice/Makefile voice/README voice/TODO \
	voice/voclib.c voice/voclib.h voice/vpaths.c voice/zplay.c \
	voice/vanswer.c \
	voice/pvfmain.c voice/pvflib.c voice/pvflib.h voice/pvflin.c \
	voice/pvfutil.c voice/pvfadpcm.c voice/pvfau.c voice/pvfvoc.c \
	voice/pvfsine.c \
	voice/rsynth-0.9.linuxA.pch voice/speakdate \
	voice/vg_button voice/vg_dtmf voice/vg_message voice/vg_nmp \
	voice/vg_say voice/play_messages voice/pvf.1 voice/zplay.1

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

sedscript: mksed
	./mksed >sedscript
	chmod 700 sedscript

mksed: mksed.c policy.h Makefile
	$(CC) $(CFLAGS) -DBINDIR=\"$(BINDIR)\" -DSBINDIR=\"$(SBINDIR)\" \
		-DLIBDIR=\"$(LIBDIR)\" \
		-DFAX_SPOOL_OUT=\"$(FAX_SPOOL)/outgoing\" \
		-DAWK=\"$(AWK)\" \
	-o mksed mksed.c

policy.h-dist: policy.h
	@rm -f policy.h-dist
	cp policy.h policy.h-dist
	@chmod u+w policy.h-dist

mgetty0$(VS).tar.gz:	$(DISTRIB)
	rm -f mgetty-0.$(VS)
	ln -sf . mgetty-0.$(VS)
	echo "$(DISTRIB)" \
	    | tr " " "\\012" \
	    | sed -e 's;^;mgetty-0.$(VS)/;g' \
	    | gtar cvvfT mgetty0$(VS).tar -
	gzip -f -9 -v mgetty0$(VS).tar

tar:	mgetty0$(VS).tar.gz

uu:	mgetty0$(VS).tar.gz
	uuencode mgetty0.$(VS)-`date +%b%d`.tar.gz <mgetty0$(VS).tar.gz >mg.uue

shar1:	$(DISTRIB)
	shar -M -l 40 -n mgetty+sendfax-0.$(VS) -a -o mgetty.sh $(DISTRIB)

shar:	$(DISTRIB)
	shar -M $(DISTRIB) >mgetty0$(VS).sh

doc-tar:
	cd doc ; $(MAKE) "VS=$(VS)" doc-tar

policy.h:
	@echo
	@echo "You have to create your local policy.h first."
	@echo "Copy policy.h-dist and edit it."
	@echo
	@exit 1

clean:
	rm -f *.o getdisk
	cd tools ; $(MAKE) clean
	cd fax ; $(MAKE) clean
	cd contrib ; $(MAKE) clean
	cd doc ; $(MAKE) clean
	cd voice ; $(MAKE) clean

fullclean:
	rm -f *.o mgetty sendfax testgetty getdisk mksed sedscript *~
	cd tools ; $(MAKE) fullclean
	cd fax ; $(MAKE) fullclean
	cd contrib ; $(MAKE) fullclean
	cd doc ; $(MAKE) fullclean
	cd voice ; $(MAKE) fullclean

install: mgetty sendfax sedscript
	-test -d $(SBINDIR) || mkdir $(SBINDIR)
	-mv -f $(SBINDIR)/mgetty $(SBINDIR)/mgetty.old
	-mv -f $(SBINDIR)/sendfax $(SBINDIR)/sendfax.old
	$(INSTALL) -s -m 700 mgetty $(SBINDIR)
	$(INSTALL) -s -m 700 sendfax $(SBINDIR)
#
# data files
#
	test -d $(LIBDIR) || mkdir $(LIBDIR)
	test -f $(LIBDIR)/login.config || \
		$(INSTALL) -o root -m 600 login.config $(LIBDIR)/
	test -f $(LIBDIR)/dialin.config || \
		$(INSTALL) -o root -m 600 dialin.config $(LIBDIR)/
#
# test for outdated stuff
#
	-@if test -f $(LIBDIR)/mgetty.login ; \
	then \
	    echo "WARNING: the format of $(LIBDIR)/mgetty.login has " ;\
	    echo "been changed. Because of this, to avoid confusions, it's called " ;\
	    echo "$(LIBDIR)/login.config now." ;\
	    echo "" ;\
	fi
#
# fax spool directories
#
	test -d $(FAX_SPOOL) || mkdir $(FAX_SPOOL)
	test -d $(FAX_SPOOL_IN) || mkdir $(FAX_SPOOL_IN)
	test -d $(FAX_SPOOL)/outgoing || mkdir $(FAX_SPOOL)/outgoing
#
# g3 tool programs
#
	cd tools ; $(MAKE) install INSTALL="$(INSTALL)" \
				BINDIR=$(BINDIR) LIBDIR=$(LIBDIR)
#
# fax programs / scripts / font file
#
	cd fax ; $(MAKE) install INSTALL="$(INSTALL)" \
				 BINDIR=$(BINDIR) LIBDIR=$(LIBDIR)
#
# documentation
#
	cd doc ; $(MAKE) install INSTALL="$(INSTALL)" \
				MAN1DIR=$(MAN1DIR) \
				MAN4DIR=$(MAN4DIR) \
				MAN5DIR=$(MAN5DIR) \
				MAN8DIR=$(MAN8DIR) \
				INFODIR=$(INFODIR)

#
# voice extensions, consult the `voice' chapter in the documentation
#

vgetty:
	@$(MAKE) mgetty
	cd voice; $(MAKE) CFLAGS="$(CFLAGS)" CC="$(CC)" LDFLAGS="$(LDFLAGS)" \
	LN="$(LN)" ZYXEL_R610=$(ZYXEL_R610) VOICE_DIR="$(VOICE_DIR)" \
	vgetty-all

vgetty-install:
	cd voice; $(MAKE) CFLAGS="$(CFLAGS)" CC="$(CC)" LDFLAGS="$(LDFLAGS)" \
	BINDIR="$(BINDIR)" SBINDIR="$(SBINDIR)" LIBDIR="$(LIBDIR)" \
	MAN1DIR="$(MAN1DIR)" INSTALL="$(INSTALL)" \
	LN="$(LN)" ZYXEL_R610=$(ZYXEL_R610) VOICE_DIR="$(VOICE_DIR)" \
	vgetty-install

######## anything below this line was generated by gcc -MM *.c
cnd.o : cnd.c policy.h mgetty.h ugly.h 
config.o : config.c mgetty.h ugly.h config.h 
do_chat.o : do_chat.c mgetty.h ugly.h policy.h tio.h 
faxhng.o : faxhng.c mgetty.h ugly.h 
faxlib.o : faxlib.c mgetty.h ugly.h policy.h fax_lib.h 
faxrec.o : faxrec.c mgetty.h ugly.h tio.h policy.h fax_lib.h 
faxsend.o : faxsend.c mgetty.h ugly.h tio.h policy.h fax_lib.h 
files.o : files.c mgetty.h ugly.h policy.h 
getdisk.o : getdisk.c policy.h mgetty.h ugly.h 
gettydefs.o : gettydefs.c mgetty.h ugly.h policy.h 
io.o : io.c mgetty.h ugly.h 
locks.o : locks.c mgetty.h ugly.h policy.h 
logfile.o : logfile.c mgetty.h ugly.h policy.h 
login.o : login.c mgetty.h ugly.h config.h policy.h 
logname.o : logname.c mgetty.h ugly.h policy.h tio.h 
mgetty.o : mgetty.c mgetty.h ugly.h policy.h tio.h mg_utmp.h
mksed.o : mksed.c mgetty.h ugly.h policy.h 
sendfax.o : sendfax.c mgetty.h ugly.h tio.h policy.h fax_lib.h 
tio.o : tio.c mgetty.h ugly.h tio.h 
utmp.o : utmp.c mgetty.h ugly.h mg_utmp.h
