/* ------------------------------------------------------------ *
 * file:        timetest.c                                      *
 * purpose:     Sample program for two 4-digit 7-Segment LED    *
 *              displays. It shows the current time on 8 digits *
 *              incl. hour, min, sec and 2 digit sub-seconds.   *
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
#include "tm1640.h"

int main() {
  int res;                                // program returncode
  struct tm *time;                        // standard time struct
  struct timespec now;                    // nanosec time struct
  long ms = 0;                            // milliseconds
  char timestr[10] = "000000.00";         // 7Seg display string;
  tm1640_display *d1 = tm1640_init(3,2);  // tm1640 clock and data pins
  tm1640_displayOn(d1, 2);                // display on + brightness 0..4
  tm1640_displayClear(d1);                // display zero out

  while(1) {
    /* ----------------------------------------------------------- *
     * get current time (now)                                      *
     * ----------------------------------------------------------- */
    clock_gettime(CLOCK_REALTIME, &now);
    time_t tsnow = now.tv_sec;
    time = gmtime(&tsnow);
    ms = round(now.tv_nsec / 1.0e6); // Convert nanosec to millisec
    if (ms > 999) { tsnow++; ms = 0; }

    snprintf(timestr, sizeof(timestr), "%02d%02d%02d.%02lu",
             time->tm_hour, time->tm_min, time->tm_sec, ms);

    res = tm1640_displayWrite(d1, 0, timestr, strlen(timestr), INVERT_MODE_NONE);
    if(ms < 500) tm1640_setColon(d1, 0, 1);         // display-1 Colon on
    else tm1640_setColon(d1, 0, 0);                 // display-1 Colon off
  }
  return res;
}
