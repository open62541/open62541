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
    UA_free(entry);
}

void
UA_Server_removeSession(UA_Server *server, session_list_entry *sentry,
                        UA_ShutdownReason shutdownReason) {
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
        server->config.accessControl.
            closeSession(server, &server->config.accessControl,
                         &session->sessionId, session->context);
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

    switch(shutdownReason) {
    case UA_SHUTDOWNREASON_CLOSE:
    case UA_SHUTDOWNREASON_PURGE:
        break;
    case UA_SHUTDOWNREASON_TIMEOUT:
        server->serverDiagnosticsSummary.sessionTimeoutCount++;
        break;
    case UA_SHUTDOWNREASON_REJECT:
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        break;
    case UA_SHUTDOWNREASON_SECURITYREJECT:
        server->serverDiagnosticsSummary.securityRejectedSessionCount++;
        break;
    case UA_SHUTDOWNREASON_ABORT:
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
    UA_EventLoop *el = server->config.eventLoop;
    el->addDelayedCallback(el, &sentry->cleanupCallback);
}

UA_StatusCode
UA_Server_removeSessionByToken(UA_Server *server, const UA_NodeId *token,
                               UA_ShutdownReason shutdownReason) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    session_list_entry *entry;
    LIST_FOREACH(entry, &server->sessions, pointers) {
        if(UA_NodeId_equal(&entry->session.authenticationToken, token)) {
            UA_Server_removeSession(server, entry, shutdownReason);
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
        UA_LOG_INFO_SESSION(server->config.logging, &sentry->session,
                            "Session has timed out");
        UA_Server_removeSession(server, sentry, UA_SHUTDOWNREASON_TIMEOUT);
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
        if(!UA_NodeId_equal(&current->session.authenticationToken, token))
            continue;

        /* Session has timed out */
        UA_EventLoop *el = server->config.eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        if(now > current->session.validTill) {
            UA_LOG_INFO_SESSION(server->config.logging, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        return &current->session;
    }

    return NULL;
}

UA_Session *
getSessionById(UA_Server *server, const UA_NodeId *sessionId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    session_list_entry *current = NULL;
    LIST_FOREACH(current, &server->sessions, pointers) {
        /* Token does not match */
        if(!UA_NodeId_equal(&current->session.sessionId, sessionId))
            continue;

        /* Session has timed out */
        UA_EventLoop *el = server->config.eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        if(now > current->session.validTill) {
            UA_LOG_INFO_SESSION(server->config.logging, &current->session,
                                "Client tries to use a session that has timed out");
            return NULL;
        }

        return &current->session;
    }

    if(UA_NodeId_equal(sessionId, &server->adminSession.sessionId))
        return &server->adminSession;

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
    const UA_SecurityPolicySignatureAlgorithm *signAlg =
        &securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm;
    size_t signatureSize = signAlg->getLocalSignatureSize(channel->channelContext);
    UA_StatusCode retval = UA_String_copy(&signAlg->uri, &signatureData->algorithm);
    retval |= UA_ByteString_allocBuffer(&signatureData->signature, signatureSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Allocate a temp buffer */
    size_t dataToSignSize =
        request->clientCertificate.length + request->clientNonce.length;
    UA_ByteString dataToSign;
    retval = UA_ByteString_allocBuffer(&dataToSign, dataToSignSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval; /* signatureData->signature is cleaned up with the response */

    /* Sign the signature */
    memcpy(dataToSign.data, request->clientCertificate.data,
           request->clientCertificate.length);
    memcpy(dataToSign.data + request->clientCertificate.length,
           request->clientNonce.data, request->clientNonce.length);
    retval = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
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
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
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
    newentry->session.authenticationToken = UA_NODEID_GUID(1, UA_Guid_random());

    newentry->session.timeout = server->config.maxSessionTimeout;
    if(request->requestedSessionTimeout <= server->config.maxSessionTimeout &&
       request->requestedSessionTimeout > 0)
        newentry->session.timeout = request->requestedSessionTimeout;

    /* Attach the session to the channel. But don't activate for now. */
    if(channel)
        UA_Session_attachToSecureChannel(&newentry->session, channel);

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime now = el->dateTime_now(el);
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_Session_updateLifetime(&newentry->session, now, nowMonotonic);

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
    UA_LOG_DEBUG_CHANNEL(server->config.logging, channel, "Trying to create session");

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
            UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
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
        response->responseHeader.serviceResult =
            UA_CertificateUtils_verifyApplicationURI(server->config.allowAllCertificateUris,
                                                     &request->clientCertificate,
                                                     &request->clientDescription.applicationUri);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                                   "The client's ApplicationURI did not match the certificate");
            server->serverDiagnosticsSummary.securityRejectedSessionCount++;
            server->serverDiagnosticsSummary.rejectedSessionCount++;
            return;
        }
    }

    /* Create the Session */
    UA_Session *newSession = NULL;
    response->responseHeader.serviceResult =
        UA_Server_createSession(server, channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "Processing CreateSessionRequest failed");
        server->serverDiagnosticsSummary.rejectedSessionCount++;
        return;
    }

    /* If the session name is empty, use the generated SessionId */
    response->responseHeader.serviceResult |=
        UA_String_copy(&request->sessionName, &newSession->sessionName);
    if(newSession->sessionName.length == 0)
        response->responseHeader.serviceResult |=
            UA_NodeId_print(&newSession->sessionId, &newSession->sessionName);

    response->responseHeader.serviceResult |= UA_Session_generateNonce(newSession);
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    newSession->maxRequestMessageSize = channel->config.localMaxMessageSize;
    response->responseHeader.serviceResult |=
        UA_ApplicationDescription_copy(&request->clientDescription,
                                       &newSession->clientDescription);

#ifdef UA_ENABLE_DIAGNOSTICS
    response->responseHeader.serviceResult |=
        UA_String_copy(&request->serverUri, &newSession->diagnostics.serverUri);
    response->responseHeader.serviceResult |=
        UA_String_copy(&request->endpointUrl, &newSession->diagnostics.endpointUrl);
#endif

    /* Prepare the response */
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    response->responseHeader.serviceResult |=
        UA_ByteString_copy(&newSession->serverNonce, &response->serverNonce);

    /* Copy the server's endpointdescriptions into the response */
    response->responseHeader.serviceResult =
        setCurrentEndPointsArray(server, request->endpointUrl, NULL, 0,
                                 &response->serverEndpoints,
                                 &response->serverEndpointsSize);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Server_removeSessionByToken(server, &newSession->authenticationToken,
                                       UA_SHUTDOWNREASON_REJECT);
        return;
    }

    /* Return the server certificate from the SecurityPolicy of the current
     * channel. Or, if the channel is unencrypted, return the standard policy
     * used for usertoken encryption. */
    const UA_SecurityPolicy *sp = channel->securityPolicy;
    if(UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &sp->policyUri) ||
       sp->localCertificate.length == 0)
        sp = getDefaultEncryptedSecurityPolicy(server);
    if(sp)
        response->responseHeader.serviceResult |=
            UA_ByteString_copy(&sp->localCertificate, &response->serverCertificate);

    /* Sign the signature */
    response->responseHeader.serviceResult |=
       signCreateSessionResponse(server, channel, request, response);

    /* Failure -> remove the session */
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Server_removeSessionByToken(server, &newSession->authenticationToken,
                                       UA_SHUTDOWNREASON_REJECT);
        return;
    }

