# Makefile for the "g3 tools" subtree
# of mgetty+sendfax
#
# SCCS: $Id: Makefile,v 3.1 1995/08/30 12:39:08 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I.. -g

G3_PROGRAMS=pbmtog3 g3cat g3topbm

all: $(G3_PROGRAMS)

pbmtog3: pbmtog3.o g3.o run_tbl.o
	$(CC) $(CFLAGS) -o pbmtog3 pbmtog3.o g3.o run_tbl.o $(LDFLAGS)

g3topbm: g3topbm.o g3.o
	$(CC) $(CFLAGS) -o g3topbm g3topbm.o g3.o $(LDFLAGS)

g3cat: g3cat.o g3.o
	$(CC) $(CFLAGS) -o g3cat g3cat.o g3.o $(LDFLAGS)

g3.o : g3.c g3.h ../ugly.h
g3cat.o : g3cat.c g3.h ../ugly.h 
g3topbm.o : g3topbm.c g3.h ../ugly.h
pmbtog3.o : pbmtog3.c g3.h ../ugly.h
run_tbl.o : run_tbl.c ../ugly.h 

clean:
	rm -f *.o

fullclean:
	rm -f *.o $(G3_PROGRAMS)

install: all
#
# save pbmplus' g3topbm to g3topbm.pbm
#
	-if [ -f $(BINDIR)/g3topbm -a ! -f $(BINDIR)/g3topbm.pbm \
	      -a ! -f $(BINDIR)/g3cat ] ; \
	then \
	    mv $(BINDIR)/g3topbm $(BINDIR)/g3topbm.pbm ; \
	fi
	-if [ -f $(BINDIR)/pbmtog3 -a ! -f $(BINDIR)/pbmtog3.pbm \
	      -a -f $(BINDIR)/g3topbm.pbm ] ; \
	then \
	    mv $(BINDIR)/pbmtog3 $(BINDIR)/pbmtog3.pbm ; \
	fi
#
# install programs
#
	for f in $(G3_PROGRAMS) ; do \
		$(INSTALL) -s -m 755 $$f $(BINDIR) ; \
	done
