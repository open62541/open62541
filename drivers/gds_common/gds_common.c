/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

#include "gds_common.h"

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

#include <string.h>

static const char pemHeader[] = "-----BEGIN CERTIFICATE-----";
static const char pemFooter[] = "-----END CERTIFICATE-----";

/* Returns the position of the first match at or after start, or the length of
 * the ByteString if there is no match */
static size_t
findString(const UA_ByteString bs, size_t start, const char *str) {
    size_t strLen = strlen(str);
    for(size_t i = start; i + strLen <= bs.length; i++) {
        if(memcmp(&bs.data[i], str, strLen) == 0)
            return i;
    }
    return bs.length;
}

UA_ByteString
UA_GDSReceiver_getLeafCertificate(UA_ByteString chain) {
    /* Detect DER encoded X.509 v3 certificate. The certificate length is
     * encoded in the two bytes after the magic numbers. */
    if(chain.length >= 4 && chain.data[0] == 0x30 && chain.data[1] == 0x82) {
        size_t leafLen = 4; /* Magic numbers + length bytes */
        leafLen += (size_t)(((uint16_t)chain.data[2]) << 8);
        leafLen += chain.data[3];
        if(leafLen > chain.length)
            return UA_BYTESTRING_NULL;
        chain.length = leafLen;
        return chain;
    }

    /* Detect PEM. The leaf starts at the first header, so that the crypto
     * backends detect PEM by the first character. Text before the header is
     * cut off. */
    size_t begin = findString(chain, 0, pemHeader);
    if(begin == chain.length)
        return chain;
    size_t end = findString(chain, begin, pemFooter);
    if(end == chain.length)
        return UA_BYTESTRING_NULL;

    /* Keep the line break after the footer */
    end += strlen(pemFooter);
    if(end < chain.length && chain.data[end] == '\r')
        end++;
    if(end < chain.length && chain.data[end] == '\n')
        end++;

    chain.data += begin;
    chain.length = end - begin;
    return chain;
}

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */
