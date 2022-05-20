/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifndef ARCH_UA_ARCHITECTURE_DEFINITIONS_H_
#define ARCH_UA_ARCHITECTURE_DEFINITIONS_H_

#include <open62541/config.h>

/**
 * C99 Definitions
 * --------------- */
#include <string.h>
#include <stddef.h>

/* Include stdint.h and stdbool.h or workaround for older Visual Studios */
#ifdef UNDER_CE
# include "stdint.h"
#endif
#if !defined(_MSC_VER) || _MSC_VER >= 1800
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

/* Include inttypes.h or workaround for older Visual Studios */
#if !defined(_MSC_VER) || _MSC_VER >= 1800
# include <inttypes.h>
#else
# define __PRI_8_LENGTH_MODIFIER__ "hh"
# define __PRI_64_LENGTH_MODIFIER__ "ll"

# define PRId8 __PRI_8_LENGTH_MODIFIER__ "d"
# define PRIi8 __PRI_8_LENGTH_MODIFIER__ "i"
# define PRIo8 __PRI_8_LENGTH_MODIFIER__ "o"
# define PRIu8 __PRI_8_LENGTH_MODIFIER__ "u"
# define PRIx8 __PRI_8_LENGTH_MODIFIER__ "x"
# define PRIX8 __PRI_8_LENGTH_MODIFIER__ "X"

# define PRId16 "hd"
# define PRIi16 "hi"
# define PRIo16 "ho"
# define PRIu16 "hu"
# define PRIx16 "hx"
# define PRIX16 "hX"

# define PRId32 "ld"
# define PRIi32 "li"
# define PRIo32 "lo"
# define PRIu32 "lu"
# define PRIx32 "lx"
# define PRIX32 "lX"

# define PRId64 __PRI_64_LENGTH_MODIFIER__ "d"
# define PRIi64 __PRI_64_LENGTH_MODIFIER__ "i"
# define PRIo64 __PRI_64_LENGTH_MODIFIER__ "o"
# define PRIu64 __PRI_64_LENGTH_MODIFIER__ "u"
# define PRIx64 __PRI_64_LENGTH_MODIFIER__ "x"
# define PRIX64 __PRI_64_LENGTH_MODIFIER__ "X"
#endif

/**
 * Thread-local variables
 * ---------------------- */
#if UA_MULTITHREADING >= 100
# if defined(__GNUC__) /* Also covers clang */
#  define UA_THREAD_LOCAL __thread
# elif defined(_MSC_VER)
#  define UA_THREAD_LOCAL __declspec(thread)
# endif
#endif
#ifndef UA_THREAD_LOCAL
# define UA_THREAD_LOCAL
#endif

/**
 * Memory Management
 * -----------------
 *
 * The flag ``UA_ENABLE_MALLOC_SINGLETON`` enables singleton (global) variables
 * with method pointers for memory management (malloc et al.). The method
 * pointers can be switched out at runtime. Use-cases for this are testing of
 * constrained memory conditions and arena-based custom memory management.
 *
 * If the flag is undefined, then ``UA_malloc`` etc. are set to the default
 * malloc, as defined in ``/arch/<architecture>/ua_architecture.h``.
 */

#ifdef UA_ENABLE_MALLOC_SINGLETON
extern UA_THREAD_LOCAL void * (*UA_mallocSingleton)(size_t size);
extern UA_THREAD_LOCAL void (*UA_freeSingleton)(void *ptr);
extern UA_THREAD_LOCAL void * (*UA_callocSingleton)(size_t nelem, size_t elsize);
extern UA_THREAD_LOCAL void * (*UA_reallocSingleton)(void *ptr, size_t size);
# define UA_malloc(size) UA_mallocSingleton(size)
# define UA_free(ptr) UA_freeSingleton(ptr)
# define UA_calloc(num, size) UA_callocSingleton(num, size)
# define UA_realloc(ptr, size) UA_reallocSingleton(ptr, size)
#endif

