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
}

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

    int parseResult = message_parse(&m, dataCopy);

    free(dataCopy);

    if (!parseResult)
        return 0;

    mdns_daemon_t *d = mdnsd_new(QCLASS_IN, 1000);

    struct in_addr addr = {};
    mdnsd_in(d, &m, addr, 2000);

    mdnsd_free(d);

    return 0;
}
