/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017-2019 (c) Fraunhofer IOSB (Author: Mark Giraud)
 */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>

#include "ua_util_internal.h"

#include "ua_client_internal.h"

#define UA_MINMESSAGESIZE 8192
#define UA_SESSION_LOCALNONCELENGTH 32
#define MAX_DATA_SIZE 4096

static void closeSession(UA_Client *client);
static UA_StatusCode createSessionAsync(UA_Client *client);

static UA_SecurityPolicy *
getSecurityPolicy(UA_Client *client, UA_String policyUri) {
    for(size_t i = 0; i < client->config.securityPoliciesSize; i++) {
        if(UA_String_equal(&policyUri, &client->config.securityPolicies[i].policyUri))
            return &client->config.securityPolicies[i];
    }
    return NULL;
}

static UA_Boolean
endpointUnconfigured(UA_Client *client) {
    char test = 0;
    char *pos = (char *)&client->config.endpoint;
    for(size_t i = 0; i < sizeof(UA_EndpointDescription); i++)
        test = test | *(pos + i);
    pos = (char *)&client->config.userTokenPolicy;
    for(size_t i = 0; i < sizeof(UA_UserTokenPolicy); i++)
        test = test | *(pos + i);
    return (test == 0);
}

#ifdef UA_ENABLE_ENCRYPTION

/* Function to create a signature using remote certificate and nonce */
static UA_StatusCode
signActivateSessionRequest(UA_Client *client, UA_SecureChannel *channel,
                           UA_ActivateSessionRequest *request) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_SignatureData *sd = &request->clientSignature;

    /* Prepare the signature */
    size_t signatureSize = sp->certificateSigningAlgorithm.
        getLocalSignatureSize(channel->channelContext);
    UA_StatusCode retval = UA_String_copy(&sp->certificateSigningAlgorithm.uri,
                                          &sd->algorithm);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ByteString_allocBuffer(&sd->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temporary buffer */
    size_t dataToSignSize = channel->remoteCertificate.length + client->remoteNonce.length;
    if(dataToSignSize > MAX_DATA_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString dataToSign;
    retval = UA_ByteString_allocBuffer(&dataToSign, dataToSignSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval; /* sd->signature is cleaned up with the response */

    /* Sign the signature */
    memcpy(dataToSign.data, channel->remoteCertificate.data,
           channel->remoteCertificate.length);
    memcpy(dataToSign.data + channel->remoteCertificate.length,
           client->remoteNonce.data, client->remoteNonce.length);
    retval = sp->certificateSigningAlgorithm.sign(channel->channelContext,
                                                  &dataToSign, &sd->signature);

    /* Clean up */
    UA_ByteString_clear(&dataToSign);
    return retval;
}

static UA_StatusCode
encryptUserIdentityToken(UA_Client *client, const UA_String *userTokenSecurityPolicy,
                         UA_ExtensionObject *userIdentityToken) {
    UA_IssuedIdentityToken *iit = NULL;
    UA_UserNameIdentityToken *unit = NULL;
    UA_ByteString *tokenData;
    if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]) {
        iit = (UA_IssuedIdentityToken*)userIdentityToken->content.decoded.data;
        tokenData = &iit->tokenData;
    } else if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        unit = (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;
        tokenData = &unit->password;
    } else {
        return UA_STATUSCODE_GOOD;
    }

    /* No encryption */
    const UA_String none = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    if(userTokenSecurityPolicy->length == 0 ||
       UA_String_equal(userTokenSecurityPolicy, &none)) {
        return UA_STATUSCODE_GOOD;
    }

    UA_SecurityPolicy *sp = getSecurityPolicy(client, *userTokenSecurityPolicy);
    if(!sp) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Could not find the required SecurityPolicy for the UserToken");
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    }

    /* Create a temp channel context */

    void *channelContext;
    UA_StatusCode retval = sp->channelModule.
        newContext(sp, &client->config.endpoint.serverCertificate, &channelContext);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Could not instantiate the SecurityPolicy for the UserToken");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Compute the encrypted length (at least one byte padding) */
    size_t plainTextBlockSize = sp->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemotePlainTextBlockSize(channelContext);
    size_t encryptedBlockSize = sp->asymmetricModule.cryptoModule.
        encryptionAlgorithm.getRemoteBlockSize(channelContext);
    UA_UInt32 length = (UA_UInt32)(tokenData->length + client->remoteNonce.length);
    UA_UInt32 totalLength = length + 4; /* Including the length field */
    size_t blocks = totalLength / plainTextBlockSize;
    if(totalLength % plainTextBlockSize != 0)
        blocks++;
    size_t encryptedLength = blocks * encryptedBlockSize;

    /* Allocate memory for encryption overhead */
    UA_ByteString encrypted;
    retval = UA_ByteString_allocBuffer(&encrypted, encryptedLength);
    if(retval != UA_STATUSCODE_GOOD) {
        sp->channelModule.deleteContext(channelContext);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Byte *pos = encrypted.data;
    const UA_Byte *end = &encrypted.data[encrypted.length];
    UA_UInt32_encodeBinary(&length, &pos, end);
    memcpy(pos, tokenData->data, tokenData->length);
    memcpy(&pos[tokenData->length], client->remoteNonce.data, client->remoteNonce.length);

    /* Add padding
     *
     * 7.36.2.2 Legacy Encrypted Token Secret Format: A Client should not add any
     * padding after the secret. If a Client adds padding then all bytes shall
     * be zero. A Server shall check for padding added by Clients and ensure
     * that all padding bytes are zeros. */
    size_t paddedLength = plainTextBlockSize * blocks;
    for(size_t i = totalLength; i < paddedLength; i++)
        encrypted.data[i] = 0;
    encrypted.length = paddedLength;

    retval = sp->asymmetricModule.cryptoModule.encryptionAlgorithm.
        encrypt(channelContext, &encrypted);
    encrypted.length = encryptedLength;

    if(iit) {
        retval |= UA_String_copy(&sp->asymmetricModule.cryptoModule.encryptionAlgorithm.uri,
                                 &iit->encryptionAlgorithm);
    } else {
        retval |= UA_String_copy(&sp->asymmetricModule.cryptoModule.encryptionAlgorithm.uri,
                                 &unit->encryptionAlgorithm);
    }

    UA_ByteString_clear(tokenData);
    *tokenData = encrypted;

    /* Delete the temp channel context */
    sp->channelModule.deleteContext(channelContext);

    return retval;
}

/* Function to verify the signature corresponds to ClientNonce
 * using the local certificate */
