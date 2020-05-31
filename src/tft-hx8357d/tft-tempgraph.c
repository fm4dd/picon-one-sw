/* ------------------------------------------------------------ *
 * file:        tft-tempgraph.c                                 *
 * purpose:     TFT test program for Adafruit 3.5" TFT on RPi   *
 *              creates a graph from the CPU temperature.       *
 * return:      0 on success, and -1 on errors.                 *
 * requires:    TFT as framebuffer /dev/fb0, openvg lib from    *
 *              https://github.com/ajstarks/openvg              *
 * author:      03/10/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include "fontinfo.h"
#include "shapes.h"

/* ------------------------------------------------------------ *
 * coordpoint() marks a coordinate, preserving a previous color *
 * ------------------------------------------------------------ */
void coordpoint(VGfloat x, VGfloat y, VGfloat size, VGfloat pcolor[4]) {
        Fill(255, 255, 255, 1);
        Circle(x, y, size);
        setfill(pcolor);
}

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

/* ------------------------------------------------------------ *
 * tempToChart() calculates the chart point for the given temp  *
 * ------------------------------------------------------------ */
int tempToChart(float temp) {
   int chartpt = 50;  // Y-axis zero value
   if(temp <= 30.0)
      return chartpt;
   if(temp > 60.0)
      return 200;
   if(temp > 30.0) 
      chartpt = (int) round(50.0 + ((temp - 30.0) * 5));
   return chartpt;
}

int main() {
   int width, height;
   FILE *file;
   float systemp, millideg, sysfreq;
   uint8_t r; 
   uint8_t g;
   uint8_t b;

   static char time_str[9];
   static char date_str[9];
   static char temp_str[50];
   time_t now;
   struct tm *now_tm;
   struct timespec sleep;
   int xcount = 0;

   VGfloat shapecolor[4];
   RGB(255, 125, 125, shapecolor);
   int chartset[440] = { 0 };

   /* --------------------------------------------------------- *
    * Setup sleep time and display control                      *
    * --------------------------------------------------------- */
   sleep.tv_sec = 1;                       // set sleep time
   //sleep.tv_nsec = 500000000;            // to 0.5 seconds
   init(&width, &height);                  // Graphics init

   while(1) {
     /* ------------------------------------------------------ *
      * TFT display output, upper header 70px from 251..319    *
      * ------------------------------------------------------ */
      Start(width, height);                     // start the picture
      Background(0, 0, 0);                      // set background black
      StrokeWidth(1);                           // set line size
      Stroke(0, 255, 0, 1);                     // set line color green
      Line(0, 250, 479, 250);                   // draw a separator line
      Image(20, 253, 64, 64, "./images/rpi-logo64.jpg"); // load RPI logo

     /* --------------------------------------------------------- *
      * get system time and write it into the string variables    *
      * --------------------------------------------------------- */
      now = time(0); // Get the system time
      now_tm = localtime(&now);
      strftime(date_str, sizeof(date_str), "%y-%m-%d",now_tm);

      hexToRGB(0x3536, &r, &g, &b);             // use the Arduino 16bit values
      Fill(r, g, b, 1);                         // set foreground blue-ish
      Text(130, 297, "Raspberry", NotoMonoTypeface, 19);
      Text(130, 273, "Pi Zero-W", NotoMonoTypeface, 19);
      Text(320, 290, date_str, MonoTypeface, 22);

      strftime(time_str, sizeof(time_str), "%H:%M:%S",now_tm);

      hexToRGB(0xfce0, &r, &g, &b);             // use Arduino 16bit values
      Fill(r, g, b, 1);                         // set foreground amber
      Text(130, 256, "PiCon One v1.0", NotoMonoTypeface, 12);
      Text(320, 260, time_str, MonoTypeface, 22);

      Stroke(0, 255, 0, 1);                     // set line color green

     /* --------------------------------------------------------- *
      * get CPU temp into variable systemp, and write to string   *
      * --------------------------------------------------------- */
      file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
      fscanf(file, "%f", &millideg);
      fclose(file);
      systemp = (millideg / 10.0) / 100.0;

      file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
      fscanf(file, "%f", &sysfreq);
      fclose(file);

      sprintf(temp_str, "CPU: %.2f°C %.0fMHz", systemp, sysfreq/1000);

      Fill(255, 255, 255, 1);                   // set foreground White
      Text(130, 220, temp_str,                  // write temp value
           NotoMonoTypeface, 18);               // to pixel pos 30v220h

     /* --------------------------------------------------------- *
      * draw coordinate system for graph: 150px vert, 450px horiz *
      * --------------------------------------------------------- */
      StrokeWidth(2);                           // set line size
      Stroke(255, 255, 255, 1);                 // set line color white
      Line(29, 230, 29, 50);                    // draw Y-axis (60.0 °C)
      Line(29, 49, 469, 49);                    // draw X-axis (30.0 °C)

     /* --------------------------------------------------------- *
      * Y-axis legend text and reference lines                    *
      * --------------------------------------------------------- */
      StrokeWidth(1);                           // set line size
      Stroke(155, 155, 155, 1);                 // set line color grey
      Line(30, 100, 469, 100);                  // ref line 40.0 °C
      Line(30, 150, 469, 150);                  // ref line 50.0 °C
      Text(4, 44, "30", NotoMonoTypeface, 12);
      Text(4, 94, "40", NotoMonoTypeface, 12);
      Text(4, 144, "50", NotoMonoTypeface, 12);
      Text(4, 194, "60", NotoMonoTypeface, 12);
      Text(4, 216, "°C", NotoMonoTypeface, 12);


     /* --------------------------------------------------------- *
      * X-axis legend text and reference lines                    *
      * --------------------------------------------------------- */
      Text(74, 30, "1", NotoMonoTypeface, 12);
      Text(124, 30, "2", NotoMonoTypeface, 12);
      Text(184, 30, "3", NotoMonoTypeface, 12);
      Text(244, 30, "4", NotoMonoTypeface, 12);
      Text(304, 30, "5", NotoMonoTypeface, 12);
      Text(364, 30, "6", NotoMonoTypeface, 12);
      Text(400, 30, "Minutes", NotoMonoTypeface, 12);

     /* --------------------------------------------------------- *
      * draw the chart points                                     *
      * --------------------------------------------------------- */
      Stroke(255, 90, 90, 1);                   // Set Line color red
      chartset[xcount] = tempToChart(systemp);
      for(int cpt = 0; cpt < 440; cpt++) {
         if(chartset[cpt] > 0) Line(cpt+30, 50, cpt+30, chartset[cpt]);
         else Line(cpt+30, 50, cpt+30, 50);
      }
      xcount++;

     /* --------------------------------------------------------- *
      * When graph reaches right side end, wipe and restart left  *
      * --------------------------------------------------------- */
      if(xcount > 439) {
         xcount = 0;                            // reset position
         memset(chartset, 0, sizeof(chartset)); // clear all values
      }

      End();                                    // End the picture
      nanosleep(&sleep, NULL);                  // sleep 0.5 seconds
   }
      
   finish();					// Graphics cleanup
   exit(0);
}
