/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#ifndef UA_SERVER_INTERNAL_H_
#define UA_SERVER_INTERNAL_H_

#include <open62541/server.h>
#include <open62541/plugin/nodestore.h>

#include "ua_connection_internal.h"
#include "ua_session.h"
#include "ua_server_async.h"
#include "ua_timer.h"
#include "ua_util_internal.h"

_UA_BEGIN_DECLS

#if UA_MULTITHREADING >= 100
#undef UA_THREADSAFE
#define UA_THREADSAFE UA_DEPRECATED
#endif

#ifdef UA_ENABLE_PUBSUB
#include "ua_pubsub_manager.h"
#endif

#ifdef UA_ENABLE_DISCOVERY
#include "ua_discovery_manager.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"

typedef struct {
    UA_MonitoredItem monitoredItem;
    void *context;
    union {
        UA_Server_DataChangeNotificationCallback dataChangeCallback;
        /* UA_Server_EventNotificationCallback eventCallback; */
    } callback;
} UA_LocalMonitoredItem;

#endif

typedef enum {
    UA_DIAGNOSTICEVENT_CLOSE,
    UA_DIAGNOSTICEVENT_REJECT,
    UA_DIAGNOSTICEVENT_SECURITYREJECT,
    UA_DIAGNOSTICEVENT_TIMEOUT,
    UA_DIAGNOSTICEVENT_ABORT,
    UA_DIAGNOSTICEVENT_PURGE
} UA_DiagnosticEvent;

typedef struct channel_entry {
    UA_TimerEntry cleanupCallback;
    TAILQ_ENTRY(channel_entry) pointers;
    UA_SecureChannel channel;
} channel_entry;

typedef struct session_list_entry {
    UA_TimerEntry cleanupCallback;
    LIST_ENTRY(session_list_entry) pointers;
    UA_Session session;
} session_list_entry;

typedef enum {
    UA_SERVERLIFECYCLE_FRESH,
    UA_SERVERLIFECYLE_RUNNING
} UA_ServerLifecycle;

struct UA_Server {
    /* Config */
    UA_ServerConfig config;
    UA_DateTime startTime;
    UA_DateTime endTime; /* Zeroed out. If a time is set, then the server shuts
                          * down once the time has been reached */

    UA_ServerLifecycle state;

    /* SecureChannels */
    TAILQ_HEAD(, channel_entry) channels;
    UA_UInt32 lastChannelId;
    UA_UInt32 lastTokenId;

#if UA_MULTITHREADING >= 100
    UA_AsyncManager asyncManager;
#endif

    /* Session Management */
    LIST_HEAD(session_list, session_list_entry) sessions;
    UA_UInt32 sessionCount;
    UA_Session adminSession; /* Local access to the services (for startup and
                              * maintenance) uses this Session with all possible
                              * access rights (Session Id: 1) */

    /* Namespaces */
    size_t namespacesSize;
    UA_String *namespaces;

    /* Callbacks with a repetition interval */
    UA_Timer timer;

    /* For bootstrapping, omit some consistency checks, creating a reference to
     * the parent and member instantiation */
    UA_Boolean bootstrapNS0;

    /* Discovery */
#ifdef UA_ENABLE_DISCOVERY
    UA_DiscoveryManager discoveryManager;
#endif

    /* Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    size_t subscriptionsSize;  /* Number of active subscriptions */
    size_t monitoredItemsSize; /* Number of active monitored items */
    LIST_HEAD(, UA_Subscription) subscriptions; /* All subscriptions in the
                                                 * server. They may be detached
                                                 * from a session. */
    UA_UInt32 lastSubscriptionId; /* To generate unique SubscriptionIds */

    /* To be cast to UA_LocalMonitoredItem to get the callback and context */
    LIST_HEAD(, UA_MonitoredItem) localMonitoredItems;
    UA_UInt32 lastLocalMonitoredItemId;

# ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    LIST_HEAD(, UA_ConditionSource) headConditionSource;
# endif

#endif

    /* Publish/Subscribe */
#ifdef UA_ENABLE_PUBSUB
    UA_PubSubManager pubSubManager;
#endif

#if UA_MULTITHREADING >= 100
    UA_LOCK_TYPE(networkMutex)
    UA_LOCK_TYPE(serviceMutex)
#endif

    /* Statistics */
    UA_ServerStatistics serverStats;
};


extern const struct aa_head nameTreeHead;

/**************************/
/* SecureChannel Handling */
/**************************/

/* Remove a all securechannels */
void
UA_Server_deleteSecureChannels(UA_Server *server);

/* Remove timed out securechannels with a delayed callback. So all currently
 * scheduled jobs with a pointer to a securechannel can finish first. */
void
UA_Server_cleanupTimedOutSecureChannels(UA_Server *server, UA_DateTime nowMonotonic);