static UA_StatusCode
checkCreateSessionSignature(UA_Client *client, const UA_SecureChannel *channel,
                            const UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    if(!channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    const UA_ByteString *lc = &sp->localCertificate;

    size_t dataToVerifySize = lc->length + client->localNonce.length;
    UA_ByteString dataToVerify = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, lc->data, lc->length);
    memcpy(dataToVerify.data + lc->length,
           client->localNonce.data, client->localNonce.length);

    retval = sp->certificateSigningAlgorithm.verify(channel->channelContext, &dataToVerify,
                                                    &response->serverSignature.signature);
    UA_ByteString_clear(&dataToVerify);
    return retval;
}

#endif

/***********************/
/* Open the Connection */
/***********************/

void
processERRResponse(UA_Client *client, const UA_ByteString *chunk) {
    client->channel.state = UA_SECURECHANNELSTATE_CLOSING;

    size_t offset = 0;
    UA_TcpErrorMessage errMessage;
    UA_StatusCode res = UA_TcpErrorMessage_decodeBinary(chunk, &offset, &errMessage);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CHANNEL(&client->config.logger, &client->channel,
                             "Received an ERR response that could not be decoded with StatusCode %s",
                             UA_StatusCode_name(res));
        client->connectStatus = res;
        return;
    }

    UA_LOG_ERROR_CHANNEL(&client->config.logger, &client->channel,
                         "Received an ERR response with StatusCode %s and the following reason: %.*s",
                         UA_StatusCode_name(errMessage.error), (int)errMessage.reason.length, errMessage.reason.data);
    client->connectStatus = errMessage.error;
    UA_TcpErrorMessage_clear(&errMessage);
}

void
processACKResponse(UA_Client *client, const UA_ByteString *chunk) {
    UA_SecureChannel *channel = &client->channel;
    if(channel->state != UA_SECURECHANNELSTATE_HEL_SENT) {
        UA_LOG_ERROR_CHANNEL(&client->config.logger, channel,
                             "Expected an ACK response");
        channel->state = UA_SECURECHANNELSTATE_CLOSING;
        return;
    }

    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");

    /* Decode the message */
    size_t offset = 0;
    UA_TcpAcknowledgeMessage ackMessage;
    client->connectStatus = UA_TcpAcknowledgeMessage_decodeBinary(chunk, &offset, &ackMessage);
    if(client->connectStatus != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Decoding ACK message failed");
        closeSecureChannel(client);
        return;
    }

    client->connectStatus =
        UA_SecureChannel_processHELACK(channel, &ackMessage);
    if(client->connectStatus != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Processing the ACK message failed with StatusCode %s",
                     UA_StatusCode_name(client->connectStatus));
        closeSecureChannel(client);
        return;
    }

    client->channel.state = UA_SECURECHANNELSTATE_ACK_RECEIVED;
}

static UA_StatusCode
sendHELMessage(UA_Client *client) {
    /* Get a buffer */
    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    UA_StatusCode retval = conn->getSendBuffer(conn, UA_MINMESSAGESIZE, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Prepare the HEL message and encode at offset 8 */
    UA_TcpHelloMessage hello;
    hello.protocolVersion = 0;
    hello.receiveBufferSize = client->config.localConnectionConfig.recvBufferSize;
    hello.sendBufferSize = client->config.localConnectionConfig.sendBufferSize;
    hello.maxMessageSize = client->config.localConnectionConfig.localMaxMessageSize;
    hello.maxChunkCount = client->config.localConnectionConfig.localMaxChunkCount;
    hello.endpointUrl = client->endpointUrl;

    UA_Byte *bufPos = &message.data[8]; /* skip the header */
    const UA_Byte *bufEnd = &message.data[message.length];
    client->connectStatus = UA_TcpHelloMessage_encodeBinary(&hello, &bufPos, bufEnd);

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_HEL;
    messageHeader.messageSize = (UA_UInt32) ((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval = UA_TcpMessageHeader_encodeBinary(&messageHeader, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return retval;
    }

    /* Send the HEL message */
    message.length = messageHeader.messageSize;
    retval = conn->send(conn, &message);
    if(retval == UA_STATUSCODE_GOOD) {
        client->channel.state = UA_SECURECHANNELSTATE_HEL_SENT;
        UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Sent HEL message");
    } else {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Sending HEL failed");
    }
    return retval;
}

void
processOPNResponse(UA_Client *client, const UA_ByteString *message) {
    /* Is the content of the expected type? */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_NodeId expectedId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE_ENCODING_DEFAULTBINARY);
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD) {
        closeSecureChannel(client);
        return;
    }

    if(!UA_NodeId_equal(&responseId, &expectedId)) {
        UA_NodeId_clear(&responseId);
        closeSecureChannel(client);
        return;
    }

    /* Decode the response */
    UA_OpenSecureChannelResponse response;
    retval = UA_OpenSecureChannelResponse_decodeBinary(message, &offset, &response);
    if(retval != UA_STATUSCODE_GOOD) {
        closeSecureChannel(client);
        return;
    }

    /* Check whether the nonce was reused */
    if(client->channel.securityMode != UA_MESSAGESECURITYMODE_NONE &&
       UA_ByteString_equal(&client->channel.remoteNonce,
                           &response.serverNonce)) {
        UA_LOG_ERROR_CHANNEL(&client->config.logger, &client->channel,
                             "The server reused the last nonce");
        client->connectStatus = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        closeSecureChannel(client);
        return;
    }

    /* Response.securityToken.revisedLifetime is UInt32 we need to cast it to
     * DateTime=Int64 we take 75% of lifetime to start renewing as described in
     * standard */
    client->nextChannelRenewal = UA_DateTime_nowMonotonic()
            + (UA_DateTime) (response.securityToken.revisedLifetime
                    * (UA_Double) UA_DATETIME_MSEC * 0.75);

    /* Move the nonce out of the response */
    UA_ByteString_clear(&client->channel.remoteNonce);
    client->channel.remoteNonce = response.serverNonce;
    UA_ByteString_init(&response.serverNonce);
    UA_ResponseHeader_clear(&response.responseHeader);

    /* Replace the token. Keep the current token as the old token. Messages
     * might still arrive for the old token. */
    client->channel.altSecurityToken = client->channel.securityToken;
    client->channel.securityToken = response.securityToken;
    client->channel.renewState = UA_SECURECHANNELRENEWSTATE_NEWTOKEN_CLIENT;

    /* Compute the new local keys. The remote keys are updated when a message
     * with the new SecurityToken is received. */
    retval = UA_SecureChannel_generateLocalKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD) {
        closeSecureChannel(client);
        return;
    }

    UA_Float lifetime = (UA_Float)response.securityToken.revisedLifetime / 1000;
    UA_Boolean renew = (client->channel.state == UA_SECURECHANNELSTATE_OPEN);
    if(renew) {
        UA_LOG_INFO_CHANNEL(&client->config.logger, &client->channel, "SecureChannel "
                            "renewed with a revised lifetime of %.2fs", lifetime);
    } else {
        UA_LOG_INFO_CHANNEL(&client->config.logger, &client->channel,
                            "SecureChannel opened with SecurityPolicy %.*s "
                            "and a revised lifetime of %.2fs",
                            (int)client->channel.securityPolicy->policyUri.length,
                            client->channel.securityPolicy->policyUri.data, lifetime);
    }

    client->channel.state = UA_SECURECHANNELSTATE_OPEN;
}

