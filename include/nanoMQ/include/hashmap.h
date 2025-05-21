#ifndef SHEREDOM_HASHMAP_H_INCLUDED
#define SHEREDOM_HASHMAP_H_INCLUDED

#if defined(_MSC_VER)
// Workaround a bug in the MSVC runtime where it uses __cplusplus when not
// defined.
#pragma warning(push, 0)
#pragma warning(disable : 4668)
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "nng/nng.h"
#include "nng/supplemental/util/platform.h"


#if (defined(_MSC_VER) && defined(__AVX__)) ||                                 \
    (!defined(_MSC_VER) && defined(__SSE4_2__))
#define HASHMAP_SSE42
#endif

#if defined(HASHMAP_SSE42)
#include <nmmintrin.h>
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
#pragma warning(push)
/* Stop MSVC complaining about unreferenced functions */
#pragma warning(disable : 4505)
/* Stop MSVC complaining about not inlining functions */
#pragma warning(disable : 4710)
/* Stop MSVC complaining about inlining functions! */
#pragma warning(disable : 4711)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#if defined(_MSC_VER)
#define HASHMAP_USED
#elif defined(__GNUC__)
#define HASHMAP_USED __attribute__((used))
#else
#define HASHMAP_USED
#endif

typedef struct hashmap_s hashmap_s;
/* We need to keep keys and values. */
struct hashmap_element_s {
  const char *key;
  unsigned key_len;
  int in_use;
  uint32_t data;
};

/* A hashmap has some maximum size and current size, as well as the data to
 * hold. */
struct hashmap_s {
  unsigned table_size;
  unsigned size;
  struct hashmap_element_s *data;

};


#define HASHMAP_MAX_CHAIN_LENGTH (8)

#if defined(__cplusplus)
extern "C" {
#endif

int nano_hashmap_create(const unsigned initial_size,
                   struct hashmap_s * out_hashmap) HASHMAP_USED;

int nano_hashmap_put(struct hashmap_s * hashmap, const char *const key,
                const unsigned len, uint32_t value) HASHMAP_USED;

uint32_t nano_hashmap_get(const struct hashmap_s *const hashmap,
                  const char *const key,
                  const unsigned len) HASHMAP_USED;

int nano_hashmap_remove(struct hashmap_s * m, const char *const key,
            const unsigned len);

void nano_hashmap_destroy(struct hashmap_s * m);


#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
#define HASHMAP_CAST(type, x) static_cast<type>(x)
#define HASHMAP_PTR_CAST(type, x) reinterpret_cast<type>(x)
#define HASHMAP_NULL NULL
#else
#define HASHMAP_CAST(type, x) ((type)x)
#define HASHMAP_PTR_CAST(type, x) ((type)x)
#define HASHMAP_NULL 0
#endif


#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif