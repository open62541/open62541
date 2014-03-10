bin_PROGRAMS= $(top_builddir)/bin/exampleServer
__top_builddir__bin_exampleServer_LDFLAGS = -all-static
__top_builddir__bin_exampleServer_SOURCES = opcuaServer.c
__top_builddir__bin_exampleServer_LDADD= ../../lib/libopen62541.a

#optimization levels depending on debug
if DEBUG
        AM_CFLAGS = -O0
else
        AM_CFLAGS = -O2
endif
