TESTS = check_stack
check_PROGRAMS = check_stack
check_stack_SOURCES = check_stack.c

check_stack_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
check_stack_LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0
else
        AM_CFLAGS = -O2
endif
