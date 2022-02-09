#ifndef ATOI_H
#define ATOI_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_MSC_VER) || _MSC_VER >= 1800
# include <stddef.h>
# include <stdint.h>
# include <stdbool.h> /* C99 Boolean */
#else
# include "ms_stdint.h"
# if !defined(__bool_true_false_are_defined)
#  define bool unsigned char
#  define true 1
#  define false 0
#  define __bool_true_false_are_defined
# endif
#endif

/* Parse integer numbers. Returns the number of parsed digits until the first
 * non-valid character. Returns 0 upon failure.
 *
 * Hex numbers start with 0x.
 * Signed numbers may have a +/- prefix. */

size_t parseUInt64(const char *str, size_t size, uint64_t *result);
size_t parseInt64(const char *str, size_t size, int64_t *result);
size_t parseDouble(const char *str, size_t size, double *result);
    
#ifdef __cplusplus
}
#endif

#endif /* ATOI_H */