#ifdef UA_ENABLE_DIAGNOSTICS
    UA_EventLoop *el = server->config.eventLoop;
    newSession->diagnostics.clientConnectionTime = el->dateTime_now(el);
    newSession->diagnostics.clientLastContactTime =
        newSession->diagnostics.clientConnectionTime;

    /* Create the object in the information model */
    createSessionObject(server, newSession);
#endif

    UA_LOG_INFO_SESSION(server->config.logging, newSession, "Session created");
}

static UA_StatusCode
checkCertificateSignature(const UA_Server *server, const UA_SecurityPolicy *securityPolicy,
                          void *channelContext, const UA_ByteString *serverNonce,
                          const UA_SignatureData *signature,
                          const bool isUserTokenSignature) {
    /* Check for zero signature length */
    if(signature->signature.length == 0) {
        if(isUserTokenSignature)
            return UA_STATUSCODE_BADUSERSIGNATUREINVALID;
        return UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    }

    if(!securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Server certificate */
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
    retval = securityPolicy->asymmetricModule.cryptoModule.signatureAlgorithm.
        verify(channelContext, &dataToVerify, &signature->signature);
    UA_ByteString_clear(&dataToVerify);
    if(retval != UA_STATUSCODE_GOOD) {
        if(isUserTokenSignature)
            retval = UA_STATUSCODE_BADUSERSIGNATUREINVALID;
        else
            retval = UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    }
    return retval;
}

static void
selectEndpointAndTokenPolicy(UA_Server *server, UA_SecureChannel *channel,
                             const UA_ExtensionObject *identityToken,
                             const UA_EndpointDescription **ed,
                             const UA_UserTokenPolicy **utp,
                             const UA_SecurityPolicy **tokenSp) {
    for(size_t i = 0; i < server->config.endpointsSize; ++i) {
        const UA_EndpointDescription *desc = &server->config.endpoints[i];

        /* Match the Security Mode */
        if(desc->securityMode != channel->securityMode)
            continue;

        /* Match the SecurityPolicy of the endpoint with the current channel */
        if(!UA_String_equal(&desc->securityPolicyUri, &channel->securityPolicy->policyUri))
            continue;

        /* If no UserTokenPolicies are configured in the endpoint, then use
         * those of the AccessControl plugin. */
        size_t identPoliciesSize = desc->userIdentityTokensSize;
        const UA_UserTokenPolicy *identPolicies = desc->userIdentityTokens;
        if(identPoliciesSize == 0) {
            identPoliciesSize = server->config.accessControl.userTokenPoliciesSize;
            identPolicies = server->config.accessControl.userTokenPolicies;
        }

        /* Match the UserTokenType */
        const UA_DataType *tokenDataType = identityToken->content.decoded.type;
        for(size_t j = 0; j < identPoliciesSize ; j++) {
            const UA_UserTokenPolicy *pol = &identPolicies[j];

            if(!UA_String_equal(&desc->securityPolicyUri, &pol->securityPolicyUri))
                continue;

            /* Part 4, Section 5.6.3.2, Table 17: A NULL or empty
             * UserIdentityToken should be treated as Anonymous */
            if(identityToken->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY &&
               pol->tokenType == UA_USERTOKENTYPE_ANONYMOUS) {
                *ed = desc;
                *utp = pol;
                return;
            }

            /* Expect decoded content if not anonymous */
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

            /* In setCurrentEndPointsArray we prepend the policyId with the
             * security mode to make it unique. Remove that here. */
            if(pol->policyId.length > token->policyId.length)
                continue;
            UA_String tmpId = token->policyId;
            tmpId.length = pol->policyId.length;
            if(!UA_String_equal(&tmpId, &pol->policyId))
                continue;

            /* Match found */
            *ed = desc;
            *utp = pol;

            /* Set the SecurityPolicy used to encrypt the token. If the
             * userTokenPolicy doesn't specify a security policy the security
             * policy of the secure channel is used. */
            *tokenSp = channel->securityPolicy;
            if(pol->securityPolicyUri.length > 0)
                *tokenSp = getSecurityPolicyByUri(server, &pol->securityPolicyUri);

            /* If the server does not allow unencrypted passwords, select the
             * default encrypted policy for the UserTokenPolicy */
#ifdef UA_ENABLE_ENCRYPTION
            if(!*tokenSp ||
               (!server->config.allowNonePolicyPassword &&
                ((*tokenSp)->localCertificate.length == 0 ||
                 UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &(*tokenSp)->policyUri))))
                *tokenSp = getDefaultEncryptedSecurityPolicy(server);
#endif

            /* Found SecurityPolicy and UserTokenPoliy. Stop here. */
            return;
        }
    }
}

