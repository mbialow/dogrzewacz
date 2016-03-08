#!/bin/bash

stty -F /dev/ttyUSB0 4800 -cstopb -parenb -icanon min 1 time 1
echo -e 'dump\x0A' > /dev/ttyUSB0
#\x0A=\n
echo `date` >  dump.txt
line=""

while true; do
        cat -v /dev/ttyUSB0 |tee -a dump.txt
done

#while read -r line < /dev/ttyUSB0; do
  ## $line is the line read, do something with it
#  echo $line
#done
