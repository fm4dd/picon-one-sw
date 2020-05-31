// src/libtm1640/tm1640.c - Main interface code for TM1640
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


#include "tm1640.h"
#include "font.h"


char tm1640_invertVertical(char input) {
   return (input & 0xC0) | // swap top and bottom
   ((input & 0x01) << 3) |
   ((input & 0x08) >> 3) | // swap lefts
   ((input & 0x02) << 1) |
   ((input & 0x04) >> 1) | // swap rights
   ((input & 0x10) << 1) |
   ((input & 0x20) >> 1);
}


int tm1640_displayWrite(tm1640_display* display, int offset, const char * string, char length, int invertMode) {
   int c=0;
   char buffer[33];
   memset(buffer, 0, sizeof(buffer));
   memcpy(buffer, string, sizeof(buffer));

   // Translate input to segments
   // Return -EINVAL if input string is too long.  Allowance is made for
   // decimal points.
   // TODO: provide function to allow raw writing of segments
   for (c=0; c<length; c++) {
      if (((buffer[c] == '.') && (offset + c) >= 9) ||
          ((buffer[c] != '.') && (offset + c) >= 8)) {
         return -EINVAL;
      }
      buffer[c] = tm1640_ascii_to_7segment(buffer[c]);

      switch (invertMode) {
         case INVERT_MODE_NONE:
            // do nothing
            break;
         case INVERT_MODE_VERTICAL:
            buffer[c] = tm1640_invertVertical(buffer[c]);
            break;
         default:
            return -EINVAL;
      }

      // If possible merge the decimal point with the previous
      // character.  This is only possible if it is not the first
      // character or if the previous character has not already had
      // a decimal point merged.
      if(c!=0 && (0b10000000 & buffer[c]) && !(0b10000000 & buffer[c-1])) {
         buffer[c-1] |= 0b10000000;
         memmove(&buffer[c], (const char *)(&buffer[c+1]), (sizeof(char)*(17-c)));
         memset(&buffer[16], 0, sizeof(char));;
         c--;
         length--;
      }
   }
   tm1640_sendCmd(display, 0x44);
      delayMicroseconds(2);
   tm1640_send(display, 0xC0 + offset, buffer, c);
   return 0;
}


char tm1640_ascii_to_7segment(char ascii) {
   if (ascii < FONT_FIRST_CHAR || ascii > FONT_LAST_CHAR) {
      // character than is not in font, skip.
      return 0;
   }
   return DEFAULT_FONT[ascii - FONT_FIRST_CHAR];
}

void tm1640_setColon(tm1640_display* display, int num, int state) {
   char colon;
   if(state == 1) colon = 0b00000011;
   else colon = 0b00000000;
   tm1640_sendCmd(display, 0x44);
      delayMicroseconds(2);
   tm1640_send(display, 0xC0 + 8 + num, &colon, 1);
}

void tm1640_setDegree(tm1640_display* display, int num, int state) {
   char degree;
   if(state == 1) degree = 0b00000001;
   else degree = 0b00000000;
   tm1640_sendCmd(display, 0x44);
      delayMicroseconds(2);
   tm1640_send(display, 0xC0 + 8 + num, &degree, 1);
}

void tm1640_displayClear(tm1640_display* display) {
   char buffer[16];
   memset( buffer, 0x00, 16 );
   tm1640_send(display, 0xC0, buffer, 16);
}

void tm1640_displayOn(tm1640_display* display, char brightness) {
   if (brightness < 1) brightness = 1;
   if (brightness > 7) brightness = 7;
   tm1640_sendCmd(display, 0x88 + brightness);
}


void tm1640_displayOff(tm1640_display* display) {
   tm1640_sendCmd(display, 0x80);
}

tm1640_display* tm1640_init(int clockPin, int dataPin) {
   if(wiringPiSetup() == -1) {
     printf("Error WiringPi\n");
     return NULL;
   }
   pinMode(clockPin, OUTPUT);
   pinMode(dataPin, OUTPUT);
   digitalWrite(clockPin, HIGH);
   digitalWrite(dataPin, HIGH);
   tm1640_display* display = malloc(sizeof(tm1640_display));
   // clear for good measure
   memset(display, 0, sizeof(tm1640_display));
   display->clockPin = clockPin;
   display->dataPin = dataPin;
   return display;
}

void tm1640_destroy(tm1640_display* display) {
   free(display);
}

// send one byte to IC. CLK=L after start 
void tm1640_sendRaw(tm1640_display* display, char out) {
   int i;
   for(i = 0; i < 8; i++) {
      digitalWrite(display->dataPin, out & (1 << i));
      digitalWrite(display->clockPin, HIGH);
      delayMicroseconds(1);
      digitalWrite(display->clockPin, LOW);
      delayMicroseconds(1);
   }
}

void tm1640_send(tm1640_display* display, char cmd, char * data, int len) {
   //Issue start command
   //CLK=H, Data changes from H->L
   digitalWrite(display->dataPin, LOW);
   delayMicroseconds(1);
   digitalWrite(display->clockPin, LOW);
   delayMicroseconds(1);

   tm1640_sendRaw(display, cmd);
   if(data != NULL) {
      int i;
      for(i = 0; i < len; i++) {
         tm1640_sendRaw(display, data[i]);
      }
   }

   //Issue stop command
   //When CLK=H, Data changes from L->H
   digitalWrite(display->clockPin, HIGH);
   delayMicroseconds(1);
   digitalWrite(display->dataPin, HIGH);
   delayMicroseconds(1);
}

void tm1640_sendCmd(tm1640_display* display, char cmd ) {
   tm1640_send(display, 0x40, 0 ,0);
   tm1640_send(display, cmd, 0, 0);
   digitalWrite(display->dataPin, LOW);
   digitalWrite( display->clockPin, LOW);
   delayMicroseconds(1);
   digitalWrite( display->clockPin, HIGH);
   digitalWrite( display->dataPin, HIGH);
}
