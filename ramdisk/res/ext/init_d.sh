#!/sbin/busybox sh
#
# Detectar si existe el directorio en /system/etc y si no la crea. - by Javilonas
#

if [ ! -d "/system/etc/init.d" ] ; then
busybox mount -o remount,rw -t auto /system
busybox mkdir /system/etc/init.d
busybox chmod 0755 /system/etc/init.d/*
busybox mount -o remount,ro -t auto /system
fi

