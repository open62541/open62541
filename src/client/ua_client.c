/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015 (c) hfaham
 *    Copyright 2015-2017 (c) Florian Palm
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2015 (c) Holger Jeromin
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) Lykurg
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include "ua_client_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_util.h"
#include "ua_securitypolicies.h"
#include "ua_pki_certificate.h"

#define STATUS_CODE_BAD_POINTER 0x01

/********************/
/* Client Lifecycle */
/********************/

// TODO: return StatusCode
static void
UA_Client_init(UA_Client* client, UA_ClientConfig config) {
    memset(client, 0, sizeof(UA_Client));

    client->config = config;
    UA_StatusCode retval = config.configureNetworkManager(&client->config, &client->networkManager);
    if(retval != UA_STATUSCODE_GOOD)
        return;

    /* TODO: Select policy according to the endpoint */
    UA_SecurityPolicy_None(&client->securityPolicy, NULL, UA_BYTESTRING_NULL,
                           &client->config.logger);
    UA_SecureChannel_init(&client->channel);
    if(client->config.stateCallback)
        client->config.stateCallback(client, client->state);
    /* Catch error during async connection */
    client->connectStatus = UA_STATUSCODE_GOOD;

    UA_Timer_init(&client->timer);
    UA_WorkQueue_init(&client->workQueue);
}

UA_Client *
UA_Client_new(UA_ClientConfig config) {
    UA_Client *client = (UA_Client*)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;
    UA_Client_init(client, config);
    return client;
}

#ifdef UA_ENABLE_ENCRYPTION
/* Initializes a secure client with the required configuration, certificate
 * privatekey, trustlist and revocation list.
 *
 * @param  client                   client to store configuration
 * @param  config                   new secure configuration for client
 * @param  certificate              client certificate
 * @param  privateKey               client's private key
 * @param  remoteCertificate        server certificate form the endpoints
 * @param  trustList                list of trustable certificate
 * @param  trustListSize            count of trustList
 * @param  revocationList           list of revoked digital certificate
 * @param  revocationListSize       count of revocationList
 * @param  securityPolicyFunction   securityPolicy function
 * @return Returns a client configuration for secure channel */
static UA_StatusCode
UA_Client_secure_init(UA_Client* client, UA_ClientConfig config,
                      const UA_ByteString certificate,
                      const UA_ByteString privateKey,
                      const UA_ByteString *remoteCertificate,
                      const UA_ByteString *trustList, size_t trustListSize,
                      const UA_ByteString *revocationList,
                      size_t revocationListSize,
                      UA_SecurityPolicy_Func securityPolicyFunction) {
    if(client == NULL || remoteCertificate == NULL)
        return STATUS_CODE_BAD_POINTER;

    memset(client, 0, sizeof(UA_Client));

    client->config = config;
    /* Allocate memory for certificate verification */
    client->securityPolicy.certificateVerification =
                           (UA_CertificateVerification *)
                            UA_malloc(sizeof(UA_CertificateVerification));

    UA_StatusCode retval = config.configureNetworkManager(&client->config, &client->networkManager);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_CertificateVerification_Trustlist(client->securityPolicy.certificateVerification,
                                                  trustList, trustListSize,
                                                  revocationList, revocationListSize);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->channel.securityPolicy->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Trust list parsing failed with error %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Initiate client security policy */
    retval = (*securityPolicyFunction)(&client->securityPolicy,
                                       client->securityPolicy.certificateVerification,
                                       certificate, privateKey, &client->config.logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->channel.securityPolicy->logger, UA_LOGCATEGORY_CLIENT,
                     "Failed to setup the SecurityPolicy with error %s", UA_StatusCode_name(retval));
        return retval;
    }

    if(client->config.stateCallback)
        client->config.stateCallback(client, client->state);

    /* Catch error during async connection */
    client->connectStatus = UA_STATUSCODE_GOOD;

    UA_Timer_init(&client->timer);
    UA_WorkQueue_init(&client->workQueue);

    /* Initialize the SecureChannel */
    UA_SecureChannel_init(&client->channel);
    client->channel.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    retval = UA_SecureChannel_setSecurityPolicy(&client->channel, &client->securityPolicy,
                                                remoteCertificate);
    return retval;
}

