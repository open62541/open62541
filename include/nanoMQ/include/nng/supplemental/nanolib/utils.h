#ifndef _NNG_SUPPLEMENTAL_NANOLIB_UTILS_H
#define _NNG_SUPPLEMENTAL_NANOLIB_UTILS_H

#include "nng/nng.h"
#include <ctype.h>
#include <stdio.h>

NNG_DECL void fatal(const char *msg, ...);
NNG_DECL void nng_fatal(const char *msg, int rv);

#endif
