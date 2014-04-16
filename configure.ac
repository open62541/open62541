dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(Open62541, 1.0)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()
AC_PROG_MAKE_SET
#default CFLAGS is -g -02
#reset it just to -g and make optimization dependend on --enable-debug
if test -z $CFLAGS; then
    CFLAGS='-g'
fi
AM_PATH_PYTHON
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_CC_C_O
AC_CHECK_LIB([m],[exp],,AC_MSG_ERROR([Libm missing]))
PKG_CHECK_MODULES([CHECK], [check >= 0.9.12],[have_check="yes"],AC_MSG_WARN([Check not found or check version lower than 0.9.12; cannot run unit tests!])
[have_check="no"])
AM_CONDITIONAL([HAVE_CHECK], [test x"$have_check" = "xyes"])
AM_CONDITIONAL([TARGET_WIN],[test "${host_os}" = "mingw32"])
AM_CONDITIONAL([TARGET_LINUX],[test "${host_os}" = "linux-gnu" || test "${host_os}" = "linux"]) 
#adding platform-dependent information to compile flags
AM_COND_IF([TARGET_WIN],
    AC_DEFINE([WINDOWS])) #define WINDOWS is accessible from pre-processor

AM_COND_IF([TARGET_LINUX],
    AC_DEFINE([LINUX]))
   
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
			   [case "${enableval}" in
               		 yes) debug=true ;;
             		 no)  debug=false ;;
             		 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;; esac],
				[debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF([DEBUG],
    AC_DEFINE([DEBUG])) #define DEBUG is accessible from pre-processor

AC_ARG_ENABLE(multithreading,
AS_HELP_STRING([--enable-multithreading],
               [enable multithreading, default: yes]),
			   [case "${enableval}" in
               		 yes) multithreading=true ;;
             		 no)  multithreading=false ;;
             		 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-multithreading]) ;; esac],
[multithreading=false])
AM_CONDITIONAL(MULTITHREADING, test x"$multithreading" = x"true")
AM_COND_IF([MULTITHREADING],
    AC_DEFINE([MULTITHREADING])) #define MULTITHREADING is accessible from pre-processor

AC_ARG_ENABLE(doxygen,
	AS_HELP_STRING([--enable-doxygen],
               [enable doxygen, default: no]),
			   [case "${enableval}" in
               		 yes) enabledoxygen=true ;;
             		 no)  enabledoxygen=false ;;
             		 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-doxygen]) ;; esac],
[enabledoxygen=false])

AM_CONDITIONAL(ENABLE_DOXYGEN, test x"$enabledoxygen" = x"true")
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN],[test -n "$DOXYGEN"])

AM_COND_IF([ENABLE_DOXYGEN],[AM_COND_IF([HAVE_DOXYGEN],[usedoxygen=true],[usedoxygen=false])],[usedoxygen=false])

#if ENABLE_DOXYGEN;then
   # if HAVE_DOXYGEN;then
   #     usedoxygen = true
   # else
  #      usedoxygen = false
 #  fi
#else
 #  usedoxygen = false
#fi
AM_CONDITIONAL([USE_DOXYGEN], test x"$usedoxygen" = x"true")

AM_COND_IF([USE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])





    
if test -z "$DOXYGEN";then 
    AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi







   
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES(Makefile include/Makefile src/Makefile tool/Makefile tests/Makefile examples/src/Makefile doc/Makefile)
AC_OUTPUT

