#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TCDBGEN], [1.0.1], [Jun@telechips.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/TCDBGen.c])
AC_CONFIG_MACRO_DIR([m4])

#AC_PREFIX_DEFAULT(/usr)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_LIBTOOL

# Checks PKG-CONFIG
PKG_CHECK_MODULES([TCDB], [sqlite3 libxml-2.0])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

AC_ARG_VAR(ARCH, [Set arm architecture])

AM_CONDITIONAL(AARCH_64, [test x$ARCH = xarm64])

AC_OUTPUT([Makefile])
