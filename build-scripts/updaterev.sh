#!/bin/sh
#
# Generate a header file with the current source revision

outdir=`pwd`
cd `dirname $0`
srcdir=..
header=$outdir/include/SDL_revision.h

rev=`sh showrev.sh 2>/dev/null`
if [ "$rev" != "" -a "$rev" != "hg-0:baadf00d" ]; then
    echo "#define SDL_REVISION \"$rev\"" >$header.new
    if diff $header $header.new >/dev/null 2>&1; then
        rm $header.new
    else
        mv $header.new $header
    fi
fi
