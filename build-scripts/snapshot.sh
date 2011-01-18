#!/bin/sh
#
# Generate a current snapshot from source control

echo "Checking out source"
hg clone http://hg.libsdl.org/SDL || exit $?
(cd SDL && ./autogen.sh && rm -rf autom4te.cache)
sh SDL/build-scripts/updaterev.sh
cp SDL/include/SDL_config.h.default SDL/include/SDL_config.h

major=`fgrep "#define SDL_MAJOR_VERSION" SDL/include/SDL_version.h | \
       sed 's,[^0-9]*\([0-9]*\),\1,'`
minor=`fgrep "#define SDL_MINOR_VERSION" SDL/include/SDL_version.h | \
       sed 's,[^0-9]*\([0-9]*\),\1,'`
patch=`fgrep "#define SDL_PATCHLEVEL" SDL/include/SDL_version.h | \
       sed 's,[^0-9]*\([0-9]*\),\1,'`
rev=`fgrep "#define SDL_REVISION" SDL/include/SDL_revision.h | \
       sed 's,[^0-9]*\([0-9]*\):.*,\1,'`
path="SDL-$major.$minor.$patch-$rev"

mv SDL $path
echo $path.tar.gz
tar zcf $path.tar.gz $path
echo $path.zip
rm -f $path.zip
zip -rq $path.zip $path
rm -rf $path
