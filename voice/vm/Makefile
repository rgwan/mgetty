#
# Makefile
#
# This is the Makefile for libvoice.
#
# $Id: Makefile,v 1.3 1998/03/25 23:06:20 marc Exp $
#

MGETTYOBJS=../../cnd.o ../../faxhng.o ../../faxlib.o ../../faxrecp.o \
        ../../faxsend.o

VMOBJS=main.o event.o usage.o

VOICELIBS=$(MGETTYLIB) $(LIBVOICE) $(LIBUTIL)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: $(VMOBJS) vm

install:

clean:
	@$(RM) -f vm $(VMOBJS)

fullclean: clean

vm: $(VMOBJS) $(VOICELIBS)
	$(CC) $(LDFLAGS) -o vm $(VMOBJS) $(MGETTYOBJS) $(VOICELIBS) $(LIBS)

include depend
