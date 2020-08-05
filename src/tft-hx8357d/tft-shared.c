/* ------------------------------------------------------------ *
 * file:        tft-shared.c                                    *
 * purpose:  	shared functions for TFT output                 *
 * author:      07/23/2020 Frank4DD                             *
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

#define SW1_UP		21
#define SW2_MODE	22
#define SW3_DOWN	23
#define SW4_ENTER	24

#define RPILOGO         "/home/pi/picon-one-sw/src/tft-hx8357d/images/rpi-logo64.jpg"

extern bool detect_up;
extern bool detect_mode;
extern bool detect_down;
extern bool detect_enter;
extern char statestr[12];
extern int  prgsel;

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
   Image(20, 253, 64, 64, RPILOGO);        // load RPI logo

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
 * tftaction: draw a activity screen based on button press.  *
 * --------------------------------------------------------- */
void tftaction(int action){
      uint8_t r, g, b;
      StrokeWidth(1);                      // set line size
      hexToRGB(0xfce0, &r, &g, &b);        // amber
      Fill(r, g, b, 1);                    // set foreground
      Stroke(r, g, b, 1);                  // set line color
      Circle(220, 82, 18); CircleOutline(220, 82, 24);
      Circle(220, 46, 18); CircleOutline(220, 46, 24);
      Circle(260, 82, 18); CircleOutline(260, 82, 24);
      Circle(260, 46, 18); CircleOutline(260, 46, 24);

     // action  = 1;
      switch(action) {
         case 1:
            Fill(0, 255, 0, 1);                    // set foreground
            Stroke(0, 255, 0, 1);                  // set line color
            Circle(220, 82, 18); CircleOutline(220, 82, 24);
            break;
         case 2:
            Fill(0, 255, 0, 1);                    // set foreground
            Stroke(0, 255, 0, 1);                  // set line color
            Circle(260, 82, 18); CircleOutline(260, 82, 24);
            break;
         case 3:
            Fill(0, 255, 0, 1);                    // set foreground
            Stroke(0, 255, 0, 1);                  // set line color
            Circle(220, 46, 18); CircleOutline(220, 46, 24);
            break;
         case 4:
            Fill(0, 255, 0, 1);                    // set foreground
            Stroke(0, 255, 0, 1);                  // set line color
            Circle(260, 46, 18); CircleOutline(260, 46, 24);
            break;
      }

      hexToRGB(0x3536, &r, &g, &b);
      Fill(r, g, b, 1);              // set foreground blueish
      Text(88,   76, "RESET = Up", MonoTypeface, 14);    // button 1
      Text(75,   38, "EXIT = Down", MonoTypeface, 14);   // button 2
      Text(280,   76, "Mode = START", MonoTypeface, 14); // button 3
      Text(280,   38, "Enter = STOP", MonoTypeface, 14); // button 4
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

/* --------------------------------------------------------- *
 * sw_detect: detects button press and set button flag       *
 * --------------------------------------------------------- */
uint8_t sw_detect() {
   /* ----------------------------------------------------- *
    * Check button press UP                                 *
    * ----------------------------------------------------- */
   if((digitalRead(SW1_UP) == LOW) && (detect_up == FALSE)) {
      detect_up = TRUE;
      snprintf(statestr, sizeof(statestr), "UP");
      return 1;
   }
   /* ----------------------------------------------------- *
    * Check button press MODE                               *
    * ----------------------------------------------------- */
   if((digitalRead(SW2_MODE) == LOW) && (detect_mode == FALSE)) {
      detect_mode = TRUE;
      snprintf(statestr, sizeof(statestr), "MODE");
      return 2;
   }
   /* ----------------------------------------------------- *
    * Check button press DOWN                               *
    * ----------------------------------------------------- */
   if((digitalRead(SW3_DOWN) == LOW) && (detect_down == FALSE)) {
      detect_down = TRUE;
      snprintf(statestr, sizeof(statestr), "DOWN");
      return 3;
   }
   /* ----------------------------------------------------- *
    * Check button press ENTER                              *
    * ----------------------------------------------------- */
   if((digitalRead(SW4_ENTER) == LOW) && (detect_enter == FALSE)) {
      detect_enter = TRUE;
      snprintf(statestr, sizeof(statestr), "ENTER %d", prgsel);
      return 4;
   }
   return 0;
}

/* ------------------------------------------------------------ *
 * time_elapsed() returns milliseconds since timespec ref time  *
 * ------------------------------------------------------------ */
uint32_t time_elapsed(struct timespec ref) {
   struct timespec now, elapsed;

   clock_gettime(CLOCK_MONOTONIC_RAW, &now); // get latest time
   if ((now.tv_nsec - ref.tv_nsec) < 0) {
      elapsed.tv_sec = now.tv_sec - ref.tv_sec - 1;
      elapsed.tv_nsec = now.tv_nsec - ref.tv_nsec + 1000000000;
   } 
   else {
      elapsed.tv_sec = now.tv_sec - ref.tv_sec;
      elapsed.tv_nsec = now.tv_nsec - ref.tv_nsec;
   }
   return elapsed.tv_sec * 1000 + elapsed.tv_nsec / 1000000;
}
// original function:
//unsigned int time_elapsed(struct timespec refts) {
//   uint64_t now;
//   uint64_t ref;
//   struct timespec nowts;
//   clock_gettime(CLOCK_MONOTONIC_RAW, &nowts);
//   ref=(uint64_t)refts.tv_sec * (uint64_t)1000 + (uint64_t)(refts.tv_nsec / 1000000L);
//   now=(uint64_t)nowts.tv_sec * (uint64_t)1000 + (uint64_t)(nowts.tv_nsec / 1000000L);
//   return (uint32_t)(now-ref);
//}

