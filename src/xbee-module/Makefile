CC=gcc
CFLAGS= -O1 -Wall -g -I/opt/vc/include -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I./fonts
TFTLIBS= -L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -ljpeg -lm
AR=ar

ALLBIN=xbee-test tft-xbee-info

all: ${ALLBIN}

xbee-test: serial.o xbee-test.o xbee.o
	${CC} ${CFLAGS} -o xbee-test xbee-test.o serial.o xbee.o

tft-xbee-info: ip.o tft-xbee-info.o libshapes.o oglinit.o xbee.o serial.o
	${CC} ${CFLAGS} -o tft-xbee-info ip.o tft-xbee-info.o libshapes.o oglinit.o xbee.o serial.o ${TFTLIBS}

clean:
	$(RM) *.o ${ALLBIN}
