# Makefile for the "t/" subtree
# of mgetty+sendfax
#
# CVS: $Id: Makefile,v 1.1 2004/11/02 08:13:52 gert Exp $ Copyright (c) Gert Doering
#

CC=gcc
CFLAGS=-O2 -I.. -g 

TEST_PROGRAMS=t_config

test: $(TEST_PROGRAMS)
	@for f in $(TEST_PROGRAMS) ; \
	do \
	    echo "run t/$$f..." ; \
	    ./$$f && echo "OK" ; \
	done
