cmd_drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o := arm-eabi-gcc -Wp,-MD,drivers/misc/mediatek/base/power/cpuidle_v1/.cpu_dormant-v7.o.d  -nostdinc -isystem /home/kirito9/android/toolchain/arm-eabi-4.8/bin/../lib/gcc/arm-eabi/4.8/include -I./arch/arm/include -Iarch/arm/include/generated  -Iinclude -I./arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I./include/uapi -Iinclude/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-mediatek/include -I./drivers/misc/mediatek/include/mt-plat/mt6580/include -I./arch/arm/mach-mediatek/include/  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfp -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -Wa,-gdwarf-2              -c -o drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.S

source_drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o := drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.S

deps_drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o := \
  arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o: $(deps_drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o)

$(deps_drivers/misc/mediatek/base/power/cpuidle_v1/cpu_dormant-v7.o):
