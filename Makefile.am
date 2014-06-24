#AM_CFLAGS  = -std=c99 -pedantic -pipe -fPIC -fno-exceptions -fstack-protector -Wl,-z,relro -Wl,-z,now -fvisibility=hidden -W -Wall -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -Werror
AM_CFLAGS  = -std=c99 -pedantic -pipe -funsigned-char -Wno-pointer-sign -fno-exceptions -fstack-protector -fvisibility=hidden -W -Wall -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -Werror
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
AM_LDADD += -lpthread
endif

export GLOBAL_AM_CFLAGS = $(AM_CFLAGS)
export GLOBAL_AM_LDADD = $(AM_LDADD)

SUBS=src
if HAVE_CHECK
	SUBS+= tests
endif

if ENABLE_DOXYGEN
	SUBS+= doc
endif
#SUBDIRS = $(SUBS) examples/src		
#under Windows this is not possible because of include <sys/select.h> in open62541\example\scr\networklayer.h
