/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 * Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 * Copyright 2025 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_pubsub.h>

#ifdef UA_ENABLE_PUBSUB_SKS /* conditional compilation */

#include "ua_pubsub_internal.h"
#include "ua_pubsub_keystorage.h"

#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32

static void
UA_SecurityGroup_delete(UA_SecurityGroup *sg) {
    UA_String_clear(&sg->config.securityGroupName);
    UA_String_clear(&sg->config.securityPolicyUri);
    UA_String_clear(&sg->securityGroupId);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_NodeId_clear(&sg->securityGroupFolderId);
#endif
    UA_NodeId_clear(&sg->securityGroupNodeId);
    UA_free(sg);
}

UA_SecurityGroup *
UA_SecurityGroup_findByName(UA_PubSubManager *psm, const UA_String name) {
    if(!psm)
        return NULL;
    UA_SecurityGroup *sg;
    TAILQ_FOREACH(sg, &psm->securityGroups, listEntry) {
        if(UA_String_equal(&name, &sg->config.securityGroupName))
            break;
    }
    return sg;
}

UA_SecurityGroup *
UA_SecurityGroup_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_SecurityGroup *sg;
    TAILQ_FOREACH(sg, &psm->securityGroups, listEntry) {
        if(UA_NodeId_equal(&id, &sg->securityGroupNodeId))
            break;
    }
    return sg;
}

UA_StatusCode
UA_SecurityGroupConfig_copy(const UA_SecurityGroupConfig *src,
                            UA_SecurityGroupConfig *dst) {
    memcpy(dst, src, sizeof(UA_SecurityGroupConfig));
    dst->securityGroupName = UA_STRING_NULL;
    dst->securityPolicyUri = UA_STRING_NULL;

    UA_StatusCode res =
        UA_String_copy(&src->securityGroupName, &dst->securityGroupName);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_String_copy(&src->securityPolicyUri,
                             &dst->securityPolicyUri);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&dst->securityGroupName);
        UA_String_clear(&dst->securityPolicyUri);
    }
    return res;
}

static UA_StatusCode
generateKeyData(UA_PubSubSecurityPolicy *policy, UA_ByteString *key) {
    if(!key || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!policy->generateNonce || !policy->generateKey)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    UA_StatusCode retVal;

    /* Can't not found in specification for pubsub key generation, so use the
     * idea of securechannel, see specification 1.0.3 6.7.5 Deriving keys for
     * more details In pubsub we do get have OpenSecureChannel request, so we
     * cannot have Client or Server Nonce*/
    UA_Byte secretBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString secret;
    secret.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;
    secret.data = secretBytes;

    UA_Byte seedBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString seed;
    seed.data = seedBytes;
    seed.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;
    memset(seed.data, 0, seed.length);
    retVal = policy->generateNonce(policy, NULL, &secret);
    retVal |= policy->generateNonce(policy, NULL, &seed);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = policy->generateKey(policy, NULL, &secret, &seed, key);
    return retVal;
}

