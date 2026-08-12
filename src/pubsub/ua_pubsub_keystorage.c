/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include "ua_pubsub_keystorage.h"

#ifdef UA_ENABLE_PUBSUB_SKS /* conditional compilation */

#define UA_REQ_CURRENT_TOKEN 0

#include "../server/ua_server_internal.h"
#include "../client/ua_client_internal.h"

/* State retained by the asynchronous SKS client from connect until the
 * client has been shut down and deleted. */
typedef struct {
    UA_PubSubManager *psm;
    UA_PubSubKeyStorage *ks;
    UA_UInt32 startingTokenId;
    UA_UInt32 requestedKeyCount;
    UA_DelayedCallback dc;
    UA_Boolean deletingSynchronously;
} sksClientContext;

static void sksClientCleanupCb(void *client, void *context);
static void addDelayedSksClientCleanupCb(UA_Client *client,
                                         sksClientContext *context);

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

    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_ServerConfig *config = &psm->drv.server->config;
    for(size_t i = 0; i < config->pubSubConfig.securityPoliciesSize; i++) {
        if(UA_String_equal(securityPolicyUri,
                           &config->pubSubConfig.securityPolicies[i].policyUri))
            return &config->pubSubConfig.securityPolicies[i];
    }
    return NULL;
}

static void
prepareSksClientForDelete(UA_Client *client) {
    client->config.stateCallback = NULL;
    /* These members are borrowed from the key-storage client configuration. */
    client->config.securityPolicies = NULL;
    client->config.securityPoliciesSize = 0;
    client->config.authSecurityPolicies = NULL;
    client->config.authSecurityPoliciesSize = 0;
    client->config.certificateVerification.context = NULL;
    client->config.logging = NULL;
    client->config.clientContext = NULL;
}

void
UA_PubSubKeyStorage_clearKeyList(UA_PubSubKeyStorage *ks) {
    UA_PubSubKeyListItem *item, *item_tmp;
    TAILQ_FOREACH_SAFE(item, &ks->keyList, keyListEntry, item_tmp) {
        TAILQ_REMOVE(&ks->keyList, item, keyListEntry);
        UA_ByteString_clear(&item->key);
        UA_free(item);
    }
    ks->keyListSize = 0;
    /* Clear currentItem so the next rollover callback or
     * splitCurrentKeyMaterial does not dereference freed memory. */
    ks->currentItem = NULL;
    ks->currentTokenId = 0;
}

void
UA_PubSubKeyStorage_delete(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    /* Cancellation was already requested. Repeated manager-clear attempts
     * must not enqueue another disconnect/cleanup callback for the same
     * client. The existing callback retains ownership until final deletion. */
    if(ks->pendingDelete)
        return;

    /* requestActive covers connection setup, the method call and asynchronous
     * client shutdown. reqId alone misses the connection phase. */
    if(ks->sksConfig.requestActive) {
        ks->pendingDelete = true;

        UA_Client *client = ks->sksConfig.client;
        sksClientContext *ctx =
            (sksClientContext*)ks->sksConfig.clientContext;
        UA_EventLoop *el = psm->drv.server->config.eventLoop;

        /* Server shutdown stops the EventLoop before freeing drivers. At that
         * point no delayed callback will run, so finalize the client
         * synchronously and do not leave callback state pointing at psm. */
        if(client && ctx &&
           (el->state == UA_EVENTLOOPSTATE_STOPPED ||
            el->state == UA_EVENTLOOPSTATE_FRESH)) {
            /* UA_Client_delete cancels outstanding async service calls and
             * invokes their callbacks. Mark this path so storeFetchedKeys does
             * not recursively take the already-held server lock. */
            ctx->deletingSynchronously = true;
            prepareSksClientForDelete(client);
            UA_Client_delete(client);
            UA_free(ctx);
            ks->sksConfig.client = NULL;
            ks->sksConfig.clientContext = NULL;
            ks->sksConfig.reqId = 0;
            ks->sksConfig.requestActive = false;
            ks->pendingDelete = false;
            UA_PubSubKeyStorage_deleteNow(psm, ks);
            return;
        }

        /* During normal operation, actively cancel connection/request work.
         * The delayed cleanup is idempotent and owns final deletion. */
        if(client && ctx) {
            UA_Client_disconnectAsync(client);
            addDelayedSksClientCleanupCb(client, ctx);
        }
        return;
    }

    UA_PubSubKeyStorage_deleteNow(psm, ks);
}

