#
# Makefile for rtalgs
#

CFLAGS=-g
LDLIBS=-lm -ldmalloc

default: rtalgs

# build rtalgs
rtalgs: rtalgs.o skipl.o

# clean all generated files
clean:
	rm -rf rtalgs *.o

# run an example
run: rtalgs
	./rtalgs -l example.tst

# debug
debug: rtalgs
	eval `dmalloc -l logfile -i 100 low` && ./rtalgs -e example.tst

.PHONY: default clean run

# include dependencies
rtalgs.o: rtalgs.h skipl.h
skiptl.o: skipl.h

