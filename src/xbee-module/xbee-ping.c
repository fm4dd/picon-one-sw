/* ------------------------------------------------------------ *
 * file:        xbee-ping.c                                     *
 * purpose:     XBEE network discovery sending the ATND command *
 *              It returns a list of parameters for each node   *
 * return:      0 on success, and -1 on errors.                 *
 * compile:     see Makefile                                    *
 * example:     ./xbee-ping                                     *
 *                                                              *
 * author:      06/10/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "serial.h"
#include "xbee.h"

/* ------------------------------------------------------------ *
 * global variables                                             *
 * ------------------------------------------------------------ */
int verbose  = 0;              // 0 = off, 1 = on
char progver[] = "1.0";        // program version
char *port   = "/dev/ttySC1";  // serial port device
int speed    = 115200;         // XBee modified speed
//int speed    = 9600;         // XBee default speed
int timeout  = 3;              // 3 seconds timeout

/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: ./xbee-ping [-s speed] [-v]\n\
Command line parameters have the following format:\n\
   -s   set serial ine speed. Default = 115200. Example -s 9600\n\
   -h   display this message\n\
   -v   enable debug output\n\
\n\
Usage examples:\n\
./xbee-ping -s 9600 -v\n";
   printf("xee-ping v%s\n\n", progver);
   printf(usage);
}

/* ----------------------------------------------------------- *
 * parseargs() checks the commandline arguments with C getopt  *
 * ----------------------------------------------------------- */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   while ((arg = (int) getopt (argc, argv, "cei:s:rhv")) != -1) {
      switch (arg) {
         // arg -v verbose, type: flag, optional
         case 'v':
            verbose = 1; break;

         // arg -s speed type: int
         case 's':
            if(verbose == 1) printf("Debug: arg -s, value %s\n", optarg);
            speed = (int) strtol(optarg, (char **)NULL, 10);
            if(speed != 9600 && speed != 19200 && speed != 38400
               && speed != 57600 && speed != 115200) {
               printf("Error: Invalid speed, must be 9600/19200/38400/57600/115200.\n");
               exit(-1);
            }
            break;

         // arg -h usage, type: flag, optional
         case 'h':
            usage(); exit(0);
            break;

         case '?':
            if(isprint (optopt))
               printf ("Error: Unknown option `-%c'.\n", optopt);
            else {
               printf ("Error: Unknown option character `\\x%x'.\n", optopt);
               usage();
               exit(-1);
            }
            break;
         default:
            usage();
            break;
      }
   }
}

/* ------------------------------------------------------------ *
 * main() function to execute the program                       *
 * ------------------------------------------------------------ */
int main(int argc, char *argv[]) {
   char response[4096];
   int ret;

   /* ---------------------------------------------------------- *
    * process the cmdline parameters                             *
    * ---------------------------------------------------------- */
   parseargs(argc, argv);

   /* ---------------------------------------------------------- *
    * Open the port with the speed in -s, or the program default *
    * ---------------------------------------------------------- */
   printf("XBee open with %s %dB\n", port, speed);
   int fd = xbee_enable(port, speed);
   if(fd != -1) printf("XBee connected %s %dB\n", port, speed);
   else printf("Error: XBee not connected\n");

   printf("XBee Network Node Discovery\n");

   /* ------------------------------------------------- *
    * Enter CMD mode                                    *
    * ------------------------------------------------- */
   if(xbee_startcmdmode(fd, timeout) == -1) return -1;

   /* ------------------------------------------------- *
    * Send node discovery command with ATND             *
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATND\r", response);
   if(ret == -1) return -1; // exit with failure code
   printf(response);

   // Data returned for ATND:
   // -----------------------
   // SH\r
   // SL\r
   // Node Name as in ATNI (0..20 bytes)
   // parent network address \r (2bytes)
   // 1-byte device type: 0=coord, 1=router, 2=enddevice \r
   // status \r (1byte reserved)
   // profile_id \r (2bytes)
   // manufacturer_id \r (2 bytes)
   // \r

   /* ------------------------------------------------- *
    * Finished, end command mode                        *
    * ------------------------------------------------- */
   if(xbee_endcmdmode(fd, timeout) == -1)   return -1;

   closeserial(fd);
   return 0;
}