void
UA_PubSubKeyStorage_deleteNow(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    if(ks->listed) {
        LIST_REMOVE(ks, keyStorageList);
        ks->listed = false;
    }

    /* Remove the key-rollover callback timer if it is armed.
     * The previous guard was inverted (`!ks->callBackId`), which removed the
     * timer only when callBackId was 0 (no timer) and skipped removal when a
     * timer was actually armed. The freed key storage was then dereferenced by
     * the EventLoop on the next rollover tick -> use-after-free. */
    if(ks->callBackId != 0) {
        removeCallback(psm->drv.server, ks->callBackId);
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
    ks->listed = true;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubKeyStorage_addSecurityKeys(UA_PubSubKeyStorage *ks, size_t keysSize,
                                     UA_ByteString *keys, UA_UInt32 currentKeyId) {
    if(!ks || (keysSize > 0 && !keys))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* nonceLength is the complete SKS key-material length for the PubSub
     * policy. Do not add the component lengths a second time. */
    if(!ks->policy || ks->policy->nonceLength == 0)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    size_t expectedLen = UA_PubSubSecurityPolicy_getKeyMaterialLength(ks->policy);
    for(size_t i = 0; i < keysSize; ++i) {
        if(keys[i].length != expectedLen || !keys[i].data)
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

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
        UA_malloc(sizeof(UA_PubSubKeyListItem));
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

    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_EventLoop *el = psm->drv.server->config.eventLoop;

    if(*callbackID != 0)
        el->removeTimer(el, *callbackID);

    return el->addTimer(el, callback, psm, ks,
                        timeToNextMs, NULL, UA_TIMERPOLICY_ONCE, callbackID);

}

static UA_StatusCode
splitCurrentKeyMaterial(UA_PubSubKeyStorage *ks, UA_ByteString *signingKey,
                        UA_ByteString *encryptingKey, UA_ByteString *keyNonce) {
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;
    if(!ks->policy)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* Guard against a NULL currentItem (can happen after clearKeyList). */
    if(!ks->currentItem)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_PubSubSecurityPolicy *policy = ks->policy;
    UA_ByteString key = ks->currentItem->key;

    /* Get key length according to policy */
    size_t signingkeyLength = policy->getSignatureKeyLength(policy, NULL);
    size_t encryptkeyLength = policy->getEncryptionKeyLength(policy, NULL);

    /* The full key material must be large enough to hold signing + encrypting
     * + nonce parts. Without this check, keyNonceLength below underflows
     * (size_t) when key.length is smaller than signingkeyLength + encryptkeyLength,
     * and keyNonce->data points past the buffer -> OOB read. */
    if(key.length != policy->nonceLength ||
       key.length < signingkeyLength + encryptkeyLength)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Rest of the part is the keyNonce */
    size_t keyNonceLength = key.length - signingkeyLength - encryptkeyLength;

    /* DivideKeys in origin ByteString */
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
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);
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
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

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
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);
    if(securityGroupId.data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubKeyStorage *ks =
        UA_PubSubKeyStorage_find(psm, securityGroupId);
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!ks->policy || !ks->currentItem || ks->keyListSize == 0)
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
nextGetSecuritykeysCallback(void *application /* UA_PubSubManager */,
                            void *context /* UA_PubSubKeyStorage */) {
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_PubSubKeyStorage *ks = (UA_PubSubKeyStorage*)context;
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
UA_PubSubKeyStorage_keyRolloverCallback(void *application /* UA_PubSubManager */,
                                        void *context /* UA_PubSubKeyStorage */) {
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_PubSubKeyStorage *ks = (UA_PubSubKeyStorage*)context;
    /* Callbacks from the EventLoop are initially unlocked */
    lockServer(psm->drv.server);

    UA_StatusCode retval =
        UA_PubSubKeyStorage_addKeyRolloverCallback(psm, ks,
                                                   UA_PubSubKeyStorage_keyRolloverCallback,
                                                   ks->keyLifeTime, &ks->callBackId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "Failed to update keys for security group id '%S'. Reason: '%s'.",
                     ks->securityGroupID, UA_StatusCode_name(retval));
    }

    if(ks->currentItem &&
       ks->currentItem != TAILQ_LAST(&ks->keyList, keyListItems)) {
        ks->currentItem = TAILQ_NEXT(ks->currentItem, keyListEntry);
        ks->currentTokenId = ks->currentItem->keyID;
        retval = UA_PubSubKeyStorage_activateKeyToChannelContext(psm, UA_NODEID_NULL,
                                                                 ks->securityGroupID);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                         "Failed to update keys for security group id '%S'. Reason: '%s'.",
                         ks->securityGroupID, UA_StatusCode_name(retval));
        }
    } else if(ks->sksConfig.endpointUrl && !ks->sksConfig.requestActive) {
        /* Publishers using a central SKS shall call GetSecurityKeys at a period
         * of half the KeyLifetime */
        UA_Duration msTimeToNextGetSecurityKeys = ks->keyLifeTime / 2;
        UA_EventLoop *el = psm->drv.server->config.eventLoop;
        retval = el->addTimer(el, nextGetSecuritykeysCallback, psm,
                              ks, msTimeToNextGetSecurityKeys, NULL,
                              UA_TIMERPOLICY_ONCE, NULL);
    }

    unlockServer(psm->drv.server);
}

