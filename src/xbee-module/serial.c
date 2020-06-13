#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "serial.h"

/* ------------------------------------------------------------ *
 * getserial() Opens and inits the serial port with given speed *
 * ------------------------------------------------------------ */
int getserial(const char *device, const int baud) {
  struct termios options;
  speed_t bps;
  int status, fd;

  /* --------------------------------------------------------- *
   * convert speed number into termios constants               *
   * --------------------------------------------------------- */
  switch (baud){
    case      50: bps =      B50; break;
    case      75: bps =      B75; break;
    case     110: bps =     B110; break;
    case     134: bps =     B134; break;
    case     150: bps =     B150; break;
    case     200: bps =     B200; break;
    case     300: bps =     B300; break;
    case     600: bps =     B600; break;
    case    1200: bps =    B1200; break;
    case    1800: bps =    B1800; break;
    case    2400: bps =    B2400; break;
    case    4800: bps =    B4800; break;
    case    9600: bps =    B9600; break;
    case   19200: bps =   B19200; break;
    case   38400: bps =   B38400; break;
    case   57600: bps =   B57600; break;
    case  115200: bps =  B115200; break;
    case  230400: bps =  B230400; break;
    case  460800: bps =  B460800; break;
    case  500000: bps =  B500000; break;
    case  576000: bps =  B576000; break;
    case  921600: bps =  B921600; break;
    case 1000000: bps = B1000000; break;
    case 1152000: bps = B1152000; break;
    case 1500000: bps = B1500000; break;
    case 2000000: bps = B2000000; break;
    case 2500000: bps = B2500000; break;
    case 3000000: bps = B3000000; break;
    case 3500000: bps = B3500000; break;
    case 4000000: bps = B4000000; break;
    default: return -2;
  }

  /* --------------------------------------------------------- *
   * try to open the port read-write                           *
   * --------------------------------------------------------- */
  if((fd=open(device, O_RDWR | O_NOCTTY
      | O_NDELAY | O_NONBLOCK)) == -1) return -1;
  fcntl (fd, F_SETFL, O_RDWR);

  /* --------------------------------------------------------- *
   * get current port params and modify                        *
   * --------------------------------------------------------- */
  tcgetattr(fd, &options);
  cfmakeraw(&options);
  cfsetispeed(&options, bps);
  cfsetospeed(&options, bps);

  options.c_cflag |= (CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;       // disabe parity
  options.c_cflag &= ~CSTOPB;       // disable 2nd stop bit
  options.c_cflag &= ~CSIZE;        // disable char size mask
  options.c_cflag |= CS8;           // set char to 8bit
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;        // disable ext. output processing
  options.c_cc [VMIN]  =   0;       // min chars for noncanonical read
  options.c_cc [VTIME] = 100;	    // 10sec noncanonical read timeout

  tcsetattr(fd, TCSANOW, &options); // apply changes immediately
  ioctl(fd, TIOCMGET, &status);
  status |= TIOCM_DTR;
  status |= TIOCM_RTS;

  ioctl (fd, TIOCMSET, &status);
  usleep(10000);                   // wait 10millisecs
  return fd;
}

/* ------------------------------------------------------------ *
 * flushserial() empty out the tx and rx buffers                *
 * ------------------------------------------------------------ */
void flushserial(const int fd){ tcflush(fd, TCIOFLUSH); }

/* ------------------------------------------------------------ *
 * closeserial() close the serial port and release the fd       *
 * ------------------------------------------------------------ */
void closeserial(const int fd) { close(fd); }

/* ------------------------------------------------------------ *
 * charserial() sends one char to the serial port               *
 * ------------------------------------------------------------ */
void charserial(const int fd, const unsigned char c){ write(fd, &c, 1); }

/* ------------------------------------------------------------ *
 * strserial() writes a string to the serial port               *
 * ------------------------------------------------------------ */
void strserial(const int fd, const char *s) { write(fd, s, strlen(s)); }

/* ------------------------------------------------------------ *
 * prtserial() send a printf formatted string to the serial port*
 * ------------------------------------------------------------ */
void prtserial(const int fd, const char *message, ...) {
  va_list argp;
  char buffer[1024];
  va_start(argp, message);
    vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp);
  strserial(fd, buffer);
}

/* ------------------------------------------------------------ *
 * checkserial() returns num bytes waiting to be read from port *
 * ------------------------------------------------------------ */
int checkserial(const int fd) {
  int res;
  if(ioctl (fd, FIONREAD, &res) == -1) return -1;
  return res;
}

/* ------------------------------------------------------------ *
 * getcharserial() get one char from the port. (10sec timeout)  *
 * ------------------------------------------------------------ */
int getcharserial(const int fd) {
  uint8_t x;
  if(read (fd, &x, 1) != 1) return -1;
  return ((int)x) & 0xFF;
}

/* ------------------------------------------------------------ *
 * msec:get milliseconds as unsigned int (49d roll-over)        *
 * ------------------------------------------------------------ */
unsigned int msec(void){
  uint64_t now;
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  now=(uint64_t)ts.tv_sec * (uint64_t)1000 
       + (uint64_t)(ts.tv_nsec / 1000000L);
  return (uint32_t)(now);
}
