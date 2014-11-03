#!/sbin/busybox sh
#
# Limpiador cache- by Javilonas
#

# remove cache, tmp, and unused files
/sbin/busybox sync
/sbin/busybox rm -f /cache/*.apk
/sbin/busybox rm -f /cache/*.tmp
/sbin/busybox rm -f /data/dalvik-cache/*.apk
/sbin/busybox rm -f /data/dalvik-cache/*.tmp
/sbin/busybox rm -f /data/data/com.google.android.gms/files/flog
/sbin/busybox sync
/sbin/busybox sleep 1
