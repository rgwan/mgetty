# Makefile for the "tools" subtree
# of mgetty+sendfax
#
# RCS: $Id: Makefile,v 1.3 1998/07/07 14:32:07 gert Exp $
#

CC=gcc
CFLAGS=-O2 -I..

SCRIPTS=kvg
PROGRAMS=ltest mid
TOOLS=

all: $(SCRIPTS) $(PROGRAMS)

kvg: kvg.in ../sedscript
	../sedscript <kvg.in >kvg

../sedscript: ../policy.h ../Makefile ../mksed.c
	@cd .. ; $(MAKE) sedscript

ltest: ltest.o ../tio.o ../io.o
	$(CC) $(CFLAGS) -o ltest ltest.o ../tio.o ../io.o 

ltest.o: ltest.c ../tio.h ../mgetty.h
	$(CC) $(CFLAGS) -c -o ltest.o ltest.c

mid: mid.o ../tio.o ../io.o ../locks.o
	$(CC) $(CFLAGS) -o mid mid.o ../tio.o ../io.o ../locks.o

mid.o: mid.c ../tio.h ../mgetty.h
	$(CC) $(CFLAGS) -c -o mid.o mid.c

clean:
	-rm -f $(SCRIPTS) $(PROGRAMS) *.o

fullclean:
	-rm -f $(SCRIPTS) $(PROGRAMS)

install: all
#
# programs / scripts
#
	-for i in $(SCRIPTS); do \
		mv -f $(BINDIR)/$$i $(BINDIR)/$$i.old 2>/dev/null ; \
		$(INSTALL) -m 755 $$i $(BINDIR) ; \
	done
