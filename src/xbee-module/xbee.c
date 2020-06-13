/* ------------------------------------------------------------ *
 * file:        xbee.c                                          *
 * purpose:     XBEE configuration and communication functions  *
 *              On PiCon One, XBEE connects to /dev/ttySC1:     *
 *                                                              *
 *              rpi0w <-- ttySC1 RX ---\/--- RX XBEE pin-3 DIN  *
 *              rpi0w --> ttySC1 TX ---/\--- TX XBEE pin-2 DOUT *
 *                                                              *
 * compile:     see Makefile                                    *
 * author:      06/10/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>        // exit()
#include <signal.h>
#include <unistd.h>
#include <stdint.h>        // uint8_t data types
#include <string.h>
#include <strings.h>
#include <time.h>
#include "serial.h"
#include "xbee.h"

extern int verbose;        // verbose set in the main prog
extern char *port;         // port is set in the main prog
XBee_Info info;            // XBee device information
XBee_Status status;        // XBee device status

// Coordinator configuration (PiCon One default)
const char *coord_conf[7] = {
   "ATID24\r",           // PAN ID = 24
   "ATCE1\r",            // Enable Coordinator role
   "ATJV0\r",            // Dont try to join NW
   "ATDH0\r",            // Dest addr high & low = 0/FFFF
   "ATDLFFFF\r",         // Broadcast to all clients
   "ATNISPiCon-One\r",   // Device name
   "ATAP0\r" };          // Set transparent mode
// End Coordinator configuration

// Device configuration (PiCon One default)
const char *device_conf[7] = {
   "ATID24",             // PAN ID = 24
   "ATCE0",              // Disable Coordinator role
   "ATJV1",              // Join network at boot
   "ATDH0",              // Dest addr high & low = 0/0
   "ATDL0",              // we only talk to coordinator
   "ATNIS2R4-node1",     // Device name
   "ATAP0" };            // Set transparent mode
// End Device configuration

/* ---------------------------------------------------- * 
 * xbee_enable() connects the XBee to the given serial  * 
 * port. Returns the fd for success, -1 for errors      * 
 * ---------------------------------------------------- */
int xbee_enable(char *port, int speed) {
   int fd;

   /* ------------------------------------------------- *
    * open serial port                                  *
    * ------------------------------------------------- */
   if((fd=getserial(port, speed)) < 0) {
      printf("Error opening port %s %d Baud\n", port, speed);
      return -1;
   }
   if(verbose == 1) printf("Debug: %s %d Baud connected\n", port, speed);

   /* ------------------------------------------------- * 
    * Test Xbee S2C module by sending break signal +++  * 
    * ------------------------------------------------- */
   if(xbee_startcmdmode(fd, timeout) == -1) return -1;
   if(xbee_endcmdmode(fd, timeout) == -1)   return -1;
   return fd;
} // end xbee_enable()

/* ---------------------------------------------------- * 
 * getinfo() gets XBee S2C module HW information incl.  * 
 * MAC, firmware version, hardware model, bus voltage   * 
 * returns 0 for success, -1 for errors.                * 
 * ---------------------------------------------------- */
int xbee_getinfo(int fd) {
   char response[512];
   int ret;

   /* ------------------------------------------------- * 
    * Enter CMD mode                                    * 
    * ------------------------------------------------- */
   if(xbee_startcmdmode(fd, timeout) == -1) return -1;

   /* ------------------------------------------------- * 
    * Get firmware version with ATVR                    * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATVR\r", response);
   if(ret == -1) return -1; // exit with failure code
   strncpy(info.firmware, response, sizeof(info.firmware));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- */
   /* Get hardware version with ATHV, returns 4 bytes   */
   /* ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATHV\r", response);
   if(ret == -1) return -1; // exit with failure code
   strncpy(info.hardware, response, sizeof(info.hardware));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get node identifier with ATNI, may return 0 bytes * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATNI\r", response);
   if(ret == -1) return -1; // exit with failure code
   strncpy(info.nodeid, response, sizeof(info.nodeid));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get MAC address with ATSH and ATSL, ret 6/8 bytes * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATSH\r", response);
   if(ret == -1) return -1; // exit with failure code

   // assign response to MAC string, e.g. 0013A200417D5111
   char mac[17];
   uint8_t leadzeros = 8 - strlen(response);
   while(leadzeros > 0) {
     leadzeros--;
     mac[leadzeros] = '0';
   }
   strcat(mac, response);
   if(verbose == 1) printf("Debug: MAC %s\n", mac);
   memset(response,0,sizeof(response));

   ret = xbee_sendcmd(fd, "ATSL\r", response);
   if(ret == -1) return -1; // exit with failure code
   // add response to MAC string e.g. 0013A200417D5111
   leadzeros = 8 - strlen(response);
   while(leadzeros > 0) {
     leadzeros--;
     mac[leadzeros] = '0';
   }
   strcat(mac, response);
   strncpy(info.mac, mac, sizeof(info.mac));
   if(verbose == 1) printf("Debug: MAC %s\n", mac);
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get bus voltage in mV with AT%V, returns 3 bytes  * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "AT%V\r", response);
   if(ret == -1) return -1; // exit with failure code

   // convert string to float
   uint32_t millivolt = strtol(response, NULL, 16);
   info.volt = (float) millivolt / 1000.0;
   if(verbose == 1) printf("Debug: Convert Volt: %.3f\n", info.volt);

   /* ------------------------------------------------- * 
    * Finished data collection, end command mode        * 
    * ------------------------------------------------- */
   if(xbee_endcmdmode(fd, timeout) == -1)   return -1;
   return 0;                               // return success
} // end xbee_getinfo()

