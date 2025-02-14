/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/server_pubsub.h>
#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

UA_ReaderGroup *
UA_ReaderGroup_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PubSubConnection *psc;
    TAILQ_FOREACH(psc, &psm->connections, listEntry) {
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &psc->readerGroups, listEntry) {
            if(UA_NodeId_equal(&id, &rg->head.identifier))
                return rg;
        }
    }
    return NULL;
}

/* ReaderGroup Config Handling */

UA_StatusCode
UA_ReaderGroupConfig_copy(const UA_ReaderGroupConfig *src,
                          UA_ReaderGroupConfig *dst) {
    memcpy(dst, src, sizeof(UA_ReaderGroupConfig));
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_KeyValueMap_copy(&src->groupProperties, &dst->groupProperties);
    res |= UA_String_copy(&src->securityGroupId, &dst->securityGroupId);
    res |= UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    if(res != UA_STATUSCODE_GOOD)
        UA_ReaderGroupConfig_clear(dst);
    return res;
}

void
UA_ReaderGroupConfig_clear(UA_ReaderGroupConfig *readerGroupConfig) {
    UA_String_clear(&readerGroupConfig->name);
    UA_KeyValueMap_clear(&readerGroupConfig->groupProperties);
    UA_String_clear(&readerGroupConfig->securityGroupId);
    UA_ExtensionObject_clear(&readerGroupConfig->transportSettings);
}

UA_StatusCode
UA_ReaderGroup_create(UA_PubSubManager *psm, UA_NodeId connectionId,
                      const UA_ReaderGroupConfig *rgc,
                      UA_NodeId *readerGroupId) {
    /* Check for valid readergroup configuration */
    if(!psm || !rgc)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionId);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Allocate memory for new reader group and add settings */
    UA_ReaderGroup *newGroup = (UA_ReaderGroup *)UA_calloc(1, sizeof(UA_ReaderGroup));
    if(!newGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newGroup->head.componentType = UA_PUBSUBCOMPONENT_READERGROUP;
    newGroup->linkedConnection = c;

    /* Deep copy of the config */
    UA_StatusCode retval = UA_ReaderGroupConfig_copy(rgc, &newGroup->config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newGroup);
        return retval;
    }

    /* Add to the connection */
    LIST_INSERT_HEAD(&c->readerGroups, newGroup, listEntry);
    c->readerGroupsSize++;

#ifdef UA_ENABLE_PUBSUB_SKS
    if(rgc->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       rgc->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_String_isEmpty(&rgc->securityGroupId) && rgc->securityPolicy) {
            /* Does the key storage already exist? */
            newGroup->keyStorage = UA_PubSubKeyStorage_find(psm, rgc->securityGroupId);

            if(!newGroup->keyStorage) {
                /* Create a new key storage */
                newGroup->keyStorage = (UA_PubSubKeyStorage *)
                    UA_calloc(1, sizeof(UA_PubSubKeyStorage));
                if(!newGroup->keyStorage) {
                    UA_ReaderGroup_remove(psm, newGroup);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                retval = UA_PubSubKeyStorage_init(psm, newGroup->keyStorage,
                                                  &rgc->securityGroupId,
                                                  rgc->securityPolicy, 0, 0);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_ReaderGroup_remove(psm, newGroup);
                    return retval;
                }
            }

            /* Increase the ref count */
            newGroup->keyStorage->referenceCount++;
        }
    }
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval |= addReaderGroupRepresentation(psm->sc.server, newGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReaderGroup_remove(psm, newGroup);
        return retval;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &newGroup->head.identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%SReaderGroup %N\t| ",
                c->head.logIdString, newGroup->head.identifier);
    newGroup->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(psm->logging, newGroup, "ReaderGroup created (State: %s)",
                       UA_PubSubState_name(newGroup->head.state));

    /* Validate the connection settings */
    retval = UA_ReaderGroup_connect(psm, newGroup, true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, newGroup,
                            "Could not validate the connection parameters");
        UA_ReaderGroup_remove(psm, newGroup);
        return retval;
    }

    /* Trigger the connection */
    UA_PubSubConnection_setPubSubState(psm, c, c->head.state);

    /* Copying a numeric NodeId always succeeds */
    if(readerGroupId)
        UA_NodeId_copy(&newGroup->head.identifier, readerGroupId);

    return UA_STATUSCODE_GOOD;
}

void
UA_ReaderGroup_remove(UA_PubSubManager *psm, UA_ReaderGroup *rg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    UA_PubSubConnection *connection = rg->linkedConnection;
    UA_assert(connection);

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    rg->deleteFlag = true;
    UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);

    UA_DataSetReader *dsr, *tmp_dsr;
    LIST_FOREACH_SAFE(dsr, &rg->readers, listEntry, tmp_dsr) {
        UA_DataSetReader_remove(psm, dsr);
    }

    if(rg->config.securityPolicy && rg->securityPolicyContext) {
        rg->config.securityPolicy->deleteContext(rg->securityPolicyContext);
        rg->securityPolicyContext = NULL;
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    if(rg->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(psm, rg->keyStorage);
        rg->keyStorage = NULL;
    }
#endif

    if(rg->recvChannelsSize == 0) {
        /* Unlink from the connection */
        LIST_REMOVE(rg, listEntry);
        connection->readerGroupsSize--;
        rg->linkedConnection = NULL;

        /* Actually remove the ReaderGroup */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
        deleteNode(psm->sc.server, rg->head.identifier, true);
#endif

        UA_LOG_INFO_PUBSUB(psm->logging, rg, "ReaderGroup deleted");

        UA_ReaderGroupConfig_clear(&rg->config);
        UA_PubSubComponentHead_clear(&rg->head);
        UA_free(rg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(psm, connection, connection->head.state);
}

UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                              UA_PubSubState targetState) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    if(rg->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "The ReaderGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Are we doing a top-level state update or recursively? */
    UA_Boolean isTransient = rg->head.transientState;
    rg->head.transientState = true;

    UA_Server *server = psm->sc.server;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = rg->head.state;
    UA_PubSubConnection *connection = rg->linkedConnection;

    /* Custom state machine */
    if(rg->config.customStateMachine) {
        ret = rg->config.customStateMachine(server, rg->head.identifier, rg->config.context,
                                            &rg->head.state, targetState);
        goto finalize_state_machine;
    }

    /* Internal state machine */
    switch(targetState) {
        /* Disabled or Error */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        rg->head.state = targetState;
        UA_ReaderGroup_disconnect(rg);
        rg->hasReceived = false;
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        if(psm->sc.state != UA_LIFECYCLESTATE_STARTED) {
            /* Avoid repeat warnings */
            if(oldState != UA_PUBSUBSTATE_PAUSED) {
                UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                                      "Cannot enable the ReaderGroup while the "
                                      "server is not running -> Paused State");
            }
            rg->head.state = UA_PUBSUBSTATE_PAUSED;
            UA_ReaderGroup_disconnect(rg);
            break;
        }

        /* Connection is not operational -> ReaderGroup paused */
        if(connection->head.state != UA_PUBSUBSTATE_OPERATIONAL) {
            UA_ReaderGroup_disconnect(rg);
            rg->head.state = UA_PUBSUBSTATE_PAUSED;
            break;
        }

        /* Connect RG-specific connections. For example for MQTT. */
        if(UA_ReaderGroup_canConnect(rg))
            ret = UA_ReaderGroup_connect(psm, rg, false);

        /* Preoperational until a message was received */
        rg->head.state = (rg->hasReceived) ?
            UA_PUBSUBSTATE_OPERATIONAL : UA_PUBSUBSTATE_PREOPERATIONAL;
        break;

        /* Unknown case */
    default:
        ret = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }

    /* Failure */
    if(ret != UA_STATUSCODE_GOOD) {
        rg->head.state = UA_PUBSUBSTATE_ERROR;
        UA_ReaderGroup_disconnect(rg);
        rg->hasReceived = false;
    }

 finalize_state_machine:

    /* Only the top-level state update (if recursive calls are happening)
     * notifies the application and updates Reader and WriterGroups */
    rg->head.transientState = isTransient;
    if(rg->head.transientState)
        return ret;

    /* Inform application about state change */
    if(rg->head.state != oldState) {
        UA_LOG_INFO_PUBSUB(psm->logging, rg, "%s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(rg->head.state));
        if(server->config.pubSubConfig.stateChangeCallback != 0) {
            server->config.pubSubConfig.
                stateChangeCallback(server, rg->head.identifier, rg->head.state, ret);
        }
    }

    /* Update the attached DataSetReaders */
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        UA_DataSetReader_setPubSubState(psm, dsr, dsr->head.state,
                                        UA_STATUSCODE_GOOD);
    }

    /* Update the PubSubManager state. It will go from STOPPING to STOPPED when
     * the last socket has closed. */
    UA_PubSubManager_setState(psm, psm->sc.state);

    return ret;
}

