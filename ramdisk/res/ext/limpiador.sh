#!/sbin/busybox sh
#
# Limpiador cache- by Javilonas
#

BB=/sbin/busybox

# remove cache, tmp, and unused files
$BB sync
$BB rm -f /cache/*.apk
$BB rm -f /cache/*.tmp
$BB rm -f /data/dalvik-cache/*.apk
$BB rm -f /data/dalvik-cache/*.tmp
$BB rm -f /data/data/com.google.android.gms/files/flog
$BB sync
$BB sleep 1
