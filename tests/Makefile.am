TESTS = check_main
check_PROGRAMS = check_main
check_main_SOURCES = 	check_main.c\
						check_stack.c

check_main_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
check_main_LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0
else
        AM_CFLAGS = -O2
endif
