dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(Open62541, 1.0)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_MAKE_SET
# Default CFLAGS is -g -02. Set depending on debug flag.
if test -z $CFLAGS; then
   CFLAGS=''
fi
AM_PATH_PYTHON
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_CC_C_O
AC_CHECK_LIB([m],[exp],,AC_MSG_ERROR([Libm missing]))
PKG_CHECK_MODULES([CHECK], [check >= 0.9.12],[have_check="yes"],
                  AC_MSG_WARN([Check not found or check version lower than 0.9.12; cannot run unit tests!])
				  [have_check="no"])
AM_CONDITIONAL([HAVE_CHECK], [test x"$have_check" = "xyes"])
AM_CONDITIONAL([TARGET_WIN],[test "${host_os}" = "mingw32"])
AM_CONDITIONAL([TARGET_LINUX],[test "${host_os}" = "linux-gnu" || test "${host_os}" = "linux"]) 
#adding platform-dependent information to compile flags
AM_COND_IF([TARGET_WIN],
    AC_DEFINE([WINDOWS])) #define WINDOWS is accessible from pre-processor

AM_COND_IF([TARGET_LINUX],
    AC_DEFINE([LINUX]))
   
#debugging
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
	[case "${enableval}" in
        	yes) debug=true; verbose=false ;;
             	no)  debug=false; verbose=false ;;
             	verbose) debug=true; verbose=true ;;
             	*)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;; 
	esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF([DEBUG],
    AC_DEFINE([DEBUG])) #define DEBUG is accessible from pre-processor

AM_CONDITIONAL(VERBOSE, test x"$verbose" = x"true")
AM_COND_IF([VERBOSE],
    AC_DEFINE([VERBOSE])) #define VERBOSE is accessible from pre-processor

#profile
# 100: nano
# 200: embedded
# 900: all available features
AC_ARG_ENABLE(profile,
AS_HELP_STRING([--enable-profile], [select supported features, default: all]),
	[case "${enableval}" in
        	nano) profile=100;;
            embedded) profile=200;;
            all) profile=900;;
            *) AC_MSG_ERROR([bad value ${enableval} for --enable-profile]);; 
	esac],[profile=900])
AC_DEFINE([PROFILE], [x"$profile"])

#xml encoding
AM_CONDITIONAL(UA_ENCODING_XML, test x"$profile" -ge 200)
AM_COND_IF([UA_ENCODING_XML],
    AC_DEFINE([UA_ENCODING_XML], [1])
    AC_CHECK_LIB([expat],[XML_Parse],,AC_MSG_ERROR([Libexpat missing])))

#multithreading
AC_ARG_ENABLE(multithreading,
AS_HELP_STRING([--enable-multithreading],
               [enable multithreading, default: yes]),
	[case "${enableval}" in
		yes) multithreading=true ;;
		no)  multithreading=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-multithreading]) ;; 
	esac],[multithreading=false])
AM_CONDITIONAL(MULTITHREADING, test x"$multithreading" = x"true")
AM_COND_IF([MULTITHREADING],
    AC_DEFINE([MULTITHREADING])) #define MULTITHREADING is accessible from pre-processor

#coverage
AC_ARG_ENABLE(coverage,
AS_HELP_STRING([--enable-coverage],
               [enable coverage, default: no]),
	[case "${enableval}" in
		yes) coverage=true ;;
		no)  coverag=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-coverage]) ;; 
	esac],[coverage=false])
AM_CONDITIONAL(COVERAGE, test x"$coverage" = x"true")
AM_COND_IF([COVERAGE],
    AC_DEFINE([COVERAGE])) #define MULTITHREADING is accessible from pre-processor

#doxygen start
AC_ARG_ENABLE(doxygen,
AS_HELP_STRING([--enable-doxygen],
               [enable doxygen, default: no]),
	[case "${enableval}" in
		yes) enabledoxygen=true ;;
		no)  enabledoxygen=false ;;
             	*)   AC_MSG_ERROR([bad value ${enableval} for --enable-doxygen]) ;; 
	esac],[enabledoxygen=false])

AM_CONDITIONAL(ENABLE_DOXYGEN, test x"$enabledoxygen" = x"true")
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AC_CHECK_PROGS([DOT], [dot])

#break if doxygen enabled but not present
if test x"$enabledoxygen" = x"true"; then    
	if test -z "$DOXYGEN"; then 
    		AC_MSG_ERROR([Doxygen not found])
	fi
	if test -z "$DOT"; then 
    		AC_MSG_ERROR([Graphviz not found])
	fi
fi
AM_COND_IF([ENABLE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])
#doxygen end

#language wrappers
AC_ARG_ENABLE(python_wrapper,
AS_HELP_STRING([--enable-python-wrapper],
               [build python-wrapper, default: no]),
	[case "${enableval}" in
		yes) python_wrapper=true ;;
		no)  python_wrapper=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-python-wrapper]) ;; 
	esac],[python_wrapper=false])
AM_CONDITIONAL(PYTHON_WRAPPER, test x"$python_wrapper" = x"true")
AM_COND_IF([PYTHON_WRAPPER],
    AC_DEFINE([PYTHON_WRAPPER]))

AC_ARG_ENABLE(lua_wrapper,
AS_HELP_STRING([--enable-lua-wrapper],
               [build lua-wrapper, default: no]),
	[case "${enableval}" in
		yes) lua_wrapper=true ;;
		no)  lua_wrapper=false ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-lua-wrapper]) ;; 
	esac],[lua_wrapper=false])
AM_CONDITIONAL(LUA_WRAPPER, test x"$lua_wrapper" = x"true")
AM_COND_IF([LUA_WRAPPER],
    AC_DEFINE([LUA_WRAPPER]))

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES(Makefile src/Makefile tests/Makefile examples/src/Makefile doc/Makefile wrappers/Makefile wrappers/python/Makefile wrappers/lua/Makefile)
AC_OUTPUT

