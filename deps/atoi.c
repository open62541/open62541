/* Originally released by the musl project (http://www.musl-libc.org/) under the
 * MIT license. Taken and adapted from the file src/stdlib/atoi.c 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

#include "atoi.h"

size_t atoiUnsigned(const char *pos, size_t size, UA_UInt64 *result) {
    size_t i = 0;
    UA_UInt64 n = 0;
    while(i < size) {
        if(pos[i] < '0' || pos[i] > '9')
            break;
        n *= 10;
        n += (UA_UInt64)(pos[i] - '0');
        i++;
    }
    
    *result = n;
    return i;
}

size_t atoiSigned(const char *pos, size_t size, UA_Int64 *result) {
    size_t i = 0;
    UA_Int64 n = 0;
    UA_Boolean neg = false;
    if(*pos == '-') {
        neg = true;
        pos++;
    }

    while(i < size) {
        if(pos[i] < '0' || pos[i] > '9')
            break;
        n *= 10;
        n += pos[i] - '0';
        i++;
    }

    if(neg)
        n = -n;

    *result = n;
    return i;
}
