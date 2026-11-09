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

size_t
parseUInt64(const char *str, size_t size, uint64_t *result) {
    size_t i = 0;
    uint64_t n = 0;

    /* Hex */
    if(size > 2 && str[0] == '0' && (str[1] | 32) == 'x') {
        i = 2;
        for(; i < size; i++) {
            uint8_t c = (uint8_t)str[i] | 32;
            if(c >= '0' && c <= '9')
                c = (uint8_t)(c - '0');
            else if(c >= 'a' && c <='f')
                c = (uint8_t)(c - 'a' + 10);
            else if(c >= 'A' && c <='F')
                c = (uint8_t)(c - 'A' + 10);
            else
                break;
            /* Check for overflow before the shift drops the top bits */
            if(n > (UINT64_MAX >> 4))
                return 0;
            n = (n << 4) | (c & 0xF);
        }
        *result = n;
        return (i > 2) ? i : 0; /* 2 -> No digit was parsed */
    }

    /* Decimal */
    for(; i < size; i++) {
        if(str[i] < '0' || str[i] > '9')
            break;
        uint8_t digit = (uint8_t)(str[i] - '0');
        /* Check for overflow before n*10 + digit wraps around. A wrapped
         * result is not necessarily smaller than the previous value. Both
         * bounds are compile-time constants. */
        if(n > UINT64_MAX / 10 || (n == UINT64_MAX / 10 && digit > UINT64_MAX % 10))
            return 0;
        /* Fast multiplication: n*10 == (n*8) + (n*2) */
        n = (n << 3) + (n << 1) + digit;
    }
    *result = n;
    return i;
}

size_t
parseInt64(const char *str, size_t size, int64_t *result) {
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
        /* n is unsigned, so 9223372036854775808UL == 2^63 fits without
         * overflow. (int64_t)n would also fit for n in [0, 2^63], but
         * the negation -(int64_t)(2^63) is undefined because the result
         * (which is +2^63) cannot be represented. Use the unsigned
         * representation and reinterpret as int64_t instead. */
        if(n > 9223372036854775808UL)
            return 0;
        *result = (n == 9223372036854775808UL)
            ? (int64_t)(-9223372036854775807LL - 1)
            : -(int64_t)n;
    }
    return len + i;
}
