
bin_PROGRAMS= $(top_builddir)/bin/exampleServer
__top_builddir__bin_exampleServer_LDFLAGS = -all-static
__top_builddir__bin_exampleServer_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include
__top_builddir__bin_exampleServer_SOURCES = opcuaServer.c
__top_builddir__bin_exampleServer_LDADD= $(top_builddir)/lib/libopen62541.a

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0
else
        AM_CFLAGS = -O2
endif
