# Makefile for the mgetty fax package
#
# SCCS-ID: $Id: Makefile,v 1.23 1995/08/30 12:39:42 gert Exp $ (c) Gert Doering
#
# this is the C compiler to use (on SunOS, the standard "cc" does not
# grok my code, so please use gcc there. On ISC 4.0, use "icc".).
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
# For SunOS 4.x, please define -Dsunos4.
#   (We can't use "#ifdef sun" because that's defined on solaris as well)
#   If you use gcc, add -Wno-missing-prototypes, our code is OK, but
#   the Sun4 header files lack a lot standard definitions...
#   Be warned, Hardware handshake (and serial operations in general)
#   work a lot more reliably with patch 100513-04 (jumbo tty patch)!
#
# For Solaris 2.x, please define -Dsolaris2, which will automatically
#   #define SVR4.
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
# On NeXTStep, add "-posix -DBSD" (otherwise, it won't compile)
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
# For Systems with broken termios / VMIN/VTIME mechanism (symptom: mgetty
# hangs in "waiting for line to clear"), define -DBROKEN_VTIME. Notably
# this hits FreeBSD 0.9 and some SVR4.2 users...
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
#CFLAGS=-Wall -O2 -pipe -DSECUREWARE -DUSE_POLL
CFLAGS=-O2 -Wall -pipe
#CFLAGS=-O -DSVR4
#CFLAGS=-O -DSVR4 -DSVR42
#CFLAGS=-O -DUSE_POLL
#CFLAGS=-Wall -g -O2 -pipe
# 3B1: You can remove _NOSTDLIB_H and USE_READ if you have the
# networking library and gcc.
#CFLAGS=-D_3B1_ -D_NOSTDLIB_H -DUSE_READ -DSHORT_FILENAMES
#CFLAGS=-std -DPOSIX_TERMIOS -O2 -D_BSD -DBSD	# for OSF/1 (w/ /bin/cc)
#CFLAGS=-posix -DBSD -O2			# for NeXT
#CFLAGS=-D_HPUX_SOURCE -Aa -DBSDSTATFS		# for HP-UX 9.x
#CFLAGS=-cckr -D__STDC__ -O -DSVR42		# for IRIX 5.2


#
# LDFLAGS specify flags to pass to the linker. You could specify
# additional libraries here, special link flags, ...
#
# To use the "setluid()" function on SCO, link "-lprot", and to use
# the "syslog()" function, link "-lsocket".
#
# For SVR4(.2) and Solaris 2, you may need "-lsocket -lnsl" for syslog().
#
# For the 3B1, add "-s -shlib"
#
# For ISC, add "-linet -lpt" (and -lcposix, if necessary)
#
# For Sequent Dynix/ptx, you have to add "-lsocket"
#
# For OSF/1, add "-lbsd".
#
# On SCO Xenix, add -ldir -lx
#
# For FreeBSD, add "-lutil" if the linker complains about
# 	"utmp.o: unresolved symbod _login"
#
LDFLAGS=
#LDFLAGS=-lprot -lsocket
#LDFLAGS=-s -shlib
#LDFLAGS=-lsocket
#LDFLAGS=-lbsd					# OSF/1
#LDFLAGS=-posix					# NeXT
#
#
# the following things are mainly used for ``make install''
#
#
# program to use for installing files
#
# "-o <username>" sets the username that will own the binaries after installing.
# "-g <group>" sets the group
# "-c" means "copy" (as opposed to "move")
#
# if your systems doesn't have one, use the shell script that I provide
# in "inst.sh" (taken from X11R5). Needed on IRIX5.2
INSTALL=install -c -o bin -g bin
#INSTALL=install -c -o root -g wheel		# NeXT/BSD
#INSTALL=installbsd -c -o bin -g bin		# OSF/1
#INSTALL=/usr/ucb/install -c -o bin -g bin	# AIX, Solaris 2.x
#INSTALL=installbsd -c -o bin -g bin		# AIX 4.1
#
# prefix, where most (all?) of the stuff lives, usually /usr/local or /usr
#
prefix=/usr/local
#
# prefix for all the spool directories (usually /usr/spool or /var/spool)
#
spool=/var/spool
#
# where the mgetty + sendfax binaries live (used for "make install")
#
SBINDIR=$(prefix)/sbin
#
# where the user executable binaries live
#
BINDIR=$(prefix)/bin
#
# where the font+coverpage files go to (check vs. policy.h!)
#
LIBDIR=$(prefix)/lib/mgetty+sendfax
#
# where the configuration files (*.config, aliases, fax.allow/deny) go to
#
CONFDIR=$(prefix)/etc/mgetty+sendfax
#CONFDIR=/etc/default/
#
#
# the fax spool directory
#
FAX_SPOOL=$(spool)/fax
FAX_SPOOL_IN=$(FAX_SPOOL)/incoming
FAX_SPOOL_OUT=$(FAX_SPOOL)/outgoing
#
#
# Where section 1 manual pages should be placed
MAN1DIR=$(prefix)/man/man1
#
# Where section 4 manual pages (mgettydefs.4) should be placed
MAN4DIR=$(prefix)/man/man4
#
# Section 5 man pages (faxqueue.5)
MAN5DIR=$(prefix)/man/man5
#
# Section 8 man pages (sendfax.8)
MAN8DIR=$(prefix)/man/man8
#
# Where the GNU Info-Files are located
#
INFODIR=$(prefix)/info
#
#
# A shell that understands bourne-shell syntax
# (on some ultrix systems, you may need /bin/sh5 here)
#
SHELL=/bin/sh
#
# If you have problems with the awk-programs in the fax/ shell scripts,
# try using "nawk" or "gawk" (or whatever works...) here
# needed on most SunOS/Solaris/ISC/NeXTStep versions.
#
AWK=awk
#
# An echo program that understands escapes like "\n" for newline or
# "\c" for "no-newline-at-end". On SunOS, this is /usr/5bin/echo, in the
# bash, it's "echo -e"
# (don't forget the quotes, otherwise compiling mksed will break!!)
#
# If you do not have *any* echo program at all that will understand "\c",
# please use the "mg.echo" program provided in the compat/ subdirectory.
# Set ECHO="mg.echo" and INSTALL_MECHO to mg.echo
#
ECHO="echo"
#
# INSTALL_MECHO=mg.echo