UA_StatusCode
UA_ReaderGroup_setEncryptionKeys(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                 UA_UInt32 securityTokenId,
                                 const UA_ByteString signingKey,
                                 const UA_ByteString encryptingKey,
                                 const UA_ByteString keyNonce) {
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "JSON encoding is enabled. The message security is "
                              "only defined for the UADP message mapping.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!rg->config.securityPolicy) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "No SecurityPolicy configured for the ReaderGroup");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(securityTokenId != rg->securityTokenId) {
        rg->securityTokenId = securityTokenId;
        rg->nonceSequenceNumber = 1;
    }

    /* Create a new context */
    if(!rg->securityPolicyContext) {
        return rg->config.securityPolicy->
            newContext(rg->config.securityPolicy->policyContext,
                       &signingKey, &encryptingKey, &keyNonce,
                       &rg->securityPolicyContext);
    }

    /* Update the context */
    return rg->config.securityPolicy->
        setSecurityKeys(rg->securityPolicyContext, &signingKey,
                        &encryptingKey, &keyNonce);
}

UA_Boolean
UA_ReaderGroup_process(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                       UA_NetworkMessage *nm) {
    /* Check if the ReaderGroup is enabled */
    if(rg->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
       rg->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
        return false;

    /* Set to operational if required */
    rg->hasReceived = true;
    UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);

    /* Safe iteration. The current Reader might be deleted in the ReaderGroup
     * _setPubSubState callback. */
    UA_Boolean processed = false;
    UA_DataSetReader *reader, *reader_tmp;
    LIST_FOREACH_SAFE(reader, &rg->readers, listEntry, reader_tmp) {
        /* Check if the reader is enabled */
        if(reader->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
           reader->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;

        UA_StatusCode res = UA_DataSetReader_checkIdentifier(psm, reader, nm);
        if(res != UA_STATUSCODE_GOOD)
            continue;

        /* Update the ReaderGroup state if this is the first received message */
        if(!rg->hasReceived) {
            rg->hasReceived = true;
            UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);
        }

        /* The message was processed by at least one reader */
        processed = true;

        UA_LOG_TRACE_PUBSUB(psm->logging, rg, "Processing a NetworkMessage");

        /* No payload header. The message ontains a single DataSetMessage that
         * is processed by every Reader. */
        if(!nm->payloadHeaderEnabled) {
            UA_DataSetReader_process(psm, reader,
                                     nm->payload.dataSetPayload.dataSetMessages);
            continue;
        }

        /* Process only the payloads where the WriterId from the header is expected */
        size_t count = nm->payload.dataSetPayload.dataSetMessagesSize;
        for(size_t i = 0; i < count; i++) {
            if(reader->config.dataSetWriterId == nm->payload.dataSetPayload.
               dataSetMessages[i].dataSetWriterId) {
                UA_DataSetReader_process(psm, reader,
                                         &nm->payload.dataSetPayload.dataSetMessages[i]);
            }
        }
    }

    return processed;
}

/******************************/
/* Decrypt the NetworkMessage */
/******************************/

static UA_StatusCode
needsDecryption(const UA_Logger *logger,
                const UA_NetworkMessage *networkMessage,
                const UA_MessageSecurityMode securityMode,
                UA_Boolean *doDecrypt) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Boolean requiresEncryption = securityMode > UA_MESSAGESECURITYMODE_SIGN;
    UA_Boolean isEncrypted = networkMessage->securityHeader.networkMessageEncrypted;

    if(isEncrypted && requiresEncryption) {
        *doDecrypt = true;
    } else if(!isEncrypted && !requiresEncryption) {
        *doDecrypt = false;
    } else {
        if(isEncrypted) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is encrypted but ReaderGroup does not expect encryption");
            retval = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is not encrypted but ReaderGroup requires encryption");
            retval = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        }
    }
    return retval;
}

static UA_StatusCode
needsValidation(const UA_Logger *logger,
                const UA_NetworkMessage *networkMessage,
                const UA_MessageSecurityMode securityMode,
                UA_Boolean *doValidate) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Boolean isSigned = networkMessage->securityHeader.networkMessageSigned;
    UA_Boolean requiresSignature = securityMode > UA_MESSAGESECURITYMODE_NONE;

    if(isSigned &&
       requiresSignature) {
        *doValidate = true;
    } else if(!isSigned && !requiresSignature) {
        *doValidate = false;
    } else {
        if(isSigned) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is signed but ReaderGroup does not expect signatures");
            retval = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is not signed but ReaderGroup requires signature");
            retval = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        }
    }
    return retval;
}

