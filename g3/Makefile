# Makefile for the "g3 tools" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 1.1 1993/10/28 23:00:45 gert Exp $ Copyright (c) Gert Doering
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

