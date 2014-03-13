INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LIBS = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

#STEP 1: add new tests to this line
TESTS = check_stack check_list

#STEP 2: add new tests to the following list
check_stack_SOURCES = check_stack.c
check_list_SOURCES = check_list.c

AM_LDADD = $(LIBS)

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0 $(INCLUDE)
else
        AM_CFLAGS = -O2 $(INCLUDE)
endif
