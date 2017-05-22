#!/bin/bash

export USE_CCACHE=1
export ARCH=arm ARCH_MTK_PLATFORM=mt6580
make clean 
make pixi4_5_defconfig
make -j4
