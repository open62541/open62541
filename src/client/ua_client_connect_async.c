/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/transport_generated.h>
#include <open62541/transport_generated_encoding_binary.h>
#include <open62541/transport_generated_handling.h>
#include <open62541/types_generated_encoding_binary.h>

#include "ua_client_internal.h"

#define UA_MINMESSAGESIZE                8192
#define UA_SESSION_LOCALNONCELENGTH      32
#define MAX_DATA_SIZE 4096

/* Function to create a signature using remote certificate and nonce */
#ifdef UA_ENABLE_ENCRYPTION
static UA_StatusCode
signActivateSessionRequest(UA_SecureChannel *channel,
                           UA_ActivateSessionRequest *request) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    UA_SignatureData *sd = &request->clientSignature;

    /* Prepare the signature */
    size_t signatureSize = sp->certificateSigningAlgorithm.
        getLocalSignatureSize(sp, channel->channelContext);
    UA_StatusCode retval = UA_String_copy(&sp->certificateSigningAlgorithm.uri,
                                          &sd->algorithm);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ByteString_allocBuffer(&sd->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temporary buffer */
    size_t dataToSignSize = channel->remoteCertificate.length + channel->remoteNonce.length;
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
           channel->remoteNonce.data, channel->remoteNonce.length);
    retval = sp->certificateSigningAlgorithm.sign(sp, channel->channelContext,
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
        encryptionAlgorithm.getRemotePlainTextBlockSize(sp, channelContext);
    UA_UInt32 length = (UA_UInt32)(tokenData->length + client->channel.remoteNonce.length);
    UA_UInt32 totalLength = length + 4; /* Including the length field */
    size_t blocks = totalLength / plainTextBlockSize;
    if(totalLength  % plainTextBlockSize != 0)
        blocks++;
    size_t overHead =
        UA_SecurityPolicy_getRemoteAsymEncryptionBufferLengthOverhead(sp, channelContext,
                                                                      blocks * plainTextBlockSize);

    /* Allocate memory for encryption overhead */
    UA_ByteString encrypted;
    retval = UA_ByteString_allocBuffer(&encrypted, (blocks * plainTextBlockSize) + overHead);
    if(retval != UA_STATUSCODE_GOOD) {
        sp->channelModule.deleteContext(channelContext);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Byte *pos = encrypted.data;
    const UA_Byte *end = &encrypted.data[encrypted.length];
    UA_UInt32_encodeBinary(&length, &pos, end);
    memcpy(pos, tokenData->data, tokenData->length);
    memcpy(&pos[tokenData->length], client->channel.remoteNonce.data,
           client->channel.remoteNonce.length);

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

    retval = sp->asymmetricModule.cryptoModule.encryptionAlgorithm.encrypt(sp, channelContext,
                                                                           &encrypted);
    encrypted.length = (blocks * plainTextBlockSize) + overHead;

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
checkCreateSessionSignature(const UA_SecureChannel *channel,
                            const UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    if(!channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_SecurityPolicy *sp = channel->securityPolicy;
    const UA_ByteString *lc = &sp->localCertificate;

    size_t dataToVerifySize = lc->length + channel->localNonce.length;
    UA_ByteString dataToVerify = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, lc->data, lc->length);
    memcpy(dataToVerify.data + lc->length,
           channel->localNonce.data, channel->localNonce.length);

    retval = sp->certificateSigningAlgorithm.
        verify(sp, channel->channelContext, &dataToVerify,
               &response->serverSignature.signature);
    UA_ByteString_clear(&dataToVerify);
    return retval;
}

#endif

/***********************/
/* Open the Connection */
/***********************/
static void sendOPNAsync(UA_Client *client);
static UA_StatusCode requestGetEndpoints(UA_Client *client);

void
processACKResponseAsync(void *application, UA_SecureChannel *channel,
                        UA_MessageType messageType, UA_UInt32 requestId,
                        UA_ByteString *chunk) {
    UA_Client *client = (UA_Client*)application;

    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");

    /* Decode the message */
    size_t offset = 8;
    UA_TcpAcknowledgeMessage ackMessage;
    client->connectStatus = UA_TcpAcknowledgeMessage_decodeBinary(chunk, &offset, &ackMessage);
    if(client->connectStatus != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Decoding ACK message failed");
        UA_Client_disconnect(client);
        return;
    }

    client->connectStatus =
        UA_SecureChannel_processHELACK(channel, &ackMessage);
    if(client->connectStatus != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Processing the ACK message failed with StatusCode %s",
                     UA_StatusCode_name(client->connectStatus));
        UA_Client_disconnect(client);
        return;
    }

    client->state = UA_CLIENTSTATE_CONNECTED;

    /* Open a SecureChannel. TODO: Select with endpoint  */
    client->channel.connection = &client->connection;
    sendOPNAsync(client);
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
    retval = conn->send (conn, &message);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Sent HEL message");
    } else {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_NETWORK, "Sending HEL failed");
    }
    return retval;
}

