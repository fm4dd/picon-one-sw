/*
 * Copyright (c) 2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "xbee-term.h"

typedef struct {
  char device[20];
  long unsigned int baudrate;
  int fd;
} xbee_serial_t;

int xbee_ser_invalid( xbee_serial_t *serial) {
  if (serial && serial->fd >= 0) return 0;
  #ifdef XBEE_SERIAL_VERBOSE
    if (serial == NULL) printf( "%s: serial=NULL\n", __FUNCTION__);
    else {
      printf( "%s: serial=%p, serial->fd=%d (invalid)\n", __FUNCTION__,
             serial, serial->fd);
    }
  #endif
  return 1;
}

#define _BAUDCASE(b)        case b: baud = B ## b; break
int xbee_ser_baudrate(xbee_serial_t *serial, uint32_t baudrate) {
  struct termios options;
  speed_t baud;
  // wait for the port ot become available
  do {if (xbee_ser_invalid(serial)) return -EINVAL;} while (0);
  switch (baudrate) {
      _BAUDCASE(0);
      _BAUDCASE(9600);
      _BAUDCASE(19200);
      _BAUDCASE(38400);
      _BAUDCASE(57600);
      _BAUDCASE(115200);
#ifdef B230400
      _BAUDCASE(230400);
#endif
#ifdef B460800
      _BAUDCASE(460800);
#endif
#ifdef B921600
      _BAUDCASE(921600);
#endif
      default:
          return -EINVAL;
  }

  // Get the current options for the port...
  if (tcgetattr( serial->fd, &options) == -1) {
    #ifdef XBEE_SERIAL_VERBOSE
        printf( "%s: %s failed (%d) for %" PRIu32 "\n",
            __FUNCTION__, "tcgetattr", errno, baudrate);
    #endif
    return -errno;
  }

  cfsetispeed( &options, baud); // Set the baud rates...
  cfsetospeed( &options, baud);
  cfmakeraw( &options);         // disable serial input/output processing
  options.c_cflag |= CLOCAL;    // ignore modem status lines

  // Set the new options for the port, waiting until buffered data is sent
  if (tcsetattr( serial->fd, TCSADRAIN, &options) == -1) {
      #ifdef XBEE_SERIAL_VERBOSE
          printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcsetattr", errno);
      #endif
      return -errno;
  }
  serial->baudrate = baudrate;
  return 0;
}

enum char_source {
    SOURCE_UNKNOWN,             // startup condition
    SOURCE_KEYBOARD,            // from user entering data at keyboard
    SOURCE_SERIAL,              // from XBee on serial port
    SOURCE_STATUS,              // status messages
};

struct termios _ttystate_orig;

void parse_serial_arguments(int argc, char *argv[], xbee_serial_t *serial) {
    int i;
    uint32_t baud;
    memset( serial, 0, sizeof *serial);
    serial->baudrate = 115200; // default baud rate

    for (i = 1; i < argc; ++i) {
        if (strncmp( argv[i], "/dev", 4) == 0) {
            strncpy( serial->device, argv[i], (sizeof serial->device) - 1);
            serial->device[(sizeof serial->device) - 1] = '\0';
        }
        if ( (baud = (uint32_t) strtoul( argv[i], NULL, 0)) > 0) {
            serial->baudrate = baud;
        }
    }

    while (*serial->device == '\0') {
        printf( "Connect to which device? ");
        fgets( serial->device, sizeof serial->device, stdin);
        // strip any trailing newline characters (CR/LF)
        serial->device[strcspn(serial->device, "\r\n")] = '\0';
    }
}


void xbee_term_set_color(enum char_source source) {
   const char *color;
   
   // match X-CTU's terminal of blue for keyboard text and red for serial text
   // (actually, CYAN for keyboard since BLUE is too dark for black background)
   switch (source) {
      case SOURCE_KEYBOARD:
         color = "\x1B[36;1m";      // bright cyan
         break;
      case SOURCE_STATUS:
         color = "\x1B[32;1m";      // bright green
         break;
      case SOURCE_SERIAL:
         color = "\x1B[31;1m";      // bright red
         break;
      default:
         color = "\x1B[0m";         // All attributes off
         break;
   }
   
   fputs( color, stdout);
   fflush( stdout);
}

void xbee_term_console_restore( void) {
   xbee_term_set_color(SOURCE_UNKNOWN);
   tcsetattr(STDIN_FILENO, TCSANOW, &_ttystate_orig);
}

void xbee_term_console_init( void) {
    static int init = 1;
    struct termios ttystate;

    if (init) {
        init = 0;
        tcgetattr( STDIN_FILENO, &_ttystate_orig);
        atexit( xbee_term_console_restore);
    }

    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);

    //turn off canonical mode AND ECHO
    ttystate.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
    //minimum of number input read.
    ttystate.c_cc[VMIN] = 0;

    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

int xbee_term_getchar(void) {
   int c = getc(stdin);
   if (c == EOF && feof(stdin)) {
      clearerr(stdin);
      usleep(500);
      return -EAGAIN;
   }
   if (c == EOF && ferror(stdin)) return -EIO;
   return c;
}

int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate) {
   if (serial == NULL) {
       #ifdef XBEE_SERIAL_VERBOSE
           printf( "%s: NULL parameter, return -EINVAL\n", __FUNCTION__);
       #endif
       return -EINVAL;
   }

   // make sure device name is null terminated
   serial->device[(sizeof serial->device) - 1] = '\0';

   // if device isn't already open
   if (serial->fd <= 0) {
      serial->fd = open( serial->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
      if (serial->fd < 0) {
          #ifdef XBEE_SERIAL_VERBOSE
              printf( "%s: open('%s') failed (errno=%d)\n", __FUNCTION__,
                  serial->device, errno);
          #endif
          return -errno;
      }

      fcntl( serial->fd, F_SETFL, FNDELAY); // Config fd to not block on read()
   }                                        // if there aren't any chars available
   return xbee_ser_baudrate(serial, baudrate);
}


int xbee_ser_close(xbee_serial_t *serial) {
   int result = 0;
   do {if (xbee_ser_invalid(serial)) return -EINVAL;} while (0);
   if (close( serial->fd) == -1) {
       #ifdef XBEE_SERIAL_VERBOSE
           printf( "%s: close(%d) failed (errno=%d)\n", __FUNCTION__,
               serial->fd, errno);
       #endif
       result = -errno;
   }
   serial->fd = -1;
   return result;
}

void set_color( enum char_source new_source) {
   static enum char_source last_source = SOURCE_UNKNOWN;
   if (last_source == new_source)  return;
   last_source = new_source;
   xbee_term_set_color( last_source);
}

void print_baudrate(xbee_serial_t * port) {
   set_color( SOURCE_STATUS);
   printf( "[%" PRIu32 " bps]", port->baudrate);
   fflush( stdout);
}

void next_baudrate(xbee_serial_t *port) {
   uint32_t rates[] = { 9600, 19200, 38400, 57600, 115200 };
   int i;
   for (i = 0; i < _TABLE_ENTRIES( rates); ++i) {
      if (port->baudrate == rates[i]) {
         ++i;
         break;
      }
   }
   // At end of loop, if port was using a rate not in the list above,
   // the code will end up setting the baudrate to 9600.
   xbee_ser_baudrate( port, rates[i % _TABLE_ENTRIES( rates)]);
   print_baudrate( port);
}

void check_cts(xbee_serial_t *port) {
   static int last_cts = -1;
   int status;
   int cts;
   do {if (xbee_ser_invalid(port)) printf("Error Invalid serial settings\n");} while (0);
   if (ioctl(port->fd, TIOCMGET, &status) == -1) {
       #ifdef XBEE_SERIAL_VERBOSE
          printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
             "TIOCMGET", errno);
        #endif
   }
   cts = (status & TIOCM_CTS) ? 1 : 0;
   if (cts >= 0 && cts != last_cts) {
      last_cts = cts;
      set_color(SOURCE_STATUS);
      printf("[CTS %s]", cts ? "set" : "cleared");
      fflush(stdout);
   }
}

int xbee_ser_flowcontrol(xbee_serial_t *serial, int enabled) {
   struct termios options;
   do {if (xbee_ser_invalid(serial)) return -EINVAL;} while (0);

    // Get the current options for the port...
    if (tcgetattr( serial->fd, &options) == -1) {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcgetattr", errno);
        #endif
        return -errno;
    }

#ifdef CRTSXOFF
    #define XBEE_FLOW_FLAGS (CRTSCTS | CRTSXOFF)
#else
    #define XBEE_FLOW_FLAGS (CRTSCTS)
#endif
    if (enabled) options.c_cflag |= XBEE_FLOW_FLAGS;
    else options.c_cflag &= ~XBEE_FLOW_FLAGS;
    // Set the new options for the port immediately
    if (tcsetattr( serial->fd, TCSANOW, &options) == -1) {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcsetattr", errno);
        #endif
        return -errno;
    }
    return 0;
}

enum rts_setting { RTS_ASSERT, RTS_DEASSERT, RTS_TOGGLE, RTS_UNKNOWN };
void set_rts( xbee_serial_t *port, enum rts_setting new_setting) {
   static enum rts_setting current_setting = RTS_UNKNOWN;
   bool rts;
   int status;
   if (new_setting == RTS_TOGGLE) {
      new_setting = (current_setting == RTS_ASSERT ? RTS_DEASSERT : RTS_ASSERT);
   }

   if (new_setting != current_setting) {
      current_setting = new_setting;
      rts = (current_setting == RTS_ASSERT);

      do {if (xbee_ser_invalid(port)) printf("Error invalid serial settings\n"); } while (0);
      xbee_ser_flowcontrol(port, 0); // disable flow control so our manual setting will stick
      if (ioctl(port->fd, TIOCMGET, &status) == -1) {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
                "TIOCMGET", errno);
        #endif
      }

      if (rts) status |= TIOCM_RTS;
      else status &= ~TIOCM_RTS;

      if (ioctl(port->fd, TIOCMSET, &status) == -1) {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
                "TIOCMSET", errno);
        #endif
      }

      set_color( SOURCE_STATUS);
      printf( "[%s RTS]", rts ? "set" : "cleared");
      fflush(stdout);
   }
}


enum break_setting { BREAK_ENTER, BREAK_CLEAR, BREAK_TOGGLE };
void set_break( xbee_serial_t *port, enum break_setting new_setting) {
   static enum break_setting current_setting = BREAK_CLEAR;
   bool set;

   if (new_setting == BREAK_TOGGLE) {
      new_setting = (current_setting == BREAK_CLEAR ? BREAK_ENTER : BREAK_CLEAR);
   }

   if (new_setting != current_setting) {
      current_setting = new_setting;
      set = (current_setting == BREAK_ENTER);

    do {if (xbee_ser_invalid(port)) printf("Error invalid serial settings\n"); } while (0);

    if (ioctl(port->fd, set ? TIOCSBRK : TIOCCBRK) == -1)
    {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
                "TIOCMGET", errno);
        #endif
    }
      set_color( SOURCE_STATUS);
      printf( "[%s break]", set ? "set" : "cleared");
      fflush( stdout);
   }
}

void dump_serial_data( const char *buffer, int length) {
   /// Set to TRUE if last character was a carriage return with line feed
   /// automatically appended.  When TRUE, we ignore a linefeed character.
   /// So CR -> CRLF and CRLF -> CRLF.
   static bool ignore_lf = FALSE;
   int i, ch;

   set_color(SOURCE_SERIAL);
   for (i = length; i; ++buffer, --i) {
      ch = *buffer;
      if (ch == '\r') puts( "");      // automatically insert a line feed
      else if (ch != '\n' || !ignore_lf) putchar( ch);
      ignore_lf = (ch == '\r');
   }
   fflush( stdout);
}

int xbee_ser_write(xbee_serial_t *serial, const void FAR *buffer, int length) {
    int result;
    do {if (xbee_ser_invalid(serial)) return -EINVAL;} while (0);
    if (length < 0) return -EINVAL;

    result = write( serial->fd, buffer, length);

    if (result < 0) {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: error %d trying to write %d bytes\n", __FUNCTION__,
                errno, length);
        #endif
        return -errno;
    }

    #ifdef XBEE_SERIAL_VERBOSE
        printf( "%s: wrote %d of %d bytes\n", __FUNCTION__, result, length);
        hex_dump( buffer, result, HEX_DUMP_FLAG_TAB);
    #endif
    return result;
}

int xbee_ser_read(xbee_serial_t *serial, void FAR *buffer, int bufsize) {
    int result;
    do {if (xbee_ser_invalid(serial)) return -EINVAL;} while (0);
    if (! buffer || bufsize < 0) {
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: buffer=%p, bufsize=%d; return -EINVAL\n", __FUNCTION__,
                buffer, bufsize);
        #endif
        return -EINVAL;
    }

    result = read( serial->fd, buffer, bufsize);
    if (result == -1) {
        if (errno == EAGAIN) return 0;
        #ifdef XBEE_SERIAL_VERBOSE
            printf( "%s: error %d trying to read %d bytes\n", __FUNCTION__,
                errno, bufsize);
        #endif
        return -errno;
    }

    #ifdef XBEE_SERIAL_VERBOSE
        printf( "%s: read %d bytes\n", __FUNCTION__, result);
        hex_dump( buffer, result, HEX_DUMP_FLAG_TAB);
    #endif
    return result;
}


int xbee_ser_putchar( xbee_serial_t *serial, uint8_t ch) {
    int retval;
    retval = xbee_ser_write( serial, &ch, 1);
    if (retval == 1) return 0;
    else if (retval == 0) return -ENOSPC;
    else return retval;
}


int main( int argc, char *argv[]) {
   int ch, retval;
   char buffer[40];
   xbee_serial_t serport;

   parse_serial_arguments(argc, argv, &serport);
   retval = xbee_ser_open(&serport, serport.baudrate);

   if (retval != 0) {
      fprintf( stderr, "Error %d opening serial port\n", retval);
      return EXIT_FAILURE;
   }

   // start terminal
   xbee_term_console_init(); // set up the console for nonblocking

   puts( "Simple XBee Terminal");
   puts( "CTRL-X to EXIT, CTRL-K toggles break, CTRL-R toggles RTS, " "TAB changes bps.");
   print_baudrate(&serport);
   set_rts(&serport, RTS_ASSERT);
   check_cts(&serport);
   puts("");

   for (;;) {
      check_cts(&serport);
      ch = xbee_term_getchar();
      if (ch == CTRL('X')) break;    // exit terminal
      else if (ch == CTRL('R')) set_rts(&serport, RTS_TOGGLE);
      else if (ch == CTRL('K')) set_break(&serport, BREAK_TOGGLE);
      else if (ch == CTRL('I')) {    // tab
         next_baudrate(&serport);
      }
      else if (ch > 0) {
         // Pass all characters out serial port, converting LF to CR
         // since XBee expects CR for line endings.
         xbee_ser_putchar(&serport, ch == '\n' ? '\r' : ch);
         // Only print printable characters or CR, LF or backspace to stdout.
         if (isprint(ch) || ch == '\r' || ch == '\n' || ch == '\b') {
            set_color(SOURCE_KEYBOARD);
            // stdout expects LF for line endings
            putchar(ch == '\r' ? '\n' : ch);
            fflush(stdout);
         }
      }

      retval = xbee_ser_read(&serport, buffer, sizeof buffer);
      if (retval > 0) dump_serial_data( buffer, retval);
   }

   xbee_term_console_restore();
   puts("");
   retval = xbee_ser_close(&serport);
   return 0;
}
