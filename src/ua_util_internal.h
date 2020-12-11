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
 */

#ifndef UA_UTIL_H_
#define UA_UTIL_H_

#define UA_INTERNAL
#include <open62541/types.h>
#include <open62541/util.h>

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
    } while(0);

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

_UA_END_DECLS

#endif /* UA_UTIL_H_ */
