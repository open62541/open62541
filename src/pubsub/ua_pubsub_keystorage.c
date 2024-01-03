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

#include "server/ua_server_internal.h"
#include "client/ua_client_internal.h"

UA_PubSubKeyStorage *
UA_PubSubKeyStorage_findKeyStorage(UA_Server *server, UA_String securityGroupId) {
    if(!server || UA_String_isEmpty(&securityGroupId))
        return NULL;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_PubSubKeyStorage *outKeyStorage;
    LIST_FOREACH(outKeyStorage, &server->pubSubManager.pubSubKeyList, keyStorageList) {
        if(UA_String_equal(&outKeyStorage->securityGroupID, &securityGroupId))
            return outKeyStorage;
    }
    return NULL;
}

UA_PubSubSecurityPolicy *
findPubSubSecurityPolicy(UA_Server *server, const UA_String *securityPolicyUri) {
    if(!server || !securityPolicyUri)
        return NULL;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_ServerConfig *config = &server->config;
    for(size_t i = 0; i < config->pubSubConfig.securityPoliciesSize; i++) {
        if(UA_String_equal(securityPolicyUri,
                           &config->pubSubConfig.securityPolicies[i].policyUri))
            return &config->pubSubConfig.securityPolicies[i];
    }
    return NULL;
}

static void
UA_PubSubKeyStorage_clearKeyList(UA_PubSubKeyStorage *keyStorage) {
    if(TAILQ_EMPTY(&keyStorage->keyList))
        return;

    UA_PubSubKeyListItem *item, *item_tmp;
    TAILQ_FOREACH_SAFE(item, &keyStorage->keyList, keyListEntry, item_tmp) {
        TAILQ_REMOVE(&keyStorage->keyList, item, keyListEntry);
        UA_ByteString_clear(&item->key);
        UA_free(item);
    }
    keyStorage->keyListSize = 0;
}

