#
# Makefile
#
# This is the Makefile for libpvf.
#
# $Id: Makefile,v 1.2 1998/01/21 10:24:28 marc Exp $
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