static void
processOPNResponseDecoded(UA_Client *client, const UA_ByteString *message) {
    /* Is the content of the expected type? */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_NodeId expectedId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        return;
    }

    if(!UA_NodeId_equal(&responseId, &expectedId)) {
        UA_NodeId_clear(&responseId);
        UA_Client_disconnect(client);
        return;
    }

    /* Decode the response */
    UA_OpenSecureChannelResponse response;
    retval = UA_OpenSecureChannelResponse_decodeBinary(message, &offset, &response);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        return;
    }

    /* Response.securityToken.revisedLifetime is UInt32 we need to cast it to
     * DateTime=Int64 we take 75% of lifetime to start renewing as described in
     * standard */
    client->nextChannelRenewal = UA_DateTime_nowMonotonic()
            + (UA_DateTime) (response.securityToken.revisedLifetime
                    * (UA_Double) UA_DATETIME_MSEC * 0.75);

    /* Replace the token. On the client side we don't use NextSecurityToken. */
    UA_ChannelSecurityToken_clear(&client->channel.securityToken);
    client->channel.securityToken = response.securityToken;
    UA_ChannelSecurityToken_init(&response.securityToken);

    /* Replace the nonce */
    UA_ByteString_clear(&client->channel.remoteNonce);
    client->channel.remoteNonce = response.serverNonce;
    UA_ByteString_init(&response.serverNonce);

    UA_ResponseHeader_clear(&response.responseHeader); /* the other members were moved */

    retval = UA_SecureChannel_generateNewKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        return;
    }

    UA_Boolean renew = (client->channel.state == UA_SECURECHANNELSTATE_OPEN);
    if(renew)
        UA_LOG_INFO_CHANNEL(&client->config.logger, &client->channel, "SecureChannel renewed");
    else
        UA_LOG_INFO_CHANNEL(&client->config.logger, &client->channel,
                            "Opened SecureChannel with SecurityPolicy %.*s",
                            (int)client->channel.securityPolicy->policyUri.length,
                            client->channel.securityPolicy->policyUri.data);
    client->channel.state = UA_SECURECHANNELSTATE_OPEN;

    if(client->state < UA_CLIENTSTATE_SECURECHANNEL)
        setClientState(client, UA_CLIENTSTATE_SECURECHANNEL);
}