void
UA_PubSubKeyStorage_delete(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {
    UA_assert(keyStorage != NULL);
    UA_assert(server != NULL);

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove callback */
    if(!keyStorage->callBackId) {
        removeCallback(server, keyStorage->callBackId);
        keyStorage->callBackId = 0;
    }

    UA_PubSubKeyStorage_clearKeyList(keyStorage);
    UA_String_clear(&keyStorage->securityGroupID);
    UA_ClientConfig_clear(&keyStorage->sksConfig.clientConfig);
    UA_free(keyStorage);
}

UA_StatusCode
UA_PubSubKeyStorage_init(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                         const UA_String *securityGroupId,
                         UA_PubSubSecurityPolicy *policy,
                         UA_UInt32 maxPastKeyCount, UA_UInt32 maxFutureKeyCount) {
    UA_StatusCode res = UA_String_copy(securityGroupId, &keyStorage->securityGroupID);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_UInt32 currentkeyCount = 1;
    keyStorage->maxPastKeyCount = maxPastKeyCount;
    keyStorage->maxFutureKeyCount = maxFutureKeyCount;
    keyStorage->maxKeyListSize = maxPastKeyCount + currentkeyCount + maxFutureKeyCount;
    keyStorage->policy = policy;

    /* Add this keystorage to the server keystoragelist */
    LIST_INSERT_HEAD(&server->pubSubManager.pubSubKeyList, keyStorage, keyStorageList);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubKeyStorage_storeSecurityKeys(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                                      UA_UInt32 currentTokenId, const UA_ByteString *currentKey,
                                      UA_ByteString *futureKeys, size_t futureKeyCount,
                                      UA_Duration msKeyLifeTime) {
    UA_assert(server);
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_StatusCode retval = UA_STATUSCODE_BAD;

    if(futureKeyCount > 0 && !futureKeys) {
        retval = UA_STATUSCODE_BADARGUMENTSMISSING;
        goto error;
    }

    size_t keyNumber = futureKeyCount;

    if(currentKey && keyStorage->keyListSize == 0) {

        keyStorage->keyListSize++;
        UA_PubSubKeyListItem *keyItem =
            (UA_PubSubKeyListItem *)UA_calloc(1, sizeof(UA_PubSubKeyListItem));
        if(!keyItem)
            goto error;
        retval = UA_ByteString_copy(currentKey, &keyItem->key);
        if(UA_StatusCode_isBad(retval))
            goto error;

        keyItem->keyID = currentTokenId;

        TAILQ_INIT(&keyStorage->keyList);
        TAILQ_INSERT_HEAD(&keyStorage->keyList, keyItem, keyListEntry);
    }

    UA_PubSubKeyListItem *keyListIterator = TAILQ_FIRST(&keyStorage->keyList);
    UA_UInt32 startingTokenID = currentTokenId + 1;
    for(size_t i = 0; i < keyNumber; ++i) {
        retval = UA_PubSubKeyStorage_getKeyByKeyID(
            startingTokenID, keyStorage, &keyListIterator);
        /*Skipping key with matching KeyID in existing list*/
        if(retval == UA_STATUSCODE_BADNOTFOUND) {
            keyListIterator = UA_PubSubKeyStorage_push(keyStorage, &futureKeys[i], startingTokenID);
            if(!keyListIterator)
                goto error;

            keyStorage->keyListSize++;
        }
        if(startingTokenID == UA_UINT32_MAX)
            startingTokenID = 1;
        else
            ++startingTokenID;
    }

    /*update keystorage references*/
    retval = UA_PubSubKeyStorage_getKeyByKeyID(currentTokenId, keyStorage, &keyStorage->currentItem);
    if (retval != UA_STATUSCODE_GOOD && !keyStorage->currentItem)
        goto error;

    keyStorage->keyLifeTime = msKeyLifeTime;

    return retval;
error:
    if(keyStorage) {
        UA_PubSubKeyStorage_clearKeyList(keyStorage);
    }
    return retval;
}

UA_StatusCode
UA_PubSubKeyStorage_getKeyByKeyID(const UA_UInt32 keyId, UA_PubSubKeyStorage *keyStorage,
                                  UA_PubSubKeyListItem **keyItem) {

    if(!keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubKeyListItem *item;
    TAILQ_FOREACH(item, &keyStorage->keyList, keyListEntry){
        if(item->keyID == keyId) {
            *keyItem = item;
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_PubSubKeyListItem *
UA_PubSubKeyStorage_push(UA_PubSubKeyStorage *keyStorage, const UA_ByteString *key,
                         UA_UInt32 keyID) {
    UA_PubSubKeyListItem *newItem = (UA_PubSubKeyListItem *)malloc(sizeof(UA_PubSubKeyListItem));
    if (!newItem)
        return NULL;

    newItem->keyID = keyID;
    UA_ByteString_copy(key, &newItem->key);
    TAILQ_INSERT_TAIL(&keyStorage->keyList, newItem, keyListEntry);

    return TAILQ_LAST(&keyStorage->keyList, keyListItems);
}

UA_StatusCode
UA_PubSubKeyStorage_addKeyRolloverCallback(UA_Server *server,
                                           UA_PubSubKeyStorage *keyStorage,
                                           UA_ServerCallback callback,
                                           UA_Duration timeToNextMs,
                                           UA_UInt64 *callbackID) {
    if(!server || !keyStorage || !callback || timeToNextMs <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime dateTimeToNextKey = el->dateTime_nowMonotonic(el) +
        (UA_DateTime)(UA_DATETIME_MSEC * timeToNextMs);
    return el->addTimedCallback(el, (UA_Callback)callback, server, keyStorage,
                                dateTimeToNextKey, callbackID);
}

static UA_StatusCode
splitCurrentKeyMaterial(UA_PubSubKeyStorage *keyStorage, UA_ByteString *signingKey,
                        UA_ByteString *encryptingKey, UA_ByteString *keyNonce) {
    if(!keyStorage)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!keyStorage->policy)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_PubSubSecurityPolicy *policy = keyStorage->policy;

    UA_ByteString key = keyStorage->currentItem->key;

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
setPubSubGroupEncryptingKey(UA_Server *server, UA_NodeId PubSubGroupId, UA_UInt32 securityTokenId,
                            UA_ByteString signingKey, UA_ByteString encryptingKey,
                            UA_ByteString keyNonce) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode retval =
        setWriterGroupEncryptionKeys(server, PubSubGroupId, securityTokenId,
                                     signingKey, encryptingKey, keyNonce);
    if(retval == UA_STATUSCODE_BADNOTFOUND)
        retval = setReaderGroupEncryptionKeys(server, PubSubGroupId, securityTokenId,
                                              signingKey, encryptingKey, keyNonce);
    return retval;
}

static UA_StatusCode
setPubSubGroupEncryptingKeyForMatchingSecurityGroupId(UA_Server *server,
                                                      UA_String securityGroupId,
                                                      UA_UInt32 securityTokenId,
                                                      UA_ByteString signingKey,
                                                      UA_ByteString encryptingKey,
                                                      UA_ByteString keyNonce) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_PubSubConnection *tmpPubSubConnections;

    /* Key storage is the same for all reader / writer groups, channel context isn't
     * => Update channelcontext in all Writergroups / ReaderGroups which have the same
     * securityGroupId*/
    TAILQ_FOREACH(tmpPubSubConnections, &server->pubSubManager.connections, listEntry) {
        /* For each writerGroup in server with matching SecurityGroupId */
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &tmpPubSubConnections->writerGroups, listEntry) {
            if(UA_String_equal(&tmpWriterGroup->config.securityGroupId, &securityGroupId)) {
                retval = setWriterGroupEncryptionKeys(server, tmpWriterGroup->identifier,
                                                      securityTokenId, signingKey,
                                                      encryptingKey, keyNonce);
                if(retval != UA_STATUSCODE_GOOD)
                    return retval;
            }
        }

        /* For each readerGroup in server with matching SecurityGroupId */
        UA_ReaderGroup *tmpReaderGroup;
        LIST_FOREACH(tmpReaderGroup, &tmpPubSubConnections->readerGroups, listEntry) {
            if(UA_String_equal(&tmpReaderGroup->config.securityGroupId, &securityGroupId)) {
                retval = setReaderGroupEncryptionKeys(server, tmpReaderGroup->identifier,
                                                      securityTokenId, signingKey,
                                                      encryptingKey, keyNonce);
                if(retval != UA_STATUSCODE_GOOD)
                    return retval;
            }
        }
    }
    return retval;
}

UA_StatusCode
UA_PubSubKeyStorage_activateKeyToChannelContext(UA_Server *server, UA_NodeId pubSubGroupId,
                                                UA_String securityGroupId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(securityGroupId.data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubKeyStorage *keyStorage =
        UA_PubSubKeyStorage_findKeyStorage(server, securityGroupId);
    if(!keyStorage)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!keyStorage->policy && !(keyStorage->keyListSize > 0))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_UInt32 securityTokenId = keyStorage->currentItem->keyID;

    /*DivideKeys in origin ByteString*/
    UA_ByteString signingKey;
    UA_ByteString encryptKey;
    UA_ByteString keyNonce;
    UA_StatusCode retval = splitCurrentKeyMaterial(keyStorage, &signingKey,
                                                   &encryptKey, &keyNonce);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(!UA_NodeId_isNull(&pubSubGroupId))
        retval = setPubSubGroupEncryptingKey(server, pubSubGroupId, securityTokenId,
                                             signingKey, encryptKey, keyNonce);
    else
        retval = setPubSubGroupEncryptingKeyForMatchingSecurityGroupId(
            server, securityGroupId, securityTokenId, signingKey, encryptKey, keyNonce);

    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Failed to set Encrypting keys with Error: %s",
                     UA_StatusCode_name(retval));

    return retval;
}

static void
nextGetSecuritykeysCallback(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    if(!keyStorage) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "GetSecurityKeysCall Failed with error: KeyStorage does not exist "
                     "in the server");
        return;
    }
    retval = getSecurityKeysAndStoreFetchedKeys(server, keyStorage);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "GetSecurityKeysCall Failed with error: %s ",
                     UA_StatusCode_name(retval));
}

