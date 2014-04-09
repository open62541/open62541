AM_CFLAGS = -std=c99 -pedantic -pipe -fPIC -fno-exceptions -fstack-protector -Wl,-z,relro -Wl,-z,now -fvisibility=hidden -W -Wall -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -Werror
if DEBUG
AM_CFLAGS += -g3 -O0  
else
AM_CFLAGS += -O2 
endif

export GLOBAL_AM_CFLAGS = $(AM_CFLAGS)

if HAVE_CHECK
    SUBS=tool include src tests

else 
    SUBS=tool include src
endif

SUBDIRS = $(SUBS) examples/src	

