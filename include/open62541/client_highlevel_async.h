/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_CLIENT_HIGHLEVEL_ASYNC_H_
#define UA_CLIENT_HIGHLEVEL_ASYNC_H_

#include <open62541/client.h>
#include <open62541/common.h>
#include <open62541/types.h>

_UA_BEGIN_DECLS

/**
 * .. _client_async:
 *
 * Async Services
 * ^^^^^^^^^^^^^^
 *
 * Call OPC UA Services asynchronously with a callback. The (optional) requestId
 * output can be used to cancel the service while it is still pending. */

typedef void
(*UA_ClientAsyncReadCallback)(UA_Client *client, void *userdata,
                              UA_UInt32 requestId, UA_ReadResponse *rr);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Client_sendAsyncReadRequest(
    UA_Client *client, UA_ReadRequest *request,
    UA_ClientAsyncReadCallback readCallback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_AsyncService(
        client, request, &UA_TYPES[UA_TYPES_READREQUEST],
        (UA_ClientAsyncServiceCallback)readCallback,
        &UA_TYPES[UA_TYPES_READRESPONSE], userdata, reqId);
})

typedef void
(*UA_ClientAsyncWriteCallback)(UA_Client *client, void *userdata,
                               UA_UInt32 requestId, UA_WriteResponse *wr);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Client_sendAsyncWriteRequest(
    UA_Client *client, UA_WriteRequest *request,
    UA_ClientAsyncWriteCallback writeCallback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_AsyncService(
        client, request, &UA_TYPES[UA_TYPES_WRITEREQUEST],
        (UA_ClientAsyncServiceCallback)writeCallback,
        &UA_TYPES[UA_TYPES_WRITERESPONSE], userdata, reqId);
})

typedef void
(*UA_ClientAsyncBrowseCallback)(UA_Client *client, void *userdata,
                                UA_UInt32 requestId, UA_BrowseResponse *wr);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Client_sendAsyncBrowseRequest(
    UA_Client *client, UA_BrowseRequest *request,
    UA_ClientAsyncBrowseCallback browseCallback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_AsyncService(
        client, request, &UA_TYPES[UA_TYPES_BROWSEREQUEST],
        (UA_ClientAsyncServiceCallback)browseCallback,
        &UA_TYPES[UA_TYPES_BROWSERESPONSE], userdata, reqId);
})

typedef void
(*UA_ClientAsyncBrowseNextCallback)(UA_Client *client, void *userdata,
                                    UA_UInt32 requestId,
                                    UA_BrowseNextResponse *wr);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Client_sendAsyncBrowseNextRequest(
    UA_Client *client, UA_BrowseNextRequest *request,
    UA_ClientAsyncBrowseNextCallback browseNextCallback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_AsyncService(
        client, request, &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST],
        (UA_ClientAsyncServiceCallback)browseNextCallback,
        &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE], userdata, reqId);
})

/**
 * Asynchronous Operations
 * ^^^^^^^^^^^^^^^^^^^^^^^
 *
 * Many Services can be called with an array of operations. For example, a
 * request to the Read Service contains an array of ReadValueId, each
 * corresponding to a single read operation. For convenience, wrappers are
 * provided to call single operations for the most common Services.
 *
 * All async operations have a callback of the following structure: The returned
 * StatusCode is split in two parts. The status indicates the overall success of
 * the request and the operation. The result argument is non-NULL only if the
 * status is no good. */
typedef void
(*UA_ClientAsyncOperationCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, void *result);

/**
 * Read Attribute
 * ^^^^^^^^^^^^^^
 *
 * Asynchronously read a single attribute. The attribute is unpacked from the
 * response as the datatype of the attribute is known ahead of time. Value
 * attributes are variants.
 *
 * Note that the last argument (value pointer) of the callbacks can be NULL if
 * the status of the operation is not good. */

