/* ------------------------------------------------------------ *
 * file:        xbee-config.c                                   *
 * purpose:     Write configuration to a local XBEE module      *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 * compile:     see Makefile                                    *
 * example:     ./xbee-config -v -i config_file                 *
 *              ./xbee-config -v -d coordinator                 *
 *              ./xbee-config -v -d enddevice                   *
 *              -r restores defaults before applying new config *
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
int factoryreset = 0;          // reset config to factory settings
int defaultcoord = 0;          // default coordinator config
int defaultdevice = 0;         // default enddevice config
extern const char *coord_conf[];
extern const char *device_conf[];

/* ------------------------------------------------------------ *
 * print_usage() prints the programs commandline instructions.  *
 * ------------------------------------------------------------ */
void usage() {
   static char const usage[] = "Usage: ./xbee-config [-s speed] [-r] [-c]  [-e] [-i config_file] [-v]\n\
Command line parameters have the following format:\n\
   -c   use default coordinator config in xbee.c\n\
   -e   use default enddevice config in xbee.c\n\
   -i   load config from file. Example: -i ./xbee-config.txt\n\
   -r   reset configuration to factory default\n\
   -s   set serial ine speed. Default = 115200. Example -s 9600\n\
   -h   display this message\n\
   -v   enable debug output\n\
\n\
Usage examples:\n\
./xbee-config -c -v\n";
   printf("xee-config v%s\n\n", progver);
   printf(usage);
}

/* ----------------------------------------------------------- *
 * parseargs() checks the commandline arguments with C getopt  *
 * ----------------------------------------------------------- */
void parseargs(int argc, char* argv[]) {
   int arg;
   opterr = 0;

   if(argc == 1 || (argc == 2 && strcmp(argv[1], "-v") == 0)) {
       printf("Error: No arguments. Need either -c, -e, -r or -i file.\n");
       printf("See ./xbee-config -h for further usage.\n");
       exit (-1);
   }

   while ((arg = (int) getopt (argc, argv, "cei:s:rhv")) != -1) {
      switch (arg) {
         // arg -v verbose, type: flag, optional
         case 'v':
            verbose = 1; break;

         // arg -c 
         case 'c':
            defaultcoord = 1;          // default coordinator config
            break;

         // arg -e
         case 'e':
            defaultdevice = 1;         // default enddevice config
            break;

         // arg -i config_file type: string
         case 'i':
            if(verbose == 1) printf("Debug: arg -i, value %s\n", optarg);
            printf("Error: Not implemented.\n");
            exit(-1);
            break;

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

         // arg -r factory reset
         case 'r':
            factoryreset = 1;          // reset config to factory settings
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

   /* ---------------------------------------------------------- *
    * process the cmdline parameters                             *
    * ---------------------------------------------------------- */
   parseargs(argc, argv);

   if(defaultcoord == 1 && defaultdevice == 1)
      printf("Error: either -c, or -e option needs to be selected.\n");

   /* ---------------------------------------------------------- *
    * Open the port with the speed in -s, or the program default *
    * ---------------------------------------------------------- */
   printf("XBee open with %s %dB\n", port, speed);
   int fd = xbee_enable(port, speed);
   if(fd != -1) printf("XBee connected %s %dB\n", port, speed);
   else printf("Error: XBee not connected\n");

   /* ---------------------------------------------------------- *
    * Reset the config to factory settings. This may disconnect  *
    * if tjhe serial speed was different from the default 9600.  *
    * ---------------------------------------------------------- */
   if(factoryreset == 1) {
      printf("Reset XBee config to factory settings\n");
      xbee_factoryreset(fd);
   }

   /* ---------------------------------------------------------- *
    * Write the default coordinator settings                     *
    * ---------------------------------------------------------- */
   if(defaultcoord == 1) {
      printf("Write XBee default coordinator config\n");
      xbee_setconfig(fd, coord_conf, 7);
   }

   /* ---------------------------------------------------------- *
    * Write the default end device settings                      *
    * ---------------------------------------------------------- */
   if(defaultdevice == 1) {
      printf("Write XBee default enddevice config\n");
      xbee_setconfig(fd, device_conf, 7);
   }

   closeserial(fd);
   return 0;
}
