#
# $Id: Makefile,v 1.2 1997/10/31 10:58:27 gert Exp $
#
CC=gcc
CFLAGS=-I.. -Wall
LIBS=-lsocket

OBJ=../tio.o ../logfile.o ../io.o ../modem.o ../locks.o

all: tap

tap: tap.o $(OBJ)
	$(CC) -o tap tap.o $(OBJ) $(LIBS)
