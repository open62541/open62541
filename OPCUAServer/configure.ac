dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(OPCUAServer, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()
AC_PROG_MAKE_SET
if test -z $CFLAGS; then
    CFLAGS='-O2'
fi
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_CC_C_O
AC_CHECK_LIB([m],[exp],,AC_MSG_ERROR([Libm missing]))
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],[have_check="yes"],AC_MSG_WARN([Check not found; cannot run unit tests!])
[have_check="no"])
AM_CONDITIONAL([HAVE_CHECK], [test x"$have_check" = "xyes"])
AM_CONDITIONAL([TARGET_WIN],[test "${host_os}" = "mingw32"])
AM_CONDITIONAL([TARGET_LINUX],[test "${host_os}" = "linux-gnu" || test "${host_os}" = "linux"]) 
#adding platform-dependent information to compile flags
AM_COND_IF([TARGET_WIN],
    AC_DEFINE([WINDOWS])) #define WINDOWS is accessible from pre-processor

AM_COND_IF([TARGET_LINUX],
    AC_DEFINE([LINUX]))
    
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES(Makefile src/Makefile tests/Makefile examples/src/Makefile)
AC_OUTPUT

