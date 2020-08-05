CC=gcc
CFLAGS= -O3 -Wall -g -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I./fonts
LIBS= -L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -ljpeg -lm -lwiringPi
AR=ar

ALLBIN=tft-stopwatch tft-tempgraph tft-startmenu

all: ${ALLBIN}

tft-stopwatch: ip.o tft-shared.o tft-stopwatch.o libshapes.o oglinit.o
	${CC} ${CFLAGS} -o tft-stopwatch ip.o tft-shared.o tft-stopwatch.o libshapes.o oglinit.o ${LIBS}

tft-tempgraph: ip.o tft-shared.o tft-tempgraph.o libshapes.o oglinit.o
	${CC} ${CFLAGS} -o tft-tempgraph ip.o tft-shared.o tft-tempgraph.o libshapes.o oglinit.o ${LIBS}

tft-startmenu: tft-startmenu.o tft-shared.o libshapes.o oglinit.o ip.o
	${CC} ${CFLAGS} -o tft-startmenu tft-shared.o ip.o tft-startmenu.o libshapes.o oglinit.o ${LIBS}

clean:
	rm -f *.o tft-stopwatch tft-tempgraph tft-startmenu