#
# for mgetty, that's it. If you want to use Klaus Weidner's voice
# extentions, go ahead (don't forget to read the manual!)
#
#
# All the files needed will be put here,
# except for binaries, which are put into BINDIR (defined above)

VOICE_DIR=$(spool)/voice

# To maintain security, I recommend creating a new group for
# users who are allowed to manipulate the recorded voice messages.
PHONE_GROUP=phone
PHONE_PERMS=770

# Add -DNO_STRSTR to CFLAGS if you don't have strstr().

# create hard/soft links, add `-s' if you want symlinks
LN=ln -s
#LN=/usr/5bin/ln

RM=rm
MV=mv

#
# Nothing to change below this line
#
VS=98
#
#
OBJS=mgetty.o logfile.o do_chat.o locks.o utmp.o logname.o login.o \
     mg_m_init.o faxrec.o faxsend.o faxlib.o faxhng.o \
     io.o gettydefs.o tio.o cnd.o getdisk.o goodies.o \
     config.o conf_mg.o do_stat.o

SFAXOBJ=sendfax.o logfile.o locks.o faxlib.o faxsend.o faxrec.o \
     io.o tio.o faxhng.o cnd.o getdisk.o config.o conf_sf.o

all:	mgetty sendfax g3-tools fax-stuff

# a few C files need extra compiler arguments

mgetty.o : mgetty.c syslibs.h mgetty.h ugly.h policy.h tio.h fax_lib.h \
	config.h mg_utmp.h Makefile
	$(CC) $(CFLAGS) -DFAX_SPOOL_IN=\"$(FAX_SPOOL_IN)\" \
		-DCONFDIR=\"$(CONFDIR)\" -c mgetty.c

conf_mg.o : conf_mg.c mgetty.h ugly.h policy.h syslibs.h \
	config.h conf_mg.h Makefile
	$(CC) $(CFLAGS) -DCONFDIR=\"$(CONFDIR)\" -c conf_mg.c

conf_sf.o : conf_sf.c mgetty.h ugly.h policy.h syslibs.h \
	config.h conf_sf.h Makefile
	$(CC) $(CFLAGS) -DCONFDIR=\"$(CONFDIR)\" -c conf_sf.c

login.o : login.c mgetty.h ugly.h config.h policy.h mg_utmp.h  Makefile
	$(CC) $(CFLAGS) -DCONFDIR=\"$(CONFDIR)\" -c login.c

