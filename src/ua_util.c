/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014, 2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2014 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_util.h"
#include "ua_plugin_network.h"

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
    /* Url must begin with "opc.tcp://" or opc.udp:// (if pubsub enabled) */
    if(endpointUrl->length < 11) {
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    } else if (strncmp((char*)endpointUrl->data, "opc.tcp://", 10) != 0) {
#ifdef UA_ENABLE_PUBSUB
        if (strncmp((char*)endpointUrl->data, "opc.udp://", 10) != 0) {
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        }
#else
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
#endif
    }

    /* Where does the hostname end? */
    size_t curr = 10;
    if(endpointUrl->data[curr] == '[') {
        /* IPv6: opc.tcp://[2001:0db8:85a3::8a2e:0370:7334]:1234/path */
        for(; curr < endpointUrl->length; ++curr) {
            if(endpointUrl->data[curr] == ']')
                break;
        }
        if(curr == endpointUrl->length)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        curr++;
    } else {
        /* IPv4 or hostname: opc.tcp://something.something:1234/path */
        for(; curr < endpointUrl->length; ++curr) {
            if(endpointUrl->data[curr] == ':' || endpointUrl->data[curr] == '/')
                break;
        }
    }

    /* Set the hostname */
    outHostname->data = &endpointUrl->data[10];
    outHostname->length = curr - 10;
    if(curr == endpointUrl->length)
        return UA_STATUSCODE_GOOD;

    /* Set the port */
    if(endpointUrl->data[curr] == ':') {
        if(++curr == endpointUrl->length)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        u32 largeNum;
        size_t progress = UA_readNumber(&endpointUrl->data[curr], endpointUrl->length - curr, &largeNum);
        if(progress == 0 || largeNum > 65535)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        /* Test if the end of a valid port was reached */
        curr += progress;
        if(curr == endpointUrl->length || endpointUrl->data[curr] == '/')
            *outPort = (u16)largeNum;
        if(curr == endpointUrl->length)
            return UA_STATUSCODE_GOOD;
    }

    /* Set the path */
    UA_assert(curr < endpointUrl->length);
    if(endpointUrl->data[curr] != '/')
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    if(++curr == endpointUrl->length)
        return UA_STATUSCODE_GOOD;
    outPath->data = &endpointUrl->data[curr];
    outPath->length = endpointUrl->length - curr;

    /* Remove trailing slash from the path */
    if(endpointUrl->data[endpointUrl->length - 1] == '/')
        outPath->length--;

    return UA_STATUSCODE_GOOD;
}