/* OPN messges to renew the channel are sent asynchronous */
static UA_StatusCode
sendOPNAsync(UA_Client *client, UA_Boolean renew) {
    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTIONSTATE_ESTABLISHED) {
        closeSecureChannel(client);
        return UA_STATUSCODE_BADNOTCONNECTED;
    }

    UA_StatusCode retval = UA_SecureChannel_generateLocalNonce(&client->channel);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Prepare the OpenSecureChannelRequest */
    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    opnSecRq.securityMode = client->channel.securityMode;
    opnSecRq.clientNonce = client->channel.localNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;
    if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG_CHANNEL(&client->config.logger, &client->channel,
                             "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG_CHANNEL(&client->config.logger, &client->channel,
                             "Requesting to open a SecureChannel");
    }

    /* Prepare the entry for the linked list */
    UA_UInt32 requestId = ++client->requestId;

    /* Send the OPN message */
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                 "Requesting to open a SecureChannel");
    retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&client->channel, requestId, &opnSecRq,
                                                  &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST]);
    if(retval != UA_STATUSCODE_GOOD) {
        client->connectStatus = retval;
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                      "Sending OPN message failed with error %s",
                      UA_StatusCode_name(retval));
        closeSecureChannel(client);
        return retval;
    }

    client->channel.renewState = UA_SECURECHANNELRENEWSTATE_SENT;
    if(client->channel.state < UA_SECURECHANNELSTATE_OPN_SENT)
        client->channel.state = UA_SECURECHANNELSTATE_OPN_SENT;
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "OPN message sent");
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_renewSecureChannel(UA_Client *client) {
    /* Check if OPN has been sent or the SecureChannel is still valid */
    if(client->channel.state != UA_SECURECHANNELSTATE_OPEN ||
       client->channel.renewState == UA_SECURECHANNELRENEWSTATE_SENT ||
       client->nextChannelRenewal > UA_DateTime_nowMonotonic())
        return UA_STATUSCODE_GOODCALLAGAIN;
    sendOPNAsync(client, true);
    return client->connectStatus;
}

static void
responseActivateSession(UA_Client *client, void *userdata, UA_UInt32 requestId,
                        void *response) {
    UA_ActivateSessionResponse *activateResponse =
            (UA_ActivateSessionResponse *)response;
    if(activateResponse->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "ActivateSession failed with error code %s",
                     UA_StatusCode_name(activateResponse->responseHeader.serviceResult));
        if(activateResponse->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONIDINVALID ||
           activateResponse->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONCLOSED) {
            /* The session is lost. Create a new one. */
            closeSession(client);
            createSessionAsync(client);
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Session cannot be activated. Create a new Session.");
        } else {
            /* Something else is wrong. Give up. */
            client->connectStatus = activateResponse->responseHeader.serviceResult;
        }
        return;
    }

    client->sessionState = UA_SESSIONSTATE_ACTIVATED;
    notifyClientState(client);
}

static UA_StatusCode
activateSessionAsync(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now ();
    request.requestHeader.timeoutHint = 600000;
    UA_StatusCode retval =
        UA_ExtensionObject_copy(&client->config.userIdentityToken, &request.userIdentityToken);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if (client->config.sessionLocaleIdsSize && client->config.sessionLocaleIds) {
        retval = UA_Array_copy(client->config.sessionLocaleIds, client->config.sessionLocaleIdsSize,
                               (void **)&request.localeIds, &UA_TYPES[UA_TYPES_LOCALEID]);
        if (retval != UA_STATUSCODE_GOOD)
            return retval;

        request.localeIdsSize = client->config.sessionLocaleIdsSize;
    }

    /* If not token is set, use anonymous */
    if(request.userIdentityToken.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        UA_AnonymousIdentityToken *t = UA_AnonymousIdentityToken_new();
        if(!t) {
            UA_ActivateSessionRequest_clear(&request);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        request.userIdentityToken.content.decoded.data = t;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    }

    /* Set the policy-Id from the endpoint. Every IdentityToken starts with a
     * string. */
    retval = UA_String_copy(&client->config.userTokenPolicy.policyId,
                            (UA_String*)request.userIdentityToken.content.decoded.data);

#ifdef UA_ENABLE_ENCRYPTION
    /* Encrypt the UserIdentityToken */
    const UA_String *userTokenPolicy = &client->channel.securityPolicy->policyUri;
    if(client->config.userTokenPolicy.securityPolicyUri.length > 0)
        userTokenPolicy = &client->config.userTokenPolicy.securityPolicyUri;
    retval |= encryptUserIdentityToken(client, userTokenPolicy, &request.userIdentityToken);
    retval |= signActivateSessionRequest(client, &client->channel, &request);
#endif

    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_Client_sendAsyncRequest(client, &request,
                                            &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                                            (UA_ClientAsyncServiceCallback) responseActivateSession,
                                            &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL, NULL);

    UA_ActivateSessionRequest_clear(&request);
    if(retval == UA_STATUSCODE_GOOD)
        client->sessionState = UA_SESSIONSTATE_ACTIVATE_REQUESTED;
    else
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "ActivateSession failed when sending the request with error code %s",
                           UA_StatusCode_name(retval));

    return retval;
}

