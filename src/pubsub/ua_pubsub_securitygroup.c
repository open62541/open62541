/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/server_pubsub.h>

#include "ua_pubsub_ns0.h"

#ifdef UA_ENABLE_PUBSUB_SKS /* conditional compilation */

#include "server/ua_server_internal.h"

#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32

UA_SecurityGroup *
UA_SecurityGroup_findSGbyName(UA_Server *server, UA_String securityGroupName) {
    UA_SecurityGroup *tmpSG;
    TAILQ_FOREACH(tmpSG, &server->pubSubManager.securityGroups, listEntry) {
        if(UA_String_equal(&securityGroupName, &tmpSG->config.securityGroupName))
            return tmpSG;
    }
    return NULL;
}

UA_StatusCode
UA_SecurityGroupConfig_copy(const UA_SecurityGroupConfig *src,
                            UA_SecurityGroupConfig *dst) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    memcpy(dst, src, sizeof(UA_SecurityGroupConfig));
    if(UA_String_copy(&src->securityGroupName, &dst->securityGroupName) !=
       UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BAD;

    if(UA_String_copy(&src->securityPolicyUri, &dst->securityPolicyUri) !=
       UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BAD;
    return retval;
}

static UA_StatusCode
generateKeyData(const UA_PubSubSecurityPolicy *policy, UA_ByteString *key) {
    if(!key || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;

    /* Can't not found in specification for pubsub key generation, so use the idea of
     * securechannel, see specification 1.0.3 6.7.5 Deriving keys for more details
     In pubsub we do get have OpenSecureChannel request, so we cannot have Client or Server Nonce*/
    UA_Byte secretBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString secret;
    secret.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;
    secret.data = secretBytes;

    UA_Byte seedBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString seed;
    seed.data = seedBytes;
    seed.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;
    memset(seed.data, 0, seed.length);
    retVal = policy->symmetricModule.generateNonce(policy->policyContext, &secret);
    retVal |= policy->symmetricModule.generateNonce(policy->policyContext, &seed);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = policy->symmetricModule.generateKey(policy->policyContext, &secret, &seed, key);
    return retVal;
}

static void
updateSKSKeyStorage(UA_Server *server, UA_SecurityGroup *securityGroup){

    if(!securityGroup) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_PUBSUB,
                       "UpdateSKSKeyStorage callback failed with Error: %s ",
                       UA_StatusCode_name(UA_STATUSCODE_BADINVALIDARGUMENT));
        return;
    }

    UA_PubSubKeyStorage *keyStorage = securityGroup->keyStorage;

    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_ByteString newKey;
    size_t keyLength = keyStorage->policy->symmetricModule.secureChannelNonceLength;

    retval = UA_ByteString_allocBuffer(&newKey, keyLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_PUBSUB,
                       "UpdateSKSKeyStorage callback failed to allocate memory for new key with Error: %s ",
                       UA_StatusCode_name(retval));
        return;
    }

    generateKeyData(keyStorage->policy, &newKey);
    UA_UInt32 newKeyID = TAILQ_LAST(&keyStorage->keyList, keyListItems)->keyID;

    if(newKeyID >= UA_UINT32_MAX)
        newKeyID = 1;
    else
        ++newKeyID;

    if(keyStorage->keyListSize >= keyStorage->maxKeyListSize) {
        /* reusing the preallocated memory of the oldest key for the new key material */
        UA_PubSubKeyListItem *oldestKey = TAILQ_FIRST(&keyStorage->keyList);
        TAILQ_REMOVE(&keyStorage->keyList, oldestKey, keyListEntry);
        TAILQ_INSERT_TAIL(&keyStorage->keyList, oldestKey, keyListEntry);
        UA_ByteString_clear(&oldestKey->key);
        oldestKey->keyID = newKeyID;
        UA_ByteString_copy(&newKey, &oldestKey->key);
    } else {
        UA_PubSubKeyListItem *newItem =
            UA_PubSubKeyStorage_push(keyStorage, &newKey, newKeyID);
        if(!newItem) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_PUBSUB,
                           "UpdateSKSKeyStorage callback failed to add new key to the "
                           "sks keystorage for the SecurityGroup %.*s",
                           (int)securityGroup->securityGroupId.length,
                           securityGroup->securityGroupId.data);
            UA_Byte_delete(newKey.data);
            return;
        }
        keyStorage->keyListSize++;
    }

    UA_PubSubKeyListItem *nextCurrentItem = TAILQ_NEXT(keyStorage->currentItem, keyListEntry);
    if(nextCurrentItem)
        keyStorage->currentItem = nextCurrentItem;

    UA_EventLoop *el = server->config.eventLoop;
    securityGroup->baseTime = el->dateTime_nowMonotonic(el);

    /* We allocated memory for data with allocBuffer so now we free it */
    UA_ByteString_clear(&newKey);
}

