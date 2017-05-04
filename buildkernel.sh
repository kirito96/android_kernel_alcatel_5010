#!/bin/bash

# Get kernel configuration
if [ -f kernel.conf ]
  then
    source "kernel.conf"
  else
	echo "Kernel configuration file (kernel.conf) does not exist!"
	exit -1
fi

export PATH=$PATH:$TOOLCHAIN_PATH
export CROSS_COMPILE=arm-eabi-

#cd ~/android/kernel/5010
#export CROSS_COMPILE=~/android/toolchain/arm-eabi-4.8/bin/arm-eabi-
export USE_CCACHE=1
export ARCH=arm ARCH_MTK_PLATFORM=mt6580
#make clean 
make pixi4_5_defconfig
./build.sh


#make -C $PWD O=$PWD/out ARCH=arm pixi4_5_defconfig
#make -j$(getconf _NPROCESSORS_ONLN) -C $PWD O=$PWD/out ARCH=arm

repack(){
if [-f repack.conf ]
	then
	  source "repack.conf"
	else 
	  echo "Repack configuration file (repack.conf) does not exist!"
	  exit -1
fi

time=$(date +"%Y%m%d-%H%M");
cd $PWD/boot
cp ../kernel/arch/arm/boot/zImage-dtb boot/split_img/boot.img-zImage
./repackimg.sh
rm -r ramdisk-new.cpio.gz
mv image-new.img boot_$time.img
echo -e "Your new boot is boot_$time.";
}

