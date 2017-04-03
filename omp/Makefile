OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CPPFLAGS = -g -Wall $(OMPFLAGS)
LDFLAGS = -g -Wall
LDLIBS = $(OMPLIBS) -lm


all: sensrev tree

sensrev: sensrev.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tree: tree.o queue.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o sensrev tree
