/*
 * Copyright (C) 2014-2016 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_CLIENT_HIGHLEVEL_H_
#define UA_CLIENT_HIGHLEVEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_client.h"

/**
 * .. _client-highlevel:
 *
 * Highlevel Client Functionality
 * ------------------------------
 * The following definitions are convenience functions making use of the
 * standard OPC UA services in the background.
 *
 * The high level abstractions concetrate on getting the job done in a simple
 * manner for the user. This is a less flexible way of handling the stack,
 * because at many places sensible defaults are presumed; at the same time using
 * these functions is the easiest way of implementing an OPC UA application, as
 * you will not have to consider all the details that go into the OPC UA
 * services. A concept of how nodes and datatypes are used are completely
 * sufficient to use OPC UA with this layer.
 *
 * If more flexibility is needed, you can always achieve the same functionality
 * using the raw :ref:`OPC UA services <client-services>`.
 *
 * Read Attributes
 * ===============
 * The following functions can be used to retrieve a single node attribute. Use
 * the regular service to read several attributes at once. */
/* Don't call this function, use the typed versions */
UA_StatusCode UA_EXPORT
__UA_Client_readAttribute(UA_Client *client, const UA_NodeId *nodeId, UA_AttributeId attributeId,
                          void *out, const UA_DataType *outDataType);

static UA_INLINE UA_StatusCode
UA_Client_readNodeIdAttribute(UA_Client *client, const UA_NodeId nodeId, UA_NodeId *outNodeId) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_NODEID, outNodeId, &UA_TYPES[UA_TYPES_NODEID]); }

static UA_INLINE UA_StatusCode
UA_Client_readNodeClassAttribute(UA_Client *client, const UA_NodeId nodeId, UA_NodeClass *outNodeClass) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_NODECLASS, outNodeClass, &UA_TYPES[UA_TYPES_NODECLASS]); }

static UA_INLINE UA_StatusCode
UA_Client_readBrowseNameAttribute(UA_Client *client, const UA_NodeId nodeId, UA_QualifiedName *outBrowseName) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_BROWSENAME, outBrowseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]); }

static UA_INLINE UA_StatusCode
UA_Client_readDisplayNameAttribute(UA_Client *client, const UA_NodeId nodeId, UA_LocalizedText *outDisplayName) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME, outDisplayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_readDescriptionAttribute(UA_Client *client, const UA_NodeId nodeId, UA_LocalizedText *outDescription) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_DESCRIPTION, outDescription, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_readWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId, UA_UInt32 *outWriteMask) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_WRITEMASK, outWriteMask, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readUserWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId, UA_UInt32 *outUserWriteMask) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_USERWRITEMASK, outUserWriteMask, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readIsAbstractAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Boolean *outIsAbstract) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_ISABSTRACT, outIsAbstract, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readSymmetricAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Boolean *outSymmetric) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_SYMMETRIC, outSymmetric, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readInverseNameAttribute(UA_Client *client, const UA_NodeId nodeId, UA_LocalizedText *outInverseName) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_INVERSENAME, outInverseName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_readContainsNoLoopsAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Boolean *outContainsNoLoops) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS, outContainsNoLoops, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readEventNotifierAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Byte *outEventNotifier) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER, outEventNotifier, &UA_TYPES[UA_TYPES_BYTE]); }

static UA_INLINE UA_StatusCode
UA_Client_readValueAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Variant *outValue) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_VALUE, outValue, &UA_TYPES[UA_TYPES_VARIANT]); }

static UA_INLINE UA_StatusCode
UA_Client_readDataTypeAttribute(UA_Client *client, const UA_NodeId nodeId, UA_NodeId *outDataType) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_DATATYPE, outDataType, &UA_TYPES[UA_TYPES_NODEID]); }

static UA_INLINE UA_StatusCode
UA_Client_readValueRankAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Int32 *outValueRank) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_VALUERANK, outValueRank, &UA_TYPES[UA_TYPES_INT32]); }

UA_StatusCode UA_EXPORT
UA_Client_readArrayDimensionsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                       UA_Int32 **outArrayDimensions, size_t *outArrayDimensionsSize);

static UA_INLINE UA_StatusCode
UA_Client_readAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId, UA_UInt32 *outAccessLevel) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL, outAccessLevel, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readUserAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId, UA_UInt32 *outUserAccessLevel) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_USERACCESSLEVEL, outUserAccessLevel, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readMinimumSamplingIntervalAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Double *outMinimumSamplingInterval) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, outMinimumSamplingInterval, &UA_TYPES[UA_TYPES_DOUBLE]); }

