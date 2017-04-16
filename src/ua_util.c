/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_connection.h"

size_t
UA_readNumber(u8 *buf, size_t buflen, u32 *number) {
    UA_assert(buf);
    UA_assert(number);
    u32 n = 0;
    size_t progress = 0;
    /* read numbers until the end or a non-number character appears */
    while(progress < buflen) {
        u8 c = buf[progress];
        if(c < '0' || c > '9')
            break;
        n = (n*10) + (u32)(c-'0');
        ++progress;
    }
    *number = n;
    return progress;
}

UA_StatusCode
UA_parseEndpointUrl(const UA_String *endpointUrl, UA_String *outHostname,
                    u16 *outPort, UA_String *outPath) {
    /* Url must begin with "opc.tcp://" */
    if(endpointUrl->length < 11 || strncmp((char*)endpointUrl->data, "opc.tcp://", 10) != 0)
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;

    /* Where does the hostname end? */
    size_t pos = 10;
    if(endpointUrl->data[pos] == '[') {
        /* IPv6: opc.tcp://[2001:0db8:85a3::8a2e:0370:7334]:1234/path */
        for(; pos < endpointUrl->length; ++pos) {
            if(endpointUrl->data[pos] == ']')
                break;
        }
        if(pos == endpointUrl->length)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        pos++;
    } else {
        /* IPv4 or hostname: opc.tcp://something.something:1234/path */
        for(; pos < endpointUrl->length; ++pos) {
            if(endpointUrl->data[pos] == ':' || endpointUrl->data[pos] == '/')
                break;
        }
    }

    /* Set the hostname */
    outHostname->data = &endpointUrl->data[10];
    outHostname->length = pos - 10;
    if(pos == endpointUrl->length)
        return UA_STATUSCODE_GOOD;

    /* Set the port */
    if(endpointUrl->data[pos] == ':') {
        if(++pos == endpointUrl->length)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        u32 largeNum;
        size_t progress = UA_readNumber(&endpointUrl->data[pos], endpointUrl->length - pos, &largeNum);
        if(progress == 0 || largeNum > 65535)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        /* Test if the end of a valid port was reached */
        pos += progress;
        if(pos == endpointUrl->length || endpointUrl->data[pos] == '/')
            *outPort = (u16)largeNum;
        if(pos == endpointUrl->length)
            return UA_STATUSCODE_GOOD;
    }

    /* Set the path */
    UA_assert(pos < endpointUrl->length);
    if(endpointUrl->data[pos] != '/')
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    if(++pos == endpointUrl->length)
        return UA_STATUSCODE_GOOD;
    outPath->data = &endpointUrl->data[pos];
    outPath->length = endpointUrl->length - pos;

    /* Remove trailing slash from the path */
    if(endpointUrl->data[endpointUrl->length - 1] == '/')
        outPath->length--;

    return UA_STATUSCODE_GOOD;
}
