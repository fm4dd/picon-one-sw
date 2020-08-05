/* ------------------------------------------------------------ *
 * file:        tft-stopwatch.c                                 *
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
   struct tm *time;                        // standard time struct
   struct timespec tp1, tp2, tp3, tp4;     // nanosec time structs
   long ms;                                // milliseconds
   uint8_t swstate = 0;                    // button press state

   VGfloat shapecolor[4];
   RGB(255, 125, 125, shapecolor);
   tp3.tv_sec = 0;                         // initialize elapsed time
   tp3.tv_nsec = 0;
   tp4.tv_sec = 0;                         // initialize intermed time
   tp4.tv_nsec = 0;

   /* --------------------------------------------------------- *
    * Setup GPIO pins for button control                        *
    * --------------------------------------------------------- */
   wiringPiSetup ();
   pinMode (SW1_UP,    INPUT);  // SW1 Up
   pinMode (SW2_MODE,  INPUT);  // SW2 Mode
   pinMode (SW3_DOWN,  INPUT);  // SW3 Down
   pinMode (SW4_ENTER, INPUT);  // SW4 Enter
   bool runstate = FALSE;
   char statestr[12] = "Stop";
   char timestr[22] = "00:00:00.000";

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

      if(swstate>0) swstate = 0;
      swstate = sw_detect();

      /* ----------------------------------------------------- *
       * Check button press MODE for start action              *
       * ----------------------------------------------------- */
      if((detect_mode == TRUE) && (runstate == FALSE)) {
            snprintf(statestr, sizeof(statestr), "Start");
            runstate = TRUE; detect_mode = FALSE;
            clock_gettime(CLOCK_MONOTONIC_RAW, &tp1);
      }
      /* ----------------------------------------------------- *
       * Check button press ENTER for stop action              *
       * ----------------------------------------------------- */
      if((detect_enter == TRUE) && (runstate == TRUE)) {
         snprintf(statestr, sizeof(statestr), "Stop");
         runstate = FALSE; detect_enter = FALSE;
         tp4.tv_sec = tp3.tv_sec;
         tp4.tv_nsec = tp3.tv_nsec;
      }
      /* ----------------------------------------------------- *
       * Check button press UP for clear action                *
       * ----------------------------------------------------- */
      if((detect_up == TRUE) && (runstate == FALSE)) {
         snprintf(timestr, sizeof(timestr), "00:00:00.000");
         tp3.tv_sec = 0;  tp4.tv_sec = 0;
         tp3.tv_nsec = 0;  tp4.tv_nsec = 0;
         detect_up = FALSE;
      }
      /* ----------------------------------------------------- *
       * Check button press DOWN for program exit              *
       * ----------------------------------------------------- */
      if((detect_down == TRUE) && (runstate == FALSE)) {
         exit(0);
      }
      /* ----------------------------------------------------- *
       * In runstate, check and display the elapsing time      *
       * ----------------------------------------------------- */
      if(runstate == TRUE) {
         clock_gettime(CLOCK_MONOTONIC_RAW, &tp2);
         if (tp1.tv_nsec > (tp2.tv_nsec + tp4.tv_nsec)) {
            tp3.tv_sec = tp4.tv_sec + tp2.tv_sec - tp1.tv_sec - 1;
            tp3.tv_nsec = (tp4.tv_nsec + tp2.tv_nsec + 1e9) - tp1.tv_nsec;
         }
         else {
            tp3.tv_sec = tp4.tv_sec + tp2.tv_sec - tp1.tv_sec;
            tp3.tv_nsec = tp4.tv_nsec + tp2.tv_nsec - tp1.tv_nsec;
         }
         if(tp3.tv_nsec > 1e9) {
            tp3.tv_nsec = tp3.tv_nsec / 10;
            tp3.tv_sec = tp3.tv_sec + 1;
         }
         time_t tsnow = tp3.tv_sec;
         time = gmtime(&tsnow);
         ms = round(tp3.tv_nsec / 1.0e6); // nanosec to millisec
         if (ms > 999) { tsnow++; ms = 0; }
         snprintf(timestr, sizeof(timestr), "%02d:%02d:%02d.%03ld",
               time->tm_hour, time->tm_min, time->tm_sec, ms);
      }

      /* ----------------------------------------------------- *
       * Stopwatch TFT output, showing the elapsing time       *
       * ----------------------------------------------------- */
      Text(110, 180, "StopWatch:", MonoTypeface, 22);
      Text(290, 180, statestr, MonoTypeface, 22);
      Fill(255, 255, 255, 1);              // set foreground white
      Text(130, 140, timestr, MonoTypeface, 22);

      tftaction(swstate);
      tftbottom(addr, mask);
      End();                               // End the picture
   }
      
   finish();                               // Graphics cleanup
   exit(0);
}
