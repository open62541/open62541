/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) fortiss (Author: Stefan Profanter)
 */



/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
#include <base64.h>
#include <malloc.h>

extern "C" int
LLVMFuzzerTestOneInput(uint8_t *data, size_t size) {
    size_t retLen;
    unsigned char* ret = UA_base64(data, size, &retLen);
    if (retLen > 0)
        free(ret);
    return 0;
}
