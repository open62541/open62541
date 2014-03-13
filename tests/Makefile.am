INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@
TESTS =
# --- no changes above this line needed ---

#add new tests using the following templates
TESTS += check_stack
check_stack_SOURCES = check_stack.c
check_stack_LDADD = $(LDADD)

TESTS += check_list
check_list_SOURCES = check_list.c
check_list_LDADD = $(LDADD)

# --- no changes beyond this line needed ---
check_PROGRAMS = $(TESTS)

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0 $(INCLUDE)
else
        AM_CFLAGS = -O2 $(INCLUDE)
endif
