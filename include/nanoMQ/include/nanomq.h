#ifndef NANOMQ_NANOMQ_H
#define NANOMQ_NANOMQ_H

#include "nng/supplemental/nanolib/log.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define NNI_PUT16(ptr, u)                                    \
	do {                                                 \
		(ptr)[0] = (uint8_t)(((uint16_t)(u)) >> 8u); \
		(ptr)[1] = (uint8_t)((uint16_t)(u));         \
	} while (0)

#define NNI_PUT32(ptr, u)                                     \
	do {                                                  \
		(ptr)[0] = (uint8_t)(((uint32_t)(u)) >> 24u); \
		(ptr)[1] = (uint8_t)(((uint32_t)(u)) >> 16u); \
		(ptr)[2] = (uint8_t)(((uint32_t)(u)) >> 8u);  \
		(ptr)[3] = (uint8_t)((uint32_t)(u));          \
	} while (0)

#define NNI_PUT64(ptr, u)                                     \
	do {                                                  \
		(ptr)[0] = (uint8_t)(((uint64_t)(u)) >> 56u); \
		(ptr)[1] = (uint8_t)(((uint64_t)(u)) >> 48u); \
		(ptr)[2] = (uint8_t)(((uint64_t)(u)) >> 40u); \
		(ptr)[3] = (uint8_t)(((uint64_t)(u)) >> 32u); \
		(ptr)[4] = (uint8_t)(((uint64_t)(u)) >> 24u); \
		(ptr)[5] = (uint8_t)(((uint64_t)(u)) >> 16u); \
		(ptr)[6] = (uint8_t)(((uint64_t)(u)) >> 8u);  \
		(ptr)[7] = (uint8_t)((uint64_t)(u));          \
	} while (0)

#define NNI_GET16(ptr, v)                             \
	v = (((uint16_t)((uint8_t)(ptr)[0])) << 8u) + \
	    (((uint16_t)(uint8_t)(ptr)[1]))

#define NNI_GET32(ptr, v)                              \
	v = (((uint32_t)((uint8_t)(ptr)[0])) << 24u) + \
	    (((uint32_t)((uint8_t)(ptr)[1])) << 16u) + \
	    (((uint32_t)((uint8_t)(ptr)[2])) << 8u) +  \
	    (((uint32_t)(uint8_t)(ptr)[3]))

#define NNI_GET64(ptr, v)                              \
	v = (((uint64_t)((uint8_t)(ptr)[0])) << 56u) + \
	    (((uint64_t)((uint8_t)(ptr)[1])) << 48u) + \
	    (((uint64_t)((uint8_t)(ptr)[2])) << 40u) + \
	    (((uint64_t)((uint8_t)(ptr)[3])) << 32u) + \
	    (((uint64_t)((uint8_t)(ptr)[4])) << 24u) + \
	    (((uint64_t)((uint8_t)(ptr)[5])) << 16u) + \
	    (((uint64_t)((uint8_t)(ptr)[6])) << 8u) +  \
	    (((uint64_t)(uint8_t)(ptr)[7]))

#define NANO_UNUSED(x) (x) __attribute__((unused))

#define NANO_NNG_FATAL(s, rv)				\
	do {									\
		log_fatal(s);						\
		nng_fatal((s), (rv));				\
	} while(0)

extern int    get_cache_argc();
extern char **get_cache_argv();

#endif
