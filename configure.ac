#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libae], [1.0], [binape@126.com])

AC_CONFIG_AUX_DIR(examples)

AC_CONFIG_SRCDIR([ae.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE
LT_INIT

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Search for libraries
#AC_SEARCH_LIBS([malloc], [jemalloc], [
#  AC_MSG_ERROR([unable to find the jemalloc library])
#])

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday strtol])

AC_OUTPUT(Makefile examples/Makefile)
