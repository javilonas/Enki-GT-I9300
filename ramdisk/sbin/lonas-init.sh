#!/sbin/busybox sh
#
# Script inicio lonas-init.sh
#

BB=/sbin/busybox

# Inicio
$BB mount -o remount,rw -t auto /system
$BB mount -t rootfs -o remount,rw rootfs

# Detectar y generar INIT.D
/res/ext/init_d.sh

# Limpiador
/res/ext/limpiador.sh

# Iniciar SQlite
/res/ext/sqlite.sh

# Iniciar Zipalign
/res/ext/zipalign.sh

# Iniciar Tweaks Lonas_KL
/res/ext/tweaks.sh

# Iniciar Sensor
/res/ext/sensors.sh

# Iniciar MTP/adb
/res/ext/usb_mtp.sh

# Iniciar Init.d
/res/ext/init_d2.sh

$BB sync

$BB mount -t rootfs -o remount,ro rootfs
$BB mount -o remount,ro -t auto /system
