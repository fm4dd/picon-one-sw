## PiCon One - OS and DS3231 RTC Setup

This is the first setup step for PiCon One.
It covers the steps for basic OS install, 
and enabling the DS3231 realtime clock.

### Rasbian lite OS install

[Raspbian Buster Lite](https://www.raspberrypi.org/downloads/raspbian/)
Minimal image based on Debian Buster
Version:February 2020
Release date:2020-02-13
Kernel version:4.19
Size:434 MB

#### Write the lite image file to SD card.

#### Enable the serial console in /boot/config.txt,
e.g. edit the file on a host PC:
```
echo "enable_uart=1" >> /mnt/boot/config.txt
```

### Rasbian lite OS 1st boot

I am connected through the FTDI USB-serial cable,
attached to the serial console pin header row.

1st power-up is monitored on the PC through Putty
using the serial configuration.

#### Change the password, run raspi-config

Login with the Raspbian default user/password.
Run raspi-config to enable SSH, I2C, SPI, and Serial.

```
pi@rpi0w:~ $ sudo su
root@rpi0w:/home/pi# ./raspi-config
```

#### Verify HW settings in /boot/config.txt
```
pi@rpi0w:~ $ egrep -v '^#' /boot/config.txt |grep -v '^$'
dtparam=i2c_arm=on
dtparam=spi=on
dtparam=audio=on
[pi4]
dtoverlay=vc4-fkms-v3d
max_framebuffers=2
[all]
enable_uart=1
```

### Update the Rasbian OS

This requires network connectivity. 
If not already set up, configure the network settings (enter Wifi key, IP address).
Check if routing works (e.g. ping).

root@rpi0w:/home/pi# apt-get update
root@rpi0w:/home/pi# apt-get upgrade

### Get 1st set of pre-requisite packages

apt-get install git i2c-tools libi2c-dev

### Get PiCon Software

```
pi@rpi0w:~ $ git clone https://github.com/fm4dd/picon-one-sw.git
Cloning into 'picon-one-sw'...
...
```

### Enumerate I2C Devices

```
pi@rpi0w:~ $ i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- 48 -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```

- 0x48 = SC16IS752 2x UART expander IC
- 0x68 = DS3231 realtime clock IC

#### Verify DS3231 realtime clock

A test program is located udner 'picon-one-sw/src/rtc-ds3231'.
After compiation, it will query the RTC over I2C, and return its time.

```
pi@rpi0w:~ $ cd picon-one-sw/src/rtc-ds3231/
pi@rpi0w:~/picon-one-sw/src/rtc-ds3231 $ make
gcc -Wall -g -O1    test-ds3231.c   -o test-ds3231
pi@rpi0w:~/picon-one-sw/src/rtc-ds3231 $ ./test-ds3231
DS3231 DATE: Tuesday 2020-06-02 TIME: 00:36:16
pi@rpi0w:~/picon-one-sw/src/rtc-ds3231 $
```
The 'C' program 'test-ds3231.c' has a function set=rtc(). 
If set_rtc() gets enabled in main(),
It will set the RTC time according to the current system time.

#### Enable the DS3231 realtime clock driver

```
pi@rpi0w:~ $ sudo su
root@rpi0w:/home/pi# echo "dtoverlay=i2c-rtc,ds3231" >> /boot/config.txt

root@rpi0w:/home/pi# tail -2 /boot/config.txt
enable_uart=1
dtoverlay=i2c-rtc,ds3231

root@rpi0w:/home/pi# reboot
```

After the reboot, the new /dev/rtc0 device will become available,
lsmod lists the loaded kernel module, and i2cdetect -y 1  now
returns 'UU' to show the device is used by a driver.

```
pi@rpi0w:~ $ dmesg |grep rtc
[   22.531026] rtc-ds1307 1-0068: registered as rtc0

pi@rpi0w:~ $ lsmod |grep rtc
rtc_ds1307             24576  0
hwmon                  16384  2 rtc_ds1307,raspberrypi_hwmon

pi@rpi0w:~ $ i2cdetect -y 1 |grep 60
60: -- -- -- -- -- -- -- -- UU -- -- -- -- -- -- --
```

Lastly, there is one more step to edit '/lib/udev/hwclock-set'.

### Reference Articles

- https://www.raspberrypi.org/forums/viewtopic.php?t=161133
- https://learn.adafruit.com/adding-a-real-time-clock-to-raspberry-pi/set-rtc-time
