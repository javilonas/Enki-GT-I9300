#!/bin/bash

TOOLCHAIN="/home/lonas/Kernel_Lonas/toolchains/arm-eabi-4.4.3/bin/arm-eabi-"
DIR="/home/lonas/Kernel_Lonas/Enki-GT-I9300"

echo "#################### Eliminando Restos ####################"
if [ -e boot.img ]; then
	rm boot.img
fi

if [ -e boot.dt.img ]; then
	rm bboot.dt.img
fi

if [ -e compile.log ]; then
	rm compile.log
fi

if [ -e ramdisk.cpio ]; then
	rm ramdisk.cpio
fi

if [ -e ramdisk.cpio.gz ]; then
        rm ramdisk.cpio.gz
fi

#make distclean
make clean && make mrproper
rm Module.symvers > /dev/null 2>&1

# clean ccache
read -t 6 -p "Eliminar ccache, (y/n)?"
if [ "$REPLY" == "y" ]; then
	ccache -C
fi

echo "ramfs_tmp = $RAMFS_TMP"

echo "#################### Eliminando build anterior ####################"

echo "Cleaning latest build"
make ARCH=arm CROSS_COMPILE=$TOOLCHAIN -j`grep 'processor' /proc/cpuinfo | wc -l` mrproper
make ARCH=arm CROSS_COMPILE=$TOOLCHAIN -j`grep 'processor' /proc/cpuinfo | wc -l` clean

find -name '*.ko' -exec rm -rf {} \;
rm -f $DIR/releasetools/tar/*.tar > /dev/null 2>&1
rm -f $DIR/releasetools/zip/*.zip > /dev/null 2>&1
rm -rf $DIR/arch/arm/boot/zImage > /dev/null 2>&1
rm -rf $DIR/arch/arm/boot/zImage-dtb > /dev/null 2>&1
rm -f $DIR/arch/arm/boot/*.dtb > /dev/null 2>&1
rm -f $DIR/arch/arm/boot/*.cmd > /dev/null 2>&1
rm -rf $DIR/arch/arm/boot/Image > /dev/null 2>&1
rm $DIR/boot.img > /dev/null 2>&1
rm $DIR/zImage > /dev/null 2>&1
rm $DIR/zImage-dtb > /dev/null 2>&1
rm $DIR/boot.dt.img > /dev/null 2>&1
rm $DIR/dt.img > /dev/null 2>&1
