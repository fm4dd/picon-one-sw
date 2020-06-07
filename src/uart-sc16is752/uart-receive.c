/* ------------------------------------------------------------ *
 * file:        uart-receive.c                                  *
 * purpose:     Sample program to test serial communication for *
 *              SC16IS752 ports /dev/ttySC0 and /dev/ttySC1.    *
 *              uart-receive prints received serial data        *
 *                                                              *
 *               ttySC0 RX ---\/--- RX ttySC1 --> uart-receive  *
 * uart-send --> ttySC0 TX ---/\--- TX ttySC1                   *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 * compile:     see Makefile                                    *
 * example:     ./uart-receive (waits for ./uart-send to start) *
 * author:      05/15/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>     //exit()
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include "serial.h"

#define UART1    "/dev/ttySC0"
#define UART2    "/dev/ttySC1"
#define SPEED    115200

int fd;

void  Handler(int signo){
  //System Exit
  printf("\r\nHandler:serialClose \r\n");
  closeserial(fd);
  exit(0);
}

int main(void) {
  char str;

  if((fd=getserial(UART2, SPEED)) < 0) {
    printf("Error opening port %s\n", UART2);
    return -1;
  }

  printf("%s [%d] receive: ", UART2, SPEED);
  fflush(stdout);
  signal(SIGINT, Handler); // Exception handling:ctrl+c
  for (;;) {
    str = getcharserial(fd);
    //putchar(str) ;
    printf("%c", str);
    fflush(stdout);

    if(str == '~'){
      printf("\r\n");
      break;
    }
  }
  closeserial(fd);
  return 0;
}
