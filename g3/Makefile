# Makefile for the "g3 tools" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 3.2 1997/01/02 15:10:23 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I.. -g

G3_PROGRAMS=pbm2g3 g3cat g32pbm

all: $(G3_PROGRAMS)

pbm2g3: pbm2g3.o g3.o run_tbl.o
	$(CC) $(CFLAGS) -o pbm2g3 pbm2g3.o g3.o run_tbl.o $(LDFLAGS)

g32pbm: g32pbm.o g3.o
	$(CC) $(CFLAGS) -o g32pbm g32pbm.o g3.o $(LDFLAGS)

g3cat: g3cat.o g3.o
	$(CC) $(CFLAGS) -o g3cat g3cat.o g3.o $(LDFLAGS)

g3.o : g3.c g3.h ../ugly.h
g3cat.o : g3cat.c g3.h ../ugly.h 
g32pbm.o : g32pbm.c g3.h ../ugly.h
pbm2g3.o : pbm2g3.c g3.h ../ugly.h
run_tbl.o : run_tbl.c ../ugly.h 

clean:
	rm -f *.o

fullclean:
	rm -f *.o $(G3_PROGRAMS)

install: all
#
# install programs
#
	for f in $(G3_PROGRAMS) ; do \
		$(INSTALL) -s -m 755 $$f $(BINDIR) ; \
	done
