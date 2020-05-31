CC = gcc
CFLAGS = -Wall -g -O1
ALL= test-ds3231

all: test-ds3231

install:
	/usr/bin/install -s -m 755 -t ../../bin ${ALL}

clean:
	rm -f *.o ${ALL}

ds3231-test: test-ds3231.c
		$(CC) $(CFLAGS) -o $@ $^
