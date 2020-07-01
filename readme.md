# PiCon-One Software, Code and Programs

![test](https://github.com/fm4dd/picon-one-sw/workflows/test/badge.svg)

## Background

This is the software repository for the PiCon One Controller based on a Raspberry Pi Zero W.
The hardware repository is located at https://github.com/fm4dd/picon-one-hw.

## Components

### DS3231 Realtime Clock (I2C 0x68)

- Setup log [setup1-rtc-ds3231.md](./setup1-rtc-ds3231.md)
- Test Code: src/rtc-ds3231/test-ds3231.c
```
pi@rpi0w:~ $ cd picon-one-sw/src/rtc-ds3231/

pi@rpi0w:~/picon-one-sw/src/rtc-ds3231 $ make
gcc -Wall -g -O1    test-ds3231.c   -o test-ds3231

pi@rpi0w:~/picon-one-sw/src/rtc-ds3231 $ ./test-ds3231
DS3231 DATE: Tuesday 2020-06-02 TIME: 00:36:16
```

### 7-Segment display TM1640 (2-wire serial)

- Setup:
The 7-segment display examples require the WiringPi library:

```
pi@rpi0w:~/picon-one-sw $ sudo apt-get install wiringpi
```

- Test Code: src/rtc-7seg-tm1640

The test code is based on the TM1640 userspace driver written
by Michael Farell, with thanks and full credits.
The original driver code is located in https://github.com/micolous/tm1640-rpi, and
licensed under GPLv3.

I modified the code for supporting the two 4-digit modules
with a total of 8-digits, adding function to support
the extra colon and degree LED segments, and confirming signal
generation with an oscilloscope.

```
pi@rpi0w:~/picon-one-sw/src/7seg-tm1640 $ make
gcc -Wall -g -O1   -c -o tm1640.o tm1640.c
gcc -Wall -g -O1   -c -o tm1640-ctl.o tm1640-ctl.c
gcc tm1640.o tm1640-ctl.o -o tm1640-ctl -lwiringPi -lm
gcc -Wall -g -O1   -c -o timetest.o timetest.c
gcc tm1640.o timetest.o -o timetest -lwiringPi -lm
gcc -Wall -g -O1   -c -o stopwatch.o stopwatch.c
gcc tm1640.o stopwatch.o -o stopwatch -lwiringPi -lm
gcc -Wall -g -O1   -c -o temptime.o temptime.c
gcc tm1640.o temptime.o -o temptime -lwiringPi -lm
```

1. timetest

This program shows the current time on all eight digits: hour, min, seconds, and two-digit sub-seconds. It demonstrates the responsiveness of apps under RPi Linux.

2. temptime

This program shows the current time in HH:MM on the first (left) display, and the CPU temperature on the second (right) display. This program is helpful to monitor how the controller heat develops inside the case when the protective lid is closed.

3. stopwatch

This program implements a simple stopwatch, showing the elapsed time over all eight digits. It is controlled by three push-buttons: "Mode" starts or restarts the clock, "Enter" stops the clock, and "Up" resets the clock to zero.

4. tm1640-ctl 

This is the display control program from the original driver, with an added -ctl.

```
pi@rpi0w:~/picon-zero/src/7seg-tm1640$ ./tm1640-ctl
TM1640 display tool
Usage:
  tm1640-ctl on <0..7>  : Turn on and set brightness. 1 (lowest)..7 (highest)
  tm1640-ctl off        : Turn off display, preserving data.
  tm1640-ctl clear      : Clear display.
  tm1640-ctl write <num>: Write digits to display, up to 8 digits.
```

### 3.5" TFT display Adafruit 2050

- Setup log [setup2-tft-hx8357d.md](./setup2-tft-hx8357d.md)
- Test Code: src/tft-hx8357d

The test programs require libjpeg:
```
pi@rpi0w:~/picon-one-sw/src/tft-hx8357d $ sudo apt-get install libjpeg-dev
...
Need to get 236 kB of archives.
After this operation, 543 kB of additional disk space will be used.
```

1. tft-stopwatch

This program implements a simple stopwatch on the TFT screen. It is controlled by three push-buttons: "Mode" starts or restarts the clock, "Enter" stops the clock, and "Up" resets the clock to zero.

2. tft-tempgraph

This program measures the CPU temperature in 500ms intervals, and creates a history graph over the past 6 minutes. This is useful to see CPU load impact on heat generation.

### SC16IS572 dual-UART (I2C 0x48)

- Setup
```
pi@rpi0w:~ $ sudo vi /boot/config.txt
dtoverlay=sc16is752-i2c,int_pin=24,addr=0x48
```
- Test Code: src/uart-sc16is752
```
pi@rpi0w:~/picon-one-sw/src/uart-sc16is752 $ make
gcc -O1 -Wall -g   -c -o uart-send.o uart-send.c
gcc -O1 -Wall -g   -c -o serial.o serial.c
gcc -O1 -Wall -g -o uart-send uart-send.o serial.o
gcc -O1 -Wall -g   -c -o uart-receive.o uart-receive.c
gcc -O1 -Wall -g -o uart-receive uart-receive.o serial.o
```

Cross-connect pinheader contacts on J2: RX1 --> TX2, and TX1 --> RX2

Open two terminals. Terminal-1:
```
pi@rpi0w:~/picon-one-sw/src/uart-sc16is752 $ ./uart-receive
/dev/ttySC1 [115200] receive: !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRS
TUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
```
Terminal-2:
```
pi@rpi0w:~/picon-one-sw/src/uart-sc16is752 $ ./uart-send
/dev/ttySC0 [115200] send: !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUV
WXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}
```

### XBee RF module

```
pi@rpi0w:~/picon-one-sw/src/xbee-module $ ./xbee-term /dev/ttySC1
Simple XBee Terminal
CTRL-X to EXIT, CTRL-K toggles break, CTRL-R toggles RTS, TAB changes bps.
[115200 bps][set RTS][CTS cleared]
+++OK
ATSL
41B7962A
```

## License

MIT License

Other license forms for derivative work may apply, as as listed from original authors.