/* Creates a new secure client.
 *
 * @param  config                   new secure configuration for client
 * @param  certificate              client certificate
 * @param  privateKey               client's private key
 * @param  remoteCertificate        server certificate form the endpoints
 * @param  trustList                list of trustable certificate
 * @param  trustListSize            count of trustList
 * @param  revocationList           list of revoked digital certificate
 * @param  revocationListSize       count of revocationList
 * @param  securityPolicyFunction   securityPolicy function
 * @return Returns a client with secure configuration */
UA_Client *
UA_Client_secure_new(UA_ClientConfig config, UA_ByteString certificate,
                     UA_ByteString privateKey, const UA_ByteString *remoteCertificate,
                     const UA_ByteString *trustList, size_t trustListSize,
                     const UA_ByteString *revocationList, size_t revocationListSize,
                     UA_SecurityPolicy_Func securityPolicyFunction) {
    if(remoteCertificate == NULL)
        return NULL;

    UA_Client *client = (UA_Client *)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;

    UA_StatusCode retval = UA_Client_secure_init(client, config, certificate, privateKey,
                                                 remoteCertificate, trustList, trustListSize,
                                                 revocationList, revocationListSize,
                                                 securityPolicyFunction);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(client);
        return NULL;
    }

    return client;
}
#endif

static void
UA_Client_deleteMembers(UA_Client* client) {
    UA_Client_disconnect(client);
    client->securityPolicy.deleteMembers(&client->securityPolicy);
    /* Commented as UA_SecureChannel_deleteMembers already done
     * in UA_Client_disconnect function */
    //UA_SecureChannel_deleteMembersCleanup(&client->channel);

    client->networkManager.shutdown(&client->networkManager);
    client->networkManager.deleteMembers(&client->networkManager);

    if(client->connection != NULL)
        UA_Connection_close(client->connection);
    if(client->endpointUrl.data)
        UA_String_deleteMembers(&client->endpointUrl);
    UA_UserTokenPolicy_deleteMembers(&client->token);
    UA_NodeId_deleteMembers(&client->authenticationToken);
    if(client->username.data)
        UA_String_deleteMembers(&client->username);
    if(client->password.data)
        UA_String_deleteMembers(&client->password);

    /* Delete the async service calls */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

    /* Delete the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_clean(client);
#endif

    if(client->repeatedCallbackSocket != NULL) {
        client->repeatedCallbackSocket->free(client->repeatedCallbackSocket);
        client->repeatedCallbackSocket = NULL;
    }

    if(client->openRepeatedCallbackId != 0) {
        UA_Client_removeRepeatedCallback(client, client->openRepeatedCallbackId);
        client->openRepeatedCallbackId = 0;
    }

    /* Delete the timed work */
    UA_Timer_deleteMembers(&client->timer);

    /* Clean up the work queue */
    UA_WorkQueue_cleanup(&client->workQueue);
}

void
UA_Client_reset(UA_Client* client) {
    UA_Client_deleteMembers(client);
    UA_Client_init(client, client->config);
}

void
UA_Client_delete(UA_Client* client) {
    /* certificate verification is initialized for secure client
     * which is deallocated */
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if (client->securityPolicy.certificateVerification->deleteMembers)
            client->securityPolicy.certificateVerification->deleteMembers(client->securityPolicy.certificateVerification);
        UA_free(client->securityPolicy.certificateVerification);
    }

    UA_Client_deleteMembers(client);
    UA_free(client);
}

UA_ClientState
UA_Client_getState(UA_Client *client) {
    return client->state;
}

UA_ClientConfig *
UA_Client_getConfig(UA_Client *client) {
    if(!client)
        return NULL;
    return &client->config;
}

/****************/
/* Raw Services */
/****************/

