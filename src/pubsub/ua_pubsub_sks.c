#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB_SECURITY /* conditional compilation */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_sks.h>

#include "ua_pubsub_sks.h"

/**
 * Debug control log levels
 * Todo: remove
 */
#define UA_CUSTOM_SHOW_KEYSTORAGE_LOGLEVEL 250

/**
 * Todo: remove
 */
#define UA_CUSTOM_UPDATEKEY_INTERVAL 10000

/**
 * length of the nonce of the generated keys
 */
#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32

/**
 * value of uninitialized ids in the key storage
 */
#define UA_PUBSUB_SKS_KEYLIST_EMPTY_SLOT_ID 0

/**
 * number of current keys in the sks key list
 */
#define NUMBER_OF_CURRENT_KEYS 1

/**
 * Debug output for byte string.
 * Todo: remove
 */
static void
UA_Print_ByteStringHex(const UA_ByteString byteString, const char *name) {
    printf("\n%s:\n", name);
    for(size_t i = 0; i < byteString.length; i++) {
        printf("%02x", byteString.data[i]);
    }
}

/**
 * Debug output of security keys
 * Todo: remove
 */
static void
UA_tempDebugger(const UA_Server *server, int loglevel) {
    UA_PubSubSKSKeyStorage *keyStorageEntry;
    if(UA_LOGLEVEL < loglevel) {
        LIST_FOREACH(keyStorageEntry, &server->pubSubSKSKeyList, keyStorageList) {
            printf("\n%s\n", (char *)keyStorageEntry->securityGroupID.data);
            for(size_t i = 0; i < keyStorageEntry->keyListSize; ++i) {
                printf("\nKeyID%d\n", keyStorageEntry->keyList[i].keyID);
                UA_Print_ByteStringHex(keyStorageEntry->keyList[i].key, "");
            }
        }
    }
    return;
}

