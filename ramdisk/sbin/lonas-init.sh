#!/sbin/busybox sh
#
# Script inicio lonas-init.sh
#

# Inicio
busybox mount -o remount,rw -t auto /system
busybox mount -t rootfs -o remount,rw rootfs

# Detectar y generar INIT.D
/res/ext/init_d.sh

# Iniciar Bootanimation personalizado
/res/ext/bootanimation.sh

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

busybox sync

busybox mount -t rootfs -o remount,ro rootfs
busybox mount -o remount,ro -t auto /system
