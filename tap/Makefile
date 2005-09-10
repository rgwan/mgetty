#
# $Id: Makefile,v 1.5 2005/09/10 16:23:53 gert Exp $
#
CC=gcc
CFLAGS=-I.. -Wall
#LIBS=-lsocket
LIBS=

OBJ=../tio.o ../logfile.o ../io.o ../modem.o ../locks.o ../config.o
SOBJ=../tio.o ../io.o ../modem.o ../locks.o ../config.o ../logfile.o 

all: tap atsms

tap: tap.o $(OBJ)
	$(CC) -o tap tap.o $(OBJ) $(LIBS)

atsms: atsms.o $(SOBJ)
	$(CC) -o atsms atsms.o $(SOBJ) $(LIBS)