void
UA_PubSubKeyStorage_detachKeyStorage(UA_PubSubManager *psm, UA_PubSubKeyStorage *ks) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);
    ks->referenceCount--;
    if(ks->referenceCount == 0)
        UA_PubSubKeyStorage_delete(psm, ks);
}

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
        prepareSksClientForDelete(sksClient);
        UA_Client_delete(sksClient);

        /* No client callback can access the key storage after the client has
         * been deleted. Release the asynchronous ownership and honor a
         * deletion that was requested during connect or the service call. */
        UA_PubSubManager *psm = ctx->psm;
        UA_PubSubKeyStorage *ks = ctx->ks;
        lockServer(psm->drv.server);
        ks->sksConfig.client = NULL;
        ks->sksConfig.clientContext = NULL;
        ks->sksConfig.reqId = 0;
        ks->sksConfig.requestActive = false;
        if(ks->pendingDelete)
            UA_PubSubKeyStorage_deleteNow(psm, ks);
        unlockServer(psm->drv.server);
        UA_free(context);
    } else {
        sksClient->config.eventLoop->
            addDelayedCallback(sksClient->config.eventLoop, &ctx->dc);
    }
}

static void
storeFetchedKeys(UA_Client *client, void *userdata, UA_UInt32 requestId,
                 UA_CallResponse *response);

UA_StatusCode
UA_PubSubKeyStorage_validateGetSecurityKeysResponse(
    const UA_CallResponse *response) {
    if(!response || response->resultsSize == 0 || !response->results)
        return UA_STATUSCODE_BADDECODINGERROR;

    const UA_CallMethodResult *result = &response->results[0];
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return result->statusCode;
    if(result->outputArgumentsSize < 5 || !result->outputArguments)
        return UA_STATUSCODE_BADDECODINGERROR;

    const UA_Variant *args = result->outputArguments;
    if(!UA_Variant_hasScalarType(&args[0], &UA_TYPES[UA_TYPES_STRING]) ||
       (!UA_Variant_hasScalarType(&args[1], &UA_TYPES[UA_TYPES_UINT32]) &&
        !UA_Variant_hasScalarType(&args[1], &UA_TYPES[UA_TYPES_INTEGERID])) ||
       !UA_Variant_hasArrayType(&args[2], &UA_TYPES[UA_TYPES_BYTESTRING]) ||
       args[2].arrayLength < 1 ||
       (!UA_Variant_hasScalarType(&args[3], &UA_TYPES[UA_TYPES_DURATION]) &&
        !UA_Variant_hasScalarType(&args[3], &UA_TYPES[UA_TYPES_DOUBLE])) ||
       (!UA_Variant_hasScalarType(&args[4], &UA_TYPES[UA_TYPES_DURATION]) &&
        !UA_Variant_hasScalarType(&args[4], &UA_TYPES[UA_TYPES_DOUBLE])))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    return UA_STATUSCODE_GOOD;
}