/* ---------------------------------------------------- * 
 * sendstring() sends a string over / to the XBee radio * 
 * args: String to send, flag to send AT cmds to module * 
 * ---------------------------------------------------- */
int xbee_sendstring(int fd, const char * sendstr) {
   uint8_t sendbytes = strlen(sendstr);
   if(verbose == 1) {
      printf("Debug: send %s (%d bytes)", sendstr, sendbytes);
   }
   strserial(fd, sendstr);
   flushserial(fd);
   return 0;
}

/* ---------------------------------------------------- * 
 * xbee_startcmdmode() enters command mode to send AT   * 
 * cmds. Returns 0 on success, -1 for errors.           * 
 * ---------------------------------------------------- */
int xbee_startcmdmode(int fd, int timeout) {
   int cycles = timeout * 10; // check interval is 100ms
   int i = 0; int wait = 0;
   char response[512];

   /* ------------------------------------------------- *
    * wait guard time, send cmd char sequence (cc): +++ *
    * ------------------------------------------------- */
   sleep(1);              // wait guard time 1 second
   strserial(fd, "+++");
   if(verbose == 1) printf("Debug: %s send +++\n", port);

   /* ------------------------------------------------- *
    * wait guard time, check for the 3 bytes response   *
    * ------------------------------------------------- */
   while((wait = checkserial(fd)) == 0) {
      usleep(100000);     // wait 100ms
      if(i>cycles) break; // stop waiting after timeout s
      i++;
   }
   if(verbose == 1) printf("Debug: %s got %d bytes after %d ms\n", port, wait, i*100);
   /* ------------------------------------------------- *
    * If no response we have XBee communication failure *
    * Either no XBee connected, or has different speed. *
    * ------------------------------------------------- */
   if(wait == 0) {
      printf("Error: No XBee responding\n");
      return -1;
   }

   /* ------------------------------------------------ *
    * retrieve reply and save it into response string  *
    * ------------------------------------------------ */
   i = 0;
   for (i=0; i<1024; i++) {
      if(checkserial(fd)>0) response[i] = getcharserial(fd);
      else break;
   }
   /* ------------------------------------------------- *
    * Remove '\r' carriage return in response last char *
    * ------------------------------------------------- */
   i--;                                        // back to last char
   if(response[i] == '\r') response[i] = '\0'; // eliminate '\r'

   /* ------------------------------------------------- *
    * Confirm response string == "OK"                   *
    * ------------------------------------------------- */
   if(verbose == 1) printf("Debug: port %s reply: %s (%d bytes)\n", port, response, i);
   if(strcmp(response, "OK") != 0) {
      if(verbose == 1) printf("Debug: XBee CMD mode start failed.\n");
      return -1;       // exit with failure code
   }
   if(verbose == 1) printf("Debug: XBee CMD mode start complete.\n");
   return 0;
}

