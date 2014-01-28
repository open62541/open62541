dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(OPCUAServer, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()
AC_PROG_CC
AM_PROG_CC_C_O
AC_CHECK_LIB([m],[exp],,AC_MSG_ERROR([Libm missing]))
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],[have_check="yes"],AC_MSG_WARN([Check not found; cannot run unit tests!])
[have_check="no"])
AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes")
AM_CONDITIONAL([TARGET_WIN],[test "${host_os}" = "mingw32"])
AM_CONDITIONAL([TARGET_LINUX],[test "${host_os}" = "linux-gnu" || test "${host_os}" = "linux"]) 
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES(Makefile src/Makefile tests/Makefile)
AC_OUTPUT