cnd.o : cnd.c syslibs.h policy.h mgetty.h ugly.h config.h  Makefile
	$(CC) $(CFLAGS) -DCONFDIR=\"$(CONFDIR)\" -c cnd.c

# here are the binaries...

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

doc-all: 
	cd doc ; $(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS) -I.." "LDFLAGS=$(LDFLAGS)" doc-all

getdisk: getdisk.c
	$(CC) $(CFLAGS) -DTESTDISK getdisk.c -o getdisk

testdisk:	getdisk
	./getdisk / .

# README PROBLEMS
DISTRIB=README.1st THANKS TODO BUGS FTP FAQ inittab.aix inst.sh version.h \
	Makefile ChangeLog policy.h-dist \
	login.cfg.in mgetty.cfg.in sendfax.cfg.in dialin.config \
        mgetty.c mgetty.h ugly.h do_chat.c logfile.c logname.c locks.c \
	mg_m_init.c faxrec.c faxsend.c faxlib.c fax_lib.h sendfax.c \
	io.c tio.c tio.h gettydefs.c login.c do_stat.c faxhng.c \
	config.h config.c conf_sf.h conf_sf.c conf_mg.h conf_mg.c \
	cnd.c getdisk.c mksed.c utmp.c mg_utmp.h syslibs.h goodies.c \
	patches/README patches/pbmtog3.p1 patches/taylor104.p1 \
	patches/ecu320.p1 patches/pnmtops.p1 patches/dip337.p1 \
	patches/gslp.ps-iso.p \
	fax/faxspool.in fax/faxrunq.in fax/faxq.in fax/faxrm.in \
	fax/faxcvt fax/printfax fax/cour25.pbm fax/cour25n.pbm \
	fax/Makefile fax/etc-magic fax/faxheader.in \
	tools/Makefile tools/g3cat.c tools/g3topbm.c tools/g3.c tools/g3.h \
	tools/pbmtog3.c tools/run_tbl.c \
	compat/mg.echo.c kvg \
	contrib/README contrib/scrts.c contrib/pbmscale.c contrib/pgx.c \
	contrib/g3toxwd.c contrib/g3tolj.c contrib/g3hack.c \
	contrib/g3toxwd.1 contrib/g3tolj.1 \
	contrib/3b1 contrib/faxin.c contrib/Makefile contrib/lp-fax \
	contrib/faxiobe.sh contrib/sun.readme \
	contrib/dvi-fax contrib/two-modems \
	contrib/faxdvi2.perl contrib/faxdvi.config \
	contrib/log_diags contrib/readme.supra contrib/ttyenable \
	contrib/logparse.c contrib/gs-security.fix \
	frontends/README frontends/Makefile \
	frontends/tcl/faxview-0.1 \
	frontends/tkperl/README \
	frontends/tkperl/faxman.pl frontends/tkperl/faxman.doc \
	frontends/tkperl/FileSelector.pm frontends/tkperl/XYListbox.pm \
	frontends/tkperl/handle_commands.pl frontends/tkperl/xfax.pl \
	frontends/X11/xforms.note \
	frontends/X11/viewfax-2.1.tar \
	frontends/dialog/Makefile \
	frontends/dialog/faxv.in frontends/dialog/listen.in \
	frontends/dialog/xfaxq.in frontends/dialog/xfaxq.1in \
	frontends/mail-to-fax/README frontends/mail-to-fax/faxgate.pl \
	samples/new_fax.lj samples/new_fax.mail samples/new_fax.pbm \
	samples/new_fax.mime samples/new_fax.hpa samples/new_fax.vacation \
	samples/coverpg.pbm samples/coverpg.ps \
	samples/fax samples/faxmemo \
	doc/Makefile doc/modems.db doc/mgetty.texi-in doc/fhng-codes \
	doc/g3topbm.1in doc/g3cat.1in doc/sendfax.8in doc/faxspool.1in \
	doc/faxrunq.1in doc/faxq.1in doc/faxrm.1in doc/mgettydefs.4in \
	doc/pbmtog3.1in doc/faxqueue.5in doc/mgetty.8in \
	doc/coverpg.1in doc/fax.1in doc/scanner.txt \
	voice/Readme voice/Files voice/Announce voice/version.h voice/dtmf \
	voice/ChangeLog voice/Makefile voice/depend voice/ToDo \
	voice/voice.h voice/vconfig.c voice/vconfig.h \
	voice/voclib.c voice/zplay.c voice/vanswer.c voice/vmodem.c \
	voice/pvffft.c voice/vg_fft.in voice/util.c \
	voice/pvfmain.c voice/pvflib.c voice/pvflib.h voice/pvflin.c \
	voice/pvfutil.c voice/pvfadpcm.c voice/pvfau.c voice/pvfvoc.c \
	voice/pvfsine.c voice/rockwell.c \
	voice/rsynth-0.9.linuxA.pch voice/speakdate.sh voice/speakdate.pl \
	voice/vg_button.in voice/vg_dtmf.in voice/vg_message.in \
	voice/vg_nmp.in voice/vg_say.in voice/play_messages.in \
	voice/vg_call.in voice/message \
	voice/pvf.1 voice/zplay.1

