
bin_PROGRAMS= $(top_builddir)/bin/exampleServer $(top_builddir)/bin/simpleTest
#__top_builddir__bin_exampleServer_LDFLAGS = -all-static
__top_builddir__bin_exampleServer_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include
__top_builddir__bin_exampleServer_SOURCES = opcuaServer.c
__top_builddir__bin_exampleServer_LDADD= $(top_builddir)/lib/libopen62541.a

__top_builddir__bin_simpleTest_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include
__top_builddir__bin_simpleTest_SOURCES = simpleTest.c
__top_builddir__bin_simpleTest_LDADD= $(top_builddir)/lib/libopen62541.a

AM_CFLAGS = $(GLOBAL_AM_CFLAGS)