static UA_INLINE UA_StatusCode
UA_Client_readHistorizingAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Boolean *outHistorizing) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_HISTORIZING, outHistorizing, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readExecutableAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Boolean *outExecutable) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_EXECUTABLE, outExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readUserExecutableAttribute(UA_Client *client, const UA_NodeId nodeId, UA_Boolean *outUserExecutable) {
    return __UA_Client_readAttribute(client, &nodeId, UA_ATTRIBUTEID_USEREXECUTABLE, outUserExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]); }

/**
 * Write Attributes
 * ================
 * The following functions can be use to write a single node attribute at a
 * time. Use the regular write service to write several attributes at once. */
/* Don't call this function, use the typed versions */
UA_StatusCode UA_EXPORT
__UA_Client_writeAttribute(UA_Client *client, const UA_NodeId *nodeId,
                           UA_AttributeId attributeId, const void *in,
                           const UA_DataType *inDataType);

static UA_INLINE UA_StatusCode
UA_Client_writeNodeIdAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_NodeId *newNodeId) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_NODEID, newNodeId, &UA_TYPES[UA_TYPES_NODEID]); }

static UA_INLINE UA_StatusCode
UA_Client_writeNodeClassAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_NodeClass *newNodeClass) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_NODECLASS, newNodeClass, &UA_TYPES[UA_TYPES_NODECLASS]); }

static UA_INLINE UA_StatusCode
UA_Client_writeBrowseNameAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_QualifiedName *newBrowseName) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_BROWSENAME, newBrowseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]); }

static UA_INLINE UA_StatusCode
UA_Client_writeDisplayNameAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_LocalizedText *newDisplayName) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME, newDisplayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_writeDescriptionAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_LocalizedText *newDescription) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_DESCRIPTION, newDescription, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_writeWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_UInt32 *newWriteMask) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_WRITEMASK, newWriteMask, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_writeUserWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_UInt32 *newUserWriteMask) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_USERWRITEMASK, newUserWriteMask, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_writeIsAbstractAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Boolean *newIsAbstract) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_ISABSTRACT, newIsAbstract, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_writeSymmetricAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Boolean *newSymmetric) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_SYMMETRIC, newSymmetric, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_writeInverseNameAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_LocalizedText *newInverseName) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_INVERSENAME, newInverseName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_writeContainsNoLoopsAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Boolean *newContainsNoLoops) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS, newContainsNoLoops, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_writeEventNotifierAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Byte *newEventNotifier) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER, newEventNotifier, &UA_TYPES[UA_TYPES_BYTE]); }

static UA_INLINE UA_StatusCode
UA_Client_writeValueAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Variant *newValue) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_VALUE, newValue, &UA_TYPES[UA_TYPES_VARIANT]); }

static UA_INLINE UA_StatusCode
UA_Client_writeDataTypeAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_NodeId *newDataType) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_DATATYPE, newDataType, &UA_TYPES[UA_TYPES_NODEID]); }

static UA_INLINE UA_StatusCode
UA_Client_writeValueRankAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Int32 *newValueRank) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_VALUERANK, newValueRank, &UA_TYPES[UA_TYPES_INT32]); }

UA_StatusCode UA_EXPORT
UA_Client_writeArrayDimensionsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                        const UA_Int32 *newArrayDimensions, size_t newArrayDimensionsSize);

static UA_INLINE UA_StatusCode
UA_Client_writeAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_UInt32 *newAccessLevel) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL, newAccessLevel, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_writeUserAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_UInt32 *newUserAccessLevel) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_USERACCESSLEVEL, newUserAccessLevel, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_writeMinimumSamplingIntervalAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Double *newMinimumSamplingInterval) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, newMinimumSamplingInterval, &UA_TYPES[UA_TYPES_DOUBLE]); }

static UA_INLINE UA_StatusCode
UA_Client_writeHistorizingAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Boolean *newHistorizing) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_HISTORIZING, newHistorizing, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_writeExecutableAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Boolean *newExecutable) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_EXECUTABLE, newExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_writeUserExecutableAttribute(UA_Client *client, const UA_NodeId nodeId, const UA_Boolean *newUserExecutable) {
    return __UA_Client_writeAttribute(client, &nodeId, UA_ATTRIBUTEID_USEREXECUTABLE, newUserExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]); }

/**
 * Method Calling
 * ============== */
UA_StatusCode UA_EXPORT
UA_Client_call(UA_Client *client, const UA_NodeId objectId, const UA_NodeId methodId,
               size_t inputSize, const UA_Variant *input, size_t *outputSize, UA_Variant **output);

/**
 * Node Management
 * ===============
 *
 * See the section on :ref:`server-side node management <addnodes>`.
 */
UA_StatusCode UA_EXPORT
UA_Client_addReference(UA_Client *client, const UA_NodeId sourceNodeId, const UA_NodeId referenceTypeId,
                       UA_Boolean isForward, const UA_String targetServerUri,
                       const UA_ExpandedNodeId targetNodeId, UA_NodeClass targetNodeClass);

