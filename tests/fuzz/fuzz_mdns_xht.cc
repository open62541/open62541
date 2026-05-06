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

unsigned char message_buf[MAX_PACKET_LEN];

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size >= MAX_PACKET_LEN)
        return 0;
    memcpy(message_buf, data, size);
    message_buf[size] = 0; /* zero terminate */

    xht_t *sd = txt2sd(message_buf, size);
    if(!sd)
        return 0;

    int len;
    unsigned char *out = sd2txt(sd, &len);
    assert(out != NULL);

    free(out);
    xht_free(sd);

    return 0;
}
