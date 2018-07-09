/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_types_generated_handling.h"

static UA_StatusCode
signCreateSessionResponse(UA_Server *server, UA_SecureChannel *channel,
                          const UA_CreateSessionRequest *request,
                          UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *const securityPolicy = channel->securityPolicy;
    UA_SignatureData *signatureData = &response->serverSignature;

    /* Prepare the signature */
    size_t signatureSize = securityPolicy->certificateSigningAlgorithm.
        getLocalSignatureSize(securityPolicy, channel->channelContext);
    UA_StatusCode retval = UA_String_copy(&securityPolicy->certificateSigningAlgorithm.uri,
                                          &signatureData->algorithm);
    retval |= UA_ByteString_allocBuffer(&signatureData->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temp buffer */
    size_t dataToSignSize = request->clientCertificate.length + request->clientNonce.length;
    UA_ByteString dataToSign;
    retval = UA_ByteString_allocBuffer(&dataToSign, dataToSignSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval; /* signatureData->signature is cleaned up with the response */

    /* Sign the signature */
    memcpy(dataToSign.data, request->clientCertificate.data, request->clientCertificate.length);
    memcpy(dataToSign.data + request->clientCertificate.length,
           request->clientNonce.data, request->clientNonce.length);
    retval = securityPolicy->certificateSigningAlgorithm.
        sign(securityPolicy, channel->channelContext, &dataToSign, &signatureData->signature);

    /* Clean up */
    UA_ByteString_deleteMembers(&dataToSign);
    return retval;
}

void
Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                      const UA_CreateSessionRequest *request,
                      UA_CreateSessionResponse *response) {
    if(!channel) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    if(!channel->connection) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Trying to create session");

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_ByteString_equal(&request->clientCertificate,
                                &channel->remoteCertificate)) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADCERTIFICATEINVALID;
            return;
        }
    }

    if(channel->securityToken.channelId == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
        return;
    }

    if(!UA_ByteString_equal(&channel->securityPolicy->policyUri,
                            &UA_SECURITY_POLICY_NONE_URI) &&
       request->clientNonce.length < 32) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNONCEINVALID;
        return;
    }

    /* TODO: Compare application URI with certificate uri (decode certificate) */
    UA_CertificateVerification *cv = channel->securityPolicy->certificateVerification;
    if(cv && cv->verifyApplicationURI) {
        response->responseHeader.serviceResult =
            cv->verifyApplicationURI(cv->context, &request->clientCertificate,
                                     &request->clientDescription.applicationUri);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
            return;
    }

    /* Allocate the response */
    response->serverEndpoints = (UA_EndpointDescription *)
        UA_Array_new(server->config.endpointsSize,
                     &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->serverEndpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->serverEndpointsSize = server->config.endpointsSize;

    /* Copy the server's endpointdescriptions into the response */
    for(size_t i = 0; i < server->config.endpointsSize; ++i)
        response->responseHeader.serviceResult |=
            UA_EndpointDescription_copy(&server->config.endpoints[i].endpointDescription,
                                        &response->serverEndpoints[i]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Mirror back the endpointUrl */
    for(size_t i = 0; i < response->serverEndpointsSize; ++i) {
        UA_String_deleteMembers(&response->serverEndpoints[i].endpointUrl);
        UA_String_copy(&request->endpointUrl,
                       &response->serverEndpoints[i].endpointUrl);
    }

    UA_Session *newSession = NULL;
    response->responseHeader.serviceResult =
        UA_SessionManager_createSession(&server->sessionManager, channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                             "Processing CreateSessionRequest failed");
        return;
    }

    UA_assert(newSession != NULL);

    /* Attach the session to the channel. But don't activate for now. */
    UA_Session_attachToSecureChannel(newSession, channel);

    /* Fill the session information */
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize =
        channel->connection->localConf.maxMessageSize;
    response->responseHeader.serviceResult |=
        UA_ApplicationDescription_copy(&request->clientDescription,
                                       &newSession->clientDescription);

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->header.authenticationToken;
    response->responseHeader.serviceResult =
        UA_String_copy(&request->sessionName, &newSession->sessionName);

    if(server->config.endpointsSize > 0)
        response->responseHeader.serviceResult |=
            UA_ByteString_copy(&channel->securityPolicy->localCertificate,
                               &response->serverCertificate);

    /* Create a session nonce */
    response->responseHeader.serviceResult = UA_Session_generateNonce(newSession);
    response->responseHeader.serviceResult |=
        UA_ByteString_copy(&newSession->serverNonce, &response->serverNonce);

    /* Sign the signature */
    response->responseHeader.serviceResult |=
       signCreateSessionResponse(server, channel, request, response);

    /* Failure -> remove the session */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager,
                                        &newSession->header.authenticationToken);
        return;
    }

    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel,
                         "Session " UA_PRINTF_GUID_FORMAT " created",
                         UA_PRINTF_GUID_DATA(newSession->sessionId.identifier.guid));
}

