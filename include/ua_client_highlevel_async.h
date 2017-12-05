/*
 * ua_client_highlevel_async.h
 *
 *  Created on: Nov 20, 2017
 *      Author: sun
 */

#ifndef UA_CLIENT_HIGHLEVEL_ASYNC_H_
#define UA_CLIENT_HIGHLEVEL_ASYNC_H_
#include "ua_client.h"

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
		UA_AttributeId attributeId, void *out, const UA_DataType *outDataType,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId);

typedef void (*UA_ClientAsyncReadDataTypeAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_NodeId *var);
static UA_INLINE UA_StatusCode UA_Client_readDataTypeAttribute_async(
		UA_Client *client, const UA_NodeId nodeId, UA_NodeId *outValue,
		UA_ClientAsyncServiceCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_DATATYPE, outValue, &UA_TYPES[UA_TYPES_NODEID],
			(UA_ClientAsyncServiceCallback) callback, userdata, reqId);
}

typedef void (*UA_ClientAsyncReadValueAttributeCallback)(UA_Client *client,
		void *userdata, UA_UInt32 requestId, UA_Variant *var);
static UA_INLINE UA_StatusCode UA_Client_readValueAttribute_async(
		UA_Client *client, const UA_NodeId nodeId, UA_Variant *outValue,
		UA_ClientAsyncReadValueAttributeCallback callback, void *userdata,
		UA_UInt32 *reqId) {
	return __UA_Client_readAttribute_async(client, &nodeId,
			UA_ATTRIBUTEID_VALUE, outValue, &UA_TYPES[UA_TYPES_VARIANT],
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
