# Makefile for the "t/" subtree
# of mgetty+sendfax
#
# CVS: $Id: Makefile,v 1.2 2004/11/11 20:47:45 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I.. -g 

TEST_PROGRAMS=t_config
TEST_SCRIPTS=t_echo

test: $(TEST_PROGRAMS)
	@for f in $(TEST_PROGRAMS) ; \
	do \
	    echo "run t/$$f..." ; \
	    ./$$f || exit 1 ; \
	done
	@for f in $(TEST_SCRIPTS) ; \
	do \
	    echo "run t/$$f..." ; \
	    rm -f $$f ; ../sedscript <$$f.in >$$f ; chmod +x $$f ; \
	    ./$$f || exit 1 ; \
	done
	@echo "OK"

clean:
	rm -f $(TEST_PROGRAMS)
	rm -f *.o

t_config.o: t_config.c ../mgetty.h ../conf_mg.h ../config.h 