void
decodeProcessOPNResponseAsync(void *application, UA_SecureChannel *channel,
                              UA_MessageType messageType, UA_UInt32 requestId,
                              UA_ByteString *msg) {
    UA_Client *client = (UA_Client*)application;

    /* Skip the first header. We know length and message type. */
    size_t offset = UA_SECURE_CONVERSATION_MESSAGE_HEADER_LENGTH;

    /* Decode the asymmetric algorithm security header and call the callback
     * to perform checks. */
    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    UA_StatusCode retval =
        UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(msg, &offset, &asymHeader);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&client->config.logger, channel,
                               "Could not decode the OPN header");
        UA_Client_disconnect(client);
        return;
    }

    /* Verify the certificate before creating the SecureChannel with it */
    if(asymHeader.senderCertificate.length > 0) {
        retval = client->config.certificateVerification.
            verifyCertificate(client->config.certificateVerification.context,
                              &asymHeader.senderCertificate);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(&client->config.logger, channel,
                                   "Could not verify the server's certificate");
            UA_Client_disconnect(client);
            return;
        }
    }

    retval = checkAsymHeader(channel, &asymHeader);
    UA_AsymmetricAlgorithmSecurityHeader_clear(&asymHeader);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&client->config.logger, channel,
                               "Could not verify the OPN header");
        UA_Client_disconnect(client);
        return;
    }

    UA_UInt32 sequenceNumber = 0;
    retval = decryptAndVerifyChunk(channel, &channel->securityPolicy->asymmetricModule.cryptoModule,
                                   UA_MESSAGETYPE_OPN, msg, offset, &requestId, &sequenceNumber);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&client->config.logger, channel,
                               "Could not decrypt and verify the OPN payload");
        UA_Client_disconnect(client);
        return;
    }

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    retval = processSequenceNumberAsym(channel, sequenceNumber);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&client->config.logger, channel,
                               "Could not process the OPN sequence number");
        UA_Client_disconnect(client);
        return;
    }
#endif

    processOPNResponseDecoded(client, msg);
}

/* OPN messges to renew the channel are sent asynchronous */
static void
sendOPNAsync(UA_Client *client) {
    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTION_ESTABLISHED) {
        UA_Client_disconnect(client);
        return;
    }

    /* Prepare the OpenSecureChannelRequest */
    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                 "Requesting to open a SecureChannel");
    opnSecRq.securityMode = client->channel.securityMode;

    opnSecRq.clientNonce = client->channel.localNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;

    /* Prepare the entry for the linked list */
    UA_UInt32 requestId = ++client->requestId;

    /* Send the OPN message */
    UA_StatusCode retval = UA_SecureChannel_sendAsymmetricOPNMessage (
            &client->channel, requestId, &opnSecRq,
            &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST]);
    client->connectStatus = retval;

    if(retval != UA_STATUSCODE_GOOD) {
        client->connectStatus = retval;
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                      "Sending OPN message failed with error %s",
                      UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        return;
    }

    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                 "OPN message sent");
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
            createSessionAsync(client);
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Session cannot be activated. Create a new Session.");
        } else {
            /* Something else is wrong. Give up. */
            client->connectStatus = activateResponse->responseHeader.serviceResult;
        }
        return;
    }

    client->connection.state = UA_CONNECTION_ESTABLISHED;
    setClientState(client, UA_CLIENTSTATE_SESSION);
    client->sessionHandshake = false;

    /* Call into userland to signal the activated session */
    if(client->asyncConnectCall.callback)
        client->asyncConnectCall.callback(client, client->asyncConnectCall.userdata,
                                          requestId + 1,
                                          &activateResponse->responseHeader.serviceResult);
}

UA_StatusCode
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
    retval |= signActivateSessionRequest(&client->channel, &request);
#endif

    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_Client_sendAsyncRequest(
                    client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                    (UA_ClientAsyncServiceCallback) responseActivateSession,
                    &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL, NULL);
    UA_ActivateSessionRequest_clear(&request);
    return retval;
}

