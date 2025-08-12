/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2016 (c) Chris Iatrou
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Frank Meerkötter
 *    Copyright 2018 (c) Fabian Arndt
 *    Copyright 2018 (c) Peter Rustler, basyskom GmbH
 */

#ifndef UA_CLIENT_HIGHLEVEL_H_
#define UA_CLIENT_HIGHLEVEL_H_

#include <open62541/client.h>

_UA_BEGIN_DECLS

/**
 * .. _client-highlevel:
 *
 * Highlevel Client Functionality
 * ------------------------------
 *
 * The following definitions are convenience functions making use of the
 * standard OPC UA services in the background. This is a less flexible way of
 * handling the stack, because at many places sensible defaults are presumed; at
 * the same time using these functions is the easiest way of implementing an OPC
 * UA application, as you will not have to consider all the details that go into
 * the OPC UA services. If more flexibility is needed, you can always achieve
 * the same functionality using the raw :ref:`OPC UA services
 * <client-services>`.
 *
 * Read Attributes
 * ~~~~~~~~~~~~~~~
 *
 * The following functions can be used to retrieve a single node attribute. Use
 * the regular service to read several attributes at once. */

UA_DataValue UA_EXPORT UA_THREADSAFE
UA_Client_read(UA_Client *client, const UA_ReadValueId *rvi);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readNodeIdAttribute(UA_Client *client, const UA_NodeId nodeId,
                              UA_NodeId *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readNodeClassAttribute(UA_Client *client, const UA_NodeId nodeId,
                                 UA_NodeClass *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readBrowseNameAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  UA_QualifiedName *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readDisplayNameAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   UA_LocalizedText *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readDescriptionAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   UA_LocalizedText *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId,
                                 UA_UInt32 *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readUserWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId,
                                     UA_UInt32 *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readIsAbstractAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readSymmetricAttribute(UA_Client *client, const UA_NodeId nodeId,
                                 UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readInverseNameAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   UA_LocalizedText *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readContainsNoLoopsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                       UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readEventNotifierAttribute(UA_Client *client, const UA_NodeId nodeId,
                                     UA_Byte *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readValueAttribute(UA_Client *client, const UA_NodeId nodeId,
                             UA_Variant *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readDataTypeAttribute(UA_Client *client, const UA_NodeId nodeId,
                                UA_NodeId *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readValueRankAttribute(UA_Client *client, const UA_NodeId nodeId,
                                 UA_Int32 *out);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readArrayDimensionsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                       size_t *outArrayDimensionsSize,
                                       UA_UInt32 **outArrayDimensions);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   UA_Byte *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readAccessLevelExAttribute(UA_Client *client, const UA_NodeId nodeId,
                                     UA_UInt32 *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readUserAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId,
                                       UA_Byte *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readMinimumSamplingIntervalAttribute(UA_Client *client,
                                               const UA_NodeId nodeId,
                                               UA_Double *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readHistorizingAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readExecutableAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  UA_Boolean *out);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_readUserExecutableAttribute(UA_Client *client,
                                      const UA_NodeId nodeId,
                                      UA_Boolean *out);

/**
 * Historical Access
 * ~~~~~~~~~~~~~~~~~
 *
 * The following functions can be used to read a single node historically.
 * Use the regular service to read several nodes at once. */

typedef UA_Boolean
(*UA_HistoricalIteratorCallback)(
    UA_Client *client, const UA_NodeId *nodeId, UA_Boolean moreDataAvailable,
    const UA_ExtensionObject *data, void *callbackContext);

UA_StatusCode UA_EXPORT
UA_Client_HistoryRead_events(
    UA_Client *client, const UA_NodeId *nodeId,
    const UA_HistoricalIteratorCallback callback, UA_DateTime startTime,
    UA_DateTime endTime, UA_String indexRange, const UA_EventFilter filter,
    UA_UInt32 numValuesPerNode, UA_TimestampsToReturn timestampsToReturn,
    void *callbackContext);

UA_StatusCode UA_EXPORT
UA_Client_HistoryRead_raw(
    UA_Client *client, const UA_NodeId *nodeId,
    const UA_HistoricalIteratorCallback callback, UA_DateTime startTime,
    UA_DateTime endTime, UA_String indexRange, UA_Boolean returnBounds,
    UA_UInt32 numValuesPerNode, UA_TimestampsToReturn timestampsToReturn,
    void *callbackContext);

UA_StatusCode UA_EXPORT
UA_Client_HistoryRead_modified(
    UA_Client *client, const UA_NodeId *nodeId,
    const UA_HistoricalIteratorCallback callback, UA_DateTime startTime,
    UA_DateTime endTime, UA_String indexRange, UA_Boolean returnBounds,
    UA_UInt32 numValuesPerNode, UA_TimestampsToReturn timestampsToReturn,
    void *callbackContext);

UA_StatusCode UA_EXPORT
UA_Client_HistoryUpdate_insert(
    UA_Client *client, const UA_NodeId *nodeId, UA_DataValue *value);

UA_StatusCode UA_EXPORT
UA_Client_HistoryUpdate_replace(
    UA_Client *client, const UA_NodeId *nodeId, UA_DataValue *value);

UA_StatusCode UA_EXPORT
UA_Client_HistoryUpdate_update(
    UA_Client *client, const UA_NodeId *nodeId, UA_DataValue *value);

UA_StatusCode UA_EXPORT
UA_Client_HistoryUpdate_deleteRaw(
    UA_Client *client, const UA_NodeId *nodeId,
    UA_DateTime startTimestamp, UA_DateTime endTimestamp);

/**
 * Write Attributes
 * ~~~~~~~~~~~~~~~~
 *
 * The following functions can be use to write a single node attribute at a
 * time. Use the regular write service to write several attributes at once. */

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_write(UA_Client *client, const UA_WriteValue *wv);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeNodeIdAttribute(UA_Client *client, const UA_NodeId nodeId,
                               const UA_NodeId *newNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeNodeClassAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  const UA_NodeClass *newNodeClass);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeBrowseNameAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   const UA_QualifiedName *newBrowseName);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeDisplayNameAttribute(UA_Client *client, const UA_NodeId nodeId,
                                    const UA_LocalizedText *newDisplayName);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeDescriptionAttribute(UA_Client *client, const UA_NodeId nodeId,
                                    const UA_LocalizedText *newDescription);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  const UA_UInt32 *newWriteMask);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeUserWriteMaskAttribute(UA_Client *client, const UA_NodeId nodeId,
                                      const UA_UInt32 *newUserWriteMask);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeIsAbstractAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   const UA_Boolean *newIsAbstract);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeSymmetricAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  const UA_Boolean *newSymmetric);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeInverseNameAttribute(UA_Client *client, const UA_NodeId nodeId,
                                    const UA_LocalizedText *newInverseName);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeContainsNoLoopsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                        const UA_Boolean *newContainsNoLoops);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeEventNotifierAttribute(UA_Client *client, const UA_NodeId nodeId,
                                      const UA_Byte *newEventNotifier);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeValueAttribute(UA_Client *client, const UA_NodeId nodeId,
                              const UA_Variant *newValue);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeValueAttribute_scalar(UA_Client *client, const UA_NodeId nodeId,
                                     const void *newValue,
                                     const UA_DataType *valueType);

/* Write a DataValue that can include timestamps and status codes */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeValueAttributeEx(UA_Client *client, const UA_NodeId nodeId,
                                const UA_DataValue *newValue);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeDataTypeAttribute(UA_Client *client, const UA_NodeId nodeId,
                                 const UA_NodeId *newDataType);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeValueRankAttribute(UA_Client *client, const UA_NodeId nodeId,
                                  const UA_Int32 *newValueRank);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_writeArrayDimensionsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                        size_t newArrayDimensionsSize,
                                        const UA_UInt32 *newArrayDimensions);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId,
                                    const UA_Byte *newAccessLevel);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeAccessLevelExAttribute(UA_Client *client, const UA_NodeId nodeId,
                                      UA_UInt32 *newAccessLevelEx);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeUserAccessLevelAttribute(UA_Client *client, const UA_NodeId nodeId,
                                        const UA_Byte *newUserAccessLevel);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeMinimumSamplingIntervalAttribute(UA_Client *client,
                                                const UA_NodeId nodeId,
                                                const UA_Double *newMinInterval);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeHistorizingAttribute(UA_Client *client, const UA_NodeId nodeId,
                                    const UA_Boolean *newHistorizing);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeExecutableAttribute(UA_Client *client, const UA_NodeId nodeId,
                                   const UA_Boolean *newExecutable);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_writeUserExecutableAttribute(UA_Client *client, const UA_NodeId nodeId,
                                       const UA_Boolean *newUserExecutable);

