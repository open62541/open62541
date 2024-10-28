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

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SKS

#include "open62541_queue.h"
#include "ua_pubsub_internal.h"

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

    /* This key is not used directly since the protocol associated with the
     * PubSubGroup(s) specifies an algorithm to generate distinct keys for
     * different types of cryptography operations*/
    UA_ByteString key;

    /* Pointers to the key list entries*/
    TAILQ_ENTRY(UA_PubSubKeyListItem) keyListEntry;
} UA_PubSubKeyListItem;

/* Queue Definition*/
typedef TAILQ_HEAD(keyListItems, UA_PubSubKeyListItem) keyListItems;

/* Used to hold configuration information required to connect an SKS server and
 * fetch the security keys */
typedef struct UA_PubSubSKSConfig {
    UA_ClientConfig clientConfig;
    const char *endpointUrl;
    UA_Server_sksPullRequestCallback userNotifyCallback;
    void *context;
    UA_UInt32 reqId;
} UA_PubSubSKSConfig;

/* Holds all info and keys related to one SecurityGroup */
struct UA_PubSubKeyStorage {

    /* Security group id of the security group related to this storage */
    UA_String securityGroupID;

    /* Non-owning pointer to the security policy related to this storage */
    UA_PubSubSecurityPolicy *policy;

    /* In case of the SKS server, the key storage structure is deleted when
     * removing the security group. in case of publisher / subscriber, one key
     * storage structure is referenced by multiple reader / writer groups have a
     * reference count to manage free */
    UA_UInt32 referenceCount;

    /* Array of keys. the elements inside this array have a next pointer.
     * keyList can therefore be used as linked list. */
    keyListItems keyList;
    size_t keyListSize;

    /* Maximum number of past keys a keystorage is allowed to store */
    UA_UInt32 maxPastKeyCount;

    /* Maximum number of Future keys a keyStorage is allowed to store */
    UA_UInt32 maxFutureKeyCount;

    /* Maximum keylist size, calculated from maxPastKeyCount and
     * maxFutureKeyCount */
    UA_UInt32 maxKeyListSize;

    /* The SecurityTokenId that appears in the header of messages secured with
     * the CurrentKey. It starts at 1 and is incremented by 1 each time the
     * KeyLifetime elapses even if no keys are requested. If the CurrentTokenId
     * increments past the maximum value of UInt32 it restarts a 1. */
    UA_UInt32 currentTokenId;

    /* The current key used to secure the messages */
    UA_PubSubKeyListItem *currentItem;

    /* KeyLifeTime used to update the CurrentKey from the Local KeyStorage */
    UA_Duration keyLifeTime;

    /* Id used to register the callback to retrieve the keys related to this
     * security group */
    UA_UInt64 callBackId;

    /* Sks related information to connect with SKS server and fetch security
     * keys */
    UA_PubSubSKSConfig sksConfig;

    /* Pointer to the key storage list */
    LIST_ENTRY(UA_PubSubKeyStorage) keyStorageList;

};

/**
 * @brief Find the Keystorage from the KeyStorageList and returns the pointer to
 * the keystorage
 *
 * @param psm holds the keystoragelist
 * @param securityGroupId of the keystorage to be found
 * @return Pointer to the keystorage on success, null pointer on failure
 */
UA_PubSubKeyStorage *
UA_PubSubKeyStorage_find(UA_PubSubManager *psm, UA_String securityGroupId);

/**
 * @brief retreives the security policy pointer from the PubSub configuration by
 * SecurityPolicyUri
 *
 * @param psm the PubSubManager
 * @param securityPolicyUri the URI of the security policy
 * @param policy the pointer to the security policy
 * @return UA_StatusCode return status code
 */
UA_PubSubSecurityPolicy *
findPubSubSecurityPolicy(UA_PubSubManager *psm,
                         const UA_String *securityPolicyUri);

/**
 * @brief Deletes the keystorage from the server and its members
 *
 * @param psm the PubSubManager
 * @param keyStorage pointer to the keystorage
 */
void
UA_PubSubKeyStorage_delete(UA_PubSubManager *psm,
                           UA_PubSubKeyStorage *keyStorage);