UA_StatusCode
verifyAndDecryptNetworkMessage(const UA_Logger *logger, UA_ByteString buffer,
                               Ctx *ctx, UA_NetworkMessage *nm,
                               UA_ReaderGroup *rg) {
    UA_MessageSecurityMode securityMode = rg->config.securityMode;
    UA_Boolean doValidate = false;
    UA_Boolean doDecrypt = false;

    UA_StatusCode rv = needsValidation(logger, nm, securityMode, &doValidate);
    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. Validation security mode error");

    rv = needsDecryption(logger, nm, securityMode, &doDecrypt);
    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. Decryption security mode error");

    if(!doValidate && !doDecrypt)
        return UA_STATUSCODE_GOOD;

    void *channelContext = rg->securityPolicyContext;
    UA_PubSubSecurityPolicy *securityPolicy = rg->config.securityPolicy;
    UA_CHECK_MEM_ERROR(channelContext, return UA_STATUSCODE_BADINVALIDARGUMENT,
                       logger, UA_LOGCATEGORY_PUBSUB,
                       "PubSub receive. securityPolicyContext must be initialized "
                       "when security mode is enabled to sign and/or encrypt");
    UA_CHECK_MEM_ERROR(securityPolicy, return UA_STATUSCODE_BADINVALIDARGUMENT,
                       logger, UA_LOGCATEGORY_PUBSUB,
                       "PubSub receive. securityPolicy must be set when security mode"
                       "is enabled to sign and/or encrypt");

    /* Validate the signature */
    if(doValidate) {
        size_t sigSize = securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(channelContext);
        UA_ByteString toBeVerified = {buffer.length - sigSize, buffer.data};
        UA_ByteString signature = {sigSize, buffer.data + buffer.length - sigSize};

        rv = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            verify(channelContext, &toBeVerified, &signature);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Signature invalid");

        /* Remove the signature from the ctx->end. We do not want to decode that. */
        ctx->end -= sigSize;
    }

    /* Decrypt the content */
    if(doDecrypt) {
        const UA_ByteString nonce = {
            (size_t)nm->securityHeader.messageNonceSize,
            (UA_Byte*)(uintptr_t)nm->securityHeader.messageNonce
        };
        rv = securityPolicy->setMessageNonce(channelContext, &nonce);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Faulty Nonce set");

        UA_ByteString toBeDecrypted = {(uintptr_t)(ctx->end - ctx->pos), ctx->pos};
        rv = securityPolicy->symmetricModule.cryptoModule
            .encryptionAlgorithm.decrypt(channelContext, &toBeDecrypted);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Faulty Decryption");
    }

    return UA_STATUSCODE_GOOD;
}

/***********************/
/* Connection Handling */
/***********************/

static UA_StatusCode
UA_ReaderGroup_connectMQTT(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                           UA_Boolean validate);

typedef struct  {
    UA_String profileURI;
    UA_String protocol;
    UA_Boolean json;
    UA_StatusCode (*connectReaderGroup)(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                        UA_Boolean validate);
} ReaderGroupProfileMapping;

static ReaderGroupProfileMapping readerGroupProfiles[UA_PUBSUB_PROFILES_SIZE] = {
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"),
     UA_STRING_STATIC("udp"), false, NULL},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"),
     UA_STRING_STATIC("mqtt"), false, UA_ReaderGroup_connectMQTT},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"),
     UA_STRING_STATIC("mqtt"), true, UA_ReaderGroup_connectMQTT},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"),
     UA_STRING_STATIC("eth"), false, NULL}
};

static void
UA_ReaderGroup_detachConnection(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                UA_ConnectionManager *cm, uintptr_t connectionId) {
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(rg->recvChannels[i] != connectionId)
            continue;
        UA_LOG_INFO_PUBSUB(psm->logging, rg, "Detach receive-connection %S %u",
                           cm->protocol, (unsigned)connectionId);
        rg->recvChannels[i] = 0;
        rg->recvChannelsSize--;
        return;
    }
}

static UA_StatusCode
UA_ReaderGroup_attachRecvConnection(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                    UA_ConnectionManager *cm, uintptr_t connectionId) {
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(rg->recvChannels[i] == connectionId)
            return UA_STATUSCODE_GOOD;
    }
    if(rg->recvChannelsSize >= UA_PUBSUB_MAXCHANNELS)
        return UA_STATUSCODE_BADINTERNALERROR;
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(rg->recvChannels[i] != 0)
            continue;
        UA_LOG_INFO_PUBSUB(psm->logging, rg, "Attach receive-connection %S %u",
                           cm->protocol, (unsigned)connectionId);
        rg->recvChannels[i] = connectionId;
        rg->recvChannelsSize++;
        break;
    }
    return UA_STATUSCODE_GOOD;
}

