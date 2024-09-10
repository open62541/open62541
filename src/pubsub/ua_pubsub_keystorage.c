/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include "ua_pubsub_keystorage.h"
#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_SKS /* conditional compilation */

#define UA_REQ_CURRENT_TOKEN 0

#include "../server/ua_server_internal.h"
#include "../client/ua_client_internal.h"

UA_PubSubKeyStorage *
UA_PubSubKeyStorage_find(UA_PubSubManager *psm, UA_String securityGroupId) {
    if(!psm)
        return NULL;
    UA_PubSubKeyStorage *ks;
    LIST_FOREACH(ks, &psm->pubSubKeyList, keyStorageList) {
        if(UA_String_equal(&ks->securityGroupID, &securityGroupId))
            break;
    }
    return ks;
}

UA_PubSubSecurityPolicy *
findPubSubSecurityPolicy(UA_PubSubManager *psm, const UA_String *securityPolicyUri) {
    if(!psm || !securityPolicyUri)
        return NULL;

    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    UA_ServerConfig *config = &psm->sc.server->config;
    for(size_t i = 0; i < config->pubSubConfig.securityPoliciesSize; i++) {
        if(UA_String_equal(securityPolicyUri,
                           &config->pubSubConfig.securityPolicies[i].policyUri))
            return &config->pubSubConfig.securityPolicies[i];
    }
    return NULL;
}

void
UA_PubSubKeyStorage_clearKeyList(UA_PubSubKeyStorage *ks) {
    if(TAILQ_EMPTY(&ks->keyList))
        return;

    UA_PubSubKeyListItem *item, *item_tmp;
    TAILQ_FOREACH_SAFE(item, &ks->keyList, keyListEntry, item_tmp) {
        TAILQ_REMOVE(&ks->keyList, item, keyListEntry);
        UA_ByteString_clear(&item->key);
        UA_free(item);
    }
    ks->keyListSize = 0;
}

void
UA_PubSubKeyStorage_delete(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    /* Remove callback */
    if(!ks->callBackId) {
        removeCallback(psm->sc.server, ks->callBackId);
        ks->callBackId = 0;
    }

    UA_PubSubKeyStorage_clearKeyList(ks);
    UA_String_clear(&ks->securityGroupID);
    UA_ClientConfig_clear(&ks->sksConfig.clientConfig);
    UA_free(ks);
}

