/* ------------------------------------------------------------ *
 * file:        gpio-keys.c                                     *
 * purpose:     Sample code for GPIO-connected push buttons.    *
 *              We have 4 buttons UP - DOWN - MODE - ENTER:     *
 *              UP = RPI1 LED ON, DOWN = RPI1 LED OFF           *
 *              MODE = RPI2 LED ON, ENTER = RPI2 LED OFF        *
 *                                                              *
 * requires:    WiringPi: sudo apt-get install wiringpi         *
 *                                                              *
 * compile:     see Makefile, needs -lWiringPi -lm              *
 *                                                              *
 * example:     ./gpio-keys                                     *
 *                                                              *
 * author:      05/30/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <wiringPi.h>

int main (void) {
  wiringPiSetup ();
  pinMode (21, INPUT);  // SW1 Up
  pinMode (22, INPUT);  // SW2 Mode
  pinMode (23, INPUT);  // SW3 Down
  pinMode (24, INPUT);  // SW4 Enter

  pinMode (26, OUTPUT); // LED D2 RPI1 (green)
  pinMode (4, OUTPUT);  // LED D3 RPI2 (orange)

  for (;;) {
    if(digitalRead(21) == LOW) digitalWrite (26, HIGH);
    if(digitalRead(23) == LOW) digitalWrite (26, LOW);
    if(digitalRead(22) == LOW) digitalWrite (4, HIGH);
    if(digitalRead(24) == LOW) digitalWrite (4, LOW);
    delay (200);
  }
  return 0;
}
