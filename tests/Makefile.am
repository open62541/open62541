TESTS_ENVIRONMENT=
TESTS=
UNIT_TESTS=
check_PROGRAMS=

UNIT_TESTS += check_stack check_list check_indexedList check_generated check_namespace check_encode check_decode check_create check_delete
TESTS += $(UNIT_TESTS)
check_PROGRAMS += $(UNIT_TESTS)

TESTS_ENVIRONMENT_MEM="libtool --mode=execute valgrind --leak-check=full --quiet --suppressions=suppressions --gen-suppressions=no --error-exitcode=666"

.PHONY: check-mem
check-mem:
	$(MAKE) $(AM_MAKEFLAGS) check TESTS="$(UNIT_TESTS)" TESTS_ENVIRONMENT=$(TESTS_ENVIRONMENT_MEM)
		
# --- no changes beyond this line needed ---
INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

AM_LDFLAGS = $(LDADD)  --coverage

AM_CFLAGS = $(GLOBAL_AM_CFLAGS) $(INCLUDE) --coverage