/* Combination of UA_Client_getEndpointsInternal and getEndpoints */
static void
responseGetEndpoints(UA_Client *client, void *userdata, UA_UInt32 requestId,
                     void *response) {
    client->endpointsHandshake = false;

    UA_GetEndpointsResponse *resp = (UA_GetEndpointsResponse*)response;
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
        /* look out for binary transport endpoints */
        /* Note: Siemens returns empty ProfileUrl, we will accept it as binary */
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
            if(tokenPolicy->securityPolicyUri.length > 0 &&
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
                        "Selected Endpoint %.*s with SecurityMode %s and SecurityPolicy %.*s",
                        (int)endpoint->endpointUrl.length, endpoint->endpointUrl.data,
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
}

static UA_StatusCode
requestGetEndpoints(UA_Client *client) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.endpointUrl = client->endpointUrl;

    client->connectStatus = UA_Client_sendAsyncRequest(
            client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
            (UA_ClientAsyncServiceCallback) responseGetEndpoints,
            &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE], NULL, NULL);

    if(client->connectStatus == UA_STATUSCODE_GOOD)
        client->endpointsHandshake = true;

    return client->connectStatus;
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
        res = checkCreateSessionSignature(&client->channel, sessionResponse);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }
#endif
    
    /* Copy nonce and and authenticationtoken */
    UA_ByteString_clear(&client->channel.remoteNonce);
    UA_NodeId_clear(&client->authenticationToken);
    res |= UA_ByteString_copy(&sessionResponse->serverNonce, &client->channel.remoteNonce);
    res |= UA_NodeId_copy(&sessionResponse->authenticationToken, &client->authenticationToken);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Activate the new Session */
    res = activateSessionAsync(client);

 cleanup:
    client->connectStatus = res;
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        client->sessionHandshake = false;
}

UA_StatusCode
createSessionAsync(UA_Client *client) {
    /* A new session is created. We need to clean up the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_clean(client);
    client->currentlyOutStandingPublishRequests = 0;
#endif

    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(client->channel.localNonce.length != UA_SESSION_LOCALNONCELENGTH) {
           UA_ByteString_clear(&client->channel.localNonce);
            retval = UA_ByteString_allocBuffer(&client->channel.localNonce,
                                               UA_SESSION_LOCALNONCELENGTH);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }

        retval = client->channel.securityPolicy->symmetricModule.
                 generateNonce(client->channel.securityPolicy, &client->channel.localNonce);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel.localNonce, &request.clientNonce);
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

    retval = UA_Client_sendAsyncRequest (
            client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
            (UA_ClientAsyncServiceCallback) responseSessionCallback,
            &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL, NULL);
    UA_CreateSessionRequest_clear(&request);

    if(retval == UA_STATUSCODE_GOOD)
        client->sessionHandshake = true;

    client->connectStatus = retval;
    return client->connectStatus;
}

UA_StatusCode
UA_Client_connect_iterate(UA_Client *client) {
    UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Client connect iterate");
    if (client->connection.state == UA_CONNECTION_ESTABLISHED){
        if(client->state == UA_CLIENTSTATE_SECURECHANNEL) {
            if(endpointUnconfigured(client)) {
                if(!client->endpointsHandshake)
                    requestGetEndpoints(client);
            } else {
                if(!client->sessionHandshake)
                    createSessionAsync(client);
            }
            return client->connectStatus;
        }
        if(client->state < UA_CLIENTSTATE_WAITING_FOR_ACK) {
            client->connectStatus = sendHELMessage(client);
            if(client->connectStatus == UA_STATUSCODE_GOOD) {
                setClientState(client, UA_CLIENTSTATE_WAITING_FOR_ACK);
            } else {
                client->connection.close(&client->connection);
                client->connection.free(&client->connection);
            }
            return client->connectStatus;
        }
    }

    /* If server is not connected */
    if(client->connection.state == UA_CONNECTION_CLOSED) {
        client->connectStatus = UA_STATUSCODE_BADCONNECTIONCLOSED;
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "No connection to server.");
    }

    if(client->connectStatus != UA_STATUSCODE_GOOD) {
        client->connection.close(&client->connection);
        client->connection.free(&client->connection);
    }

    return client->connectStatus;
}