/* Stack-allocation of memory. Use C99 variable-length arrays if possible.
 * Otherwise revert to alloca. Note that alloca is not supported on some
 * plattforms. */
#ifndef UA_STACKARRAY
# if defined(__GNUC__) || defined(__clang__)
#  define UA_STACKARRAY(TYPE, NAME, SIZE) TYPE NAME[SIZE]
# else
# if defined(__GNUC__) || defined(__clang__)
#  define UA_alloca(size) __builtin_alloca (size)
# elif defined(_WIN32)
#  define UA_alloca(SIZE) _alloca(SIZE)
# else
#  include <alloca.h>
#  define UA_alloca(SIZE) alloca(SIZE)
# endif
#  define UA_STACKARRAY(TYPE, NAME, SIZE) \
    /* cppcheck-suppress allocaCalled */  \
    TYPE *(NAME) = (TYPE*)UA_alloca(sizeof(TYPE) * (SIZE))
# endif
#endif

/**
 * Assertions
 * ----------
 * The assert macro is disabled by defining NDEBUG. It is often forgotten to
 * include -DNDEBUG in the compiler flags when using the single-file release. So
 * we make assertions dependent on the UA_DEBUG definition handled by CMake. */
#ifdef UA_DEBUG
# include <assert.h>
# define UA_assert(ignore) assert(ignore)
#else
# define UA_assert(ignore) do {} while(0)
#endif

/* Outputs an error message at compile time if the assert fails.
 * Example usage:
 * UA_STATIC_ASSERT(sizeof(long)==7, use_another_compiler_luke)
 * See: https://stackoverflow.com/a/4815532/869402 */
#if defined(__cplusplus) && __cplusplus >= 201103L /* C++11 or above */
# define UA_STATIC_ASSERT(cond,msg) static_assert(cond, #msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L /* C11 or above */
# define UA_STATIC_ASSERT(cond,msg) _Static_assert(cond, #msg)
#elif defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER) /* GCC, Clang, MSC */
# define UA_CTASTR2(pre,post) pre ## post
# define UA_CTASTR(pre,post) UA_CTASTR2(pre,post)
# ifndef __COUNTER__ /* PPC GCC fix */
#  define __COUNTER__ __LINE__
# endif
# define UA_STATIC_ASSERT(cond,msg)                             \
    typedef struct {                                            \
        unsigned int UA_CTASTR(static_assertion_failed_,msg) : !!(cond); \
    } UA_CTASTR(static_assertion_failed_,__COUNTER__)
#else /* Everybody else */
# define UA_STATIC_ASSERT(cond,msg) typedef char static_assertion_##msg[(cond)?1:-1]
#endif

/**
 * Dynamic Linking
 * ---------------
 * Explicit attribute for functions to be exported in a shared library. */
#if defined(_WIN32) && defined(UA_DYNAMIC_LINKING)
# ifdef UA_DYNAMIC_LINKING_EXPORT /* export dll */
#  ifdef __GNUC__
#   define UA_EXPORT __attribute__ ((dllexport))
#  else
#   define UA_EXPORT __declspec(dllexport)
#  endif
# else /* import dll */
#  ifdef __GNUC__
#   define UA_EXPORT __attribute__ ((dllimport))
#  else
#   define UA_EXPORT __declspec(dllimport)
#  endif
# endif
#else /* non win32 */
# if __GNUC__ || __clang__
#  define UA_EXPORT __attribute__ ((visibility ("default")))
# endif
#endif
#ifndef UA_EXPORT
# define UA_EXPORT /* fallback to default */
#endif

/**
 * Threadsafe functions
 * --------------------
 * Functions that can be called from independent threads are marked with
 * the UA_THREADSAFE macro. This is currently only an information for the
 * developer. It can be used in the future for instrumentation and static
 * code analysis. */
#define UA_THREADSAFE

/**
 * Inline Functions
 * ---------------- */
