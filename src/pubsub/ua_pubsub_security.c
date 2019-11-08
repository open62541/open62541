#include "ua_pubsub_sks.h"

#ifdef UA_ENABLE_PUBSUB_SECURITY

#include <open62541/server_pubsub.h>
#include <open62541/client.h>
#include <open62541/server_sks.h>
#include <open62541/client_highlevel.h>

#include "server/ua_server_internal.h"

#include "ua_pubsub.h"

#include "ua_pubsub_ns0.h"

#include <open62541/types_generated_encoding_binary.h>

UA_StatusCode
UA_importKeyToChannelContext(const UA_ByteString key, UA_PubSub_SKSConfig *sksConfig) {

    if(!sksConfig || !sksConfig->keyStorage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_SecurityPolicy *policy = sksConfig->keyStorage->policy;

    /*Check the main key length is the same according to policy*/
    if(key.length != policy->symmetricModule.secureChannelNonceLength)
        return UA_STATUSCODE_BADINTERNALERROR;

    /*Get Key Length according to policy*/
    size_t signingkeyLength =
        policy->symmetricModule.cryptoModule.signatureAlgorithm.getLocalKeyLength(
            policy, sksConfig->channelContext);
    size_t encryptkeyLength =
        policy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalKeyLength(
            policy, sksConfig->channelContext);
    /*Rest of the part is the keyNonce*/
    size_t keyNonceLength = key.length - signingkeyLength - encryptkeyLength;

    /*DivideKeys in origin ByteString*/
    UA_ByteString signingKey = {signingkeyLength, key.data};
    UA_ByteString encryptKey = {encryptkeyLength, key.data + signingkeyLength};
    UA_ByteString keyNonce = {keyNonceLength,
                              key.data + signingkeyLength + encryptkeyLength};

    /*Import them into channelContext and sksconfig*/
    retVal = policy->channelModule.setLocalSymSigningKey(sksConfig->channelContext,
                                                         &signingKey);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = policy->channelModule.setLocalSymEncryptingKey(sksConfig->channelContext,
                                                            &encryptKey);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    /* keyNonce is allocated in UA_initializeKeyStorageByAskingCurrentKey */
    if(sksConfig->keyNonce.length != keyNonce.length) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    memcpy(sksConfig->keyNonce.data, keyNonce.data, sksConfig->keyNonce.length);

    /*No need to cleanup, all pointed to original key*/
    return retVal;
}

UA_StatusCode
UA_PubSub_getKeyByKeyID(const UA_UInt32 keyId, UA_PubSubKeyListItem *firstItem,
                        UA_PubSubKeyListItem **keyList) {

    if(!firstItem || !keyList)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubKeyListItem *keyListIterator = firstItem;
    while(keyListIterator != NULL) {
        if(keyListIterator->keyID == keyId) {
            *keyList = keyListIterator;
            return UA_STATUSCODE_GOOD;
        }
        keyListIterator = keyListIterator->next;
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_PubSub_updateKeyStorage(UA_PubSubSKSKeyStorage *keyStorage,
                           const UA_UInt32 startingKeyID, const UA_ByteString *keys,
                           const size_t keySize, UA_UInt32 currentKeyID) {

    if(!keyStorage || !keys)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PubSubKeyListItem *keyListIterator;

    /*First update CurrentKey*/
    if(currentKeyID != 0) {

        /*Try look for it in local storage*/
        UA_StatusCode searchResult = UA_PubSub_getKeyByKeyID(
            currentKeyID, keyStorage->firstItem, &keyListIterator);

        if(searchResult == UA_STATUSCODE_GOOD) {

            /*If it already exists*/
            keyStorage->currentItem = keyListIterator;

        } else if(searchResult == UA_STATUSCODE_BADNOTFOUND) {

            /*If want to update currentkey, it should be the first in the keyList*/
            if(currentKeyID != startingKeyID)
                return UA_STATUSCODE_BADINTERNALERROR;

            /*If not found , use the space of the first(oldest) one */
            keyListIterator = keyStorage->firstItem;
            keyListIterator->keyID = currentKeyID;

            /* keys are allocated in UA_initializeKeyStorageByAskingCurrentKey */
            if(keys[0].length != keyListIterator->key.length) {
                return UA_STATUSCODE_BADINVALIDARGUMENT;
            }

            memcpy(keyListIterator->key.data, keys[0].data, keys[0].length);

            /*If size of KeyStorage is more than 1*/
            if(keyStorage->firstItem != keyStorage->lastItem) {
                /*Move it from head to tail*/
                keyStorage->firstItem = keyListIterator->next;
                keyStorage->lastItem->next = keyListIterator;
                keyStorage->lastItem = keyListIterator;
                keyListIterator->next = NULL;
            }
            keyStorage->currentItem = keyListIterator;

        } else {
            return searchResult;
        }

    } else {

        UA_UInt32 startingKeyIDCopy = startingKeyID;

        for(size_t i = 0; i < keySize; ++i) {

            keyListIterator = keyStorage->firstItem;

            /*Can reuse old keys before currentItem, this state also checks if the list
             * length is 1*/
            if(keyListIterator == keyStorage->currentItem)
                break;

            /*Copy key to it*/
            keyListIterator->keyID = startingKeyIDCopy;

            memcpy(keyListIterator->key.data, keys[i].data, keys[i].length);

            /*Move it from head to tail*/
            keyStorage->firstItem = keyListIterator->next;
            keyStorage->lastItem->next = keyListIterator;
            keyStorage->lastItem = keyListIterator;
            keyListIterator->next = NULL;

            /*Update keyID and move keyList Iterator*/
            if(startingKeyIDCopy == UA_UINT32_MAX)
                startingKeyIDCopy = 1;
            else
                ++startingKeyIDCopy;
        }
    }

    return retVal;
}

void
UA_moveToNextKeyCallbackPull(UA_Server *server, UA_PubSub_SKSConfig *sksConfig) {

    UA_StatusCode retVal;
    UA_Variant *getSecurityKeysOutput = NULL;
    UA_ByteString *newKey = NULL;
    UA_Duration timeToNextKey;

    if(!server || !sksConfig) {
        retVal = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto cleanup;
    }

    UA_PubSubSKSKeyStorage *keyStorage = sksConfig->keyStorage;

    /*Get expecting keyID*/
    UA_UInt32 newKeyID = keyStorage->lastItem->keyID;
    if(newKeyID == UINT32_MAX)
        newKeyID = 1;
    else
        ++newKeyID;

    /*If currentItem is not the lastItem, this statement also check whether the list only
     * contains one key*/
    if(keyStorage->currentItem != keyStorage->lastItem) {

        retVal =
            addMoveToNextKeyCallback(server, (UA_ServerCallback)UA_moveToNextKeyCallbackPull, sksConfig,
                                     keyStorage->keyLifeTime, &keyStorage->callBackId);
        if(retVal != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        keyStorage->currentItem = keyStorage->currentItem->next;

    } else {

        /*if already reaches lastItem ,ask SKS for newKeys*/
        retVal = UA_getSecurityKeys(sksConfig->client, keyStorage->securityGroupID,
                                    newKeyID, (UA_UInt32)keyStorage->keyListSize,
                                    &getSecurityKeysOutput);
        if(retVal != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        /*FirstTokenId of the output*/
        UA_UInt32 firstTokenId = *(UA_UInt32 *)getSecurityKeysOutput[1].data;

        /*received keyNumber*/
        size_t keyNumber = getSecurityKeysOutput[2].arrayLength;

        /*check if the received key is as expected*/
        if(keyNumber < 1 || (firstTokenId != newKeyID)) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "KeyUpdateFail");
            goto cleanup;
        }

        /*Get new key and Insert into queue*/
        newKey = (UA_ByteString *)getSecurityKeysOutput[2].data;

        /*Update currentKey*/
        retVal =
            UA_PubSub_updateKeyStorage(keyStorage, firstTokenId, newKey, 1, firstTokenId);
        if(retVal != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        /*Then try to update other keys*/
        if(keyNumber > 1) {
            if(firstTokenId == UA_UINT32_MAX)
                firstTokenId = 1;
            else
                ++firstTokenId;

            retVal = UA_PubSub_updateKeyStorage(keyStorage, firstTokenId, newKey + 1,
                                                keyNumber - 1, 0);
            if(retVal != UA_STATUSCODE_GOOD) {
                goto cleanup;
            }
        }

        /*Get timeToNext Key*/
        timeToNextKey = *(UA_Duration *)getSecurityKeysOutput[3].data;

        retVal = addMoveToNextKeyCallback(server, (UA_ServerCallback)UA_moveToNextKeyCallbackPull, sksConfig,
                                          timeToNextKey, &keyStorage->callBackId);
        if(retVal != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }
    }

    /* key storage is the same for all reader / writer groups, channel context isn't
     * => Update channelcontext in all Writergroups / ReaderGroups which have the same
     * securityGroup*/
    UA_PubSubConnection *pubSubConnections = server->pubSubManager.connections;
    for(size_t i = 0; i < server->pubSubManager.connectionsSize; i++) {

        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &pubSubConnections[i].writerGroups, listEntry) {

            /* If pointing to the same keyStorage*/
            if(tmpWriterGroup->config.sksConfig.keyStorage == keyStorage) {

                retVal = UA_importKeyToChannelContext(
                    tmpWriterGroup->config.sksConfig.keyStorage->currentItem->key,
                    &tmpWriterGroup->config.sksConfig);

                if(retVal != UA_STATUSCODE_GOOD)
                    goto cleanup;
            }
        }

        UA_ReaderGroup *tmpReaderGroup;
        LIST_FOREACH(tmpReaderGroup, &pubSubConnections[i].readerGroups, listEntry) {

            /* If pointing to the same keyStorage*/
            if(tmpReaderGroup->config.sksConfig.keyStorage == keyStorage) {
                retVal = UA_importKeyToChannelContext(
                    tmpReaderGroup->config.sksConfig.keyStorage->currentItem->key,
                    &tmpReaderGroup->config.sksConfig);

                if(retVal != UA_STATUSCODE_GOOD)
                    goto cleanup;
                tmpReaderGroup->config.keyIdInChannelContext =
                    keyStorage->currentItem->keyID;
            }
        }
    }

cleanup:

    if(getSecurityKeysOutput) {
        UA_Array_delete(getSecurityKeysOutput, UA_GETSECURITYKEYS_OUTPUT_SIZE,
                        &UA_TYPES[UA_TYPES_VARIANT]);
    }

    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                     (int)keyStorage->securityGroupID.length,
                     keyStorage->securityGroupID.data, UA_StatusCode_name(retVal));
    }
}

void
UA_moveToNextKeyCallbackPush(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage) {
    if(keyStorage->currentItem != keyStorage->lastItem) {

        UA_StatusCode retVal = addMoveToNextKeyCallback(
            server, (UA_ServerCallback)UA_moveToNextKeyCallbackPush, keyStorage,
            keyStorage->keyLifeTime, &keyStorage->callBackId);

        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(
                &server->config.logger, UA_LOGCATEGORY_SERVER,
                "Failed to update keys for security group id '%.*s'. Reason: '%s'.",
                (int)keyStorage->securityGroupID.length, keyStorage->securityGroupID.data,
                UA_StatusCode_name(retVal));
        }

        keyStorage->currentItem = keyStorage->currentItem->next;
    }
}

