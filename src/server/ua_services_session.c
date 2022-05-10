/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2018-2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include "ua_server_internal.h"
#include "ua_services.h"

/* Delayed callback to free the session memory */
static void
removeSessionCallback(UA_Server *server, session_list_entry *entry) {
    UA_LOCK(&server->serviceMutex);
    UA_Session_clear(&entry->session, server);
    UA_UNLOCK(&server->serviceMutex);
}

void
UA_Server_removeSession(UA_Server *server, session_list_entry *sentry,
                        UA_DiagnosticEvent event) {
    UA_Session *session = &sentry->session;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove the Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Subscription *sub, *tempsub;
    TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, tempsub) {
        UA_Subscription_delete(server, sub);
    }

    UA_PublishResponseEntry *entry;
    while((entry = UA_Session_dequeuePublishReq(session))) {
        UA_PublishResponse_clear(&entry->response);
        UA_free(entry);
    }
#endif

    /* Callback into userland access control */
    if(server->config.accessControl.closeSession) {
        UA_UNLOCK(&server->serviceMutex);
        server->config.accessControl.closeSession(server, &server->config.accessControl,
                                                  &session->sessionId, session->sessionHandle);
        UA_LOCK(&server->serviceMutex);
    }

    /* Detach the Session from the SecureChannel */
    UA_Session_detachFromSecureChannel(session);

    /* Deactivate the session */
    if(sentry->session.activated) {
        sentry->session.activated = false;
        server->activeSessionCount--;
    }

    /* Detach the session from the session manager and make the capacity
     * available */
    LIST_REMOVE(sentry, pointers);
    server->sessionCount--;

    switch(event) {
    case UA_DIAGNOSTICEVENT_CLOSE:
    case UA_DIAGNOSTICEVENT_PURGE:
        break;
    case UA_DIAGNOSTICEVENT_TIMEOUT:
        server->serverDiagnosticsSummary.sessionTimeoutCount++;
        break;
    case UA_DIAGNOSTICEVENT_REJECT:
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        break;
    case UA_DIAGNOSTICEVENT_SECURITYREJECT:
        server->serverDiagnosticsSummary.securityRejectedSessionCount++;
        break;
    case UA_DIAGNOSTICEVENT_ABORT:
        server->serverDiagnosticsSummary.sessionAbortCount++;
        break;
    default:
        UA_assert(false);
        break;
    }

    /* Add a delayed callback to remove the session when the currently
     * scheduled jobs have completed */
    sentry->cleanupCallback.callback = (UA_Callback)removeSessionCallback;
    sentry->cleanupCallback.application = server;
    sentry->cleanupCallback.context = sentry;
    server->config.eventLoop->
        addDelayedCallback(server->config.eventLoop, &sentry->cleanupCallback);
}

UA_StatusCode
UA_Server_removeSessionByToken(UA_Server *server, const UA_NodeId *token,
                               UA_DiagnosticEvent event) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    session_list_entry *entry;
    LIST_FOREACH(entry, &server->sessions, pointers) {
        if(UA_NodeId_equal(&entry->session.header.authenticationToken, token)) {
            UA_Server_removeSession(server, entry, event);
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADSESSIONIDINVALID;
}

void
UA_Server_cleanupSessions(UA_Server *server, UA_DateTime nowMonotonic) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    session_list_entry *sentry, *temp;
    LIST_FOREACH_SAFE(sentry, &server->sessions, pointers, temp) {
        /* Session has timed out? */
        if(sentry->session.validTill >= nowMonotonic)
            continue;
        UA_LOG_INFO_SESSION(&server->config.logger, &sentry->session, "Session has timed out");
        UA_Server_removeSession(server, sentry, UA_DIAGNOSTICEVENT_TIMEOUT);
    }
}

/************/
/* Services */
/************/

UA_Session *
getSessionByToken(UA_Server *server, const UA_NodeId *token) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    session_list_entry *current = NULL;
    LIST_FOREACH(current, &server->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.header.authenticationToken, token))
            continue;

        /* Session has timed out */
        if(UA_DateTime_nowMonotonic() > current->session.validTill) {
            UA_LOG_INFO_SESSION(&server->config.logger, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        return &current->session;
    }

    return NULL;
}

UA_Session *
UA_Server_getSessionById(UA_Server *server, const UA_NodeId *sessionId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    session_list_entry *current = NULL;
    LIST_FOREACH(current, &server->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.sessionId, sessionId))
            continue;

        /* Session has timed out */
        if(UA_DateTime_nowMonotonic() > current->session.validTill) {
            UA_LOG_INFO_SESSION(&server->config.logger, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        return &current->session;
    }

    return NULL;
}

static UA_StatusCode
signCreateSessionResponse(UA_Server *server, UA_SecureChannel *channel,
                          const UA_CreateSessionRequest *request,
                          UA_CreateSessionResponse *response) {
    if(channel->securityMode != UA_MESSAGESECURITYMODE_SIGN &&
       channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_GOOD;

    const UA_SecurityPolicy *securityPolicy = channel->securityPolicy;
    UA_SignatureData *signatureData = &response->serverSignature;

    /* Prepare the signature */
    size_t signatureSize = securityPolicy->certificateSigningAlgorithm.
        getLocalSignatureSize(channel->channelContext);
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
        sign(channel->channelContext, &dataToSign, &signatureData->signature);

    /* Clean up */
    UA_ByteString_clear(&dataToSign);
    return retval;
}

/* Creates and adds a session. But it is not yet attached to a secure channel. */
UA_StatusCode
UA_Server_createSession(UA_Server *server, UA_SecureChannel *channel,
                        const UA_CreateSessionRequest *request, UA_Session **session) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(server->sessionCount >= server->config.maxSessions) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "Could not create a Session - Server limits reached");
        return UA_STATUSCODE_BADTOOMANYSESSIONS;
    }

    session_list_entry *newentry = (session_list_entry*)
        UA_malloc(sizeof(session_list_entry));
    if(!newentry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Initialize the Session */
    UA_Session_init(&newentry->session);
    newentry->session.sessionId = UA_NODEID_GUID(1, UA_Guid_random());
    newentry->session.header.authenticationToken = UA_NODEID_GUID(1, UA_Guid_random());

    newentry->session.timeout = server->config.maxSessionTimeout;
    if(request->requestedSessionTimeout <= server->config.maxSessionTimeout &&
       request->requestedSessionTimeout > 0)
        newentry->session.timeout = request->requestedSessionTimeout;

    /* Attach the session to the channel. But don't activate for now. */
    if(channel)
        UA_Session_attachToSecureChannel(&newentry->session, channel);
    UA_Session_updateLifetime(&newentry->session);

    /* Add to the server */
    LIST_INSERT_HEAD(&server->sessions, newentry, pointers);
    server->sessionCount++;

    *session = &newentry->session;
    return UA_STATUSCODE_GOOD;
}

