# Makefile for the "fax" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 1.8 1994/03/01 01:59:56 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I..

FAX_SCRIPTS=faxspool faxrunq faxq faxrm

all: $(FAX_SCRIPTS)

faxspool: faxspool.in ../sedscript
	../sedscript <faxspool.in >faxspool

faxrunq: faxrunq.in ../sedscript
	../sedscript <faxrunq.in >faxrunq

faxq: faxq.in ../sedscript
	../sedscript <faxq.in >faxq

faxrm: faxrm.in ../sedscript
	../sedscript <faxrm.in >faxrm

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
		$(INSTALL) -m 775 $$i $(BINDIR) ; \
	done
	chmod 700 $(BINDIR)/faxrunq
#
# data files
#
	$(INSTALL) -m 644 cour24i.pbm $(LIBDIR)
	-test -f $(LIBDIR)/faxheader || \
		$(INSTALL) -m 644 faxheader $(LIBDIR)
#
# /etc/magic
#
	-@test -r /etc/magic && \
	    if grep "string.*P1" /etc/magic >/dev/null ; then : ; \
	    else \
		echo "WARNING: no PBM magic (string P1) found in /etc/magic" ; \
		echo "         maybe you should append the definitions in \`\`fax/etc-magic''." ; \
	    fi
