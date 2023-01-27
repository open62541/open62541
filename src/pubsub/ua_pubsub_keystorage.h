/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#ifndef UA_PUBSUB_KEYSTORAGE
#define UA_PUBSUB_KEYSTORAGE

#include <open62541/plugin/securitypolicy.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/client.h>

#include "open62541_queue.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SKS

/**
 * PubSubKeyStorage
 * ================
 * A PubSubKeyStorage provides a linked list to store all the keys used to
 * secure the messages. It keeps the records of old keys (past keys), current
 * key, new keys (futurekeys), time to move to next key and callback id.
 *
 * PubSubKeyListItem is the basic item stored in the KeyList of KeyStorage. It
 * provides keyId, Key, and pointer to the next key in KeyList. The KeyId is used
 * to identify and update currentKey in the keystorage. The KeyId is the SecurityTokenId
 * that appears in the header of messages secured with the CurrentKey.
 *
 * Working
 * =======
 *                     +------------------------------+
 *                     |AddReaderGroup/AddWriterGroup |
 *                     +------------------------------+
 *                                    |
 *                                    V
 *                     +--------------------+
 *                     |CheckSecurityGroupId|
 *                     +--------------------+
 *                                    |Yes
 *                                    V
 *                     +--------------------+
 *                     |InitializeKeyStorage|
 *                     +--------------------+
 *                                    |
 *                                    V
 *                     +----------------------------+
 *                     |store/updateKeysInKeyStorage|
 *                     +----------------------------+
 *                                    |
 *                                    V
 *                     +------------------------------------------+
 *                     |activateKeysToAllPubSubGroupChannelContext|
 *                     +------------------------------------------+
 *                                    |                        Ʌ
 *                                    V                        |
 *                     +-----------------------+               |
 *                     |addKeyRolloverCallbacks|               |
 *                     +-----------------------+               |
 *                                    |                        |
 *                                    V                        |
 *                     +-------------------+                   |
 *                     |keyRolloverCallback|                   |
 *                     +-------------------+                   |
 *                                    |CurrentKey!=LastItem    |
 *                                    -------------------------+
 *
 * A KeyStorage is created and initialized when a ReaderGroup or WriterGroup is
 * created with securityGroupId and SecurityMode SignAndEncrypt. The new
 * KeyStorage is added to the server KeyStorageList. At this time KeyList is empty.
 *
 * UA_PubSubKeyStorage_storeSecurityKeys is used to push the keys into existing
 * keystorage. In order to update the KeyList of an existing keyStorage,
 * UA_PubSubKeyStorage_update is called.
 *
 * After adding/updating the keys to keystorage, the current key should be
 * activated to the associated PubSub Group's ChannelContext in the server. The
 * security Policy associated with PubSub Group will take the keys from
 * channel context and use them to secure the messages.
 * The UA_PubSubKeyStorage_storeSecurityKeys and UA_PubSubKeyStorage_update
 * method will be used by setSecurityKeysAction and getSecurityKeysAction to
 * retrieve the keys from SKS server and store keys in local storage.
 *
 * Each key has a life time, after which the current key is expired and move to
 * next key in the existing list. For this a callback function is added to the
 * server. The callback function keyRolloverCallback is added to the server as a
 * timed callback. The addKeyRolloverCallbacks function calculates the time
 * stamp to trigger the callback when the current Key expires and roll
 * over to the next key in existing list.
 *
 */

/**
 * @brief This structure holds the information about the keys
 */
typedef struct UA_PubSubKeyListItem {
    /* The SecurityTokenId associated with Key*/
    UA_UInt32 keyID;

    /* This key is not used directly since the protocol associated with the PubSubGroup(s)
     * specifies an algorithm to generate distinct keys for different types of
     * cryptography operations*/
    UA_ByteString key;

    /* Pointers to the key list entries*/
    TAILQ_ENTRY(UA_PubSubKeyListItem) keyListEntry;
} UA_PubSubKeyListItem;

/* Queue Definition*/
typedef TAILQ_HEAD(keyListItems, UA_PubSubKeyListItem) keyListItems;

/**
 * @brief It is used to hold configuration information required to connect an SKS server
 * and fetch the security keys
 */
typedef struct UA_PubSubSKSConfig {
    UA_ClientConfig clientConfig;
    const char *endpointUrl;
    UA_Server_sksPullRequestCallback userNotifyCallback;
    void *context;
    UA_UInt32 reqId;
} UA_PubSubSKSConfig;

/**
 * @brief This structure holds all info and keys related to one SecurityGroup.
 * it is used as a list.
 */