static UA_StatusCode
checkSignature(const UA_Server *server, const UA_SecureChannel *channel,
               UA_Session *session, const UA_ActivateSessionRequest *request) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    if(!channel->securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;
    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    const UA_ByteString *localCertificate = &securityPolicy->localCertificate;

    size_t dataToVerifySize = localCertificate->length + session->serverNonce.length;

    UA_ByteString dataToVerify;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, localCertificate->data, localCertificate->length);
    memcpy(dataToVerify.data + localCertificate->length,
           session->serverNonce.data, session->serverNonce.length);

    retval = securityPolicy->certificateSigningAlgorithm.verify(securityPolicy, channel->channelContext, &dataToVerify,
                                                                &request->clientSignature.signature);
    UA_ByteString_deleteMembers(&dataToVerify);
    return retval;
}

/* TODO: Check all of the following:
 *
 * Part 4, §5.6.3: When the ActivateSession Service is called for the first time
 * then the Server shall reject the request if the SecureChannel is not same as
 * the one associated with the CreateSession request. Subsequent calls to
 * ActivateSession may be associated with different SecureChannels. If this is
 * the case then the Server shall verify that the Certificate the Client used to
 * create the new SecureChannel is the same as the Certificate used to create
 * the original SecureChannel. In addition, the Server shall verify that the
 * Client supplied a UserIdentityToken that is identical to the token currently
 * associated with the Session. Once the Server accepts the new SecureChannel it
 * shall reject requests sent via the old SecureChannel. */

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        UA_Session *session, const UA_ActivateSessionRequest *request,
                        UA_ActivateSessionResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Execute ActivateSession");

    if(session->validTill < UA_DateTime_nowMonotonic()) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: SecureChannel %i wants "
                            "to activate, but the session has timed out",
                            channel->securityToken.channelId);
        response->responseHeader.serviceResult =
            UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
    }

    /* Check if the signature corresponds to the ServerNonce that was last sent
     * to the client */
    response->responseHeader.serviceResult = checkSignature(server, channel, session, request);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "Signature check failed with status code %s",
                            UA_StatusCode_name(response->responseHeader.serviceResult));
        return;
    }

    /* Find the matching endpoint */
    const UA_EndpointDescription *ed = NULL;
    for(size_t i = 0; ed == NULL && i < server->config.endpointsSize; ++i) {
        const UA_Endpoint *e = &server->config.endpoints[i];

        /* Match the Security Mode */
        if(e->endpointDescription.securityMode != channel->securityMode)
            continue;

        /* Match the SecurityPolicy */
        if(!UA_String_equal(&e->securityPolicy.policyUri,
                            &channel->securityPolicy->policyUri))
            continue;

        /* Match the UserTokenType */
        for(size_t j = 0; j < e->endpointDescription.userIdentityTokensSize; j++) {
            const UA_UserTokenPolicy *u = &e->endpointDescription.userIdentityTokens[j];
            if(u->tokenType == UA_USERTOKENTYPE_ANONYMOUS) {
                if(request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN])
                    continue;
            } else if(u->tokenType == UA_USERTOKENTYPE_USERNAME) {
                if(request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])
                    continue;
            } else if(u->tokenType == UA_USERTOKENTYPE_CERTIFICATE) {
                if(request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN])
                    continue;
            } else if(u->tokenType == UA_USERTOKENTYPE_ISSUEDTOKEN) {
                if(request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN])
                    continue;
            } else {
                response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
                return;
            }

            /* Match found */
            ed = &e->endpointDescription;
            break;
        }

    }

    /* No matching endpoint found */
    if(!ed) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENREJECTED;
        return;
    }

    /* Callback into userland access control */
    response->responseHeader.serviceResult =
        server->config.accessControl.activateSession(server, &server->config.accessControl,
                                                     ed, &channel->remoteCertificate,
                                                     &session->sessionId,
                                                     &request->userIdentityToken,
                                                     &session->sessionHandle);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: Could not generate a server nonce");
        return;
    }

    if(session->header.channel && session->header.channel != channel) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: Detach from old channel");
        /* Detach the old SecureChannel and attach the new */
        UA_Session_detachFromSecureChannel(session);
        UA_Session_attachToSecureChannel(session, channel);
    }

    /* Activate the session */
    session->activated = true;
    UA_Session_updateLifetime(session);

    /* Generate a new session nonce for the next time ActivateSession is called */
    response->responseHeader.serviceResult = UA_Session_generateNonce(session);
    response->responseHeader.serviceResult |=
        UA_ByteString_copy(&session->serverNonce, &response->serverNonce);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_detachFromSecureChannel(session);
        session->activated = false;
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: Could not generate a server nonce");
        return;
    }

    UA_LOG_INFO_SESSION(server->config.logger, session,
                        "ActivateSession: Session activated");
}

void
Service_CloseSession(UA_Server *server, UA_Session *session,
                     const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response) {
    UA_LOG_INFO_SESSION(server->config.logger, session, "CloseSession");

    /* Callback into userland access control */
    server->config.accessControl.closeSession(server, &server->config.accessControl,
                                              &session->sessionId, session->sessionHandle);
    response->responseHeader.serviceResult =
        UA_SessionManager_removeSession(&server->sessionManager,
                                        &session->header.authenticationToken);
}