UA_StatusCode
UA_PubSubKeyStorage_init(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks,
                         const UA_String *securityGroupId,
                         UA_PubSubSecurityPolicy *policy,
                         UA_UInt32 maxPastKeyCount, UA_UInt32 maxFutureKeyCount) {
    UA_StatusCode res = UA_String_copy(securityGroupId, &ks->securityGroupID);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_UInt32 currentkeyCount = 1;
    ks->maxPastKeyCount = maxPastKeyCount;
    ks->maxFutureKeyCount = maxFutureKeyCount;
    ks->maxKeyListSize = maxPastKeyCount + currentkeyCount + maxFutureKeyCount;
    ks->policy = policy;

    TAILQ_INIT(&ks->keyList);

    /* Add this keystorage to the keystoragelist */
    LIST_INSERT_HEAD(&psm->pubSubKeyList, ks, keyStorageList);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubKeyStorage_addSecurityKeys(UA_PubSubKeyStorage *ks, size_t keysSize,
                                    UA_ByteString *keys, UA_UInt32 currentKeyId) {
    for(size_t i = 0; i < keysSize; ++i) {
        currentKeyId++; /* Increase the keyId */
        if(currentKeyId == 0)
            currentKeyId = 1; /* Rollover the keyId */

        /* Search for an existing item matching the tokenId */
        UA_PubSubKeyListItem *item = UA_PubSubKeyStorage_getKeyByKeyId(ks, currentKeyId);

        /* Not found. Add it. */
        if(!item) {
            item = UA_PubSubKeyStorage_push(ks, &keys[i], currentKeyId);
            if(!item)
                return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubKeyStorage_setCurrentKey(UA_PubSubKeyStorage *ks, UA_UInt32 keyId) {
    UA_PubSubKeyListItem *item = UA_PubSubKeyStorage_getKeyByKeyId(ks, keyId);
    if(!item)
        return UA_STATUSCODE_BADNOTFOUND;
    ks->currentItem = item;
    return UA_STATUSCODE_GOOD;
}

UA_PubSubKeyListItem *
UA_PubSubKeyStorage_getKeyByKeyId(UA_PubSubKeyStorage *ks,
                                  const UA_UInt32 keyId) {
    UA_PubSubKeyListItem *item;
    TAILQ_FOREACH(item, &ks->keyList, keyListEntry) {
        if(item->keyID == keyId)
            return item;
    }
    return NULL;
}

UA_PubSubKeyListItem *
UA_PubSubKeyStorage_push(UA_PubSubKeyStorage *ks, const UA_ByteString *key,
                         UA_UInt32 keyID) {
    UA_PubSubKeyListItem *newItem = (UA_PubSubKeyListItem *)
        malloc(sizeof(UA_PubSubKeyListItem));
    if(!newItem)
        return NULL;
    newItem->keyID = keyID;
    UA_StatusCode res = UA_ByteString_copy(key, &newItem->key);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(newItem);
        return NULL;
    }
    TAILQ_INSERT_TAIL(&ks->keyList, newItem, keyListEntry);
    ks->keyListSize++;
    return newItem;
}

UA_StatusCode
UA_PubSubKeyStorage_addKeyRolloverCallback(UA_PubSubManager *psm,
                                           UA_PubSubKeyStorage *ks,
                                           UA_Callback callback,
                                           UA_Duration timeToNextMs,
                                           UA_UInt64 *callbackID) {
    if(!psm || !ks || !callback || timeToNextMs <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    return el->addTimer(el, (UA_Callback)callback, psm, ks,
                        timeToNextMs, NULL, UA_TIMERPOLICY_ONCE, callbackID);
}

static UA_StatusCode
splitCurrentKeyMaterial(UA_PubSubKeyStorage *ks, UA_ByteString *signingKey,
                        UA_ByteString *encryptingKey, UA_ByteString *keyNonce) {
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!ks->policy)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_PubSubSecurityPolicy *policy = ks->policy;

    UA_ByteString key = ks->currentItem->key;

    /*Check the main key length is the same according to policy*/
    if(key.length != policy->symmetricModule.secureChannelNonceLength)
        return UA_STATUSCODE_BADINTERNALERROR;

    /*Get Key Length according to policy*/
    size_t signingkeyLength =
        policy->symmetricModule.cryptoModule.signatureAlgorithm.getLocalKeyLength(NULL);
    size_t encryptkeyLength =
        policy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalKeyLength(NULL);
    /*Rest of the part is the keyNonce*/
    size_t keyNonceLength = key.length - signingkeyLength - encryptkeyLength;

    /*DivideKeys in origin ByteString*/
    signingKey->data = key.data;
    signingKey->length = signingkeyLength;

    encryptingKey->data = key.data + signingkeyLength;
    encryptingKey->length = encryptkeyLength;

    keyNonce->data = key.data + signingkeyLength + encryptkeyLength;
    keyNonce->length = keyNonceLength;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setPubSubGroupEncryptingKey(UA_PubSubManager *psm, UA_NodeId PubSubGroupId,
                            UA_UInt32 securityTokenId, UA_ByteString signingKey,
                            UA_ByteString encryptingKey, UA_ByteString keyNonce) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, PubSubGroupId);
    if(wg)
        return UA_WriterGroup_setEncryptionKeys(psm, wg, securityTokenId, signingKey,
                                                encryptingKey, keyNonce);

    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, PubSubGroupId);
    if(rg)
        return UA_ReaderGroup_setEncryptionKeys(psm, rg, securityTokenId, signingKey,
                                                encryptingKey, keyNonce);

    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
setPubSubGroupEncryptingKeyForMatchingSecurityGroupId(UA_PubSubManager *psm,
                                                      UA_String securityGroupId,
                                                      UA_UInt32 securityTokenId,
                                                      UA_ByteString signingKey,
                                                      UA_ByteString encryptingKey,
                                                      UA_ByteString keyNonce) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    /* Key storage is the same for all reader / writer groups, channel context isn't
     * => Update channelcontext in all Writergroups / ReaderGroups which have the same
     * securityGroupId*/
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        /* For each writerGroup in server with matching SecurityGroupId */
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            if(UA_String_equal(&wg->config.securityGroupId, &securityGroupId)) {
                retval = UA_WriterGroup_setEncryptionKeys(psm, wg, securityTokenId,
                                                          signingKey, encryptingKey, keyNonce);
                if(retval != UA_STATUSCODE_GOOD)
                    return retval;
            }
        }

        /* For each readerGroup in server with matching SecurityGroupId */
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            if(UA_String_equal(&rg->config.securityGroupId, &securityGroupId)) {
                retval = UA_ReaderGroup_setEncryptionKeys(psm, rg, securityTokenId,
                                                          signingKey, encryptingKey, keyNonce);
                if(retval != UA_STATUSCODE_GOOD)
                    return retval;
            }
        }
    }
    return retval;
}

UA_StatusCode
UA_PubSubKeyStorage_activateKeyToChannelContext(UA_PubSubManager *psm,
                                                UA_NodeId pubSubGroupId,
                                                UA_String securityGroupId) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);
    if(securityGroupId.data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubKeyStorage *ks =
        UA_PubSubKeyStorage_find(psm, securityGroupId);
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!ks->policy && !(ks->keyListSize > 0))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_UInt32 securityTokenId = ks->currentItem->keyID;

    /*DivideKeys in origin ByteString*/
    UA_ByteString signingKey;
    UA_ByteString encryptKey;
    UA_ByteString keyNonce;
    UA_StatusCode retval = splitCurrentKeyMaterial(ks, &signingKey,
                                                   &encryptKey, &keyNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(!UA_NodeId_isNull(&pubSubGroupId))
        retval = setPubSubGroupEncryptingKey(psm, pubSubGroupId, securityTokenId,
                                             signingKey, encryptKey, keyNonce);
    else
        retval = setPubSubGroupEncryptingKeyForMatchingSecurityGroupId(
            psm, securityGroupId, securityTokenId, signingKey, encryptKey, keyNonce);

    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Failed to set Encrypting keys with Error: %s",
                     UA_StatusCode_name(retval));

    return retval;
}

static void
nextGetSecuritykeysCallback(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    if(!ks) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "GetSecurityKeysCall Failed with error: KeyStorage does not exist "
                     "in the server");
        return;
    }
    retval = getSecurityKeysAndStoreFetchedKeys(psm, ks);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "GetSecurityKeysCall Failed with error: %s ",
                     UA_StatusCode_name(retval));
}

void
UA_PubSubKeyStorage_keyRolloverCallback(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    /* Callbacks from the EventLoop are initially unlocked */
    UA_LOCK(&psm->sc.server->serviceMutex);

    UA_StatusCode retval =
        UA_PubSubKeyStorage_addKeyRolloverCallback(psm, ks,
                                                   (UA_Callback)UA_PubSubKeyStorage_keyRolloverCallback,
                                                   ks->keyLifeTime, &ks->callBackId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "Failed to update keys for security group id '%S'. Reason: '%s'.",
                     ks->securityGroupID, UA_StatusCode_name(retval));
    }

    if(ks->currentItem != TAILQ_LAST(&ks->keyList, keyListItems)) {
        ks->currentItem = TAILQ_NEXT(ks->currentItem, keyListEntry);
        ks->currentTokenId = ks->currentItem->keyID;
        retval = UA_PubSubKeyStorage_activateKeyToChannelContext(psm, UA_NODEID_NULL,
                                                                 ks->securityGroupID);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                         "Failed to update keys for security group id '%S'. Reason: '%s'.",
                         ks->securityGroupID, UA_StatusCode_name(retval));
        }
    } else if(ks->sksConfig.endpointUrl && ks->sksConfig.reqId == 0) {
        /* Publishers using a central SKS shall call GetSecurityKeys at a period
         * of half the KeyLifetime */
        UA_Duration msTimeToNextGetSecurityKeys = ks->keyLifeTime / 2;
        UA_EventLoop *el = psm->sc.server->config.eventLoop;
        retval = el->addTimer(el, (UA_Callback)nextGetSecuritykeysCallback, psm,
                              ks, msTimeToNextGetSecurityKeys, NULL,
                              UA_TIMERPOLICY_ONCE, NULL);
    }
    UA_UNLOCK(&psm->sc.server->serviceMutex);
}

