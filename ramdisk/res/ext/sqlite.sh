#!/sbin/busybox sh
#
#SQlite Speed Boost - by Javilonas
#

BB=/sbin/busybox

sleep 1
for i in \
`$BB find /data -iname "*.db"`; 
do \
	/sbin/sqlite3 $i 'VACUUM;';
	/sbin/sqlite3 $i 'REINDEX;';
done;
if [ -d "/data/data" ]; then
	for i in \
	`$BB find /data/data -iname "*.db"`; 
	do \
		/sbin/sqlite3 $i 'VACUUM;';
		/sbin/sqlite3 $i 'REINDEX;';
	done;
fi;
for i in \
`$BB find /sdcard -iname "*.db"`; 
do \
	/sbin/sqlite3 $i 'VACUUM;';
	/sbin/sqlite3 $i 'REINDEX;';
done;