/* see forward declaration for documentation */
UA_StatusCode
addMoveToNextKeyCallback(UA_Server *server, UA_ServerCallback callBackFuntion,void* functionParameter,
                         UA_Double timeInMS, UA_UInt64* callBackId) {

    if(!server || !callBackFuntion || !callBackId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* got time in ms duration, need absolute time in UA_DateTime (100nanoseconds) */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime dateTimeToNextKey =
        now + (UA_DateTime)(UA_DATETIME_MSEC * timeInMS);

    return UA_Server_addTimedCallback(server, callBackFuntion,
                                      functionParameter, dateTimeToNextKey,
                                      callBackId);
}

UA_StatusCode
UA_PubSub_subscriberUpdateKeybyKeyID(const UA_UInt32 keyId,
                                     UA_PubSub_SKSConfig *sksConfig) {

    if(!sksConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;
    UA_Variant *getSecurityKeysOutput = NULL;

    /*See if it exists*/
    UA_PubSubKeyListItem *pubSubKey;
    UA_StatusCode searchResult = UA_PubSub_getKeyByKeyID(keyId, sksConfig->keyStorage->firstItem,
                                            &pubSubKey);

    if(searchResult == UA_STATUSCODE_GOOD) {

        /*If found*/
        retVal = UA_importKeyToChannelContext(pubSubKey->key, sksConfig);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;

    } else if(searchResult == UA_STATUSCODE_BADNOTFOUND) {

        /*If not, ask SKS for it*/
        UA_ByteString *keys;

        /*Current implementation: only support ask for one key*/
        retVal =
            UA_getSecurityKeys(sksConfig->client, sksConfig->keyStorage->securityGroupID,
                               keyId, 1, &getSecurityKeysOutput);
        if(retVal != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        /*check whether it is the expected key*/
        UA_UInt32 firstTokenId = *(UA_UInt32 *)getSecurityKeysOutput[1].data;
        if(firstTokenId != keyId) {
            retVal = UA_STATUSCODE_BADNOMATCH;
            goto cleanup;
        }

        /* get keys*/
        size_t keyNumber = getSecurityKeysOutput[2].arrayLength;
        if(keyNumber == 0) {
            retVal = UA_STATUSCODE_BADNODATA;
            goto cleanup;
        }

        keys = (UA_ByteString *)getSecurityKeysOutput[2].data;

        retVal = UA_importKeyToChannelContext(keys[0], sksConfig);
        if(retVal != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        retVal = UA_PubSub_updateKeyStorage(sksConfig->keyStorage, firstTokenId, keys, 1, 0);
    }
    else{
        retVal=searchResult;
    }
    cleanup:
        if(getSecurityKeysOutput) {
            UA_Array_delete(getSecurityKeysOutput, UA_GETSECURITYKEYS_OUTPUT_SIZE,
                            &UA_TYPES[UA_TYPES_VARIANT]);
        }
    return retVal;
}

UA_StatusCode
UA_initializeKeyStorageWithKeys(UA_PubSubSKSKeyStorage *keyStorage, UA_Server *server,
                                const UA_String *securityGroupId,
                                const UA_String *securityPolicyUri,
                                UA_UInt32 currentTokenId, UA_ByteString *currentKey,
                                UA_ByteString *futureKeys, size_t futureKeyCount) {

    if(!keyStorage || !server || !securityGroupId || !securityPolicyUri || !currentKey) 
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(futureKeyCount != 0 && !futureKeys)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;
    keyStorage->referenceCount = 1;

    UA_SecurityPolicy *policy;
    retVal = UA_SecurityPolicy_findPolicyBySecurityPolicyUri(server, *securityPolicyUri,
                                                             &policy);

    if(retVal != UA_STATUSCODE_GOOD) {
        goto error;
    }

    /* get Keys and the number of keys*/
    size_t keyNumber = futureKeyCount + 1;

    /*Get Keys and put them into keyList one by one*/
    /*Allocate memory for these keys*/
    keyStorage->keyList =
        (UA_PubSubKeyListItem *)UA_calloc(keyNumber, sizeof(UA_PubSubKeyListItem));
    UA_PubSubKeyListItem *keyList = keyStorage->keyList;

    if(!keyList) {
        retVal = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }
    keyStorage->keyListSize = keyNumber;

    UA_PubSubKeyListItem *keyListIterator;
    UA_PubSubKeyListItem *lastItem = NULL;
    UA_UInt32 tempTokenID = currentTokenId;

    for(size_t i = 0; i < keyNumber; i++) {
        /*Copy keys*/
        keyListIterator = &keyList[i];
        keyListIterator->keyID = tempTokenID;

        if(i == 0)
            retVal = UA_ByteString_copy(currentKey, &keyListIterator->key);
        else
            retVal = UA_ByteString_copy(&futureKeys[i - 1], &keyListIterator->key);

        if(retVal != UA_STATUSCODE_GOOD) {
            /*  UA_ByteString_copy does an alloc
             *  set the key.data to NULL
             *  => the first item with an error has key.data NULL
             *  => free items in case of error until NULL
             */
            keyListIterator->key.data = NULL;
            goto error;
        }

        if(lastItem)
            lastItem->next = keyListIterator;
        lastItem = keyListIterator;
        keyListIterator->next = NULL;

        /*Overflow control*/
        if(tempTokenID == UA_UINT32_MAX)
            tempTokenID = 1;
        else
            ++tempTokenID;
    }

    /*Save other parameters*/
    /*Need to use ByteString copy, otherwise when a writerGroup is removed the
     * securityGroupID is lost*/
    retVal = UA_String_copy(securityGroupId, &keyStorage->securityGroupID);
    if(retVal != UA_STATUSCODE_GOOD) {
        goto error;
    }

    /*The firstKey in the queue is currentKey*/
    keyStorage->currentItem = &keyList[0];
    keyStorage->firstItem = &keyList[0];
    keyStorage->lastItem = &keyList[keyNumber - 1];
    keyStorage->policy = policy;

    /*Finally save keyStorage in server*/
    LIST_INSERT_HEAD(&server->pubSubSKSKeyList, keyStorage, keyStorageList);

    return retVal;

error:
    UA_PubSubKeySKSStorage_deleteMembers(NULL, keyStorage);
    return retVal;
}



UA_StatusCode
UA_initializeKeyStorageByAskingCurrentKey(UA_Server *server,
                                          const UA_String securityGroupId,
                                          UA_PubSub_SKSConfig *sksConfig) {

    if(!sksConfig || !server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;

    /*Prepare parameters*/

    /*set startingTokenId to 0, Ask for currentKey*/
    UA_UInt32 startingTokenId = 0;

    /*Ask the maxium number that server supports*/
    UA_UInt32 requstedKeyCount = UINT32_MAX;

    /*Declare output*/
    UA_Variant *getSecurityKeysOutput = NULL;

    /*Call getSeucrityKeys on the SKS server*/
    retVal = UA_getSecurityKeys(sksConfig->client, securityGroupId, startingTokenId,

                                requstedKeyCount, &getSecurityKeysOutput);

    if(retVal != UA_STATUSCODE_GOOD) {
        goto cleanup;

    }

    /*Process outputs*/
    UA_String *securityPolicyUri = (UA_String *)getSecurityKeysOutput[0].data;

    /*FirstTokenId of the output*/
    UA_UInt32 firstTokenId = *(UA_UInt32 *)getSecurityKeysOutput[1].data;

    /* get Keys and the number of keys*/
    size_t keyNumber = getSecurityKeysOutput[2].arrayLength;

    if(keyNumber == 0) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_CLIENT, "No SecurityKeysOutput");

        retVal = UA_STATUSCODE_BADNODATA;

        goto cleanup;
    }

    UA_ByteString *keys = (UA_ByteString *)getSecurityKeysOutput[2].data;
    UA_ByteString *currentKey = &keys[0];
    UA_ByteString *futureKeys = NULL;

    if(keyNumber > 1)
        futureKeys = &keys[1];

    UA_Duration timeToNextKey = *(UA_Duration *)getSecurityKeysOutput[3].data;

    UA_Duration keyLifeTime = *(UA_Duration *)getSecurityKeysOutput[4].data;

    retVal = UA_initializeKeyStorageWithKeys(
        sksConfig->keyStorage, server, &securityGroupId, securityPolicyUri, firstTokenId,
        currentKey, futureKeys, keyNumber - 1);

    if(retVal != UA_STATUSCODE_GOOD)
        goto cleanup;

    sksConfig->keyStorage->keyLifeTime = keyLifeTime;

    retVal = addMoveToNextKeyCallback(
        server, (UA_ServerCallback)UA_moveToNextKeyCallbackPull, sksConfig, timeToNextKey,
        &sksConfig->keyStorage->callBackId);

cleanup:

    if(getSecurityKeysOutput) {
        UA_Array_delete(getSecurityKeysOutput, UA_GETSECURITYKEYS_OUTPUT_SIZE,
                        &UA_TYPES[UA_TYPES_VARIANT]);
    }
    return retVal;
}


UA_StatusCode
UA_initializeSksConfig(UA_Server *server, const UA_String securityGroupId,
                       UA_PubSub_SKSConfig *sksConfig, const UA_Boolean getSecurityKeysEnabled) {

    if(!server || !sksConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if (getSecurityKeysEnabled && !sksConfig->client)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* this is not totally implemented by now */
    if (!getSecurityKeysEnabled)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_StatusCode retVal;

    /*Initialize sequenceNumber*/
    sksConfig->sequenceNumber = 1;
    sksConfig->keyNonce.data = NULL;

    /*Try find key for the securityGroup on server*/
    UA_StatusCode searchResult = UA_getPubSubKeyStoragebySecurityGroupId(server, &securityGroupId,
                                                     &sksConfig->keyStorage);

    /*If not found, create a new one*/
    if(searchResult == UA_STATUSCODE_BADNOTFOUND) {

        retVal = UA_PubSubSKSKeyStorage_new(&sksConfig->keyStorage);
        if(retVal != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        retVal = UA_initializeKeyStorageByAskingCurrentKey(server, securityGroupId,
                                                            sksConfig);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_free(sksConfig->keyStorage);
            sksConfig->keyStorage = NULL;
            goto error;
        }

    } else if(searchResult == UA_STATUSCODE_GOOD) {
        ++sksConfig->keyStorage->referenceCount;
    } else {
        retVal = searchResult;
        goto error;
    }

    /* pre allocate sksConfig->keyNonce */
    sksConfig->keyNonce.data = (UA_Byte *)UA_malloc(UA_PUBSUB_AES_CTR_NONCELENGTH);
    if(!sksConfig->keyNonce.data) {
        retVal = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }
    sksConfig->keyNonce.length = UA_PUBSUB_AES_CTR_NONCELENGTH;

    /* initialize channelcontext for this reader/writergroup*/
    retVal = sksConfig->keyStorage->policy->channelModule.newContext(
        sksConfig->keyStorage->policy, NULL, &sksConfig->channelContext);
    if(retVal != UA_STATUSCODE_GOOD)
        goto error;

    retVal =
        UA_importKeyToChannelContext(sksConfig->keyStorage->currentItem->key, sksConfig);
    if(retVal != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    if(sksConfig->keyStorage) {
        UA_PubSubKeySKSStorage_deleteSingle(server,
                                            &sksConfig->keyStorage->securityGroupID);
        sksConfig->keyStorage = NULL;
    }

    if(sksConfig->keyNonce.data)
        UA_free(sksConfig->keyNonce.data);

    return retVal;
}

UA_StatusCode
UA_setCounterBlock(UA_SecurityPolicy *policy, void *channelContext,
                   const UA_ByteString keyNonce, const UA_ByteString messageNonce) {

    if(!policy || !channelContext)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /*A counterBlock is same as local block size*/
    size_t counterBlockLength =
        policy->symmetricModule.cryptoModule.encryptionAlgorithm.getLocalBlockSize(
            policy, channelContext);

    /*Check if length matches*/
    if((UA_PUBSUB_AES_CTR_NONCELENGTH + UA_PUBSUB_ENCODED_NONCELENGTH_IN_MESSAGE +
        UA_PUBSUB_AES_CTR_COUNTERBLOCK_ZEROBYTESLENGTH) != counterBlockLength)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Byte counterArray[UA_PUBSUB_AES_CTR_NONCELENGTH +
                         UA_PUBSUB_ENCODED_NONCELENGTH_IN_MESSAGE +
                         UA_PUBSUB_AES_CTR_COUNTERBLOCK_ZEROBYTESLENGTH];

    UA_ByteString counterBlock;
    counterBlock.data = counterArray;
    counterBlock.length = counterBlockLength;

    /*Make sure the last four bits of counterBlock are zero*/
    /*Quote:
    the setup of counter part is differnt in RFC and mbedTLS
    in RFC: the last 4 bytes should be a big endian integer start with 1
    https://tools.ietf.org/html/rfc3686#section-4
    in mbedTLS
    the last 4 bytes should be initialized to zero
    https://tls.mbed.org/api/aes_8h.html#a375c98cba4c5806d3a39c7d1e1e226da
    here follows the one defined in mbedTLS*/
    memset(counterBlock.data, 0, counterBlockLength);

    /*Then put keyNonce and messageNonce*/
    memcpy(counterBlock.data, keyNonce.data, keyNonce.length);
    memcpy(counterBlock.data + keyNonce.length, messageNonce.data, messageNonce.length);

    UA_StatusCode retVal =
        policy->channelModule.setLocalSymIv(channelContext, &counterBlock);

    return retVal;
}

UA_StatusCode
UA_getSecurityKeys(UA_Client *client, const UA_String securityGroupId,
                   const UA_UInt32 startingTokenId, const UA_UInt32 requestedKeyCount,
                   UA_Variant **getSecurityKeysOutput) {

    if(!client || !getSecurityKeysOutput)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;
    size_t outputsize;
    *getSecurityKeysOutput = NULL;

    /*Put inputs in a variant*/
    UA_Variant getSecurityKeysInput[UA_GETSECURITYKEYS_INPUT_SIZE];

    /*Do Not delete getSecurityKeysOutput*/
    retVal = UA_Variant_setScalarCopy(&getSecurityKeysInput[0], &securityGroupId,
                                      &UA_TYPES[UA_TYPES_STRING]);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = UA_Variant_setScalarCopy(&getSecurityKeysInput[1], &startingTokenId,
                                      &UA_TYPES[UA_TYPES_UINT32]);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = UA_Variant_setScalarCopy(&getSecurityKeysInput[2], &requestedKeyCount,
                                      &UA_TYPES[UA_TYPES_UINT32]);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    /*Call method on server*/
    retVal = UA_Client_call(client, NODEID_SKS_PublishSubscribe,
                            NODEID_SKS_GetSecurityKeys, UA_GETSECURITYKEYS_INPUT_SIZE,
                            getSecurityKeysInput, &outputsize, getSecurityKeysOutput);
    if(retVal != UA_STATUSCODE_GOOD) {
        goto error;
    }

    /*Check ouputs*/
    if(outputsize < 5) {
        retVal = UA_STATUSCODE_BADARGUMENTSMISSING;
        goto error;
    }
    if(outputsize > 5) {
        retVal = UA_STATUSCODE_BADTOOMANYARGUMENTS;
        goto error;
    }

    UA_Variant *outputs = *getSecurityKeysOutput;
    if(outputs[0].type->typeIndex != UA_TYPES_STRING ||
       outputs[1].type->typeIndex != UA_TYPES_UINT32 ||
       outputs[2].type->typeIndex != UA_TYPES_BYTESTRING ||
       (outputs[3].type->typeIndex != UA_TYPES_DURATION &&
        outputs[3].type->typeIndex != UA_TYPES_DOUBLE) ||
       (outputs[4].type->typeIndex != UA_TYPES_DURATION &&
        outputs[4].type->typeIndex != UA_TYPES_DOUBLE)) {
        retVal = UA_STATUSCODE_BADTYPEMISMATCH;
        goto error;
    }

    /*clean inputs*/
    for(size_t i = 0; i < UA_GETSECURITYKEYS_INPUT_SIZE; ++i) {
        UA_Variant_deleteMembers(&getSecurityKeysInput[i]);
    }

    return UA_STATUSCODE_GOOD;

error:
    if(*getSecurityKeysOutput) {
        UA_Array_delete(getSecurityKeysOutput, outputsize, &UA_TYPES[UA_TYPES_VARIANT]);
    }

    for(size_t i = 0; i < UA_GETSECURITYKEYS_INPUT_SIZE; ++i) {
        UA_Variant_deleteMembers(&getSecurityKeysInput[i]);
    }
    return retVal;
}

void
UA_PubSubKey_deleteSKSConfig(UA_PubSub_SKSConfig *sksConfig, UA_Server *server) {

    if(!sksConfig)
        return;

    UA_PubSubSKSKeyStorage *keyStorage = sksConfig->keyStorage;

    if(keyStorage) {
        if(keyStorage->policy && sksConfig->channelContext) {
            keyStorage->policy->channelModule.deleteContext(sksConfig->channelContext);
        }

        if(keyStorage->referenceCount == 1) {
            if(server) {
                UA_PubSubKeySKSStorage_delete(server, keyStorage);
            }
        } else
            --keyStorage->referenceCount;
    }

    if(sksConfig->keyNonce.data)
        UA_ByteString_clear(&sksConfig->keyNonce);
}

/**
 * Implementation of the SetSecurityKeys service
 * See UA_Server_setMethodNode_callback for parameter documentation.
 * See OPC Unified Architecture, Part 14, chapter 8 for method documentation
 * [in] String SecurityGroupId
 * [in] String SecurityPolicyUri
 * [in] IntegerId CurrentTokenId
 * [in] ByteString CurrentKey
 * [in] ByteString[] FutureKeys
 * [in] Duration TimeToNextKey
 * [in] Duration KeyLifetime
 */
static UA_StatusCode
UA_setSecurityKeysAction(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionHandle, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_StatusCode retVal;

    if(!server || !input || !output)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /*Check whether the channel is encrypted according to specification*/
    session_list_entry *session_entry;
    LIST_FOREACH(session_entry, &server->sessionManager.sessions, pointers) {
        if(UA_NodeId_equal(&session_entry->session.sessionId, sessionId)) {
            if(session_entry->session.header.channel->securityMode !=
               UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
                return UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        }
    }

    /*check inputs*/
    if(inputSize < 7)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 7 || outputSize > 0)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    if(input[0].type->typeIndex != UA_TYPES_STRING ||     /*SecurityGroupId*/
       input[1].type->typeIndex != UA_TYPES_STRING ||     /*SecurityPolicyUri*/
       input[2].type->typeIndex != UA_TYPES_UINT32 ||     /*CurrentTokenId*/
       input[3].type->typeIndex != UA_TYPES_BYTESTRING || /*CurrentKey*/
       input[4].type->typeIndex != UA_TYPES_BYTESTRING || /*FutureKeys*/
       input[5].type->typeIndex != UA_TYPES_DURATION ||   /*TimeToNextKey*/
       input[6].type->typeIndex != UA_TYPES_DURATION) /*KeyLifeTime*/ {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /*Get inputs*/
    UA_String *securityGroupId = (UA_String *)input[0].data;
    UA_String *securityPolicyUri = (UA_String *)input[1].data;
    UA_UInt32 currentKeyId = *(UA_UInt32 *)input[2].data;
    UA_ByteString *currentKey = (UA_ByteString *)input[3].data;
    UA_ByteString *futureKeys = (UA_ByteString *)input[4].data;
    size_t futureKeyNumber = input[4].arrayLength;
    UA_Duration timeToNextKey = *(UA_Duration *)input[5].data;
    UA_Duration keyLifeTime = *(UA_Duration *)input[6].data;

    /*Look for existing keyStorage*/
    UA_PubSubSKSKeyStorage *keyStorage;
    UA_StatusCode searchResult =
        UA_getPubSubKeyStoragebySecurityGroupId(server, securityGroupId, &keyStorage);

    /* when using setSecurityKeys, the callback shall be added after adding
       the writer / reader group -> if no matching security group was found,
       it is definitely an error be the caller
     */
    if(searchResult == UA_STATUSCODE_GOOD) {

        keyStorage->keyLifeTime = keyLifeTime;

        if(keyStorage->keyListSize == 0) {

            retVal = UA_initializeKeyStorageWithKeys(
                keyStorage, server, securityGroupId, securityPolicyUri, currentKeyId,
                currentKey, futureKeys, futureKeyNumber);
            if(retVal != UA_STATUSCODE_GOOD)
                return retVal;

            retVal =
                addMoveToNextKeyCallback(server, (UA_ServerCallback)UA_moveToNextKeyCallbackPush, keyStorage,
                                         timeToNextKey, &keyStorage->callBackId);
            if(retVal == UA_STATUSCODE_GOOD)
                keyStorage->callBackRegistered = UA_TRUE;

        } else {
            /*Insert and update currentKey*/
            retVal = UA_PubSub_updateKeyStorage(keyStorage, currentKeyId, currentKey, 1,
                                                currentKeyId);
            if(retVal != UA_STATUSCODE_GOOD)
                return retVal;

            /*try insert futureKeys,last parameter=0 so does not update currentKey*/
            retVal = UA_PubSub_updateKeyStorage(keyStorage, currentKeyId + 1, currentKey,
                                                futureKeyNumber, 0);
            if(retVal != UA_STATUSCODE_GOOD)
                return retVal;
        }
    } else {
        return searchResult;
    }

    return retVal;
}

/* see header for documentation */
UA_StatusCode UA_EXPORT
UA_Server_addPubSubSKSPush(UA_Server *server) {

    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_Server_setMethodNode_callback(server, NODEID_SKS_SetSecurityKeys,
                                              UA_setSecurityKeysAction);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Publisher_encodeSecurityHeaderBinary_Internal(UA_WriterGroup *writerGroup,
                                                 UA_NetworkMessage *networkMessage) {

    if(!writerGroup || !networkMessage)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_WriterGroup *wg = writerGroup;

    UA_WriterGroupConfig *wgConfig = &wg->config;
    UA_MessageSecurityMode securityMode = wgConfig->securityMode;
    UA_PubSub_SKSConfig *sksConfig = &wgConfig->sksConfig;

    /*SetCounterBlock and other SecurityFlags*/
    if(securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {

        networkMessage->securityEnabled = UA_TRUE;
        networkMessage->securityHeader.networkMessageSigned = UA_TRUE;

        if(securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            networkMessage->securityHeader.networkMessageEncrypted = UA_TRUE;
        else
            networkMessage->securityHeader.networkMessageEncrypted = UA_FALSE;

        /*Set securityTokenID*/
        networkMessage->securityHeader.securityTokenId =
            sksConfig->keyStorage->currentItem->keyID;

        networkMessage->securityHeader.securityFooterEnabled = UA_FALSE;

        /*MessageNonce contains a random part and a sequence number part*/
        /*This random can be a pseudo random number, see specifiation*/
        UA_UInt32 messageNonceRandom = UA_UInt32_random();

        UA_Byte *nonceBuf = networkMessage->securityHeader.messageNonce.data;
        UA_Byte *nonceBufEnd = nonceBuf + UA_PUBSUB_ENCODED_NONCELENGTH_IN_MESSAGE;

        /*Combine messageNonce and sequenceNumber*/
        retVal = UA_UInt32_encodeBinary(&messageNonceRandom, &nonceBuf, nonceBufEnd);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;

        retVal =
            UA_Int32_encodeBinary(&sksConfig->sequenceNumber, &nonceBuf, nonceBufEnd);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;

        /*Set counterBlock=messageNonce+sequencenumber+zerobytes, see function for
         * details*/
        retVal = UA_setCounterBlock(
            sksConfig->keyStorage->policy, wg->config.sksConfig.channelContext,
            sksConfig->keyNonce, networkMessage->securityHeader.messageNonce);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;

        /*Set a buffer for signature so the binary size is calculated correctly*/
        networkMessage->signature.length =
            sksConfig->keyStorage->policy->symmetricModule.cryptoModule.signatureAlgorithm
                .getLocalSignatureSize(sksConfig->keyStorage->policy,
                                       &sksConfig->channelContext);

        /* 0 = no certificate was set */
        if(networkMessage->signature.length == 0)
            return UA_STATUSCODE_BADNOMATCH;

        /* todo remove alloc */
        /* free will be handled by caller */
        if(UA_ByteString_allocBuffer(&networkMessage->signature,
                                     networkMessage->signature.length) !=
           UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        ++sksConfig->sequenceNumber;
    }

    return retVal;
}

UA_StatusCode
UA_Publisher_signAndEncryptNetWorkMessageBinary_Internal(
    UA_WriterGroup *writerGroup, UA_NetworkMessage *networkMessage, UA_ByteString *buf) {

    if(!writerGroup || !networkMessage || !buf)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;

    UA_PubSub_SKSConfig *sksConfig = &writerGroup->config.sksConfig;

    UA_Byte *bufPos = buf->data;
    UA_Byte *bufEnd = buf->data + buf->length;

    /*The position of these pointers ,see Fig27 in OPCUA Part14, chapter 7.2.2.2.1*/
    UA_Byte *dataToSignStart = buf->data;
    UA_Byte *dataToSignEnd =
        bufEnd - UA_ByteString_calcSizeBinary(&networkMessage->signature);

    UA_Byte *dataToEncryptEnd = dataToSignEnd;
    UA_Byte *signatureStart = dataToSignEnd;

    /*This pointer is special, need to first encode headers and then you can know
    where starts the payload which needs to be encrypted, it now points to the same
    position at payload end*/
    UA_Byte *dataToEncryptStart = dataToEncryptEnd;
    UA_SecurityPolicy *policy = sksConfig->keyStorage->policy;

    /*Encode networkmessage as normal(unencrypted) and get dataToEncryptStart back*/
    retVal = UA_NetworkMessage_encodeBinary(networkMessage, &bufPos, bufEnd,
                                            &dataToEncryptStart);
    if(retVal != UA_STATUSCODE_GOOD) {
        return retVal;
    }

    /*Encrypt payload*/
    if(networkMessage->securityHeader.networkMessageEncrypted) {
        /*The encryption takes in place, no need to encode again*/
        /*encryptBuf is inside buf*/
        UA_ByteString encryptBuf;
        encryptBuf.data = dataToEncryptStart;
        encryptBuf.length = (size_t)dataToEncryptEnd - (size_t)dataToEncryptStart;

        retVal = policy->symmetricModule.cryptoModule.encryptionAlgorithm.encrypt(
            policy, sksConfig->channelContext, &encryptBuf);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }
    }

    /*Signing the payload, it needs to be done after encrypion*/
    /*Actually this "if"  can be omitted, since a securityEnabled message should be signed
     * by default*/
    if(networkMessage->securityHeader.networkMessageSigned) {
        /*sigBuf is inside buf*/
        UA_ByteString sigBuf;
        sigBuf.length = (size_t)dataToSignEnd - (size_t)dataToSignStart;
        sigBuf.data = dataToSignStart;

        retVal = policy->symmetricModule.cryptoModule.signatureAlgorithm.sign(
            policy, sksConfig->channelContext, &sigBuf, &networkMessage->signature);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }

        /*replace the signature in original buf*/
        retVal = UA_ByteString_encodeBinary(&networkMessage->signature, &signatureStart,
                                            bufEnd);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }
    }

    return retVal;
}

UA_StatusCode
UA_Subscriber_verifyAndDecryptNetWorkMessageBinary_Internal(
    UA_ReaderGroup *readerGroup, UA_ByteString *buffer, size_t *currentPosition,
    UA_NetworkMessage *currentNetworkMessage, UA_Byte *dataToDecryptStart) {

    if(!readerGroup || !buffer || !currentPosition || !currentNetworkMessage ||
       !dataToDecryptStart)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_SecurityPolicy *policy = readerGroup->config.sksConfig.keyStorage->policy;

    /*Actually this "if"  can be omitted, since a securityEnabled message should be signed
     * by default*/
    if(currentNetworkMessage->securityHeader.networkMessageSigned == UA_TRUE) {
        /*First verify the signature, because message is first encrypted then signed*/
        size_t signatureLength =
            policy->symmetricModule.cryptoModule.signatureAlgorithm.getLocalSignatureSize(
                policy, &readerGroup->config.sksConfig.channelContext);

        /* 0 = no certificate was set */
        if(signatureLength == 0)
            return UA_STATUSCODE_BADNOMATCH;

        currentNetworkMessage->signature.length = signatureLength;

        size_t signatureBinaryLength =
            UA_ByteString_calcSizeBinary(&currentNetworkMessage->signature);

        /*Get dataToSign directly in Buffer*/
        UA_Byte *dataToSignStart = buffer->data;
        UA_Byte *dataToSignEnd = buffer->data + buffer->length - signatureBinaryLength;
        size_t dataToSignSize = (size_t)dataToSignEnd - (size_t)dataToSignStart;
        UA_ByteString dataToSign = {dataToSignSize, dataToSignStart};

        /*Decode the signature in the received Buf into networkMessage,
        here dataToSignSize performs as a offset*/
        retVal = UA_ByteString_decodeBinary(buffer, &dataToSignSize,
                                            &currentNetworkMessage->signature);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;

        /*Caculate signature and compare it with the received value*/
        retVal = policy->symmetricModule.cryptoModule.signatureAlgorithm.verify(
            policy, readerGroup->config.sksConfig.channelContext, &dataToSign,
            &currentNetworkMessage->signature);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }

        /*Then set counterBlock and do the decryption*/
        if(currentNetworkMessage->securityHeader.networkMessageEncrypted == UA_TRUE) {
            /*First set counterBlock*/
            retVal =
                UA_setCounterBlock(policy, readerGroup->config.sksConfig.channelContext,
                                   readerGroup->config.sksConfig.keyNonce,
                                   currentNetworkMessage->securityHeader.messageNonce);
            if(retVal != UA_STATUSCODE_GOOD)
                return retVal;

            /*Get DataToDecrypt directly in Buffer*/
            size_t dataToDecryptLength =
                (size_t)dataToSignEnd - (size_t)dataToDecryptStart;
            UA_ByteString dataToDecryptBuf = {dataToDecryptLength, dataToDecryptStart};

            /*Decryption*/
            retVal = policy->symmetricModule.cryptoModule.encryptionAlgorithm.decrypt(
                policy, readerGroup->config.sksConfig.channelContext, &dataToDecryptBuf);
            if(retVal != UA_STATUSCODE_GOOD) {
                return retVal;
            }
        }
    }
    return retVal;
}

#endif

