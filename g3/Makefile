# Makefile for the "g3 tools" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 1.2 1994/01/13 21:52:53 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I..

all: g3topbm g3cat

g3topbm: g3topbm.o g3.o
	$(CC) $(CFLAGS) -o g3topbm g3topbm.o g3.o $(LDFLAGS)

g3cat: g3cat.o g3.o
	$(CC) $(CFLAGS) -o g3cat g3cat.o g3.o $(LDFLAGS)

g3.o : g3.c g3.h ../mgetty.h
g3cat.o : g3cat.c g3.h ../mgetty.h 
g3topbm.o : g3topbm.c g3.h ../mgetty.h

clean:
	rm -f *.o

fullclean:
	rm -f *.o g3cat g3topbm

install: all
#
# save pbmplus' g3topbm to g3topbm.pbm
#
	-test -f $(BINDIR)/g3topbm -a ! -f $(BINDIR)/g3topbm.pbm \
	      -a ! -f $(BINDIR)/g3cat && \
	      mv $(BINDIR)/g3topbm $(BINDIR)/g3topbm.pbm
#
# install programs
#
	cp g3cat g3topbm $(BINDIR)/
