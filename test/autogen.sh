#!/bin/sh
#
aclocal
automake --foreign --add-missing
autoconf

# Run configure for this platform
#./configure $*
echo "Now you are ready to run ./configure"