void
UA_PubSubKeyStorage_detachKeyStorage(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);
    ks->referenceCount--;
    if(ks->referenceCount == 0) {
        LIST_REMOVE(ks, keyStorageList);
        UA_PubSubKeyStorage_delete(psm, ks);
    }
}

/**
 * @brief It holds the information required in the async callback to
 * GetSecurityKeys method Call.
 */
typedef struct {
    UA_PubSubManager *psm;
    UA_PubSubKeyStorage *ks;
    UA_UInt32 startingTokenId;
    UA_UInt32 requestedKeyCount;
    UA_DelayedCallback dc;
} sksClientContext;

static void sksClientCleanupCb(void *client, void *context);

static void
addDelayedSksClientCleanupCb(UA_Client *client, sksClientContext *context) {
    /* Register at most once */
    if(context->dc.application != NULL)
        return;
    context->dc.application = client;
    context->dc.callback = sksClientCleanupCb;
    context->dc.context = context;
    client->config.eventLoop->addDelayedCallback(client->config.eventLoop, &context->dc);
}

static void
sksClientCleanupCb(void *client, void *context) {
    UA_Client *sksClient = (UA_Client *)client;
    sksClientContext *ctx = (sksClientContext*)context;

    /* we do not want to call state change Callback when cleaning up */
    sksClient->config.stateCallback = NULL;

    if(sksClient->sessionState > UA_SESSIONSTATE_CLOSED &&
       sksClient->channel.state < UA_SECURECHANNELSTATE_CLOSED) {
        sksClient->config.eventLoop->
            addDelayedCallback(sksClient->config.eventLoop, &ctx->dc);
        UA_Client_disconnectAsync(sksClient);
        return;
    }

    if(sksClient->channel.state == UA_SECURECHANNELSTATE_CLOSED) {
        /* We cannot make deep copy of the following pointers because these have
         * internal structures, therefore we do not free them here. These will
         * be freed in UA_PubSubKeyStorage_delete. */
        sksClient->config.securityPolicies = NULL;
        sksClient->config.securityPoliciesSize = 0;
        sksClient->config.certificateVerification.context = NULL;
        sksClient->config.logging = NULL;
        sksClient->config.clientContext = NULL;
        UA_Client_delete(sksClient);
        UA_free(context);
    } else {
        sksClient->config.eventLoop->
            addDelayedCallback(sksClient->config.eventLoop, &ctx->dc);
    }
}

