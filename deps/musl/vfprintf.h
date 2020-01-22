#ifndef VFPRINTF_H
#define VFPRINTF_H

#include <limits.h>
#include <string.h>
//#include <stdarg.h>
//#include <stddef.h>
//#include <wchar.h>
#include <inttypes.h>
#include <endian.h>


//#include <stdio.h>

//#include <float.h>
#define LDBL_TRUE_MIN 3.6451995318824746025e-4951L
#define LDBL_MIN     3.3621031431120935063e-4932L
#define LDBL_MAX     1.1897314953572317650e+4932L
#define LDBL_EPSILON 1.0842021724855044340e-19L

#define LDBL_MANT_DIG 64
#define LDBL_MIN_EXP (-16381)
#define LDBL_MAX_EXP 16384
//libm.h
#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
#elif LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384 && __BYTE_ORDER == __LITTLE_ENDIAN
union ldshape {
	long double f;
	struct {
		uint64_t m;
		uint16_t se;
	} i;
};
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384 && __BYTE_ORDER == __LITTLE_ENDIAN
union ldshape {
	long double f;
	struct {
		uint64_t lo;
		uint32_t mid;
		uint16_t top;
		uint16_t se;
	} i;
	struct {
		uint64_t lo;
		uint64_t hi;
	} i2;
};
#elif LDBL_MANT_DIG == 113 && LDBL_MAX_EXP == 16384 && __BYTE_ORDER == __BIG_ENDIAN
union ldshape {
	long double f;
	struct {
		uint16_t se;
		uint16_t top;
		uint32_t mid;
		uint64_t lo;
	} i;
	struct {
		uint64_t hi;
		uint64_t lo;
	} i2;
};
#else
#error Unsupported long double representation
#endif


int fmt_fp(char*, long double, int, int, int, int);

#endif
