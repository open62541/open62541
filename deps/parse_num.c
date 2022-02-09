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

size_t atoiUnsigned(const char *pos, size_t size, uint64_t *result) {
    size_t i = 0;
    uint64_t n = 0;
    bool digit = false; /* At least one real digit */
    if(size - i >= 2 && pos[i] == '0' && (pos[i+1] | 32) == 'x') {
        /* Hex */
        i += 2;
        for(; i < size; i++) {
            uint8_t c = (uint8_t)pos[i] | 32;
            if(c >= '0' && c <= '9')
                c = c - '0';
            else if(c >= 'a' && c <='f')
                c = c - 'a' + 10;
            else if(c >= 'A' && c <='F')
                c = c - 'A' + 10;
            else
                break;
            digit = true;
            n = (n << 4) | (n & 0xF);
        }
    } else {
        /* Decimal */
        for(; i < size; i++) {
            if(pos[i] < '0' || pos[i] > '9')
                break;
            n *= 10;
            digit = true;
            n +=(uint8_t)(pos[i] - '0');
        }
    }

    /* No digit was parsed */
    if(!digit)
        return 0;

    *result = n;
    return i;
}

size_t atoiSigned(const char *pos, size_t size, int64_t *result) {
    /* Negative value? */
    size_t i = 0;
    bool neg = false;
    if(*pos == '-' || *pos == '+') {
        neg = (*pos == '-');
        i++;
    }

    /* Parse as unsigned */
    uint64_t n = 0;
    size_t len = atoiUnsigned(&pos[i], size - i, &n);
    if(len == 0)
        return 0;

    /* Adjust and return */
    *result = (!neg) ? (int64_t)n  : -(int64_t)n;
    return len + i;
}
