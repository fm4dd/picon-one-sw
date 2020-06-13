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
//#include <wiringPi.h>
#include "fontinfo.h"
#include "shapes.h"
#include "ip.h"
#include "serial.h"
#include "xbee.h"

/* ------------------------------------------------------------ *
 * global variables                                             *
 * ------------------------------------------------------------ */
char *port   = "/dev/ttySC1";  // serial port device
int speed    = 115200;         // XBee modified speed
int timeout  = 3;              // 3 seconds timeout
int verbose  = 0;              // no verbose output
extern XBee_Info info; 

/* ------------------------------------------------------------ *
 * hexToRGB() converts a Arduino-style hex color to RGB values  *
 * ------------------------------------------------------------ */
void hexToRGB(uint16_t hexValue, uint8_t *r, uint8_t *g, uint8_t *b) {
   *r = (hexValue & 0xF800) >> 11;
   *g = (hexValue & 0x07E0) >> 5;
   *b = hexValue & 0x001F;

   *r = (*r * 255) / 31;
   *g = (*g * 255) / 63;
   *b = (*b * 255) / 31;
}

/* ------------------------------------------------------ *
 * tftheader: outputs upper TFT header 70px from 251..319 *
 * ------------------------------------------------------ */
void tftheader(){
   static char time_str[9];
   static char date_str[9];
   time_t now;
   struct tm *now_tm;
   uint8_t r; 
   uint8_t g;
   uint8_t b;

   StrokeWidth(1);                         // set line size
   Stroke(0, 255, 0, 1);                   // set line color green
   Line(0, 250, 479, 250);                 // draw a separator line
   Image(20, 253, 64, 64, "./images/rpi-logo64.jpg"); // load RPI logo

  /* --------------------------------------------------------- *
   * get system time and write it into the string variables    *
   * --------------------------------------------------------- */
   now = time(0); // Get the system time
   now_tm = localtime(&now);
   strftime(date_str, sizeof(date_str), "%y-%m-%d",now_tm);

   hexToRGB(0x3536, &r, &g, &b);           // use the Arduino 16bit values
   Fill(r, g, b, 1);                       // set foreground blue-ish
   Text(130, 297, "Raspberry", NotoMonoTypeface, 19);
   Text(130, 273, "Pi Zero-W", NotoMonoTypeface, 19);
   Text(320, 290, date_str, MonoTypeface, 22);

   strftime(time_str, sizeof(time_str), "%H:%M:%S",now_tm);

   hexToRGB(0xfce0, &r, &g, &b);           // use Arduino 16bit values
   Fill(r, g, b, 1);                       // set foreground amber
   Text(130, 256, "PiCon One v1.0", NotoMonoTypeface, 12);
   Text(320, 260, time_str, MonoTypeface, 22);
}

/* --------------------------------------------------------- *
 * tftbottom: draw white bottom info bar with WLAN0 IP/Mask  *
 * --------------------------------------------------------- */
void tftbottom(const char *addr, const char *mask){
   Fill(255, 255, 255, 1);                 // set foreground White
   StrokeWidth(0);                         // Set Line size
   Rect(0, 0, 480, 20);                    // from 0.0 size 480x20
   Fill(0, 0, 0, 1);                       // set text color black
   char info_str[50];
   snprintf(info_str, sizeof(info_str), "WLAN0 IP: %s Mask %s", addr, mask);
   Text(2, 2, info_str, MonoTypeface, 13);
}

/* ------------------------------------------------------------ *
 * time_elapsed() returns milliseconds since timespec ref time  *
 * ------------------------------------------------------------ */
unsigned int time_elapsed(struct timespec refts) {
   uint64_t now;
   uint64_t ref;
   struct timespec nowts;
   clock_gettime(CLOCK_MONOTONIC_RAW, &nowts);
   ref=(uint64_t)refts.tv_sec * (uint64_t)1000 + (uint64_t)(refts.tv_nsec / 1000000L);
   now=(uint64_t)nowts.tv_sec * (uint64_t)1000 + (uint64_t)(nowts.tv_nsec / 1000000L);
   return (uint32_t)(now-ref);
}