/**
 * @brief Initializes an empty Keystorage for the SecurityGroupId and add it to the Server
 * KeyStorageList
 *
 * @param psm the PubSubManager
 * @param keyStorage Pointer to the keystorage to be initialized
 * @param securityGroupId The identifier of the SecurityGroup
 * @param policy The security policy assocaited with the security algorithm
 * @param maxPastKeyCount maximum number of past keys a keystorage is allowed to store
 * @param maxFutureKeyCount maximum number of future keys a keystorage is allowed to store
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_PubSubKeyStorage_init(UA_PubSubManager *psm,
                         UA_PubSubKeyStorage *keyStorage,
                         const UA_String *securityGroupId,
                         UA_PubSubSecurityPolicy *policy,
                         UA_UInt32 maxPastKeyCount, UA_UInt32 maxFutureKeyCount);

void
UA_PubSubKeyStorage_clearKeyList(UA_PubSubKeyStorage *ks);

/**
 * @brief Add keys tot the key storage. Generates the keyId internally. They get
 * appended to the end of the list. This method DOES NOT VALIDATE the
 * maxKeyListSize property! Do this before.
 *
 * @param keyStorage pointer to the keyStorage
 * @param keysSize the number of keys provided
 * @param keys pointer to the keys
 * @param currentKeyId The new keyIds start at currentKeyId + 1
 * @return UA_StatusCode the return status
 */
UA_StatusCode
UA_PubSubKeyStorage_addSecurityKeys(UA_PubSubKeyStorage *keyStorage, size_t keysSize,
                                    UA_ByteString *keys, UA_UInt32 currentKeyId);

/* Fetch the key from the list and set it as the current key */
UA_StatusCode
UA_PubSubKeyStorage_setCurrentKey(UA_PubSubKeyStorage *keyStorage,
                                  UA_UInt32 keyId);

/**
 * @brief Finds the KeyItem from the KeyList by KeyId
 *
 * @param keyStorage pointer to the keystorage
 * @param keyId the identifier of the Key
 * @return NULL or the found item
 */
UA_PubSubKeyListItem *
UA_PubSubKeyStorage_getKeyByKeyId(UA_PubSubKeyStorage *keyStorage,
                                  const UA_UInt32 keyId);

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
 * @param psm the PubSubManager
 * @param keyStorage the pointer to the existing keystorage in the server
 * @param callback the callback function to be added to the server
 * @param timeToNextMs time in milli seconds to trigger the callback function
 * @param callbackID the returned callbackId of the added callback function
 * @return UA_StatusCode the return status
 */
UA_StatusCode
UA_PubSubKeyStorage_addKeyRolloverCallback(UA_PubSubManager *psm,
                                           UA_PubSubKeyStorage *keyStorage,
                                           UA_Callback callback,
                                           UA_Duration timeToNextMs,
                                           UA_UInt64 *callbackID);

/**
 * @brief It takes the current Key data, divide it into signing key, encrypting key and
 * keyNonce according to security policy associated with PubSub Group and set it in
 * channel context of the assocaited PubSub Group. In case of pubSubGroupId is
 * UA_NODEID_NULL, all the Reader/WriterGroup's channelcontext are updated with matching
 * SecurityGroupId.
 *
 * @param psm the PubSubManager
 * @param pubSubGroupId the nodeId of the Reader/WirterGroup whose channel context to be
 * updated
 * @param securityGroupId The identifier for the SecurityGroup
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_PubSubKeyStorage_activateKeyToChannelContext(UA_PubSubManager *psm,
                                                const UA_NodeId pubSubGroupId,
                                                const UA_String securityGroupId);

/**
 * @brief The callback function to update the current key from keystorage in the server
 * and activate the current key into channel context of the associated PubSub Group
 *
 * @param psm the PubSubManager
 * @param keyStorage the pointer to the keystorage
 */
void
UA_PubSubKeyStorage_keyRolloverCallback(UA_PubSubManager *psm,
                                        UA_PubSubKeyStorage *keyStorage);

/* KeyStorage must be referenced by atleast one PubSubGroup. This method reduces
 * the reference count by one. If no PubSubGroup uses the key storage, then it
 * is deleted. */
void
UA_PubSubKeyStorage_detachKeyStorage(UA_PubSubManager *psm,
                                     UA_PubSubKeyStorage *keyStorage);

/* Calls get SecurityKeys Method and Store the returned keys into KeyStorage */
UA_StatusCode
getSecurityKeysAndStoreFetchedKeys(UA_PubSubManager *psm,
                                   UA_PubSubKeyStorage *keyStorage);

#endif

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */
