/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_HELPER_H_
#define UA_HELPER_H_

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

_UA_BEGIN_DECLS

/**
 * Forward Declarations
 * --------------------
 * Opaque pointers used by the plugins. */

struct UA_Server;
typedef struct UA_Server UA_Server;

struct UA_ServerConfig;
typedef struct UA_ServerConfig UA_ServerConfig;

typedef void (*UA_ServerCallback)(UA_Server *server, void *data);

struct UA_Client;
typedef struct UA_Client UA_Client;

/* Timer policy to handle cycle misses */
typedef enum {
    UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
    UA_TIMER_HANDLE_CYCLEMISS_WITH_BASETIME
} UA_TimerPolicy;

/**
 * Key Value Map
 * -------------
 * Helper functions to work with configuration parameters in an array of
 * UA_KeyValuePair. Lookup is linear. So this is for small numbers of
 * keys. */

/* Makes a copy of the value. Can reallocate the underlying array. This
 * invalidates pointers into the previous array. If the key exists already, the
 * value is overwritten. */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_set(UA_KeyValuePair **map, size_t *mapSize,
                   const UA_QualifiedName key,
                   const UA_Variant *value);

/* Returns a pointer to the value or NULL if the key is not found.*/
UA_EXPORT const UA_Variant *
UA_KeyValueMap_get(const UA_KeyValuePair *map, size_t mapSize,
                   const UA_QualifiedName key);

/* Returns NULL if the value for the key is not defined or not of the right
 * datatype and scalar/array */
UA_EXPORT const void *
UA_KeyValueMap_getScalar(const UA_KeyValuePair *map, size_t mapSize,
                         const UA_QualifiedName key,
                         const UA_DataType *type);

/* Remove a single entry. To delete the entire map, use UA_Array_delete. */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_delete(UA_KeyValuePair **map, size_t *mapSize,
                      const UA_QualifiedName key);

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

/* Split the given endpoint url into hostname, vid and pcp. All arguments must
 * be non-NULL. EndpointUrls have the form "opc.eth://<host>[:<VID>[.PCP]]".
 * The host is a MAC address, an IP address or a registered name like a
 * hostname. The format of a MAC address is six groups of hexadecimal digits,
 * separated by hyphens (e.g. 01-23-45-67-89-ab). A system may also accept
 * hostnames and/or IP addresses if it provides means to resolve it to a MAC
 * address (e.g. DNS and Reverse-ARP).
 *
 * Note: currently only parsing MAC address is supported.
 *
 * @param endpointUrl The endpoint URL.
 * @param vid Set to VLAN ID.
 * @param pcp Set to Priority Code Point.
 * @return Returns UA_STATUSCODE_BADINTERNALERROR if parsing failed. */
UA_StatusCode UA_EXPORT
UA_parseEndpointUrlEthernet(const UA_String *endpointUrl, UA_String *target,
                            UA_UInt16 *vid, UA_Byte *pcp);

/* Convert given byte string to a positive number. Returns the number of valid
 * digits. Stops if a non-digit char is found and returns the number of digits
 * up to that point. */
size_t UA_EXPORT
UA_readNumber(const UA_Byte *buf, size_t buflen, UA_UInt32 *number);

/* Same as UA_ReadNumber but with a base parameter */
size_t UA_EXPORT
UA_readNumberWithBase(const UA_Byte *buf, size_t buflen,
                      UA_UInt32 *number, UA_Byte base);

#ifndef UA_MIN
#define UA_MIN(A, B) ((A) > (B) ? (B) : (A))
#endif

#ifndef UA_MAX
#define UA_MAX(A, B) ((A) > (B) ? (A) : (B))
#endif

/**
 * Parse RelativePath Expressions
 * ------------------------------
 *
 * Parse a RelativePath according to the format defined in Part 4, A2. This is
 * used e.g. for the BrowsePath structure. For now, only the standard
 * ReferenceTypes from Namespace 0 are recognized (see Part 3).
 *
 *   ``RelativePath := ( ReferenceType [BrowseName]? )*``
 *
 * The ReferenceTypes have either of the following formats:
 *
 * - ``/``: *HierarchicalReferences* and subtypes
 * - ``.``: *Aggregates* ReferenceTypesand subtypes
 * - ``< [!#]* BrowseName >``: The ReferenceType is indicated by its BrowseName
 *   (a QualifiedName). Prefixed modifiers can be as follows: ``!`` switches to
 *   inverse References. ``#`` excludes subtypes of the ReferenceType.
 *
 * QualifiedNames consist of an optional NamespaceIndex and the nameitself:
 *
 *   ``QualifiedName := ([0-9]+ ":")? Name``
 *
 * The QualifiedName representation for RelativePaths uses ``&`` as the escape
 * character. Occurences of the characters ``/.<>:#!&`` in a QualifiedName have
 * to be escaped (prefixed with ``&``).
 *
 * Example RelativePaths
 * `````````````````````
 *
 * - ``/2:Block&.Output``
 * - ``/3:Truck.0:NodeVersion``
 * - ``<0:HasProperty>1:Boiler/1:HeatSensor``
 * - ``<0:HasChild>2:Wheel``
 * - ``<#Aggregates>1:Boiler/``
 * - ``<!HasChild>Truck``
 * - ``<HasChild>``
 */
#ifdef UA_ENABLE_PARSING
UA_EXPORT UA_StatusCode
UA_RelativePath_parse(UA_RelativePath *rp, const UA_String str);
#endif

/**
 * Convenience macros for complex types
 * ------------------------------------ */
#define UA_PRINTF_GUID_FORMAT "%08" PRIx32 "-%04" PRIx16 "-%04" PRIx16 \
    "-%02" PRIx8 "%02" PRIx8 "-%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8
#define UA_PRINTF_GUID_DATA(GUID) (GUID).data1, (GUID).data2, (GUID).data3, \
        (GUID).data4[0], (GUID).data4[1], (GUID).data4[2], (GUID).data4[3], \
        (GUID).data4[4], (GUID).data4[5], (GUID).data4[6], (GUID).data4[7]

#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
#define UA_PRINTF_STRING_DATA(STRING) (int)(STRING).length, (STRING).data

/**
 * Helper functions for converting data types
 * ------------------------------------------ */

/* Compare memory in constant time to mitigate timing attacks.
 * Returns true if ptr1 and ptr2 are equal for length bytes. */
static UA_INLINE UA_Boolean
UA_constantTimeEqual(const void *ptr1, const void *ptr2, size_t length) {
    volatile const UA_Byte *a = (volatile const UA_Byte *)ptr1;
    volatile const UA_Byte *b = (volatile const UA_Byte *)ptr2;
    volatile UA_Byte c = 0;
    for(size_t i = 0; i < length; ++i) {
        UA_Byte x = a[i], y = b[i];
        c |= x ^ y;
    }
    return !c;
}

_UA_END_DECLS

#endif /* UA_HELPER_H_ */
