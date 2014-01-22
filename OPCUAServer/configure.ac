dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(OPCUAServer, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()
AM_PATH_CHECK(,[have_check="yes"],AC_MSG_WARN([Check not found; cannot run unit tests!])
[have_check="no"])
AM_CONDITIONAL(HAVE_CHECK, test x"$have_check" = "xyes")
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES(Makefile src/Makefile tests/Makefile)
AC_OUTPUT