/* For both synchronous and asynchronous service calls */
static UA_StatusCode
sendSymmetricServiceRequest(UA_Client *client, const void *request,
                            const UA_DataType *requestType, UA_UInt32 *requestId) {
    /* Make sure we have a valid session */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* FIXME: this is just a dirty workaround. We need to rework some of the sync and async processing
     * FIXME: in the client. Currently a lot of stuff is semi broken and in dire need of cleaning up.*/
    /*UA_StatusCode retval = openSecureChannel(client, true);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;*/

    /* Adjusting the request header. The const attribute is violated, but we
     * only touch the following members: */
    UA_RequestHeader *rr = (UA_RequestHeader*)(uintptr_t)request;
    rr->authenticationToken = client->authenticationToken; /* cleaned up at the end */
    rr->timestamp = UA_DateTime_now();
    rr->requestHandle = ++client->requestHandle;

    /* Send the request */
    UA_UInt32 rqId = ++client->requestId;
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Sending a request of type %i", requestType->typeId.identifier.numeric);

    if (client->channel.nextSecurityToken.tokenId != 0) // Change to the new security token if the secure channel has been renewed.
        UA_SecureChannel_revolveTokens(&client->channel);
    retval = UA_SecureChannel_sendSymmetricMessage(&client->channel, rqId, UA_MESSAGETYPE_MSG,
                                                   rr, requestType);
    UA_NodeId_init(&rr->authenticationToken); /* Do not return the token to the user */
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    *requestId = rqId;
    return UA_STATUSCODE_GOOD;
}

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType) {
    UA_init(response, responseType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;

    /* Send the request */
    UA_UInt32 requestId;
    UA_StatusCode retval = sendSymmetricServiceRequest(client, request, requestType, &requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            respHeader->serviceResult = UA_STATUSCODE_BADREQUESTTOOLARGE;
        else
            respHeader->serviceResult = retval;
        UA_Client_disconnect(client);
        return;
    }

    /* Retrieve the response */
    UA_DateTime maxDate = UA_DateTime_nowMonotonic() +
        (client->config.timeout * UA_DATETIME_MSEC);
    retval = receiveServiceResponse(client, response, responseType, maxDate, &requestId);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        /* In synchronous service, if we have don't have a reply we need to close the connection */
        UA_Client_disconnect(client);
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;
}

void
UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                              UA_StatusCode statusCode) {
    /* Create an empty response with the statuscode */
    UA_STACKARRAY(UA_Byte, responseBuf, ac->responseType->memSize);
    void *resp = (void*)(uintptr_t)&responseBuf[0]; /* workaround aliasing rules */
    UA_init(resp, ac->responseType);
    ((UA_ResponseHeader*)resp)->serviceResult = statusCode;

    if(ac->callback)
        ac->callback(client, ac->userdata, ac->requestId, resp);

    /* Clean up the response. Users might move data into it. For whatever reasons. */
    UA_deleteMembers(resp, ac->responseType);
}

void UA_Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode) {
    AsyncServiceCall *ac, *ac_tmp;
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        LIST_REMOVE(ac, pointers);
        UA_Client_AsyncService_cancel(client, ac, statusCode);
        UA_free(ac);
    }
}

UA_StatusCode
__UA_Client_AsyncServiceEx(UA_Client *client, const void *request,
                           const UA_DataType *requestType,
                           UA_ClientAsyncServiceCallback callback,
                           const UA_DataType *responseType,
                           void *userdata, UA_UInt32 *requestId,
                           UA_UInt32 timeout) {
    /* Prepare the entry for the linked list */
    AsyncServiceCall *ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
    if(!ac)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ac->callback = callback;
    ac->responseType = responseType;
    ac->userdata = userdata;
    ac->timeout = timeout;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendSymmetricServiceRequest(client, request, requestType, &ac->requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ac);
        return retval;
    }

    ac->start = UA_DateTime_nowMonotonic();

    /* Store the entry for async processing */
    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
    if(requestId)
        *requestId = ac->requestId;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_AsyncServiceEx(client, request, requestType, callback,
                                      responseType, userdata, requestId,
                                      client->config.timeout);
}


UA_StatusCode
UA_Client_sendAsyncRequest(UA_Client *client, const void *request,
                           const UA_DataType *requestType,
                           UA_ClientAsyncServiceCallback callback,
                           const UA_DataType *responseType, void *userdata,
                           UA_UInt32 *requestId) {
    if (UA_Client_getState(client) < UA_CLIENTSTATE_SECURECHANNEL) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Cient must be connected to send high-level requests");
        return UA_STATUSCODE_GOOD;
    }
    return __UA_Client_AsyncService(client, request, requestType, callback,
                                    responseType, userdata, requestId);
}

UA_StatusCode UA_EXPORT
UA_Client_addTimedCallback(UA_Client *client, UA_ClientCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    return UA_Timer_addTimedCallback(&client->timer, (UA_ApplicationCallback) callback,
                                     client, data, date, callbackId);
}

UA_StatusCode
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&client->timer, (UA_ApplicationCallback) callback,
                                        client, data, interval_ms, callbackId);
}

UA_StatusCode
UA_Client_changeRepeatedCallbackInterval(UA_Client *client, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    return UA_Timer_changeRepeatedCallbackInterval(&client->timer, callbackId,
                                                   interval_ms);
}

void
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&client->timer, callbackId);
}