/* Combination of UA_Client_getEndpointsInternal and getEndpoints */
static void
responseGetEndpoints(UA_Client *client, void *userdata, UA_UInt32 requestId,
                     void *response) {
    client->endpointsHandshake = false;

    UA_GetEndpointsResponse *resp = (UA_GetEndpointsResponse*)response;
    /* GetEndpoints not possible. Fail the connection */
    if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        client->connectStatus = resp->responseHeader.serviceResult;
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "GetEndpointRequest failed with error code %s",
                     UA_StatusCode_name(client->connectStatus));
        UA_GetEndpointsResponse_clear(resp);
        return;
    }

    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    const UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                                "Transport/uatcp-uasc-uabinary");

    // TODO: compare endpoint information with client->endpointUri
    UA_EndpointDescription* endpointArray = resp->endpoints;
    size_t endpointArraySize = resp->endpointsSize;
    for(size_t i = 0; i < endpointArraySize; ++i) {
        UA_EndpointDescription* endpoint = &endpointArray[i];
        /* Look out for binary transport endpoints.
         * Note: Siemens returns empty ProfileUrl, we will accept it as binary. */
        if(endpoint->transportProfileUri.length != 0 &&
           !UA_String_equal (&endpoint->transportProfileUri, &binaryTransport))
            continue;

        /* Valid SecurityMode? */
        if(endpoint->securityMode < 1 || endpoint->securityMode > 3) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Rejecting endpoint %lu: invalid security mode",
                        (long unsigned)i);
            continue;
        }

        /* Selected SecurityMode? */
        if(client->config.securityMode > 0 &&
           client->config.securityMode != endpoint->securityMode) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Rejecting endpoint %lu: security mode doesn't match",
                        (long unsigned)i);
            continue;
        }

        /* Matching SecurityPolicy? */
        if(client->config.securityPolicyUri.length > 0 &&
           !UA_String_equal(&client->config.securityPolicyUri,
                            &endpoint->securityPolicyUri)) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Rejecting endpoint %lu: security policy doesn't match",
                        (long unsigned)i);
            continue;
        }

        /* SecurityPolicy available? */
        if(!getSecurityPolicy(client, endpoint->securityPolicyUri)) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Rejecting endpoint %lu: security policy not available",
                        (long unsigned)i);
            continue;
        }

        endpointFound = true;

        /* Look for a user token policy with an anonymous token */
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* tokenPolicy = &endpoint->userIdentityTokens[j];
            const UA_DataType *tokenType = client->config.userIdentityToken.content.decoded.type;

            /* Usertokens also have a security policy... */
            if(tokenPolicy->tokenType != UA_USERTOKENTYPE_ANONYMOUS && 
               tokenPolicy->securityPolicyUri.length > 0 &&
               !getSecurityPolicy(client, tokenPolicy->securityPolicyUri)) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                            "Rejecting UserTokenPolicy %lu in endpoint %lu: "
                            "security policy '%.*s' not available", (long unsigned)j, (long unsigned)i,
                            (int)tokenPolicy->securityPolicyUri.length,
                            tokenPolicy->securityPolicyUri.data);
                continue;
            }

            if(tokenPolicy->tokenType > 3) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                            "Rejecting UserTokenPolicy %lu in endpoint %lu: invalid token type",
                            (long unsigned)j, (long unsigned)i);
                continue;
            }

            if(tokenPolicy->tokenType == UA_USERTOKENTYPE_ANONYMOUS &&
               tokenType != &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN] &&
               tokenType != NULL) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                            "Rejecting UserTokenPolicy %lu (anonymous) in endpoint %lu: "
                            "configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }
            if(tokenPolicy->tokenType == UA_USERTOKENTYPE_USERNAME &&
               tokenType != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                            "Rejecting UserTokenPolicy %lu (username) in endpoint %lu: "
                            "configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }
            if(tokenPolicy->tokenType == UA_USERTOKENTYPE_CERTIFICATE &&
               tokenType != &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                            "Rejecting UserTokenPolicy %lu (certificate) in endpoint %lu: "
                            "configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }
            if(tokenPolicy->tokenType == UA_USERTOKENTYPE_ISSUEDTOKEN &&
               tokenType != &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]) {
                UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                            "Rejecting UserTokenPolicy %lu (token) in endpoint %lu: "
                            "configuration doesn't match", (long unsigned)j, (long unsigned)i);
                continue;
            }

            /* Endpoint with matching usertokenpolicy found */

#if UA_LOGLEVEL <= 300
            const char *securityModeNames[3] = {"None", "Sign", "SignAndEncrypt"};
            const char *userTokenTypeNames[4] = {"Anonymous", "UserName",
                                                 "Certificate", "IssuedToken"};
            UA_String *securityPolicyUri = &tokenPolicy->securityPolicyUri;
            if(securityPolicyUri->length == 0)
                securityPolicyUri = &endpoint->securityPolicyUri;

            /* Log the selected endpoint */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Selected endpoint %lu in URL %.*s with SecurityMode %s and SecurityPolicy %.*s",
                        (long unsigned)i, (int)endpoint->endpointUrl.length, endpoint->endpointUrl.data,
                        securityModeNames[endpoint->securityMode - 1],
                        (int)endpoint->securityPolicyUri.length,
                        endpoint->securityPolicyUri.data);

            /* Log the selected UserTokenPolicy */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Selected UserTokenPolicy %.*s with UserTokenType %s and SecurityPolicy %.*s",
                        (int)tokenPolicy->policyId.length, tokenPolicy->policyId.data,
                        userTokenTypeNames[tokenPolicy->tokenType],
                        (int)securityPolicyUri->length, securityPolicyUri->data);
#endif

            /* Move to the client config */
            tokenFound = true;
            UA_EndpointDescription_clear(&client->config.endpoint);
            client->config.endpoint = *endpoint;
            UA_EndpointDescription_init(endpoint);
            UA_UserTokenPolicy_clear(&client->config.userTokenPolicy);
            client->config.userTokenPolicy = *tokenPolicy;
            UA_UserTokenPolicy_init(tokenPolicy);

            break;
        }

        if(tokenFound)
            break;
    }

    if(!endpointFound) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable endpoint found");
        client->connectStatus = UA_STATUSCODE_BADINTERNALERROR;
    } else if(!tokenFound) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable UserTokenPolicy found for the possible endpoints");
        client->connectStatus = UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Close the SecureChannel if a different SecurityPolicy is defined by the Endpoint */
    if(client->config.endpoint.securityMode != client->channel.securityMode ||
       !UA_String_equal(&client->config.endpoint.securityPolicyUri,
                        &client->channel.securityPolicy->policyUri))
        closeSecureChannel(client);
}

static UA_StatusCode
requestGetEndpoints(UA_Client *client) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.endpointUrl = client->endpointUrl;
    UA_StatusCode retval =
        UA_Client_sendAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                                   (UA_ClientAsyncServiceCallback) responseGetEndpoints,
                                   &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE], NULL, NULL);
    if(retval == UA_STATUSCODE_GOOD)
        client->endpointsHandshake = true;
    else
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "RequestGetEndpoints failed when sending the request with error code %s",
                           UA_StatusCode_name(retval));
    return retval;
}

static void
responseSessionCallback(UA_Client *client, void *userdata,
                        UA_UInt32 requestId, void *response) {
    UA_CreateSessionResponse *sessionResponse = (UA_CreateSessionResponse*)response;
    UA_StatusCode res = sessionResponse->responseHeader.serviceResult;
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

#ifdef UA_ENABLE_ENCRYPTION
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* Verify the session response was created with the same certificate as
         * the SecureChannel */
        if(!UA_ByteString_equal(&sessionResponse->serverCertificate,
                                &client->channel.remoteCertificate)) {
            res = UA_STATUSCODE_BADCERTIFICATEINVALID;
            goto cleanup;
        }

        /* Verify the client signature */
        res = checkCreateSessionSignature(client, &client->channel, sessionResponse);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }
#endif
    
    /* Copy nonce and AuthenticationToken */
    UA_ByteString_clear(&client->remoteNonce);
    UA_NodeId_clear(&client->authenticationToken);
    res |= UA_ByteString_copy(&sessionResponse->serverNonce, &client->remoteNonce);
    res |= UA_NodeId_copy(&sessionResponse->authenticationToken, &client->authenticationToken);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Activate the new Session */
    client->sessionState = UA_SESSIONSTATE_CREATED;

 cleanup:
    client->connectStatus = res;
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        client->sessionState = UA_SESSIONSTATE_CLOSED;
}

static UA_StatusCode
createSessionAsync(UA_Client *client) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(client->localNonce.length != UA_SESSION_LOCALNONCELENGTH) {
           UA_ByteString_clear(&client->localNonce);
            retval = UA_ByteString_allocBuffer(&client->localNonce,
                                               UA_SESSION_LOCALNONCELENGTH);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
        retval = client->channel.securityPolicy->symmetricModule.
                 generateNonce(client->channel.securityPolicy->policyContext,
                               &client->localNonce);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->localNonce, &request.clientNonce);
    request.requestedSessionTimeout = client->config.requestedSessionTimeout;
    request.maxResponseMessageSize = UA_INT32_MAX;
    UA_String_copy(&client->config.endpoint.endpointUrl, &request.endpointUrl);
    UA_ApplicationDescription_copy(&client->config.clientDescription,
                                   &request.clientDescription);

    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_ByteString_copy(&client->channel.securityPolicy->localCertificate,
                           &request.clientCertificate);
    }

    retval = UA_Client_sendAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                                        (UA_ClientAsyncServiceCallback)responseSessionCallback,
                                        &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL, NULL);
    UA_CreateSessionRequest_clear(&request);

    if(retval == UA_STATUSCODE_GOOD)
        client->sessionState = UA_SESSIONSTATE_CREATE_REQUESTED;
    else
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "CreateSession failed when sending the request with error code %s",
                           UA_StatusCode_name(retval));

    return retval;
}

static UA_StatusCode
initConnect(UA_Client *client);
static UA_StatusCode
UA_Client_make_connection(UA_Client *client);

UA_StatusCode
initConnection(uintptr_t connectionId, void **connectionContext,
                          UA_BasicClientConnectionContext *ctx);
UA_StatusCode
connectionCallbackSend(UA_ClientConnectionContext *ctx);
UA_StatusCode
connectionCallbackReceive(UA_ClientConnectionContext *ctx, UA_ByteString msg);

UA_StatusCode
connectIterate(UA_Client *client, UA_UInt32 timeout) {
    UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Client connect iterate");

    /* Already connected */
    if(client->sessionState == UA_SESSIONSTATE_ACTIVATED)
        return UA_STATUSCODE_GOOD;

    /* Could not connect */
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        return client->connectStatus;

    /* The connection was already closed */
    if(client->channel.state == UA_SECURECHANNELSTATE_CLOSING) {
        client->connectStatus = UA_STATUSCODE_BADCONNECTIONCLOSED;
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* The connection is closed. Reset the SecureChannel and open a new TCP
     * connection TODO: do connection creation via cm */
    if(client->connection.state == UA_CONNECTIONSTATE_CLOSED)
        return UA_Client_make_connection(client);

    /* Poll the connection status */
    if(client->connection.state == UA_CONNECTIONSTATE_OPENING) {
        client->connectStatus =
            client->config.pollConnectionFunc(&client->connection, timeout,
                                              &client->config.logger);
        return client->connectStatus;
    }

    /* Attach the connection to the SecureChannel */
    if(!client->channel.connection)
        UA_Connection_attachSecureChannel(&client->connection, &client->channel);

    /* Set the SecurityPolicy */
    if(!client->channel.securityPolicy) {
        client->channel.securityMode = client->config.endpoint.securityMode;
        if(client->channel.securityMode == UA_MESSAGESECURITYMODE_INVALID)
            client->channel.securityMode = UA_MESSAGESECURITYMODE_NONE;

        UA_SecurityPolicy *sp = NULL;
        if(client->config.endpoint.securityPolicyUri.length == 0) {
            sp = getSecurityPolicy(client,
                                   UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None"));
        } else {
            sp = getSecurityPolicy(client, client->config.endpoint.securityPolicyUri);
        }
        if(!sp) {
            client->connectStatus = UA_STATUSCODE_BADINTERNALERROR;
            return client->connectStatus;
        }

        client->connectStatus =
            UA_SecureChannel_setSecurityPolicy(&client->channel, sp,
                                               &client->config.endpoint.serverCertificate);
        if(client->connectStatus != UA_STATUSCODE_GOOD)
            return client->connectStatus;
    }

    /* Open the SecureChannel */
    switch(client->channel.state) {
    case UA_SECURECHANNELSTATE_FRESH:
        client->connectStatus = sendHELMessage(client);
        if(client->connectStatus == UA_STATUSCODE_GOOD) {
            client->channel.state = UA_SECURECHANNELSTATE_HEL_SENT;
        } else {
            client->connection.close(&client->connection);
            client->connection.free(&client->connection);
        }
        return client->connectStatus;
    case UA_SECURECHANNELSTATE_ACK_RECEIVED:
        client->connectStatus = sendOPNAsync(client, false);
        return client->connectStatus;
    default:
        break;
    }

    /* Have a SecureChannel but no session */
    if(client->noSession)
        return client->connectStatus;

    /* Create and Activate the Session */
    switch(client->sessionState) {
    case UA_SESSIONSTATE_CLOSED:
        if(!endpointUnconfigured(client)) {
            client->connectStatus = createSessionAsync(client);
            return client->connectStatus;
        }
        if(!client->endpointsHandshake) {
            client->connectStatus = requestGetEndpoints(client);
            return client->connectStatus;
        }
        receiveResponseAsync(client, timeout);
        return client->connectStatus;
    case UA_SESSIONSTATE_CREATE_REQUESTED:
    case UA_SESSIONSTATE_ACTIVATE_REQUESTED:
        receiveResponseAsync(client, timeout);
        return client->connectStatus;
    case UA_SESSIONSTATE_CREATED:
        client->connectStatus = activateSessionAsync(client);
        return client->connectStatus;
    default:
        break;
    }

    return client->connectStatus;
}

/* The local ApplicationURI has to match the certificates of the
 * SecurityPolicies */
static void
verifyClientApplicationURI(const UA_Client *client) {
#if defined(UA_ENABLE_ENCRYPTION) && (UA_LOGLEVEL <= 400)
    for(size_t i = 0; i < client->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &client->config.securityPolicies[i];
        UA_StatusCode retval =
            client->config.certificateVerification.
            verifyApplicationURI(client->config.certificateVerification.context,
                                 &sp->localCertificate,
                                 &client->config.clientDescription.applicationUri);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "The configured ApplicationURI does not match the URI "
                           "specified in the certificate for the SecurityPolicy %.*s",
                           (int)sp->policyUri.length, sp->policyUri.data);
        }
    }
#endif
}