typedef struct UA_PubSubKeyStorage {

    /**
     * security group id of the security group related to this storage
     */
    UA_String securityGroupID;

    /**
     * none-owning pointer to the security policy related to this storage
     */
    UA_PubSubSecurityPolicy *policy;

    /**
     * in case of the SKS server, the key storage structure is deleted when removing the
     * security group.
     * in case of publisher / subscriber, one key storage structure is
     * referenced by multiple reader / writer groups have a reference count to manage free
     */
    UA_UInt32 referenceCount;

    /**
     * array of keys. the elements inside this array have a next pointer.
     * keyList can therefore be used as linked list.
     */
    keyListItems keyList;

    /**
     * size of the keyList
     */
    size_t keyListSize;

    /**
     * The maximum number of Past keys a keystorage is allowed to store
     */
    UA_UInt32 maxPastKeyCount;

    /**
     * The maximum number of Future keys a keyStorage is allowed to store
     */
    UA_UInt32 maxFutureKeyCount;

    /*
     * The maximum keylist size, calculated from maxPastKeyCount and maxFutureKeyCount
     */
    UA_UInt32 maxKeyListSize;

    /**
     * The SecurityTokenId that appears in the header of messages secured with the
     * CurrentKey. It starts at 1 and is incremented by 1 each time the KeyLifetime
     * elapses even if no keys are requested. If the CurrentTokenId increments past the
     * maximum value of UInt32 it restarts a 1.
     */
    UA_UInt32 currentTokenId;

    /**
     *  the current key used to secure the messages
     */
    UA_PubSubKeyListItem *currentItem;

    /**
     * keyLifeTime used to update the CurrentKey from the Local KeyStorage
     */
    UA_Duration keyLifeTime;

    /**
     * id used to register the callback to retrieve the keys related to this security
     * group
     */
    UA_UInt64 callBackId;

    /**
     * used to store the sks related information to connect with SKS server and fetch security keys.
     */
    UA_PubSubSKSConfig sksConfig;

    /**
     * Pointer to the key storage list
     */
    LIST_ENTRY(UA_PubSubKeyStorage) keyStorageList;

} UA_PubSubKeyStorage;

/**
 * @brief Find the Keystorage from the Server KeyStorageList and returns the pointer to
 * the keystorage
 *
 * @param server holds the keystoragelist
 * @param securityGroupId of the keystorage to be found
 * @return Pointer to the keystorage on success, null pointer on failure
 */
UA_PubSubKeyStorage *
UA_PubSubKeyStorage_findKeyStorage(UA_Server *server, UA_String securityGroupId);

/**
 * @brief retreives the security policy pointer from the PubSub configuration by
 * SecurityPolicyUri
 *
 * @param server the server object
 * @param securityPolicyUri the URI of the security policy
 * @param policy the pointer to the security policy
 * @return UA_StatusCode return status code
 */
UA_PubSubSecurityPolicy *
findPubSubSecurityPolicy(UA_Server *server, const UA_String *securityPolicyUri);

/**
 * @brief Deletes the keystorage from the server and its members
 *
 * @param server where the keystorage is created
 * @param keyStorage pointer to the keystorage
 */
void
UA_PubSubKeyStorage_delete(UA_Server *server, UA_PubSubKeyStorage *keyStorage);

/**
 * @brief Initializes an empty Keystorage for the SecurityGroupId and add it to the Server
 * KeyStorageList
 *
 * @param server The server object
 * @param keyStorage Pointer to the keystorage to be initialized
 * @param securityGroupId The identifier of the SecurityGroup
 * @param policy The security policy assocaited with the security algorithm
 * @param maxPastKeyCount maximum number of past keys a keystorage is allowed to store
 * @param maxFutureKeyCount maximum number of future keys a keystorage is allowed to store
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_PubSubKeyStorage_init(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                         const UA_String *securityGroupId,
                         UA_PubSubSecurityPolicy *policy,
                         UA_UInt32 maxPastKeyCount, UA_UInt32 maxFutureKeyCount);

/**
 * @brief After Keystorage is initialized and added to the server, this method is called
 * to store the current Keys and futurekeys.
 *
 * @param server the server object
 * @param keyStorage pointer to the keyStorage
 * @param currentTokenId The token Id of the current key it starts with 1 and increaments
 * each time keylifetime expires
 * @param currentKey the key used for encrypt the current messages
 * @param futureKeys pointer to the future keys
 * @param futureKeyCount the number future keys provided
 * @param keyLifeTime the time period when the key expires and move to next future key in
 * milli seconds
 * @return UA_StatusCode the return status
 */
