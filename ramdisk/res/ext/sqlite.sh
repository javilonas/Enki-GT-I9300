#!/sbin/busybox sh
#
#SQlite Speed Boost - by Javilonas
#

sleep 1
for i in \
`/sbin/busybox find /data -iname "*.db"`; 
do \
	/sbin/sqlite3 $i 'VACUUM;';
	/sbin/sqlite3 $i 'REINDEX;';
done;
if [ -d "/data/data" ]; then
	for i in \
	`/sbin/busybox find /data/data -iname "*.db"`; 
	do \
		/sbin/sqlite3 $i 'VACUUM;';
		/sbin/sqlite3 $i 'REINDEX;';
	done;
fi;
for i in \
`/sbin/busybox find /sdcard -iname "*.db"`; 
do \
	/sbin/sqlite3 $i 'VACUUM;';
	/sbin/sqlite3 $i 'REINDEX;';
done;