/* ---------------------------------------------------- */
/* endcmdmode() close command mode after sending AT cmd */
/* returns 0 for success, -1 for errors.                */
/* ---------------------------------------------------- */
int xbee_endcmdmode(int fd, int timeout) {
   int cycles = timeout * 10; // check interval is 100ms
   int i = 0; int wait = 0;
   char response[512];

   /* ------------------------------------------------- * 
    * Send ATCN command to leave CMD mode               * 
    * ------------------------------------------------- */
   strserial(fd, "ATCN\r");
   wait = checkserial(fd);
   if(verbose == 1) printf("Debug: %s send ATCN\\r\n", port);

   /* ------------------------------------------------- *
    * Check if response is received, wait up to timeout *
    * ------------------------------------------------- */
   i = 0;
   while((wait = checkserial(fd)) == 0) {
      usleep(100000);     // wait 100ms
      if(i>cycles) break; // stop waiting after 3 seconds
      i++;
   }
   if(verbose == 1) printf("Debug: %s got %d bytes after %d ms\n", port, wait, i*100);
   /* ------------------------------------------------- *
    * If no response, its a XBee communication failure  *
    * Either no XBee connected, or on different speed.  *
    * ------------------------------------------------- */
   if(wait == 0) {
      printf("Error: No XBee response received\n");
      return -1;
   }

   /* ------------------------------------------------- *
    * retrieve the reply, save it into response string  *
    * ------------------------------------------------- */
   for (i=0; i<1024; i++) {
      if(checkserial(fd)>0) response[i] = getcharserial(fd);
      else break;
   }

   /* ------------------------------------------------- *
    * Remove '\r' carriage return from last char        *
    * ------------------------------------------------- */
   i--;                                        // back to last char
   if(response[i] == '\r') response[i] = '\0'; // eliminate '\r'

   /* ------------------------------------------------- *
    * Confirm response string == "OK"                   *
    * ------------------------------------------------- */
   if(verbose == 1) printf("Debug: port %s reply: %s (%d bytes)\n", port, response, i);
   if(strcmp(response, "OK") != 0) {
      if(verbose == 1) printf("Debug: XBee CMD ATCN failed.\n");
      return -1;       // exit with failure code
   }
   if(verbose == 1) printf("Debug: XBee CMD mode finished.\n");
   return 0;
}

/* ---------------------------------------------------- * 
 * xbee_recvstring() receives a string over XBee radio  * 
 * returns 0 for success, -1 for errors                 * 
 * ---------------------------------------------------- */
int xbee_recvstring(int fd, char *received) {
   int i;
   while(0) {                            // wait for serial
      if(checkserial(fd)>0) break;
      usleep(100000);
   }

   /* ------------------------------------------------- *
    * retrieve the reply, save it into response string  *
    * ------------------------------------------------- */
   for (i=0; i<1024; i++) {
      if(checkserial(fd)>0) received[i] = getcharserial(fd);
      else break;
   }

   /* ------------------------------------------------- *
    * Remove '\r' carriage return from last char        *
    * ------------------------------------------------- */
   i--;                                        // back to last char
   if(received[i] == '\r') received[i] = '\0'; // eliminate '\r'
   if(verbose == 1) printf("Debug: Data recv %s (%d bytes)\n", received, i);
   return 0;       // exit with success
}

/* ---------------------------------------------------- * 
 * xbee_setconfig() configures XBee as device or coord. * 
 * args: array of config commands and # config commands * 
 * returns 0 for success, -1 for errors                 * 
 * ---------------------------------------------------- */
int xbee_setconfig(int fd, const char **conf, uint8_t entries) {
   char response[512];
   int ret;

   /* ------------------------------------------------- *
    * Enter CMD mode                                    *
    * ------------------------------------------------- */
   if(xbee_startcmdmode(fd, timeout) == -1) return -1;

   while(entries){
      /* ---------------------------------------------- *
       *  Send the next config command                  *
       * ---------------------------------------------- */
      ret = xbee_sendcmd(fd, conf[entries-1], response);
      if(ret == -1) return -1; // exit with failure code
      /* ---------------------------------------------- *
       * Confirm response string == "OK"                *
       * ---------------------------------------------- */
      if(strcmp(response, "OK") != 0) return -1;
      memset(response,0,sizeof(response));
      entries--;
   }

   /* ------------------------------------------------- *
    *  ATWR writes the updated config to flash          *
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATWR\r", response);
   if(ret == -1) return -1; // exit with failure code
   /* ------------------------------------------------- *
    * Confirm response string == "OK"                   *
    * ------------------------------------------------- */
   if(strcmp(response, "OK") != 0) return -1;
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- *
    *  ATAC applies the config changes immediately      *
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATAC\r", response);
   if(ret == -1) return -1; // exit with failure code
   /* ------------------------------------------------- *
    * Confirm response string == "OK"                   *
    * ------------------------------------------------- */
   if(strcmp(response, "OK") != 0) return -1;

   if(xbee_endcmdmode(fd, timeout) == -1) return -1;
   return 0;
}

/* ---------------------------------------------------- * 
 * getstatus() gets XBee S2C module read-only status,   * 
 * incl. association, signal strength, free device, etc * 
 * returns 0 for success, -1 for errors                 * 
 * ---------------------------------------------------- */