static void
ReaderGroupChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                          void *application, void **connectionContext,
                          UA_ConnectionState state, const UA_KeyValueMap *params,
                          UA_ByteString msg) {
    if(!connectionContext)
        return;

    /* Get the context pointers */
    UA_ReaderGroup *rg = (UA_ReaderGroup*)*connectionContext;
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_Server *server = psm->sc.server;

    lockServer(server);

    /* The connection is closing in the EventLoop. This is the last callback
     * from that connection. Clean up the SecureChannel in the client. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Reset the connection identifiers */
        UA_ReaderGroup_detachConnection(psm, rg, cm, connectionId);

        /* PSC marked for deletion and the last EventLoop connection has closed */
        if(rg->deleteFlag && rg->recvChannelsSize == 0) {
            UA_ReaderGroup_remove(psm, rg);
            unlockServer(server);
            return;
        }

        /* Reconnect if still operational */
        UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);

        /* Switch the psm state from stopping to stopped once the last
         * connection has closed */
        UA_PubSubManager_setState(psm, psm->sc.state);

        unlockServer(server);
        return;
    }

    /* Store the connectionId (if a new connection) */
    UA_StatusCode res = UA_ReaderGroup_attachRecvConnection(psm, rg, cm, connectionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "No more space for an additional EventLoop connection");
        UA_PubSubConnection *c = rg->linkedConnection;
        if(c && c->cm)
            c->cm->closeConnection(c->cm, connectionId);
        unlockServer(server);
        return;
    }

    /* The connection has opened - set the ReaderGroup to operational */
    UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);

    /* No message received */
    if(msg.length == 0) {
        unlockServer(server);
        return;
    }

    if(rg->head.state != UA_PUBSUBSTATE_OPERATIONAL) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "Received a messaage for a non-operational ReaderGroup");
        unlockServer(server);
        return;
    }

    /* Decode message */
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
        res = UA_PubSubConnection_decodeNetworkMessage(psm, rg->linkedConnection, msg, &nm);
    } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
#ifdef UA_ENABLE_JSON_ENCODING
        res = UA_NetworkMessage_decodeJson(&msg, &nm, NULL);
#else
        res = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "Verify, decrypt and decode network message failed");
        unlockServer(server);
        return;
    }

    /* Process the decoded message */
    UA_ReaderGroup_process(psm, rg, &nm);
    UA_NetworkMessage_clear(&nm);
    unlockServer(server);
}

static UA_StatusCode
UA_ReaderGroup_connectMQTT(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                           UA_Boolean validate) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubConnection *c = rg->linkedConnection;
    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Get the TransportSettings */
    UA_ExtensionObject *ts = &rg->config.transportSettings;
    if((ts->encoding != UA_EXTENSIONOBJECT_DECODED &&
        ts->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       ts->content.decoded.type !=
       &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE]) {
        UA_LOG_ERROR_PUBSUB(psm->logging, rg,
                            "Wrong TransportSettings type for MQTT");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_BrokerDataSetReaderTransportDataType *transportSettings =
        (UA_BrokerDataSetReaderTransportDataType*)ts->content.decoded.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port = 1883; /* Default */
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "Could not parse the MQTT network URL");
        return res;
    }

    /* Set up the connection parameters.
     * TODO: Complete the MQTT parameters. */
    UA_Boolean listen = true;
    UA_KeyValuePair kvp[5];
    UA_KeyValueMap kvm = {5, kvp};
    kvp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&kvp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&kvp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[2].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&kvp[2].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    kvp[3].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&kvp[3].value, &transportSettings->queueName,
                         &UA_TYPES[UA_TYPES_STRING]);
    kvp[4].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&kvp[4].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* Connect */
    res = c->cm->openConnection(c->cm, &kvm, psm, rg, ReaderGroupChannelCallback);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, rg, "Could not open the MQTT connection");
    }
    return res;
}

void
UA_ReaderGroup_disconnect(UA_ReaderGroup *rg) {
    UA_PubSubConnection *c = rg->linkedConnection;
    if(!c)
        return;
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(rg->recvChannels[i] != 0)
            c->cm->closeConnection(c->cm, rg->recvChannels[i]);
    }
}

UA_Boolean
UA_ReaderGroup_canConnect(UA_ReaderGroup *rg) {
    return rg->recvChannelsSize == 0;
}

