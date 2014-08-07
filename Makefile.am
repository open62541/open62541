AM_CFLAGS  = -std=c99 -pedantic -pipe -fstack-protector -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -Werror -ffunction-sections -fdata-sections -Wl,--gc-sections
AM_LDFLAGS =
AM_LDADD =

if TARGET_WIN
else
AM_CFLAGS += -fPIC 
endif

if DEBUG
AM_CFLAGS += -g3 -O0
else
AM_CFLAGS += -O2
endif

if COVERAGE
AM_CFLAGS  += --coverage
AM_LDFLAGS += --coverage
endif

if MULTITHREADING
AM_LDADD += -lpthread -lurcu-cds -lurcu
endif

# additional encodings
UA_ENCODING_AMOUNT = 1
if UA_ENCODING_XML
UA_ENCODING_AMOUNT = 2 #$$(echo $(UA_ENCODING_AMOUNT)+1 | bc)
endif

if UA_ENCODING_JSON
UA_ENCODING_AMOUNT = 3 #$$(echo $(UA_ENCODING_AMOUNT)+1 | bc)
endif

AM_CFLAGS += -DUA_ENCODING_AMOUNT=$(UA_ENCODING_AMOUNT)

# export flags for sub-makefiles
export GLOBAL_AM_CFLAGS = $(AM_CFLAGS)
export GLOBAL_AM_LDADD = $(AM_LDADD)

SUBDIRS=src

if HAVE_CHECK
	SUBDIRS += tests
endif

if ENABLE_DOXYGEN
	SUBDIRS += doc
endif

if TARGET_WIN
else
SUBDIRS += examples/src	
endif

if PYTHON_WRAPPER
SUBDIRS += wrappers wrappers/python
endif

if LUA_WRAPPER
SUBDIRS += wrappers wrappers/lua
endif
