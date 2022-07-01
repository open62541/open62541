/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include "ua_pubsub_keystorage.h"

#ifdef UA_ENABLE_PUBSUB_SKS /* conditional compilation */

#include "server/ua_server_internal.h"

UA_PubSubKeyStorage *
UA_Server_findKeyStorage(UA_Server *server, UA_String securityGroupId) {

    if(!server || UA_String_isEmpty(&securityGroupId))
        return NULL;

    UA_PubSubKeyStorage *outKeyStorage, *keyStorageTemp;
    LIST_FOREACH_SAFE(outKeyStorage, &server->pubSubManager.pubSubKeyList, keyStorageList,
                      keyStorageTemp) {
        if(UA_String_equal(&outKeyStorage->securityGroupID, &securityGroupId)) {
            return outKeyStorage;
        }
    }
    return NULL;
}

UA_StatusCode
UA_Server_findPubSubSecurityPolicy(UA_Server *server, const UA_String *securityPolicyUri,
                                   UA_PubSubSecurityPolicy **policy) {
    if(!server || !securityPolicyUri)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_BADNOTFOUND;

    UA_ServerConfig *config = UA_Server_getConfig(server);

    for(size_t i = 0; i < config->pubSubConfig.securityPoliciesSize; i++) {
        if(UA_String_equal(securityPolicyUri,
                           &config->pubSubConfig.securityPolicies[i].policyUri)) {
            *policy = &config->pubSubConfig.securityPolicies[i];
            retval = UA_STATUSCODE_GOOD;
            break;
        }
    }
    return retval;
}

static void
UA_PubSubKeyStorage_clearKeyList(UA_PubSubKeyStorage *keyStorage) {
    if(TAILQ_EMPTY(&keyStorage->keyList))
        return;

    UA_PubSubKeyListItem *item;
    TAILQ_FOREACH(item, &keyStorage->keyList, keyListEntry){
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

    /*Remove callback*/
    if(!keyStorage->callBackId) {
        removeCallback(server, keyStorage->callBackId);
        keyStorage->callBackId = 0;
    }

    UA_PubSubKeyStorage_clearKeyList(keyStorage);

    if(keyStorage->securityGroupID.data)
        UA_String_clear(&keyStorage->securityGroupID);

    memset(keyStorage, 0, sizeof(UA_PubSubKeyStorage));

    UA_free(keyStorage);
}

UA_StatusCode
UA_PubSubKeyStorage_init(UA_Server *server, const UA_String *securityGroupId,
                               const UA_String *securityPolicyUri,
                               UA_UInt32 maxPastKeyCount, UA_UInt32 maxFutureKeyCount,
                               UA_PubSubKeyStorage *keyStorage) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;

    if(!server || !securityPolicyUri || !securityGroupId || !keyStorage) {
        retval = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto error;
    }

    UA_PubSubSecurityPolicy *policy;

    UA_PubSubKeyStorage *tmpKeyStorage =
        UA_Server_findKeyStorage(server, *securityGroupId);
    if(tmpKeyStorage) {
        ++tmpKeyStorage->referenceCount;
        keyStorage = tmpKeyStorage;
        return UA_STATUSCODE_GOOD;
    }

    keyStorage->referenceCount = 1;

    retval = UA_String_copy(securityGroupId, &keyStorage->securityGroupID);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    retval = UA_Server_findPubSubSecurityPolicy(server, securityPolicyUri,
                                                              &policy);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
    keyStorage->policy = policy;

    UA_UInt32 currentkeyCount = 1;
    keyStorage->maxKeyListSize =
        maxPastKeyCount + currentkeyCount + maxFutureKeyCount;
    keyStorage->maxPastKeyCount = maxPastKeyCount;
    keyStorage->maxFutureKeyCount = maxFutureKeyCount;

    /*Add this keystorage to the server keystoragelist*/
    LIST_INSERT_HEAD(&server->pubSubManager.pubSubKeyList, keyStorage, keyStorageList);

    return retval;
error:
    UA_PubSubKeyStorage_delete(server, keyStorage);
    return retval;
}

