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

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {


    struct message m;
    memset(&m, 0, sizeof(struct message));

    unsigned char *dataCopy = (unsigned char *)malloc(size);
    if (!dataCopy) {
        return 0;
    }

    memcpy(dataCopy, data, size);

    bool success = message_parse(&m, dataCopy, size);

    free(dataCopy);

    if (!success)
        return 0;

    mdns_daemon_t *d = mdnsd_new(QCLASS_IN, 1000);

    in_addr_t addr = 0;
    mdnsd_in(d, &m, addr, 2000);

    mdnsd_free(d);

    return 0;
}
