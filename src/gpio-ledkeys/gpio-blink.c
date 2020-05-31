/* ------------------------------------------------------------ *
 * file:        gpio-blink.c                                    *
 * purpose:     Sample code for GPIO-connected LEDs. It blinks  *
 *              the two LED on pin-32 (green) & pin-16 (orange) *
 *                                                              *
 * requires:    WiringPi: sudo apt-get install wiringpi         *
 *                                                              *
 * compile:     see Makefile, needs -lWiringPi                  *
 *                                                              *
 * example:     ./gpio-blink                                    *
 *                                                              *
 * author:      05/30/2020 Frank4DD                             *
 * ------------------------------------------------------------ */
#include <wiringPi.h>

int main (void) {
  wiringPiSetup ();       // load WiringPi lib
  pinMode (4, OUTPUT);    // phys. pin # 16 is digital out
  pinMode (26, OUTPUT);   // phys. pin # 32 is digital out

  for (;;) {
    digitalWrite (4, LOW);
    digitalWrite (26, HIGH);
    delay (500);
    digitalWrite (4,  HIGH);
    digitalWrite (26,  LOW);
    delay(500);
  }
  return 0;
}
