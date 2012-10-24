CC=mpicc
CFLAGS=-std=c99 -O3 -Wall -DNDEBUG
#CFLAGS+=-Wall -O0 -g -pg
LFLAGS=-L/home/fweik/Base/lib -lgsl -lgslcblas -lfftw3 -lm

OBJECTS=sort.o generate_system.o visit_writer.o window-functions.o  charge-assign.o common.o error.o ewald.o interpol.o io.o p3m-common.o p3m-ik.o realpart.o timings.o p3m-ik-i.o p3m-ad.o p3m-ad-i.o p3m-ad-self-forces.o domain-decomposition.o

all: p3mstandalone

test_dd: $(OBJECTS) Makefile test-dd.c domain-decomposition.o
	$(CC) $(CFLAGS) -o test-dd test-dd.c domain-decomposition.o $(OBJECTS) $(LFLAGS)

p3mstandalone: $(OBJECTS) Makefile main.c
	$(CC) $(CFLAGS) -o p3m main.c $(OBJECTS) $(LFLAGS)

test_vtf: $(OBJECTS) Makefile test_vtf.c
	$(CC) $(CFLAGS) -o test_vtf test_vtf.c $(OBJECTS) $(LFLAGS)

dipolar_system: $(OBJECTS) Makefile dipolar_system.c
	$(CC) $(CFLAGS) -o dipolar_system dipolar_system.c $(OBJECTS) $(LFLAGS)

test: $(OBJECTS) Makefile test.c
	$(CC) $(CFLAGS) -o test test.c $(OBJECTS) $(LFLAGS)

makefile.dep : *.[ch] Makefile
	for i in *.[c]; do $(CC) -MM $(CFLAGS) "$${i}"; done > $@

include makefile.dep

visit_writer.o:
	gcc -I./tools -c tools/visit_writer.c

clean:
	rm -rf *.o p3m

.PHONY = all
