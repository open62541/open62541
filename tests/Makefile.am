TESTS_ENVIRONMENT=
TESTS=
UNIT_TESTS=
check_PROGRAMS=

UNIT_TESTS += check_stack check_list check_indexedList check_builtin check_namespace check_memory check_services_view check_base64
TESTS += $(UNIT_TESTS)
check_PROGRAMS += $(UNIT_TESTS)

TESTS_ENVIRONMENT_MEM="libtool --mode=execute valgrind --leak-check=full --quiet --suppressions=suppressions --gen-suppressions=no --error-exitcode=666"

cov-report:
	mkdir -p coverage
	lcov --compat-libtool --directory . --directory ../src --capture --output-file coverage/app.info  
	genhtml -o coverage/ coverage/app.info 

.PHONY: check-mem
check-mem:
	$(MAKE) $(AM_MAKEFLAGS) check TESTS="$(UNIT_TESTS)" TESTS_ENVIRONMENT=$(TESTS_ENVIRONMENT_MEM)
if COVERAGE
	$(MAKE) cov-report
endif

check_memory-mem: check_memory
	$(MAKE) $(AM_MAKEFLAGS) check TESTS="check_memory" TESTS_ENVIRONMENT=$(TESTS_ENVIRONMENT_MEM)
	
clean-local:
	rm -rf coverage || true;
	rm -rf *.gcda *.gcdo *.gcno *.o *.gcov *.log *.trs
	lcov --directory . --zerocounters || true;
	
# --- no changes beyond this line needed ---
INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

AM_LDFLAGS = $(LDADD)
if COVERAGE
AM_LDFLAGS += --coverage
endif

AM_CFLAGS = $(GLOBAL_AM_CFLAGS) $(INCLUDE)