void
UA_PubSubKeyStorage_keyRolloverCallback(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {
    /* Callbacks from the EventLoop are initially unlocked */
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval =
        UA_PubSubKeyStorage_addKeyRolloverCallback(server, keyStorage,
                                     (UA_ServerCallback)UA_PubSubKeyStorage_keyRolloverCallback,
                                                   keyStorage->keyLifeTime, &keyStorage->callBackId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                     (int)keyStorage->securityGroupID.length,
                     keyStorage->securityGroupID.data, UA_StatusCode_name(retval));
    }

    if(keyStorage->currentItem != TAILQ_LAST(&keyStorage->keyList, keyListItems)) {
        keyStorage->currentItem = TAILQ_NEXT(keyStorage->currentItem, keyListEntry);
        keyStorage->currentTokenId = keyStorage->currentItem->keyID;
        retval = UA_PubSubKeyStorage_activateKeyToChannelContext(server, UA_NODEID_NULL,
                                                                 keyStorage->securityGroupID);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                         (int)keyStorage->securityGroupID.length, keyStorage->securityGroupID.data,
                         UA_StatusCode_name(retval));
        }
    } else if(keyStorage->sksConfig.endpointUrl && keyStorage->sksConfig.reqId == 0) {
        UA_EventLoop *el = server->config.eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        /*Publishers using a central SKS shall call GetSecurityKeys at a period of half the KeyLifetime */
        UA_Duration msTimeToNextGetSecurityKeys = keyStorage->keyLifeTime / 2;
        UA_DateTime dateTimeToNextGetSecurityKeys =
            now + (UA_DateTime)(UA_DATETIME_MSEC * msTimeToNextGetSecurityKeys);
        retval = server->config.eventLoop->addTimedCallback(
            server->config.eventLoop, (UA_Callback)nextGetSecuritykeysCallback, server,
            keyStorage, dateTimeToNextGetSecurityKeys, NULL);
    }
    UA_UNLOCK(&server->serviceMutex);
}

