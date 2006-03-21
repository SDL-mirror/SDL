#!/bin/sh
#
# Regenerate configuration files
cp acinclude.m4 aclocal.m4
for autoconf in autoconf autoconf259
do if which $autoconf >/dev/null; then $autoconf; break; fi
done
