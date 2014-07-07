WRAPPER_DIR = $(top_builddir)/wrappers
#AM_CFLAGS = $(GLOBAL_AM_CFLAGS) -I$(WRAPPER_DIR)
AM_CFLAGS = -fPIC -I$(WRAPPER_DIR)

open62541.py open62541_wrap_python.c: $(WRAPPER_DIR)/open62541.i \
									  $(WRAPPER_DIR)/open62541_expanded.h
	swig -python -o ./open62541_wrap_python.c $(WRAPPER_DIR)/open62541.i

open62541_wrap_python.o: open62541_wrap_python.c
	gcc -c $(subst -pedantic, , $(AM_CFLAGS)) -I/usr/include/python2.7 open62541_wrap_python.c

_open62541.so: open62541_wrap_python.o
	ld -shared open62541_wrap_python.o $(top_builddir)/lib/libopen62541.a -o _open62541.so

all: open62541.py _open62541.so

clean-local:
	rm -rf open62541.py open62541.pyc _open62541.so open62541_wrap_python.c open62541_wrap_python.o || true;
