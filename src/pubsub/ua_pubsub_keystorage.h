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
#include <open62541/server.h>

#include "open62541_queue.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SKS

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
UA_Server_findKeyStorage(UA_Server *server, UA_String securityGroupId);

/**
 * @brief retreives the security policy pointer from the PubSub configuration by
 * SecurityPolicyUri
 *
 * @param server the server object
 * @param securityPolicyUri the URI of the security policy
 * @param policy the pointer to the security policy
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_Server_findPubSubSecurityPolicy(UA_Server *server, const UA_String *securityPolicyUri,
                                   UA_PubSubSecurityPolicy **policy);

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
 * @param server the server object
 * @param securityGroupId the identifier of the SecurityGroup
 * @param securityPolicyUri the security policy assocaited with the security algorithm
 * @param maxPastKeyCount maximum number of past keys a keystorage is allowed to store
 * @param maxFutureKeyCount maximum number of future keys a keystorage is allowed to store
 * @param keyStorage pointer to the keystorage to be initialized
 * @return UA_StatusCode return status code
 */
UA_StatusCode
UA_PubSubKeyStorage_init(UA_Server *server, const UA_String *securityGroupId,
                               const UA_String *securityPolicyUri,
                               UA_UInt32 maxPastKeyCount, UA_UInt32 maxFutureKeyCount,
                               UA_PubSubKeyStorage *keyStorage);

#endif

_UA_END_DECLS

#endif /* UA_ENABLE_PUBSUB */
