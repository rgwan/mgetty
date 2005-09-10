#
# $Id: Makefile,v 1.4 2005/09/10 15:41:45 gert Exp $
#
CC=gcc
CFLAGS=-I.. -Wall
#LIBS=-lsocket
LIBS=

OBJ=../tio.o ../logfile.o ../io.o ../modem.o ../locks.o ../config.o
SOBJ=../tio.o ../io.o ../modem.o ../locks.o ../config.o

all: tap atsms

tap: tap.o $(OBJ)
	$(CC) -o tap tap.o $(OBJ) $(LIBS)

atsms: atsms.o $(SOBJ)
	$(CC) -o atsms atsms.o $(SOBJ) $(LIBS)
