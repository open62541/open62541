/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UTF8_H_
#define UTF8_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
# define UTF_INLINE __inline
#else
# define UTF_INLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
# define UTF_LIKELY(x) __builtin_expect((x), 1)
# define UTF_UNLIKELY(x) __builtin_expect((x), 0)
#else
# define UTF_LIKELY(x) (x)
# define UTF_UNLIKELY(x) (x)
#endif

/* Extract the next utf8 codepoint from the buffer. Returns the length (1-4) of
 * the codepoint encoding or 0 upon an error. */
unsigned
utf8_to_codepoint(const unsigned char *str, size_t len, unsigned *codepoint);

/* Encodes the codepoint in utf8. The string needs to have enough space (at most
 * four byte) available. Returns the encoding length. */
static UTF_INLINE unsigned
utf8_from_codepoint(unsigned char *str, unsigned codepoint) {
    if(UTF_LIKELY(codepoint <= 0x7F)) { /* Plain ASCII */
        str[0] = (unsigned char)codepoint;
        return 1;
    }
    if(UTF_LIKELY(codepoint <= 0x07FF)) { /* 2-byte unicode */
        str[0] = (unsigned char)(((codepoint >> 6) & 0x1F) | 0xC0);
        str[1] = (unsigned char)(((codepoint >> 0) & 0x3F) | 0x80);
        return 2;
    }
    if(UTF_LIKELY(codepoint <= 0xFFFF)) { /* 3-byte unicode */
        str[0] = (unsigned char)(((codepoint >> 12) & 0x0F) | 0xE0);
        str[1] = (unsigned char)(((codepoint >>  6) & 0x3F) | 0x80);
        str[2] = (unsigned char)(((codepoint >>  0) & 0x3F) | 0x80);
        return 3;
    }
    if(UTF_LIKELY(codepoint <= 0x10FFFF)) { /* 4-byte unicode */
        str[0] = (unsigned char)(((codepoint >> 18) & 0x07) | 0xF0);
        str[1] = (unsigned char)(((codepoint >> 12) & 0x3F) | 0x80);
        str[2] = (unsigned char)(((codepoint >>  6) & 0x3F) | 0x80);
        str[3] = (unsigned char)(((codepoint >>  0) & 0x3F) | 0x80);
        return 4;
    }
    return 0; /* Not a unicode codepoint */
}

/* Returns the encoding length of the codepoint */
static UTF_INLINE unsigned
utf8_length(unsigned codepoint) {
    if(UTF_LIKELY(codepoint <= 0x7F))
        return 1; /* Plain ASCII */
    if(UTF_LIKELY(codepoint <= 0x07FF))
        return 2; /* 2-byte unicode */
    if(UTF_LIKELY(codepoint <= 0xFFFF))
        return 3; /* 3-byte unicode */
    if(UTF_LIKELY(codepoint <= 0x10FFFF))
        return 4; /* 4-byte unicode */
    return 0; /* Not a unicode codepoint */
}

#ifdef __cplusplus
}
#endif

#endif /* UTF8_H_ */