int main() {
   int width, height;
   char addr[16];
   char mask[16];
   char connect_str[50];
   VGfloat shapecolor[4];
   RGB(255, 125, 125, shapecolor);
   uint8_t r, g, b;
   hexToRGB(0xfce0, &r, &g, &b);           // amber
   char voltage[7];                        // formatted voltage outut string
   uint32_t volt_interval = 300;           // volt refresh interval in milliseconds
   uint32_t ms_elapsed;                    // time since last measurement
   struct timespec refts;                  // reference time for update interval
   char response[8];                       // serial byte response for voltage read

   int fd = xbee_enable(port, speed);
   if(fd != -1) snprintf(connect_str, sizeof(connect_str), "XBee connected %s %dB", port, speed);
   else snprintf(connect_str, sizeof(connect_str), "XBee not connected");
   xbee_getinfo(fd);
   snprintf(voltage, 7, "%.3fV", info.volt);

   /* --------------------------------------------------------- *
    * Setup GPIO pins for button control                        *
    * --------------------------------------------------------- */
//   wiringPiSetup ();
//   pinMode (21, INPUT);  // SW1 Up
//   pinMode (22, INPUT);  // SW2 Mode
//   pinMode (23, INPUT);  // SW3 Down
//   pinMode (24, INPUT);  // SW4 Enter
//   bool runstate = FALSE;
//   char statestr[6] = "Stop";
//   char timestr[22] = "00:00:00.000";

   /* --------------------------------------------------------- *
    * Setup display control. Get IP and Netmask.                *
    * --------------------------------------------------------- */
   init(&width, &height);                  // Graphics init
   getip("wlan0", addr);                   // get wlan0 IP address
   getmask("wlan0", mask);                 // get wlan0 netmask
   Start(width, height);                   // start the picture

   xbee_startcmdmode(fd, 2);

   while(1) {
      /* ------------------------------------------------------ *
       * watch bus voltage in mV with AT%V, and convert 3 bytes *
       * to the display string. volt_interval = the update rate *
       * ------------------------------------------------------ */
      ms_elapsed = time_elapsed(refts);
      if(ms_elapsed >= volt_interval) {
         int ret = xbee_sendcmd(fd, "AT%V\r", response);
         if(ret == 0) {
            // convert string to float
            uint32_t millivolt = strtol(response, NULL, 16);
            info.volt = (float) millivolt / 1000.0;
            snprintf(voltage, 7, "%.3fV", info.volt);
         }
         //printf("Debug: read %f volt %d ms\n", info.volt, ms_elapsed);
         clock_gettime(CLOCK_MONOTONIC_RAW, &refts);
      }
         
      Background(0, 0, 0);                 // set background black
      tftheader();

      Image(10, 170, 460, 66, "./images/xbee-logo66.jpg"); // load XBee logo
      Fill(255, 255, 255, 1);                // set foreground White
      Text(5, 145, connect_str, MonoTypeface, 13);

      StrokeWidth(2);                        // Set Line size
      Stroke(r, g, b, 1);                    // set line color amber
      Fill(0, 0, 0, 1);                      // set foreground black
      Rect(0, 23, 479, 114);               // from 0.0 size 480x20
      StrokeWidth(0);                        // Set Line size
      Fill(r, g, b, 1);                      // set foreground amber
      Rect(4, 114, 135, 20);                 // left Firmware
      Rect(4,  92, 135, 20);                 // left Hardware
      Rect(4,  70, 135, 20);                 // left Node Name
      Rect(4,  48, 135, 20);                 // left MAC
      Rect(4,  26, 135, 20);                 // left Volt

      Rect(380, 114, 95, 20);                 // right
      Rect(380,  92, 95, 20);                 // right
      Rect(380,  70, 95, 20);                 // right
      Rect(380,  48, 95, 20);                 // right
      Rect(380,  26, 95, 20);                 // right

      Fill(0, 0, 0, 1);                      // set foreground black
      Text(12, 118, "Firmware",    MonoTypeface, 13);
      Text(12,  96, "Hardware",    MonoTypeface, 13);
      Text(12,  74, "Node Name",   MonoTypeface, 13);
      Text(12,  52, "MAC Address", MonoTypeface, 13);
      Text(12,  30, "PWR Voltage", MonoTypeface, 13);

      Text(390, 118, "ATVR",    MonoTypeface, 13);
      Text(390,  96, "ATHV",    MonoTypeface, 13);
      Text(390,  74, "ATNI",    MonoTypeface, 13);
      Text(390,  52, "ATDH/DL", MonoTypeface, 13);
      Text(390,  30, "AT%V",    MonoTypeface, 13);
 
      Fill(255, 255, 255, 1);                // set foreground White
      Text(150, 118, info.firmware, MonoTypeface, 13);  // 4 chars
      Text(150,  96, info.hardware, MonoTypeface, 13);  // 4 chars
      Text(150,  74, info.nodeid,   MonoTypeface, 13);  // 20 chars
      Text(150,  52, info.mac,      MonoTypeface, 13);  // 16 chars
      Text(150,  30, voltage,       MonoTypeface, 13);  // 7 chars


      tftbottom(addr, mask);
      End();                               // End the picture
   }
      
   finish();                               // Graphics cleanup
   xbee_endcmdmode(fd, 2);
   closeserial(fd);
   exit(0);
}