#ifdef _MSC_VER
# define UA_INLINE __inline
#else
# define UA_INLINE inline
#endif

/**
 * Non-aliasing pointers
 * -------------------- */
#ifdef _MSC_VER
# define UA_RESTRICT __restrict
#elif defined(__GNUC__)
# define UA_RESTRICT __restrict__
#elif defined(__CODEGEARC__)
# define UA_RESTRICT _RESTRICT
#else
# define UA_RESTRICT restrict
#endif

/**
 * Likely/Unlikely Conditions
 * --------------------------
 * Condition is likely/unlikely, to help branch prediction. */
#if defined(__GNUC__) || defined(__clang__)
# define UA_LIKELY(x) __builtin_expect((x), 1)
# define UA_UNLIKELY(x) __builtin_expect((x), 0)
#else
# define UA_LIKELY(x) x
# define UA_UNLIKELY(x) x
#endif

/**
 * Function attributes
 * ------------------- */
#if defined(__GNUC__) || defined(__clang__)
# define UA_FUNC_ATTR_MALLOC __attribute__((malloc))
# define UA_FUNC_ATTR_PURE __attribute__ ((pure))
# define UA_FUNC_ATTR_CONST __attribute__((const))
# define UA_FUNC_ATTR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
# define UA_FORMAT(X,Y) __attribute__ ((format (printf, X, Y)))
#elif defined(_MSC_VER) && _MSC_VER >= 1800
# include <sal.h>
# define UA_FUNC_ATTR_MALLOC
# define UA_FUNC_ATTR_PURE
# define UA_FUNC_ATTR_CONST
# define UA_FUNC_ATTR_WARN_UNUSED_RESULT _Check_return_
# define UA_FORMAT(X,Y)
#else
# define UA_FUNC_ATTR_MALLOC
# define UA_FUNC_ATTR_PURE
# define UA_FUNC_ATTR_CONST
# define UA_FUNC_ATTR_WARN_UNUSED_RESULT
# define UA_FORMAT(X,Y)
#endif

#if defined(__GNUC__) || defined(__clang__)
# define UA_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
# define UA_DEPRECATED __declspec(deprecated)
#else
# define UA_DEPRECATED
#endif

/**
 * Internal Attributes
 * -------------------
 * These attributes are only defined if the macro UA_INTERNAL is defined. That
 * way public methods can be annotated (e.g. to warn for unused results) but
 * warnings are only triggered for internal code. */

#if defined(UA_INTERNAL) && (defined(__GNUC__) || defined(__clang__))
# define UA_INTERNAL_DEPRECATED \
    _Pragma ("GCC warning \"Macro is deprecated for internal use\"")
#else
# define UA_INTERNAL_DEPRECATED
#endif

#if defined(UA_INTERNAL) && (defined(__GNUC__) || defined(__clang__))
# define UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT \
    __attribute__((warn_unused_result))
#else
# define UA_INTERNAL_FUNC_ATTR_WARN_UNUSED_RESULT
#endif

/**
 * Detect Endianness and IEEE 754 floating point
 * ---------------------------------------------
 * Integers and floating point numbers are transmitted in little-endian (IEEE
 * 754 for floating point) encoding. If the target architecture uses the same
 * format, numeral datatypes can be memcpy'd (overlayed) on the network buffer.
 * Otherwise, a slow default encoding routine is used that works for every
 * architecture.
 *
 * Integer Endianness
 * ^^^^^^^^^^^^^^^^^^
 * The definition ``UA_LITTLE_ENDIAN`` is true when the integer representation
 * of the target architecture is little-endian. */