noident: policy.h
	    for file in $(DISTRIB) policy.h ; do \
	    echo "$$file..."; \
	    case $$file in \
	      *.c) \
		mv -f $$file tmp-noident; \
		sed -e "s/^#ident\(.*\)$$/static char sccsid[] =\1;/" <tmp-noident >$$file; \
		;; \
	      *.h) \
		mv -f $$file tmp-noident; \
		f=`basename $$file .h`; \
		sed -e "s/^#ident\(.*\)$$/static char sccs_$$f[] =\1;/" <tmp-noident >$$file; \
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
		-DCONFDIR=\"$(CONFDIR)\" \
		-DFAX_SPOOL=\"$(FAX_SPOOL)\" \
		-DFAX_SPOOL_IN=\"$(FAX_SPOOL_IN)\" \
		-DFAX_SPOOL_OUT=\"$(FAX_SPOOL_OUT)\" \
		-DAWK=\"$(AWK)\" \
		-DECHO=\"$(ECHO)\" \
		-DSHELL=\"$(SHELL)\" \
		-DVOICE_DIR=\"$(VOICE_DIR)\" \
	-o mksed mksed.c

policy.h-dist: policy.h
	@rm -f policy.h-dist
	cp policy.h policy.h-dist
	@chmod u+w policy.h-dist

version.h: $(DISTRIB)
	rm -f version.h
	if expr $(VS) : ".[13579]" >/dev/null ; then \
	    date=`date "+%B%d"` ;\
	    echo "static char * mgetty_version = \"experimental test release 0.$(VS)-$$date\";" >version.h ;\
	else \
	    echo "static char * mgetty_version = \"official release 0.$(VS)\";" >version.h ;\
	fi
	chmod 444 version.h

mgetty0$(VS).tar.gz:	$(DISTRIB)
	rm -f mgetty-0.$(VS)
	ln -sf . mgetty-0.$(VS)
	echo "$(DISTRIB)" \
	    | tr " " "\\012" \
	    | sed -e 's;^;mgetty-0.$(VS)/;g' \
	    | gtar cvvfT mgetty0$(VS).tar -
	gzip -f -9 -v mgetty0$(VS).tar

tar:	mgetty0$(VS).tar.gz

mg.uue:	mgetty0$(VS).tar.gz
	uuencode mgetty0.$(VS)-`date +%b%d`.tar.gz <mgetty0$(VS).tar.gz >mg.uue

uu:	mg.uue

uu2:	mg.uue
	split -3600 mg.uue mg.uu.

# this is for automatic uploading to the beta site. 
# DO NOT USE IT if you're not ME! Please!
#
beta:	mgetty0$(VS).tar.gz
	test `hostname` = greenie.muc.de || exit 1
# local
	cp mgetty0$(VS).tar.gz /pub/uploads/
# beta ftp site
	echo "ftp -v ftp.informatik.tu-muenchen.de <<EOF" >ftp.sh
#	echo "ftp -v -n ftp <<EOF" >ftp.sh
#	echo "user ftp gert@greenie.muc.de" >>ftp.sh
	echo "cd ~ftp/pub/comp/networking/communication/modem/mgetty" >>ftp.sh
	echo "bin" >>ftp.sh
	echo "hash" >>ftp.sh
	echo "put mgetty0"$(VS)".tar.gz mgetty0"$(VS)-`date +%b%d`.tar.gz >>ftp.sh
	echo "quit" >>ftp.sh
	sh ftp.sh
	rm ftp.sh
	rcmd hp2 -l doering 'cd $$HOME ; ./beta'

shar1:	$(DISTRIB)
	shar -M -c -l 40 -n mgetty+sendfax-0.$(VS) -a -o mgetty.sh $(DISTRIB)

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

login.config: login.cfg.in sedscript
	./sedscript <login.cfg.in >login.config