void
Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                      const UA_CreateSessionRequest *request,
                      UA_CreateSessionResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_LOG_DEBUG_CHANNEL(&server->config.logger, channel, "Trying to create session");

    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        /* Compare the clientCertificate with the remoteCertificate of the channel.
         * Both the clientCertificate of this request and the remoteCertificate
         * of the channel may contain a partial or a complete certificate chain.
         * The compareCertificate function of the channelModule will compare the
         * first certificate of each chain. The end certificate shall be located
         * first in the chain according to the OPC UA specification Part 6 (1.04),
         * chapter 6.2.3.*/
        UA_StatusCode retval = channel->securityPolicy->channelModule.
            compareCertificate(channel->channelContext, &request->clientCertificate);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "The client certificate did not validate");
            response->responseHeader.serviceResult = UA_STATUSCODE_BADCERTIFICATEINVALID;
            return;
        }
    }

    UA_assert(channel->securityToken.channelId != 0);

    if(!UA_ByteString_equal(&channel->securityPolicy->policyUri,
                            &UA_SECURITY_POLICY_NONE_URI) &&
       request->clientNonce.length < 32) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNONCEINVALID;
        return;
    }

    if(request->clientCertificate.length > 0) {
        UA_CertificateVerification *cv = &server->config.certificateVerification;
        response->responseHeader.serviceResult =
            cv->verifyApplicationURI(cv->context, &request->clientCertificate,
                                     &request->clientDescription.applicationUri);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                                   "The client's ApplicationURI did not match the certificate");
            server->serverDiagnosticsSummary.securityRejectedSessionCount++;
            server->serverDiagnosticsSummary.rejectedSessionCount++;
            return;
        }
    }

    UA_Session *newSession = NULL;
    response->responseHeader.serviceResult =
        UA_Server_createSession(server, channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "Processing CreateSessionRequest failed");
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        return;
    }

    UA_assert(newSession != NULL);

    /* Allocate the response */
    response->serverEndpoints = (UA_EndpointDescription *)
        UA_Array_new(server->config.endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->serverEndpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_Server_removeSessionByToken(server, &newSession->header.authenticationToken,
                                       UA_DIAGNOSTICEVENT_REJECT);
        return;
    }
    response->serverEndpointsSize = server->config.endpointsSize;

    /* Copy the server's endpointdescriptions into the response */
    for(size_t i = 0; i < server->config.endpointsSize; ++i)
        response->responseHeader.serviceResult |=
            UA_EndpointDescription_copy(&server->config.endpoints[i],
                                        &response->serverEndpoints[i]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Server_removeSessionByToken(server, &newSession->header.authenticationToken,
                                       UA_DIAGNOSTICEVENT_REJECT);
        return;
    }

    /* Mirror back the endpointUrl */
    for(size_t i = 0; i < response->serverEndpointsSize; ++i) {
        UA_String_clear(&response->serverEndpoints[i].endpointUrl);
        response->responseHeader.serviceResult |=
            UA_String_copy(&request->endpointUrl,
                           &response->serverEndpoints[i].endpointUrl);
    }

    /* Fill the session information */
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize = channel->config.localMaxMessageSize;
    response->responseHeader.serviceResult |=
        UA_ApplicationDescription_copy(&request->clientDescription,
                                       &newSession->clientDescription);

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->header.authenticationToken;
    response->responseHeader.serviceResult |=
        UA_String_copy(&request->sessionName, &newSession->sessionName);

#ifdef UA_ENABLE_DIAGNOSTICS
    response->responseHeader.serviceResult |=
        UA_String_copy(&request->serverUri, &newSession->diagnostics.serverUri);
    response->responseHeader.serviceResult |=
        UA_String_copy(&request->endpointUrl, &newSession->diagnostics.endpointUrl);
#endif

    UA_ByteString_init(&response->serverCertificate);

    if(server->config.endpointsSize > 0)
       for(size_t i = 0; i < response->serverEndpointsSize; ++i) {
          if(response->serverEndpoints[i].securityMode==channel->securityMode &&
             UA_ByteString_equal(&response->serverEndpoints[i].securityPolicyUri,
                                 &channel->securityPolicy->policyUri) &&
             UA_String_equal(&response->serverEndpoints[i].endpointUrl,
                             &request->endpointUrl))
          {
             response->responseHeader.serviceResult |=
                 UA_ByteString_copy(&response->serverEndpoints[i].serverCertificate,
                                    &response->serverCertificate);
          }
       }

    /* Create a session nonce */
    response->responseHeader.serviceResult |= UA_Session_generateNonce(newSession);
    response->responseHeader.serviceResult |=
        UA_ByteString_copy(&newSession->serverNonce, &response->serverNonce);

    /* Sign the signature */
    response->responseHeader.serviceResult |=
       signCreateSessionResponse(server, channel, request, response);

    /* Failure -> remove the session */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Server_removeSessionByToken(server, &newSession->header.authenticationToken,
                                       UA_DIAGNOSTICEVENT_REJECT);
        return;
    }

