#!/bin/sh
#
# Print the current source revision, if available

srcdir=`dirname $0`/..

if [ -d $srcdir/.svn ]; then
    cd $srcdir
    (svnversion -c 2>/dev/null || svnversion .) | \
        sed -e 's,\([0-9]*\)[A-Z]*,\1,' \
            -e 's,[0-9]*:\([0-9]*\)[A-Z]*,\1,'
else
     cd $srcdir
     git svn info | grep Revision | awk '{ print $2 }'
fi
