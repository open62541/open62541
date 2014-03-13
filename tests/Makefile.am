TESTS = check_stack check_list

# --- no changes beyond this line needed ---
INCLUDE = @CHECK_CFLAGS@ -I$(top_builddir)/src -I$(top_builddir)/include
LDADD = $(top_builddir)/lib/libopen62541.a @CHECK_LIBS@

check_PROGRAMS = $(TESTS)
AM_LDFLAGS = $(LDADD)

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0 $(INCLUDE)
else
        AM_CFLAGS = -O2 $(INCLUDE)
endif