static UA_StatusCode
decryptUserNamePW(UA_Server *server, UA_Session *session,
                  const UA_SecurityPolicy *sp,
                  UA_UserNameIdentityToken *userToken) {
    /* If SecurityPolicy is None there shall be no EncryptionAlgorithm  */
    if(UA_String_equal(&sp->policyUri, &UA_SECURITY_POLICY_NONE_URI)) {
        if(userToken->encryptionAlgorithm.length > 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        UA_LOG_WARNING_SESSION(server->config.logging, session, "ActivateSession: "
                               "Received an unencrypted username/passwort. "
                               "Is the server misconfigured to allow that?");
        return UA_STATUSCODE_GOOD;
    }

    /* Test if the correct encryption algorithm is used */
    if(!UA_String_equal(&userToken->encryptionAlgorithm,
                        &sp->asymmetricModule.cryptoModule.encryptionAlgorithm.uri))
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* Encrypted password -- Create a temporary channel context.
     * TODO: We should not need a ChannelContext at all for asymmetric
     * decryption where the remote certificate is not used. */
    void *tempChannelContext = NULL;
    UA_UNLOCK(&server->serviceMutex);
    UA_StatusCode res =
        sp->channelModule.newContext(sp, &sp->localCertificate, &tempChannelContext);
    UA_LOCK(&server->serviceMutex);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Failed to create a "
                               "context for the SecurityPolicy %.*s",
                               (int)sp->policyUri.length,
                               sp->policyUri.data);
        return res;
    }

    UA_UInt32 secretLen = 0;
    UA_ByteString secret, tokenNonce;
    size_t tokenpos = 0;
    size_t offset = 0;
    UA_ByteString *sn = &session->serverNonce;
    const UA_SecurityPolicyEncryptionAlgorithm *asymEnc =
        &sp->asymmetricModule.cryptoModule.encryptionAlgorithm;

    res = UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* Decrypt the secret */
    if(UA_ByteString_copy(&userToken->password, &secret) != UA_STATUSCODE_GOOD ||
       asymEnc->decrypt(tempChannelContext, &secret) != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* The secret starts with a UInt32 length for the content */
    if(UA_UInt32_decodeBinary(&secret, &offset,
                              &secretLen) != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* The decrypted data must be large enough to include the Encrypted Token
     * Secret Format and the length field must indicate enough data to include
     * the server nonce. */
    if(secret.length < sizeof(UA_UInt32) + sn->length ||
       secret.length < sizeof(UA_UInt32) + secretLen ||
       secretLen < sn->length)
        goto cleanup;

    /* If the Encrypted Token Secret contains padding, the padding must be
     * zeroes according to the 1.04.1 specification errata, chapter 3. */
    for(size_t i = sizeof(UA_UInt32) + secretLen; i < secret.length; i++) {
        if(secret.data[i] != 0)
            goto cleanup;
    }

    /* The server nonce must match according to the 1.04.1 specification errata,
     * chapter 3. */
    tokenpos = sizeof(UA_UInt32) + secretLen - sn->length;
    tokenNonce.length = sn->length;
    tokenNonce.data = &secret.data[tokenpos];
    if(!UA_ByteString_equal(sn, &tokenNonce))
        goto cleanup;

    /* The password was decrypted successfully. Replace usertoken with the
     * decrypted password. The encryptionAlgorithm and policyId fields are left
     * in the UserToken as an indication for the AccessControl plugin that
     * evaluates the decrypted content. */
    memcpy(userToken->password.data,
           &secret.data[sizeof(UA_UInt32)], secretLen - sn->length);
    userToken->password.length = secretLen - sn->length;
    res = UA_STATUSCODE_GOOD;

 cleanup:
    UA_ByteString_clear(&secret);

    /* Remove the temporary channel context */
    UA_UNLOCK(&server->serviceMutex);
    sp->channelModule.deleteContext(tempChannelContext);
    UA_LOCK(&server->serviceMutex);

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Failed to decrypt the "
                               "password with the StatusCode %s",
                               UA_StatusCode_name(res));
    }
    return res;
}

