#!/sbin/busybox sh
#
# Limpiador cache- by Javilonas
#

# remove cache, tmp, and unused files
busybox sync
busybox rm -f /cache/*.apk
busybox rm -f /cache/*.tmp
busybox rm -f /data/dalvik-cache/*.apk
busybox rm -f /data/dalvik-cache/*.tmp
busybox rm -f /data/data/com.google.android.gms/files/flog
busybox sync
busybox sleep 1