static UA_StatusCode
client_configure_securechannel(void *application, UA_SecureChannel *channel,
                               const UA_AsymmetricAlgorithmSecurityHeader *asymHeader) {
    // TODO: Verify if certificate is the same as configured in the client endpoint config
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
initConnect(UA_Client *client) {
    if(client->connection.state > UA_CONNECTIONSTATE_CLOSED) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Client already connected");
        return UA_STATUSCODE_GOOD;
    }

    if(client->config.initConnectionFunc == NULL) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Client connection not configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Consistency check the client's own ApplicationURI */
    verifyClientApplicationURI(client);

    /* Reset the connect status */
    client->connectStatus = UA_STATUSCODE_GOOD;
    client->channel.renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
    client->endpointsHandshake = false;

    /* Initialize the SecureChannel */
    UA_SecureChannel_init(&client->channel, &client->config.localConnectionConfig);
    client->channel.certificateVerification = &client->config.certificateVerification;
    client->channel.processOPNHeader = client_configure_securechannel;

    // if(client->connection.free)
    //     client->connection.free(&client->connection);

    /* Initialize the TCP connection */
    // client->connection =
    //     client->config.initConnectionFunc(client->config.localConnectionConfig,
    //                                       client->endpointUrl, client->config.timeout,
    //                                       &client->config.logger);
    // if(client->connection.state != UA_CONNECTIONSTATE_OPENING) {
    //     UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
    //                    "Could not open a TCP connection to %.*s",
    //                    (int)client->endpointUrl.length, client->endpointUrl.data);
    //     client->connectStatus = UA_STATUSCODE_BADCONNECTIONCLOSED;
    //     closeSecureChannel(client);
    // }

    return client->connectStatus;
}

UA_StatusCode
UA_Client_connectAsync(UA_Client *client, const char *endpointUrl) {
    /* Set the endpoint URL the client connects to */
    UA_String_clear(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Open a Session when possible */
    client->noSession = false;

    /* Connect Async */
    return initConnect(client);
}

UA_StatusCode
UA_Client_connectSecureChannelAsync(UA_Client *client, const char *endpointUrl) {
    /* Set the endpoint URL the client connects to */
    UA_String_clear(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Don't open a Session */
    client->noSession = true;

    /* Connect Async */
    return initConnect(client);
}
/*
static UA_StatusCode
connectSync(UA_Client *client) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + ((UA_DateTime)client->config.timeout * UA_DATETIME_MSEC);

    UA_StatusCode retval = initConnect(client);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    while(retval == UA_STATUSCODE_GOOD) {
        if(client->sessionState == UA_SESSIONSTATE_ACTIVATED)
            break;
        if(client->noSession && client->channel.state == UA_SECURECHANNELSTATE_OPEN)
            break;
        now = UA_DateTime_nowMonotonic();
        if(maxDate < now)
            return UA_STATUSCODE_BADTIMEOUT;
        retval = UA_Client_run_iterate(client,
                                       (UA_UInt32)((maxDate - now) / UA_DATETIME_MSEC));
    }

    return retval;
}
*/
/*
static UA_INLINE UA_String
UA_CONST_STRING(const char *chars) {
    UA_String s; s.length = 0; s.data = NULL;
    if(!chars)
        return s;
    s.length = strlen(chars); s.data = (const UA_Byte*)chars; return s;
}
*/

typedef struct {
    UA_Boolean isInitial;
    UA_ConnectionManager *cm;
    UA_Client *client;
} UA_BasicConnectionContext;

typedef struct {
    UA_BasicConnectionContext base;
    uintptr_t connectionId;
    UA_Connection connection;
} UA_ConnectionContext;


static UA_StatusCode
UA_Client_make_connection(UA_Client *client) {

    if (client->connection.handle == NULL) {
        client->connection = client->config.initConnectionFunc(
            client->config.localConnectionConfig, client->endpointUrl,
            client->config.timeout, &client->config.logger);
    }
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    if (cc->cm == NULL) {
        UA_StatusCode rv = UA_Client_setupEventLoop(client);
        UA_CHECK_STATUS(rv, return rv);
    }

    // if (UA_EventLoop_getState(cc->eventLoop) == UA_EVENTLOOPSTATE_STOPPED) {
    //     UA_StatusCode rv = UA_EventLoop_start(cc->eventLoop);
    //     UA_CHECK_STATUS(rv, return rv);
    // }

    UA_StatusCode rv = initConnect(client);
    UA_CHECK_STATUS(rv, return rv);
    /* TODO: check rv */

    UA_BasicClientConnectionContext *ctx = (UA_BasicClientConnectionContext*) UA_malloc(sizeof(UA_ClientConnectionContext));
    memset(ctx, 0, sizeof(UA_BasicClientConnectionContext));

    ctx->isInitial = true;
    ctx->cm = client->config.cm;
    ctx->client = client;

    if (client->connection.handle != NULL) {
        return UA_STATUSCODE_GOOD;
    }

    rv = client->config.cm->openConnection(client->config.cm, client->endpointUrl, ctx);
    UA_CHECK_STATUS_ERROR(rv, return UA_STATUSCODE_BADCONNECTIONREJECTED,
                          &client->config.logger, UA_LOGCATEGORY_CLIENT, "Error on opening connection");

    client->connection.handle = ctx;
    client->connection.state = UA_CONNECTIONSTATE_ESTABLISHED;

    return rv;
}

static UA_Boolean
isConnected(UA_Client *client) {
    return (client->sessionState == UA_SESSIONSTATE_ACTIVATED) ||
    (client->noSession && client->channel.state == UA_SECURECHANNELSTATE_OPEN);
}

static UA_StatusCode
connectSyncEventLoop(UA_Client *client) {
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + ((UA_DateTime)client->config.timeout * UA_DATETIME_MSEC);

    UA_StatusCode rv = UA_Client_make_connection(client);
    UA_CHECK_STATUS(rv, return rv);

    while(!isConnected(client)) {

        now = UA_DateTime_nowMonotonic();
        if(maxDate < now)
            return UA_STATUSCODE_BADTIMEOUT;
        rv = UA_Client_run_iterate(client,
                                       (UA_UInt32)((maxDate - now) / UA_DATETIME_MSEC));
        UA_CHECK_STATUS_ERROR(rv, return rv, &client->config.logger, UA_LOGCATEGORY_CLIENT,
                              "connecting client failed");
    }
    return rv;
}

UA_StatusCode
UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    /* Set the endpoint URL the client connects to */
    UA_String_clear(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Open a Session when possible */
    client->noSession = false;

    /* Connect Synchronous */
    return connectSyncEventLoop(client);
}

UA_StatusCode
UA_Client_connectSecureChannel(UA_Client *client, const char *endpointUrl) {
    /* Set the endpoint URL the client connects to */
    UA_String_clear(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    /* Don't open a Session */
    client->noSession = true;

    /* Connect Synchronous */
    return connectSyncEventLoop(client);
}

/************************/
/* Close the Connection */
/************************/

void
closeSecureChannel(UA_Client *client) {
    /* Send CLO if the SecureChannel is open */
    if(client->channel.state == UA_SECURECHANNELSTATE_OPEN) {
        UA_CloseSecureChannelRequest request;
        UA_CloseSecureChannelRequest_init(&request);
        request.requestHeader.requestHandle = ++client->requestHandle;
        request.requestHeader.timestamp = UA_DateTime_now();
        request.requestHeader.timeoutHint = 10000;
        request.requestHeader.authenticationToken = client->authenticationToken;
        UA_SecureChannel_sendSymmetricMessage(&client->channel, ++client->requestId,
                                              UA_MESSAGETYPE_CLO, &request,
                                              &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST]);
    }

    /* Clean up */
    client->channel.renewState = UA_SECURECHANNELRENEWSTATE_NORMAL;
    UA_SecureChannel_close(&client->channel);
    if(client->connection.free)
        client->connection.free(&client->connection);

    /* Set the Session to "Created" if it was "Activated" */
    if(client->sessionState > UA_SESSIONSTATE_CREATED)
        client->sessionState = UA_SESSIONSTATE_CREATED;
}

static void
sendCloseSession(UA_Client *client) {
    /* Set before sending the message to prevent recursion */
    client->sessionState = UA_SESSIONSTATE_CLOSING;

    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;
    UA_CloseSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    UA_CloseSessionRequest_clear(&request);
    UA_CloseSessionResponse_clear(&response);
}

static void
closeSession(UA_Client *client) {
    /* Is a session established? */
    if(client->sessionState == UA_SESSIONSTATE_ACTIVATED)
        sendCloseSession(client);

    UA_NodeId_clear(&client->authenticationToken);
    client->requestHandle = 0;
    client->sessionState = UA_SESSIONSTATE_CLOSED;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* We need to clean up the subscriptions */
    UA_Client_Subscriptions_clean(client);
#endif

    /* Reset so the next async connect creates a session by default */
    client->noSession = false;

    /* Delete outstanding async services */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSESSIONCLOSED);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    client->currentlyOutStandingPublishRequests = 0;
#endif
}

static void
closeSessionCallback(UA_Client *client, void *userdata,
                     UA_UInt32 requestId, void *response) {
    closeSession(client);
    closeSecureChannel(client);
    notifyClientState(client);
}

UA_StatusCode UA_EXPORT
UA_Client_disconnectAsync(UA_Client *client) {
    /* Set before sending the message to prevent recursion */
    client->sessionState = UA_SESSIONSTATE_CLOSING;

    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;
    UA_StatusCode res =
        UA_Client_sendAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                                   (UA_ClientAsyncServiceCallback)closeSessionCallback,
                                   &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], NULL, NULL);
    notifyClientState(client);
    return res;
}

