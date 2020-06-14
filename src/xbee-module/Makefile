CC=gcc
CFLAGS= -O1 -Wall -g -I/opt/vc/include -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I./fonts
TFTLIBS= -L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -ljpeg -lm
AR=ar

ALLBIN=xbee-test tft-xbee-info xbee-config xbee-ping xbee-sendhello

all: ${ALLBIN}

xbee-test: serial.o xbee-test.o xbee.o
	${CC} ${CFLAGS} -o xbee-test xbee-test.o serial.o xbee.o

xbee-sendhello: serial.o xbee-sendhello.o
	${CC} ${CFLAGS} -o xbee-sendhello xbee-sendhello.o serial.o

xbee-config: serial.o xbee-config.o xbee.o
	${CC} ${CFLAGS} -o xbee-config xbee-config.o serial.o xbee.o

xbee-ping: serial.o xbee-ping.o xbee.o
	${CC} ${CFLAGS} -o xbee-ping xbee-ping.o serial.o xbee.o

tft-xbee-info: ip.o tft-xbee-info.o libshapes.o oglinit.o xbee.o serial.o
	${CC} ${CFLAGS} -o tft-xbee-info ip.o tft-xbee-info.o libshapes.o oglinit.o xbee.o serial.o ${TFTLIBS}

clean:
	$(RM) *.o ${ALLBIN}
