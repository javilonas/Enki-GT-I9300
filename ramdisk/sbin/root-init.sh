#!/sbin/busybox sh
#
# Script root
#

# Inicio
busybox mount -o remount,rw -t auto /system
busybox mount -t rootfs -o remount,rw rootfs

busybox rm /system/bin/reboot
busybox ln -s /sbin/toolbox /system/bin/reboot

if [ ! -f /system/xbin/busybox ]; then
busybox ln -s /sbin/busybox /system/xbin/busybox
busybox ln -s /sbin/busybox /system/xbin/pkill
fi

if [ ! -f /system/bin/busybox ]; then
busybox ln -s /sbin/busybox /system/bin/busybox
busybox ln -s /sbin/busybox /system/bin/pkill
fi

# Hacer root si no detecta bianrio SU
if [ ! -f /system/xbin/su ] && [ ! -f /system/bin/su ]; then

busybox mkdir /system/bin/.ext
busybox cp /sbin/su /system/xbin/su
busybox cp /sbin/daemonsu /system/xbin/daemonsu
busybox cp /sbin/su /system/bin/.ext/.su
busybox cp /res/ext/install-recovery.sh /system/etc/install-recovery.sh
busybox cp /res/ext/99SuperSUDaemon /system/etc/init.d/99SuperSUDaemon
busybox echo /system/etc/.installed_su_daemon

busybox chown 0.0 /system/bin/.ext
busybox chmod 0777 /system/bin/.ext
busybox chown 0.0 /system/xbin/su
busybox chmod 6755 /system/xbin/su
busybox chown 0.0 /system/xbin/daemonsu
busybox chmod 6755 /system/xbin/daemonsu
busybox chown 0.0 /system/bin/.ext/.su
busybox chmod 6755 /system/bin/.ext/.su
busybox chown 0.0 /system/etc/install-recovery.sh
busybox chmod 0755 /system/etc/install-recovery.sh
busybox chown 0.0 /system/etc/.installed_su_daemon
busybox chmod 0644 /system/etc/.installed_su_daemon

busybox chattr +i /system/xbin/su
busybox chattr +i /system/bin/.ext/.su
busybox chattr +i /system/xbin/daemonsu

busybox /system/bin/sh /system/etc/install-recovery.sh

fi

busybox sleep 1

# Aplicar Fstrim
busybox /sbin/fstrim -v /data
busybox /sbin/fstrim -v /cache
busybox /sbin/fstrim -v /system

busybox sync

busybox sleep 2

# Fix Permisos
busybox chmod 0644 /data/app/*.apk
busybox chown 1000:1000 /data/app/*.apk
busybox chmod 0644 /system/app/*.apk
busybox chown 0:0 /system/app/*.apk
busybox chmod 0644 /system/framework/*.apk
busybox chown 0:0 /system/framework/*.apk
busybox chmod 0644 /system/framework/*.jar
busybox chown 0:0 /system/framework/*.jar
busybox chmod 0755 /system/etc/init.d/*
busybox chmod 0777 /data/log/
busybox chmod 0777 /data/log/*

busybox sync

busybox mount -t rootfs -o remount,ro rootfs
busybox mount -o remount,ro -t auto /system