/* Reading a single attribute */
typedef void
(*UA_ClientAsyncReadAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_DataValue *attribute);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readAttribute_async(
    UA_Client *client, const UA_ReadValueId *rvi,
    UA_TimestampsToReturn timestampsToReturn,
    UA_ClientAsyncReadAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single Value attribute */
typedef void
(*UA_ClientAsyncReadValueAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_DataValue *value);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readValueAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadValueAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single DataType attribute */
typedef void
(*UA_ClientAsyncReadDataTypeAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_NodeId *dataType);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readDataTypeAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadDataTypeAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single ArrayDimensions attribute. If the status is good, the variant
 * carries an UInt32 array. */
typedef void
(*UA_ClientReadArrayDimensionsAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Variant *arrayDimensions);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readArrayDimensionsAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientReadArrayDimensionsAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single NodeClass attribute */
typedef void
(*UA_ClientAsyncReadNodeClassAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_NodeClass *nodeClass);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readNodeClassAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadNodeClassAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single BrowseName attribute */
typedef void
(*UA_ClientAsyncReadBrowseNameAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_QualifiedName *browseName);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readBrowseNameAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadBrowseNameAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single DisplayName attribute */
typedef void
(*UA_ClientAsyncReadDisplayNameAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_LocalizedText *displayName);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readDisplayNameAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadDisplayNameAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single Description attribute */
typedef void
(*UA_ClientAsyncReadDescriptionAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_LocalizedText *description);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readDescriptionAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadDescriptionAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single WriteMask attribute */
typedef void
(*UA_ClientAsyncReadWriteMaskAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_UInt32 *writeMask);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readWriteMaskAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadWriteMaskAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single UserWriteMask attribute */
typedef void
(*UA_ClientAsyncReadUserWriteMaskAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_UInt32 *writeMask);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readUserWriteMaskAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadUserWriteMaskAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single IsAbstract attribute */
typedef void
(*UA_ClientAsyncReadIsAbstractAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Boolean *isAbstract);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readIsAbstractAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadIsAbstractAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single Symmetric attribute */
typedef void
(*UA_ClientAsyncReadSymmetricAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Boolean *symmetric);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readSymmetricAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadSymmetricAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single InverseName attribute */
typedef void
(*UA_ClientAsyncReadInverseNameAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_LocalizedText *inverseName);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readInverseNameAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadInverseNameAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single ContainsNoLoops attribute */
typedef void
(*UA_ClientAsyncReadContainsNoLoopsAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Boolean *containsNoLoops);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readContainsNoLoopsAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadContainsNoLoopsAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single EventNotifier attribute */
typedef void
(*UA_ClientAsyncReadEventNotifierAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Byte *eventNotifier);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readEventNotifierAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadEventNotifierAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single ValueRank attribute */
typedef void
(*UA_ClientAsyncReadValueRankAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Int32 *valueRank);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readValueRankAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadValueRankAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single AccessLevel attribute */
typedef void
(*UA_ClientAsyncReadAccessLevelAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Byte *accessLevel);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readAccessLevelAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadAccessLevelAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single AccessLevelEx attribute */
typedef void
(*UA_ClientAsyncReadAccessLevelExAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_UInt32 *accessLevelEx);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readAccessLevelExAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadAccessLevelExAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single UserAccessLevel attribute */
typedef void
(*UA_ClientAsyncReadUserAccessLevelAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Byte *userAccessLevel);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readUserAccessLevelAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadUserAccessLevelAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single MinimumSamplingInterval attribute */
typedef void
(*UA_ClientAsyncReadMinimumSamplingIntervalAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Double *minimumSamplingInterval);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readMinimumSamplingIntervalAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadMinimumSamplingIntervalAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single Historizing attribute */
typedef void
(*UA_ClientAsyncReadHistorizingAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Boolean *historizing);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readHistorizingAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadHistorizingAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single Executable attribute */
typedef void
(*UA_ClientAsyncReadExecutableAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Boolean *executable);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readExecutableAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadExecutableAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Read a single UserExecutable attribute */
typedef void
(*UA_ClientAsyncReadUserExecutableAttributeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_StatusCode status, UA_Boolean *userExecutable);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_readUserExecutableAttribute_async(
    UA_Client *client, const UA_NodeId nodeId,
    UA_ClientAsyncReadUserExecutableAttributeCallback callback,
    void *userdata, UA_UInt32 *requestId);

/**
 * Write Attribute
 * ^^^^^^^^^^^^^^^
 *
 * The methods for async writing of attributes all have a similar API::
 *
 *     UA_StatusCode
 *     UA_Client_writeValueAttribute_async(
 *         UA_Client *client, const UA_NodeId nodeId,
 *         const UA_Variant *attr, UA_ClientAsyncWriteCallback callback,
 *         void *userdata, UA_UInt32 *reqId);
 *
 * We generate the methods for the different attributes with a macro. */

UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Client_writeAttribute_async(
    UA_Client *client, const UA_NodeId *nodeId,
    UA_AttributeId attributeId, const void *in,
    const UA_DataType *inDataType,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *reqId);

#define UA_CLIENT_ASYNCWRITE(NAME, ATTR_ID, ATTR_TYPE, ATTR_TYPEDESC)   \
    UA_INLINABLE( UA_THREADSAFE UA_StatusCode NAME(                     \
        UA_Client *client, const UA_NodeId nodeId,                      \
        const ATTR_TYPE *attr, UA_ClientAsyncWriteCallback callback,    \
        void *userdata, UA_UInt32 *reqId), {                            \
    return __UA_Client_writeAttribute_async(                            \
        client, &nodeId, UA_ATTRIBUTEID_##ATTR_ID, attr,                \
        &UA_TYPES[UA_TYPES_##ATTR_TYPEDESC],                            \
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);      \
})

UA_CLIENT_ASYNCWRITE(UA_Client_writeNodeIdAttribute_async,
                     NODEID, UA_NodeId, NODEID)
UA_CLIENT_ASYNCWRITE(UA_Client_writeNodeClassAttribute_async,
                     NODECLASS, UA_NodeClass, NODECLASS)
UA_CLIENT_ASYNCWRITE(UA_Client_writeBrowseNameAttribute_async,
                     BROWSENAME, UA_QualifiedName, QUALIFIEDNAME)
UA_CLIENT_ASYNCWRITE(UA_Client_writeDisplayNameAttribute_async,
                     DISPLAYNAME, UA_LocalizedText, LOCALIZEDTEXT)
UA_CLIENT_ASYNCWRITE(UA_Client_writeDescriptionAttribute_async,
                     DESCRIPTION, UA_LocalizedText, LOCALIZEDTEXT)
UA_CLIENT_ASYNCWRITE(UA_Client_writeWriteMaskAttribute_async,
                     WRITEMASK, UA_UInt32, UINT32)
UA_CLIENT_ASYNCWRITE(UA_Client_writeIsAbstractAttribute_async,
                     ISABSTRACT, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE(UA_Client_writeSymmetricAttribute_async,
                     SYMMETRIC, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE(UA_Client_writeInverseNameAttribute_async,
                     INVERSENAME, UA_LocalizedText, LOCALIZEDTEXT)
UA_CLIENT_ASYNCWRITE(UA_Client_writeContainsNoLoopsAttribute_async,
                     CONTAINSNOLOOPS, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE(UA_Client_writeEventNotifierAttribute_async,
                     EVENTNOTIFIER, UA_Byte, BYTE)
UA_CLIENT_ASYNCWRITE(UA_Client_writeValueAttribute_async,
                     VALUE, UA_Variant, VARIANT)
UA_CLIENT_ASYNCWRITE(UA_Client_writeDataTypeAttribute_async,
                     DATATYPE, UA_NodeId, NODEID)
UA_CLIENT_ASYNCWRITE(UA_Client_writeValueRankAttribute_async,
                     VALUERANK, UA_Int32, INT32)
UA_CLIENT_ASYNCWRITE(UA_Client_writeAccessLevelAttribute_async,
                     ACCESSLEVEL, UA_Byte, BYTE)
UA_CLIENT_ASYNCWRITE(UA_Client_writeMinimumSamplingIntervalAttribute_async,
                     MINIMUMSAMPLINGINTERVAL, UA_Double, DOUBLE)
UA_CLIENT_ASYNCWRITE(UA_Client_writeHistorizingAttribute_async,
                     HISTORIZING, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE(UA_Client_writeExecutableAttribute_async,
                     EXECUTABLE, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE(UA_Client_writeAccessLevelExAttribute_async,
                     ACCESSLEVELEX, UA_UInt32, UINT32)

/**
 * Method Calling
 * ^^^^^^^^^^^^^^ */
UA_StatusCode UA_EXPORT UA_THREADSAFE
__UA_Client_call_async(
    UA_Client *client,
    const UA_NodeId objectId, const UA_NodeId methodId,
    size_t inputSize, const UA_Variant *input,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *reqId);

typedef void
(*UA_ClientAsyncCallCallback)(
    UA_Client *client, void *userdata,
    UA_UInt32 requestId, UA_CallResponse *cr);

UA_INLINABLE( UA_THREADSAFE UA_StatusCode
UA_Client_call_async(
    UA_Client *client, const UA_NodeId objectId,
    const UA_NodeId methodId, size_t inputSize,
    const UA_Variant *input, UA_ClientAsyncCallCallback callback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_call_async(
        client, objectId, methodId, inputSize, input,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

/**
 * Node Management
 * ^^^^^^^^^^^^^^^ */
typedef void
(*UA_ClientAsyncAddNodesCallback)(
    UA_Client *client, void *userdata,
    UA_UInt32 requestId, UA_AddNodesResponse *ar);

UA_StatusCode UA_EXPORT
__UA_Client_addNode_async(
    UA_Client *client, const UA_NodeClass nodeClass,
    const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
    const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
    const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
    const UA_DataType *attributeType, UA_NodeId *outNewNodeId,
    UA_ClientAsyncServiceCallback callback, void *userdata,
    UA_UInt32 *reqId);

UA_INLINABLE( UA_StatusCode
UA_Client_addVariableNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
    const UA_VariableAttributes attr, UA_NodeId *outNewNodeId,
    UA_ClientAsyncAddNodesCallback callback, void *userdata,
    UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_VARIABLE, requestedNewNodeId,
        parentNodeId, referenceTypeId, browseName,
        typeDefinition, (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addVariableTypeNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_VariableTypeAttributes attr,
    UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_VARIABLETYPE, requestedNewNodeId, parentNodeId,
        referenceTypeId, browseName, UA_NODEID_NULL,
        (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addObjectNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
    const UA_ObjectAttributes attr, UA_NodeId *outNewNodeId,
    UA_ClientAsyncAddNodesCallback callback, void *userdata,
    UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_OBJECT, requestedNewNodeId,
        parentNodeId, referenceTypeId,
        browseName, typeDefinition, (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addObjectTypeNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_ObjectTypeAttributes attr,
    UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_OBJECTTYPE, requestedNewNodeId, parentNodeId,
        referenceTypeId, browseName, UA_NODEID_NULL,
        (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addViewNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName,
    const UA_ViewAttributes attr, UA_NodeId *outNewNodeId,
    UA_ClientAsyncAddNodesCallback callback, void *userdata,
    UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_VIEW, requestedNewNodeId,
        parentNodeId, referenceTypeId,
        browseName, UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_VIEWATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addReferenceTypeNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_ReferenceTypeAttributes attr,
    UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_REFERENCETYPE, requestedNewNodeId, parentNodeId,
        referenceTypeId, browseName, UA_NODEID_NULL,
        (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addDataTypeNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_DataTypeAttributes attr,
    UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_DATATYPE, requestedNewNodeId,
        parentNodeId, referenceTypeId, browseName,
        UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

UA_INLINABLE( UA_StatusCode
UA_Client_addMethodNode_async(
    UA_Client *client, const UA_NodeId requestedNewNodeId,
    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
    const UA_QualifiedName browseName, const UA_MethodAttributes attr,
    UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
    void *userdata, UA_UInt32 *reqId), {
    return __UA_Client_addNode_async(
        client, UA_NODECLASS_METHOD, requestedNewNodeId, parentNodeId,
        referenceTypeId, browseName, UA_NODEID_NULL,
        (const UA_NodeAttributes *)&attr,
        &UA_TYPES[UA_TYPES_METHODATTRIBUTES], outNewNodeId,
        (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
})

_UA_END_DECLS

#endif /* UA_CLIENT_HIGHLEVEL_ASYNC_H_ */
