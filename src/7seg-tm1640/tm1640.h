// src/libtm1640/tm1640.h - Main interface code for TM1640
// Copyright 2013 FuryFire
// Copyright 2013 Michael Farrell <http://micolous.id.au/>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// original code located in https://github.com/micolous/tm1640-rpi
// This is a derivate copy

#ifndef TM1640_H
#define TM1640_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>

/**
 * Default data GPIO pin to use.
 *
 * Used by the standalone tm1640 application in order to set which display to use.
 */
#define DIN_PIN 2

/**
 * Default clock GPIO pin to use.
 *
 * Used by the standalone tm1640 application in order to set which display to use.
 */
#define SCLK_PIN 3

/**
 * Used by tm1640_displayWrite
 *
 * Sets an inversion mode of "none", that the segments will be output in the way they were input.
 */
#define INVERT_MODE_NONE 0

/**
 * Used by tm1640_displayWrite
 *
 * Sets an inversion mode of "vertical", that the segments will be flipped vertically when output on the display.
 */
#define INVERT_MODE_VERTICAL 1


/**
 * Structure that defines a connection to a TM1640 IC.
 *
 * You should not manipulate this structure directly, and always create
 * new instances of this with tm1640_init
 */
typedef struct {
	/**
	 * WiringPi GPIO pin for display clock (SCLK).
	 */
	int clockPin;
	
	/**
	 * WiringPi GPIO pin for display data (DIN).
	 */
	int dataPin;
} tm1640_display;

/**
 * Initialises the display.
 *
 * @param clockPin WiringPi pin identifier to use for clock (SCLK)
 * @param dataPin WiringPi pin identifier to use for data (DIN)
 *
 * @return NULL if wiringPiSetup() fails (permission error)
 * @return pointer to tm1640_display on successful initialisation.
 */
tm1640_display* tm1640_init(int clockPin, int dataPin);

/**
 * Destroys (frees) the structure associated with the connection to the TM1640.
 *
 * @param display TM1640 display connection to dispose of.
 */
void tm1640_destroy(tm1640_display* display);

/**
 * @private
 * Flips 7-segment characters vertically, for display in a mirror.
 *
 * @param input Bitmask of segments to flip.
 * @return Bitmask of segments flipped vertically.
 */
char tm1640_invertVertical(char input);

/**
 * displayWrite
 *
 * @param display TM1640 display to write to
 * @param offset offset on the display to start writing from
 * @param string string to write to the display
 * @param length length of the string to write to the display
 * @param invertMode invert mode to apply to text written to the display
 *
 * @return -EINVAL if invertMode is invalid
 * @return -EINVAL if offset + length > 16
 * @return 0 on success.
 */
int tm1640_displayWrite(tm1640_display* display, int offset, const char * string, char length, int invertMode);

/**
 * setColon
 *
 * @param display TM1640 display to write to
 * @param num 0 = fist display colon, 1 = 2nd display colon
 * @param state 1 = On, 0 = off
 */
void tm1640_setColon(tm1640_display* display, int num, int state);

/**
 * setDegree
 *
 * @param display TM1640 display to write to
 * @param num 0 = fist display degree, 1 = 2nd display degree
 * @param state 1 = On, 0 = off
 */
void tm1640_setDegree(tm1640_display* display, int num, int state);

/**
 * @private
 * Converts an ASCII character into 7 segment binary form for display.
 *
 * @param ascii Input ASCII byte to translate.
 * @return 0 if there is no translation available.
 * @return bitmask of segments that represents the input character.
 */
char tm1640_ascii_to_7segment(char ascii);

/**
 * Clears the display
 *
 * @param display TM1640 display to clear
 */
void tm1640_displayClear(tm1640_display* display);

/**
 * Turns on the display and sets the brightness level
 *
 * @param display TM1640 display to set brightness of
 * @param brightness Brightness to set (1 is lowest, 7 is highest)
 */
void tm1640_displayOn(tm1640_display* display, char brightness);

/**
 * Turns off the display preserving display data.
 *
 * @param display TM1640 display to turn off
 */
void tm1640_displayOff(tm1640_display* display);

/**
 * @private
 * Sends a cmd followed by len amount of data. Includes delay from wiringPi.
 *
 * Bitbanging the output pins too fast creates unpredictable results.
 *
 * @param display TM1640 display structure to use for this operation.
 * @param cmd The command
 * @param data Pointer to data that should be appended, or NULL if no data is to be passed.
 * @param len Length of data.
 */
void tm1640_send(tm1640_display* display, char cmd, char * data, int len);

/**
 * @private
 * Shifts out the byte on the port.
 *
 * Implementing this with WiringPi directly is too fast for the IC.
 *
 * @param display TM1640 display structure to use this for this operation.
 * @param out Byte to send
 */
void tm1640_sendRaw(tm1640_display* display, char out);

/**
 * @private
 * Send a single byte command
 *
 * @param display TM1640 display structure to use for this operation.
 * @param cmd Command code to send
 */
void tm1640_sendCmd(tm1640_display* display, char cmd);

#endif
