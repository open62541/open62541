
if DEBUG
AM_CFLAGS = -g -O0
else
AM_CFLAGS = -O2
endif

if HAVE_CHECK
    SUBS=src tests tool

else 
    SUBS=src tool
endif

SUBDIRS = $(SUBS) examples/src	

