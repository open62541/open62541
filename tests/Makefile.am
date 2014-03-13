INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LIBS = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

#add new tests to this line
TESTS = check_stack check_list

check_PROGRAMS = $(TESTS)
AM_LDADD = $(LIBS)

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0 $(INCLUDE)
else
        AM_CFLAGS = -O2 $(INCLUDE)
endif