#ifdef UA_ENABLE_DIAGNOSTICS
    newSession->diagnostics.clientConnectionTime = UA_DateTime_now();
    newSession->diagnostics.clientLastContactTime =
        newSession->diagnostics.clientConnectionTime;

    /* Create the object in the information model */
    createSessionObject(server, newSession);
#endif

    UA_LOG_INFO_SESSION(&server->config.logger, newSession, "Session created");
}

static UA_StatusCode
checkSignature(const UA_Server *server, const UA_SecurityPolicy *securityPolicy,
               void *channelContext, const UA_ByteString *serverNonce,
               const UA_SignatureData *signature) {
    /* Check for zero signature length */
    if(signature->signature.length == 0)
        return UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;

    if(!securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_ByteString *localCertificate = &securityPolicy->localCertificate;
    /* Data to verify is calculated by appending the serverNonce to the local certificate */
    UA_ByteString dataToVerify;
    size_t dataToVerifySize = localCertificate->length + serverNonce->length;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&dataToVerify, dataToVerifySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    memcpy(dataToVerify.data, localCertificate->data, localCertificate->length);
    memcpy(dataToVerify.data + localCertificate->length,
           serverNonce->data, serverNonce->length);
    retval = securityPolicy->certificateSigningAlgorithm.
        verify(channelContext, &dataToVerify, &signature->signature);
    UA_ByteString_clear(&dataToVerify);
    return retval;
}

#ifdef UA_ENABLE_ENCRYPTION

static UA_StatusCode
decryptPassword(UA_SecurityPolicy *securityPolicy, void *tempChannelContext,
                const UA_ByteString *serverNonce, UA_UserNameIdentityToken *userToken) {
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc =
        &securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm;
    if(!UA_String_equal(&userToken->encryptionAlgorithm, &asymEnc->uri))
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    UA_UInt32 tokenSecretLength;
    UA_ByteString decryptedTokenSecret, tokenServerNonce;
    size_t tokenpos = 0;
    size_t offset = 0;
    if(UA_ByteString_copy(&userToken->password, &decryptedTokenSecret) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    UA_StatusCode retval = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    if(asymEnc->decrypt(tempChannelContext, &decryptedTokenSecret) != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_UInt32_decodeBinary(&decryptedTokenSecret, &offset, &tokenSecretLength);

    /* The decrypted data must be large enough to include the Encrypted Token
     * Secret Format and the length field must indicate enough data to include
     * the server nonce. */
    if(decryptedTokenSecret.length < sizeof(UA_UInt32) + serverNonce->length ||
       decryptedTokenSecret.length < sizeof(UA_UInt32) + tokenSecretLength ||
       tokenSecretLength < serverNonce->length)
        goto cleanup;

    /* If the Encrypted Token Secret contains padding, the padding must be
     * zeroes according to the 1.04.1 specification errata, chapter 3. */
    for(size_t i = sizeof(UA_UInt32) + tokenSecretLength; i < decryptedTokenSecret.length; i++) {
        if(decryptedTokenSecret.data[i] != 0)
            goto cleanup;
    }

    /* The server nonce must match according to the 1.04.1 specification errata,
     * chapter 3. */
    tokenpos = sizeof(UA_UInt32) + tokenSecretLength - serverNonce->length;
    tokenServerNonce.length = serverNonce->length;
    tokenServerNonce.data = &decryptedTokenSecret.data[tokenpos];
    if(!UA_ByteString_equal(serverNonce, &tokenServerNonce))
        goto cleanup;

    /* The password was decrypted successfully. Replace usertoken with the
     * decrypted password. The encryptionAlgorithm and policyId fields are left
     * in the UserToken as an indication for the AccessControl plugin that
     * evaluates the decrypted content. */
    memcpy(userToken->password.data, &decryptedTokenSecret.data[sizeof(UA_UInt32)],
           tokenSecretLength - serverNonce->length);
    userToken->password.length = tokenSecretLength - serverNonce->length;
    retval = UA_STATUSCODE_GOOD;

 cleanup:
    UA_ByteString_clear(&decryptedTokenSecret);
    return retval;
}
#endif

static void
selectEndpointAndTokenPolicy(UA_Server *server, UA_SecureChannel *channel,
                             const UA_ExtensionObject *identityToken,
                             const UA_EndpointDescription **ed,
                             const UA_UserTokenPolicy **utp) {
    for(size_t i = 0; i < server->config.endpointsSize; ++i) {
        const UA_EndpointDescription *desc = &server->config.endpoints[i];

        /* Match the Security Mode */
        if(desc->securityMode != channel->securityMode)
            continue;

        /* Match the SecurityPolicy of the endpoint with the current channel */
        if(!UA_String_equal(&desc->securityPolicyUri, &channel->securityPolicy->policyUri))
            continue;

        /* Match the UserTokenType */
        const UA_DataType *tokenDataType = identityToken->content.decoded.type;
        for(size_t j = 0; j < desc->userIdentityTokensSize; j++) {
            const UA_UserTokenPolicy *pol = &desc->userIdentityTokens[j];

            /* Part 4, Section 5.6.3.2, Table 17: A NULL or empty
             * UserIdentityToken should be treated as Anonymous */
            if(identityToken->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY &&
               pol->tokenType == UA_USERTOKENTYPE_ANONYMOUS) {
                *ed = desc;
                *utp = pol;
                return;
            }

            /* Expect decoded content */
            if(!tokenDataType)
                continue;

            if(pol->tokenType == UA_USERTOKENTYPE_ANONYMOUS) {
                if(tokenDataType != &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN])
                    continue;
            } else if(pol->tokenType == UA_USERTOKENTYPE_USERNAME) {
                if(tokenDataType != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])
                    continue;
            } else if(pol->tokenType == UA_USERTOKENTYPE_CERTIFICATE) {
                if(tokenDataType != &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN])
                    continue;
            } else if(pol->tokenType == UA_USERTOKENTYPE_ISSUEDTOKEN) {
                if(tokenDataType != &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN])
                    continue;
            } else {
                continue;
            }

            /* All valid token data types start with a string policyId */
            UA_AnonymousIdentityToken *token = (UA_AnonymousIdentityToken*)
                identityToken->content.decoded.data;

            if(!UA_String_equal(&pol->policyId, &token->policyId))
                continue;

            /* Match found */
            *ed = desc;
            *utp = pol;
            return;
        }
    }
}

