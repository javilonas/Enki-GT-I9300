#!/sbin/busybox sh
#
# Iniciar Init.d en Android 4.4.4 correctamente - by Javilonas
#

export PATH=/sbin:/system/sbin:/system/bin:/system/xbin

if [ -d /system/etc/init.d ]; then
if cd /system/etc/init.d >/dev/null 2>&1 ; then
	for file in * ; do
		if ! cat "$file" >/dev/null 2>&1 ; then continue ; fi
		if [[ "$file" == *zipalign* ]]; then continue ; fi
		/system/bin/sh "$file"
	done
fi
  /sbin/busybox /sbin/run-parts /system/etc/init.d
fi

