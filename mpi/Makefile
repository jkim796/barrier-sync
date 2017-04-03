OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CPPFLAGS = -g -Wall
LDFLAGS = -g -Wall
LDLIBS = $(OMPLIBS) -lm

MPICC = mpicc
MPICH = /usr/lib64/openmpi/1.4-gcc
CFLAGS = -I$(MPICH)/include

all: tbar centbar

tbar: tbar.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

centbar: centbar.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

clean:
	rm -f *.o tbar centbar
