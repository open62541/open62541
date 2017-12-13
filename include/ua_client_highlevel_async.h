/*
 * ua_client_highlevel_async.h
 *
 *  Created on: Nov 20, 2017
 *      Author: sun
 */

#ifndef UA_CLIENT_HIGHLEVEL_ASYNC_H_
#define UA_CLIENT_HIGHLEVEL_ASYNC_H_
#include "ua_client.h"

/*Raw Services
 * ^^^^^^^^^^^^^^ */
typedef void (*UA_ClientAsyncReadCallback)(UA_Client *client, void *userdata,
		UA_UInt32 requestId, UA_ReadResponse *rr);
static UA_INLINE UA_StatusCode UA_Client_sendAsyncReadRequest(UA_Client *client,
		UA_ReadRequest *request, UA_ClientAsyncReadCallback readCallback,
		void *userdata, UA_UInt32 *reqId) {
	return UA_Client_sendAsyncRequest(client, request,
			&UA_TYPES[UA_TYPES_READREQUEST],
			(UA_ClientAsyncServiceCallback) readCallback,
			&UA_TYPES[UA_TYPES_READRESPONSE], userdata, reqId);
}

typedef void (*UA_ClientAsyncWriteCallback)(UA_Client *client, void *userdata,
		UA_UInt32 requestId, UA_WriteResponse *wr);
static UA_INLINE UA_StatusCode UA_Client_sendAsyncWriteRequest(
		UA_Client *client, UA_WriteRequest *request,
		UA_ClientAsyncWriteCallback writeCallback, void *userdata,
		UA_UInt32 *reqId) {
	return UA_Client_sendAsyncRequest(client, request,
			&UA_TYPES[UA_TYPES_WRITEREQUEST],
			(UA_ClientAsyncServiceCallback) writeCallback,
			&UA_TYPES[UA_TYPES_WRITERESPONSE], userdata, reqId);
}

typedef void (*UA_ClientAsyncBrowseCallback)(UA_Client *client, void *userdata,
		UA_UInt32 requestId, UA_BrowseResponse *wr);
static UA_INLINE UA_StatusCode UA_Client_sendAsyncBrowseRequest(
		UA_Client *client, UA_BrowseRequest *request,
		UA_ClientAsyncBrowseCallback browseCallback, void *userdata,
		UA_UInt32 *reqId) {
	return UA_Client_sendAsyncRequest(client, request,
			&UA_TYPES[UA_TYPES_BROWSEREQUEST],
			(UA_ClientAsyncServiceCallback) browseCallback,
			&UA_TYPES[UA_TYPES_BROWSERESPONSE], userdata, reqId);
}

/**
 * Read Attribute
 * ^^^^^^^^^^^^^^ */
UA_StatusCode UA_EXPORT
__UA_Client_readAttribute_async(UA_Client *client, const UA_NodeId *nodeId,
		UA_AttributeId attributeId, const UA_DataType *outDataType,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId);

typedef void (*UA_ClientAsyncReadDataTypeAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_NodeId *var);
static UA_INLINE UA_StatusCode UA_Client_readDataTypeAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadDataTypeAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_DATATYPE, &UA_TYPES[UA_TYPES_NODEID],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}

typedef void (*UA_ClientAsyncReadValueAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_Variant *var);
static UA_INLINE UA_StatusCode UA_Client_readValueAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadValueAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_VALUE, &UA_TYPES[UA_TYPES_VARIANT],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}

typedef void (*UA_ClientAsyncReadNodeIdAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_NodeId *out);
static UA_INLINE UA_StatusCode UA_Client_readNodeIdAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadNodeIdAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_NODEID, &UA_TYPES[UA_TYPES_NODEID],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadNodeClassAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_NodeClass *out);
static UA_INLINE UA_StatusCode UA_Client_readNodeClassAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadNodeClassAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_NODECLASS, &UA_TYPES[UA_TYPES_NODECLASS],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadBrowseNameAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_QualifiedName *out);
static UA_INLINE UA_StatusCode UA_Client_readBrowseNameAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadBrowseNameAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_BROWSENAME, &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadDisplayNameAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId,
		UA_LocalizedText *out);
static UA_INLINE UA_StatusCode UA_Client_readDisplayNameAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadDisplayNameAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_DISPLAYNAME, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadDescriptionAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId,
		UA_LocalizedText *out);
static UA_INLINE UA_StatusCode UA_Client_readDescriptionAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadDescriptionAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_DESCRIPTION, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadWriteMaskAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_UInt32 *out);
static UA_INLINE UA_StatusCode UA_Client_readWriteMaskAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadWriteMaskAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_WRITEMASK, &UA_TYPES[UA_TYPES_UINT32],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadUserWriteMaskAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId, UA_UInt32 *out);
static UA_INLINE UA_StatusCode UA_Client_readUserWriteMaskAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadUserWriteMaskAttributeCallback callback,
		void *userdata, UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_USERWRITEMASK, &UA_TYPES[UA_TYPES_UINT32],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadIsAbstractAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_Boolean *out);
static UA_INLINE UA_StatusCode UA_Client_readIsAbstractAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadIsAbstractAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_ISABSTRACT, &UA_TYPES[UA_TYPES_BOOLEAN],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadSymmetricAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_Boolean *out);
static UA_INLINE UA_StatusCode UA_Client_readSymmetricAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadSymmetricAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_SYMMETRIC, &UA_TYPES[UA_TYPES_BOOLEAN],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadInverseNameAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId,
		UA_LocalizedText *out);
