#
# Makefile
#
# This is the Makefile for libvoice.
#
# $Id: Makefile,v 1.8 1999/01/30 18:42:30 marcs Exp $
#

VOICEOBJS=analyze.o close.o command.o detect.o event.o flush.o \
	init.o mode.o open.o play.o read.o record.o shell.o signal.o write.o \
	voice_fax.o \
	Cirrus_Logic.o Digi_RAS.o Dolphin.o Dr_Neuhaus.o Elsa.o IS_101.o \
	ISDN4Linux.o Multitech_2834.o Rockwell.o Sierra.o Supra.o UMC.o \
	US_Robotics.o ZyXEL_1496.o ZyXEL_2864.o Multitech_5634.o \
        Multitech_5600ZDv.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: libvoice.a

libvoice.a: $(VOICEOBJS)
	$(RM) -f libvoice.a
	$(AR) $(ARFLAGS) libvoice.a $(VOICEOBJS)
	-$(RANLIB) libvoice.a

install:

clean:
	@$(RM) -f libvoice.a $(VOICEOBJS)

fullclean: clean

include depend