/**
 * Method Calling
 * ~~~~~~~~~~~~~~ */

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_call(UA_Client *client,
               const UA_NodeId objectId, const UA_NodeId methodId,
               size_t inputSize, const UA_Variant *input,
               size_t *outputSize, UA_Variant **output);

/**
 * Browsing
 * ~~~~~~~~ */

UA_EXPORT UA_THREADSAFE UA_BrowseResult
UA_Client_browse(UA_Client *client,
                 const UA_ViewDescription *view,
                 UA_UInt32 requestedMaxReferencesPerNode,
                 const UA_BrowseDescription *nodesToBrowse);

UA_EXPORT UA_THREADSAFE UA_BrowseResult
UA_Client_browseNext(UA_Client *client,
                     UA_Boolean releaseContinuationPoint,
                     UA_ByteString continuationPoint);

UA_EXPORT UA_THREADSAFE UA_BrowsePathResult
UA_Client_translateBrowsePathToNodeIds(UA_Client *client,
                                       const UA_BrowsePath *browsePath);

/**
 * Node Management
 * ~~~~~~~~~~~~~~~
 * See the section on :ref:`server-side node management <server-node-management>`. */

UA_StatusCode UA_EXPORT
UA_Client_addReference(UA_Client *client, const UA_NodeId sourceNodeId,
                       const UA_NodeId referenceTypeId, UA_Boolean isForward,
                       const UA_String targetServerUri,
                       const UA_ExpandedNodeId targetNodeId,
                       UA_NodeClass targetNodeClass);

