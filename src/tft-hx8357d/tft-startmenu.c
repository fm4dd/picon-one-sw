/* ------------------------------------------------------------ *
 * file:        tft-startmenu.c                                 *
 * purpose:     TFT test program for Adafruit 3.5" TFT on RPi   *
 * return:      0 on success, and -1 on errors.                 *
 * requires:    TFT as framebuffer /dev/fb0, openvg lib from    *
 *              https://github.com/ajstarks/openvg              *
 *              WiringPI lib for pushbutton start/stop control  *
 *              libjpeg, e.g. sudo apt-get install libjpeg-dev  *
 * author:      05/23/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <signal.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include <stdbool.h>
#include <wiringPi.h>
#include "fontinfo.h"
#include "shapes.h"
#include "ip.h"
#include "tft-shared.h"

int main() {
   int width, height;
   char addr[16];
   char mask[16];
   VGfloat shapecolor[4];
   RGB(255, 125, 125, shapecolor);
   uint8_t r, g, b;
   uint32_t ms_elapsed = 0;                // time since last measurement
   struct timespec refts;                  // reference time for update interval
   uint32_t swi_interval = 200;            // sw check interval in milliseconds
   uint8_t swstate = 0;                    // button press state

   /* --------------------------------------------------------- *
    * Setup sleep time and display control                      *
    * --------------------------------------------------------- */
   struct timespec sleep;
   sleep.tv_sec = 1;                       // set sleep time
   //sleep.tv_nsec = 500000000;            // to 0.5 seconds

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
      Background(0, 0, 0);                 // set background black
      tftheader();
      ms_elapsed = time_elapsed(refts);
      if(ms_elapsed >= swi_interval) {

         if(statestr[0] != 'W') snprintf(statestr, sizeof(statestr), "WAIT-4-KEY");
         if(swstate>0) swstate = 0;
         swstate = sw_detect();
         if(detect_down == TRUE) {
            if(prgsel < 5) prgsel++;
            else prgsel = 0;
            detect_down = FALSE;
         }
         if(detect_up == TRUE) {
            if(prgsel > 0) prgsel--;
            else prgsel = 5;
            detect_up = FALSE;
         }
         if(detect_mode == TRUE) detect_mode = FALSE;

         if(detect_enter == TRUE) {
            detect_enter = FALSE;
            switch(prgsel) {
               case 0: break; // select 0 frame
               case 1: system("/home/pi/picon-one-sw/src/tft-hx8357d/tft-stopwatch");
                       break; // select 1 frame
               case 2: system("/home/pi/picon-one-sw/src/tft-hx8357d/tft-tempgraph");
                       break; // select 2 frame
               case 3: system("/home/pi/picon-one-sw/src/xbee-module/tft-xbee-info");
                       break; // select 3 frame
               //case 4: system("/usr/bin/sudo - TERM=vt100 /usr/bin/gpsmon <> /dev/tty1 >&0");
               case 4: End();finish();
                       system("/home/pi/picon-one-sw/src/tft-hx8357d/down_btn_ends_gpsmon.sh &");
                       system("/usr/bin/sudo TERM=vt100 /bin/sh -c '/usr/bin/gpsmon <> /dev/tty1 >&0'");
                       init(&width, &height); Start(width, height);                   // start the picture
                       break; // select 4 frame
               case 5: End();finish();
                       system("/usr/bin/sudo /home/pi/picon-one-sw/src/tft-hx8357d/system_shutdown.sh &");
                       exit(0); // select 5 frame
            }
         }
         //if(detect_enter == TRUE) detect_enter = FALSE;
         //printf("Debug: %d ms prgsel %d\n", ms_elapsed, prgsel);
         clock_gettime(CLOCK_MONOTONIC_RAW, &refts); // reset reftime
      }

      /* ----------------------------------------------------- *
       * TFT menu output                                       *
       * ----------------------------------------------------- */
      StrokeWidth(4);                        // Set Line size
      hexToRGB(0x3536, &r, &g, &b);          // use blueish
      Stroke(r, g, b, 1);                    // set line color
      Fill(0, 0, 0, 1);                      // set foreground black

      switch(prgsel) {
         case 0: Rect(  2, 176, 156, 52); break; // select 0 frame
         case 1: Rect(  2, 116, 156, 52); break; // select 1 frame
         case 2: Rect(162, 176, 156, 52); break; // select 2 frame
         case 3: Rect(162, 116, 156, 52); break; // select 3 frame
         case 4: Rect(322, 176, 156, 52); break; // select 4 frame
         case 5: Rect(322, 116, 156, 52); break; // select 5 frame
        default: Rect(  2, 176, 156, 52); break; // select 0 frame
      }

      StrokeWidth(0);                        // Set Line size
      hexToRGB(0xfce0, &r, &g, &b);          // amber
      Fill(r, g, b, 1);                      // set foreground amber
      Rect(  8, 182, 144, 40);               // select 0 box
      Rect(  8, 122, 144, 40);               // select 1 box
      Rect(168, 182, 144, 40);               // select 2 box
      Rect(168, 122, 144, 40);               // select 3 box
      Rect(328, 182, 144, 40);               // select 4 box
      Rect(328, 122, 144, 40);               // select 5 box

      Fill(0, 0, 0, 1);                      // set foreground black
      Text(16,  194, statestr, MonoTypeface, 16);    // 0 box text
      Text(16,  134, "Stopwatch", MonoTypeface, 16); // 1 box text
      Text(176, 194, "Tempgraph", MonoTypeface, 16); // 2 box text
      Text(176, 134, "XBee Info", MonoTypeface, 16); // 3 box text
      Text(336, 194, "GNSS Info", MonoTypeface, 16); // 4 box text
      Text(336, 134, "Shutdown", MonoTypeface, 16);  // 5 box text

      tftaction(swstate);
      tftbottom(addr, mask);
      End();                               // End the picture
      nanosleep(&sleep, NULL);             // sleep 0.1 seconds
   }
   finish();                               // Graphics cleanup
   exit(0);
}
