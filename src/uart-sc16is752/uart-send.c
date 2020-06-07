/* ------------------------------------------------------------ *
 * file:        uart-send.c                                     *
 * purpose:     Sample program to test serial communication for *
 *              SC16IS752 ports /dev/ttySC0 and /dev/ttySC1.    *
 *              uart-send sends a set of ASCII chars to serial  *
 *                                                              *
 *               ttySC0 RX ---\/--- RX ttySC1 --> uart-receive  *
 * uart-send --> ttySC0 TX ---/\--- TX ttySC1                   *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 * compile:     see Makefile                                    *
 * example:     ./uart-send (run ./uart-receive to see serial)  *
 * author:      05/15/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include "serial.h"

#define UART1 "/dev/ttySC0"
#define UART2 "/dev/ttySC1"
#define SPEED 115200

int main(void) {
  int fd;
  if((fd=getserial(UART1, SPEED)) < 0){
    printf("Error opening port %s\n", UART1);
    return -1;
  }
  printf("%s [%d] send: ", UART1, SPEED);

  char count;
  unsigned int next;
  next = msec()+100;

  for (count = 33; count < 128;){ //Visible characters
    if (msec() > next){
      printf("%c", count);
      fflush(stdout);
      charserial(fd, count);
      next += 100;
      ++count;
    }
  }
  printf("\n");
  closeserial(fd);
  return 0;
}
