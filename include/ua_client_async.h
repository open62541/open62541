/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_CLIENT_ASYNC_H_
#define UA_CLIENT_ASYNC_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "ua_client.h"



/**
 * Async Client Services.
*/

/**
 * Attribute Service Set
 * ^^^^^^^^^^^^^^^^^^^^^ */

static UA_INLINE UA_StatusCode
UA_Client_AsyncService_read(UA_Client *client, const UA_ReadRequest request,
    UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32* requestId) {
    return __UA_Client_AsyncService(client, &request,
                        &UA_TYPES[UA_TYPES_READREQUEST], callback,
                        &UA_TYPES[UA_TYPES_READRESPONSE], userdata,    requestId);
}

static UA_INLINE UA_StatusCode
UA_Client_AsyncService_write(UA_Client *client, const UA_WriteRequest request,
    UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32* requestId) {
    return __UA_Client_AsyncService(client, &request,
                        &UA_TYPES[UA_TYPES_WRITEREQUEST], callback,
                        &UA_TYPES[UA_TYPES_WRITERESPONSE], userdata,
                        requestId);
}

/**
 * Method Service Set
 * ^^^^^^^^^^^^^^^^^^ */
#ifdef UA_ENABLE_METHODCALLS
static UA_INLINE UA_StatusCode
UA_Client_AsyncService_call(UA_Client *client, const UA_CallRequest request,
    UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32* requestId) {
    return __UA_Client_AsyncService(client, &request,
                        &UA_TYPES[UA_TYPES_CALLREQUEST], callback,
                        &UA_TYPES[UA_TYPES_CALLRESPONSE], userdata,
                        requestId);
}
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
static UA_INLINE UA_StatusCode
UA_Client_AsyncService_publish(UA_Client *client, const UA_PublishRequest request,
    UA_ClientAsyncServiceCallback callback, void *userdata, UA_UInt32* requestId) {
    return __UA_Client_AsyncService(client, &request,
                        &UA_TYPES[UA_TYPES_PUBLISHREQUEST], callback,
                        &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], userdata,
                        requestId);
}
#endif


#ifdef __cplusplus
} // extern "C"
#endif


#endif /* UA_CLIENT_ASYNC_H_ */
