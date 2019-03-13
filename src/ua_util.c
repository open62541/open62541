/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014, 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/types_generated_handling.h>
#include <open62541/util.h>

#include "ua_util_internal.h"

#include "base64.h"

size_t
UA_readNumberWithBase(const UA_Byte *buf, size_t buflen, UA_UInt32 *number, UA_Byte base) {
    UA_assert(buf);
    UA_assert(number);
    u32 n = 0;
    size_t progress = 0;
    /* read numbers until the end or a non-number character appears */
    while(progress < buflen) {
        u8 c = buf[progress];
        if(c >= '0' && c <= '9' && c <= '0' + (base-1))
           n = (n * base) + c - '0';
        else if(base > 9 && c >= 'a' && c <= 'z' && c <= 'a' + (base-11))
           n = (n * base) + c-'a' + 10;
        else if(base > 9 && c >= 'A' && c <= 'Z' && c <= 'A' + (base-11))
           n = (n * base) + c-'A' + 10;
        else
           break;
        ++progress;
    }
    *number = n;
    return progress;
}

size_t
UA_readNumber(UA_Byte *buf, size_t buflen, UA_UInt32 *number)
{
    return UA_readNumberWithBase(buf, buflen, number, 10);
}

UA_StatusCode
UA_parseEndpointUrl(const UA_String *endpointUrl, UA_String *outHostname,
                    u16 *outPort, UA_String *outPath) {
    /* Url must begin with "opc.tcp://" or opc.udp:// (if pubsub enabled) */
    if(endpointUrl->length < 11) {
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    }
    if (strncmp((char*)endpointUrl->data, "opc.tcp://", 10) != 0) {
#ifdef UA_ENABLE_PUBSUB
        if (strncmp((char*)endpointUrl->data, "opc.udp://", 10) != 0 &&
                strncmp((char*)endpointUrl->data, "opc.mqtt://", 11) != 0) {
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

UA_StatusCode
UA_parseEndpointUrlEthernet(const UA_String *endpointUrl, UA_String *target,
                            UA_UInt16 *vid, UA_Byte *pcp) {
    /* Url must begin with "opc.eth://" */
    if(endpointUrl->length < 11) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(strncmp((char*) endpointUrl->data, "opc.eth://", 10) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Where does the host address end? */
    size_t curr = 10;
    for(; curr < endpointUrl->length; ++curr) {
        if(endpointUrl->data[curr] == ':') {
           break;
        }
    }

    /* set host address */
    target->data = &endpointUrl->data[10];
    target->length = curr - 10;
    if(curr == endpointUrl->length) {
        return UA_STATUSCODE_GOOD;
    }

    /* Set VLAN */
    u32 value = 0;
    curr++;  /* skip ':' */
    size_t progress = UA_readNumber(&endpointUrl->data[curr],
                                    endpointUrl->length - curr, &value);
    if(progress == 0 || value > 4096) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    curr += progress;
    if(curr == endpointUrl->length || endpointUrl->data[curr] == '.') {
        *vid = (UA_UInt16) value;
    }
    if(curr == endpointUrl->length) {
        return UA_STATUSCODE_GOOD;
    }

    /* Set priority */
    if(endpointUrl->data[curr] != '.') {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    curr++;  /* skip '.' */
    progress = UA_readNumber(&endpointUrl->data[curr],
                             endpointUrl->length - curr, &value);
    if(progress == 0 || value > 7) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    curr += progress;
    if(curr != endpointUrl->length) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    *pcp = (UA_Byte) value;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_ByteString_toBase64String(const UA_ByteString *byteString, UA_String *str) {
    if (str->length != 0) {
        UA_free(str->data);
        str->data = NULL;
        str->length = 0;
    }
    if (byteString == NULL || byteString->data == NULL)
        return UA_STATUSCODE_GOOD;
    if (byteString == str)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    int resSize = 0;
    str->data = (UA_Byte*)UA_base64(byteString->data, (int)byteString->length, &resSize);
    str->length = (size_t) resSize;
    if (str->data == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NodeId_toString(const UA_NodeId *nodeId, UA_String *nodeIdStr) {
    if (nodeIdStr->length != 0) {
        UA_free(nodeIdStr->data);
        nodeIdStr->data = NULL;
        nodeIdStr->length = 0;
    }
    if (nodeId == NULL)
        return UA_STATUSCODE_GOOD;

    char *nsStr = NULL;
    long snprintfLen = 0;
    size_t nsLen = 0;
    if (nodeId->namespaceIndex != 0) {
        nsStr = (char*)UA_malloc(9+1); // strlen("ns=XXXXX;") = 9 + Nullbyte
        snprintfLen = UA_snprintf(nsStr, 10, "ns=%d;", nodeId->namespaceIndex);
        if (snprintfLen < 0 || snprintfLen >= 10) {
            UA_free(nsStr);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        nsLen = (size_t)(snprintfLen);
    }


    UA_ByteString byteStr = UA_BYTESTRING_NULL;
    switch (nodeId->identifierType) {
        case UA_NODEIDTYPE_NUMERIC:
            /* ns (2 byte, 65535) = 5 chars, numeric (4 byte, 4294967295) = 10 chars, delim = 1 , nullbyte = 1-> 17 chars */
            nodeIdStr->length = nsLen + 2 + 10 + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL) {
                nodeIdStr->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen =UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "%si=%lu",
                        nsLen > 0 ? nsStr : "",
                        (unsigned long )nodeId->identifier.numeric);
            break;
        case UA_NODEIDTYPE_STRING:
            /* ns (16bit) = 5 chars, strlen + nullbyte */
            nodeIdStr->length = nsLen + 2 + nodeId->identifier.string.length + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL) {
                nodeIdStr->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen =UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "%ss=%.*s",
                        nsLen > 0 ? nsStr : "",
                        (int)nodeId->identifier.string.length, nodeId->identifier.string.data);
            break;
        case UA_NODEIDTYPE_GUID:
            /* ns (16bit) = 5 chars + strlen(A123456C-0ABC-1A2B-815F-687212AAEE1B)=36 + nullbyte */
            nodeIdStr->length = nsLen + 2 + 36 + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL) {
                nodeIdStr->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "%sg=" UA_PRINTF_GUID_FORMAT,
                        nsLen > 0 ? nsStr : "",
                        UA_PRINTF_GUID_DATA(nodeId->identifier.guid));
            break;
        case UA_NODEIDTYPE_BYTESTRING:
            UA_ByteString_toBase64String(&nodeId->identifier.byteString, &byteStr);
            /* ns (16bit) = 5 chars + LEN + nullbyte */
            nodeIdStr->length = nsLen + 2 + byteStr.length + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL) {
                nodeIdStr->length = 0;
                UA_String_deleteMembers(&byteStr);
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "%sb=%.*s",
                        nsLen > 0 ? nsStr : "",
                        (int)byteStr.length, byteStr.data);
            UA_String_deleteMembers(&byteStr);
            break;
    }
    UA_free(nsStr);

    if (snprintfLen < 0 || snprintfLen >= (long) nodeIdStr->length) {
        UA_free(nodeIdStr->data);
        nodeIdStr->data = NULL;
        nodeIdStr->length = 0;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    nodeIdStr->length = (size_t)snprintfLen;

    return UA_STATUSCODE_GOOD;
}

