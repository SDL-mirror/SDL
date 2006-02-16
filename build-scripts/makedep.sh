#!/bin/sh
#
# Generate dependencies from a list of source files

# Check to make sure our environment variables are set
if test x"$INCLUDE" = x -o x"$SOURCES" = x -o x"$objects" = x -o x"$output" = x; then
    echo "SOURCES, INCLUDE, objects, and output needs to be set"
    exit 1
fi
cache_prefix=".#$$"

generate_var()
{
    echo $1 | sed -e 's|^.*/||' -e 's|\.|_|g'
}

search_deps()
{
    base=`echo $1 | sed 's|/[^/]*$||'`
    grep '#include "' <$1 | sed -e 's|.*"\([^"]*\)".*|\1|' | \
    while read file
    do cache=${cache_prefix}_`generate_var $file`
       if test -f $cache; then
          # We already ahve this cached
          cat $cache
          continue;
       fi
       for path in $base `echo $INCLUDE | sed 's|-I||g'`
       do dep="$path/$file"
          if test -f "$dep"; then
             echo "	$dep \\" >$cache
             echo "	$dep \\"
             generate_dep $dep
             break
          fi
       done
    done
}

generate_dep()
{
    cat >>${output}.new <<__EOF__
$1:	\\
`search_deps $1`

__EOF__
}

:>${output}.new
for src in $SOURCES
do  echo "Generating dependencies for $src"
    generate_dep $src
    ext=`echo $src | sed 's|.*\.\(.*\)|\1|'`
    obj=`echo $src | sed "s|^.*/\([^ ]*\)\..*|$objects/\1.lo|g"`
    echo "$obj: $src" >>${output}.new
    case $ext in
        asm) echo "	\$(BUILDASM)" >>${output}.new;;
        cc)  echo "	\$(BUILDCC)" >>${output}.new;;
        c)   echo "	\$(BUILDC)" >>${output}.new;;
        m)   echo "	\$(BUILDM)" >>${output}.new;;
        *)   echo "Unknown file extension: $ext";;
    esac
    echo "" >>${output}.new
done
rm -f ${cache_prefix}*
mv ${output}.new ${output}
