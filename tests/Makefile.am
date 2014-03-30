TESTS = check_stack check_list check_indexedList check_generated check_namespace check_encode check_decode check_create check_delete
TESTS_ENVIRONMENT=

TESTS_ENVIRONMENT_MEM="libtool --mode=execute valgrind --leak-check=full"

.PHONY: check-mem
check-mem:
	$(MAKE) $(AM_MAKEFLAGS) check TESTS=$(TESTS) TESTS_ENVIRONMENT=$(TESTS_ENVIRONMENT_MEM)
	
# --- no changes beyond this line needed ---
INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

check_PROGRAMS = $(TESTS)
AM_LDFLAGS = $(LDADD)  --coverage

AM_CFLAGS = $(GLOBAL_AM_CFLAGS) $(INCLUDE) --coverage