#ifdef UA_ENABLE_DIAGNOSTICS
static UA_StatusCode
saveClientUserId(const UA_ExtensionObject *userIdentityToken,
                 UA_SessionSecurityDiagnosticsDataType *diag) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_String_clear(&diag->clientUserIdOfSession);
    if(userIdentityToken->encoding != UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_GOOD;

    if(userIdentityToken->content.decoded.type ==
       &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        /* String of length 0 */
    } else if(userIdentityToken->content.decoded.type ==
       &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken*)
            userIdentityToken->content.decoded.data;
        res = UA_String_copy(&userToken->userName, &diag->clientUserIdOfSession);
    } else if(userIdentityToken->content.decoded.type ==
       &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
        /* TODO: return the X509 Subject Name of the certificate */
    } else {
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    if(res != UA_STATUSCODE_GOOD)
        return res;

    return UA_Array_appendCopy((void**)&diag->clientUserIdHistory,
                               &diag->clientUserIdHistorySize,
                               &diag->clientUserIdOfSession,
                               &UA_TYPES[UA_TYPES_STRING]);
}
#endif


/* TODO: Check all of the following: The Server shall verify that the
 * Certificate the Client used to create the new SecureChannel is the same as
 * the Certificate used to create the original SecureChannel. In addition, the
 * Server shall verify that the Client supplied a UserIdentityToken that is
 * identical to the token currently associated with the Session. Once the Server
 * accepts the new SecureChannel it shall reject requests sent via the old
 * SecureChannel. */

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        const UA_ActivateSessionRequest *request,
                        UA_ActivateSessionResponse *response) {
    const UA_EndpointDescription *ed = NULL;
    const UA_UserTokenPolicy *utp = NULL;
    UA_String *tmpLocaleIds;
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_Session *session = getSessionByToken(server, &request->requestHeader.authenticationToken);
    if(!session) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "ActivateSession: Session not found");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto rejected;
    }

    /* Part 4, §5.6.3: When the ActivateSession Service is called for the
     * first time then the Server shall reject the request if the
     * SecureChannel is not same as the one associated with the
     * CreateSession request. Subsequent calls to ActivateSession may be
     * associated with different SecureChannels. */
    if(!session->activated && session->header.channel != channel) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "ActivateSession: The Session has to be initially activated "
                               "on the SecureChannel that created it");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto rejected;
    }

    /* Has the session timed out? */
    if(session->validTill < UA_DateTime_nowMonotonic()) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "ActivateSession: The Session has timed out");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto rejected;
    }

    /* Check the client signature */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        response->responseHeader.serviceResult =
            checkSignature(server, channel->securityPolicy, channel->channelContext,
                           &session->serverNonce, &request->clientSignature);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(&server->config.logger, session,
                                   "ActivateSession: Client signature check failed with StatusCode %s",
                                   UA_StatusCode_name(response->responseHeader.serviceResult));
            goto securityRejected;
        }
    }

    /* Find the matching Endpoint with UserTokenPolicy */
    selectEndpointAndTokenPolicy(server, channel, &request->userIdentityToken, &ed, &utp);
    if(!ed) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        goto rejected;
    }

    /* If it is a UserNameIdentityToken, the password may be encrypted */
    if(utp->tokenType == UA_USERTOKENTYPE_USERNAME) {
       UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken *)
           request->userIdentityToken.content.decoded.data;

       /* If the userTokenPolicy doesn't specify a security policy the security
        * policy of the secure channel is used. */
       UA_SecurityPolicy* securityPolicy;
       if(!utp->securityPolicyUri.data)
           securityPolicy = getSecurityPolicyByUri(server, &ed->securityPolicyUri);
       else
           securityPolicy = getSecurityPolicyByUri(server, &utp->securityPolicyUri);
       if(!securityPolicy) {
          response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
          goto securityRejected;
       }

       /* Test if the encryption algorithm is correctly specified */
       if(!UA_String_equal(&userToken->encryptionAlgorithm,
                           &securityPolicy->asymmetricModule.cryptoModule.encryptionAlgorithm.uri)) {
           response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
           goto securityRejected;
       }

