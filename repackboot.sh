#!/bin/bash

time=$(date +"%Y%m%d-%H%M");
cd ~/android/kernel/5010/boot
cp ../kernel/arch/arm/boot/zImage-dtb boot/split_img/boot.img-zImage
./repackimg.sh
rm -r ramdisk-new.cpio.gz
mv image-new.img boot_$time.img
echo -e "Your new boot is boot_$time.";
