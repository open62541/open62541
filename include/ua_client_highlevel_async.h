/*
 * ua_client_highlevel_async.h
 *
 *  Created on: Nov 20, 2017
 *      Author: sun
 */

#ifndef UA_CLIENT_HIGHLEVEL_ASYNC_H_
#define UA_CLIENT_HIGHLEVEL_ASYNC_H_
#include "ua_client.h"
UA_StatusCode UA_EXPORT
__UA_Client_readAttribute_async(UA_Client *client, const UA_NodeId *nodeId,
                          UA_AttributeId attributeId, void *out,
                          const UA_DataType *outDataType,
                          UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32 *reqId);

UA_StatusCode UA_EXPORT
__UA_Client_writeAttribute_async(UA_Client *client, const UA_NodeId *nodeId,
                           UA_AttributeId attributeId, const void *in,
                           const UA_DataType *inDataType,
                           UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32 *reqId);

static UA_INLINE UA_StatusCode
UA_Client_readDataTypeAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                             UA_Variant *outValue, UA_ClientAsyncServiceCallback
                             callback, void *userdata,UA_UInt32 *reqId) {
    return __UA_Client_readAttribute_async(client, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                                     outValue, &UA_TYPES[UA_TYPES_NODEID], callback, userdata, reqId);
}


static UA_INLINE UA_StatusCode
UA_Client_readValueAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                             UA_Variant *outValue,
		UA_ClientAsyncServiceCallback
                             callback, void *userdata,UA_UInt32 *reqId) {
    return __UA_Client_readAttribute_async(client, &nodeId, UA_ATTRIBUTEID_VALUE,
                                     outValue, &UA_TYPES[UA_TYPES_VARIANT], callback, userdata, reqId);
}

static UA_INLINE UA_StatusCode
UA_Client_writeValueAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                              const UA_Variant *newValue, UA_ClientAsyncServiceCallback callback,
                              void *userdata, UA_UInt32 *reqId) {

    return __UA_Client_writeAttribute_async(client, &nodeId, UA_ATTRIBUTEID_VALUE,
                                      newValue, &UA_TYPES[UA_TYPES_VARIANT], callback, userdata, reqId);
}

#endif /* UA_CLIENT_HIGHLEVEL_ASYNC_H_ */