static UA_StatusCode
checkActivateSessionX509(UA_Server *server, UA_Session *session,
                         const UA_SecurityPolicy *sp, UA_X509IdentityToken* token,
                         const UA_SignatureData *tokenSignature) {
    /* The SecurityPolicy must be None */
    if(UA_String_equal(&sp->policyUri, &UA_SECURITY_POLICY_NONE_URI))
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* We need a channel context with the user certificate in order to reuse
     * the signature checking code. */
    void *tempChannelContext;
    UA_UNLOCK(&server->serviceMutex);
    UA_StatusCode res = sp->channelModule.
        newContext(sp, &token->certificateData, &tempChannelContext);
    UA_LOCK(&server->serviceMutex);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Failed to create a context "
                               "for the SecurityPolicy %.*s",
                               (int)sp->policyUri.length,
                               sp->policyUri.data);
        return res;
    }

    /* Check the user token signature */
    res = checkCertificateSignature(server, sp, tempChannelContext,
                                    &session->serverNonce, tokenSignature, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: User token signature check "
                               "failed with StatusCode %s", UA_StatusCode_name(res));
    }

    /* Delete the temporary channel context */
    UA_UNLOCK(&server->serviceMutex);
    sp->channelModule.deleteContext(tempChannelContext);
    UA_LOCK(&server->serviceMutex);
    return res;
}

