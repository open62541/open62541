TESTS = check_stack
check_PROGRAMS = check_stack
check_stack_SOURCES = check_stack.c $(top_builddir)/src/opcua_transportLayer.h\
						$(top_builddir)/src/opcua_advancedDatatypes.h\
						$(top_builddir)/src/opcua_connectionHelper.h

check_stack_CFLAGS = @CHECK_CFLAGS@
check_stack_LDADD = $(top_builddir)/bin/libOpen62541.la @CHECK_LIBS@
