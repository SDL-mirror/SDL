#!/bin/sh
#
# Build a fat binary on Mac OS X, thanks Ryan!

# PowerPC compiler flags (10.2 runtime compatibility)
CFLAGS_PPC="-arch ppc \
-F/Developer/SDKs/MacOSX10.2.8.sdk/System/Library/Frameworks \
-I/Developer/SDKs/MacOSX10.2.8.sdk/Developer/Headers/FlatCarbon \
-DMAC_OS_X_VERSION_MIN_REQUIRED=1020 \
-I/Developer/SDKs/MacOSX10.2.8.sdk/usr/include/gcc/darwin/3.3 \
-I/Developer/SDKs/MacOSX10.2.8.sdk/usr/include/gcc/darwin/3.3/c++ \
-I/Developer/SDKs/MacOSX10.2.8.sdk/usr/include/gcc/darwin/3.3/c++/ppc-darwin \
-isystem /Developer/SDKs/MacOSX10.2.8.sdk/usr/include"

# PowerPC linker flags 
LFLAGS_PPC="-arch ppc -mmacosx-version-min=10.2 \
-L/Developer/SDKs/MacOSX10.2.8.sdk/usr/lib/gcc/darwin/3.3 \
-F/Developer/SDKs/MacOSX10.2.8.sdk/System/Library/Frameworks \
-Wl,-syslibroot,/Developer/SDKs/MacOSX10.2.8.sdk"

# Intel compiler flags (10.4 runtime compatibility)
CFLAGS_X86="-arch i386 -mmacosx-version-min=10.4 \
-DMAC_OS_X_VERSION_MIN_REQUIRED=1040 -isysroot /Developer/SDKs/MacOSX10.4u.sdk"

# Intel linker flags
LFLAGS_X86="-arch i386 -mmacosx-version-min=10.4 \
-L/Developer/SDKs/MacOSX10.4u.sdk/usr/lib/gcc/i686-apple-darwin8/4.0.0 \
-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk"

#
# Find the configure script
#
cd `dirname $0`/..

#
# Figure out which phase to build:
# all,
# configure, configure-ppc, configure-x86,
# make, make-ppc, make-x86, merge
# install
if test x"$1" = x; then
    phase=all
else
    phase="$1"
fi
case $phase in
    all)
        configure_ppc="yes"
        configure_x86="yes"
        make_ppc="yes"
        make_x86="yes"
        merge="yes"
        ;;
    configure)
        configure_ppc="yes"
        configure_x86="yes"
        ;;
    configure-ppc)
        configure_ppc="yes"
        ;;
    configure-x86)
        configure_x86="yes"
        ;;
    make)
        make_ppc="yes"
        make_x86="yes"
        merge="yes"
        ;;
    make-ppc)
        make_ppc="yes"
        ;;
    make-x86)
        make_x86="yes"
        ;;
    merge)
        merge="yes"
        ;;
    install)
        make_x86="yes"
        ;;
esac

#
# Create the build directories
#
for dir in build build/ppc build/x86; do
    if test -d $dir; then
        :
    else
        mkdir $dir || exit 1
    fi
done

#
# Build the PowerPC binary
#
if test x$configure_ppc = xyes; then
    (cd build/ppc && \
     sh ../../configure CFLAGS="$CFLAGS_PPC" LDFLAGS="$LFLAGS_PPC") || exit 2
fi
if test x$make_ppc = xyes; then
    (cd build/ppc && make) || exit 3
fi

#
# Build the Intel binary
#
if test x$configure_x86 = xyes; then
    (cd build/x86 && \
     sh ../../configure CFLAGS="$CFLAGS_X86" LDFLAGS="$LFLAGS_X86") || exit 2
fi
if test x$make_x86 = xyes; then
    (cd build/x86 && make) || exit 3
fi

#
# Combine into fat binary
#
target=`find x86 -type f -name '*.dylib' | sed 's|.*/||'`
if test x$merge = xyes; then
    (cd build && \
     lipo -create -o $target `find ppc x86 -type f -name "*.dylib"` &&
     ln -s $target libSDL-1.2.0.dylib
     ln -s $target libSDL.dylib
     lipo -create -o SDLMain.o */build/SDLMain.o &&
     ar cru libSDLmain.a SDLMain.o && ranlib libSDLmain.a &&
     echo "Build complete!" &&
     echo "Files can be found in the build directory.") || exit 4
fi

#
# Install
#
if test x$install = xyes; then
    echo "Install not implemented"
    exit 1
fi