static UA_StatusCode
initializeKeyStorageWithKeys(UA_Server *server, UA_SecurityGroup *securityGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_PubSubSecurityPolicy *policy =
        findPubSubSecurityPolicy(server, &securityGroup->config.securityPolicyUri);
    if(!policy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    UA_PubSubKeyStorage *ks = (UA_PubSubKeyStorage *)
        UA_calloc(1, sizeof(UA_PubSubKeyStorage));
    if(!ks)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_PubSubKeyStorage_init(server, ks, &securityGroup->securityGroupId,
                                 policy, securityGroup->config.maxPastKeyCount,
                                 securityGroup->config.maxFutureKeyCount);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ks);
        return retval;
    }

    ks->referenceCount++;
    securityGroup->keyStorage = ks;

    UA_ByteString currentKey;
    size_t keyLength = ks->policy->symmetricModule.secureChannelNonceLength;
    retval = UA_ByteString_allocBuffer(&currentKey, keyLength);
    retval = generateKeyData(ks->policy, &currentKey);

    UA_ByteString *futurekeys = (UA_ByteString *)UA_calloc(securityGroup->config.maxFutureKeyCount, sizeof(UA_ByteString));
    for(size_t i = 0; i < securityGroup->config.maxFutureKeyCount; i++) {
        retval = UA_ByteString_allocBuffer(&futurekeys[i], keyLength);
        retval = generateKeyData(ks->policy, &futurekeys[i]);
    }

    UA_UInt32 startingKeyId = 1;
    retval = UA_PubSubKeyStorage_storeSecurityKeys(server, securityGroup->keyStorage,
                                                   startingKeyId, &currentKey, futurekeys,
                                                   securityGroup->config.maxFutureKeyCount,
                                                   securityGroup->config.keyLifeTime);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_EventLoop *el = server->config.eventLoop;
    securityGroup->baseTime = el->dateTime_nowMonotonic(el);
    retval = addRepeatedCallback(server, (UA_ServerCallback)updateSKSKeyStorage,
                                 securityGroup, securityGroup->config.keyLifeTime,
                                 &securityGroup->callbackId);

cleanup:
    UA_Array_delete(futurekeys, securityGroup->config.maxFutureKeyCount,
                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_ByteString_clear(&currentKey);
    if(retval != UA_STATUSCODE_GOOD)
        UA_PubSubKeyStorage_delete(server, ks);
    return retval;
}

