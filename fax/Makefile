# Makefile for the "fax" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 4.1 1997/01/12 14:53:58 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I..

FAX_SCRIPTS=faxspool faxrunq faxq faxrm 

all: $(FAX_SCRIPTS) faxrunqd faxheader

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

clean:
	-rm -f $(FAX_SCRIPTS)

fullclean:
	-rm -f $(FAX_SCRIPTS)

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
