TESTS = check_stack check_list check_indexedList check_generated check_namespace

# --- no changes beyond this line needed ---
INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

check_PROGRAMS = $(TESTS)
AM_LDFLAGS = $(LDADD)

AM_CFLAGS = $(GLOBAL_AM_CFLAGS) $(INCLUDE)