mgetty.config: mgetty.cfg.in sedscript
	./sedscript <mgetty.cfg.in >mgetty.config

sendfax.config: sendfax.cfg.in sedscript
	./sedscript <sendfax.cfg.in >sendfax.config

# internal: use this to create a "clean" mgetty+sendfax tree
bindist: all doc-all sedscript
	-rm -rf bindist
	mkdir bindist
	-mkdir bindist`dirname $(prefix)` 2>/dev/null
	-mkdir bindist`dirname $(spool)` 2>/dev/null
	bd=`pwd`/bindist; $(MAKE) prefix=$$bd$(prefix) \
		BINDIR=$$bd$(BINDIR) SBINDIR=$$bd$(SBINDIR) \
		LIBDIR=$$bd$(LIBDIR) CONFDIR=$$bd$(CONFDIR) \
		spool=$$bd$(spool) FAX_SPOOL=$$bd$(FAX_SPOOL) \
		FAX_SPOOL_IN=$$bd$(FAX_SPOOL_IN) \
		FAX_SPOOL_OUT=$$bd$(FAX_SPOOL_OUT) \
		VOICE_DIR=$$bd$(VOICE_DIR) \
		MAN1DIR=$$bd$(MAN1DIR) MAN4DIR=$$bd$(MAN4DIR) \
		MAN5DIR=$$bd$(MAN5DIR) MAN8DIR=$$bd$(MAN8DIR) \
		INFODIR=$$bd$(INFODIR) install
	cd bindist; gtar cvvfz mgetty$(VS)-bin.tgz *


install: install-bin install-doc

install-bin: mgetty sendfax login.config mgetty.config sendfax.config
#
# binaries
#
	-test -d $(prefix)  || ( mkdir $(prefix)  ; chmod 755 $(prefix)  )
	-test -d $(BINDIR)  || ( mkdir $(BINDIR)  ; chmod 755 $(BINDIR)  )
	$(INSTALL) -m 755 kvg $(BINDIR)

	-test -d $(SBINDIR) || ( mkdir $(SBINDIR) ; chmod 755 $(SBINDIR) )
	-mv -f $(SBINDIR)/mgetty $(SBINDIR)/mgetty.old
	-mv -f $(SBINDIR)/sendfax $(SBINDIR)/sendfax.old
	$(INSTALL) -s -m 700 mgetty $(SBINDIR)
	$(INSTALL) -s -m 700 sendfax $(SBINDIR)
#
# data files + directories
#
	test -d $(LIBDIR)  || \
		( mkdir `dirname $(LIBDIR)` $(LIBDIR) ; chmod 755 $(LIBDIR) )
	test -d $(CONFDIR) || \
		( mkdir `dirname $(CONFDIR)` $(CONFDIR); chmod 755 $(CONFDIR))
	test -f $(CONFDIR)/login.config || \
		$(INSTALL) -o root -m 600 login.config $(CONFDIR)/
	test -f $(CONFDIR)/mgetty.config || \
		$(INSTALL) -o root -m 600 mgetty.config $(CONFDIR)/
	test -f $(CONFDIR)/sendfax.config || \
		$(INSTALL) -o root -m 600 sendfax.config $(CONFDIR)/
	test -f $(CONFDIR)/dialin.config || \
		$(INSTALL) -o root -m 600 dialin.config $(CONFDIR)/
#
# test for outdated stuff
#
	-@if test -f $(LIBDIR)/mgetty.login ; \
	then \
	    echo "WARNING: the format of $(LIBDIR)/mgetty.login has " ;\
	    echo "been changed. Because of this, to avoid confusions, it's called " ;\
	    echo "$(CONFDIR)/login.config now." ;\
	    echo "" ;\
	fi
#
# fax spool directories
#
	test -d $(spool) || \
		( mkdir $(spool) ; chmod 755 $(spool) )
	test -d $(FAX_SPOOL) || \
		( mkdir $(FAX_SPOOL) ; chmod 755 $(FAX_SPOOL) )
	test -d $(FAX_SPOOL_IN) || \
		( mkdir $(FAX_SPOOL_IN) ; chmod 755 $(FAX_SPOOL_IN) )
	test -d $(FAX_SPOOL_OUT) || \
		( mkdir $(FAX_SPOOL_OUT) ; chmod 1777 $(FAX_SPOOL_OUT) )
