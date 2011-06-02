#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([runner], [0.01], [markus.kauppila@gmail.com])
AC_CONFIG_SRCDIR([runner.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR(acinclude)
AC_CONFIG_AUX_DIR(build-scripts)

AM_INIT_AUTOMAKE

#libdir="$(pwd)/tests"
#AC_SUBST([libdir])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# without this debugging information will be stripped (at least on OS X)
CFLAGS="-g"

# Checks for library functions.
AC_FUNC_FORK

AC_CONFIG_FILES([Makefile
				 tests/Makefile testrect/Makefile])
AC_OUTPUT

echo ""
echo "========================================"
echo ""
echo "./configure ready!"
echo "you're ready to run: 'make && make install'"