static void
storeFetchedKeys(UA_Client *client, void *userdata, UA_UInt32 requestId,
                 UA_CallResponse *response) {
    sksClientContext *ctx = (sksClientContext *)userdata;
    if(ctx->deletingSynchronously)
        return;
    UA_PubSubKeyStorage *ks = ctx->ks;
    UA_PubSubManager *psm = ctx->psm;
    UA_StatusCode retval = response ? response->responseHeader.serviceResult :
                                      UA_STATUSCODE_BADDECODINGERROR;

    lockServer(psm->drv.server);

    /* A deletion request cancels result processing. Final key-storage cleanup
     * happens only after the client is closed, in sksClientCleanupCb. */
    if(ks->pendingDelete) {
        ks->sksConfig.reqId = 0;
        UA_Client_disconnectAsync(client);
        addDelayedSksClientCleanupCb(client, ctx);
        unlockServer(psm->drv.server);
        return;
    }

    /* Check the service result before inspecting method output. */
    if(retval != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "SKS Client: Failed to call GetSecurityKeys on SKS server with error: %s ",
                     UA_StatusCode_name(retval));
        goto cleanup;
    }

    retval = UA_PubSubKeyStorage_validateGetSecurityKeysResponse(response);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "SKS Client: GetSecurityKeys returned malformed output arguments");
        goto cleanup;
    }

    UA_String *securityPolicyUri = (UA_String *)response->results->outputArguments[0].data;
    UA_UInt32 firstTokenId = *(UA_UInt32 *)response->results->outputArguments[1].data;
    UA_ByteString *keys = (UA_ByteString *)response->results->outputArguments[2].data;
    UA_UInt32 currentKeyCount = 1;
    UA_ByteString *currentKey = &keys[0];
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
        psm->drv.server->config.eventLoop->removeTimer(psm->drv.server->config.eventLoop,
                                                       ks->callBackId);
        ks->callBackId = 0;
    }

    UA_Duration msTimeToNextKey =
        *(UA_Duration *)response->results->outputArguments[3].data;
    if(!(msTimeToNextKey > 0))
        msTimeToNextKey = ks->keyLifeTime;
    retval = UA_PubSubKeyStorage_addKeyRolloverCallback(
        psm, ks, UA_PubSubKeyStorage_keyRolloverCallback,
        msTimeToNextKey, &ks->callBackId);

cleanup:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Failed to store the fetched keys from SKS server with error: %s",
                     UA_StatusCode_name(retval));
    }

    /* Call user callback to notify about the status */
    if(ks->sksConfig.userNotifyCallback)
        ks->sksConfig.userNotifyCallback(psm->drv.server, retval, ks->sksConfig.context);
    ks->sksConfig.reqId = 0;
    UA_Client_disconnectAsync(client);
    addDelayedSksClientCleanupCb(client, ctx);

    unlockServer(psm->drv.server);
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
    sksClientContext *ctx = (sksClientContext *)client->config.clientContext;
    UA_PubSubKeyStorage *ks = ctx->ks;
    if(ks->pendingDelete) {
        UA_Client_disconnectAsync(client);
        addDelayedSksClientCleanupCb(client, ctx);
        return;
    }

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
        if(ks->sksConfig.userNotifyCallback)
            ks->sksConfig.userNotifyCallback(ctx->psm->drv.server, connectStatus,
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

    if(ks->sksConfig.requestActive) {
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

    setServerEventloopOnSksClient(&cc, psm->drv.server->config.eventLoop);

    /* this is cleanedup in sksClientCleanupCb */
    sksClientContext *ctx   = (sksClientContext *)UA_calloc(1, sizeof(sksClientContext));
    if(!ctx) {
        UA_ClientConfig_clear(&cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    ctx->ks = ks;
    ctx->psm = psm;
    ctx->startingTokenId = startingTokenId;
    ctx->requestedKeyCount = requestKeyCount;
    cc.clientContext = ctx;
    /* Install the callback before creating/connecting the client. A fast
     * asynchronous connection must never complete while the callback is still
     * unset, otherwise the request remains permanently active. */
    cc.stateCallback = onConnect;

    UA_Client *client = UA_Client_newWithConfig(&cc);
    if(!client) {
        UA_free(ctx);
        UA_ClientConfig_clear(&cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Set before connectAsync: the connect callback already owns ks even
     * though no service request id has been assigned yet. */
    ks->sksConfig.client = client;
    ks->sksConfig.clientContext = ctx;
    ks->sksConfig.requestActive = true;
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

    return retval;
}

UA_StatusCode
UA_Server_setSksClient(UA_Server *server, UA_String securityGroupId,
                       UA_ClientConfig *clientConfig, const char *endpointUrl,
                       UA_Server_sksPullRequestCallback callback, void *context) {
    if(!server || !clientConfig || !endpointUrl)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_BADNOTFOUND;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_find(psm, securityGroupId);
    if(!ks) {
        unlockServer(server);
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
    unlockServer(server);
    return retval;
}

#endif