UA_StatusCode
UA_PubSubKeyStorage_update(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                           const UA_ByteString *currentKey, UA_UInt32 currentKeyID,
                           const size_t futureKeySize, UA_ByteString *futureKeys,
                           UA_Duration msKeyLifeTime) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(!keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_PubSubKeyListItem *keyListIterator = NULL;

    if(currentKeyID != 0){
        /* If currentKeyId is known then update keystorage currentItem */
        retval = UA_PubSubKeyStorage_getKeyByKeyID(currentKeyID, keyStorage,
                                                   &keyListIterator);
        if(retval == UA_STATUSCODE_GOOD && keyListIterator) {
            keyStorage->currentItem = keyListIterator;
            /* Add new keys at the end of KeyList */
            retval = UA_PubSubKeyStorage_storeSecurityKeys(server, keyStorage, currentKeyID,
                                                           NULL, futureKeys, futureKeySize,
                                                           msKeyLifeTime);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        } else if(retval == UA_STATUSCODE_BADNOTFOUND) {
            /* If the CurrentTokenId is unknown, the existing list shall be
             * discarded and replaced by the fetched list */
            UA_PubSubKeyStorage_clearKeyList(keyStorage);
            retval = UA_PubSubKeyStorage_storeSecurityKeys(server, keyStorage,
                                                           currentKeyID, currentKey, futureKeys,
                                                           futureKeySize, msKeyLifeTime);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }
    return retval;
}

void
UA_PubSubKeyStorage_detachKeyStorage(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    keyStorage->referenceCount--;
    if(keyStorage->referenceCount == 0) {
        LIST_REMOVE(keyStorage, keyStorageList);
        UA_PubSubKeyStorage_delete(server, keyStorage);
    }
}

/**
 * @brief It holds the information required in the async callback to
 * GetSecurityKeys method Call.
 */
typedef struct {
    UA_Server *server;
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
    UA_Server *server = ctx->server;
    UA_StatusCode retval = response->responseHeader.serviceResult;

    UA_LOCK(&server->serviceMutex);
    /* check if the call to getSecurityKeys was a success */
    if(response->resultsSize != 0)
        retval = response->results->statusCode;
    if(retval != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
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

    if(ks->keyListSize == 0) {
        retval = UA_PubSubKeyStorage_storeSecurityKeys(server, ks, firstTokenId,
                                                       currentKey, futureKeys,
                                                       futureKeySize, msKeyLifeTime);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    } else {
        retval = UA_PubSubKeyStorage_update(server, ks, currentKey, firstTokenId,
                                            futureKeySize, futureKeys, msKeyLifeTime);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /**
     * After a new batch of keys is fetched from SKS server, the key storage is updated
     * with new keys and new keylifetime. Also the remaining time for current
     * keyRollover is also returned. When setting a new keyRollover callback, the
     * previous callback must be removed so that the keyRollover does not happen twice
     */
    if(ks->callBackId != 0) {
        server->config.eventLoop->removeCyclicCallback(server->config.eventLoop,
                                                       ks->callBackId);
        ks->callBackId = 0;
    }

    UA_Duration msTimeToNextKey =
        *(UA_Duration *)response->results->outputArguments[3].data;
    if(!(msTimeToNextKey > 0))
        msTimeToNextKey = ks->keyLifeTime;
    retval = UA_PubSubKeyStorage_addKeyRolloverCallback(
        server, ks, (UA_ServerCallback)UA_PubSubKeyStorage_keyRolloverCallback,
        msTimeToNextKey, &ks->callBackId);

cleanup:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Failed to store the fetched keys from SKS server with error: %s",
                     UA_StatusCode_name(retval));
    }
    /* call user callback to notify about the status */
    UA_UNLOCK(&server->serviceMutex);
    if(ks->sksConfig.userNotifyCallback)
        ks->sksConfig.userNotifyCallback(server, retval, ks->sksConfig.context);
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

    UA_StatusCode retval = UA_Client_call_async(client, objectId, methodId, inputArgumentsSize,
                                inputArguments, storeFetchedKeys, (void *)ctx, &ctx->ks->sksConfig.reqId);
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
            ks->sksConfig.userNotifyCallback(ctx->server, connectStatus,
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
getSecurityKeysAndStoreFetchedKeys(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_UInt32 startingTokenId = UA_REQ_CURRENT_TOKEN;
    UA_UInt32 requestKeyCount = UA_UINT32_MAX;

    if(keyStorage->sksConfig.reqId != 0) {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "SKS Client: SKS Pull request in process ");
        return UA_STATUSCODE_GOOD;
    }

    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(UA_ClientConfig));

    /* over write the client config with user specified SKS config */
    retval = UA_ClientConfig_copy(&keyStorage->sksConfig.clientConfig, &cc);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    setServerEventloopOnSksClient(&cc, server->config.eventLoop);

    /* this is cleanedup in sksClientCleanupCb */
    sksClientContext *ctx   = (sksClientContext *)UA_calloc(1, sizeof(sksClientContext));
    if(!ctx)
         return UA_STATUSCODE_BADOUTOFMEMORY;
    ctx->ks = keyStorage;
    ctx->server = server;
    ctx->startingTokenId = startingTokenId;
    ctx->requestedKeyCount = requestKeyCount;
    cc.clientContext = ctx;

    UA_Client *client = UA_Client_newWithConfig(&cc);
    if(!client)
        return retval;
    /* connect to sks server */
    retval = UA_Client_connectAsync(client, keyStorage->sksConfig.endpointUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Failed to connect SKS server with error: %s ",
                     UA_StatusCode_name(retval));
        /* Make sure the client channel state is closed and not fresh, otherwise, eventloop will
        keep waiting for the client status to go from Fresh to closed in UA_Client_delete*/
        client->channel.state = UA_SECURECHANNELSTATE_CLOSED;
        /* this client instance will be cleared in the next event loop iteration */
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
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, securityGroupId);
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
        retval = getSecurityKeysAndStoreFetchedKeys(server, ks);
    }
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

#endif
