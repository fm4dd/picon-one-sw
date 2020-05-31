/* ------------------------------------------------------------ *
 * file:        stopwatch.c                                     *
 * purpose:     Sample program for two 4-digit 7-Segment LED    *
 *              displays. It implements a simple stopwatch with *
 *              buttons Mode=start, Enter=stop, Up=clear.       *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 *                                                              *
 * requires:    tm1640.c/.h and font.h                          *
 *              orig. in https://github.com/micolous/tm1640-rpi *
 *                                                              *
 * compile:     see Makefile, needs -lWiringPi -lm              *
 *                                                              *
 * example:     ./timetest                                      *
 *                                                              *
 * author:      05/15/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <wiringPi.h>
#include <signal.h>
#include "tm1640.h"

int main() {
  int res;                                // program returncode
  struct tm *time;                        // standard time struct
  struct timespec tp1, tp2, tp3, tp4;     // nanosec time structs
  long ms = 0;                            // milliseconds
  char timestr[10] = "000000.00";         // 7Seg display string
  tm1640_display *d1 = tm1640_init(3,2);  // tm1640 clock and data pins
  tm1640_displayOn(d1, 2);                // display on + brightness 0..4
  tm1640_displayClear(d1);                // display zero out

  wiringPiSetup();
  pinMode (21, INPUT);  // SW1 Up
  pinMode (22, INPUT);  // SW2 Mode
  pinMode (23, INPUT);  // SW3 Down
  pinMode (24, INPUT);  // SW4 Enter

  bool runstate = FALSE;
  tp3.tv_sec  = 0;
  tp3.tv_nsec = 0;
  tp4.tv_sec  = 0;
  tp4.tv_nsec = 0;

  while(1) {
    /* ----------------------------------------------------------- *
     * Check button press MODE for start action                    *
     * ----------------------------------------------------------- */
    if((digitalRead(22) == LOW) && (runstate == FALSE)) { 
      runstate = TRUE;
      clock_gettime(CLOCK_MONOTONIC_RAW, &tp1);
      //printf("Start\n");
    }
    /* ----------------------------------------------------------- *
     * Check button press ENTER for stop action                    *
     * ----------------------------------------------------------- */
    if((digitalRead(24) == LOW) && (runstate == TRUE)) {
      runstate = FALSE;
      tp4.tv_sec = tp3.tv_sec;
      tp4.tv_nsec = tp3.tv_nsec;
    }
    /* ----------------------------------------------------- *
     * Check button press UP for clear action                *
     * ----------------------------------------------------- */
    if((digitalRead(21) == LOW) && (runstate == FALSE)) {
      snprintf(timestr, sizeof(timestr), "000000.00");
      tp4.tv_sec = 0;
      tp4.tv_nsec = 0;
    }

    /* ----------------------------------------------------------- *
     * In runstate, check and display the elapsing time            *
     * ----------------------------------------------------------- */
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

      snprintf(timestr, sizeof(timestr), "%02d%02d%02d.%02ld",
               time->tm_hour, time->tm_min, time->tm_sec, ms);

    }
    //printf("time %lu - %lu - %lu\n", tp2.tv_nsec, tp1.tv_nsec, ms);
    res = tm1640_displayWrite(d1, 0, timestr, strlen(timestr), INVERT_MODE_NONE);
    if(ms < 500) tm1640_setColon(d1, 0, 1);         // display-1 Colon on
    else tm1640_setColon(d1, 0, 0);                 // display-1 Colon off
  }
  return res;
} 
