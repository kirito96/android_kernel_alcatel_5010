#!/bin/bash
#Original script by hellsgod and modified with parts from MSF-Jarvis and The Flash.


# Bash colors
green='\033[01;32m'
red='\033[01;31m'
cyan='\033[01;36m'
blue='\033[01;34m'
blink_red='\033[05;31m'
restore='\033[0m'

clear

# Paths
KERNEL_DIR=$(pwd)
RESOURCE_DIR="${KERNEL_DIR}"
ANYKERNEL_DIR="$KERNEL_DIR/Blackthunder-AK2"
ANYKERNEL_BRANCH=pixi4_5
REPACK_DIR="$ANYKERNEL_DIR"
ZIMAGE_DIR="$KERNEL_DIR/arch/arm/boot"

# Resources
THREAD="-j4"
KERNEL="zImage-dtb"
DEFCONFIG="pixi4_5_defconfig"
TOOLCHAIN_PREFIX=arm-eabi
TOOLCHAIN_DIR=${TOOLCHAIN_PREFIX}

# BlackThunder Kernel Details
KERNEL_NAME="BlackThunder"
VER="v1.0"
LOCALVERSION="-$( date +%Y%m%d )"
BASE_BT_VER="BT"
BT_VER="$BASE_BT_VER$VER${LOCALVERSION}-$( date +%H%M )"

# Configure build
export ARCH=arm
export CROSS_COMPILE="${TOOLCHAIN_DIR}/bin/${TOOLCHAIN_PREFIX}-"

# Clean AnyKernel directory if it exists, clone it if not
if [[ -d ${ANYKERNEL_DIR} ]]; then
    cd ${ANYKERNEL_DIR}
    git checkout ${ANYKERNEL_BRANCH}
    git fetch origin
    git reset --hard origin/${ANYKERNEL_BRANCH}
    git clean -fdx > /dev/null 2>&1
    rm -rf ${KERNEL} > /dev/null 2>&1
else
	cd ${KERNEL_DIR}
    git clone https://github.com/kirito96/AnyKernel2 -b ${ANYKERNEL_BRANCH} Blackthunder-AK2
fi

# If the toolchain directory doesn't exist, clone it
if [[ ! -d ${TOOLCHAIN_DIR} ]]; then
    cd ${KERNEL_DIR}
    git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8 ${TOOLCHAIN_PREFIX}
fi

# Functions
function clean_all {
		rm -rf $REPACK_DIR/zImage-dtb
		rm -rf $ZIMAGE_DIR/$KERNEL
		make clean && make mrproper
}

function make_kernel {
		make $DEFCONFIG
		make $THREAD
		cp -vr $ZIMAGE_DIR/$KERNEL $REPACK_DIR
}

function make_zip {
		cd $REPACK_DIR
		zip -9 -r `echo $BT_VER`.zip .
		mv  `echo $BT_VER`.zip $REPACK_DIR
		cd $KERNEL_DIR
}


function push_and_flash {
  adb push "$REPACK_DIR"/${BT_VER}.zip /sdcard/
  adb shell twrp install "/sdcard/${BT_VER}.zip"
}

DATE_START=$(date +"%s")

echo -e "${green}"
echo "BlackThunder Kernel Creation Script:"
echo

echo "---------------"
echo "Kernel Version:"
echo "---------------"

echo -e "${red}"; echo -e "${blink_red}"; echo "$BT_VER"; echo -e "${restore}";

echo -e "${green}"
echo "-----------------"
echo "Making BlackThunder Kernel:"
echo "-----------------"
echo -e "${restore}"

while read -p "Please choose your option: [1]Clean-build // [2]Dirty-build // [3]Clean source // [4]Make flashable zip // [5]Flash to device // [6]Abort " cchoice
do
case "$cchoice" in
	1 )
		echo -e "${green}"
		echo
		echo "[..........Cleaning up..........]"
		echo
		echo -e "${restore}"
		clean_all
		echo -e "${green}"
		echo
		echo "[....Building `echo $BT_VER`....]"
		echo
		echo -e "${restore}"
		make_kernel
		if [ -f $ZIMAGE_DIR/$KERNEL ];
		then
			make_zip
		else
			echo -e "${red}"
			echo
			echo "Kernel build failed, check your logs!"
			echo
			echo -e "${restore}"
		fi
		echo
		echo
		echo -e "${restore}"
		break
		;;
	2 )
		echo -e "${green}"
		echo
		echo "[....Building `echo $BT_VER`....]"
		echo
		echo -e "${restore}"
		make_kernel
		if [ -f $ZIMAGE_DIR/$KERNEL ];
		then
			make_zip
		else
			echo -e "${red}"
			echo
			echo "Kernel build failed, check your logs!"
			echo
			echo -e "${restore}"
		fi
		echo		
		echo
		echo -e "${restore}"
		break
		;;
	3 )
		echo -e "${green}"
		echo
		echo "[..........Cleaning up..........]"
		echo
		echo -e "${restore}"
		clean_all
		echo -e "${green}"
		echo
		echo -e "${restore}"		
		break
		;;
	4 )
		echo -e "${green}"
		echo
		echo "[....Make `echo $BT_VER`.zip....]"
		echo
		echo -e "${restore}"
		make_zip
		echo -e "${green}"
		echo
		echo "[.....Moving `echo $BT_VER`.....]"
		echo
		echo -e "${restore}"
		break
		;;
	5 )
		echo -e "${green}"
		echo
		echo "[....Pushing and Flashing `echo $BT_VER`.zip....]"
		echo
		echo -e "${restore}"
		push_and_flash
		echo -e "${green}"
		echo
		echo "[.....Pushing and Flashing `echo $BT_VER`.....]"
		echo
		echo -e "${restore}"
		break
		;;		
	6 )
		echo 
		echo
		cd ${HOME}
		break
		;;
	* )
		echo -e "${red}"
		echo
		echo "Invalid try again!"
		echo
		echo -e "${restore}"
		;;
esac
done


if [ -f $ZIMAGE_DIR/$KERNEL ];
then
	echo -e "${green}"
	echo "-------------------"
	echo "Build Completed in:"
	echo "-------------------"
	echo -e "${restore}"
fi


DATE_END=$(date +"%s")
DIFF=$(($DATE_END - $DATE_START))

echo -e "SCRIPT DURATION: $((${DIFF} / 60)) MINUTES AND $((${DIFF} % 60)) SECONDS"
if [ -f $ZIMAGE_DIR/$KERNEL ]; then
    echo -e "ZIP LOCATION: ${ANYKERNEL_DIR}/${BT_VER}.zip"
    echo -e "SIZE: $( du -h ${ANYKERNEL_DIR}/${BT_VER}.zip | awk '{print $1}' )"
fi
echo -e ${RESTORE}
echo
