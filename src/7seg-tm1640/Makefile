CC = gcc
CFLAGS = -Wall -g -O1
LIBS = -lwiringPi -lm
ALL= tm1640-ctl timetest stopwatch temptime

all: tm1640-ctl timetest stopwatch temptime

install:
	/usr/bin/install -s -m 755 -t ../../bin ${ALL}

clean:
	rm -f *.o ${ALL}

tm1640-ctl: tm1640.o tm1640-ctl.o
	$(CC) tm1640.o tm1640-ctl.o -o tm1640-ctl ${LIBS}

timetest: tm1640.o timetest.o
	$(CC) tm1640.o timetest.o -o timetest ${LIBS}

stopwatch: tm1640.o stopwatch.o
	$(CC) tm1640.o stopwatch.o -o stopwatch ${LIBS}

temptime: tm1640.o temptime.o
	$(CC) tm1640.o temptime.o -o temptime ${LIBS}