/* TODO: Check all of the following: The Server shall verify that the
 * Certificate the Client used to create the new SecureChannel is the same as
 * the Certificate used to create the original SecureChannel. In addition, the
 * Server shall verify that the Client supplied a UserIdentityToken that is
 * identical to the token currently associated with the Session. Once the Server
 * accepts the new SecureChannel it shall reject requests sent via the old
 * SecureChannel. */

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                        const UA_ActivateSessionRequest *req,
                        UA_ActivateSessionResponse *resp) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    const UA_EndpointDescription *ed = NULL;
    const UA_UserTokenPolicy *utp = NULL;
    const UA_SecurityPolicy *tokenSp = NULL;
    UA_String *tmpLocaleIds;

    /* Get the session */
    UA_Session *session = getSessionByToken(server, &req->requestHeader.authenticationToken);
    if(!session) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "ActivateSession: Session not found");
        resp->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto rejected;
    }

    /* Part 4, §5.6.3: When the ActivateSession Service is called for the
     * first time then the Server shall reject the request if the
     * SecureChannel is not same as the one associated with the
     * CreateSession request. Subsequent calls to ActivateSession may be
     * associated with different SecureChannels. */
    if(!session->activated && session->channel != channel) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "ActivateSession: The Session has to be initially activated "
                               "on the SecureChannel that created it");
        resp->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto rejected;
    }

    /* Has the session timed out? */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    if(session->validTill < nowMonotonic) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: The Session has timed out");
        resp->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto rejected;
    }

    /* Check the client signature */
    if(channel->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       channel->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        resp->responseHeader.serviceResult =
            checkCertificateSignature(server, channel->securityPolicy,
                                      channel->channelContext,
                                      &session->serverNonce,
                                      &req->clientSignature, false);
        if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "ActivateSession: Client signature check failed "
                                   "with StatusCode %s",
                                   UA_StatusCode_name(resp->responseHeader.serviceResult));
            goto securityRejected;
        }
    }

    /* Find the matching Endpoint with UserTokenPolicy.
     * Also sets the SecurityPolicy used to encrypt the token. */
    selectEndpointAndTokenPolicy(server, channel, &req->userIdentityToken,
                                 &ed, &utp, &tokenSp);
    if(!ed || !tokenSp) {
        resp->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        goto rejected;
    }

    if(utp->tokenType == UA_USERTOKENTYPE_USERNAME) {
        /* If it is a UserNameIdentityToken, the password may be encrypted */
       UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken *)
           req->userIdentityToken.content.decoded.data;
       resp->responseHeader.serviceResult =
           decryptUserNamePW(server, session, tokenSp, userToken);
       if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
           goto securityRejected;
    } else if(utp->tokenType == UA_USERTOKENTYPE_CERTIFICATE) {
        /* If it is a X509IdentityToken, check the userTokenSignature. Note this
         * only validates that the user has the corresponding private key for
         * the given user cetificate. Checking whether the user certificate is
         * trusted has to be implemented in the access control plugin. The
         * entire token is forwarded in the call to ActivateSession. */
        UA_X509IdentityToken* token = (UA_X509IdentityToken*)
            req->userIdentityToken.content.decoded.data;
       resp->responseHeader.serviceResult =
           checkActivateSessionX509(server, session, tokenSp,
                                    token, &req->userTokenSignature);
       if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
           goto securityRejected;
    }

    /* Callback into userland access control */
    UA_UNLOCK(&server->serviceMutex);
    resp->responseHeader.serviceResult = server->config.accessControl.
        activateSession(server, &server->config.accessControl, ed,
                        &channel->remoteCertificate, &session->sessionId,
                        &req->userIdentityToken, &session->context);
    UA_LOCK(&server->serviceMutex);
    if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: The AccessControl "
                               "plugin denied the activation with the StatusCode %s",
                               UA_StatusCode_name(resp->responseHeader.serviceResult));
        goto securityRejected;
    }

    /* Attach the session to the currently used channel if the session isn't
     * attached to a channel or if the session is activated on a different
     * channel than it is attached to. */
    if(!session->channel || session->channel != channel) {
        /* Attach the new SecureChannel, the old channel will be detached if present */
        UA_Session_attachToSecureChannel(session, channel);
        UA_LOG_INFO_SESSION(server->config.logging, session,
                            "ActivateSession: Session attached to new channel");
    }

    /* Generate a new session nonce for the next time ActivateSession is called */
    resp->responseHeader.serviceResult = UA_Session_generateNonce(session);
    resp->responseHeader.serviceResult |=
        UA_ByteString_copy(&session->serverNonce, &resp->serverNonce);
    if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Session_detachFromSecureChannel(session);
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "ActivateSession: Could not generate the server nonce");
        goto rejected;
    }

    /* Set the Locale */
    if(req->localeIdsSize > 0) {
        /* Part 4, §5.6.3.2: This parameter only needs to be specified during
         * the first call to ActivateSession during a single application
         * Session. If it is not specified the Server shall keep using the
         * current localeIds for the Session. */
        resp->responseHeader.serviceResult |=
            UA_Array_copy(req->localeIds, req->localeIdsSize,
                          (void**)&tmpLocaleIds, &UA_TYPES[UA_TYPES_STRING]);
        if(resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_Session_detachFromSecureChannel(session);
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "ActivateSession: Could not store the Session LocaleIds");
            goto rejected;
        }
        UA_Array_delete(session->localeIds, session->localeIdsSize,
                        &UA_TYPES[UA_TYPES_STRING]);
        session->localeIds = tmpLocaleIds;
        session->localeIdsSize = req->localeIdsSize;
    }

    /* Update the Session lifetime */
    nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_DateTime now = el->dateTime_now(el);
    UA_Session_updateLifetime(session, now, nowMonotonic);

    /* Activate the session */
    if(!session->activated) {
        session->activated = true;
        server->activeSessionCount++;
        server->serverDiagnosticsSummary.cumulatedSessionCount++;
    }

    /* Store the ClientUserId. tokenType can be NULL for the anonymous user. */
    UA_String_clear(&session->clientUserIdOfSession);
    const UA_DataType *tokenType = req->userIdentityToken.content.decoded.type;
    if(tokenType == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken*)
            req->userIdentityToken.content.decoded.data;
        UA_String_copy(&userToken->userName, &session->clientUserIdOfSession);
    } else if(tokenType == &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
        UA_X509IdentityToken* userCertToken = (UA_X509IdentityToken*)
            req->userIdentityToken.content.decoded.data;
        UA_CertificateUtils_getSubjectName(&session->clientUserIdOfSession,
                                           &userCertToken->certificateData);
    } else {
        /* TODO: Handle issued token */
    }

