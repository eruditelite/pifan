#!/bin/sh
# From https://core-electronics.com.au/tutorials/stress-testing-your-raspberry-pi.html

gcc -o cpuburn-a53 cpuburn-a53.S
stress -c 8 -t 900s &
./cpuburn-a53 &

while true
do
    vcgencmd measure_clock arm
    vcgencmd measure_temp
    sleep 10
done
