

CC=gcc
CFLAGS+=-g -Wall
LDLIBS+=-lm
#CFLAGS+=-DDMALLOC
#LDLIBS+=-ldmalloc

default: rtalgs
#default: skipl_test

rtalgs: rtalgs.o skipl.o lcmgcd.o

skipl_test: skipl.c skipl.h
	$(CC) -o $@ $(CFLAGS) -DSKIPL_TEST $^

clean:
	rm -f *.o rtalgs skipl_test

run: rtalgs
	./rtalgs -e example.tst
	./rtalgs -l example.tst
	./rtalgs -m example.tst
	./rtalgs -r example.tst

package: clean
	DIR=`basename $$PWD`; cd .. ; tar -czvf $$DIR-`date +%Y%m%d%H%M%S`.tar.gz $$DIR
    
zip: clean
	DIR=`basename $$PWD`; cd .. ; zip -r $$DIR-`date +%Y%m%d%H%M%S`.zip $$DIR/*c $$DIR/*.h $$DIR/Makefile $$DIR/*.tst
    

#DEPENDENCIES
rtalgs.o: rtalgs.c skipl.h lcmgcd.h
skipl.o: skipl.c skipl.h
lcmgcd.o: lcmgcd.c lcmgcd.h
