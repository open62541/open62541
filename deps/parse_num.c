/* Originally released by the musl project (http://www.musl-libc.org/) under the
 * MIT license. Taken and adapted from the file src/stdlib/atoi.c 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "parse_num.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

size_t parseUInt64(const char *str, size_t size, uint64_t *result) {
    size_t i = 0;
    uint64_t n = 0, prev = 0;
    bool digit = false; /* At least one real digit */
    if(size - i >= 2 && str[i] == '0' && (str[i+1] | 32) == 'x') {
        /* Hex */
        i += 2;
        for(; i < size; i++) {
            /* Get the byte value */
            uint8_t c = (uint8_t)str[i] | 32;
            if(c >= '0' && c <= '9')
                c = (uint8_t)(c - '0');
            else if(c >= 'a' && c <='f')
                c = (uint8_t)(c - 'a' + 10);
            else if(c >= 'A' && c <='F')
                c = (uint8_t)(c - 'A' + 10);
            else
                break;
            /* Update */
            digit = true;
            n = (n << 4) | (c & 0xF);
            if(n < prev)
            /* Check for overflow */
                return 0;
            prev = n;
        }
    } else {
        /* Decimal */
        for(; i < size; i++) {
            /* Update */
            if(str[i] < '0' || str[i] > '9')
                break;
            n *= 10;
            digit = true;
            n +=(uint8_t)(str[i] - '0');
            if(n < prev) /* Check for overflow */
                return 0;
            prev = n;
        }
    }

    /* No digit was parsed */
    if(!digit)
        return 0;

    *result = n;
    return i;
}

size_t parseInt64(const char *str, size_t size, int64_t *result) {
    /* Negative value? */
    size_t i = 0;
    bool neg = false;
    if(*str == '-' || *str == '+') {
        neg = (*str == '-');
        i++;
    }

    /* Parse as unsigned */
    uint64_t n = 0;
    size_t len = parseUInt64(&str[i], size - i, &n);
    if(len == 0)
        return 0;

    /* Check for overflow, adjust and return */
    if(!neg) {
        if(n > 9223372036854775807UL)
            return 0;
        *result = (int64_t)n;
    } else {
        if(n > 9223372036854775808UL)
            return 0;
        *result = -(int64_t)n;
    }
    return len + i;
}

size_t parseDouble(const char *str, size_t size, double *result) {
    char buf[2000];
    if(size >= 2000)
        return 0;
    memcpy(buf, str, size);
    buf[size] = 0;
    errno = 0;
    char *endptr;
    *result = strtod(str, &endptr);
    if(errno != 0)
        return 0;
    return (uintptr_t)endptr - (uintptr_t)str;
}
