/* ------------------------------------------------------------ *
 * file:        temptime.c                                      *
 * purpose:     Sample program for two 4-digit 7-Segment LED    *
 *              displays. It shows the current time and CPU     *
 *              temperature to both 7Seg LED displays.          *
 *                                                              *
 * return:      0 on success, and -1 on errors.                 *
 *                                                              *
 * requires:    tm1640.c/.h and font.h                          *
 *              orig. in https://github.com/micolous/tm1640-rpi *
 *                                                              *
 * compile:     see Makefile, needs -lWiringPi -lm              *
 *                                                              *
 * example:     ./temptime                                      *
 *                                                              *
 * author:      05/15/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include "tm1640.h"

int main() {
   int res;                                // program returncode
   struct tm *now_tm;                      // standard time struct
   time_t now;                             // seconds since epoch
   struct timespec sleep;                  // new timespec struct
   char segstr[15];                        // display string
   FILE *thermal;                          // Handle for CPU temp
   float systemp, millideg;                // temp values
   int colon_state = 0;                    // blink the colon

/* ------------------------------------------------------------ *
 * Setup sleep time and display control                         *
 * ------------------------------------------------------------ */
   sleep.tv_sec = 0;                       // set sleep time
   sleep.tv_nsec = 500000000;              // to 0.5 seconds

   tm1640_display *d1 = tm1640_init(3,2);  // tm1640 clock and data pins
   tm1640_displayClear(d1);                // display zero out
   tm1640_displayOn(d1, 4);                // display on brightness 0..4

   while(1) {
      /* -------------------------------------------------------- *
       * get current time (now)                                   *
       * -------------------------------------------------------- */
      now = time(0);
      now_tm = localtime(&now);

      /* --------------------------------------------------------- *
       * get CPU temp, and write it into variable systemp          *
       * --------------------------------------------------------- */
      thermal = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
      fscanf(thermal, "%f", &millideg);
      fclose(thermal);
      systemp = (millideg / 10.0) / 100.0;

      /* --------------------------------------------------------- *
       * write time and systemp with .1 precision to 7-seg display *
       * --------------------------------------------------------- */
      snprintf(segstr, sizeof(segstr), "%02d%02d%5.1f",
               now_tm->tm_hour, now_tm->tm_min, systemp);
      res = tm1640_displayWrite(d1, 0, segstr, strlen(segstr), INVERT_MODE_NONE);

      tm1640_setColon(d1, 0, colon_state);    // display-1 Colon blink
      colon_state = 1 - colon_state;          // cycle between 0 and 1
      nanosleep(&sleep, NULL);
  }
  return res;
}
