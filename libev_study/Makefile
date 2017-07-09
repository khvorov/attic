DEFINES=-DCACHE_LINE_SIZE=64

CFLAGS=-Wall -pedantic -std=gnu99 -g $(DEFINES) -L $(HOME)/lib
CPPFLAGS=-Wall -pedantic -std=c++0x -g $(DEFINES)
LFLAGS=-lev -lpthread

CC=gcc
CPP=g++

BINDIR=bin

all: $(BINDIR) executables

executables:
	$(CC) $(CFLAGS) -o $(BINDIR)/echo_server echo_server.c $(LFLAGS)
	$(CC) $(CFLAGS) -o $(BINDIR)/parallel_echo_server parallel_echo_server.c $(LFLAGS)
	$(CC) $(CFLAGS) -o $(BINDIR)/pthreads_libev pthreads_libev.c $(LFLAGS)
	$(CPP) $(CPPFLAGS) -o $(BINDIR)/stress_tester stress_tester.cpp $(LFLAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

