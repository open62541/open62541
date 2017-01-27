/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include "ua_config.h"

void *memcpy(void *UA_RESTRICT dest, const void *UA_RESTRICT src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while(n--)
        *d++ = *s++;
    return dest;
}

void *memset(void *dest, int c, size_t n) {
    unsigned char c8 = (unsigned char)c;
    unsigned char *target = dest;
    while(n--)
        *target++ = c8;
    return dest;
}

size_t strlen(const char *str) {
    size_t len = 0;
    for(const char *s = str; *s; s++, len++);
    return len;
}

int memcmp(const void *vl, const void *vr, size_t n) {
    const unsigned char *l = vl, *r = vr;
    for(; n && *l == *r; n--, l++, r++);
    return n ? *l-*r : 0;
}