UA_StatusCode
UA_Server_createSecureChannel(UA_Server *server, UA_Connection *connection);

UA_StatusCode
UA_Server_configSecureChannel(void *application, UA_SecureChannel *channel,
                              const UA_AsymmetricAlgorithmSecurityHeader *asymHeader);

UA_StatusCode
sendServiceFault(UA_SecureChannel *channel, UA_UInt32 requestId, UA_UInt32 requestHandle,
                 const UA_DataType *responseType, UA_StatusCode statusCode);

void
UA_Server_closeSecureChannel(UA_Server *server, UA_SecureChannel *channel,
                             UA_DiagnosticEvent event);

/********************/
/* Session Handling */
/********************/

UA_StatusCode
getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                   size_t *foundIndex);

UA_StatusCode
getBoundSession(UA_Server *server, const UA_SecureChannel *channel,
                const UA_NodeId *token, UA_Session **session);

UA_StatusCode
UA_Server_createSession(UA_Server *server, UA_SecureChannel *channel,
                        const UA_CreateSessionRequest *request, UA_Session **session);

void
UA_Server_removeSession(UA_Server *server, session_list_entry *sentry,
                        UA_DiagnosticEvent event);

UA_StatusCode
UA_Server_removeSessionByToken(UA_Server *server, const UA_NodeId *token,
                               UA_DiagnosticEvent event);

void
UA_Server_cleanupSessions(UA_Server *server, UA_DateTime nowMonotonic);

UA_Session *
getSessionByToken(UA_Server *server, const UA_NodeId *token);

UA_Session *
UA_Server_getSessionById(UA_Server *server, const UA_NodeId *sessionId);

/*****************/
/* Node Handling */
/*****************/

/* Calls the callback with the node retrieved from the nodestore on top of the
 * stack. Either a copy or the original node for in-situ editing. Depends on
 * multithreading and the nodestore.*/
typedef UA_StatusCode (*UA_EditNodeCallback)(UA_Server*, UA_Session*,
                                             UA_Node *node, void*);
UA_StatusCode UA_Server_editNode(UA_Server *server, UA_Session *session,
                                 const UA_NodeId *nodeId,
                                 UA_EditNodeCallback callback,
                                 void *data);

/*********************/
/* Utility Functions */
/*********************/

void setupNs1Uri(UA_Server *server);
UA_UInt16 addNamespace(UA_Server *server, const UA_String name);

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_NodeHead *head);

/* Recursively searches "upwards" in the tree following specific reference types */
UA_Boolean
isNodeInTree(UA_Server *server, const UA_NodeId *leafNode,
             const UA_NodeId *nodeToFind, const UA_ReferenceTypeSet *relevantRefs);

/* Convenience function with just a single ReferenceTypeIndex */
UA_Boolean
isNodeInTree_singleRef(UA_Server *server, const UA_NodeId *leafNode,
                       const UA_NodeId *nodeToFind, const UA_Byte relevantRefTypeIndex);

/* Returns an array with the hierarchy of nodes. The start nodes can be returned
 * as well. The returned array starts at the leaf and continues "upwards" or
 * "downwards". Duplicate entries are removed. */
UA_StatusCode
browseRecursive(UA_Server *server, size_t startNodesSize, const UA_NodeId *startNodes,
                UA_BrowseDirection browseDirection, const UA_ReferenceTypeSet *refTypes,
                UA_UInt32 nodeClassMask, UA_Boolean includeStartNodes,
                size_t *resultsSize, UA_ExpandedNodeId **results);

/* Get the bitfield indices of a ReferenceType and possibly its subtypes.
 * refType must point to a ReferenceTypeNode. */
UA_StatusCode
referenceTypeIndices(UA_Server *server, const UA_NodeId *refType,
                     UA_ReferenceTypeSet *indices, UA_Boolean includeSubtypes);

/* Returns the recursive type and interface hierarchy of the node */ 
UA_StatusCode
getParentTypeAndInterfaceHierarchy(UA_Server *server, const UA_NodeId *typeNode,
                                   UA_NodeId **typeHierarchy, size_t *typeHierarchySize);

/* Returns the recursive interface hierarchy of the node */
UA_StatusCode
getInterfaceHierarchy(UA_Server *server, const UA_NodeId *objectNode,
                                   UA_NodeId **typeHierarchy, size_t *typeHierarchySize);

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

UA_StatusCode
UA_getConditionId(UA_Server *server, const UA_NodeId *conditionNodeId,
                  UA_NodeId *outConditionId);

void
UA_ConditionList_delete(UA_Server *server);

UA_Boolean
isConditionOrBranch(UA_Server *server,
                    const UA_NodeId *condition,
                    const UA_NodeId *conditionSource,
                    UA_Boolean *isCallerAC);

#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */

