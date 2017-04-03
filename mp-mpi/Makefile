OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CPPFLAGS = -g -Wall
LDFLAGS = -g -Wall
LDLIBS = $(OMPLIBS) -lm

MPICC = mpicc
MPICH = /usr/lib64/openmpi/1.4-gcc
CFLAGS = -I$(MPICH)/include

all: mixedbar

mixedbar: mixedbar.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS) $(OMPLIBS)

mixedbar.o: mixedbar.c
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $(OMPFLAGS) $^

clean:
	rm -f *.o mixedbar
