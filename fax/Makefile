# Makefile for the "fax" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 4.5 2002/11/17 22:37:44 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I.. -Wall

FAX_SCRIPTS=faxspool faxrunq faxq faxrm 

zigzag: 
	cd .. ; $(MAKE) subdirs

all: $(FAX_SCRIPTS) faxrunqd faxheader faxq-helper

faxspool: faxspool.in ../sedscript
	../sedscript <faxspool.in >faxspool

faxrunq: faxrunq.in ../sedscript
	../sedscript <faxrunq.in >faxrunq

faxrunqd: faxrunqd.in ../sedscript
	../sedscript <faxrunqd.in >faxrunqd

faxq: faxq.in ../sedscript
	../sedscript <faxq.in >faxq

faxrm: faxrm.in ../sedscript
	../sedscript <faxrm.in >faxrm

faxheader: faxheader.in ../sedscript
	../sedscript <faxheader.in >faxheader

../sedscript: ../policy.h ../Makefile ../mksed.c
	@cd .. ; $(MAKE) sedscript

faxq-helper: faxq-helper.o 
	rm -f faxq-helper
	$(CC) $(CFLAGS) -o faxq-helper faxq-helper.o
	SU chown root faxq-helper
	SU chmod 4711 faxq-helper

faxq-helper.o: faxq-helper.c
	$(CC) $(CFLAGS) -DFAX_SPOOL_OUT=\"$(FAX_SPOOL_OUT)\" \
		-DFAX_OUT_USER=\"$(FAX_OUT_USER)\" \
		-DFAX_ALLOW=\"$(CONFDIR)/fax.allow\" \
		-DFAX_DENY=\"$(CONFDIR)/fax.deny\" \
		-c faxq-helper.c

clean:
	-rm -f $(FAX_SCRIPTS) faxrunqd faxheader

fullclean:
	-rm -f $(FAX_SCRIPTS) faxrunqd faxheader

install: all
#
# programs / scripts
#
	-for i in $(FAX_SCRIPTS); do \
		mv -f $(BINDIR)/$$i $(BINDIR)/$$i.old 2>/dev/null ; \
		$(INSTALL) -m 755 $$i $(BINDIR) ; \
	done
	chmod 700 $(BINDIR)/faxrunq
	$(INSTALL) -m 700 faxrunqd $(SBINDIR)
#
# data files
#
	$(INSTALL) -m 644 cour25.pbm $(LIBDIR)
	$(INSTALL) -m 644 cour25n.pbm $(LIBDIR)
	-test -f $(CONFDIR)/faxheader || \
		$(INSTALL) -m 644 faxheader $(CONFDIR)
#
# /etc/magic
#
	-@test -r /etc/magic && \
	    if grep "string.*P1" /etc/magic >/dev/null ; then : ; \
	    else \
		echo "WARNING: no PBM magic (string P1) found in /etc/magic" ; \
		echo "         maybe you should append the definitions in \`\`fax/etc-magic''." ; \
	    fi
#
# faxspool.rules sample file
#
	$(INSTALL) -m 644 faxspool.rules $(CONFDIR)/faxspool.rules.sample
