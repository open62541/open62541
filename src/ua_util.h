#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#include "ua_config.h"

#include <assert.h>
#define UA_assert(ignore) assert(ignore)

/*********************/
/* Memory Management */
/*********************/

/* Replace the macros with functions for custom allocators if necessary */
#include <stdlib.h> // malloc, free
#ifdef _WIN32
# include <malloc.h>
#endif

#ifndef UA_free
# define UA_free(ptr) free(ptr)
#endif
#ifndef UA_malloc
# define UA_malloc(size) malloc(size)
#endif
#ifndef UA_calloc
# define UA_calloc(num, size) calloc(num, size)
#endif
#ifndef UA_realloc
# define UA_realloc(ptr, size) realloc(ptr, size)
#endif

#ifndef NO_ALLOCA
# ifdef __GNUC__
#  define UA_alloca(size) __builtin_alloca (size)
# elif defined(_WIN32)
#  define UA_alloca(SIZE) _alloca(SIZE)
# else
#  include <alloca.h>
#  define UA_alloca(SIZE) alloca(SIZE)
# endif
#endif

#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))

/************************/
/* Thread Local Storage */
/************************/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define UA_THREAD_LOCAL _Thread_local /* C11 */
#elif defined(__GNUC__)
# define UA_THREAD_LOCAL __thread /* GNU extension */
#elif defined(_MSC_VER)
# define UA_THREAD_LOCAL __declspec(thread) /* MSVC extension */
#else
# define UA_THREAD_LOCAL
# warning The compiler does not allow thread-local variables. The library can be built, but will not be thread safe.
#endif

/*************************/
/* External Dependencies */
/*************************/
#include "queue.h"

#ifdef UA_ENABLE_MULTITHREADING
# define _LGPL_SOURCE
# include <urcu.h>
# include <urcu/wfcqueue.h>
# include <urcu/uatomic.h>
# include <urcu/rculfhash.h>
# include <urcu/lfstack.h>
# ifdef NDEBUG
#  define UA_RCU_LOCK() rcu_read_lock()
#  define UA_RCU_UNLOCK() rcu_read_unlock()
#  define UA_ASSERT_RCU_LOCKED()
#  define UA_ASSERT_RCU_UNLOCKED()
# else
   extern UA_THREAD_LOCAL bool rcu_locked;
#   define UA_ASSERT_RCU_LOCKED() assert(rcu_locked)
#   define UA_ASSERT_RCU_UNLOCKED() assert(!rcu_locked)
#   define UA_RCU_LOCK() do {                     \
        UA_ASSERT_RCU_UNLOCKED();                 \
        rcu_locked = true;                        \
        rcu_read_lock(); } while(0)
#   define UA_RCU_UNLOCK() do {                   \
        UA_ASSERT_RCU_LOCKED();                   \
        rcu_locked = false;                       \
        rcu_read_lock(); } while(0)
# endif
#else
# define UA_RCU_LOCK()
# define UA_RCU_UNLOCK()
# define UA_ASSERT_RCU_LOCKED()
# define UA_ASSERT_RCU_UNLOCKED()
#endif

#endif /* UA_UTIL_H_ */
