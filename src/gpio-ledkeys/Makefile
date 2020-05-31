CC = gcc
CFLAGS = -Wall -g -O1

all: gpio-blink gpio-keys

gpio-blink: gpio-blink.c
	    $(CC) $(CFLAGS) -o $@ $^ -lwiringPi

gpio-keys: gpio-keys.c
	    $(CC) $(CFLAGS) -o $@ $^ -lwiringPi

clean:
	    $(RM) gpio-blink gpio-keys
