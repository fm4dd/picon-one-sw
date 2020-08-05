#!/bin/bash
while(true); do DOWN=$(gpio read 23); if [ $DOWN -eq "0" ]; then echo "down pressed"; sudo killall -s INT gpsmon; exit; fi; done