#if defined(_WIN32)
# define UA_LITTLE_ENDIAN 1
#elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
# define UA_LITTLE_ENDIAN 1
#elif (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
      (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
# define UA_LITTLE_ENDIAN 1
#elif defined(__linux__) /* Linux (including Android) */
# include <endian.h>
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define UA_LITTLE_ENDIAN 1
# endif
#elif defined(__OpenBSD__) /* OpenBSD */
# include <sys/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
#  define UA_LITTLE_ENDIAN 1
# endif
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__) /* Other BSD */
# include <sys/endian.h>
# if _BYTE_ORDER == _LITTLE_ENDIAN
#  define UA_LITTLE_ENDIAN 1
# endif
#elif defined(__APPLE__) /* Apple (MacOS, iOS) */
# include <libkern/OSByteOrder.h>
# if defined(__LITTLE_ENDIAN__)
#  define UA_LITTLE_ENDIAN 1
# endif
#elif defined(__QNX__) || defined(__QNXNTO__) /* QNX */
# include <gulliver.h>
# if defined(__LITTLEENDIAN__)
#  define UA_LITTLE_ENDIAN 1
# endif
#elif defined(_OS9000) /* OS-9 */
# if defined(_LIL_END)
#  define UA_LITTLE_ENDIAN 1
# endif
#endif
#ifndef UA_LITTLE_ENDIAN
# define UA_LITTLE_ENDIAN 0
#endif

/* Can the integers be memcpy'd onto the network buffer? Add additional checks
 * here. Some platforms (e.g. QNX) have sizeof(bool) > 1. Manually disable
 * overlayed integer encoding if that is the case. */
#if (UA_LITTLE_ENDIAN == 1)
UA_STATIC_ASSERT(sizeof(bool) == 1, cannot_overlay_integers_with_large_bool);
# define UA_BINARY_OVERLAYABLE_INTEGER 1
#else
# define UA_BINARY_OVERLAYABLE_INTEGER 0
#endif

/**
 * Float Endianness
 * ^^^^^^^^^^^^^^^^
 * The definition ``UA_FLOAT_IEEE754`` is set to true when the floating point
 * number representation of the target architecture is IEEE 754. The definition
 * ``UA_FLOAT_LITTLE_ENDIAN`` is set to true when the floating point number
 * representation is in little-endian encoding. */

#if defined(_WIN32)
# define UA_FLOAT_IEEE754 1
#elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || \
    defined(__ia64__) || defined(__powerpc__) || defined(__sparc__) || \
    defined(__arm__)
# define UA_FLOAT_IEEE754 1
#elif defined(__STDC_IEC_559__)
# define UA_FLOAT_IEEE754 1
#else
# define UA_FLOAT_IEEE754 0
#endif

/* Wikipedia says (https://en.wikipedia.org/wiki/Endianness): Although the
 * ubiquitous x86 processors of today use little-endian storage for all types of
 * data (integer, floating point, BCD), there are a number of hardware
 * architectures where floating-point numbers are represented in big-endian form
 * while integers are represented in little-endian form. */
#if defined(_WIN32)
# define UA_FLOAT_LITTLE_ENDIAN 1
#elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
# define UA_FLOAT_LITTLE_ENDIAN 1
#elif defined(__FLOAT_WORD_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    (__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__) /* Defined only in GCC */
# define UA_FLOAT_LITTLE_ENDIAN 1
#elif defined(__FLOAT_WORD_ORDER) && defined(__LITTLE_ENDIAN) && \
    (__FLOAT_WORD_ORDER == __LITTLE_ENDIAN) /* Defined only in GCC */
# define UA_FLOAT_LITTLE_ENDIAN 1
#endif
#ifndef UA_FLOAT_LITTLE_ENDIAN
# define UA_FLOAT_LITTLE_ENDIAN 0
#endif

/* Only if the floating points are litle-endian **and** in IEEE 754 format can
 * we memcpy directly onto the network buffer. */
#if (UA_FLOAT_IEEE754 == 1) && (UA_FLOAT_LITTLE_ENDIAN == 1)
# define UA_BINARY_OVERLAYABLE_FLOAT 1
#else
# define UA_BINARY_OVERLAYABLE_FLOAT 0
#endif

#endif /* ARCH_UA_ARCHITECTURE_DEFINITIONS_H_ */