#ifdef UA_ENABLE_ENCRYPTION
       /* Encrypted password? */
       if(!UA_String_equal(&securityPolicy->policyUri, &UA_SECURITY_POLICY_NONE_URI)) {
           /* Create a temporary channel context if a different SecurityPolicy is
            * used for the password from the SecureChannel */
           void *tempChannelContext = channel->channelContext;
           if(securityPolicy != channel->securityPolicy) {
               /* We use our own certificate to create a temporary channel
                * context. Because the client does not provide one in a #None
                * SecureChannel. We should not need a ChannelContext at all for
                * asymmetric decryption where the remote certificate is not
                * used. */
               UA_UNLOCK(&server->serviceMutex);
               response->responseHeader.serviceResult = securityPolicy->channelModule.
                   newContext(securityPolicy, &securityPolicy->localCertificate,
                              &tempChannelContext);
               UA_LOCK(&server->serviceMutex);
               if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                   UA_LOG_WARNING_SESSION(&server->config.logger, session, "ActivateSession: "
                                          "Failed to create a context for the SecurityPolicy %.*s",
                                          (int)securityPolicy->policyUri.length,
                                          securityPolicy->policyUri.data);
                   goto securityRejected;
               }
           }

           /* Decrypt */
           response->responseHeader.serviceResult =
               decryptPassword(securityPolicy, tempChannelContext, &session->serverNonce, userToken);

           /* Remove the temporary channel context */
           if(securityPolicy != channel->securityPolicy) {
               UA_UNLOCK(&server->serviceMutex);
               securityPolicy->channelModule.deleteContext(tempChannelContext);
               UA_LOCK(&server->serviceMutex);
           }
       } else if(userToken->encryptionAlgorithm.length != 0) {
           /* If SecurityPolicy is None there shall be no EncryptionAlgorithm  */
           response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
           goto securityRejected;
       }

       if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
           UA_LOG_WARNING_SESSION(&server->config.logger, session, "ActivateSession: "
                                  "Failed to decrypt the password with the StatusCode %s",
                                  UA_StatusCode_name(response->responseHeader.serviceResult));
           goto securityRejected;
       }
