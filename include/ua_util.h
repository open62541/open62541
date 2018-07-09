/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */


#ifndef UA_HELPER_H_
#define UA_HELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_config.h"
#include "ua_types.h"

/**
* Endpoint URL Parser
* -------------------
* The endpoint URL parser is generally useful for the implementation of network
* layer plugins. */

/* Split the given endpoint url into hostname, port and path. All arguments must
 * be non-NULL. EndpointUrls have the form "opc.tcp://hostname:port/path", port
 * and path may be omitted (together with the prefix colon and slash).
 *
 * @param endpointUrl The endpoint URL.
 * @param outHostname Set to the parsed hostname. The string points into the
 *        original endpointUrl, so no memory is allocated. If an IPv6 address is
 *        given, hostname contains e.g. '[2001:0db8:85a3::8a2e:0370:7334]'
 * @param outPort Set to the port of the url or left unchanged.
 * @param outPath Set to the path if one is present in the endpointUrl.
 *        Starting or trailing '/' are NOT included in the path. The string
 *        points into the original endpointUrl, so no memory is allocated.
 * @return Returns UA_STATUSCODE_BADTCPENDPOINTURLINVALID if parsing failed. */
UA_StatusCode UA_EXPORT
UA_parseEndpointUrl(const UA_String *endpointUrl, UA_String *outHostname,
                    UA_UInt16 *outPort, UA_String *outPath);

/**
 * Convenience macros for complex types
 * ------------------------------------ */
#define UA_PRINTF_GUID_FORMAT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define UA_PRINTF_GUID_DATA(GUID) (GUID).data1, (GUID).data2, (GUID).data3, \
        (GUID).data4[0], (GUID).data4[1], (GUID).data4[2], (GUID).data4[3], \
        (GUID).data4[4], (GUID).data4[5], (GUID).data4[6], (GUID).data4[7]

#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
#define UA_PRINTF_STRING_DATA(STRING) (int)(STRING).length, (STRING).data

/**
 * Helper functions for converting data types
 * ------------------------------------ */
/*
 * Converts a bytestring to the corresponding base64 encoded string representation.
 *
 * @param byteString the original byte string
 * @param str the resulting base64 encoded byte string
 *
 * @return UA_STATUSCODE_GOOD on success.
 */
UA_StatusCode UA_EXPORT
UA_ByteString_toBase64String(const UA_ByteString *byteString, UA_String *str);

/*
 * Converts a node id to the corresponding string representation.
 * It can be one of:
 * - Numeric: ns=0;i=123
 * - String: ns=0;s=Some String
 * - Guid: ns=0;g=A123456C-0ABC-1A2B-815F-687212AAEE1B
 * - ByteString: ns=0;b=AA==
 *
 */
UA_StatusCode UA_EXPORT
UA_NodeId_toString(const UA_NodeId *nodeId, UA_String *nodeIdStr);


#ifdef __cplusplus
}
#endif

#endif /* UA_HELPER_H_ */