static UA_INLINE UA_StatusCode UA_Client_readInverseNameAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadInverseNameAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_INVERSENAME, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadContainsNoLoopsAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId,
		UA_Boolean *out);
static UA_INLINE UA_StatusCode UA_Client_readContainsNoLoopsAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadContainsNoLoopsAttributeCallback callback,
		void *userdata, UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_CONTAINSNOLOOPS, &UA_TYPES[UA_TYPES_BOOLEAN],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadEventNotifierAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId, UA_Byte *out);
static UA_INLINE UA_StatusCode UA_Client_readEventNotifierAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadEventNotifierAttributeCallback callback,
		void *userdata, UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_EVENTNOTIFIER, &UA_TYPES[UA_TYPES_BYTE],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadValueRankAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_Int32 *out);
static UA_INLINE UA_StatusCode UA_Client_readValueRankAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadValueRankAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_VALUERANK, &UA_TYPES[UA_TYPES_INT32],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadAccessLevelAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId, UA_Byte *out);
static UA_INLINE UA_StatusCode UA_Client_readAccessLevelAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadAccessLevelAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_ACCESSLEVEL, &UA_TYPES[UA_TYPES_BYTE],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadUserAccessLevelAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId, UA_Byte *out);
static UA_INLINE UA_StatusCode UA_Client_readUserAccessLevelAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadUserAccessLevelAttributeCallback callback,
		void *userdata, UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_USERACCESSLEVEL, &UA_TYPES[UA_TYPES_BYTE],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadMinimumSamplingIntervalAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId, UA_Double *out);
static UA_INLINE UA_StatusCode UA_Client_readMinimumSamplingIntervalAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadMinimumSamplingIntervalAttributeCallback callback,
		void *userdata, UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL, &UA_TYPES[UA_TYPES_DOUBLE],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadHistorizingAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId,
		UA_Boolean *out);
static UA_INLINE UA_StatusCode UA_Client_readHistorizingAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadHistorizingAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_HISTORIZING, &UA_TYPES[UA_TYPES_BOOLEAN],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadExecutableAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_Boolean *out);
static UA_INLINE UA_StatusCode UA_Client_readExecutableAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadExecutableAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_EXECUTABLE, &UA_TYPES[UA_TYPES_BOOLEAN],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
typedef void (*UA_ClientAsyncReadUserExecutableAttributeCallback)(
		UA_Client *client, void *userdata, UA_UInt32 requestId,
		UA_Boolean *out);
static UA_INLINE UA_StatusCode UA_Client_readUserExecutableAttribute_async(
		UA_Client *client, const UA_NodeId nodeId,
		UA_ClientAsyncReadUserExecutableAttributeCallback callback,
		void *userdata, UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_USEREXECUTABLE, &UA_TYPES[UA_TYPES_BOOLEAN],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}
/**
 * Write Attribute
 * ^^^^^^^^^^^^^^ */
UA_StatusCode UA_EXPORT
__UA_Client_writeAttribute_async(UA_Client *client, const UA_NodeId *nodeId,
		UA_AttributeId attributeId, const void *in,
		const UA_DataType *inDataType, UA_ClientAsyncServiceCallback callback,
		void *userdata, UA_UInt32 *reqId);

static UA_INLINE UA_StatusCode UA_Client_writeValueAttribute_async(
		UA_Client *client, const UA_NodeId nodeId, const UA_Variant *newValue,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId) {

	return __UA_Client_writeAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_VALUE, newValue, &UA_TYPES[UA_TYPES_VARIANT],
			callback, userdata, reqId);
}

/**
 * Method Calling
 * ^^^^^^^^^^^^^^ */

UA_StatusCode UA_EXPORT __UA_Client_call_async(UA_Client *client,
		const UA_NodeId objectId, const UA_NodeId methodId, size_t inputSize,
		const UA_Variant *input, UA_ClientAsyncServiceCallback callback,
		void *userdata, UA_UInt32 *reqId);

typedef void (*UA_ClientAsyncCallCallback)(UA_Client *client, void *userdata,
		UA_UInt32 requestId, UA_CallResponse *cr);
static UA_INLINE UA_StatusCode UA_Client_call_async(UA_Client *client,
		const UA_NodeId objectId, const UA_NodeId methodId, size_t inputSize,
		const UA_Variant *input, UA_ClientAsyncCallCallback callback,
		void *userdata, UA_UInt32 *reqId) {

	return __UA_Client_call_async(client, objectId, methodId, inputSize, input,
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}

/**
 * Misc Functionalities
 * ^^^^^^^^^^^^^^ */

UA_StatusCode UA_EXPORT __UA_Client_translateBrowsePathsToNodeIds_async(
		UA_Client *client, char *paths[], UA_UInt32 ids[], size_t pathSize,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId);

typedef void (*UA_ClientAsyncTranslateCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId,
		UA_TranslateBrowsePathsToNodeIdsResponse *tr);
static UA_INLINE UA_StatusCode UA_Cient_translateBrowsePathsToNodeIds_async(
		UA_Client *client, char **paths, UA_UInt32 *ids, size_t pathSize,
		UA_ClientAsyncTranslateCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_translateBrowsePathsToNodeIds_async(client, paths, ids,
			pathSize, (UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}

/*getEndpoints not working yet*/
UA_StatusCode UA_EXPORT
__UA_Client_getEndpoints_async(UA_Client *client,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId);

static UA_INLINE UA_StatusCode UA_Client_getEndpoints_async(UA_Client *client,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_getEndpoints_async(client, callback, userdata, reqId);
}

#endif /* UA_CLIENT_HIGHLEVEL_ASYNC_H_ */
