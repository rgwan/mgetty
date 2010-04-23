#
# $Id: Makefile,v 1.6 2010/04/23 14:23:18 gert Exp $
#
CC=gcc
CFLAGS=-I.. -Wall
#LIBS=-lsocket
LIBS=
LOG_DIR=/var/log

OBJ=../tio.o ../logfile.o ../io.o ../modem.o ../locks.o ../config.o
SOBJ=../tio.o ../io.o ../modem.o ../locks.o ../config.o ../logfile.o 

all: tap atsms

tap: tap.o $(OBJ)
	$(CC) -o tap tap.o $(OBJ) $(LIBS)

atsms: atsms.o $(SOBJ)
	$(CC) -o atsms  atsms.o $(SOBJ) $(LIBS)

atsms.o: atsms.c
	$(CC) $(CFLAGS) -c -o atsms.o -DLOG_DIR=\"$(LOG_DIR)\" atsms.c