UA_StatusCode UA_EXPORT
UA_Client_deleteReference(UA_Client *client, const UA_NodeId sourceNodeId, const UA_NodeId referenceTypeId,
                          UA_Boolean isForward, const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional);

UA_StatusCode UA_EXPORT
UA_Client_deleteNode(UA_Client *client, const UA_NodeId nodeId, UA_Boolean deleteTargetReferences);

/* Don't call this function, use the typed versions */
UA_StatusCode UA_EXPORT
__UA_Client_addNode(UA_Client *client, const UA_NodeClass nodeClass,
                    const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                    const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                    const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType, UA_NodeId *outNewNodeId);

static UA_INLINE UA_StatusCode
UA_Client_addVariableNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                          const UA_VariableAttributes attr, UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_VARIABLE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, typeDefinition,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addVariableTypeNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName, const UA_VariableTypeAttributes attr,
                              UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_VARIABLETYPE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addObjectNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                        const UA_ObjectAttributes attr, UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_OBJECT, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, typeDefinition,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addObjectTypeNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName, const UA_ObjectTypeAttributes attr,
                            UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_OBJECTTYPE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addViewNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                      const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                      const UA_QualifiedName browseName, const UA_ViewAttributes attr,
                      UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_VIEW, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_VIEWATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addReferenceTypeNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                               const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                               const UA_QualifiedName browseName, const UA_ReferenceTypeAttributes attr,
                               UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_REFERENCETYPE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addDataTypeNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName, const UA_DataTypeAttributes attr,
                          UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_DATATYPE, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES],
                               outNewNodeId); }

static UA_INLINE UA_StatusCode
UA_Client_addMethodNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                          UA_NodeId *outNewNodeId) {
    return __UA_Client_addNode(client, UA_NODECLASS_METHOD, requestedNewNodeId,
                               parentNodeId, referenceTypeId, browseName, UA_NODEID_NULL,
                               (const UA_NodeAttributes*)&attr, &UA_TYPES[UA_TYPES_METHODATTRIBUTES],
                               outNewNodeId); }

/**
 * .. _client-subscriptions:
 *
 * Subscriptions Handling
 * ======================
 *
 * At this point, the client does not yet contain its own thread or event-driven
 * main-loop. So the client will not perform any actions automatically in the
 * background. This is especially relevant for subscriptions. The user will have
 * to periodically call `UA_Client_Subscriptions_manuallySendPublishRequest`.
 * See also :ref:`here <client-subscriptions>`. */
#ifdef UA_ENABLE_SUBSCRIPTIONS

typedef struct {
    UA_Double requestedPublishingInterval;
    UA_UInt32 requestedLifetimeCount;
    UA_UInt32 requestedMaxKeepAliveCount;
    UA_UInt32 maxNotificationsPerPublish;
    UA_Boolean publishingEnabled;
    UA_Byte priority;
} UA_SubscriptionSettings;

extern const UA_EXPORT UA_SubscriptionSettings UA_SubscriptionSettings_standard;

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_new(UA_Client *client, UA_SubscriptionSettings settings,
                            UA_UInt32 *newSubscriptionId);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId);

UA_StatusCode UA_EXPORT UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client);

typedef void (*UA_MonitoredItemHandlingFunction) (UA_UInt32 monId, UA_DataValue *value, void *context);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                         UA_NodeId nodeId, UA_UInt32 attributeID,
                                         UA_MonitoredItemHandlingFunction handlingFunction,
                                         void *handlingContext, UA_UInt32 *newMonitoredItemId);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId);

#endif

/**
 * Misc Highlevel Functionality
 * ============================ */
/* Get the namespace-index of a namespace-URI
 *
 * @param client The UA_Client struct for this connection
 * @param namespaceUri The interested namespace URI
 * @param namespaceIndex The namespace index of the URI. The value is unchanged
 *        in case of an error
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT
UA_Client_NamespaceGetIndex(UA_Client *client, UA_String *namespaceUri, UA_UInt16 *namespaceIndex);

#ifndef HAVE_NODEITER_CALLBACK
#define HAVE_NODEITER_CALLBACK
/* Iterate over all nodes referenced by parentNodeId by calling the callback
   function for each child node */
typedef UA_StatusCode (*UA_NodeIteratorCallback)(UA_NodeId childId, UA_Boolean isInverse,
                                                  UA_NodeId referenceTypeId, void *handle);
#endif

UA_StatusCode UA_EXPORT
UA_Client_forEachChildNodeCall(UA_Client *client, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) ;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_HIGHLEVEL_H_ */
