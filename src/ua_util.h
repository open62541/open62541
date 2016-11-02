#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include "ua_config.h"

/* Assert */
#include <assert.h>
#define UA_assert(ignore) assert(ignore)

/* container_of */
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))

/* Thread Local Storage */
#ifdef UA_ENABLE_MULTITHREADING
# if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define UA_THREAD_LOCAL _Thread_local /* C11 */
# elif defined(__GNUC__)
#  define UA_THREAD_LOCAL __thread /* GNU extension */
# elif defined(_MSC_VER)
#  define UA_THREAD_LOCAL __declspec(thread) /* MSVC extension */
# else
#  warning The compiler does not allow thread-local variables. The library can be built, but will not be thread-safe.
# endif
#endif

#ifndef UA_THREAD_LOCAL
# define UA_THREAD_LOCAL
#endif

/* BSD Queue Macros */
#include "queue.h"

#endif /* UA_UTIL_H_ */
