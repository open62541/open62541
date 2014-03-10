TESTS = check_stack
check_PROGRAMS = check_stack
check_stack_SOURCES = check_stack.c

check_stack_CFLAGS = @CHECK_CFLAGS@
check_stack_LDADD = $(top_builddir)/src/libopen62541.la @CHECK_LIBS@

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0
else
        AM_CFLAGS = -O2
endif
