#!/bin/bash
ANDROID_NDK="/home/paul/Projects/gsoc/sdk/android-ndk-r4"
TOOLS_PATH="$ANDROID_NDK/build/prebuilt/linux-x86/arm-eabi-4.2.1/bin"

export PATH=$TOOLS_PATH:$PATH

LD="arm-eabi-ld"

#ldflags
ACC_L="	-rpath-link=$ANDROID_NDK/build/platforms/android-4/arch-arm/usr/lib/ \
		-dynamic-linker=/system/bin/linker \
		-L$ANDROID_NDK/build/platforms/android-3/arch-arm/usr/lib/ -lc -nostdlib \
 		$ANDROID_NDK/build/platforms/android-4/arch-arm/usr/lib/crtbegin_static.o \
 		-L$ANDROID_NDK/build/prebuilt/linux-x86/arm-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1 "
		
$LD $ACC_L $LDFLAGS $@ -lgcc