/* Returns the type node from the node on the stack top. The type node is pushed
 * on the stack and returned. */
const UA_Node *
getNodeType(UA_Server *server, const UA_NodeHead *nodeHead);

UA_StatusCode
sendResponse(UA_Server *server, UA_Session *session, UA_SecureChannel *channel,
             UA_UInt32 requestId, UA_Response *response, const UA_DataType *responseType);

/* Many services come as an array of operations. This function generalizes the
 * processing of the operations. */
typedef void (*UA_ServiceOperation)(UA_Server *server, UA_Session *session,
                                    const void *context,
                                    const void *requestOperation,
                                    void *responseOperation);

UA_StatusCode
UA_Server_processServiceOperations(UA_Server *server, UA_Session *session,
                                   UA_ServiceOperation operationCallback,
                                   const void *context,
                                   const size_t *requestOperations,
                                   const UA_DataType *requestOperationsType,
                                   size_t *responseOperations,
                                   const UA_DataType *responseOperationsType)
    UA_FUNC_ATTR_WARN_UNUSED_RESULT;

/******************************************/
/* Internal function calls, without locks */
/******************************************/
UA_StatusCode
deleteNode(UA_Server *server, const UA_NodeId nodeId,
           UA_Boolean deleteReferences);

UA_StatusCode
addNode(UA_Server *server, const UA_NodeClass nodeClass, const UA_NodeId *requestedNewNodeId,
        const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
        const UA_QualifiedName browseName, const UA_NodeId *typeDefinition,
        const UA_NodeAttributes *attr, const UA_DataType *attributeType,
        void *nodeContext, UA_NodeId *outNewNodeId);

UA_StatusCode
setVariableNode_dataSource(UA_Server *server, const UA_NodeId nodeId,
                           const UA_DataSource dataSource);

UA_StatusCode
setMethodNode_callback(UA_Server *server,
                       const UA_NodeId methodNodeId,
                       UA_MethodCallback methodCallback);

UA_StatusCode
writeAttribute(UA_Server *server, UA_Session *session,
               const UA_NodeId *nodeId, const UA_AttributeId attributeId,
               const void *attr, const UA_DataType *attr_type);

static UA_INLINE UA_StatusCode
writeValueAttribute(UA_Server *server, UA_Session *session,
                    const UA_NodeId *nodeId, const UA_Variant *value) {
    return writeAttribute(server, session, nodeId, UA_ATTRIBUTEID_VALUE,
                          value, &UA_TYPES[UA_TYPES_VARIANT]);
}

UA_DataValue
readAttribute(UA_Server *server, const UA_ReadValueId *item,
              UA_TimestampsToReturn timestamps);

UA_StatusCode
readWithReadValue(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId, void *v);

UA_StatusCode
readObjectProperty(UA_Server *server, const UA_NodeId objectId,
                   const UA_QualifiedName propertyName,
                   UA_Variant *value);

UA_BrowsePathResult
translateBrowsePathToNodeIds(UA_Server *server, const UA_BrowsePath *browsePath);

#ifdef UA_ENABLE_SUBSCRIPTIONS

void monitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem);

UA_Subscription *
UA_Server_getSubscriptionById(UA_Server *server, UA_UInt32 subscriptionId);

#endif /* UA_ENABLE_SUBSCRIPTIONS */

UA_BrowsePathResult
browseSimplifiedBrowsePath(UA_Server *server, const UA_NodeId origin,
                           size_t browsePathSize, const UA_QualifiedName *browsePath);

UA_StatusCode
writeObjectProperty(UA_Server *server, const UA_NodeId objectId,
                    const UA_QualifiedName propertyName, const UA_Variant value);

UA_StatusCode
getNodeContext(UA_Server *server, UA_NodeId nodeId, void **nodeContext);

void
removeCallback(UA_Server *server, UA_UInt64 callbackId);

UA_StatusCode
changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId, UA_Double interval_ms);

UA_StatusCode
addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                    void *data, UA_Double interval_ms, UA_UInt64 *callbackId);

#ifdef UA_ENABLE_DISCOVERY
UA_StatusCode
register_server_with_discovery_server(UA_Server *server,
                                      void *client,
                                      const UA_Boolean isUnregister,
                                      const char* semaphoreFilePath);
#endif
/***************************************/
/* Check Information Model Consistency */
/***************************************/

/* Read a node attribute in the context of a "checked-out" node. So the
 * attribute will not be copied when possible. The variant then points into the
 * node and has UA_VARIANT_DATA_NODELETE set. */
void
ReadWithNode(const UA_Node *node, UA_Server *server, UA_Session *session,
             UA_TimestampsToReturn timestampsToReturn,
             const UA_ReadValueId *id, UA_DataValue *v);

