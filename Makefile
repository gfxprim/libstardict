CFLAGS=-W -Wall -ggdb
LDLIBS=-lz

all: sd-cmd

sd-cmd: sd-cmd.o libstardict.o

clean:
	rm -f *.o sd-cmd