UA_StatusCode
UA_PubSubKeyStorage_storeSecurityKeys(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                                      UA_UInt32 currentTokenId, const UA_ByteString *currentKey,
                                      UA_ByteString *futureKeys, size_t futureKeyCount,
                                      UA_Duration msKeyLifeTime) {

    UA_assert(server);

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

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_DateTime now = UA_DateTime_nowMonotonic();

    UA_DateTime dateTimeToNextKey = now + (UA_DateTime)(UA_DATETIME_MSEC * timeToNextMs);

    retval = UA_Server_addTimedCallback(server, callback, keyStorage, dateTimeToNextKey,
                                        callbackID);

    return retval;
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
    UA_ByteString tSigningKey = {signingkeyLength, key.data};
    UA_ByteString_copy(&tSigningKey, signingKey);

    UA_String tEncryptingKey = {encryptkeyLength, key.data + signingkeyLength};
    UA_ByteString_copy(&tEncryptingKey, encryptingKey);

    UA_ByteString tKeyNonce = {keyNonceLength,
                              key.data + signingkeyLength + encryptkeyLength};
    UA_ByteString_copy(&tKeyNonce, keyNonce);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setPubSubGroupEncryptingKey(UA_Server *server, UA_NodeId PubSubGroupId, UA_UInt32 securityTokenId,
                            UA_ByteString signingKey, UA_ByteString encryptingKey,
                            UA_ByteString keyNonce) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    retval = UA_Server_setWriterGroupEncryptionKeys(
        server, PubSubGroupId, securityTokenId, signingKey, encryptingKey, keyNonce);
    if(retval == UA_STATUSCODE_BADNOTFOUND)
        retval = UA_Server_setReaderGroupEncryptionKeys(
            server, PubSubGroupId, securityTokenId, signingKey, encryptingKey, keyNonce);

    return retval;
}