#
# g3 tool programs
#
	cd tools ; $(MAKE) install INSTALL="$(INSTALL)" \
				BINDIR=$(BINDIR) \
				LIBDIR=$(LIBDIR) CONFDIR=$(CONFDIR)
#
# fax programs / scripts / font file
#
	cd fax ; $(MAKE) install INSTALL="$(INSTALL)" \
				BINDIR=$(BINDIR) \
				LIBDIR=$(LIBDIR) CONFDIR=$(CONFDIR)
#
# compatibility
#
	if [ ! -z "$(INSTALL_MECHO)" ] ; then \
	    cd compat ; \
	    gcc -o mg.echo mg.echo.c && \
	    $(INSTALL) -s -m 755 mg.echo $(BINDIR) ; \
	fi

#
# documentation
#
install-doc:
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
	LN="$(LN)" MV="$(MV)" RM="$(RM)" VOICE_DIR="$(VOICE_DIR)" \
	FAX_SPOOL_IN="$(FAX_SPOOL_IN)" CONFDIR="$(CONFDIR)" \
	SHELL="$(SHELL)" vgetty-all

vgetty-install: sedscript
	cd voice; $(MAKE) CFLAGS="$(CFLAGS)" CC="$(CC)" LDFLAGS="$(LDFLAGS)" \
	BINDIR="$(BINDIR)" SBINDIR="$(SBINDIR)" LIBDIR="$(LIBDIR)" \
	CONFDIR="$(CONFDIR)" MAN1DIR="$(MAN1DIR)" INSTALL="$(INSTALL)" \
	PHONE_GROUP="$(PHONE_GROUP)" PHONE_PERMS="$(PHONE_PERMS)" \
	LN="$(LN)" MV="$(MV)" RM="$(RM)" VOICE_DIR="$(VOICE_DIR)" \
	vgetty-install

install-vgetty: vgetty-install

## misc

dump: logfile.o config.o conf_mg.o goodies.o getdisk.o tio.o gettydefs.o io.o
	gcc -o dump -g dump.c logfile.o config.o conf_mg.o goodies.o getdisk.o tio.o gettydefs.o io.o $(LDFLAGS)

######## anything below this line was generated by gcc -MM *.c
cnd.o : cnd.c syslibs.h policy.h mgetty.h ugly.h config.h 
conf_mg.o : conf_mg.c mgetty.h ugly.h policy.h syslibs.h tio.h config.h conf_mg.h 
conf_sf.o : conf_sf.c mgetty.h ugly.h policy.h syslibs.h config.h conf_sf.h 
config.o : config.c syslibs.h mgetty.h ugly.h config.h 
do_chat.o : do_chat.c syslibs.h mgetty.h ugly.h policy.h tio.h 
dump.o : dump.c syslibs.h mgetty.h ugly.h policy.h tio.h fax_lib.h mg_utmp.h \
  config.h conf_mg.h version.h 
faxhng.o : faxhng.c mgetty.h ugly.h 
faxlib.o : faxlib.c mgetty.h ugly.h policy.h fax_lib.h 
faxrec.o : faxrec.c syslibs.h mgetty.h ugly.h tio.h policy.h fax_lib.h 
faxsend.o : faxsend.c syslibs.h mgetty.h ugly.h tio.h policy.h fax_lib.h 
files.o : files.c mgetty.h ugly.h policy.h 
getdisk.o : getdisk.c policy.h mgetty.h ugly.h 
gettydefs.o : gettydefs.c syslibs.h mgetty.h ugly.h policy.h 
goodies.o : goodies.c syslibs.h mgetty.h ugly.h config.h 
io.o : io.c syslibs.h mgetty.h ugly.h 
locks.o : locks.c mgetty.h ugly.h policy.h 
logfile.o : logfile.c mgetty.h ugly.h policy.h 
login.o : login.c mgetty.h ugly.h config.h policy.h mg_utmp.h 
logname.o : logname.c syslibs.h mgetty.h ugly.h policy.h tio.h mg_utmp.h 
mg_m_init.o : mg_m_init.c syslibs.h mgetty.h ugly.h tio.h policy.h fax_lib.h 
mksed.o : mksed.c mgetty.h ugly.h policy.h 
sendfax.o : sendfax.c syslibs.h mgetty.h ugly.h tio.h policy.h fax_lib.h config.h \
  conf_sf.h 
tio.o : tio.c mgetty.h ugly.h tio.h 
utmp.o : utmp.c mgetty.h ugly.h mg_utmp.h 
