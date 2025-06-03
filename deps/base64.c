/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2025, Julius Pfrommer (Fraunhofer IOSB)
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
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

static unsigned char dtable[128] = {
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f, 0x7f, 0x80, 0x80, 0x7f, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 62  , 0x80, 62  , 0x80, 63  ,
	52  , 53  , 54  , 55  , 56  , 57  , 58  , 59  , 60  , 61  , 0x80, 0x80, 0x80, 0   , 0x80, 0x80,
	0x80, 0   , 1   , 2   , 3   , 4   , 5   , 6   , 7   , 8   , 9   , 10  , 11  , 12  , 13  , 14  ,
	15  , 16  , 17  , 18  , 19  , 20  , 21  , 22  , 23  , 24  , 25  , 0x80, 0x80, 0x80, 0x80, 63  ,
	0x80, 26  , 27  , 28  , 29  , 30  , 31  , 32  , 33  , 34  , 35  , 36  , 37  , 38  , 39  , 40  ,
	41  , 42  , 43  , 44  , 45  , 46  , 47  , 48  , 49  , 50  , 51  , 0x80, 0x80, 0x80, 0x80, 0x80
};

unsigned char *
UA_unbase64(const unsigned char *src, size_t len, size_t *out_len) {
    /* Empty base64 results in an empty byte-string */
    if(len == 0) {
        *out_len = 0;
        return (unsigned char*)UA_EMPTY_ARRAY_SENTINEL;
    }

    /* Allocate the output string. Append four bytes to allow missing padding */
	size_t olen = (len / 4 * 3) + 4;
    unsigned char *out = (unsigned char*)UA_malloc(olen);
	if(!out)
		return NULL;

    /* Iterate over the input */
	size_t pad = 0;
    unsigned char count = 0;
    unsigned char block[4];
    unsigned char *pos = out;
	for(size_t i = 0; i < len; i++) {
        /* Process character */
		unsigned char tmp = dtable[src[i] & 0x7f];
        if(tmp == 0x80)
            goto error;
        if(tmp == 0x7f)
            continue; /* Whitespace is ignored to accomodate RFC 2045, used in
                       * XML for xs:base64Binary. */
		block[count++] = tmp;

        /* Allow padding in the middle.
         * For example if base64 streams have been concatenated */
        if(src[i] == '=')
            pad++;

        /* Premature end of input. Fill up the block with padding. */
        if(i + 1 == len) {
            len = (len + 3) & ~0x03; /* Next multiple of four */
            for(i++; i < len; i++)
                pad++;
            for(; count < 4; count++)
                block[count] = 0;
        }

        /* Write three output characters for four characters of input */
		if(count == 4) {
            if(pad > 2)
                goto error; /* Invalid padding */
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
            pos -= pad;
            pad = 0;
		}
	}

	*out_len = (size_t)(pos - out);
    if(*out_len == 0) {
        UA_free(out);
        return (unsigned char*)UA_EMPTY_ARRAY_SENTINEL;
    }
    return out;

 error:
    UA_free(out);
    return NULL;
}