static void
storeFetchedKeys(UA_Client *client, void *userdata, UA_UInt32 requestId,
                 UA_CallResponse *response) {
    sksClientContext *ctx = (sksClientContext *)userdata;
    UA_PubSubKeyStorage *ks = ctx->ks;
    UA_PubSubManager *psm = ctx->psm;
    UA_StatusCode retval = response->responseHeader.serviceResult;

    UA_LOCK(&psm->sc.server->serviceMutex);
    /* check if the call to getSecurityKeys was a success */
    if(response->resultsSize != 0)
        retval = response->results->statusCode;
    if(retval != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "SKS Client: Failed to call GetSecurityKeys on SKS server with error: %s ",
                     UA_StatusCode_name(retval));
        goto cleanup;
    }

    UA_String *securityPolicyUri = (UA_String *)response->results->outputArguments[0].data;
    UA_UInt32 firstTokenId = *(UA_UInt32 *)response->results->outputArguments[1].data;
    UA_ByteString *keys = (UA_ByteString *)response->results->outputArguments[2].data;
    UA_ByteString *currentKey = &keys[0];
    UA_UInt32 currentKeyCount = 1;
    UA_ByteString *futureKeys = &keys[currentKeyCount];
    size_t futureKeySize = response->results->outputArguments[2].arrayLength - currentKeyCount;
    UA_Duration msKeyLifeTime = *(UA_Duration *)response->results->outputArguments[4].data;

    if(!UA_String_equal(securityPolicyUri, &ks->policy->policyUri)) {
        retval = UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        goto cleanup;
    }

    UA_PubSubKeyListItem *current = UA_PubSubKeyStorage_getKeyByKeyId(ks, firstTokenId);
    if(!current) {
        UA_PubSubKeyStorage_clearKeyList(ks);
        retval |= (UA_PubSubKeyStorage_push(ks, currentKey, firstTokenId)) ?
            UA_STATUSCODE_GOOD : UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_PubSubKeyStorage_setCurrentKey(ks, firstTokenId);
    retval |= UA_PubSubKeyStorage_addSecurityKeys(ks, futureKeySize, futureKeys, firstTokenId);
    ks->keyLifeTime = msKeyLifeTime;
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* After a new batch of keys is fetched from SKS server, the key storage is
     * updated with new keys and new keylifetime. Also the remaining time for
     * current keyRollover is also returned. When setting a new keyRollover
     * callback, the previous callback must be removed so that the keyRollover
     * does not happen twice */
    if(ks->callBackId != 0) {
        psm->sc.server->config.eventLoop->removeTimer(psm->sc.server->config.eventLoop,
                                                      ks->callBackId);
        ks->callBackId = 0;
    }

    UA_Duration msTimeToNextKey =
        *(UA_Duration *)response->results->outputArguments[3].data;
    if(!(msTimeToNextKey > 0))
        msTimeToNextKey = ks->keyLifeTime;
    retval = UA_PubSubKeyStorage_addKeyRolloverCallback(
        psm, ks, (UA_Callback)UA_PubSubKeyStorage_keyRolloverCallback,
        msTimeToNextKey, &ks->callBackId);

cleanup:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Failed to store the fetched keys from SKS server with error: %s",
                     UA_StatusCode_name(retval));
    }
    /* call user callback to notify about the status */
    UA_UNLOCK(&psm->sc.server->serviceMutex);
    if(ks->sksConfig.userNotifyCallback)
        ks->sksConfig.userNotifyCallback(psm->sc.server, retval, ks->sksConfig.context);
    ks->sksConfig.reqId = 0;
    UA_Client_disconnectAsync(client);
    addDelayedSksClientCleanupCb(client, ctx);
}

static UA_StatusCode
callGetSecurityKeysMethod(UA_Client *client) {

    sksClientContext *ctx = (sksClientContext *)client->config.clientContext;

    UA_Variant inputArguments[3];
    UA_Variant_setScalar(&inputArguments[0], &ctx->ks->securityGroupID,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[1], &ctx->startingTokenId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputArguments[2], &ctx->requestedKeyCount,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_NodeId objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    UA_NodeId methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_GETSECURITYKEYS);
    size_t inputArgumentsSize = 3;

    UA_StatusCode retval = UA_Client_call_async(
        client, objectId, methodId, inputArgumentsSize, inputArguments, storeFetchedKeys,
        (void *)ctx, &ctx->ks->sksConfig.reqId);
    return retval;
}

static void
onConnect(UA_Client *client, UA_SecureChannelState channelState,
          UA_SessionState sessionState, UA_StatusCode connectStatus) {
    UA_Boolean triggerSKSCleanup = false;
    if(connectStatus != UA_STATUSCODE_GOOD &&
       connectStatus != UA_STATUSCODE_BADNOTCONNECTED &&
       sessionState != UA_SESSIONSTATE_ACTIVATED) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "SKS Client: Failed to connect SKS server with error: %s ",
                     UA_StatusCode_name(connectStatus));
        triggerSKSCleanup = true;
    }
    if(connectStatus == UA_STATUSCODE_GOOD && sessionState == UA_SESSIONSTATE_ACTIVATED) {
        connectStatus = callGetSecurityKeysMethod(client);
        if(connectStatus != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_SERVER,
                         "SKS Client: Failed to call GetSecurityKeys on SKS server with "
                         "error: %s ",
                         UA_StatusCode_name(connectStatus));
            triggerSKSCleanup = true;
        }
    }
    if(triggerSKSCleanup) {
        /* call user callback to notify about the status */
        sksClientContext *ctx = (sksClientContext *)client->config.clientContext;
        UA_PubSubKeyStorage *ks = ctx->ks;
        if(ks->sksConfig.userNotifyCallback)
            ks->sksConfig.userNotifyCallback(ctx->psm->sc.server, connectStatus,
                                             ks->sksConfig.context);
        UA_Client_disconnectAsync(client);
        addDelayedSksClientCleanupCb(client, ctx);
    }
}

