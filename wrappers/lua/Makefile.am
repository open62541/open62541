INCLUDEDIRS = -I$(top_builddir)/src -I$(top_builddir)/src/util -I$(top_builddir)/examples/src
AM_CFLAGS = $(GLOBAL_AM_CFLAGS) $(INCLUDEDIRS)
WRAPPER_DIR = $(top_builddir)/wrappers

open62541.lua open62541_wrap_lua.c:
	swig -lua $(INCLUDEDIRS) -o ./open62541_wrap_lua.c $(WRAPPER_DIR)/open62541.i

networklayer.so:
	gcc -c $(AM_CFLAGS) $(top_builddir)/examples/src/networklayer.c -o networklayer.so

open62541_wrap_lua.o: open62541_wrap_lua.c
	gcc -c $(subst -Wall, ,$(subst -Wreturn-type, ,$(subst -pedantic, , $(AM_CFLAGS)))) -I/usr/include/lua5.2 open62541_wrap_lua.c

open62541.so: networklayer.so open62541_wrap_lua.o
	ld -shared open62541_wrap_lua.o networklayer.so $(top_builddir)/lib/libopen62541.so -o open62541.so

all: open62541.lua open62541.so

clean:
	rm networklayer.so open62541.lua open62541.so open62541_wrap_lua.c open62541_wrap_lua.o