/* see header for documentation */
UA_StatusCode
UA_SecurityPolicy_findPolicyBySecurityPolicyUri(const UA_Server *server,
                                                const UA_String securityPolicyUri,
                                                UA_SecurityPolicy **policy) {

    if(!server || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_ServerConfig *config = &server->config;

    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        if(UA_String_equal(&securityPolicyUri, &config->securityPolicies[i].policyUri)) {
            *policy = &config->securityPolicies[i];
            return UA_STATUSCODE_GOOD;
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

/**
 * Generate one Key according to the security policy.
 * The key generated is a combination of signingKey, encryptionKey and keyNonce
 * which will be divided later on publisher/subscriber
 * It needs 2 parameter, one secret and one seed.
 * It can be enhanced by choosing larger secret and seed.
 * @param policy the security policy
 * @param key a byteString key
 * @return UA_STATUSCODE_GOOD on success, otherwise
 * return value of symmetricModule.generateNonce or symmetricModule.generateKey
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
UA_generateKey(const UA_SecurityPolicy *policy, UA_ByteString *key) {
    if(!key || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;

    /* Can't not found in specification for pubsub key generation, so use the idea of
     * securechannel, see specification part6 p51 for more details*/

    UA_Byte secretBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString secret;
    secret.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;
    secret.data = secretBytes;

    UA_Byte seedBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString seed;
    seed.data = seedBytes;
    seed.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;

    retVal = policy->symmetricModule.generateNonce(policy, &secret);
    retVal |= policy->symmetricModule.generateNonce(policy, &seed);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = policy->symmetricModule.generateKey(policy, &secret, &seed, key);
    return retVal;
}

/**
 * initiate the keyStorage and store it in server after a securityGroup is added.
 * On success, memory is allocated which should be freed by UA_PubSubKeySKSStorage_delete
 * or UA_PubSubKeySKSStorage_deleteMembers
 * @param keyStorage the structure which saves all the keys
 * @param policy the security policy
 * @param maxPastKeyNumber max number of past keys that are going to be stored
 * @param maxFutureKeyNumber max number of future keys that are going to be stored
 * @param securityGroupId security group id related to the storage
 * @return UA_STATUSCODE_GOOD on success, UA_STATUSCODE_BADOUTOFMEMORY when out of memory
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 * error codes of UA_generateKey
 */
static UA_StatusCode
UA_initKeyStorage(UA_PubSubSKSKeyStorage *keyStorage, UA_SecurityPolicy *policy,
                  const UA_UInt32 maxPastKeyNumber, const UA_UInt32 maxFutureKeyNumber,
                  const UA_String* securityGroupId) {
    if(!keyStorage || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;
    size_t keyListSize = maxPastKeyNumber + NUMBER_OF_CURRENT_KEYS + maxFutureKeyNumber;

    keyStorage->keyList = NULL;
    keyStorage->getSecurityKeysKeyBuffer = NULL;
    keyStorage->referenceCount = 1;

    /*Copy information*/
    keyStorage->maxPastKeyNumber = maxPastKeyNumber;
    keyStorage->maxFutureKeyNumber = maxFutureKeyNumber;
    retVal = UA_String_copy(securityGroupId, &keyStorage->securityGroupID);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    keyStorage->policy = policy;

    /*Initial the keyQueue*/
    keyStorage->keyListSize = keyListSize;
    keyStorage->keyList =
        (UA_PubSubKeyListItem *)UA_calloc(keyListSize, sizeof(UA_PubSubKeyListItem));
    if(!keyStorage->keyList) {
        retVal = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* preallocate key list */
    size_t keyLength = policy->symmetricModule.secureChannelNonceLength;
    UA_PubSubKeyListItem *lastItem = NULL;
    for(UA_UInt32 i = 0; i < keyListSize; ++i) {
        UA_PubSubKeyListItem *item = &keyStorage->keyList[i];

        if(lastItem)
            lastItem->next = item;
        lastItem = item;

        item->key.length = keyLength;
        item->next = NULL;

        item->key.data = (UA_Byte *)UA_malloc(keyLength);
        if(!item->key.data) {
            retVal = UA_STATUSCODE_BADOUTOFMEMORY;
            goto error;
        }
        if(i < maxPastKeyNumber) {
            item->keyID = UA_PUBSUB_SKS_KEYLIST_EMPTY_SLOT_ID;
        } else if(i > maxPastKeyNumber) {
            item->keyID = i - maxPastKeyNumber + 1;
        } else {
            keyStorage->currentItem = item;
            item->keyID = 1;
        }
    }

    /* initialize current key and future keys */
    for(UA_UInt32 i = maxPastKeyNumber; i < keyListSize; ++i) {
        retVal = UA_generateKey(policy, &keyStorage->keyList[i].key);
        if(retVal != UA_STATUSCODE_GOOD)
            goto error;
    }

    keyStorage->lastItem = lastItem;
    keyStorage->firstItem = keyStorage->keyList;

    UA_assert(keyListSize != 1 || (keyStorage->lastItem == keyStorage->firstItem &&
                                   keyStorage->firstItem->next == NULL));

    /* create buffer which is reused by UA_getSecurityKeysBatch */
    keyStorage->getSecurityKeysKeyBuffer =
        (UA_ByteString *)UA_malloc(keyListSize * sizeof(UA_ByteString));
    if(!keyStorage->getSecurityKeysKeyBuffer) {
        retVal = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    return UA_STATUSCODE_GOOD;

error:
    UA_PubSubKeySKSStorage_deleteMembers(NULL, keyStorage);
    return retVal;
}

/* see header for documentation */
UA_StatusCode
UA_getPubSubKeyStoragebySecurityGroupId(const UA_Server *server,
                                        const UA_String *securityGroupId,
                                        UA_PubSubSKSKeyStorage **keyStorage) {

    if(!server || !securityGroupId || !keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubSKSKeyStorage *keyStorageIterator;
    LIST_FOREACH(keyStorageIterator, &server->pubSubSKSKeyList, keyStorageList) {
        if(UA_ByteString_equal(securityGroupId, &keyStorageIterator->securityGroupID)) {
            *keyStorage = keyStorageIterator;
            return UA_STATUSCODE_GOOD;
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

/* see header for documentation */
UA_StatusCode
UA_PubSubSKSKeyStorage_new(UA_PubSubSKSKeyStorage **storageForNewPointer) {

    if(!storageForNewPointer)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    *storageForNewPointer = NULL;

    UA_PubSubSKSKeyStorage *keyStorage =
        (UA_PubSubSKSKeyStorage *)UA_malloc(sizeof(UA_PubSubSKSKeyStorage));
    if(!keyStorage)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* initialize to have possibility to delete it in case of error somewhere */
    memset(keyStorage, 0, sizeof(UA_PubSubSKSKeyStorage));

    *storageForNewPointer = keyStorage;

    return UA_STATUSCODE_GOOD;
}

/* see header for documentation */
void
UA_PubSubKeySKSStorage_delete(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage) {

    if(!keyStorage)
        return;

    UA_PubSubKeySKSStorage_deleteMembers(server, keyStorage);
    UA_free(keyStorage);
}

/* see header for documentation */
void
UA_PubSubKeySKSStorage_deleteMembers(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage) {

    if(!keyStorage)
        return;

    LIST_REMOVE(keyStorage, keyStorageList);

    /*Remove callback*/
    if(server) {
        if(keyStorage->callBackRegistered == UA_TRUE) {
            UA_Server_removeCallback(server, keyStorage->callBackId);
        }
    }

    if(keyStorage->keyList) {

        /* if malloc of keyItem failed -> this item is NULL, remaining are uninitialized
         * -> free items until one is NULL */
        for(UA_UInt32 i = 0; i < keyStorage->keyListSize; ++i) {
            if(keyStorage->keyList[i].key.data)
                UA_free(keyStorage->keyList[i].key.data);
            else
                break;
        }

        UA_free(keyStorage->keyList);
    }

    if(keyStorage->getSecurityKeysKeyBuffer) {
        UA_free(keyStorage->getSecurityKeysKeyBuffer);
    }

    if(keyStorage->securityGroupID.data)
        UA_String_deleteMembers(&keyStorage->securityGroupID);

    memset(keyStorage, 0, sizeof(UA_PubSubSKSKeyStorage));
}

/* see header for documentation */
void
UA_PubSubKeySKSStorage_deleteSingle(UA_Server *server, const UA_String *securityGroupId) {
    /*It is stored on server, remove it*/
    UA_PubSubSKSKeyStorage *keyStorage;
    UA_StatusCode foundKeyStorage =
        UA_getPubSubKeyStoragebySecurityGroupId(server, securityGroupId, &keyStorage);
    if(foundKeyStorage == UA_STATUSCODE_GOOD)
        UA_PubSubKeySKSStorage_delete(server, keyStorage);
}

/**
 * Get SecurityKeys from the Stored keyList and stores them in the provided keyVariant
 * @param startingTokenId id of the first key to store, if 0 the current key is assumed
 * @param requestedKeyCount number of requested key, actual number of stored keys depends
 * also on the number of keys returned by getSecurityKeys
 * @param keyStorage a local list which stores the keys
 * @param firstTokenId on success contains the id of the first returned key
 * @param key on success contains an array of the requested keys
 * @return UA_STATUSCODE_GOOD on success, UA_STATUSCODE_BADNOTFOUND if no key according to
 * startingTokenId was found
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
UA_getSecurityKeysBatch(const UA_UInt32 startingTokenId, UA_UInt32 requestedKeyCount,
                        const UA_PubSubSKSKeyStorage *keyStorage,
                        UA_Variant *firstTokenId, UA_Variant *key) {

    if(!keyStorage || !firstTokenId|| !key)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_UInt32 firstTokenIdValue;
    UA_PubSubKeyListItem *tokenIndexIterator = NULL;
    /* ensure tokenIndexIterator is set */
    UA_Boolean foundStart = UA_FALSE;

    /*Set the startingToken,point TokenIndex to firstToken*/
    if(startingTokenId == 0) {
        firstTokenIdValue = keyStorage->currentItem->keyID;
        tokenIndexIterator = keyStorage->currentItem;
        foundStart = UA_TRUE;
    } else {
        /*Use this flag to skip old key placeholder*/
        UA_Boolean firstTokenInitialized = UA_FALSE;

        /*Search for the firstKey*/
        for(UA_PubSubKeyListItem *item = keyStorage->firstItem; item != NULL;
            item = item->next) {
            /*initialize firstTokenId to the oldest key, if startingTokenId not found
            startingToken, return the very old one */
            if(item->keyID != UA_PUBSUB_SKS_KEYLIST_EMPTY_SLOT_ID && (!firstTokenInitialized)) {
                firstTokenIdValue = item->keyID;
                tokenIndexIterator = item;
                firstTokenInitialized = UA_TRUE;
                foundStart = UA_TRUE;
            }

            /*Requested startingTokenId found*/
            if(startingTokenId == item->keyID) {
                firstTokenIdValue = item->keyID;
                tokenIndexIterator = item;
                foundStart = UA_TRUE;
                break;
            }
        }
    }

    if(!foundStart)
        return UA_STATUSCODE_BADNOTFOUND;

    /*Prepare output*/

    /*  from spec: requesting 0 means requesting the current key only
        therefore, 0 is the same as requesting 1 */
    if(requestedKeyCount == 0)
        requestedKeyCount = 1;

    /*Set outputSize to the maxium that can be provided*/
    UA_UInt32 maximalReturnedKeyCount =
        keyStorage->maxFutureKeyNumber + keyStorage->maxPastKeyNumber + 1;

    /*Set output buffer*/
    maximalReturnedKeyCount = UA_MIN(maximalReturnedKeyCount, requestedKeyCount);

    /*Assume can return what requested*/
    UA_ByteString *keyOutput = keyStorage->getSecurityKeysKeyBuffer;

    /*Write to output*/
    UA_UInt32 outputSize = 0;
    for(outputSize = 0; outputSize < maximalReturnedKeyCount; outputSize++) {
        keyOutput[outputSize].data = tokenIndexIterator->key.data;
        keyOutput[outputSize].length = tokenIndexIterator->key.length;

        /*Reach the end of keyList -> no more future keys available*/
        if(!tokenIndexIterator->next) {
            outputSize++;
            break;
        }
        tokenIndexIterator = tokenIndexIterator->next;
    }

    UA_Variant_setArray(key, keyOutput, (size_t)outputSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* prevent that reusable buffers are deleted by the stack */
    key->storageType = UA_VARIANT_DATA_NODELETE;

    UA_Variant_setScalarCopy(firstTokenId, &firstTokenIdValue, &UA_TYPES[UA_TYPES_UINT32]);

    return UA_STATUSCODE_GOOD;
}

/**
 * Insert one newKey into keyList.
 * Since the length of keyList is constant, this means that an old key is removed.
 * Since elements inside keyList can be used as a linked list, the oldest element is
 * removed from the linked list (but not from the array) and then added to the end of the
 * linked list to avoid allocations. The functions returns a memory location for storing a
 * new key
 * @param keyStorage: Pointer to the keyStorage structure to which a new key should be
 * added
 * @param newKey: Memory location to store a new key
 * @return UA_STATUSCODE_GOOD on success,
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
UA_insertKeyIntoKeyList(UA_PubSubSKSKeyStorage *keyStorage, UA_ByteString **newKey) {

    if(!keyStorage || !newKey)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(keyStorage->keyListSize == 1) {
        if(keyStorage->currentItem->keyID == UA_UINT32_MAX)
            keyStorage->currentItem->keyID = 1;
        else
            ++keyStorage->currentItem->keyID;
        *newKey = &keyStorage->currentItem->key;

        return UA_STATUSCODE_GOOD;
    }

    UA_PubSubKeyListItem *oldest = keyStorage->firstItem;
    UA_PubSubKeyListItem *newest = keyStorage->lastItem;

    keyStorage->firstItem = keyStorage->firstItem->next;
    keyStorage->currentItem = keyStorage->currentItem->next;
    keyStorage->lastItem = oldest;

    newest->next = oldest;
    oldest->next = NULL;

    *newKey = &oldest->key;

    if(newest->keyID == UA_UINT32_MAX)
        oldest->keyID = 1;
    else
        oldest->keyID = newest->keyID + 1;

    return UA_STATUSCODE_GOOD;
}

/** A callback function, generate one key and add it into the keyStorage
 * @param server server to which this callback is attached
 * @param keyStorage keyStorage to use for update
 */
static void
UA_updateKeyStorage(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_ByteString *newKey;

    if(!server || !keyStorage)
        ret = UA_STATUSCODE_BADINVALIDARGUMENT;

    /*Prepare New Key*/
    if(ret == UA_STATUSCODE_GOOD) {
        ret = UA_insertKeyIntoKeyList(keyStorage, &newKey);
    }

    if(ret == UA_STATUSCODE_GOOD) {
        ret = UA_generateKey(keyStorage->policy, newKey);
    }

    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                     (int)keyStorage->securityGroupID.length,
                     keyStorage->securityGroupID.data, UA_StatusCode_name(ret));
    }
}

/**
 * Checks if an object has a reference with the specified type id and target name.
 * If so: Returns UA_STATUSCODE_GOOD. If referenceTarget != NULL, referenceTarget contains
 * the node id of the target. If not: Returns a status code != UA_STATUSCODE_GOOD.
 * @param server server instance
 * @param object the object who would be the owner of the reference
 * @param referenceType type of the reference to look for
 * @param referenceName name of the reference to loook for
 * @param referenceTarget if not NULL and on success contains the referenced target
 * @param inverse true if an inverse lookup shall be performed
 * @return UA_STATUSCODE_GOOD on success, UA_STATUSCODE_BADTOOMANYMATCHES if more than one
 * reference Target was found, error codes of UA_Server_translateBrowsePathToNodeIds
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
followReference(UA_Server *server, const UA_NodeId *object, UA_UInt32 referenceType,
                const UA_QualifiedName *referenceName, UA_NodeId *referenceTarget,
                bool inverse) {

    if(!server || !object || !referenceName)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_RelativePathElement relativePath;
    UA_BrowsePath browsePath;
    UA_BrowsePathResult browseResult;

    UA_RelativePathElement_init(&relativePath);
    UA_BrowsePath_init(&browsePath);
    UA_BrowsePathResult_init(&browseResult);

    relativePath.referenceTypeId = UA_NODEID_NUMERIC(0, referenceType);
    relativePath.isInverse = inverse;
    relativePath.includeSubtypes = UA_FALSE;
    relativePath.targetName = *referenceName;

    browsePath.startingNode = *object;
    browsePath.relativePath.elementsSize = 1;
    browsePath.relativePath.elements = &relativePath;

    browseResult = UA_Server_translateBrowsePathToNodeIds(server, &browsePath);
    if(browseResult.statusCode != UA_STATUSCODE_GOOD) {
        ret = browseResult.statusCode;
        goto cleanup;
    }

    if(referenceTarget != NULL) {
        if(browseResult.targetsSize != 1) {
            ret = UA_STATUSCODE_BADTOOMANYMATCHES;
            goto cleanup;
        }

        *referenceTarget = browseResult.targets[0].targetId.nodeId;
    }

cleanup:

    UA_BrowsePathResult_deleteMembers(&browseResult);

    return ret;
}

/**
 * Puts the value of the member with name memberName of object into value
 * @param server server instance
 * @param object object containing the member
 * @param memberName: Name of the member. This is a char* instead of a const char* to
 * avoid using the UA_QUALIFIEDNAME_ALLOC function
 * @param value on success contains the value of the requested member
 * @return UA_STATUSCODE_GOOD on success, error codes of
 * UA_Server_translateBrowsePathToNodeIds and UA_Server_readValue
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
getMember(UA_Server *server, const UA_NodeId *object, char *memberName,
          UA_Variant *value) {

    if(!server || !object || !memberName || !value)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_RelativePathElement relativePath;
    UA_BrowsePath browsePath;
    UA_BrowsePathResult browseResult;

    UA_BrowsePathResult_init(&browseResult);
    UA_RelativePathElement_init(&relativePath);
    UA_BrowsePath_init(&browsePath);

    relativePath.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    relativePath.isInverse = UA_FALSE;
    relativePath.includeSubtypes = UA_FALSE;
    relativePath.targetName = UA_QUALIFIEDNAME(0, memberName);

    browsePath.startingNode = *object;
    browsePath.relativePath.elementsSize = 1;
    browsePath.relativePath.elements = &relativePath;

    browseResult = UA_Server_translateBrowsePathToNodeIds(server, &browsePath);

    if(browseResult.statusCode != UA_STATUSCODE_GOOD || browseResult.targetsSize < 1) {
        ret = browseResult.statusCode;
        goto cleanup;
    }

    ret = UA_Server_readValue(server, browseResult.targets[0].targetId.nodeId, value);

cleanup:

    UA_BrowsePathResult_deleteMembers(&browseResult);

    return ret;
}

/**
 * Sets the member with name memberName of object to the specified memberValue of the
 * specified type
 * @param server server instance
 * @param object object containing the member
 * @param memberName: Name of the member. This is a char* instead of a const char* to
 * avoid using the UA_QUALIFIEDNAME_ALLOC function
 * @param memberValue new value
 * @param type type of the member
 * @return UA_STATUSCODE_GOOD on success, error codes of
 * UA_Server_translateBrowsePathToNodeIds and UA_Server_writeValue
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
setMember(UA_Server *server, const UA_NodeId *object, char *memberName, void *memberValue,
          int type) {

    if(!server || !object || !memberName || !memberValue)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret;
    UA_RelativePathElement relativePath;
    UA_BrowsePathResult browseResult;
    UA_BrowsePath browsePath;

    UA_BrowsePathResult_init(&browseResult);
    UA_RelativePathElement_init(&relativePath);
    UA_BrowsePath_init(&browsePath);

    relativePath.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    relativePath.isInverse = UA_FALSE;
    relativePath.includeSubtypes = UA_FALSE;
    relativePath.targetName = UA_QUALIFIEDNAME(0, memberName);

    browsePath.startingNode = *object;
    browsePath.relativePath.elementsSize = 1;
    browsePath.relativePath.elements = &relativePath;

    browseResult = UA_Server_translateBrowsePathToNodeIds(server, &browsePath);

    if(browseResult.statusCode != UA_STATUSCODE_GOOD || browseResult.targetsSize < 1) {
        ret = browseResult.statusCode;
        goto cleanup;
    }

    UA_Variant value;
    UA_Variant_setScalar(&value, memberValue, &UA_TYPES[type]);
    ret = UA_Server_writeValue(server, browseResult.targets[0].targetId.nodeId, value);

cleanup:

    UA_BrowsePathResult_deleteMembers(&browseResult);

    return ret;
}

/**
 * Gets the node id related to a securityGroupId by recursively searching for a security
 * group with the provided id. On success: Puts its node id into securityGroupNodeId and
 * @param server server instance
 * @param securityGroupId group id
 * @param securityGroupNodeId on success contains the node id
 * @return UA_STATUSCODE_GOOD on success, UA_STATUSCODE_BADNODEIDUNKNOWN if securityGroup
 * was not found Error codes of UA_Server_browseRecursive.
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
static UA_StatusCode
getSecurityGroup(UA_Server *server, const UA_String *securityGroupId,
                 UA_NodeId *securityGroupNodeId) {

    if(!server || !securityGroupId || !securityGroupNodeId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_BrowseDescription b;
    b.includeSubtypes = UA_FALSE;
    b.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    b.nodeId = NODEID_SKS_SecurityRootFolder;
    b.browseDirection = UA_BROWSEDIRECTION_FORWARD;

    size_t resultSize = 0;
    UA_ExpandedNodeId *results = NULL;

    UA_StatusCode ret = UA_Server_browseRecursive(server, &b, &resultSize, &results);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    if(resultSize == 0)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    bool securityGroupNodeIdFound = false;

    for(size_t i = 0; i < resultSize; ++i) {
        UA_ExpandedNodeId *browsedNodeId = &results[i];

        UA_QualifiedName n = UA_QUALIFIEDNAME(0, "SecurityGroupType");
        UA_StatusCode followReferenceResult =
            followReference(server, &browsedNodeId->nodeId, UA_NS0ID_HASTYPEDEFINITION,
                            &n, NULL, UA_FALSE);

        if(followReferenceResult == UA_STATUSCODE_GOOD) {
            UA_Variant securityGroupIdVariant;

            if(getMember(server, &browsedNodeId->nodeId, "SecurityGroupId",
                         &securityGroupIdVariant) == UA_STATUSCODE_GOOD &&
               securityGroupIdVariant.type->typeIndex == UA_TYPES_STRING) {

                if(UA_String_equal(securityGroupId,
                                   (UA_String *)securityGroupIdVariant.data)) {

                    *securityGroupNodeId = browsedNodeId->nodeId;
                    securityGroupNodeIdFound = true;
                    UA_Variant_deleteMembers(&securityGroupIdVariant);
                    break;
                }

                UA_Variant_deleteMembers(&securityGroupIdVariant);
            }
        }
    }

    if(results != NULL)
        UA_ExpandedNodeId_delete(results);

    if(securityGroupNodeIdFound)
        return UA_STATUSCODE_GOOD;

    return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

/**
 * Implementation of the GetSecurityKeys service.
 * See UA_Server_setMethodNode_callback for parameter documentation.
 * See OPC Unified Architecture, Part 14, chapter 8 for method documentation
 * [in] String SecurityGroupId
 * [in] UInt32 StartingTokenId
 * [in] UInt32 RequestedKeyCount
 * [out] String SecurityPolicyUri
 * [out] IntegerId FirstTokenId
 * [out] ByteString[] Keys
 * [out] Duration TimeToNextKey
 * [out] Duration KeyLifetime
 */
static UA_StatusCode
getSecurityKeysAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {

    if(!server || !input || !output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /*Check inputs*/
    if(inputSize < 3 || outputSize < 5)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 3 || outputSize > 5)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    if(input[0].type->typeIndex != UA_TYPES_STRING ||
       input[1].type->typeIndex != UA_TYPES_UINT32 ||
       input[2].type->typeIndex != UA_TYPES_UINT32) {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    for (size_t i = 0; i < outputSize; ++i)
        UA_Variant_init(&output[i]);

    /*Get current session, check whether it is encrypted */
    UA_tempDebugger(server, UA_CUSTOM_SHOW_KEYSTORAGE_LOGLEVEL);
    session_list_entry *session_entry;
    LIST_FOREACH(session_entry, &server->sessionManager.sessions, pointers) {
        if(UA_NodeId_equal(&session_entry->session.sessionId, sessionId)) {
            if(session_entry->session.header.channel->securityMode !=
               UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
                return UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        }
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    /*prepare input */
    UA_String *securityGroupId = (UA_String *)input[0].data;
    UA_UInt32 startingTokenId = *(UA_UInt32 *)input[1].data;
    UA_UInt32 requestedKeycount = *(UA_UInt32 *)input[2].data;

    /* look for the security group object in the server. this contains all necessary
     information to create / return the requested keys */
    UA_NodeId securityGroupNodeId;
    ret = getSecurityGroup(server, securityGroupId, &securityGroupNodeId);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Can't find security group");
        goto error;
    }

    /*Get securityPolicyUri and keylifetime from securityGroup*/
    ret = getMember(server, &securityGroupNodeId, "SecurityPolicyUri", &output[0]);
    if(ret != UA_STATUSCODE_GOOD)
        goto error;

    UA_Variant temp;
    ret = getMember(server, &securityGroupNodeId, "KeyLifetime", &temp);
    if(ret != UA_STATUSCODE_GOOD)
        goto error;
    UA_Duration keyLifetime = *(UA_Duration *)temp.data;
    UA_Variant_deleteMembers(&temp);

    UA_PubSubSKSKeyStorage *keyStorage;
    ret = UA_getPubSubKeyStoragebySecurityGroupId(server, securityGroupId, &keyStorage);

    if(ret != UA_STATUSCODE_GOOD)
        goto error;

    /*Get securityKeys*/
    ret = UA_getSecurityKeysBatch(startingTokenId, requestedKeycount, keyStorage,
                                  &output[1], &output[2]);
    if(ret != UA_STATUSCODE_GOOD)
        goto error;

    /*KeyGenerationTimeRecord*/

    /* keyLifetime is in miliseconds unit -> need to convert to
     * 100nanoseconds(UA_DateTime) = *10000 */
    /*1milisecond = 10^6nanoseconds=100nanoseconds*10000*/

    /*This time indicates how long does the keyStorage is activated*/
    UA_DateTime timeAfterInitialization = UA_DateTime_now() - keyStorage->keyGenerateBase;
    /* convert keylifetime to UA_DateTime*/
    UA_DateTime keyLifetimeUint64 = (UA_DateTime)keyLifetime * UA_DATETIME_MSEC;

    /*For better understanding, an example here:
    saying the keyStorage is initailized at time 0 with KeyLifeTime=10
    and when a publisher/subscriber comes at time 12, the timeToNextKey should be 8
    cause SKS server will move to next key at time 20*/
    UA_Duration timeToNextKey =
        (UA_Duration)(keyLifetimeUint64 - timeAfterInitialization % keyLifetimeUint64) /
        UA_DATETIME_MSEC;
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                 "Found SecurityGroup in history,This key list is alive for :%ld "
                 "nano seconds,Time to next key is %f milli seconds",
                 timeAfterInitialization, timeToNextKey);

    ret = UA_Variant_setScalarCopy(&output[3], &timeToNextKey,
                                   &UA_TYPES[UA_TYPES_DURATION]);

    /*keyLifeTime is read from SecurityGroup  */
    if(ret == UA_STATUSCODE_GOOD)
        ret = UA_Variant_setScalarCopy(&output[4], &keyLifetime,
                                       &UA_TYPES[UA_TYPES_DURATION]);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "getSecurityKeysAction");

    if (ret != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:

    for (size_t i = 0; i < outputSize; ++i){
        UA_Variant_deleteMembers(&output[i]);
    }

    return ret;
}

/**
 * Implementation of the GetSecurityGroup service
 * See UA_Server_setMethodNode_callback for parameter documentation.
 * See OPC Unified Architecture, Part 14, chapter 8 for method documentation
 * [in] String SecurityGroupId
 * [out] NodeId SecurityGroupNodeId
 */
static UA_StatusCode
getSecurityGroupAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                       const UA_Variant *input, size_t outputSize, UA_Variant *output) {

    if(!server || !input || !output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "getSecurityGroupAction");

    if(inputSize < 1 || outputSize < 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 1 || outputSize > 1)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;
    if(input[0].type->typeIndex != UA_TYPES_STRING)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId securityGroupNodeId;

    retval = getSecurityGroup(server, (UA_String *)input[0].data, &securityGroupNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Variant_setScalarCopy(&output[0], &securityGroupNodeId,
                                      &UA_TYPES[UA_TYPES_NODEID]);

    return retval;
}

/**
 * Implementation of the AddSecurityGroup service
 * See UA_Server_setMethodNode_callback for parameter documentation.
 * See OPC Unified Architecture, Part 14, chapter 8 for method documentation
 * [in] String SecurityGroupName
 * [in] Duration KeyLifetime
 * [in] String SecurityPolicyUri
 * [in] UInt32 MaxFutureKeyCount
 * [in] UInt32 MaxPastKeyCount
 * [out] String SecurityGroupId
 * [out] NodeId SecurityGroupNodeId
 */
static UA_StatusCode
addSecurityGroupAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionHandle,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                       const UA_Variant *input, size_t outputSize, UA_Variant *output) {

    if(!server || !input || !output || !objectId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(inputSize < 5 || outputSize < 2)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 5 || outputSize > 2)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    UA_StatusCode ret;
    UA_String *groupName;

    if(input[0].type->typeIndex != UA_TYPES_STRING ||
       /* UAExpert sends the keylifetime as a double instead of a duration */
       (input[1].type->typeIndex != UA_TYPES_DURATION &&
        input[1].type->typeIndex != UA_TYPES_DOUBLE) ||
       input[2].type->typeIndex != UA_TYPES_STRING ||
       input[3].type->typeIndex != UA_TYPES_UINT32 ||
       input[4].type->typeIndex != UA_TYPES_UINT32) {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_String *securityPolicyUri = (UA_String *)input[2].data;
    UA_SecurityPolicy *policy;
    ret = UA_SecurityPolicy_findPolicyBySecurityPolicyUri(server, *securityPolicyUri, &policy);

    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    groupName = (UA_String *)input[0].data;
    if(groupName == NULL || groupName->length == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(input[1].type->typeIndex == UA_TYPES_DOUBLE && *(UA_Double *)input[1].data < 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_NodeId existing;
    if(getSecurityGroup(server, groupName, &existing) == UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNODEIDEXISTS;

    UA_QualifiedName qualifiedGroupName = UA_QUALIFIEDNAME(0, "");
    qualifiedGroupName.name = *groupName;

    UA_NodeId outId;
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;

    ret = UA_Server_addObjectNode(
        server, UA_NODEID_NULL, *objectId, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        qualifiedGroupName, NODEID_SKS_SecurityGroupType, object_attr, NULL, &outId);

    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = setMember(server, &outId, "SecurityGroupId", groupName, UA_TYPES_STRING);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = setMember(server, &outId, "KeyLifetime", input[1].data, UA_TYPES_DURATION);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = setMember(server, &outId, "SecurityPolicyUri", securityPolicyUri,
                    UA_TYPES_STRING);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = setMember(server, &outId, "MaxFutureKeyCount", input[3].data, UA_TYPES_UINT32);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = setMember(server, &outId, "MaxPastKeyCount", input[4].data, UA_TYPES_UINT32);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = UA_Variant_setScalarCopy(&output[0], groupName, &UA_TYPES[UA_TYPES_STRING]);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = UA_Variant_setScalarCopy(&output[1], &outId, &UA_TYPES[UA_TYPES_NODEID]);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    UA_UInt32 maxFutureKeyCount = *(UA_UInt32 *)input[3].data;
    UA_UInt32 maxPastKeyCount = *(UA_UInt32 *)input[4].data;

    UA_PubSubSKSKeyStorage *keyStorage;

    ret = UA_PubSubSKSKeyStorage_new(&keyStorage);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    ret = UA_initKeyStorage(keyStorage, policy, maxPastKeyCount, maxFutureKeyCount,
                            groupName);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;

    /*There could be 2 possibilities to set the keyGenerateBase,here I choose the first
    one 1:When the securityGroup is added and keyStorage is initialized 2:When the first
    getSecurity comes(That means the key is really in use)
    */
    UA_Duration keyLifeTime = *(UA_Duration *)input[1].data;
    keyStorage->keyGenerateBase = UA_DateTime_now();

    LIST_INSERT_HEAD(&server->pubSubSKSKeyList, keyStorage, keyStorageList);

    ret = UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_updateKeyStorage,
                                        keyStorage, keyLifeTime, &keyStorage->callBackId);

    return ret;
}

/**
 * Implementation of the RemoveSecurityGroup service
 * See UA_Server_setMethodNode_callback for parameter documentation.
 * See OPC Unified Architecture, Part 14, chapter 8 for method documentation
 * [in] NodeId SecurityGroupNodeId
 */
static UA_StatusCode
removeSecurityGroupAction(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionHandle, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {

    if(!server || !input || !objectId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(inputSize < 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 1 || outputSize > 0)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    if(input[0].type->typeIndex != UA_TYPES_NODEID)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId *securityGroupNodeId = (UA_NodeId *)input[0].data;

    /* only allow deleting actual security groups */
    UA_QualifiedName n = UA_QUALIFIEDNAME(0, "SecurityGroupType");
    retval = followReference(server, securityGroupNodeId, UA_NS0ID_HASTYPEDEFINITION, &n,
                             NULL, UA_FALSE);
    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNODEIDINVALID;

    /* only allow deleting security groups which are 1) in a security group folder and 2)
     */
    /* in the same folder as this method 2) includes 1) */
    UA_QualifiedName bn;
    UA_NodeId parent;
    retval = UA_Server_readBrowseName(server, *objectId, &bn);
    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    retval = followReference(server, securityGroupNodeId, UA_NS0ID_ORGANIZES, &bn,
                             &parent, UA_TRUE);
    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    if(!UA_NodeId_equal(objectId, &parent))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    /* Delete keyStorage for this securityGroup */
    /* First Get SecurityGroupID */
    UA_Variant securityGroupIdVar;
    retval =
        getMember(server, securityGroupNodeId, "SecurityGroupId", &securityGroupIdVar);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_String *securityGroupId = (UA_String *)securityGroupIdVar.data;

    /* Then remove callback and keyStorage */
    UA_PubSubKeySKSStorage_deleteSingle(server, securityGroupId);
    retval = UA_Server_deleteNode(server, *securityGroupNodeId, UA_TRUE);

    return retval;
}

/* see header for documentation */
UA_StatusCode UA_EXPORT
UA_Server_addSKS(UA_Server *server) {

    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    retval = UA_Server_setMethodNode_callback(server, NODEID_SKS_GetSecurityKeys,
                                              getSecurityKeysAction);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Server_setMethodNode_callback(server, NODEID_SKS_GetSecurityGroup,
                                              getSecurityGroupAction);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Server_setMethodNode_callback(server, NODEID_SKS_AddSecurityGroup,
                                              addSecurityGroupAction);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Server_setMethodNode_callback(server, NODEID_SKS_RemoveSecurityGroup,
                                              removeSecurityGroupAction);

    return retval;
}

#endif
