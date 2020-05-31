CC=gcc
CFLAGS= -O3 -Wall -g -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I./fonts
#LIBS= -L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -lpthread -ljpeg -lm
LIBS= -L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -ljpeg -lm
AR=ar

ALLBIN=tft-stopwatch tft-tempgraph

all: ${ALLBIN}

tft-stopwatch: ip.o tft-stopwatch.o libshapes.o oglinit.o
	${CC} ${CFLAGS} -o tft-stopwatch ip.o tft-stopwatch.o libshapes.o oglinit.o ${LIBS} -lwiringPi

tft-tempgraph: tft-tempgraph.o libshapes.o oglinit.o
	${CC} ${CFLAGS} -o tft-tempgraph tft-tempgraph.o libshapes.o oglinit.o ${LIBS}

clean:
	rm -f *.o tft-stopwatch tft-tempgraph
