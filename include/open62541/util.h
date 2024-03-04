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
 * Range Definition
 * ---------------- */

typedef struct {
    UA_UInt32 min;
    UA_UInt32 max;
} UA_UInt32Range;

typedef struct {
    UA_Duration min;
    UA_Duration max;
} UA_DurationRange;

/**
 * Query Language Eventfilter
 * @param content eventfilter query
 * @param filter generated eventfilter
 */
#ifdef UA_ENABLE_PARSING
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
UA_EXPORT UA_StatusCode
UA_EventFilter_parse(UA_EventFilter *filter, UA_ByteString *content);
#endif
#endif


/**
 * Random Number Generator
 * -----------------------
 * If UA_MULTITHREADING is defined, then the seed is stored in thread
 * local storage. The seed is initialized for every thread in the
 * server/client. */

void UA_EXPORT
UA_random_seed(UA_UInt64 seed);

UA_UInt32 UA_EXPORT
UA_UInt32_random(void); /* no cryptographic entropy */

UA_Guid UA_EXPORT
UA_Guid_random(void);   /* no cryptographic entropy */

/**
 * Key Value Map
 * -------------
 * Helper functions to work with configuration parameters in an array of
 * UA_KeyValuePair. Lookup is linear. So this is for small numbers of keys. The
 * methods below that accept a `const UA_KeyValueMap` as an argument also accept
 * NULL for that argument and treat it as an empty map. */

typedef struct {
    size_t mapSize;
    UA_KeyValuePair *map;
} UA_KeyValueMap;

UA_EXPORT extern const UA_KeyValueMap UA_KEYVALUEMAP_NULL;

UA_EXPORT UA_KeyValueMap *
UA_KeyValueMap_new(void);

UA_EXPORT void
UA_KeyValueMap_clear(UA_KeyValueMap *map);

UA_EXPORT void
UA_KeyValueMap_delete(UA_KeyValueMap *map);

/* Is the map empty (or NULL)? */
UA_EXPORT UA_Boolean
UA_KeyValueMap_isEmpty(const UA_KeyValueMap *map);

/* Does the map contain an entry for the key? */
UA_EXPORT UA_Boolean
UA_KeyValueMap_contains(const UA_KeyValueMap *map, const UA_QualifiedName key);

/* Insert a copy of the value. Can reallocate the underlying array. This
 * invalidates pointers into the previous array. If the key exists already, the
 * value is overwritten (upsert semantics). */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_set(UA_KeyValueMap *map,
                   const UA_QualifiedName key,
                   const UA_Variant *value);

/* Helper function for scalar insertion that internally calls
 * `UA_KeyValueMap_set` */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_setScalar(UA_KeyValueMap *map,
                         const UA_QualifiedName key,
                         void * UA_RESTRICT p,
                         const UA_DataType *type);

/* Returns a pointer to the value or NULL if the key is not found */
UA_EXPORT const UA_Variant *
UA_KeyValueMap_get(const UA_KeyValueMap *map,
                   const UA_QualifiedName key);

/* Returns NULL if the value for the key is not defined, not of the right
 * datatype or not a scalar */
UA_EXPORT const void *
UA_KeyValueMap_getScalar(const UA_KeyValueMap *map,
                         const UA_QualifiedName key,
                         const UA_DataType *type);

/* Remove a single entry. To delete the entire map, use `UA_KeyValueMap_clear`. */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_remove(UA_KeyValueMap *map,
                      const UA_QualifiedName key);

/* Create a deep copy of the given KeyValueMap */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_copy(const UA_KeyValueMap *src, UA_KeyValueMap *dst);

/* Copy entries from the right-hand-side into the left-hand-size. Reallocates
 * previous memory in the left-hand-side. If the operation fails, both maps are
 * left untouched. */
UA_EXPORT UA_StatusCode
UA_KeyValueMap_merge(UA_KeyValueMap *lhs, const UA_KeyValueMap *rhs);

/**
 * Binary Connection Config Parameters
 * ----------------------------------- */

