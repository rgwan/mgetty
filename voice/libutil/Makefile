#
# Makefile
#
# This is the Makefile for libutil.
#
# $Id: Makefile,v 1.3 1998/03/25 23:05:25 marc Exp $
#

UTILOBJS=check_system.o config.o no_modem.o make_path.o strsep.o variables.o \
	 ../../config.o ../../logfile.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all: libutil.a

config.o: config.c
	$(CC) $(CFLAGS) -DCONF_DIR=\"$(CONFDIR)\" -c $*.c

libutil.a: $(UTILOBJS)
	$(RM) -f libutil.a
	$(AR) $(ARFLAGS) libutil.a $(UTILOBJS)
	-$(RANLIB) libutil.a

install:

clean:
	@$(RM) -f libutil.a $(UTILOBJS)

fullclean: clean

include depend