#ifdef UA_ENABLE_DIAGNOSTICS
    /* Add the ClientUserId to the diagnostics history */
    UA_SessionSecurityDiagnosticsDataType *ssd = &session->securityDiagnostics;
    UA_StatusCode res =
        UA_Array_appendCopy((void**)&ssd->clientUserIdHistory,
                            &ssd->clientUserIdHistorySize,
                            &ssd->clientUserIdOfSession,
                            &UA_TYPES[UA_TYPES_STRING]);
    (void)res;

    /* Store the auth mechanism */
    UA_String_clear(&ssd->authenticationMechanism);
    switch(utp->tokenType) {
    case UA_USERTOKENTYPE_ANONYMOUS:
        ssd->authenticationMechanism = UA_STRING_ALLOC("Anonymous"); break;
    case UA_USERTOKENTYPE_USERNAME:
        ssd->authenticationMechanism = UA_STRING_ALLOC("UserName"); break;
    case UA_USERTOKENTYPE_CERTIFICATE:
        ssd->authenticationMechanism = UA_STRING_ALLOC("Certificate"); break;
    case UA_USERTOKENTYPE_ISSUEDTOKEN:
        ssd->authenticationMechanism = UA_STRING_ALLOC("IssuedToken"); break;
    default: break;
    }
