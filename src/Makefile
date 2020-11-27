#
# Makefile for rtalgs
#

CFLAGS=-g
LDLIBS=-lm -ldmalloc

default: rtalgs

rtalgs: rtalgs.o skipl.o

rtalgs.o: rtalgs.h skipl.h
skiptl.o: skipl.h

clean:
	rm -rf rtalgs *.o


.PHONY: default clean

