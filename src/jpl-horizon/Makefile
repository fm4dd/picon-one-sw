CC=gcc
CFLAGS= -O1 -Wall -g
AR=ar
ALLBIN=jplh-client

all: ${ALLBIN}

jplh-client: jplh-client.o
	${CC} ${CFLAGS} -o jplh-client jplh-client.o -lcurl

clean:
	$(RM) *.o ${ALLBIN}
	rm -rf ./tracker-data
