INCLUDEDIRS = -I$(top_builddir)/src -I$(top_builddir)/src/util -I$(top_builddir)/examples/src
AM_CFLAGS = $(GLOBAL_AM_CFLAGS) $(INCLUDEDIRS)
WRAPPER_DIR = $(top_builddir)/wrappers

open62541.py open62541_wrap_python.c:
	swig -python $(INCLUDEDIRS) -o ./open62541_wrap_python.c $(WRAPPER_DIR)/open62541.i

networklayer.so:
	gcc -c $(AM_CFLAGS) $(top_builddir)/examples/src/networklayer.c -o networklayer.so

open62541_wrap_python.o: open62541_wrap_python.c
	gcc -c $(subst -pedantic, , $(AM_CFLAGS)) -I/usr/include/python2.7 open62541_wrap_python.c

_open62541.so: networklayer.so open62541_wrap_python.o
	ld -shared open62541_wrap_python.o networklayer.so $(top_builddir)/lib/libopen62541.so -o _open62541.so

all: open62541.py _open62541.so

clean-local:
	rm -rf networklayer.so open62541.py open62541.pyc _open62541.so open62541_wrap_python.c open62541_wrap_python.o || true;