#endif

    /* Log the user for which the Session was activated */
    UA_LOG_INFO_SESSION(server->config.logging, session,
                        "ActivateSession: Session activated with ClientUserId \"%.*s\"",
                        (int)session->clientUserIdOfSession.length,
                        session->clientUserIdOfSession.data);
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
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "CloseSession: No Session activated to the SecureChannel");
        return;
    }

    UA_assert(session); /* Assured by the previous section */
    UA_LOG_INFO_SESSION(server->config.logging, session, "Closing the Session");

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* If Subscriptions are not deleted, detach them from the Session */
    if(!request->deleteSubscriptions) {
        UA_Subscription *sub, *sub_tmp;
        TAILQ_FOREACH_SAFE(sub, &session->subscriptions, sessionListEntry, sub_tmp) {
            UA_LOG_INFO_SUBSCRIPTION(server->config.logging, sub,
                                     "Detaching the Subscription from the Session");
            UA_Session_detachSubscription(server, session, sub, true);
        }
    }
#endif

    /* Remove the sesison */
    response->responseHeader.serviceResult =
        UA_Server_removeSessionByToken(server, &session->authenticationToken,
                                       UA_SHUTDOWNREASON_CLOSE);
}

void Service_Cancel(UA_Server *server, UA_Session *session,
                    const UA_CancelRequest *request, UA_CancelResponse *response) {
    /* If multithreading is disabled, then there are no async services. If all
     * services are answered "right away", then there are no services that can
     * be cancelled. */
#if UA_MULTITHREADING >= 100
    response->cancelCount = UA_AsyncManager_cancel(server, session, request->requestHandle);
#endif

    /* Publish requests for Subscriptions are stored separately */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_PublishResponseEntry *pre, *pre_tmp;
    UA_PublishResponseEntry *prev = NULL;
    SIMPLEQ_FOREACH_SAFE(pre, &session->responseQueue, listEntry, pre_tmp) {
        /* Skip entry and set as the previous entry that is kept in the list */
        if(pre->response.responseHeader.requestHandle != request->requestHandle) {
            prev = pre;
            continue;
        }

        /* Dequeue */
        if(prev)
            SIMPLEQ_REMOVE_AFTER(&session->responseQueue, prev, listEntry);
        else
            SIMPLEQ_REMOVE_HEAD(&session->responseQueue, listEntry);
        session->responseQueueSize--;

        /* Send response and clean up */
        response->responseHeader.serviceResult = UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;
        sendResponse(server, session->channel, pre->requestId, (UA_Response *)response,
                     &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        UA_PublishResponse_clear(&pre->response);
        UA_free(pre);

        /* Increase the CancelCount */
        response->cancelCount++;
    }
#endif
}
