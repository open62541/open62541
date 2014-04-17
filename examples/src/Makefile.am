if MULTITHREADING
MT_LDADD = -lpthread
else
MT_LDADD =
endif

if COVERAGE
COV_LDFLAGS = --coverage
else
COV_LDFLAGS = 
endif

if MULTITHREADING
bin_PROGRAMS= $(top_builddir)/bin/exampleServerMT 
else
bin_PROGRAMS= $(top_builddir)/bin/exampleServer $(top_builddir)/bin/exampleServerACPLT
endif

#__top_builddir__bin_exampleServer_LDFLAGS = -all-static
if MULTITHREADING
__top_builddir__bin_exampleServerMT_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include $(GLOBAL_AM_CFLAGS)
__top_builddir__bin_exampleServerMT_SOURCES = opcuaServerMT.c networklayer.c
__top_builddir__bin_exampleServerMT_LDADD= $(top_builddir)/lib/libopen62541.a $(MT_LDADD)
__top_builddir__bin_exampleServerMT_LDFLAGS= $(COV_LDFLAGS)
else
__top_builddir__bin_exampleServer_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include $(GLOBAL_AM_CFLAGS)
__top_builddir__bin_exampleServer_SOURCES = opcuaServer.c networklayer.c
__top_builddir__bin_exampleServer_LDADD= $(top_builddir)/lib/libopen62541.a $(MT_LDADD)
__top_builddir__bin_exampleServer_LDFLAGS= $(COV_LDFLAGS)

__top_builddir__bin_exampleServerACPLT_CFLAGS = -I$(top_builddir)/src -I$(top_builddir)/include $(GLOBAL_AM_CFLAGS)
__top_builddir__bin_exampleServerACPLT_SOURCES = opcuaServerACPLT.c networklayer.c
__top_builddir__bin_exampleServerACPLT_LDADD= $(top_builddir)/lib/libopen62541.a $(MT_LDADD)
__top_builddir__bin_exampleServerACPLT_LDFLAGS= $(COV_LDFLAGS)
endif