typedef struct {
    UA_UInt32 protocolVersion;
    UA_UInt32 recvBufferSize;
    UA_UInt32 sendBufferSize;
    UA_UInt32 localMaxMessageSize;  /* (0 = unbounded) */
    UA_UInt32 remoteMaxMessageSize; /* (0 = unbounded) */
    UA_UInt32 localMaxChunkCount;   /* (0 = unbounded) */
    UA_UInt32 remoteMaxChunkCount;  /* (0 = unbounded) */
} UA_ConnectionConfig;

/**
 * .. _default-node-attributes:
 *
 * Default Node Attributes
 * -----------------------
 * Default node attributes to simplify the use of the AddNodes services. For
 * example, Setting the ValueRank and AccessLevel to zero is often an unintended
 * setting and leads to errors that are hard to track down. */

/* The default for variables is "BaseDataType" for the datatype, -2 for the
 * valuerank and a read-accesslevel. */
UA_EXPORT extern const UA_VariableAttributes UA_VariableAttributes_default;
UA_EXPORT extern const UA_VariableTypeAttributes UA_VariableTypeAttributes_default;

/* Methods are executable by default */
UA_EXPORT extern const UA_MethodAttributes UA_MethodAttributes_default;

/* The remaining attribute definitions are currently all zeroed out */
UA_EXPORT extern const UA_ObjectAttributes UA_ObjectAttributes_default;
UA_EXPORT extern const UA_ObjectTypeAttributes UA_ObjectTypeAttributes_default;
UA_EXPORT extern const UA_ReferenceTypeAttributes UA_ReferenceTypeAttributes_default;
UA_EXPORT extern const UA_DataTypeAttributes UA_DataTypeAttributes_default;
UA_EXPORT extern const UA_ViewAttributes UA_ViewAttributes_default;

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
 * @param outPath Set to the path if one is present in the endpointUrl. Can be
 *        NULL. Then not path is returned. Starting or trailing '/' are NOT
 *        included in the path. The string points into the original endpointUrl,
 *        so no memory is allocated.
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
 * And-Escaping of Strings
 * -----------------------
 * The "and-escaping" of strings for is described in Part 4, A2. The ``&``
 * character is used to escape the reserved characters ``/.<>:#!&``.
 * So the string ``My.String`` becomes ``My&.String``.
 *
 * In addition to the standard we define "extended-and-escaping" where
 * additionaly commas, semicolons, brackets and whitespace characters are
 * escaped. This improves the parsing in a larger context, as a lexer can find
 * the end of the escaped string. The additionally reserved characters for the
 * extended escaping are ``,()[] \t\n\v\f\r``.
 *
 * This documentation always states whether "and-escaping" or the
 * "extended-and-escaping is used.
 *
 * Print and Parse RelativePath Expressions
 * ----------------------------------------
 * Parse a RelativePath according to the format defined in Part 4, A2. This is
 * used e.g. for the BrowsePath structure.
 *
 *   ``RelativePath := ( ReferenceType BrowseName )+``
 *
 * The ReferenceType has one of the following formats:
 *
 * - ``/``: *HierarchicalReferences* and subtypes
 * - ``.``: *Aggregates* ReferenceTypes and subtypes
 * - ``< [!#]* BrowseName >``: The ReferenceType is indicated by its BrowseName.
 *   Reserved characters in the BrowseName are and-escaped. The following
 *   prefix-modifiers are defined for the ReferenceType.
 *   - ``!`` switches to inverse References
 *   - ``#`` excludes subtypes of the ReferenceType.
 *   - As a non-standard extension we allow the ReferenceType in angle-brackets
 *     as a NodeId. For example ``<ns=1;i=345>``. If a string NodeId is used,
 *     the string identifier is and-escaped.
 *
 * The BrowseName is a QualifiedName. It consist of an optional NamespaceIndex
 * and the name itself. The NamespaceIndex can be left out for the default
 * Namespace zero. The name component is and-escaped (see above).
 *
 *   ``BrowseName := ([0-9]+ ":")? Name``
 *
 * The last BrowseName in a RelativePath can be omitted. This acts as a wildcard
 * that matches any BrowseName.
 *
 * Example RelativePaths
 * `````````````````````
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

/* Supports the lookup of non-standard ReferenceTypes by their browse name in
 * the information model of a server. The first matching result in the
 * ReferenceType hierarchy is used. */
UA_EXPORT UA_StatusCode
UA_RelativePath_parseWithServer(UA_Server *server, UA_RelativePath *rp,
                                const UA_String str);

/* The out-string can be pre-allocated. Then the size is adjusted or an error
 * returned. If the out-string is NULL, then memory is allocated for it. */
UA_EXPORT UA_StatusCode
UA_RelativePath_print(const UA_RelativePath *rp, UA_String *out);
#endif

/**
 * .. _parse-sao:
 *
 * Print and Parse SimpleAttributeOperand Expression
 * -------------------------------------------------
 * The SimpleAttributeOperand is used to specify the location of up values.
 * SimpleAttributeOperands are used for example in EventFilters to select the
 * values reported for each event instance.
 *
 * The TypeDefinitionId is a NodeId and restricts the starting point for the
 * lookup to instances of the TypeDefinitionNode or one of its subtypes. If not
 * defined, the NodeId defaults to the BaseEventType. The NodeId is
 * extended-and-escaped.
 *
 * The BrowsePath is a list of BrowseNames (QualifiedName expression with
 * extended-and-escaping of the name) to be followed from the TypeDefinitionNode
 * instance. The implied ReferenceTypeIds (cf. the RelativePath expressions) are
 * always the HierarchicalReferences and their subtypes. So the ``/`` separator
 * is mandatory here. The BrowsePath for the SimpleAttributeOperand is defined
 * to only follow into Variable- and ObjectNodes. If the BrowsePath is empty,
 * the value is taken from the instance of the TypeDefinition itself.
 *
 * The attribute is the textual name of the selected node attribute.
 * If undefined, the attribute defaults to the Value attribute.
 * For the index range, see the section on :ref:`numericrange`.
 * The BNF definition of the SimpleAttributeOperand is as follows::
 *
 *   SimpleAttributeOperand :=
 *     TypeDefinitionId? SimpleBrowsePath ("#" Attribute)? ("[" IndexRange "]")?
 *
 *   SimpleBrowsePath := ("/" BrowseName)*
 *
 * Example SimpleAttributeOperands
 * ```````````````````````````````
 * - ``ns=2;s=TruckEventType/3:Truck/5:Wheel#Value[1:3]``
 * - ``/3:Truck/5:Wheel``
 * - ``#BrowseName``
 * - Empty String: No NodeId, BrowsePath, Attribute and NumericRange. This
 *   indicates the value attribute of the event instance.
 */

#ifdef UA_ENABLE_PARSING
UA_EXPORT UA_StatusCode
UA_SimpleAttributeOperand_parse(UA_SimpleAttributeOperand *sao,
                                const UA_String str);

/* The out-string can be pre-allocated. Then the size is adjusted or an error
 * returned. If the out-string is NULL, then memory is allocated for it. */
UA_EXPORT UA_StatusCode
UA_SimpleAttributeOperand_print(const UA_SimpleAttributeOperand *sao,
                                UA_String *out);
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
 * Cryptography Helpers
 * -------------------- */

/* Compare memory in constant time to mitigate timing attacks.
 * Returns true if ptr1 and ptr2 are equal for length bytes. */
UA_EXPORT UA_Boolean
UA_constantTimeEqual(const void *ptr1, const void *ptr2, size_t length);

/* Zero-out memory in a way that is not removed by compiler-optimizations. Use
 * this to ensure cryptographic secrets don't leave traces after the memory was
 * freed. */
UA_EXPORT void
UA_ByteString_memZero(UA_ByteString *bs);

_UA_END_DECLS

#endif /* UA_HELPER_H_ */
