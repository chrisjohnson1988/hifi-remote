#!/bin/sh
GPIO_PIN=14
GPIO_PATH="/sys/class/gpio/gpio${GPIO_PIN}"

if [ ! -d "$GPIO_PATH" ]; then
  # Set trigger pin
  echo $GPIO_PIN > /sys/class/gpio/export
  echo "out" > "${GPIO_PATH}/direction"
fi

# Listen to changes on snapclient
journalctl -u snapclient -f -n0 | while read line; do
   if echo $line | grep -q 'PCM name'; then
     echo "1" > "${GPIO_PATH}/value"
   fi
   if echo $line | grep -q 'Closing ALSA'; then
     echo "0" > "${GPIO_PATH}/value"
   fi
done
