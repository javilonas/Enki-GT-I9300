#!/sbin/busybox sh
#
# usb connection workaround, neeed because of broken default.prop execution
#
if [ -f /data/property/persist.sys.usb.config ] ; then
	if grep -q mtp /data/property/persist.sys.usb.config; then
		echo "mtp should be working already"
	else
		setprop persist.sys.usb.config mtp,adb
	fi
else
	setprop persist.sys.usb.config mtp,adb
fi