UA_StatusCode
UA_Client_disconnectSecureChannel(UA_Client *client) {
    closeSecureChannel(client);
    notifyClientState(client);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_disconnect(UA_Client *client) {
    closeSession(client);
    closeSecureChannel(client);
    notifyClientState(client);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode UA_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                                                 UA_ByteString *buf) {
    UA_ClientConnectionContext *ctx = (UA_ClientConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicClientConnectionContext *)ctx)->cm;
    return cm->allocNetworkBuffer(cm, ctx->connectionId, buf, length);
}

static UA_StatusCode UA_Connection_send(UA_Connection *connection, UA_ByteString *buf) {
    UA_ClientConnectionContext *ctx = (UA_ClientConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicClientConnectionContext *)ctx)->cm;
    return cm->sendWithConnection(cm, ctx->connectionId, buf);
}

static
void UA_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf) {
    UA_ClientConnectionContext *ctx = (UA_ClientConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicClientConnectionContext *)ctx)->cm;
    cm->freeNetworkBuffer(cm, ctx->connectionId, buf);
}

static void UA_Connection_close(UA_Connection *connection) {
    UA_ClientConnectionContext *ctx = (UA_ClientConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicClientConnectionContext *)ctx)->cm;
    cm->closeConnection(cm, ctx->connectionId);
}

static UA_StatusCode UA_Connection_recv(UA_Connection *connection, UA_ByteString *response,
                               UA_UInt32 timeout) {
    UA_ClientConnectionContext *ctx = (UA_ClientConnectionContext *) connection->handle;
    // UA_ConnectionManager *cm = ((UA_BasicClientConnectionContext *)ctx)->cm;

    ctx->currentMessage.data = NULL;
    ctx->currentMessage.length = 0;

    ctx->receiveSync = true;

    UA_Client *client = ctx->base.client;
    UA_EventLoop *el = client->config.eventLoop;

    UA_StatusCode rv = UA_EventLoop_run(el, timeout);
    UA_CHECK_STATUS(rv, return rv);

    ctx->receiveSync = false;

    response->length = ctx->currentMessage.length;
    response->data = ctx->currentMessage.data;

    return rv;
}

static
UA_StatusCode
sessionAndChannelIteration(UA_ByteString *msg, UA_Client *client,
                           UA_ClientConnectionContext *conCtx) {

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if((*msg).data != NULL) {
        rv = connectionCallbackReceive(conCtx, (*msg));
        UA_CHECK_STATUS_ERROR(rv, return rv, &client->config.logger,
                              UA_LOGCATEGORY_CLIENT, "Receiving msg failed");
    }  // else {
    rv = connectionCallbackSend(conCtx);
    UA_CHECK_STATUS_ERROR(rv, return rv, &client->config.logger,
                          UA_LOGCATEGORY_CLIENT, "Sending msg failed");
    // }
    return rv;
}
void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode stat,
                   UA_ByteString msg) {

    /* TODO: is something needed here from poll? */
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_CLIENT,
                 "connection callback for id: %lu", connectionId);

    if (*connectionContext == NULL) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_CLIENT,
                     "running callback with NULL context");
        return;
    }

    UA_BasicClientConnectionContext *ctx = (UA_BasicClientConnectionContext *) *connectionContext;
    if (stat != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                    UA_LOGCATEGORY_CLIENT, "closing connection");
        if (!ctx->isInitial) {
            UA_free(ctx);
        }
        return;
    }

    UA_Client *client = ctx->client;
    if (ctx->isInitial) {
        client->connectStatus = initConnection(connectionId, connectionContext, ctx);
        UA_CHECK_STATUS_ERROR(client->connectStatus, return, UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                              UA_LOGCATEGORY_CLIENT, "Initializing connection failed");
    }

    UA_ClientConnectionContext *conCtx = (UA_ClientConnectionContext *) *connectionContext;
    if((client->noSession && client->channel.state != UA_SECURECHANNELSTATE_OPEN) ||
        client->sessionState < UA_SESSIONSTATE_ACTIVATED) {
        client->connectStatus = sessionAndChannelIteration(&msg, client, conCtx);
        notifyClientState(client);
    } else {
        if (msg.data) {
            if(conCtx->receiveSync) {
                conCtx->currentMessage.length = msg.length;
                conCtx->currentMessage.data = (UA_Byte *) UA_realloc(conCtx->currentMessage.data, sizeof(UA_Byte) * msg.length);
                UA_ByteString_copy(&msg, &conCtx->currentMessage);
                return;
            } else {
                client->connectStatus = processResponse(client, &msg, NULL, NULL, NULL);
            }
        }
    }
}

