#
# Makefile
#
# This is the Makefile for libpvf.
#
# $Id: Makefile,v 1.1 1997/12/16 12:20:42 marc Exp $
#

PVFOBJS=au.o fft.o lib.o linear.o multitech.o rockwell.o usr.o voc.o wav.o zyxel.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: libpvf.a

libpvf.a: $(PVFOBJS)
	$(RM) -f libpvf.a
	$(AR) $(ARFLAGS) libpvf.a $(PVFOBJS)
	-$(RANLIB) libpvf.a

install:

clean:
	@$(RM) -f libpvf.a $(PVFOBJS)

fullclean: clean

include depend