int xbee_getstatus(int fd) {
   char response[1024];
   int ret;

   /* ------------------------------------------------- *
    * Enter CMD mode                                    *
    * ------------------------------------------------- */
   if(xbee_startcmdmode(fd, timeout) == -1) return -1;

   /* ------------------------------------------------- * 
    * Get device free with ATNC, returns 1 byte 0..14   * 
    * Remaining num of devices the coordinator supports *
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATNC\r", response);
   if(ret == -1) return -1;          // exit with failure code
   strncpy(status.device_free, response, sizeof(status.device_free));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get association with ATAI, returns 1 byte, 0 = OK * 
    * if non-zero return, it will be error codes in hex * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATAI\r", response);
   if(ret == -1) return -1;          // exit with failure code
   strncpy(status.association, response, sizeof(status.association));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get operational PAN ID with ATOP, returns 8 bytes * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATOP\r", response);
   if(ret == -1) return -1; // exit with failure code
   strncpy(status.oper_panid, response, sizeof(status.oper_panid));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get operational channel with ATCH returns 2 bytes * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATCH\r", response);
   if(ret == -1) return -1; // exit with failure code
   strncpy(status.oper_chan, response, sizeof(status.oper_chan));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get signal strengh with ATDB returns 1 byte 0..FF * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATDB\r", response);
   if(ret == -1) return -1;        // exit with failure code
   strncpy(status.last_rssi, response, sizeof(status.last_rssi));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get power level 4 with ATPP, returns 1 byte 0..FF * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATPP\r", response);
   if(ret == -1) return -1;        // exit with failure code
   strncpy(status.pwr_level, response, sizeof(status.pwr_level));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * max bytes for unicasts with ATNP, returns 1 byte  * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATNP\r", response);
   if(ret == -1) return -1;          // exit with failure code
   strncpy(status.max_packets, response, sizeof(status.max_packets));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get network address with ATMY, returns 2 bytes    * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATMY\r", response);
   if(ret == -1) return -1;       // exit with failure code
   strncpy(status.nw_address, response, sizeof(status.nw_address));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Get parent nw address with ATMP, returns 2 bytes  * 
    * ------------------------------------------------- */
   ret = xbee_sendcmd(fd, "ATMP\r", response);
   if(ret == -1) return -1;       // exit with failure code
   strncpy(status.pt_address, response, sizeof(status.pt_address));
   memset(response,0,sizeof(response));

   /* ------------------------------------------------- * 
    * Write current time timestamp to last_update       *
    * ------------------------------------------------- */
    status.last_update = (uint32_t) time(NULL);

   /* ------------------------------------------------- * 
    * Finished data collection, end command mode        * 
    * ------------------------------------------------- */
   if(xbee_endcmdmode(fd, timeout) == -1)   return -1;
   return 0;                               // return success
} // end getstatus()

/* ---------------------------------------------------- * 
 * xbee_sendcmd() sends an AT command to the XBee, and  * 
 * writes the reply into the response string.           * 
 * Returns true for success, false for errors           * 
 * ---------------------------------------------------- */
int xbee_sendcmd(int fd, const char *cmd, char *response) {
   int cycles = timeout * 10; // check interval is 100ms
   int i = 0; int wait = 0;

   /* ------------------------------------------------- *
    * Send CMD to XBee                                  *
    * ------------------------------------------------- */
   if(verbose == 1) printf("Debug: send CMD %s\n", cmd);
   strserial(fd, cmd);
   wait = checkserial(fd);

   /* ------------------------------------------------- *
    * Check if response is received, wait up to timeout *
    * ------------------------------------------------- */
   i = 0;
   while((wait = checkserial(fd)) == 0) {
      usleep(100000);     // wait 100ms
      if(i>cycles) break; // stop waiting after timeout
      i++;
   }
   if(verbose == 1) printf("Debug: got %d bytes after %d ms\n", wait, i*100);
   /* ------------------------------------------------- *
    * If no response, its a XBee communication failure  *
    * Either no XBee connected, or on different speed.  *
    * ------------------------------------------------- */
   if(wait == 0) {
      printf("Error: No XBee response received\n");
      return -1;
   }

   /* ------------------------------------------------- *
    * retrieve the reply, save it into response string  *
    * ------------------------------------------------- */
   for (i=0; i<1024; i++) {
      if(checkserial(fd)>0) response[i] = getcharserial(fd);
      else break;
   }

   /* ------------------------------------------------- *
    * Remove '\r' carriage return from last char        *
    * ------------------------------------------------- */
   i--;                                        // back to last char
   if(response[i] == '\r') response[i] = '\0'; // eliminate '\r'
   if(verbose == 1) printf("Debug: reply %s (%d bytes)\n", response, i);
   return 0;       // exit with success
}
