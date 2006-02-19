#!/bin/sh
#
echo "Generating build information using autoconf"
echo "This may take a while ..."

# Regenerate configuration files
cp acinclude.m4 aclocal.m4
autoconf
# FIXME
#(cd test; aclocal; autoconf)

# Run configure for this platform
echo "Now you are ready to run ./configure"
