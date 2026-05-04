/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) fortiss (Author: Stefan Profanter)
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <open62541/config.h>

extern "C" {
#include "mdnsd.h"
#include "xht.h"
#include "sdtxt.h"
}

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {


    size_t halfSize = size / 2;
    size_t remainSize = size - halfSize;
    const uint8_t *secondHalfData = data + halfSize;

    // Cast away const for txt2sd (function doesn't modify input)
    xht_t *sd = txt2sd((unsigned char *)data, halfSize);

    uint8_t *dataCopy = (unsigned char *)malloc(remainSize);
    if (!dataCopy) {
        xht_free(sd);
        return 0;
    }
    memcpy(dataCopy, secondHalfData, remainSize);
    // make sure we have a valid null-terminated string
    if (remainSize > 0)
        dataCopy[remainSize-1] = 0;

    xht_set(sd, (char*)dataCopy, dataCopy);

    int len;
    unsigned char* out = sd2txt(sd, &len);

    free(out);
    xht_free(sd);

    free(dataCopy);

    return 0;
}
