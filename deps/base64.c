/*
 * Base64 encoding: Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 * This software may be distributed under the terms of the BSD license.
 *
 * Base64 decoding: Copyright (c) 2016, polfosol
 * Posted at https://stackoverflow.com/a/37109258 under the CC-BY-SA Creative
 * Commons license.
 */

#include "base64.h"
#include <open62541/types.h>

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char *
UA_base64(const unsigned char *src, size_t len, size_t *out_len) {
    if(len == 0) {
        *out_len = 0;
        return (unsigned char*)UA_EMPTY_ARRAY_SENTINEL;
    }

	size_t olen = 4*((len + 2) / 3); /* 3-byte blocks to 4-byte */
	if(olen < len)
		return NULL; /* integer overflow */

	unsigned char *out = (unsigned char*)UA_malloc(olen);
	if(!out)
		return NULL;

    *out_len = UA_base64_buf(src, len, out);
    return out;
}

size_t
UA_base64_buf(const unsigned char *src, size_t len, unsigned char *out) {
	const unsigned char *end = src + len;
	const unsigned char *in = src;
	unsigned char *pos = out;
	while(end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}

	if(end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if(end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

    return (size_t)(pos - out);
}

static const uint32_t from_b64[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63,
    0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

unsigned char *
UA_unbase64(const unsigned char *src, size_t len, size_t *out_len) {
    // we need a minimum length
    if(len <= 2) {
        *out_len = 0;
        return (unsigned char*)UA_EMPTY_ARRAY_SENTINEL;
    }

    const unsigned char *p = src;
    size_t pad1 = len % 4 || p[len - 1] == '=';
    size_t pad2 = pad1 && (len % 4 > 2 || p[len - 2] != '=');
    const size_t last = (len - pad1) / 4 << 2;

    unsigned char *str = (unsigned char*)UA_malloc(last / 4 * 3 + pad1 + pad2);
    if(!str)
        return NULL;

    unsigned char *pos = str;
    for(size_t i = 0; i < last; i += 4) {
        uint32_t n = from_b64[p[i]] << 18 | from_b64[p[i + 1]] << 12 |
                     from_b64[p[i + 2]] << 6 | from_b64[p[i + 3]];
        *pos++ = (unsigned char)(n >> 16);
        *pos++ = (unsigned char)(n >> 8 & 0xFF);
        *pos++ = (unsigned char)(n & 0xFF);
    }

    if(pad1) {
        if (last + 1 >= len) {
            UA_free(str);
            *out_len = 0;
            return (unsigned char*)UA_EMPTY_ARRAY_SENTINEL;
        }
        uint32_t n = from_b64[p[last]] << 18 | from_b64[p[last + 1]] << 12;
        *pos++ = (unsigned char)(n >> 16);
        if(pad2) {
            if (last + 2 >= len) {
                UA_free(str);
                *out_len = 0;
                return (unsigned char*)UA_EMPTY_ARRAY_SENTINEL;
            }
            n |= from_b64[p[last + 2]] << 6;
            *pos++ = (unsigned char)(n >> 8 & 0xFF);
        }
    }

    *out_len = (uintptr_t)(pos - str);
    return str;
}
