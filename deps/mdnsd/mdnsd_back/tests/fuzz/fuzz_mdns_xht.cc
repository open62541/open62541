/* This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/> */

#include <cstdint>
#include <cstdio>
#include <libmdnsd/mdnsd.h>
#include <cstring>
#include <cstdlib>
#include <libmdnsd/xht.h>
#include <libmdnsd/sdtxt.h>

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {


    size_t halfSize = size / 2;
    size_t remainSize = size - halfSize;
    const uint8_t *secondHalfData = data + halfSize;

    xht_t *sd = txt2sd((const unsigned  char*)data, halfSize);


    uint8_t *dataCopy = (unsigned char *)malloc(remainSize);
    if (!dataCopy) {
        return 0;
    }
    memcpy(dataCopy, secondHalfData, remainSize);
    // make sure we have a valid null-terminated string
    if (remainSize > 0)
        dataCopy[remainSize-1] = 0;

    xht_set(sd, (char*)dataCopy, dataCopy);

    int len;
    unsigned char* out = sd2txt(sd, &len);

    MDNSD_free(out);
    xht_free(sd);

    free(dataCopy);

    return 0;
}
