/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "utf8.h"

#define UTF_PARSE_BYTE(pos) do {                        \
        byte = str[pos];                                \
        if(UTF_UNLIKELY(byte < 0x80 || byte > 0xBF))    \
            return 0; /* Not a continuation byte */     \
        *codepoint = (*codepoint << 6) + (byte & 0x3F); \
    } while(0)

unsigned
utf8_to_codepoint(const unsigned char *str, size_t len, unsigned *codepoint) {
    /* Ensure there is a character to read */
    if(UTF_UNLIKELY(len == 0))
        return 0;

    *codepoint = str[0];
    if(UTF_LIKELY(*codepoint < 0x80))
        return 1; /* Normal ASCII */

    if(UTF_UNLIKELY(*codepoint <= 0xC1))
        return 0; /* Continuation byte not allowed here */

    unsigned count;
    unsigned char byte;
    if(*codepoint <= 0xDF) { /* 2-byte sequence */
        if(len < 2)
            return 0;
        count = 2;
        *codepoint &= 0x1F;
        UTF_PARSE_BYTE(1);
        if(UTF_UNLIKELY(*codepoint < 0x80))
            return 0; /* Too small for the encoding length */
    } else if(*codepoint <= 0xEF) { /* 3-byte sequence */
        if(len < 3)
            return 0;
        count = 3;
        *codepoint &= 0xF;
        UTF_PARSE_BYTE(1);
        UTF_PARSE_BYTE(2);
        if(UTF_UNLIKELY(*codepoint < 0x800))
            return 0; /* Too small for the encoding length */
    } else if(*codepoint <= 0xF4) { /* 4-byte sequence */
        if(len < 4)
            return 0;
        count = 4;
        *codepoint &= 0x7;
        UTF_PARSE_BYTE(1);
        UTF_PARSE_BYTE(2);
        UTF_PARSE_BYTE(3);
        if(UTF_UNLIKELY(*codepoint < 0x10000))
            return 0; /* Too small for the encoding length */
    } else {
        return 0; /* Invalid utf8 encoding */
    }

    if(UTF_UNLIKELY(*codepoint > 0x10FFFF))
        return 0; /* Not in the Unicode range */

    return count;
}