UA_StatusCode
UA_ReaderGroup_connect(UA_PubSubManager *psm, UA_ReaderGroup *rg, UA_Boolean validate) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Is this a ReaderGroup with custom TransportSettings beyond the
     * PubSubConnection? */
    if(rg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return UA_STATUSCODE_GOOD;

    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    if(!el) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, rg, "No EventLoop configured");
        return UA_STATUSCODE_BADINTERNALERROR;;
    }

    UA_PubSubConnection *c = rg->linkedConnection;
    if(!c)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Look up the connection manager for the connection */
    ReaderGroupProfileMapping *profile = NULL;
    for(size_t i = 0; i < UA_PUBSUB_PROFILES_SIZE; i++) {
        if(!UA_String_equal(&c->config.transportProfileUri,
                            &readerGroupProfiles[i].profileURI))
            continue;
        profile = &readerGroupProfiles[i];
        break;
    }

    UA_ConnectionManager *cm = (profile) ? getCM(el, profile->protocol) : NULL;
    if(!cm || (c->cm && cm != c->cm)) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c,
                            "The requested profile \"%S\"is not supported",
                            c->config.transportProfileUri);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    c->cm = cm;
    c->json = profile->json;

    /* If no ReaderGroup-specific connections, the ReaderGroup is set to
     * operational when we return. */
    return (profile->connectReaderGroup) ?
        profile->connectReaderGroup(psm, rg, validate) : UA_STATUSCODE_GOOD;
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_addReaderGroup(UA_Server *server, const UA_NodeId connectionIdentifier,
                         const UA_ReaderGroupConfig *readerGroupConfig,
                         UA_NodeId *readerGroupIdentifier) {
    if(!server || !readerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res =
        UA_ReaderGroup_create(psm, connectionIdentifier,
                              readerGroupConfig, readerGroupIdentifier);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removeReaderGroup(UA_Server *server, const UA_NodeId groupIdentifier) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, groupIdentifier);
    if(rg)
        UA_ReaderGroup_remove(psm, rg);
    else
        res = UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getReaderGroupConfig(UA_Server *server, const UA_NodeId rgId,
                               UA_ReaderGroupConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), rgId);
    UA_StatusCode ret = (rg) ?
        UA_ReaderGroupConfig_copy(&rg->config, config) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return ret;
}

UA_StatusCode
UA_Server_getReaderGroupState(UA_Server *server, const UA_NodeId rgId,
                              UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), rgId);
    if(rg) {
        *state = rg->head.state;
        ret = UA_STATUSCODE_GOOD;
    }
    unlockServer(server);
    return ret;
}

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
UA_Server_setReaderGroupActivateKey(UA_Server *server,
                                    const UA_NodeId readerGroupId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
    if(!rg || !rg->keyStorage || !rg->keyStorage->currentItem) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode ret =
        UA_PubSubKeyStorage_activateKeyToChannelContext(psm, rg->head.identifier,
                                                        rg->config.securityGroupId);
    unlockServer(server);
    return ret;
}
#endif

UA_StatusCode
UA_Server_enableReaderGroup(UA_Server *server, const UA_NodeId readerGroupId){
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
    UA_StatusCode ret = (rg) ?
        UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL) :
        UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return ret;
}

UA_StatusCode
UA_Server_disableReaderGroup(UA_Server *server, const UA_NodeId readerGroupId){
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
    UA_StatusCode ret = (rg) ?
        UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED) :
        UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return ret;
}

UA_StatusCode
UA_Server_setReaderGroupEncryptionKeys(UA_Server *server,
                                       const UA_NodeId readerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), readerGroup);
    UA_StatusCode res = (rg) ?
        UA_ReaderGroup_setEncryptionKeys(psm, rg, securityTokenId, signingKey,
                                         encryptingKey, keyNonce) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