#endif
    }

#ifdef UA_ENABLE_ENCRYPTION
    /* If it is a X509IdentityToken, check the userTokenSignature. Note this
     * only validates that the user has the corresponding private key for the
     * given user cetificate. Checking whether the user certificate is trusted
     * has to be implemented in the access control plugin. The entire token is
     * forwarded in the call to ActivateSession. */
    if(utp->tokenType == UA_USERTOKENTYPE_CERTIFICATE) {
        UA_X509IdentityToken* userCertToken = (UA_X509IdentityToken*)
            request->userIdentityToken.content.decoded.data;

        /* If the userTokenPolicy doesn't specify a security policy the security
         * policy of the secure channel is used. */
        UA_SecurityPolicy* securityPolicy;
        if(!utp->securityPolicyUri.data)
            securityPolicy = getSecurityPolicyByUri(server, &ed->securityPolicyUri);
        else
            securityPolicy = getSecurityPolicyByUri(server, &utp->securityPolicyUri);
        if(!securityPolicy) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
            goto securityRejected;
        }

        /* We need a channel context with the user certificate in order to reuse
         * the signature checking code. */
        void *tempChannelContext;
        UA_UNLOCK(&server->serviceMutex);
        response->responseHeader.serviceResult = securityPolicy->channelModule.
            newContext(securityPolicy, &userCertToken->certificateData, &tempChannelContext);
        UA_LOCK(&server->serviceMutex);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(&server->config.logger, session, "ActivateSession: "
                                   "Failed to create a context for the SecurityPolicy %.*s",
                                   (int)securityPolicy->policyUri.length,
                                   securityPolicy->policyUri.data);
            goto securityRejected;
        }

        /* Check the user token signature */
        response->responseHeader.serviceResult =
            checkSignature(server, channel->securityPolicy, tempChannelContext,
                           &session->serverNonce, &request->userTokenSignature);

        /* Delete the temporary channel context */
        UA_UNLOCK(&server->serviceMutex);
        securityPolicy->channelModule.deleteContext(tempChannelContext);
        UA_LOCK(&server->serviceMutex);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(&server->config.logger, session,
                "ActivateSession: User token signature check failed with StatusCode %s",
                UA_StatusCode_name(response->responseHeader.serviceResult));
            goto securityRejected;
        }
    }
