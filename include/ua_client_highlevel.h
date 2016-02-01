#ifndef UA_CLIENT_HIGHLEVEL_H_
#define UA_CLIENT_HIGHLEVEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_client.h"

/**
 * Get the namespace-index of a namespace-URI
 *
 * @param client The UA_Client struct for this connection
 * @param namespaceUri The interested namespace URI
 * @param namespaceIndex The namespace index of the URI. The value is unchanged in case of an error
 * @return Indicates whether the operation succeeded or returns an error code
 */
UA_StatusCode UA_EXPORT UA_Client_NamespaceGetIndex(UA_Client *client, UA_String *namespaceUri, UA_UInt16 *namespaceIndex);

/*******************/
/* Node Management */
/*******************/

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

/*************************/
/* Attribute Service Set */
/*************************/

/* Don't use this function. There are typed versions for every supported attribute. */
UA_StatusCode UA_EXPORT
__UA_Client_readAttribute(UA_Client *client, UA_NodeId nodeId, UA_AttributeId attributeId,
                          void *out, const UA_DataType *outDataType);
  
static UA_INLINE UA_StatusCode
UA_Client_readNodeIdAttribute(UA_Client *client, UA_NodeId nodeId, UA_NodeId *outNodeId) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_NODEID,
                                     outNodeId, &UA_TYPES[UA_TYPES_NODEID]); }

static UA_INLINE UA_StatusCode
UA_Client_readNodeClassAttribute(UA_Client *client, UA_NodeId nodeId, UA_NodeClass *outNodeClass) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_NODECLASS,
                                     outNodeClass, &UA_TYPES[UA_TYPES_NODECLASS]); }

static UA_INLINE UA_StatusCode
UA_Client_readBrowseNameAttribute(UA_Client *client, UA_NodeId nodeId, UA_QualifiedName *outBrowseName) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_BROWSENAME,
                                     outBrowseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]); }

static UA_INLINE UA_StatusCode
UA_Client_readDisplayNameAttribute(UA_Client *client, UA_NodeId nodeId, UA_LocalizedText *outDisplayName) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                                     outDisplayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_readDescriptionAttribute(UA_Client *client, UA_NodeId nodeId, UA_LocalizedText *outDescription) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                                     outDescription, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_readWriteMaskAttribute(UA_Client *client, UA_NodeId nodeId, UA_UInt32 *outWriteMask) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_WRITEMASK,
                                     outWriteMask, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readUserWriteMaskAttribute(UA_Client *client, UA_NodeId nodeId, UA_UInt32 *outUserWriteMask) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_USERWRITEMASK,
                                     outUserWriteMask, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readIsAbstractAttribute(UA_Client *client, UA_NodeId nodeId, UA_Boolean *outIsAbstract) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                                     outIsAbstract, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readSymmetricAttribute(UA_Client *client, UA_NodeId nodeId, UA_Boolean *outSymmetric) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_SYMMETRIC,
                                     outSymmetric, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readInverseNameAttribute(UA_Client *client, UA_NodeId nodeId, UA_LocalizedText *outInverseName) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_INVERSENAME,
                                     outInverseName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]); }

static UA_INLINE UA_StatusCode
UA_Client_readContainsNoLoopsAttribute(UA_Client *client, UA_NodeId nodeId, UA_Boolean *outContainsNoLoops) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS,
                                     outContainsNoLoops, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readEventNotifierAttribute(UA_Client *client, UA_NodeId nodeId, UA_Byte *outEventNotifier) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                                     outEventNotifier, &UA_TYPES[UA_TYPES_BYTE]); }

static UA_INLINE UA_StatusCode
UA_Client_readValueAttribute(UA_Client *client, UA_NodeId nodeId, UA_Variant *outValue) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_VALUE,
                                     outValue, &UA_TYPES[UA_TYPES_VARIANT]); }

static UA_INLINE UA_StatusCode
UA_Client_readDataTypeAttribute(UA_Client *client, UA_NodeId nodeId, UA_NodeId *outDataType) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_DATATYPE,
                                     outDataType, &UA_TYPES[UA_TYPES_NODEID]); }

static UA_INLINE UA_StatusCode
UA_Client_readValueRankAttribute(UA_Client *client, UA_NodeId nodeId, UA_Int32 *outValueRank) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_VALUERANK,
                                     outValueRank, &UA_TYPES[UA_TYPES_INT32]); }

// todo: fetch an array
static UA_INLINE UA_StatusCode
UA_Client_readArrayDimensionsAttribute(UA_Client *client, UA_NodeId nodeId, UA_Int32 *outArrayDimensions) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                                     outArrayDimensions, &UA_TYPES[UA_TYPES_INT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readAccessLevelAttribute(UA_Client *client, UA_NodeId nodeId, UA_UInt32 *outAccessLevel) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                                     outAccessLevel, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readUserAccessLevelAttribute(UA_Client *client, UA_NodeId nodeId, UA_UInt32 *outUserAccessLevel) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_USERACCESSLEVEL,
                                     outUserAccessLevel, &UA_TYPES[UA_TYPES_UINT32]); }

static UA_INLINE UA_StatusCode
UA_Client_readMinimumSamplingIntervalAttribute(UA_Client *client, UA_NodeId nodeId,
                                               UA_Double *outMinimumSamplingInterval) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                                     outMinimumSamplingInterval, &UA_TYPES[UA_TYPES_DOUBLE]); }

static UA_INLINE UA_StatusCode
UA_Client_readHistorizingAttribute(UA_Client *client, UA_NodeId nodeId, UA_Boolean *outHistorizing) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_HISTORIZING,
                                     outHistorizing, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readExecutableAttribute(UA_Client *client, UA_NodeId nodeId, UA_Boolean *outExecutable) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                                     outExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]); }

static UA_INLINE UA_StatusCode
UA_Client_readUserExecutableAttribute(UA_Client *client, UA_NodeId nodeId, UA_Boolean *outUserExecutable) {
    return __UA_Client_readAttribute(client, nodeId, UA_ATTRIBUTEID_USEREXECUTABLE,
                                     outUserExecutable, &UA_TYPES[UA_TYPES_BOOLEAN]); }

/**********************/
/* Method Service Set */
/**********************/

UA_StatusCode UA_EXPORT
UA_Client_call(UA_Client *client, const UA_NodeId objectId, const UA_NodeId methodId,
               size_t inputSize, const UA_Variant *input, size_t *outputSize, UA_Variant **output);

/**************************/
/* Subscriptions Handling */
/**************************/

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

void UA_EXPORT UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                         UA_NodeId nodeId, UA_UInt32 attributeID,
                                         void *handlingFunction, void *handlingContext,
                                         UA_UInt32 *newMonitoredItemId);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_HIGHLEVEL_H_ */