static void
setServerEventloopOnSksClient(UA_ClientConfig *cc, UA_EventLoop *externalEventloop) {
    UA_assert(externalEventloop != NULL);
    cc->eventLoop = externalEventloop;
    cc->externalEventLoop = true;
}

UA_StatusCode
getSecurityKeysAndStoreFetchedKeys(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_UInt32 startingTokenId = UA_REQ_CURRENT_TOKEN;
    UA_UInt32 requestKeyCount = UA_UINT32_MAX;

    if(ks->sksConfig.reqId != 0) {
        UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                    "SKS Client: SKS Pull request in process ");
        return UA_STATUSCODE_GOOD;
    }

    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(UA_ClientConfig));

    /* over write the client config with user specified SKS config */
    retval = UA_ClientConfig_copy(&ks->sksConfig.clientConfig, &cc);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    setServerEventloopOnSksClient(&cc, psm->sc.server->config.eventLoop);

    /* this is cleanedup in sksClientCleanupCb */
    sksClientContext *ctx   = (sksClientContext *)UA_calloc(1, sizeof(sksClientContext));
    if(!ctx)
         return UA_STATUSCODE_BADOUTOFMEMORY;
    ctx->ks = ks;
    ctx->psm = psm;
    ctx->startingTokenId = startingTokenId;
    ctx->requestedKeyCount = requestKeyCount;
    cc.clientContext = ctx;

    UA_Client *client = UA_Client_newWithConfig(&cc);
    if(!client)
        return retval;
    /* connect to sks server */
    retval = UA_Client_connectAsync(client, ks->sksConfig.endpointUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Failed to connect SKS server with error: %s ",
                     UA_StatusCode_name(retval));
        /* Make sure the client channel state is closed and not fresh,
         * otherwise, eventloop will keep waiting for the client status to go
         * from Fresh to closed in UA_Client_delete*/
        client->channel.state = UA_SECURECHANNELSTATE_CLOSED;
        /* this client instance will be cleared in the next event loop
         * iteration */
        addDelayedSksClientCleanupCb(client, ctx);
        return retval;
    }

    /* add user specified callback, if the client is properly configured. */
    client->config.stateCallback = onConnect;

    return retval;
}

UA_StatusCode
UA_Server_setSksClient(UA_Server *server, UA_String securityGroupId,
                       UA_ClientConfig *clientConfig, const char *endpointUrl,
                       UA_Server_sksPullRequestCallback callback, void *context) {
    if(!server || !clientConfig || !endpointUrl)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_BADNOTFOUND;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_find(psm, securityGroupId);
    if(!ks) {
        UA_UNLOCK(&server->serviceMutex);
        return retval;
    }

    UA_ClientConfig_copy(clientConfig, &ks->sksConfig.clientConfig);
    /*Clear the content of original config, so that no body can access the original config */
    clientConfig->authSecurityPolicies = NULL;
    clientConfig->certificateVerification.context = NULL;
    clientConfig->eventLoop = NULL;
    clientConfig->logging = NULL;
    clientConfig->securityPolicies = NULL;
    UA_ClientConfig_clear(clientConfig);

    ks->sksConfig.endpointUrl = endpointUrl;
    ks->sksConfig.userNotifyCallback = callback;
    ks->sksConfig.context = context;
    /* if keys are not previously fetched, then first call GetSecurityKeys*/
    if(ks->keyListSize == 0) {
        retval = getSecurityKeysAndStoreFetchedKeys(psm, ks);
    }
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

#endif