UA_StatusCode
UA_PubSubKeyStorage_storeSecurityKeys(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                                      UA_UInt32 currentTokenId, const UA_ByteString *currentKey,
                                      UA_ByteString *futureKeys, size_t futureKeyCount,
                                      UA_Duration msKeyLifeTime);

/**
 * @brief Finds the KeyItem from the KeyList by KeyId
 *
 * @param keyId the identifier of the Key
 * @param keyStorage pointer to the keystorage
 * @param keyItem returned pointer to the keyItem in the KeyList
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_PubSubKeyStorage_getKeyByKeyID(const UA_UInt32 keyId, UA_PubSubKeyStorage *keyStorage,
                                  UA_PubSubKeyListItem **keyItem);

/**
 * @brief Adds a new KeyItem at the end of the KeyList
 * to the new KeyListItem.
 *
 * @param keyStorage pointer to the keystorage
 * @param key the key to be added
 * @param keyID the keyID associated with the key to be added
 */
UA_PubSubKeyListItem *
UA_PubSubKeyStorage_push(UA_PubSubKeyStorage *keyStorage, const UA_ByteString *key,
                         UA_UInt32 keyID);

/**
 * @brief It calculates the time to trigger the callback to update current key, adds the
 * callback to the server and returns the callbackId.
 *
 * @param server the server object
 * @param keyStorage the pointer to the existing keystorage in the server
 * @param callback the callback function to be added to the server
 * @param timeToNextMs time in milli seconds to trigger the callback function
 * @param callbackID the returned callbackId of the added callback function
 * @return UA_StatusCode the return status
 */
UA_StatusCode
UA_PubSubKeyStorage_addKeyRolloverCallback(UA_Server *server,
                                          UA_PubSubKeyStorage *keyStorage,
                                          UA_ServerCallback callback,
                                          UA_Duration timeToNextMs,
                                          UA_UInt64 *callbackID);

/**
 * @brief It takes the current Key data, divide it into signing key, encrypting key and
 * keyNonce according to security policy associated with PubSub Group and set it in
 * channel context of the assocaited PubSub Group. In case of pubSubGroupId is
 * UA_NODEID_NULL, all the Reader/WriterGroup's channelcontext are updated with matching
 * SecurityGroupId.
 *
 * @param server The server object
 * @param pubSubGroupId the nodeId of the Reader/WirterGroup whose channel context to be
 * updated
 * @param securityGroupId The identifier for the SecurityGroup
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_PubSubKeyStorage_activateKeyToChannelContext(UA_Server *server, const UA_NodeId pubSubGroupId,
                                                const UA_String securityGroupId);

/**
 * @brief The callback function to update the current key from keystorage in the server
 * and activate the current key into channel context of the associated PubSub Group
 *
 * @param server the server object
 * @param keyStorage the pointer to the keystorage
 */
void
UA_PubSubKeyStorage_keyRolloverCallback(UA_Server *server, UA_PubSubKeyStorage *keyStorage);

/**
 * @brief It updates/adds the current and future keys into the existing KeyStorage.
 * If the currentKeyID is known to existing keyStorage, then it is set as the currentKey
 * and any future keys are appended to the existing list. If the currentKeyId is not know
 * then, existing keyList is discarded and replaced with the new list.
 *
 * @param server the server object
 * @param keyStorage pointer to the keystorage
 * @param currentKey the currentKey data
 * @param currentKeyID the identifier of the current Key
 * @param futureKeySize the size of the future key list
 * @param futureKeys the pointer to the future keys list
 * @param msKeyLifeTime the updated time to move to next key
 * @return UA_StatusCode the return status
 */
UA_StatusCode
UA_PubSubKeyStorage_update(UA_Server *server, UA_PubSubKeyStorage *keyStorage,
                           const UA_ByteString *currentKey, UA_UInt32 currentKeyID,
                           const size_t futureKeySize, UA_ByteString *futureKeys,
                           UA_Duration msKeyLifeTime);

/* KeyStorage must be referenced by atleast one PubSubGroup. This method reduces
 * the reference count by one. If no PubSubGroup uses the key storage, then it
 * is deleted. */
void
UA_PubSubKeyStorage_detachKeyStorage(UA_Server *server, UA_PubSubKeyStorage *keyStorage);

/*Calls get SecurityKeys Method and Store the returned keys into KeyStorage*/
UA_StatusCode
getSecurityKeysAndStoreFetchedKeys(UA_Server *server, UA_PubSubKeyStorage *keyStorage);

#endif

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */
