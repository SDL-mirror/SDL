#!/bin/sh
#
aclocal
automake --foreign
autoconf

# Run configure for this platform
#./configure $*
echo "Now you are ready to run ./configure"