UA_StatusCode
UA_SecurityGroup_invalidateKeys(UA_PubSubManager *psm, UA_SecurityGroup *sg) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);
    if(!sg || !sg->keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_PubSubKeyStorage *ks = sg->keyStorage;
    if(!ks->policy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    UA_PubSubKeyListItem *last = TAILQ_LAST(&ks->keyList, keyListItems);
    if(!last || !ks->currentItem)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Keep historical keys, but replace the current and all future keys.
     * Build the complete replacement before changing the live storage. */
    UA_PubSubKeyStorage replacement;
    memset(&replacement, 0, sizeof(replacement));
    TAILQ_INIT(&replacement.keyList);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_ByteString key = UA_BYTESTRING_NULL;
    UA_PubSubKeyListItem *item;
    TAILQ_FOREACH(item, &ks->keyList, keyListEntry) {
        if(item == ks->currentItem)
            break;
        if(!UA_PubSubKeyStorage_push(&replacement, &item->key, item->keyID)) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
    }
    res = UA_ByteString_allocBuffer(&key, ks->policy->keyMaterialLength);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    UA_UInt32 tokenId = last->keyID;
    for(size_t i = 0; i <= (size_t)sg->config.maxFutureKeyCount; i++) {
        if(++tokenId == 0)
            tokenId = 1;
        res = generateKeyData(ks->policy, &key);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        item = UA_PubSubKeyStorage_push(&replacement, &key, tokenId);
        if(!item) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        if(i == 0)
            replacement.currentItem = item;
    }

    UA_PubSubKeyStorage_clearKeyList(ks);
    while((item = TAILQ_FIRST(&replacement.keyList))) {
        TAILQ_REMOVE(&replacement.keyList, item, keyListEntry);
        TAILQ_INSERT_TAIL(&ks->keyList, item, keyListEntry);
    }
    ks->currentItem = replacement.currentItem;
    ks->currentTokenId = ks->currentItem->keyID;
    ks->keyListSize = replacement.keyListSize;
    UA_EventLoop *el = psm->drv.server->config.eventLoop;
    sg->baseTime = el->dateTime_nowMonotonic(el);
    el->modifyTimer(el, sg->callbackId, sg->config.keyLifeTime,
                    NULL, UA_TIMERPOLICY_CURRENTTIME);
    res = UA_PubSubKeyStorage_activateKeyToChannelContext(
        psm, UA_NODEID_NULL, sg->securityGroupId);
cleanup:
    UA_ByteString_clear(&key);
    UA_PubSubKeyStorage_clearKeyList(&replacement);
    return res;
}

UA_StatusCode
UA_SecurityGroup_rotateKeys(UA_PubSubManager *psm, UA_SecurityGroup *sg) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);
    if(!sg || !sg->keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_PubSubKeyStorage *keyStorage = sg->keyStorage;
    UA_ByteString newKey = UA_BYTESTRING_NULL;
    UA_PubSubSecurityPolicy *sp = keyStorage->policy;
    if(!sp)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
    size_t keyLength = sp->keyMaterialLength;

    UA_StatusCode retval = UA_ByteString_allocBuffer(&newKey, keyLength);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "UpdateSKSKeyStorage callback failed to allocate memory for new key with Error: %s ",
                       UA_StatusCode_name(retval));
        return retval;
    }

    retval = generateKeyData(keyStorage->policy, &newKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "UpdateSKSKeyStorage callback failed to generate key: %s",
                       UA_StatusCode_name(retval));
        UA_ByteString_clear(&newKey);
        return retval;
    }
    UA_PubSubKeyListItem *last = TAILQ_LAST(&keyStorage->keyList, keyListItems);
    if(!last) {
        UA_ByteString_clear(&newKey);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_UInt32 newKeyID = last->keyID;

    if(newKeyID >= UA_UINT32_MAX)
        newKeyID = 1;
    else
        ++newKeyID;

    /* Capture the logical successor before an oldest item is moved to the
     * tail. Computing TAILQ_NEXT afterwards fails when the current item was
     * the item that got recycled. */
    UA_PubSubKeyListItem *nextCurrentItem = NULL;
    if(keyStorage->currentItem)
        nextCurrentItem = TAILQ_NEXT(keyStorage->currentItem, keyListEntry);
    UA_PubSubKeyListItem *generatedItem = NULL;

    if(keyStorage->keyListSize >= keyStorage->maxKeyListSize) {
        /* reusing the preallocated memory of the oldest key for the new key material */
        UA_PubSubKeyListItem *oldestKey = TAILQ_FIRST(&keyStorage->keyList);
        TAILQ_REMOVE(&keyStorage->keyList, oldestKey, keyListEntry);
        TAILQ_INSERT_TAIL(&keyStorage->keyList, oldestKey, keyListEntry);
        UA_ByteString_clear(&oldestKey->key);
        oldestKey->keyID = newKeyID;
        /* Transfer the generated key into the recycled item. This cannot fail
         * and avoids leaving an empty list item after a second allocation. */
        oldestKey->key = newKey;
        newKey = UA_BYTESTRING_NULL;
        generatedItem = oldestKey;
    } else {
        generatedItem = UA_PubSubKeyStorage_push(keyStorage, &newKey, newKeyID);
        if(!generatedItem) {
            UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                           "UpdateSKSKeyStorage callback failed to add new key to the "
                           "sks keystorage for the SecurityGroup %S",
                           sg->securityGroupId);
            UA_ByteString_clear(&newKey);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    if(!nextCurrentItem)
        nextCurrentItem = generatedItem;
    keyStorage->currentItem = nextCurrentItem;
    keyStorage->currentTokenId = nextCurrentItem->keyID;

    /* Activate the new current key to all channel contexts that use this
     * security group. Without this, the SKS server advances currentItem
     * (visible to GetSecurityKeys callers) but the local publisher/subscriber
     * channel keeps using the old key → messages signed with a stale key
     * while the token id reports the new one. */
    UA_PubSubKeyStorage_activateKeyToChannelContext(psm, UA_NODEID_NULL,
                                                    sg->securityGroupId);

    UA_EventLoop *el = psm->drv.server->config.eventLoop;
    sg->baseTime = el->dateTime_nowMonotonic(el);
    el->modifyTimer(el, sg->callbackId, sg->config.keyLifeTime,
                    NULL, UA_TIMERPOLICY_CURRENTTIME);

    /* We allocated memory for data with allocBuffer so now we free it */
    UA_ByteString_clear(&newKey);
    return UA_STATUSCODE_GOOD;
}

void
updateSKSKeyStorage(void *application /* UA_PubSubManager */,
                    void *context /* UA_SecurityGroup */) {
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_SecurityGroup *sg = (UA_SecurityGroup*)context;
    if(!sg) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "UpdateSKSKeyStorage callback failed with Error: %s ",
                       UA_StatusCode_name(UA_STATUSCODE_BADINVALIDARGUMENT));
        return;
    }

    /* EventLoop timer callbacks enter without the server lock. */
    lockServer(psm->drv.server);
    UA_SecurityGroup_rotateKeys(psm, sg);
    unlockServer(psm->drv.server);
}