UA_StatusCode
UA_Client_connect_async(UA_Client *client, const char *endpointUrl,
                        UA_ClientAsyncServiceCallback callback,
                        void *userdata) {
    UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Client internal async");

    if(client->state >= UA_CLIENTSTATE_WAITING_FOR_ACK)
        return UA_STATUSCODE_GOOD;

    UA_ChannelSecurityToken_init(&client->channel.securityToken);
    client->channel.state = UA_SECURECHANNELSTATE_FRESH;
    client->channel.sendSequenceNumber = 0;
    client->requestId = 0;
    client->channel.config = client->config.localConnectionConfig;

    UA_String_clear(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    client->connection =
        client->config.initConnectionFunc(client->config.localConnectionConfig,
                                          client->endpointUrl,
                                          client->config.timeout, &client->config.logger);
    if(client->connection.state != UA_CONNECTION_OPENING) {
        UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Could not init async connection");
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    /* Set the channel SecurityMode if not done so far */
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_INVALID)
        client->channel.securityMode = UA_MESSAGESECURITYMODE_NONE;

    /* Set the channel SecurityPolicy if not done so far */
    if(!client->channel.securityPolicy) {
        UA_SecurityPolicy *sp =
            getSecurityPolicy(client, UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None"));
        if(!sp) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        UA_ByteString remoteCertificate = UA_BYTESTRING_NULL;
        retval = UA_SecureChannel_setSecurityPolicy(&client->channel, sp,
                                                    &remoteCertificate);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    client->asyncConnectCall.callback = callback;
    client->asyncConnectCall.userdata = userdata;

    if(!client->connection.connectCallbackID) {
        UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Adding async connection callback");
        retval = UA_Client_addRepeatedCallback(
                     client, client->config.pollConnectionFunc, &client->connection, 100.0,
                     &client->connection.connectCallbackID);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    retval = UA_SecureChannel_generateLocalNonce(&client->channel);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_Connection_attachSecureChannel(&client->connection, &client->channel);

    /* Delete async service. TODO: Move this from connect to the disconnect/cleanup phase */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    client->currentlyOutStandingPublishRequests = 0;
#endif

    UA_NodeId_clear(&client->authenticationToken);

    /* Generate new local and remote key */
    retval = UA_SecureChannel_generateNewKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    return retval;

 cleanup:
    UA_LOG_TRACE(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Failure during async connect");
    UA_Client_disconnect(client);
    return retval;
}

/* Async disconnection */
static void
sendCloseSecureChannelAsync(UA_Client *client, void *userdata,
                             UA_UInt32 requestId, void *response) {
    UA_NodeId_clear(&client->authenticationToken);
    client->requestHandle = 0;

    UA_SecureChannel *channel = &client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.requestHeader.authenticationToken = client->authenticationToken;
    UA_SecureChannel_sendSymmetricMessage(
            channel, ++client->requestId, UA_MESSAGETYPE_CLO, &request,
            &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST]);
    UA_SecureChannel_close(&client->channel);
    UA_SecureChannel_deleteMembers(&client->channel);
}

static void
sendCloseSessionAsync(UA_Client *client, UA_UInt32 *requestId) {
    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;

    UA_Client_sendAsyncRequest(
            client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
            (UA_ClientAsyncServiceCallback) sendCloseSecureChannelAsync,
            &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], NULL, requestId);

}

UA_StatusCode
UA_Client_disconnect_async(UA_Client *client, UA_UInt32 *requestId) {
    /* Is a session established? */
    if (client->state == UA_CLIENTSTATE_SESSION) {
        client->state = UA_CLIENTSTATE_SESSION_DISCONNECTED;
        sendCloseSessionAsync(client, requestId);
    }

    /* Close the TCP connection
     * shutdown and close (in tcp.c) are already async*/
    if (client->state >= UA_CLIENTSTATE_CONNECTED)
        client->connection.close(&client->connection);
    else
        UA_Client_removeRepeatedCallback(client, client->connection.connectCallbackID);

#ifdef UA_ENABLE_SUBSCRIPTIONS
// TODO REMOVE WHEN UA_SESSION_RECOVERY IS READY
    /* We need to clean up the subscriptions */
    UA_Client_Subscriptions_clean(client);
#endif

    setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
    return UA_STATUSCODE_GOOD;
}