#endif

    /* Callback into userland access control */
    UA_UNLOCK(&server->serviceMutex);
    response->responseHeader.serviceResult = server->config.accessControl.
        activateSession(server, &server->config.accessControl, ed, &channel->remoteCertificate,
                        &session->sessionId, &request->userIdentityToken, &session->sessionHandle);
    UA_LOCK(&server->serviceMutex);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session, "ActivateSession: The AccessControl "
                               "plugin denied the activation with the StatusCode %s",
                               UA_StatusCode_name(response->responseHeader.serviceResult));
        goto securityRejected;
    }

    /* Attach the session to the currently used channel if the session isn't
     * attached to a channel or if the session is activated on a different
     * channel than it is attached to. */
    if(!session->header.channel || session->header.channel != channel) {
        /* Attach the new SecureChannel, the old channel will be detached if present */
        UA_Session_attachToSecureChannel(session, channel);
        UA_LOG_INFO_SESSION(&server->config.logger, session,
                            "ActivateSession: Session attached to new channel");
    }

    /* Generate a new session nonce for the next time ActivateSession is called */
    response->responseHeader.serviceResult = UA_Session_generateNonce(session);
    response->responseHeader.serviceResult |=
        UA_ByteString_copy(&session->serverNonce, &response->serverNonce);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_detachFromSecureChannel(session);
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "ActivateSession: Could not generate the server nonce");
        goto rejected;
    }

    /* Set the locale */
    response->responseHeader.serviceResult |=
        UA_Array_copy(request->localeIds, request->localeIdsSize,
                      (void**)&tmpLocaleIds, &UA_TYPES[UA_TYPES_STRING]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_detachFromSecureChannel(session);
        UA_LOG_WARNING_SESSION(&server->config.logger, session,
                               "ActivateSession: Could not store the Session LocaleIds");
        goto rejected;
    }
    UA_Array_delete(session->localeIds, session->localeIdsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    session->localeIds = tmpLocaleIds;
    session->localeIdsSize = request->localeIdsSize;

    UA_Session_updateLifetime(session);

    /* Activate the session */
    if(!session->activated) {
        session->activated = true;
        server->activeSessionCount++;
        server->serverDiagnosticsSummary.cumulatedSessionCount++;
    }

#ifdef UA_ENABLE_DIAGNOSTICS
    saveClientUserId(&request->userIdentityToken,
                     &session->securityDiagnostics);
    UA_String_clear(&session->securityDiagnostics.authenticationMechanism);
    switch(utp->tokenType) {
    case UA_USERTOKENTYPE_ANONYMOUS:
        session->securityDiagnostics.authenticationMechanism = UA_STRING_ALLOC("Anonymous");
        break;
    case UA_USERTOKENTYPE_USERNAME:
        session->securityDiagnostics.authenticationMechanism = UA_STRING_ALLOC("UserName");
        break;
    case UA_USERTOKENTYPE_CERTIFICATE:
        session->securityDiagnostics.authenticationMechanism = UA_STRING_ALLOC("Certificate");
        break;
    case UA_USERTOKENTYPE_ISSUEDTOKEN:
        session->securityDiagnostics.authenticationMechanism = UA_STRING_ALLOC("IssuedToken");
        break;
    default: break;
    }
#endif

    UA_LOG_INFO_SESSION(&server->config.logger, session, "ActivateSession: Session activated");
    return;

securityRejected:
    server->serverDiagnosticsSummary.securityRejectedSessionCount++;
rejected:
    server->serverDiagnosticsSummary.rejectedSessionCount++;
}

void
Service_CloseSession(UA_Server *server, UA_SecureChannel *channel,
                     const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Part 4, 5.6.4: When the CloseSession Service is called before the Session
     * is successfully activated, the Server shall reject the request if the
     * SecureChannel is not the same as the one associated with the
     * CreateSession request.
     *
     * A non-activated Session is already bound to the SecureChannel that
     * created the Session. */
    UA_Session *session = NULL;
    response->responseHeader.serviceResult =
        getBoundSession(server, channel, &request->requestHeader.authenticationToken, &session);
    if(!session && response->responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(&server->config.logger, channel,
                               "CloseSession: No Session activated to the SecureChannel");
        return;
    }

    UA_assert(session); /* Assured by the previous section */
    UA_LOG_INFO_SESSION(&server->config.logger, session, "Closing the Session");

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* If Subscriptions are not deleted, detach them from the Session */
    if(!request->deleteSubscriptions) {
        UA_Subscription *sub, *sub_tmp;
        TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, sub_tmp) {
            UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, sub,
                                     "Detaching the Subscription from the Session");
            UA_Session_detachSubscription(server, session, sub, true);
        }
    }
#endif

    /* Remove the sesison */
    response->responseHeader.serviceResult =
        UA_Server_removeSessionByToken(server, &session->header.authenticationToken,
                                       UA_DIAGNOSTICEVENT_CLOSE);
}
