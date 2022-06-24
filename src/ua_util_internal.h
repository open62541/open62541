/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) LEvertz
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#define UA_INTERNAL
#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/statuscodes.h>

#include "ua_types_encoding_binary.h"

_UA_BEGIN_DECLS

/* Macro-Expand for MSVC workarounds */
#define UA_MACRO_EXPAND(x) x

/* Print a NodeId in logs */
#define UA_LOG_NODEID_INTERNAL(NODEID, LOG)          \
    do {                                             \
    UA_String nodeIdStr = UA_STRING_NULL;            \
    UA_NodeId_print(NODEID, &nodeIdStr);             \
    LOG;                                             \
    UA_String_clear(&nodeIdStr);                     \
    } while(0)

#if UA_LOGLEVEL <= 100
# define UA_LOG_NODEID_TRACE(NODEID, LOG)       \
    UA_LOG_NODEID_INTERNAL(NODEID, LOG)
#else
# define UA_LOG_NODEID_TRACE(NODEID, LOG)
#endif

#if UA_LOGLEVEL <= 200
# define UA_LOG_NODEID_DEBUG(NODEID, LOG)       \
    UA_LOG_NODEID_INTERNAL(NODEID, LOG)
#else
# define UA_LOG_NODEID_DEBUG(NODEID, LOG)
#endif

#if UA_LOGLEVEL <= 300
# define UA_LOG_NODEID_INFO(NODEID, LOG)       \
    UA_LOG_NODEID_INTERNAL(NODEID, LOG)
#else
# define UA_LOG_NODEID_INFO(NODEID, LOG)
#endif

#if UA_LOGLEVEL <= 400
# define UA_LOG_NODEID_WARNING(NODEID, LOG)     \
    UA_LOG_NODEID_INTERNAL(NODEID, LOG)
#else
# define UA_LOG_NODEID_WARNING(NODEID, LOG)
#endif

#if UA_LOGLEVEL <= 500
# define UA_LOG_NODEID_ERROR(NODEID, LOG)       \
    UA_LOG_NODEID_INTERNAL(NODEID, LOG)
#else
# define UA_LOG_NODEID_ERROR(NODEID, LOG)
#endif

#if UA_LOGLEVEL <= 600
# define UA_LOG_NODEID_FATAL(NODEID, LOG)       \
    UA_LOG_NODEID_INTERNAL(NODEID, LOG)
#else
# define UA_LOG_NODEID_FATAL(NODEID, LOG)
#endif

/* Short names for integer. These are not exposed on the public API, since many
 * user-applications make the same definitions in their headers. */
typedef UA_Byte u8;
typedef UA_SByte i8;
typedef UA_UInt16 u16;
typedef UA_Int16 i16;
typedef UA_UInt32 u32;
typedef UA_Int32 i32;
typedef UA_UInt64 u64;
typedef UA_Int64 i64;
typedef UA_StatusCode status;

/**
 * Error checking macros
 */

static UA_INLINE UA_Boolean
isGood(UA_StatusCode code) {
    return code == UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_Boolean
isNonNull(const void *ptr) {
    return ptr != NULL;
}

static UA_INLINE UA_Boolean
isTrue(uint8_t expr) {
    return expr;
}

#define UA_CHECK(A, EVAL_ON_ERROR)                                                       \
    do {                                                                                 \
        if(UA_UNLIKELY(!isTrue(A))) {                                                    \
            EVAL_ON_ERROR;                                                               \
        }                                                                                \
    } while(0)

#define UA_CHECK_STATUS(STATUSCODE, EVAL_ON_ERROR)                                       \
    UA_CHECK(isGood(STATUSCODE), EVAL_ON_ERROR)

#define UA_CHECK_MEM(STATUSCODE, EVAL_ON_ERROR)                                       \
    UA_CHECK(isNonNull(STATUSCODE), EVAL_ON_ERROR)

#ifdef UA_DEBUG_FILE_LINE_INFO
#define UA_CHECK_LOG_INTERNAL(A, STATUSCODE, EVAL, LOG, LOGGER, CAT, MSG, ...)           \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK(A, LOG(LOGGER, CAT, "" MSG "%s (%s:%d: statuscode: %s)", __VA_ARGS__,   \
                        __FILE__, __LINE__, UA_StatusCode_name(STATUSCODE));             \
                 EVAL))
#else
#define UA_CHECK_LOG_INTERNAL(A, STATUSCODE, EVAL, LOG, LOGGER, CAT, MSG, ...)           \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK(A, LOG(LOGGER, CAT, "" MSG "%s (statuscode: %s)", __VA_ARGS__,   \
                        UA_StatusCode_name(STATUSCODE));             \
                 EVAL))
#endif

#define UA_CHECK_LOG(A, EVAL, LEVEL, LOGGER, CAT, ...)                                   \
    UA_MACRO_EXPAND(UA_CHECK_LOG_INTERNAL(A, UA_STATUSCODE_BAD, EVAL, UA_LOG_##LEVEL,    \
                                          LOGGER, CAT, __VA_ARGS__, ""))

#define UA_CHECK_STATUS_LOG(STATUSCODE, EVAL, LEVEL, LOGGER, CAT, ...)                   \
    UA_MACRO_EXPAND(UA_CHECK_LOG_INTERNAL(isGood(STATUSCODE), STATUSCODE,  \
                                          EVAL, UA_LOG_##LEVEL, LOGGER, CAT,             \
                                          __VA_ARGS__, ""))

#define UA_CHECK_MEM_LOG(PTR, EVAL, LEVEL, LOGGER, CAT, ...)                   \
    UA_MACRO_EXPAND(UA_CHECK_LOG_INTERNAL(isNonNull(PTR), UA_STATUSCODE_BADOUTOFMEMORY,  \
                                          EVAL, UA_LOG_##LEVEL, LOGGER, CAT,             \
                                          __VA_ARGS__, ""))

/**
 * Check Macros
 * Usage examples:
 *
 *    void *data = malloc(...);
 *    UA_CHECK(data, return error);
 *
 *    UA_StatusCode rv = some_func(...);
 *    UA_CHECK_STATUS(rv, return rv);
 *
 *    UA_Logger *logger = &server->config.logger;
 *    rv = bar_func(...);
 *    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SERVER, "msg & args %s", "arg");
 */