static UA_StatusCode
UA_PubSubDataSetReader_generateKeyFrameMessage(UA_Server *server,
                                               UA_DataSetMessage *dsm,
                                               UA_DataSetReader *dsr) {
    /* Prepare DataSetMessageContent */
    UA_TargetVariablesDataType *tv = &dsr->config.subscribedDataSet.target;
    dsm->header.dataSetMessageValid = true;
    dsm->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dsm->data.keyFrameData.fieldCount = (UA_UInt16) tv->targetVariablesSize;
    dsm->data.keyFrameData.dataSetFields = (UA_DataValue *)
            UA_Array_new(tv->targetVariablesSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(!dsm->data.keyFrameData.dataSetFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dsm->data.keyFrameData.dataSetMetaDataType =
        &dsr->config.dataSetMetaData;

     for(size_t counter = 0; counter < tv->targetVariablesSize; counter++) {
        /* Read the value and set the source in the reader config */
        UA_DataValue *dfv = &dsm->data.keyFrameData.dataSetFields[counter];
        UA_FieldTargetDataType *ftv = &tv->targetVariables[counter];

        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        rvi.nodeId = ftv->targetNodeId;
        rvi.attributeId = ftv->attributeId;
        rvi.indexRange = ftv->writeIndexRange;
        *dfv = readWithSession(server, &server->adminSession, &rvi,
                               UA_TIMESTAMPSTORETURN_NEITHER);

        /* Deactivate statuscode? */
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dfv->hasStatus = false;

        /* Deactivate timestamps */
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dfv->hasSourceTimestamp = false;
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dfv->hasSourcePicoseconds = false;
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dfv->hasServerTimestamp = false;
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dfv->hasServerPicoseconds = false;
    }

    return UA_STATUSCODE_GOOD;
}

/* Generate a DataSetMessage for the given reader. */
static UA_StatusCode
UA_DataSetReader_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dsm,
                                        UA_DataSetReader *dsr) {
    /* Support only for UADP configuration
     * TODO: JSON encoding if UA_DataSetReader_generateDataSetMessage used other
     * that RT configuration */

    dsm->dataSetWriterId = dsr->config.dataSetWriterId;

    UA_ExtensionObject *settings = &dsr->config.messageSettings;
    if(settings->content.decoded.type != &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE])
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* The configuration Flags are included inside the std. defined
     * UA_UadpDataSetReaderMessageDataType */
    UA_UadpDataSetReaderMessageDataType defaultUadpConfiguration;
    UA_UadpDataSetReaderMessageDataType *dsrMessageDataType =
        (UA_UadpDataSetReaderMessageDataType*) settings->content.decoded.data;

    if(!(settings->encoding == UA_EXTENSIONOBJECT_DECODED ||
         settings->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       !dsrMessageDataType->dataSetMessageContentMask) {
        /* Create default flag configuration if no dataSetMessageContentMask or
         * even messageSettings in UadpDataSetWriterMessageDataType was
         * passed. */
        memset(&defaultUadpConfiguration, 0, sizeof(UA_UadpDataSetReaderMessageDataType));
        defaultUadpConfiguration.dataSetMessageContentMask = (UA_UadpDataSetMessageContentMask)
            ((u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION);
        dsrMessageDataType = &defaultUadpConfiguration;
    }

    /* Sanity-test the configuration */
    if(dsrMessageDataType &&
       (dsrMessageDataType->networkMessageNumber != 0 ||
        dsrMessageDataType->dataSetOffset != 0)) {
        dsrMessageDataType->networkMessageNumber = 0;
        dsrMessageDataType->dataSetOffset = 0;
    }

    /* The field encoding depends on the flags inside the reader config. */
    if(dsr->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        dsm->header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    } else if((u64)dsr->config.dataSetFieldContentMask &
              ((u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP |
               (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE)) {
        dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    } else {
        dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    }

    /* Std: 'The DataSetMessageContentMask defines the flags for the content
     * of the DataSetMessage header.' */
    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION) {
        dsm->header.configVersionMajorVersionEnabled = true;
        dsm->header.configVersionMajorVersion =
            dsr->config.dataSetMetaData.configurationVersion.majorVersion;
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION) {
        dsm->header.configVersionMinorVersionEnabled = true;
        dsm->header.configVersionMinorVersion =
            dsr->config.dataSetMetaData.configurationVersion.minorVersion;
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
        /* Will be modified when subscriber receives new nw msg */
        dsm->header.dataSetMessageSequenceNrEnabled = true;
        dsm->header.dataSetMessageSequenceNr = 1;
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP) {
        dsm->header.timestampEnabled = true;
        dsm->header.timestamp = UA_DateTime_now();
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_PICOSECONDS)
        dsm->header.picoSecondsIncluded = false;

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_STATUS)
        dsm->header.statusEnabled = true;

    /* Not supported for Delta frames atm */
    return UA_PubSubDataSetReader_generateKeyFrameMessage(server, dsm, dsr);
}

static UA_StatusCode
readerGroupGenerateNetworkMessage(UA_ReaderGroup *wg, UA_DataSetReader **dsr,
                                  UA_DataSetMessage *dsm, UA_Byte dsmCount,
                                  UA_ExtensionObject *messageSettings,
                                  UA_NetworkMessage *nm) {
    if(messageSettings->content.decoded.type != &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE])
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Set the header flags */
    UA_UadpDataSetReaderMessageDataType *dsrm =
        (UA_UadpDataSetReaderMessageDataType*)messageSettings->content.decoded.data;
    nm->publisherIdEnabled = ((u64)dsrm->networkMessageContentMask &
                              (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID) != 0;
    nm->groupHeaderEnabled = ((u64)dsrm->networkMessageContentMask &
                              (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
    nm->groupHeader.writerGroupIdEnabled =
        ((u64)dsrm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    nm->groupHeader.groupVersionEnabled =
        ((u64)dsrm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    nm->groupHeader.networkMessageNumberEnabled =
        ((u64)dsrm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    nm->groupHeader.sequenceNumberEnabled =
        ((u64)dsrm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    nm->payloadHeaderEnabled = ((u64)dsrm->networkMessageContentMask &
                                (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER) != 0;
    nm->timestampEnabled = ((u64)dsrm->networkMessageContentMask &
                            (u64)UA_UADPNETWORKMESSAGECONTENTMASK_TIMESTAMP) != 0;
    nm->picosecondsEnabled = ((u64)dsrm->networkMessageContentMask &
                              (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PICOSECONDS) != 0;
    nm->dataSetClassIdEnabled = ((u64)dsrm->networkMessageContentMask &
                                 (u64)UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID) != 0;
    nm->promotedFieldsEnabled = ((u64)dsrm->networkMessageContentMask &
                                 (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PROMOTEDFIELDS) != 0;

    /* Set the NetworkMessage header */
    nm->version = 1;
    nm->networkMessageType = UA_NETWORKMESSAGE_DATASET;
    nm->publisherId = dsr[0]->config.publisherId;

    /* Set the group header (use default sequence numbers) */
    nm->groupHeader.networkMessageNumber = 1;
    nm->groupHeader.sequenceNumber = 1;
    nm->groupHeader.groupVersion = dsrm->groupVersion;
    nm->groupHeader.writerGroupId = dsr[0]->config.writerGroupId;

    /* TODO Security Header */

    /* Set the payload information from the dsm */
    nm->payload.dataSetPayload.dataSetMessages = dsm;
    nm->payload.dataSetPayload.dataSetMessagesSize = dsmCount;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_computeReaderGroupOffsetTable(UA_Server *server,
                                        const UA_NodeId readerGroupId,
                                        UA_PubSubOffsetTable *ot) {
    if(!server || !ot)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    /* Get the ReaderGroup */
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = (psm) ? UA_ReaderGroup_find(psm, readerGroupId) : NULL;
    if(!rg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    memset(ot, 0, sizeof(UA_PubSubOffsetTable));

    /* Define variables here to allow the goto cleanup later on */
    size_t msgSize;
    size_t fieldindex = 0;
    UA_FieldTargetDataType *tv = NULL;

    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));

    UA_STACKARRAY(UA_DataSetMessage, dsmStore, rg->readersCount);
    UA_STACKARRAY(UA_DataSetReader *, dsrStore, rg->readersCount);
    memset(dsmStore, 0, sizeof(UA_DataSetMessage) * rg->readersCount);

    size_t dsmCount = 0;
    UA_DataSetReader *dsr;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        dsrStore[dsmCount] = dsr;
        res = UA_DataSetReader_generateDataSetMessage(server, &dsmStore[dsmCount], dsr);
        dsmCount++;
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Generate the NetworkMessage */
    dsr = LIST_FIRST(&rg->readers);
    res = readerGroupGenerateNetworkMessage(rg, dsrStore, dsmStore, (UA_Byte) dsmCount,
                                            &dsr->config.messageSettings, &networkMessage);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Compute the message length and generate the old format offset-table (done
     * inside calcSizeBinary) */
    msgSize = UA_NetworkMessage_calcSizeBinaryWithOffsetTable(&networkMessage, ot);
    if(msgSize == 0) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Create the encoded network message */
    res = UA_NetworkMessage_encodeBinary(&networkMessage, &ot->networkMessage);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Pick up the component NodeIds */
    dsr = NULL;
    for(size_t i = 0; i < ot->offsetsSize; i++) {
        UA_PubSubOffset *o = &ot->offsets[i];
        switch(o->offsetType) {
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_TIMESTAMP:
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_PICOSECONDS:
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_GROUPVERSION:
            UA_NodeId_copy(&rg->head.identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE:
            dsr = (dsr == NULL) ? LIST_FIRST(&rg->readers) : LIST_NEXT(dsr, listEntry);
            fieldindex = 0;
            /* fall through */
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_STATUS:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_TIMESTAMP:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_PICOSECONDS:
            UA_NodeId_copy(&dsr->head.identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE:
            tv = &dsr->config.subscribedDataSet.target.targetVariables[fieldindex];
            UA_NodeId_copy(&tv->targetNodeId, &o->component);
            fieldindex++;
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT:
            tv = &dsr->config.subscribedDataSet.target.targetVariables[fieldindex];
            UA_NodeId_copy(&tv->targetNodeId, &o->component);
            fieldindex++;
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW:
            tv = &dsr->config.subscribedDataSet.target.targetVariables[fieldindex];
            UA_NodeId_copy(&tv->targetNodeId, &o->component);
            fieldindex++;
            break;
        default:
            break;
        }
    }

 cleanup:
    /* Clean up and return */
    for(size_t i = 0; i < dsmCount; i++) {
        UA_DataSetMessage_clear(&dsmStore[i]);
    }

    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