UA_StatusCode
connectionCallbackReceive(UA_ClientConnectionContext *ctx, UA_ByteString msg) {

    ctx->currentMessage = msg;

    if (ctx->receiveSync) {

    }

    UA_Client *client = ctx->base.client;
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT, "received msg");
    switch(client->channel.state) {
        case UA_SECURECHANNELSTATE_HEL_SENT:
        case UA_SECURECHANNELSTATE_OPN_SENT:
            /* TODO: integrate timeout in context */
            client->connectStatus = processResponse(client, &msg, NULL, NULL, NULL);
            return client->connectStatus;
        default:
            break;
    }

    /* Have a SecureChannel but no session */
    if(client->noSession)
        return client->connectStatus;

    switch(client->sessionState) {
        case UA_SESSIONSTATE_CLOSED:
            processResponse(client, &msg, NULL, NULL, NULL);
            return client->connectStatus;
        case UA_SESSIONSTATE_CREATE_REQUESTED:
        case UA_SESSIONSTATE_ACTIVATE_REQUESTED:
            processResponse(client, &msg, NULL, NULL, NULL);
            return client->connectStatus;
        default:
            break;
    }


    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
connectionCallbackSend(UA_ClientConnectionContext *ctx) {

    UA_Client *client = ctx->base.client;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if((client->noSession && client->channel.state != UA_SECURECHANNELSTATE_OPEN) ||
       client->sessionState < UA_SESSIONSTATE_ACTIVATED) {

        /* Open the SecureChannel */
        switch(client->channel.state) {
            case UA_SECURECHANNELSTATE_FRESH:
                client->connectStatus = sendHELMessage(client);
                if(client->connectStatus == UA_STATUSCODE_GOOD) {
                    client->channel.state = UA_SECURECHANNELSTATE_HEL_SENT;
                } else {
                    client->connection.close(&client->connection);
                    client->connection.free(&client->connection);
                }
                return client->connectStatus;
            case UA_SECURECHANNELSTATE_ACK_RECEIVED:
                client->connectStatus = sendOPNAsync(client, false);
                return client->connectStatus;
            default:
                break;
        }

        /* Have a SecureChannel but no session */
        if(client->noSession)
            return client->connectStatus;
        /* Create and Activate the Session */
        switch(client->sessionState) {
            case UA_SESSIONSTATE_CLOSED:
                if(!endpointUnconfigured(client)) {
                    client->connectStatus = createSessionAsync(client);
                    return client->connectStatus;
                }
                if(!client->endpointsHandshake) {
                    client->connectStatus = requestGetEndpoints(client);
                    return client->connectStatus;
                }
                return client->connectStatus;
            case UA_SESSIONSTATE_CREATE_REQUESTED:
            case UA_SESSIONSTATE_ACTIVATE_REQUESTED:
                return client->connectStatus;
            case UA_SESSIONSTATE_CREATED:
                client->connectStatus = activateSessionAsync(client);
                return client->connectStatus;
            default:
                break;
        }

        UA_CHECK_STATUS_ERROR(retval, return UA_STATUSCODE_BAD, &client->config.logger,
                              UA_LOGCATEGORY_CLIENT, "Error connecting");
    }

    return UA_STATUSCODE_GOOD;
}



UA_StatusCode
initConnection(uintptr_t connectionId, void **connectionContext,
                          UA_BasicClientConnectionContext *ctx) {
    UA_ClientConnectionContext *newCtx = (UA_ClientConnectionContext*) UA_calloc(1, sizeof(UA_ClientConnectionContext));
    UA_CHECK_MEM(newCtx, return UA_STATUSCODE_BADOUTOFMEMORY);
    UA_Client *client = ctx->client;
    newCtx->base.isInitial = false;
    newCtx->base.cm = ctx->cm;
    newCtx->base.client = ctx->client;
    newCtx->connectionId = connectionId;
    newCtx->connection.close = UA_Connection_close;
    newCtx->connection.free = ctx->client->connection.free;
    newCtx->connection.getSendBuffer = UA_Connection_getSendBuffer;
    newCtx->connection.recv = UA_Connection_recv; // ctx->client->connection.recv;
    newCtx->connection.releaseRecvBuffer = UA_Connection_releaseBuffer;
    newCtx->connection.releaseSendBuffer = UA_Connection_releaseBuffer;
    newCtx->connection.send = UA_Connection_send;
    newCtx->connection.state = UA_CONNECTIONSTATE_ESTABLISHED;
    newCtx->connection.sockfd = (int) connectionId;

    newCtx->connection.handle = newCtx;
    newCtx->receiveSync = false;

    newCtx->base.client->connection = newCtx->connection;

    *connectionContext = newCtx;

    /* Attach the connection to the SecureChannel */
    if(!client->channel.connection) {
        UA_Connection_attachSecureChannel(&client->connection, &client->channel);
    }
    /* Set the SecurityPolicy */
    if(!client->channel.securityPolicy) {
        client->channel.securityMode = client->config.endpoint.securityMode;
        if(client->channel.securityMode == UA_MESSAGESECURITYMODE_INVALID)
            client->channel.securityMode = UA_MESSAGESECURITYMODE_NONE;

        UA_SecurityPolicy *sp = NULL;
        if(client->config.endpoint.securityPolicyUri.length == 0) {
            sp = getSecurityPolicy(client,
                                   UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None"));
        } else {
            sp = getSecurityPolicy(client, client->config.endpoint.securityPolicyUri);
        }
        UA_CHECK_MEM_ERROR(sp, return UA_STATUSCODE_BAD, &client->config.logger,
                           UA_LOGCATEGORY_CLIENT, "Error getting security policy");

        client->connectStatus =
            UA_SecureChannel_setSecurityPolicy(&client->channel, sp,
                                               &client->config.endpoint.serverCertificate);
        UA_CHECK_STATUS_ERROR(client->connectStatus, return UA_STATUSCODE_BAD, &client->config.logger,
                              UA_LOGCATEGORY_CLIENT, "Error setting security policy");
    }
    return UA_STATUSCODE_GOOD;
}
