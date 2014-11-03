#!/sbin/busybox sh
#
# Detectar si existe el directorio en /system/etc y si no la crea. - by Javilonas
#

SYSTEM_DEVICE="/dev/block/mmcblk0p9"

if [ ! -d "/system/etc/init.d" ] ; then
/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system
  /sbin/busybox mkdir /system/etc/init.d
  /sbin/busybox chmod 755 /system/etc/init.d/*
/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system
fi
