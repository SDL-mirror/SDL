#!/bin/bash
ANDROID_NDK="/home/paul/Projects/gsoc/sdk/android-ndk-r4"
TOOLS_PATH="$ANDROID_NDK/build/prebuilt/linux-x86/arm-eabi-4.2.1/bin"

export PATH=$TOOLS_PATH:$PATH

CC="arm-eabi-gcc"

#cflags
ACC_C="	-I$ANDROID_NDK/build/platforms/android-8/common/include \
		-I$ANDROID_NDK/build/platforms/android-8/arch-arm/usr/include \
		-DANDROID -DANDROID_NDK -c"
		
		
$CC $CFLAGS $ACC_C $@
