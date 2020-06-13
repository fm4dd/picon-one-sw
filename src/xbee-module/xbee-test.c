/* ------------------------------------------------------------ *
 * file:        xbee-test.c                                     *
 * purpose:     Sample program tests XBEE module communication  *
 *              sends +++ for CMD mode and expects OK returned  *
 *                                                              *
 *              rpi0w <-- ttySC1 RX ---\/--- RX XBEE            *
 *              rpi0w --> ttySC1 TX ---/\--- TX XBEE            *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 * compile:     see Makefile                                    *
 * example:     ./xbee-test                                     *
 *              XBee DEV open: /dev/ttySC1 9600B                *
 *              XBee CMD mode: OK                               *
 *              Xbee cmd ATSL: [417D5111]                       *
 *                                                              *
 * author:      06/10/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "serial.h"
#include "xbee.h"

/* ------------------------------------------------------------ *
 * global variables                                             *
 * ------------------------------------------------------------ */
int verbose  = 1;              // 0 = off, 1 = on
char *port   = "/dev/ttySC1";  // serial port device
int speed    = 115200;         // XBee modified speed
//int speed    = 9600;         // XBee default speed
int timeout  = 3;              // 3 seconds timeout

int main(void) {
   printf("XBee open with %s %dB\n", port, speed);
   int fd = xbee_enable(port, speed);
   if(fd != -1) printf("XBee connected %s %dB\n", port, speed);
   else printf("Error: XBee not connected\n");
   printf("XBee getinfo\n");
   xbee_getinfo(fd);
   printf("XBee getstatus\n");
   xbee_getstatus(fd);
   closeserial(fd);
   return 0;
}
