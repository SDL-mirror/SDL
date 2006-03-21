#!/bin/sh
#
echo "Generating build information using autoconf"
echo "This may take a while ..."

# Regenerate configuration files
cp acinclude.m4 aclocal.m4
for autoconf in autoconf autoconf259
do if which $autoconf >/dev/null; then $autoconf; break; fi
done
(cd test; sh autogen.sh)

# Run configure for this platform
echo "Now you are ready to run ./configure"