#define UA_CHECK_FATAL(A, EVAL, LOGGER, CAT, ...)                                        \
    UA_MACRO_EXPAND(UA_CHECK_LOG(A, EVAL, FATAL, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_ERROR(A, EVAL, LOGGER, CAT, ...)                                        \
    UA_MACRO_EXPAND(UA_CHECK_LOG(A, EVAL, ERROR, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_WARN(A, EVAL, LOGGER, CAT, ...)                                         \
    UA_MACRO_EXPAND(UA_CHECK_LOG(A, EVAL, WARNING, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_INFO(A, EVAL, LOGGER, CAT, ...)                                         \
    UA_MACRO_EXPAND(UA_CHECK_LOG(A, EVAL, INFO, LOGGER, CAT, __VA_ARGS__))

#define UA_CHECK_STATUS_FATAL(STATUSCODE, EVAL, LOGGER, CAT, ...)                        \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_STATUS_LOG(STATUSCODE, EVAL, FATAL, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_STATUS_ERROR(STATUSCODE, EVAL, LOGGER, CAT, ...)                        \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_STATUS_LOG(STATUSCODE, EVAL, ERROR, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_STATUS_WARN(STATUSCODE, EVAL, LOGGER, CAT, ...)                         \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_STATUS_LOG(STATUSCODE, EVAL, WARNING, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_STATUS_INFO(STATUSCODE, EVAL, LOGGER, CAT, ...)                         \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_STATUS_LOG(STATUSCODE, EVAL, INFO, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_STATUS_DEBUG(STATUSCODE, EVAL, LOGGER, CAT, ...)                         \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_STATUS_LOG(STATUSCODE, EVAL, DEBUG, LOGGER, CAT, __VA_ARGS__))

#define UA_CHECK_MEM_FATAL(PTR, EVAL, LOGGER, CAT, ...)                        \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_MEM_LOG(PTR, EVAL, FATAL, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_MEM_ERROR(PTR, EVAL, LOGGER, CAT, ...)                        \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_MEM_LOG(PTR, EVAL, ERROR, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_MEM_WARN(PTR, EVAL, LOGGER, CAT, ...)                         \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_MEM_LOG(PTR, EVAL, WARNING, LOGGER, CAT, __VA_ARGS__))
#define UA_CHECK_MEM_INFO(PTR, EVAL, LOGGER, CAT, ...)                         \
    UA_MACRO_EXPAND(                                                                     \
        UA_CHECK_MEM_LOG(PTR, EVAL, INFO, LOGGER, CAT, __VA_ARGS__))

/**
 * Utility Functions
 * ----------------- */

const UA_DataType *
UA_findDataTypeWithCustom(const UA_NodeId *typeId,
                          const UA_DataTypeArray *customTypes);

/* Get the number of optional fields contained in an structure type */
size_t UA_EXPORT
getCountOfOptionalFields(const UA_DataType *type);

/* Dump packet for debugging / fuzzing */
#ifdef UA_DEBUG_DUMP_PKGS
void UA_EXPORT
UA_dump_hex_pkg(UA_Byte* buffer, size_t bufferLen);
#endif

/* Chunked for loop */
#define FOR_EACH_CHUNK(CURSOR, SIZE, chunkSize, arraySize) \
        for((CURSOR) = 0, (SIZE) = (arraySize) <= (chunkSize) ? (arraySize) : (chunkSize); \
            (CURSOR) < (arraySize); \
            (CURSOR) = (CURSOR) + (chunkSize), (SIZE) = (arraySize) - (CURSOR) <= (chunkSize) ? (arraySize) - (CURSOR) : (chunkSize))

/* Unions that represent any of the supported request or response message */
typedef union {
    UA_RequestHeader requestHeader;
    UA_FindServersRequest findServersRequest;
    UA_GetEndpointsRequest getEndpointsRequest;
#ifdef UA_ENABLE_DISCOVERY
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_FindServersOnNetworkRequest findServersOnNetworkRequest;
# endif
    UA_RegisterServerRequest registerServerRequest;
    UA_RegisterServer2Request registerServer2Request;
#endif
    UA_OpenSecureChannelRequest openSecureChannelRequest;
    UA_CreateSessionRequest createSessionRequest;
    UA_ActivateSessionRequest activateSessionRequest;
    UA_CloseSessionRequest closeSessionRequest;
    UA_AddNodesRequest addNodesRequest;
    UA_AddReferencesRequest addReferencesRequest;
    UA_DeleteNodesRequest deleteNodesRequest;
    UA_DeleteReferencesRequest deleteReferencesRequest;
    UA_BrowseRequest browseRequest;
    UA_BrowseNextRequest browseNextRequest;
    UA_TranslateBrowsePathsToNodeIdsRequest translateBrowsePathsToNodeIdsRequest;
    UA_RegisterNodesRequest registerNodesRequest;
    UA_UnregisterNodesRequest unregisterNodesRequest;
    UA_ReadRequest readRequest;
    UA_WriteRequest writeRequest;
#ifdef UA_ENABLE_HISTORIZING
    UA_HistoryReadRequest historyReadRequest;
    UA_HistoryUpdateRequest historyUpdateRequest;
#endif
#ifdef UA_ENABLE_METHODCALLS
    UA_CallRequest callRequest;
#endif
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_CreateMonitoredItemsRequest createMonitoredItemsRequest;
    UA_DeleteMonitoredItemsRequest deleteMonitoredItemsRequest;
    UA_ModifyMonitoredItemsRequest modifyMonitoredItemsRequest;
    UA_SetMonitoringModeRequest setMonitoringModeRequest;
    UA_CreateSubscriptionRequest createSubscriptionRequest;
    UA_ModifySubscriptionRequest modifySubscriptionRequest;
    UA_SetPublishingModeRequest setPublishingModeRequest;
    UA_PublishRequest publishRequest;
    UA_RepublishRequest republishRequest;
    UA_DeleteSubscriptionsRequest deleteSubscriptionsRequest;
#endif
} UA_Request;

typedef union {
    UA_ResponseHeader responseHeader;
    UA_FindServersResponse findServersResponse;
    UA_GetEndpointsResponse getEndpointsResponse;
#ifdef UA_ENABLE_DISCOVERY
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_FindServersOnNetworkResponse findServersOnNetworkResponse;
# endif
    UA_RegisterServerResponse registerServerResponse;
    UA_RegisterServer2Response registerServer2Response;
#endif
    UA_OpenSecureChannelResponse openSecureChannelResponse;
    UA_CreateSessionResponse createSessionResponse;
    UA_ActivateSessionResponse activateSessionResponse;
    UA_CloseSessionResponse closeSessionResponse;
    UA_AddNodesResponse addNodesResponse;
    UA_AddReferencesResponse addReferencesResponse;
    UA_DeleteNodesResponse deleteNodesResponse;
    UA_DeleteReferencesResponse deleteReferencesResponse;
    UA_BrowseResponse browseResponse;
    UA_BrowseNextResponse browseNextResponse;
    UA_TranslateBrowsePathsToNodeIdsResponse translateBrowsePathsToNodeIdsResponse;
    UA_RegisterNodesResponse registerNodesResponse;
    UA_UnregisterNodesResponse unregisterNodesResponse;
    UA_ReadResponse readResponse;
    UA_WriteResponse writeResponse;
#ifdef UA_ENABLE_HISTORIZING
    UA_HistoryReadResponse historyReadResponse;
    UA_HistoryUpdateResponse historyUpdateResponse;
#endif
#ifdef UA_ENABLE_METHODCALLS
    UA_CallResponse callResponse;
#endif
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_CreateMonitoredItemsResponse createMonitoredItemsResponse;
    UA_DeleteMonitoredItemsResponse deleteMonitoredItemsResponse;
    UA_ModifyMonitoredItemsResponse modifyMonitoredItemsResponse;
    UA_SetMonitoringModeResponse setMonitoringModeResponse;
    UA_CreateSubscriptionResponse createSubscriptionResponse;
    UA_ModifySubscriptionResponse modifySubscriptionResponse;
    UA_SetPublishingModeResponse setPublishingModeResponse;
    UA_PublishResponse publishResponse;
    UA_RepublishResponse republishResponse;
    UA_DeleteSubscriptionsResponse deleteSubscriptionsResponse;
#endif
} UA_Response;

/* Do not expose UA_String_equal_ignorecase to public API as it currently only handles
 * ASCII strings, and not UTF8! */
UA_Boolean UA_EXPORT
UA_String_equal_ignorecase(const UA_String *s1, const UA_String *s2);

/********************/
/* Encoding Helpers */
/********************/

#define UA_ENCODING_HELPERS(TYPE, UPCASE_TYPE)                          \
    static UA_INLINE size_t                                             \
    UA_##TYPE##_calcSizeBinary(const UA_##TYPE *src) {                    \
        return UA_calcSizeBinary(src, &UA_TYPES[UA_TYPES_##UPCASE_TYPE]); \
    }                                                                   \
    static UA_INLINE UA_StatusCode                                      \
    UA_##TYPE##_encodeBinary(const UA_##TYPE *src, UA_Byte **bufPos, const UA_Byte *bufEnd) { \
        return UA_encodeBinaryInternal(src, &UA_TYPES[UA_TYPES_##UPCASE_TYPE], \
                                       bufPos, &bufEnd, NULL, NULL);    \
    }                                                                   \
    static UA_INLINE UA_StatusCode                                      \
    UA_##TYPE##_decodeBinary(const UA_ByteString *src, size_t *offset, UA_##TYPE *dst) { \
    return UA_decodeBinaryInternal(src, offset, dst, \
                                   &UA_TYPES[UA_TYPES_##UPCASE_TYPE], NULL); \
    }

UA_ENCODING_HELPERS(Boolean, BOOLEAN)
UA_ENCODING_HELPERS(SByte, SBYTE)
UA_ENCODING_HELPERS(Byte, BYTE)
UA_ENCODING_HELPERS(Int16, INT16)
UA_ENCODING_HELPERS(UInt16, UINT16)
UA_ENCODING_HELPERS(Int32, INT32)
UA_ENCODING_HELPERS(UInt32, UINT32)
UA_ENCODING_HELPERS(Int64, INT64)
UA_ENCODING_HELPERS(UInt64, UINT64)
UA_ENCODING_HELPERS(Float, FLOAT)
UA_ENCODING_HELPERS(Double, DOUBLE)
UA_ENCODING_HELPERS(String, STRING)
UA_ENCODING_HELPERS(DateTime, DATETIME)
UA_ENCODING_HELPERS(Guid, GUID)
UA_ENCODING_HELPERS(ByteString, BYTESTRING)
UA_ENCODING_HELPERS(XmlElement, XMLELEMENT)
UA_ENCODING_HELPERS(NodeId, NODEID)
UA_ENCODING_HELPERS(ExpandedNodeId, EXPANDEDNODEID)
UA_ENCODING_HELPERS(StatusCode, STATUSCODE)
UA_ENCODING_HELPERS(QualifiedName, QUALIFIEDNAME)
UA_ENCODING_HELPERS(LocalizedText, LOCALIZEDTEXT)
UA_ENCODING_HELPERS(ExtensionObject, EXTENSIONOBJECT)
UA_ENCODING_HELPERS(DataValue, DATAVALUE)
UA_ENCODING_HELPERS(Variant, VARIANT)
UA_ENCODING_HELPERS(DiagnosticInfo, DIAGNOSTICINFO)

_UA_END_DECLS

#endif /* UA_UTIL_H_ */
