
if DEBUG
AM_CFLAGS = -g -O0
else
AM_CFLAGS = -O2
endif

if HAVE_CHECK
    SUBS=src . tests .
else 
    SUBS=src .
endif

SUBDIRS = $(SUBS) examples/src	

