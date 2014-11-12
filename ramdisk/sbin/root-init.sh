#!/sbin/busybox sh
#
# Script root
#

SYSTEM_DEVICE="/dev/block/mmcblk0p9"

# Inicio
/sbin/busybox mount -o remount,rw -t ext4 $SYSTEM_DEVICE /system
/sbin/busybox mount -t rootfs -o remount,rw rootfs

/sbin/busybox rm /system/bin/reboot
/sbin/busybox ln -s /sbin/toolbox /system/bin/reboot

if [ ! -f /system/xbin/busybox ]; then
/sbin/busybox ln -s /sbin/busybox /system/xbin/busybox
/sbin/busybox ln -s /sbin/busybox /system/xbin/pkill
fi

if [ ! -f /system/bin/busybox ]; then
/sbin/busybox ln -s /sbin/busybox /system/bin/busybox
/sbin/busybox ln -s /sbin/busybox /system/bin/pkill
fi

# Hacer root si no detecta bianrio SU
if [ ! -f /system/xbin/su ] && [ ! -f /system/bin/su ]; then

/sbin/busybox mkdir /system/bin/.ext
/sbin/busybox cp /sbin/su /system/xbin/su
/sbin/busybox cp /sbin/daemonsu /system/xbin/daemonsu
/sbin/busybox cp /sbin/su /system/bin/.ext/.su
/sbin/busybox cp /res/ext/install-recovery.sh /system/etc/install-recovery.sh
/sbin/busybox cp /res/ext/99SuperSUDaemon /system/etc/init.d/99SuperSUDaemon
/sbin/busybox echo /system/etc/.installed_su_daemon

/sbin/busybox chown 0.0 /system/bin/.ext
/sbin/busybox chmod 0777 /system/bin/.ext
/sbin/busybox chown 0.0 /system/xbin/su
/sbin/busybox chmod 6755 /system/xbin/su
/sbin/busybox chown 0.0 /system/xbin/daemonsu
/sbin/busybox chmod 6755 /system/xbin/daemonsu
/sbin/busybox chown 0.0 /system/bin/.ext/.su
/sbin/busybox chmod 6755 /system/bin/.ext/.su
/sbin/busybox chown 0.0 /system/etc/install-recovery.sh
/sbin/busybox chmod 0755 /system/etc/install-recovery.sh
/sbin/busybox chown 0.0 /system/etc/.installed_su_daemon
/sbin/busybox chmod 0644 /system/etc/.installed_su_daemon

/sbin/busybox chattr +i /system/xbin/su
/sbin/busybox chattr +i /system/bin/.ext/.su
/sbin/busybox chattr +i /system/xbin/daemonsu

/system/bin/sh /system/etc/install-recovery.sh

fi

/sbin/busybox sleep 1

# Aplicar Fstrim
/sbin/fstrim -v /data
/sbin/fstrim -v /cache
/sbin/fstrim -v /system

/sbin/busybox sync

/sbin/busybox sleep 2

# Fix Permisos
/sbin/busybox chmod 644 /data/app/*.apk
/sbin/busybox chown 1000:1000 /data/app/*.apk
/sbin/busybox chmod 644 /system/app/*.apk
/sbin/busybox chown 0:0 /system/app/*.apk
/sbin/busybox chmod 644 /system/framework/*.apk
/sbin/busybox chown 0:0 /system/framework/*.apk
/sbin/busybox chmod 644 /system/framework/*.jar
/sbin/busybox chown 0:0 /system/framework/*.jar

/sbin/busybox sync

/sbin/busybox mount -t rootfs -o remount,ro rootfs
/sbin/busybox mount -o remount,ro -t ext4 $SYSTEM_DEVICE /system

