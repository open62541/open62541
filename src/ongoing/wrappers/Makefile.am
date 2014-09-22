# We expand open62541.h with the preprocessor. SWIG can't handle all our macros.
# The standard libs are not included into the expanded header file.

# the lib is stated as a dependency to refresh if a header changes in ../src
open62541_expanded.h: open62541.h open62541.i $(top_builddir)/lib/libopen62541.a
	gcc -E open62541.h > open62541_expanded.h -I../src -D_STDIO_H=1 -D_STDDEF_H=1 -D_STDLIB_H=1 -D_STRING_H=1 -D_CTYPE_H=1 -D_UNISTD_H=1 -D_STDINT_H=1

all: open62541_expanded.h

clean-local:
	rm -rf open62541_expanded.h