UA_StatusCode
readValueAttribute(UA_Server *server, UA_Session *session,
                   const UA_VariableNode *vn, UA_DataValue *v);

/* Test whether the value matches a variable definition given by
 * - datatype
 * - valueranke
 * - array dimensions.
 * Sometimes it can be necessary to transform the content of the value, e.g.
 * byte array to bytestring or uint32 to some enum. If editableValue is non-NULL,
 * we try to create a matching variant that points to the original data. */
UA_Boolean
compatibleValue(UA_Server *server, UA_Session *session, const UA_NodeId *targetDataTypeId,
                UA_Int32 targetValueRank, size_t targetArrayDimensionsSize,
                const UA_UInt32 *targetArrayDimensions, const UA_Variant *value,
                const UA_NumericRange *range);

/* Is the DataType compatible */
UA_Boolean
compatibleDataTypes(UA_Server *server, const UA_NodeId *dataType,
                    const UA_NodeId *constraintDataType);

/* Is the Value compatible with the DataType? Can perform additional checks
 * compared to compatibleDataTypes. */
UA_Boolean
compatibleValueDataType(UA_Server *server, const UA_DataType *dataType,
                        const UA_NodeId *constraintDataType);


UA_Boolean
compatibleArrayDimensions(size_t constraintArrayDimensionsSize,
                          const UA_UInt32 *constraintArrayDimensions,
                          size_t testArrayDimensionsSize,
                          const UA_UInt32 *testArrayDimensions);

UA_Boolean
compatibleValueArrayDimensions(const UA_Variant *value, size_t targetArrayDimensionsSize,
                               const UA_UInt32 *targetArrayDimensions);

UA_Boolean
compatibleValueRankArrayDimensions(UA_Server *server, UA_Session *session,
                                   UA_Int32 valueRank, size_t arrayDimensionsSize);

UA_Boolean
compatibleValueRanks(UA_Int32 valueRank, UA_Int32 constraintValueRank);

struct BrowseOpts {
    UA_UInt32 maxReferences;
    UA_Boolean recursive;
};

void
Operation_Browse(UA_Server *server, UA_Session *session, const UA_UInt32 *maxrefs,
                 const UA_BrowseDescription *descr, UA_BrowseResult *result);

UA_DataValue
UA_Server_readWithSession(UA_Server *server, UA_Session *session,
                          const UA_ReadValueId *item,
                          UA_TimestampsToReturn timestampsToReturn);

/*****************************/
/* AddNodes Begin and Finish */
/*****************************/

/* Creates a new node in the nodestore. */
UA_StatusCode
AddNode_raw(UA_Server *server, UA_Session *session, void *nodeContext,
            const UA_AddNodesItem *item, UA_NodeId *outNewNodeId);

/* Check the reference to the parent node; Add references. */
UA_StatusCode
AddNode_addRefs(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                const UA_NodeId *typeDefinitionId);

/* Type-check type-definition; Run the constructors */
UA_StatusCode
AddNode_finish(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId);

/**********************/
/* Create Namespace 0 */
/**********************/

UA_StatusCode UA_Server_initNS0(UA_Server *server);

UA_StatusCode writeNs0VariableArray(UA_Server *server, UA_UInt32 id, void *v,
                      size_t length, const UA_DataType *type);

/***************************/
/* Nodestore Access Macros */
/***************************/

#define UA_NODESTORE_NEW(server, nodeClass)                             \
    server->config.nodestore.newNode(server->config.nodestore.context, nodeClass)

#define UA_NODESTORE_DELETE(server, node)                               \
    server->config.nodestore.deleteNode(server->config.nodestore.context, node)

#define UA_NODESTORE_GET(server, nodeid)                                \
    server->config.nodestore.getNode(server->config.nodestore.context, nodeid)

#define UA_NODESTORE_RELEASE(server, node)                              \
    server->config.nodestore.releaseNode(server->config.nodestore.context, node)

#define UA_NODESTORE_GETCOPY(server, nodeid, outnode)                      \
    server->config.nodestore.getNodeCopy(server->config.nodestore.context, \
                                         nodeid, outnode)

#define UA_NODESTORE_INSERT(server, node, addedNodeId)                    \
    server->config.nodestore.insertNode(server->config.nodestore.context, \
                                        node, addedNodeId)

#define UA_NODESTORE_REPLACE(server, node)                              \
    server->config.nodestore.replaceNode(server->config.nodestore.context, node)

#define UA_NODESTORE_REMOVE(server, nodeId)                             \
    server->config.nodestore.removeNode(server->config.nodestore.context, nodeId)

#define UA_NODESTORE_GETREFERENCETYPEID(server, index)                  \
    server->config.nodestore.getReferenceTypeId(server->config.nodestore.context, \
                                                index)

_UA_END_DECLS

#endif /* UA_SERVER_INTERNAL_H_ */