static UA_StatusCode
setPubSubGroupEncryptingKeyForMatchingSecurityGroupId(
    UA_Server *server, UA_String securityGroupId, UA_UInt32 securityTokenId,
    UA_ByteString signingKey, UA_ByteString encryptingKey, UA_ByteString keyNonce) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_PubSubConnection *tmpPubSubConnections;

    /* key storage is the same for all reader / writer groups, channel context isn't
     * => Update channelcontext in all Writergroups / ReaderGroups which have the same
     * securityGroupId*/
    TAILQ_FOREACH(tmpPubSubConnections, &server->pubSubManager.connections, listEntry) {
        /*ForEach writerGroup in server with matching SecurityGroupId*/
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &tmpPubSubConnections->writerGroups, listEntry) {

            if(UA_String_equal(&tmpWriterGroup->config.securityGroupId,
                               &securityGroupId)) {
                retval = UA_Server_setWriterGroupEncryptionKeys(
                    server, tmpWriterGroup->identifier, securityTokenId, signingKey,
                    encryptingKey, keyNonce);
                if(retval != UA_STATUSCODE_GOOD)
                    return retval;
            }
        }

        /*ForEach readerGroup in server with matching SecurityGroupId*/
        UA_ReaderGroup *tmpReaderGroup;
        LIST_FOREACH(tmpReaderGroup, &tmpPubSubConnections->readerGroups, listEntry) {

            if(UA_String_equal(&tmpReaderGroup->config.securityGroupId,
                               &securityGroupId)) {
                retval = UA_Server_setReaderGroupEncryptionKeys(
                    server, tmpReaderGroup->identifier, securityTokenId, signingKey,
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

    if(securityGroupId.data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_PubSubKeyStorage *keyStorage =
        UA_Server_findKeyStorage(server, securityGroupId);
    if(!keyStorage)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!keyStorage->policy && !(keyStorage->keyListSize > 0))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_UInt32 securityTokenId = keyStorage->currentItem->keyID;

    /*DivideKeys in origin ByteString*/
    UA_ByteString signingKey;
    UA_ByteString encryptKey;
    UA_ByteString keyNonce;
    splitCurrentKeyMaterial(keyStorage, &signingKey, &encryptKey, &keyNonce);

    if(!UA_NodeId_isNull(&pubSubGroupId))
        retval = setPubSubGroupEncryptingKey(server, pubSubGroupId, securityTokenId,
                                             signingKey, encryptKey, keyNonce);
    else
        retval = setPubSubGroupEncryptingKeyForMatchingSecurityGroupId(
            server, securityGroupId, securityTokenId, signingKey, encryptKey, keyNonce);

    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Failed to set Encrypting keys with Error: %s",
                     UA_StatusCode_name(retval));

    return retval;
}

void
UA_PubSubKeyStorage_keyRolloverCallback(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {

    UA_StatusCode retval = UA_PubSubKeyStorage_addKeyRolloverCallback(
        server, keyStorage, (UA_ServerCallback)UA_PubSubKeyStorage_keyRolloverCallback,
        keyStorage->keyLifeTime, &keyStorage->callBackId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                     (int)keyStorage->securityGroupID.length,
                     keyStorage->securityGroupID.data, UA_StatusCode_name(retval));
    }

    if(keyStorage->currentItem != TAILQ_LAST(&keyStorage->keyList, keyListItems)) {
        keyStorage->currentItem = TAILQ_NEXT(keyStorage->currentItem, keyListEntry);
        keyStorage->currentTokenId = keyStorage->currentItem->keyID;

        retval = UA_PubSubKeyStorage_activateKeyToChannelContext(
            server, UA_NODEID_NULL, keyStorage->securityGroupID);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(
                &server->config.logger, UA_LOGCATEGORY_SERVER,
                "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                (int)keyStorage->securityGroupID.length, keyStorage->securityGroupID.data,
                UA_StatusCode_name(retval));
        }
    }
}

UA_StatusCode
UA_PubSubKeyStorage_update(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                           const UA_ByteString *currentKey, UA_UInt32 currentKeyID,
                           const size_t futureKeySize, UA_ByteString *futureKeys,
                           UA_Duration msKeyLifeTime) {
    if(!keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_PubSubKeyListItem *keyListIterator = NULL;

    if (currentKeyID != 0){
        /*if currentKeyId is known then update keystorage currentItem*/
        retval = UA_PubSubKeyStorage_getKeyByKeyID(currentKeyID, keyStorage,
                                                   &keyListIterator);
        if (retval == UA_STATUSCODE_GOOD && keyListIterator) {
            keyStorage->currentItem = keyListIterator;
            /*Add new keys at the end of KeyList*/
            retval = UA_PubSubKeyStorage_storeSecurityKeys(
                server, keyStorage, currentKeyID, NULL,
                futureKeys, futureKeySize, msKeyLifeTime);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        } else if(retval == UA_STATUSCODE_BADNOTFOUND) {
            /*If the CurrentTokenId is unknown, the existing list shall be discarded and
             * replaced by the fetched list*/
            UA_PubSubKeyStorage_clearKeyList(keyStorage);
            retval = UA_PubSubKeyStorage_storeSecurityKeys(server, keyStorage,
                                                  currentKeyID, currentKey, futureKeys,
                                                  futureKeySize, msKeyLifeTime);
            if (retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }
    return retval;
}

void
UA_PubSubKeyStorage_removeKeyStorage(UA_Server *server, UA_PubSubKeyStorage *keyStorage) {
    if(!keyStorage) {
        return;
    }
    if(keyStorage->referenceCount > 1) {
        --keyStorage->referenceCount;
        return;
    }
    if(keyStorage->referenceCount == 1) {
        LIST_REMOVE(keyStorage, keyStorageList);
        UA_PubSubKeyStorage_delete(server, keyStorage);
    }
    return;
}
#endif
