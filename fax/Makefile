# Makefile for the "fax" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 1.1 1994/01/13 21:52:25 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I..

all: faxspool faxrunq faxq

install: all
	for i in faxrunq faxq faxspool; do mv $(BINDIR)/$$i $(BINDIR)/$$i.old ; done
#
# programs / scripts
#
	cp faxspool faxq faxrunq $(BINDIR)/
	chmod 700 $(BINDIR)/faxrunq
	chmod 755 $(BINDIR)/faxspool $(BINDIR)/g3cat
#
# data files
#
	cp cour24i.pbm $(LIBDIR)/cour24i.pbm
	chmod 644 $(LIBDIR)/cour24i.pbm
