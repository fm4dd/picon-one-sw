CC=gcc
CFLAGS= -O1 -Wall -g
AR=ar
ALLBIN=uart-send uart-receive

all: ${ALLBIN}

uart-send: uart-send.o serial.o
	${CC} ${CFLAGS} -o uart-send uart-send.o serial.o

uart-receive: uart-receive.o serial.o
	${CC} ${CFLAGS} -o uart-receive uart-receive.o serial.o

clean:
	$(RM) *.o ${ALLBIN}
