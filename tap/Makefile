#
# $Id: Makefile,v 1.3 1998/07/24 21:39:08 gert Exp $
#
CC=gcc
CFLAGS=-I.. -Wall
LIBS=-lsocket

OBJ=../tio.o ../logfile.o ../io.o ../modem.o ../locks.o ../config.o

all: tap

tap: tap.o $(OBJ)
	$(CC) -o tap tap.o $(OBJ) $(LIBS)
