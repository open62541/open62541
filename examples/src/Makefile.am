
bin_PROGRAMS= $(top_builddir)/bin/exampleServer $(top_builddir)/bin/exampleServerMT $(top_builddir)/bin/exampleServerACPLT
#__top_builddir__bin_exampleServer_LDFLAGS = -all-static
__top_builddir__bin_exampleServer_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include
__top_builddir__bin_exampleServer_SOURCES = opcuaServer.c
__top_builddir__bin_exampleServer_LDADD= $(top_builddir)/lib/libopen62541.a

__top_builddir__bin_exampleServerMT_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include
__top_builddir__bin_exampleServerMT_SOURCES = opcuaServerMT.c
__top_builddir__bin_exampleServerMT_LDADD= $(top_builddir)/lib/libopen62541.a -lpthread

__top_builddir__bin_exampleServerACPLT_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include
__top_builddir__bin_exampleServerACPLT_SOURCES = opcuaServerMT.c
__top_builddir__bin_exampleServerACPLT_LDADD= $(top_builddir)/lib/libopen62541.a -lpthread

AM_CFLAGS = $(GLOBAL_AM_CFLAGS)
