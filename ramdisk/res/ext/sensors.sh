#!/system/bin/sh

# re-calibrate proximity sensor
prox_recal=true

# force prox sensor to recalibrate
if $prox_recal
then
	echo 0 > /sys/class/sensors/proximity_sensor/prox_cal
	echo 1 > /sys/class/sensors/proximity_sensor/prox_cal
fi