static UA_StatusCode
initializeKeyStorageWithKeys(UA_PubSubManager *psm, UA_SecurityGroup *sg) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PubSubSecurityPolicy *policy =
        findPubSubSecurityPolicy(psm, &sg->config.securityPolicyUri);
    if(!policy)
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    UA_PubSubKeyStorage *ks = (UA_PubSubKeyStorage *)
        UA_calloc(1, sizeof(UA_PubSubKeyStorage));
    if(!ks)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_PubSubKeyStorage_init(psm, ks, &sg->securityGroupId,
                                 policy, sg->config.maxPastKeyCount,
                                 sg->config.maxFutureKeyCount);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ks);
        return retval;
    }

    ks->referenceCount++;
    sg->keyStorage = ks;

    UA_ByteString currentKey = UA_BYTESTRING_NULL;
    UA_ByteString *futurekeys = NULL;
    size_t keyLength = ks->policy->keyMaterialLength;
    retval = UA_ByteString_allocBuffer(&currentKey, keyLength);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    retval = generateKeyData(ks->policy, &currentKey);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(sg->config.maxFutureKeyCount > 0) {
        futurekeys = (UA_ByteString *)
            UA_calloc(sg->config.maxFutureKeyCount, sizeof(UA_ByteString));
        if(!futurekeys) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
    }
    for(size_t i = 0; i < sg->config.maxFutureKeyCount; i++) {
        retval = UA_ByteString_allocBuffer(&futurekeys[i], keyLength);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        retval = generateKeyData(ks->policy, &futurekeys[i]);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    UA_UInt32 startingKeyId = 1;
    retval |= (UA_PubSubKeyStorage_push(ks, &currentKey, startingKeyId)) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADOUTOFMEMORY;
    UA_PubSubKeyStorage_setCurrentKey(ks, startingKeyId);
    retval |= UA_PubSubKeyStorage_addSecurityKeys(ks, sg->config.maxFutureKeyCount,
                                                  futurekeys, startingKeyId);
    ks->keyLifeTime = sg->config.keyLifeTime;
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_EventLoop *el = psm->drv.server->config.eventLoop;
    sg->baseTime = el->dateTime_nowMonotonic(el);
    retval = el->addTimer(el, updateSKSKeyStorage, psm,
                          sg, sg->config.keyLifeTime, NULL,
                          UA_TIMERPOLICY_CURRENTTIME, &sg->callbackId);

cleanup:
    if(futurekeys)
        UA_Array_delete(futurekeys, sg->config.maxFutureKeyCount,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_ByteString_clear(&currentKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_PubSubKeyStorage_delete(psm, ks);
        sg->keyStorage = NULL;
    }
    return retval;
}

static UA_StatusCode
addSecurityGroup(UA_PubSubManager *psm, UA_NodeId securityGroupFolderNodeId,
                 const UA_SecurityGroupConfig *securityGroupConfig,
                 UA_NodeId *securityGroupNodeId) {
    if(!securityGroupConfig || !psm)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /*check minimal config parameters*/
    if(!securityGroupConfig->keyLifeTime ||
       UA_String_isEmpty(&securityGroupConfig->securityGroupName) ||
       UA_String_isEmpty(&securityGroupConfig->securityPolicyUri))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(UA_SecurityGroup_findByName(psm, securityGroupConfig->securityGroupName))
        return UA_STATUSCODE_BADNODEIDEXISTS;

    UA_PubSubSecurityPolicy *policy =
        findPubSubSecurityPolicy(psm, &securityGroupConfig->securityPolicyUri);
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
    retval = UA_SecurityGroupConfig_copy(securityGroupConfig,
                                         &newSecurityGroup->config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }

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

    retval = initializeKeyStorageWithKeys(psm, newSecurityGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval = addSecurityGroupRepresentation(psm->drv.server, newSecurityGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_SERVER,
                     "Add SecurityGroup failed with error: %s.",
                     UA_StatusCode_name(retval));
        UA_SecurityGroup_delete(newSecurityGroup);
        return retval;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &newSecurityGroup->securityGroupNodeId);
#endif
    if(securityGroupNodeId)
        UA_NodeId_copy(&newSecurityGroup->securityGroupNodeId, securityGroupNodeId);

    TAILQ_INSERT_TAIL(&psm->securityGroups, newSecurityGroup, listEntry);

    psm->securityGroupsSize++;
    return retval;
}

UA_StatusCode
UA_Server_addSecurityGroup(UA_Server *server, UA_NodeId securityGroupFolderNodeId,
                           const UA_SecurityGroupConfig *securityGroupConfig,
                           UA_NodeId *securityGroupNodeId) {
    if(!server || !securityGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retval = addSecurityGroup(psm, securityGroupFolderNodeId,
                                            securityGroupConfig, securityGroupNodeId);
    unlockServer(server);
    return retval;
}

void
UA_SecurityGroup_remove(UA_PubSubManager *psm, UA_SecurityGroup *sg) {
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(psm->drv.server, sg->securityGroupNodeId, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&psm->securityGroups, sg, listEntry);
    psm->securityGroupsSize--;
    if(sg->callbackId > 0)
        removeCallback(psm->drv.server, sg->callbackId);

    if(sg->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(psm, sg->keyStorage);
        sg->keyStorage = NULL;
    }

    UA_SecurityGroup_delete(sg);
}

UA_StatusCode
UA_Server_removeSecurityGroup(UA_Server *server, const UA_NodeId securityGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_SecurityGroup *sg = UA_SecurityGroup_find(psm, securityGroup);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(sg)
        UA_SecurityGroup_remove(psm, sg);
    else
        res = UA_STATUSCODE_BADBOUNDNOTFOUND;
    unlockServer(server);
    return res;
}

#endif
