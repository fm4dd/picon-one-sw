#!/bin/bash
/bin/dmesg --console-on
/bin/sleep 1
/usr/local/bin/tm1640-ctl off
/sbin/shutdown -h now