UA_StatusCode UA_EXPORT
UA_Client_deleteReference(UA_Client *client, const UA_NodeId sourceNodeId,
                          const UA_NodeId referenceTypeId, UA_Boolean isForward,
                          const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional);

UA_StatusCode UA_EXPORT
UA_Client_deleteNode(UA_Client *client, const UA_NodeId nodeId,
                     UA_Boolean deleteTargetReferences);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addVariableNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_NodeId typeDefinition,
                          const UA_VariableAttributes attr,
                          UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addVariableTypeNode(UA_Client *client,
                              const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId,
                              const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName,
                              const UA_VariableTypeAttributes attr,
                              UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addObjectNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const UA_ObjectAttributes attr, UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addObjectTypeNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId,
                            const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName,
                            const UA_ObjectTypeAttributes attr,
                            UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addViewNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                      const UA_NodeId parentNodeId,
                      const UA_NodeId referenceTypeId,
                      const UA_QualifiedName browseName,
                      const UA_ViewAttributes attr,
                      UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addReferenceTypeNode(UA_Client *client,
                               const UA_NodeId requestedNewNodeId,
                               const UA_NodeId parentNodeId,
                               const UA_NodeId referenceTypeId,
                               const UA_QualifiedName browseName,
                               const UA_ReferenceTypeAttributes attr,
                               UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addDataTypeNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_DataTypeAttributes attr,
                          UA_NodeId *outNewNodeId);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_Client_addMethodNode(UA_Client *client, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_MethodAttributes attr,
                        UA_NodeId *outNewNodeId);

/**
 * Misc Highlevel Functionality
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* Get the namespace-index of a namespace-URI
 *
 * @param client The UA_Client struct for this connection
 * @param namespaceUri The interested namespace URI
 * @param namespaceIndex The namespace index of the URI. The value is unchanged
 *        in case of an error
 * @return Indicates whether the operation succeeded or returns an error code */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_NamespaceGetIndex(UA_Client *client, UA_String *namespaceUri,
                            UA_UInt16 *namespaceIndex);

#ifndef HAVE_NODEITER_CALLBACK
#define HAVE_NODEITER_CALLBACK
/* Iterate over all nodes referenced by parentNodeId by calling the callback
 * function for each child node */
typedef UA_StatusCode
(*UA_NodeIteratorCallback)(UA_NodeId childId, UA_Boolean isInverse,
                           UA_NodeId referenceTypeId, void *handle);
#endif

typedef struct{
    UA_BrowseDirection direction;
    UA_Boolean includeSubtypes;
    UA_NodeId* refType;
} UA_BrowseOptions;

UA_StatusCode UA_EXPORT
UA_Client_forEachChildNodeCall_Ex(UA_Client *client, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle,
                               const UA_BrowseOptions* options);

/* Macro for compatibility with old version of function without UA_BrowseOptions structure */
#define UA_Client_forEachChildNodeCall(client, parentNodeId, callback, handle) \
    UA_Client_forEachChildNodeCall_Ex(client, parentNodeId, callback, handle, NULL);

_UA_END_DECLS

#endif /* UA_CLIENT_HIGHLEVEL_H_ */
