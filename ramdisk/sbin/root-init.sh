#!/sbin/busybox sh
#
# Script root
#

BB=/sbin/busybox

# Inicio
$BB mount -o remount,rw -t auto /system
$BB mount -t rootfs -o remount,rw rootfs

$BB rm /system/bin/reboot
$BB ln -s /sbin/toolbox /system/bin/reboot

if [ ! -f /system/xbin/busybox ]; then
$BB ln -s /sbin/busybox /system/xbin/busybox
$BB ln -s /sbin/busybox /system/xbin/pkill
fi

if [ ! -f /system/bin/busybox ]; then
$BB ln -s /sbin/busybox /system/bin/busybox
$BB ln -s /sbin/busybox /system/bin/pkill
fi

# Hacer root si no detecta bianrio SU
if [ ! -f /system/xbin/su ] && [ ! -f /system/bin/su ]; then

$BB mkdir /system/bin/.ext
$BB cp /sbin/su /system/xbin/su
$BB cp /sbin/daemonsu /system/xbin/daemonsu
$BB cp /system/lib/libsupol.so /system/lib/libsupol.so
$BB cp /sbin/su /system/bin/.ext/.su
$BB cp /res/ext/install-recovery.sh /system/etc/install-recovery.sh
$BB cp /res/ext/99SuperSUDaemon /system/etc/init.d/99SuperSUDaemon
$BB echo /system/etc/.installed_su_daemon

$BB chown 0.0 /system/bin/.ext
$BB chmod 0777 /system/bin/.ext
$BB chown 0.0 /system/xbin/su
$BB chmod 6755 /system/xbin/su
$BB chown 0.0 /system/xbin/daemonsu
$BB chmod 6755 /system/xbin/daemonsu
$BB chown 0.0 /system/bin/.ext/.su
$BB chmod 6755 /system/bin/.ext/.su
$BB chown 0.0 /system/etc/install-recovery.sh
$BB chmod 0755 /system/etc/install-recovery.sh
$BB chown 0.0 /system/etc/.installed_su_daemon
$BB chmod 0644 /system/etc/.installed_su_daemon
$BB chown 0.0 /system/lib/libsupol.so
$BB chmod 0644 /system/lib/libsupol.so

$BB chattr +i /system/xbin/su
$BB chattr +i /system/bin/.ext/.su
$BB chattr +i /system/xbin/daemonsu

$BB /system/bin/sh /system/etc/install-recovery.sh

fi

$BB sleep 1

# Aplicar Fstrim
$BB /sbin/fstrim -v /data
$BB /sbin/fstrim -v /cache
$BB /sbin/fstrim -v /system

$BB sync

$BB sleep 2

# Fix Permisos
$BB chmod 0644 /data/app/*.apk
$BB chown 1000:1000 /data/app/*.apk
$BB chmod 0644 /system/app/*.apk
$BB chown 0:0 /system/app/*.apk
$BB chmod 0644 /system/framework/*.apk
$BB chown 0:0 /system/framework/*.apk
$BB chmod 0644 /system/framework/*.jar
$BB chown 0:0 /system/framework/*.jar
$BB chmod 0755 /system/etc/init.d/*
$BB chmod 0777 /data/log/
$BB chmod 0777 /data/log/*

$BB sync

$BB mount -t rootfs -o remount,ro rootfs
$BB mount -o remount,ro -t auto /system

