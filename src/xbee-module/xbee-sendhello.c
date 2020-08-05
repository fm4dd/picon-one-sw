/* ------------------------------------------------------------ *
 * file:        xbee-sendhello.c                                *
 * purpose:     Sample program tests Xbee serial communication  *
 *              by sending "Hello World" strings to /dev/ttySC1 *
 *              A remote Xbee should receive these strings in   *
 *              transparent mode.                               *
 *                                                              *
 *               ttySC0 RX ---\/--- RX ttySC1 --> uart-receive  *
 * uart-send --> ttySC0 TX ---/\--- TX ttySC1                   *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 * compile:     see Makefile                                    *
 * example:     ./xbee-sendhelloi                               *
 * author:      06/12/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include "serial.h"

#define UART "/dev/ttySC1"
#define SPEED 115200

int main(void) {
  int fd;
  if((fd=getserial(UART, SPEED)) < 0){
    printf("Error opening port %s\n", UART);
    return -1;
  }
  printf("%s [%d] send:\n", UART, SPEED);

  char outstr[255];
  for(int i=0; i<10; i++) {
     snprintf(outstr, sizeof(outstr), "0123456789 Hello World %d\r", i);
     printf("%s\n", outstr);
     fflush(stdout);
     strserial(fd, outstr);
     sleep(1);
  }

  printf("\n");
  flushserial(fd);
  closeserial(fd);
  return 0;
}
