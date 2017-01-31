/* This Source Code Form is subject to the terms of the Mozilla Public 
 * License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include "ua_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Assert */
#include <assert.h>
#define UA_assert(ignore) assert(ignore)

/* BSD Queue Macros */
#include "queue.h"

/* C++ Access to datatypes defined inside structs (for queue.h) */
#ifdef __cplusplus
# define memberstruct(container,member) container::member
#else
# define memberstruct(container,member) member
#endif

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

/* Atomic Operations
 * -----------------
 * Atomic operations that synchronize across processor cores (for
 * multithreading). Only the inline-functions defined next are used. Replace
 * with architecture-specific operations if necessary. */
#ifndef UA_ENABLE_MULTITHREADING
# define UA_atomic_sync()
#else
# ifdef _MSC_VER /* Visual Studio */
#  define UA_atomic_sync() _ReadWriteBarrier()
# else /* GCC/Clang */
#  define UA_atomic_sync() __sync_synchronize()
# endif
#endif

static UA_INLINE void *
UA_atomic_xchg(void * volatile * addr, void *newptr) {
#ifndef UA_ENABLE_MULTITHREADING
    void *old = *addr;
    *addr = newptr;
    return old;
#else
# ifdef _MSC_VER /* Visual Studio */
    return _InterlockedExchangePointer(addr, newptr);
# else /* GCC/Clang */
    return __sync_lock_test_and_set(addr, newptr);
# endif
#endif
}

static UA_INLINE void *
UA_atomic_cmpxchg(void * volatile * addr, void *expected, void *newptr) {
#ifndef UA_ENABLE_MULTITHREADING
    void *old = *addr;
    if(old == expected) {
        *addr = newptr;
    }
    return old;
#else
# ifdef _MSC_VER /* Visual Studio */
    return _InterlockedCompareExchangePointer(addr, expected, newptr);
# else /* GCC/Clang */
    return __sync_val_compare_and_swap(addr, expected, newptr);
# endif
#endif
}

static UA_INLINE uint32_t
UA_atomic_add(volatile uint32_t *addr, uint32_t increase) {
#ifndef UA_ENABLE_MULTITHREADING
    *addr += increase;
    return *addr;
#else
# ifdef _MSC_VER /* Visual Studio */
    return _InterlockedExchangeAdd(addr, increase) + increase;
# else /* GCC/Clang */
    return __sync_add_and_fetch(addr, increase);
# endif
#endif
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_UTIL_H_ */
