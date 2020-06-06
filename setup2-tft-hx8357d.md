## PiCon One - TFT display setup

This step describes the TFT display setup for PiCon One.

The TFT display I am using is Adafruit 3.5 480x320 display.
Link: [Adafruit Product ID 2050](https://www.adafruit.com/product/2050)

The current pin assignment for this TFT on PiCon One v1.0a is matching
the Adafruit 3.5 PiTFT, product ID 2097.
This allows to use the original Adafruit drivers and
follow Adafruit documentation. The TFT screen itself is identical
for both product IDs 2050 and 2097, their pinout is different.

The choice for the 2050 display comes from the pin row location,
which are non-populated. This allows to reduce the stand-off height by 3mm
when using low-profile pinframes.

### Get framebuffer driver code and pre-requisites

I choose the 'juj' SPI TFT driver over Adafruits original. Two specifics make it
attractive: Great performance, and its design as a "standalone" binary driver.
The driver creates the /dev/fb0 device, eliminating the need to mirror framebuffers.

```
pi@rpi0w:~ $ git clone https://github.com/juj/fbcp-ili9341.git
Cloning into 'fbcp-ili9341'...
...
Resolving deltas: 100% (1063/1063), done.
```

The compile configuration of this driver requires 'cmake'.

```
pi@rpi0w:~ $ sudo apt-get install cmake
...
Need to get 4,601 kB of archives.
After this operation, 22.5 MB of additional disk space will be used.
...
```

### Configure framebuffer compile options

Now the driver settings need to be specified for TFT hardware and pin configuration:

```
pi@rpi0w:~/fbcp-ili9341 $ cmake -DSPI_BUS_CLOCK_DIVISOR=8 -DHX8357D=ON -DADAFRUIT_HX8357D_PITFT=ON -DGPIO_TFT_DATA_CONTROL=25 -DGPIO_TFT_RESET_PIN=17 -DGPIO_TFT_BACKLIGHT=18 -DSINGLE_CORE_BOARD=ON -DARMV6Z=ON -DSTATISTICS=0 -DDISPLAY_ROTATE_180_DEGREES=ON -S .
-- Doing a Release build
-- Board revision: 9000c1
-- Detected this Pi to be one of: Pi A, A+, B rev. 1, B rev. 2, B+, CM1, Zero or Zero W, with single hardware core and ARMv6Z instruction set CPU.
-- Targeting a Raspberry Pi with only one hardware core
-- Enabling optimization flags that target ARMv6Z instruction set (Pi Model A, Pi Model B, Compute Module 1, Pi Zero/Zero W)
-- Using 4-wire SPI mode of communication, with GPIO pin 25 for Data/Control line
-- Using GPIO pin 17 for Reset line
-- Using GPIO pin 18 for backlight
-- Scaling source image to view. If the HDMI resolution does not match the SPI display resolution, this will produce blurriness. Match the HDMI display resolution with the SPI resolution in /boot/config.txt to get crisp pixel perfect rendering, or alternatively pass -DDISPLAY_CROPPED_INSTEAD_OF_SCALING=ON to crop instead of scale if you want to view the center of the screen pixel perfect when HDMI and SPI resolutions do not match.
-- Preserving aspect ratio when scaling source image to the SPI display, introducing letterboxing/pillarboxing if HDMI and SPI aspect ratios are different (Pass -DDISPLAY_BREAK_ASPECT_RATIO_WHEN_SCALING=ON to stretch HDMI to cover full screen if you do not care about aspect ratio)
-- SPI_BUS_CLOCK_DIVISOR set to 8. Try setting this to a higher value (must be an even number) if this causes problems. Display update speed = core_freq/divisor. (on Pi3B, by default core_freq=400). A safe starting default value may be -DSPI_BUS_CLOCK_DIVISOR=40
-- Rotating display output by 180 degrees
-- USE_DMA_TRANSFERS enabled, this improves performance. Try running CMake with -DUSE_DMA_TRANSFERS=OFF it this causes problems, or try adjusting the DMA channels to use with -DDMA_TX_CHANNEL=<num> -DDMA_RX_CHANNEL=<num>.
-- Targeting Adafruit 3.5 inch PiTFT with HX8357D
-- Configuring done
-- Generating done
-- Build files have been written to: /home/pi/fbcp-ili9341
```

### Compile the framebuffer driver

```
pi@rpi0w:~/fbcp-ili9341 $ make
Scanning dependencies of target fbcp-ili9341
[  5%] Building CXX object CMakeFiles/fbcp-ili9341.dir/diff.cpp.o
[ 95%] Building CXX object CMakeFiles/fbcp-ili9341.dir/text.cpp.o
[100%] Linking CXX executable fbcp-ili9341
[100%] Built target fbcp-ili9341

pi@rpi0w:~/fbcp-ili9341 $ ls -l fbcp-ili9341
-rwxr-xr-x 1 pi pi 65304 Jun  5 05:39 fbcp-ili9341

pi@rpi0w:~/fbcp-ili9341 $ file fbcp-ili9341
fbcp-ili9341: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 3.2.0, BuildID[sha1]=d2fc883388c782d08b7565f1ae22c95466928b66, with debug_info, not stripped
```

### Test the framebuffer driver

Now we can start the framebuffer driver. It should initialize the display and show console output on the screen.

```
pi@rpi0w:~/fbcp-ili9341 $ sudo ./fbcp-ili9341 &
[1] 2101
pi@rpi0w:~/fbcp-ili9341 $ bcm_host_get_peripheral_address: 0x20000000, bcm_host_get_peripheral_size: 33554432, bcm_host_get_sdram_address: 0x40000000
BCM core speed: current: 250000000hz, max turbo: 250000000hz. SPI CDIV: 8, SPI max frequency: 31250000hz
Allocated DMA channel 7
Allocated DMA channel 1
Enabling DMA channels Tx:7 and Rx:1
DMA hardware register file is at ptr: 0xb4afc000, using DMA TX channel: 7 and DMA RX channel: 1
DMA hardware TX channel register file is at ptr: 0xb4afc700, DMA RX channel register file is at ptr: 0xb4afc100
Resetting DMA channels for use
DMA all set up
Initializing display
Resetting display at reset GPIO pin 17
InitSPI done
Relevant source display area size with overscan cropped away: 720x480.
Source GPU display is 720x480. Output SPI display is 480x320 with a drawable area of 480x320. Applying scaling factor horiz=0.67x & vert=0.67x, xOffset: 0, yOffset: 0, scaledWidth: 480, scaledHeight: 320
Creating dispmanX resource of size 480x320 (aspect ratio=1.500000).
GPU grab rectangle is offset x=0,y=0, size w=480xh=320, aspect ratio=1.500000
All initialized, now running main loop...
```

A second test of the new framebuffer is to display an image to it, using the fbi command.
A test image 480x320-test.jpg is located under  picon-one-sw/src/tft-hx8357d/images.

```
pi@rpi0w:~ $ sudo apt-get install fbi
...
Need to get 17.1 MB of archives.
After this operation, 56.2 MB of additional disk space will be used.
...

pi@rpi0w:~ $ sudo fbi -t 60 -cachemem 0 --autozoom --noverbose -d /dev/fb0 --vt 1 picon-one-sw/src/tft-hx8357d/images/480x320-test.jpg
using "DejaVu Sans Mono-16", pixelsize=16.67 file=/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf
```

### framebuffer driver auto-start

Adjust console font:

```
pi@rpi0w:~ $ sudo vi /etc/default/console-setup
CHARMAP="UTF-8"

CODESET="guess"
FONTFACE="Terminus"
FONTSIZE="6x12"
```

Set kernel options in /boot/config.txt

```
pi@rpi0w:~ $ vi /boot/config.txt
framebuffer_heigth=320
framebuffer_width=480
hdmi_force_hotplug=1
hdmi_cvt=480 320 60 1 0 0 0
hdmi_group=2
hdmi_mode=87
```

For more information on aboev display settings, see
https://www.raspberrypi.org/documentation/configuration/config-txt/video.md

Runninng the driver at startup:

```
pi@rpi0w:~ $ vi /etc/rc.local
sudo /home/pi/fbcp-ili9341/fbcp-ili9341 &
```
The console idle function may turn off the display backlight.
The next line attempts to prevent that.
```
pi@rpi0w:~ $ vi /etc/rc.local
sudo sh -c "TERM=linux setterm -blank 0 >/dev/tty1"
```

### TFT display console on/off

By default, the TFT display gets console output.
This is defined as a kernel option in /boot/cmdline.txt: "console=tty1".

If we run a custom program on the TFT display, it can happen that
unwanted console messages pop up and ruin the nicely drawn-up screen.

In that case, removing the console option will boot with a empty screen.

### TFT display backlight pull-up

Adafruits TFT displays have a pull-up resistor that enables the backlight
if no dedicated signal is connected. This "default" brings up the screen
"white" until screen output is send to it.
