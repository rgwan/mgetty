# Makefile for the "fax" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 1.7 1994/02/04 15:07:50 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I..

all: faxspool faxrunq faxq faxrm

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
	-rm -f faxspool faxrunq faxq faxrm

fullclean:
	-rm -f faxspool faxrunq faxq faxrm

install: all
	-for i in faxrunq faxq faxspool faxrm; do mv $(BINDIR)/$$i $(BINDIR)/$$i.old ; done
#
# programs / scripts
#
	cp faxspool faxq faxrunq faxcvt faxrm $(BINDIR)/
	chmod 700 $(BINDIR)/faxrunq
	chmod 755 $(BINDIR)/faxspool $(BINDIR)/faxq $(BINDIR)/faxrm
#
# data files
#
	cp cour24i.pbm $(LIBDIR)/cour24i.pbm
	chmod 644 $(LIBDIR)/cour24i.pbm
	-test -f $(LIBDIR)/faxheader || \
	    cp faxheader $(LIBDIR)
	chmod 644 $(LIBDIR)/faxheader
#
# /etc/magic
#
	-@test -r /etc/magic && \
	    if grep "string.*P1" /etc/magic >/dev/null ; then : ; \
	    else \
		echo "WARNING: no PBM magic (string P1) found in /etc/magic" ; \
		echo "         maybe you should append the definitions in \`\`fax/etc-magic''." ; \
	    fi
