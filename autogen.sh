#!/bin/sh
#
echo "Generating build information using aclocal and autoconf"
echo "This may take a while ..."

# Regenerate configuration files
(aclocal && autoconf) || exit $?
(cd test; aclocal; autoconf)

# Run configure for this platform
echo "Now you are ready to run ./configure"
