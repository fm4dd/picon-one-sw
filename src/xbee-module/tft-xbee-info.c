/* ------------------------------------------------------------ *
 * file:        tft-xbee-info.c                                 *
 * purpose:     TFT test program for Digi XBee modules on RPi   *
 * return:      0 on success, and -1 on errors.                 *
 * requires:    TFT as framebuffer /dev/fb0, openvg headers     *
 *                                                              *
 * author:      05/23/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include <stdbool.h>
#include <wiringPi.h>
#include "fontinfo.h"
#include "shapes.h"
#include "ip.h"
#include "serial.h"
#include "xbee.h"
#include "tft-shared.h"

#define XBEELOGO_PATH "/home/pi/picon-one-sw/src/xbee-module/images/xbee-logo66.jpg"
/* ------------------------------------------------------------ *
 * global variables                                             *
 * ------------------------------------------------------------ */
char *port   = "/dev/ttySC1";  // serial port device
int speed    = 115200;         // XBee modified speed
int timeout  = 3;              // 3 seconds timeout
int verbose  = 0;              // no verbose output
extern XBee_Info info; 

int main() {
   int width, height;
   char addr[16];
   char mask[16];
   char connect_str[50];
   VGfloat shapecolor[4];
   RGB(255, 125, 125, shapecolor);
   uint8_t r, g, b;
   uint8_t prgstat = 0;                    // program status: 0=query, 1=result
   hexToRGB(0xfce0, &r, &g, &b);           // amber
   char voltage[7];                        // formatted voltage outut string
   uint32_t volt_interval = 300;           // volt refresh interval in milliseconds
   uint32_t ms_elapsed;                    // time since last measurement
   struct timespec refts;                  // reference time for update interval
   char response[8];                       // serial byte response for voltage read
   int fd;
   uint8_t swstate = 0;                    // button press status
   bool runstate = FALSE;
   char outstr[31];
   uint8_t i = 0;

   /* --------------------------------------------------------- *
    * Setup GPIO pins for button control                        *
    * --------------------------------------------------------- */
   wiringPiSetup ();
   pinMode (SW1_UP,    INPUT);  // SW1 Up
   pinMode (SW2_MODE,  INPUT);  // SW2 Mode
   pinMode (SW3_DOWN,  INPUT);  // SW3 Down
   pinMode (SW4_ENTER, INPUT);  // SW4 Enter

   /* --------------------------------------------------------- *
    * Setup display control. Get IP and Netmask.                *
    * --------------------------------------------------------- */
   init(&width, &height);                  // Graphics init
   getip("wlan0", addr);                   // get wlan0 IP address
   getmask("wlan0", mask);                 // get wlan0 netmask
   Start(width, height);                   // start the picture

   while(1) {
      Background(0, 0, 0);                   // black background
      tftheader();                           // display header

      if(swstate>0) swstate = 0;
      swstate = sw_detect();

      /* ----------------------------------------------------- *
       * Check button press MODE for start action              *
       * ----------------------------------------------------- */
      if((detect_mode == TRUE) && (runstate == FALSE)) {
            runstate = TRUE; detect_mode = FALSE;
      }
      /* ----------------------------------------------------- *
       * Check button press ENTER for stop action              *
       * ----------------------------------------------------- */
      if((detect_enter == TRUE) && (runstate == TRUE)) {
         runstate = FALSE; detect_enter = FALSE;
      }
      /* ----------------------------------------------------- *
       * Check button press DOWN for program exit              *
       * ----------------------------------------------------- */
      if(detect_down == TRUE) exit(0);

      if(prgstat == 0) {
         Image(10, 170, 460, 66, XBEELOGO_PATH);// load XBee logo
         snprintf(connect_str, sizeof(connect_str), "Connecting to %s %dB", port, speed);
         Fill(255, 255, 255, 1);                // set foreground White
         Text(5, 145, connect_str, MonoTypeface, 13);

         prgstat = 1;
      }
      else if(prgstat == 1) {
         Image(10, 170, 460, 66, XBEELOGO_PATH);// load XBee logo
         fd = xbee_enable(port, speed);
         if(fd != -1) snprintf(connect_str, sizeof(connect_str), "Connecting to %s %dB ... OK.", port, speed);
         else snprintf(connect_str, sizeof(connect_str), "XBee not connected");


         Fill(255, 255, 255, 1);                // set foreground White
         Text(5, 145, connect_str, MonoTypeface, 13);

         xbee_getinfo(fd);
         snprintf(voltage, 7, "%.3fV", info.volt);
         prgstat = 2;
      } else {
      /* ------------------------------------------------------ *
       * watch bus voltage in mV with AT%V, and convert 3 bytes *
       * to the display string. volt_interval = the update rate *
       * ------------------------------------------------------ */
      ms_elapsed = time_elapsed(refts);
      if(ms_elapsed >= volt_interval) {
         xbee_startcmdmode(fd, 2);
         int ret = xbee_sendcmd(fd, "AT%V\r", response);
         if(ret == 0) {
            // convert string to float
            uint32_t millivolt = strtol(response, NULL, 16);
            info.volt = (float) millivolt / 1000.0;
            snprintf(voltage, 7, "%.3fV", info.volt);
            xbee_endcmdmode(fd, 2);
         }
         //printf("Debug: read %f volt %d ms\n", info.volt, ms_elapsed);
         clock_gettime(CLOCK_MONOTONIC_RAW, &refts);
         
         if(runstate == TRUE) {
            snprintf(outstr, sizeof(outstr), "%d Picon-One Hello World\r", i);
            strserial(fd, outstr);
            snprintf(connect_str, sizeof(connect_str), "Transmit: %s", outstr);
            i++;
            if(i>99) i = 0;
         }
         else snprintf(connect_str, sizeof(connect_str), "XBee connected");
      }

      Fill(255, 255, 255, 1);                // set foreground White
      Text(5, 224, connect_str, MonoTypeface, 13);

      StrokeWidth(2);                        // Set Line size
      Stroke(r, g, b, 1);                    // set line color amber
      Fill(0, 0, 0, 1);                      // set foreground black
      Rect(0, 103, 479, 114);               // from 0.0 size 480x20
      StrokeWidth(0);                        // Set Line size
      Fill(r, g, b, 1);                      // set foreground amber
      Rect(4, 194, 135, 20);                 // left Firmware
      Rect(4, 172, 135, 20);                 // left Hardware
      Rect(4, 150, 135, 20);                 // left Node Name
      Rect(4, 128, 135, 20);                 // left MAC
      Rect(4, 106, 135, 20);                 // left Volt

      Rect(380, 194, 95, 20);                 // right
      Rect(380, 172, 95, 20);                 // right
      Rect(380, 150, 95, 20);                 // right
      Rect(380, 128, 95, 20);                 // right
      Rect(380, 106, 95, 20);                 // right

      Fill(0, 0, 0, 1);                      // set foreground black
      Text(12, 198, "Firmware",    MonoTypeface, 13);
      Text(12, 176, "Hardware",    MonoTypeface, 13);
      Text(12, 154, "Node Name",   MonoTypeface, 13);
      Text(12, 132, "MAC Address", MonoTypeface, 13);
      Text(12, 110, "PWR Voltage", MonoTypeface, 13);

      Text(390, 198, "ATVR",    MonoTypeface, 13);
      Text(390, 176, "ATHV",    MonoTypeface, 13);
      Text(390, 154, "ATNI",    MonoTypeface, 13);
      Text(390, 132, "ATDH/DL", MonoTypeface, 13);
      Text(390, 110, "AT%V",    MonoTypeface, 13);
 
      Fill(255, 255, 255, 1);                // set foreground White
      Text(150, 198, info.firmware, MonoTypeface, 13);  // 4 chars
      Text(150, 176, info.hardware, MonoTypeface, 13);  // 4 chars
      Text(150, 154, info.nodeid,   MonoTypeface, 13);  // 20 chars
      Text(150, 132, info.mac,      MonoTypeface, 13);  // 16 chars
      Text(150, 110, voltage,       MonoTypeface, 13);  // 7 chars
      }
      tftaction(swstate);
      tftbottom(addr, mask);
      End();                               // End the picture
   }
      
   finish();                               // Graphics cleanup
   closeserial(fd);
   exit(0);
}
