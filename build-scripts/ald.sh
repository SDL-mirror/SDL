#!/bin/bash
ANDROID_NDK="/home/paul/Projects/gsoc/sdk/android-ndk-r4"
TOOLS_PATH="$ANDROID_NDK/build/prebuilt/linux-x86/arm-eabi-4.2.1/bin"
ADDITIONAL_LIBS=`dirname "$0"`/android_libs/

export PATH=$TOOLS_PATH:$PATH

LD="arm-eabi-ld"

#ldflags
ACC_L="	-rpath-link=$ANDROID_NDK/build/platforms/android-8/arch-arm/usr/lib/ \
		-dynamic-linker=/system/bin/linker \
		-lc -nostdlib \
 		$ANDROID_NDK/build/platforms/android-8/arch-arm/usr/lib/crtbegin_static.o \
 		-L$ANDROID_NDK/build/platforms/android-8/arch-arm/usr/lib/ \
 		-L$ANDROID_NDK/build/prebuilt/linux-x86/arm-eabi-4.2.1/lib/gcc/arm-eabi/4.2.1 \
 		-L$ADDITIONAL_LIBS "
		
$LD $ACC_L $LDFLAGS $@ -lgcc

