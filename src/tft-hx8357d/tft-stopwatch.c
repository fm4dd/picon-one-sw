/* ------------------------------------------------------------ *
 * file:        tft-stopwatch.c                                 *
 * purpose:     TFT test program for Adafruit 3.5" TFT on RPi   *
 * return:      0 on success, and -1 on errors.                 *
 * requires:    TFT as framebuffer /dev/fb0, openvg lib from    *
 *              https://github.com/ajstarks/openvg              *
 *              WiringPI lib for pushbutton start/stop control  *
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

int main() {
   int width, height;
   char addr[16];
   char mask[16];
   struct tm *time;                        // standard time struct
   struct timespec tp1, tp2, tp3, tp4;     // nanosec time structs
   long ms;                                // milliseconds

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
   pinMode (21, INPUT);  // SW1 Up
   pinMode (22, INPUT);  // SW2 Mode
   pinMode (23, INPUT);  // SW3 Down
   pinMode (24, INPUT);  // SW4 Enter
   bool runstate = FALSE;
   char statestr[6] = "Stop";
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

      /* ----------------------------------------------------- *
       * Check button press MODE for start action              *
       * ----------------------------------------------------- */
      if((digitalRead(22) == LOW) && (runstate == FALSE)) {
         runstate = TRUE;
         clock_gettime(CLOCK_MONOTONIC_RAW, &tp1);
         snprintf(statestr, sizeof(statestr), "Start");
      }
      /* ----------------------------------------------------- *
       * Check button press ENTER for stop action              *
       * ----------------------------------------------------- */
      if((digitalRead(24) == LOW) && (runstate == TRUE)) {
         runstate = FALSE;
         snprintf(statestr, sizeof(statestr), "Stop");
         tp4.tv_sec = tp3.tv_sec;
         tp4.tv_nsec = tp3.tv_nsec;
      }
      /* ----------------------------------------------------- *
       * Check button press UP for clear action                *
       * ----------------------------------------------------- */
      if((digitalRead(21) == LOW) && (runstate == FALSE)) {
         snprintf(timestr, sizeof(timestr), "00:00:00.000");
         tp4.tv_sec = 0;
         tp4.tv_nsec = 0;
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

         // debug output:
         // printf("tp2 %lu tp1 %lu tp3 %lu ms %lu\n",
         //         tp2.tv_nsec, tp1.tv_nsec, tp3.tv_nsec, ms);
      }

      /* ----------------------------------------------------- *
       * Stopwatch TFT output, showing the elapsing time       *
       * ----------------------------------------------------- */
      Text(20, 160, "StopWatch:", MonoTypeface, 22);
      Text(200, 160, statestr, MonoTypeface, 22);
      Fill(255, 255, 255, 1);              // set foreground white
      Text(20, 120, timestr, MonoTypeface, 22);

      tftbottom(addr, mask);
      End();                               // End the picture
   }
      
   finish();                               // Graphics cleanup
   exit(0);
}