static UA_StatusCode
addSecurityGroup(UA_Server *server, UA_NodeId securityGroupFolderNodeId,
                 const UA_SecurityGroupConfig *securityGroupConfig,
                 UA_NodeId *securityGroupNodeId) {
    if(!securityGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /*check minimal config parameters*/
    if(!securityGroupConfig->keyLifeTime ||
       UA_String_isEmpty(&securityGroupConfig->securityGroupName) ||
       UA_String_isEmpty(&securityGroupConfig->securityPolicyUri))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(UA_SecurityGroup_findSGbyName(server, securityGroupConfig->securityGroupName))
        return UA_STATUSCODE_BADNODEIDEXISTS;

    UA_PubSubSecurityPolicy *policy =
        findPubSubSecurityPolicy(server, &securityGroupConfig->securityPolicyUri);
    if(!policy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    if(securityGroupConfig->securityGroupName.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_STATUSCODE_BAD;

    UA_SecurityGroup *newSecurityGroup =
        (UA_SecurityGroup *)UA_calloc(1, sizeof(UA_SecurityGroup));
    if(!newSecurityGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memset(newSecurityGroup, 0, sizeof(UA_SecurityGroup));
    UA_SecurityGroupConfig_copy(securityGroupConfig, &newSecurityGroup->config);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval = UA_NodeId_copy(&securityGroupFolderNodeId,
                            &newSecurityGroup->securityGroupFolderId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }
#endif

    retval = UA_String_copy(&securityGroupConfig->securityGroupName,
                            &newSecurityGroup->securityGroupId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }

    retval = initializeKeyStorageWithKeys(server, newSecurityGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval = addSecurityGroupRepresentation(server, newSecurityGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Add SecurityGroup failed with error: %s.",
                     UA_StatusCode_name(retval));
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newSecurityGroup->securityGroupNodeId);
#endif
    if(securityGroupNodeId)
        UA_NodeId_copy(&newSecurityGroup->securityGroupNodeId, securityGroupNodeId);

    TAILQ_INSERT_TAIL(&server->pubSubManager.securityGroups, newSecurityGroup, listEntry);

    server->pubSubManager.securityGroupsSize++;
    return retval;
}

UA_StatusCode
UA_Server_addSecurityGroup(UA_Server *server, UA_NodeId securityGroupFolderNodeId,
                           const UA_SecurityGroupConfig *securityGroupConfig,
                           UA_NodeId *securityGroupNodeId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = addSecurityGroup(server, securityGroupFolderNodeId,
                                            securityGroupConfig, securityGroupNodeId);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

UA_SecurityGroup *
UA_SecurityGroup_findSGbyId(UA_Server *server, UA_NodeId identifier) {
    UA_SecurityGroup *tmpSG;
    TAILQ_FOREACH(tmpSG, &server->pubSubManager.securityGroups, listEntry) {
        if(UA_NodeId_equal(&identifier, &tmpSG->securityGroupNodeId))
            return tmpSG;
    }
    return NULL;
}

static void
UA_SecurityGroupConfig_clear(UA_SecurityGroupConfig *config) {
    config->keyLifeTime = 0;
    config->maxFutureKeyCount = 0;
    UA_String_clear(&config->securityGroupName);
    UA_String_clear(&config->securityPolicyUri);
}

static void
UA_SecurityGroup_clear(UA_SecurityGroup *securityGroup) {
    UA_SecurityGroupConfig_clear(&securityGroup->config);
    UA_String_clear(&securityGroup->securityGroupId);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_NodeId_clear(&securityGroup->securityGroupFolderId);
#endif
    UA_NodeId_clear(&securityGroup->securityGroupNodeId);
}

void
UA_SecurityGroup_delete(UA_SecurityGroup *securityGroup) {
    UA_SecurityGroup_clear(securityGroup);
    UA_free(securityGroup);
}

void
removeSecurityGroup(UA_Server *server, UA_SecurityGroup *securityGroup) {
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, securityGroup->securityGroupNodeId, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.securityGroups, securityGroup, listEntry);
    server->pubSubManager.securityGroupsSize--;
    if(securityGroup->callbackId > 0)
        removeCallback(server, securityGroup->callbackId);

    if(securityGroup->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(server, securityGroup->keyStorage);
        securityGroup->keyStorage = NULL;
    }

    UA_SecurityGroup_delete(securityGroup);
}

UA_StatusCode
UA_Server_removeSecurityGroup(UA_Server *server, const UA_NodeId securityGroup) {
    UA_LOCK(&server->serviceMutex);
    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(server, securityGroup);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(sg) {
        removeSecurityGroup(server, sg);
    } else {
        res = UA_STATUSCODE_BADBOUNDNOTFOUND;
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif
