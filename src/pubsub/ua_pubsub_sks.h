/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 ifak e.V. Magdeburg (Holger Zipper)
 */

#ifndef UA_PUBSUB_SKS_H_
#define UA_PUBSUB_SKS_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_SECURITY /* conditional compilation */

#include <open62541/client.h>

#include "ua_pubsub_networkmessage.h"

#include "ua_pubsub.h"

/**
 * Size of the input of GetSecurityKeys method
 */
#define UA_GETSECURITYKEYS_INPUT_SIZE 3

/**
 * Size of the output of GetSecurityKeys method
 */
#define UA_GETSECURITYKEYS_OUTPUT_SIZE 5

/**
 * The interval of key update
 * Todo: remove
 */
#define UA_PUBSUB_CUSTOM_UPDATEKEY_INTERVAL 10000

/**
 * a custom loglevel for debuging, if loglevel <250 it will show the encryption
 * details
 * Todo: remove
 */
#define UA_CUSTOM_SHOWENCRYPTION_LOGLEVEL 250

/**
 * the number of zerobytes in aes ctr
 */
#define UA_PUBSUB_AES_CTR_COUNTERBLOCK_ZEROBYTESLENGTH 4

/**
 * the number of bytes in the nonce
 */
#define UA_PUBSUB_AES_CTR_NONCELENGTH 4

/**
 * the number of bytes in the nonce
 */
#define UA_PUBSUB_ENCODED_NONCELENGTH_IN_MESSAGE 8

/*
 * Max size of keyTailQueue,for now not really used
 */
#define UA_MAX_KEY_BUFFER 10

/**
 * .. _sks:
 *
 * Security Key Service
 * ======================
 * Work In Progress!
 * Security Key Service(SKS) extenstion for OPC UA enables secure PubSub communication.
 * It manages a the relation ship between securityGroup and keys::
 *         +----------------+   +----------------+
 *         |Security Group 1| ->|Security Group 2| -> ...
 *         +----------------+   +----------------+
 *                 |
 *                 |
 *            +----------+
 *            |KeyStorage|
 *            +----------+
 *                 |
 *             +------+    +------+
 *             | Key1 | -> | Key2 | -> ...
 *             +------+    +------+
 * 
 * SecurityGroup: each writer/readerGroup is bound to a security group when pubSub security is enabled,
 * it defines the seucirty policy, number of keys and keyLifeTime
 * 
 * UA_PubSubSKSKeyStorage: each SecurityGroup has a keyStorage structure. it stores a list of keys and indicators
 * of oldest key, current key and future key
 * 
 * UA_PubSubKeyListItem: a basic item which contains key, keyID and a pointer to the next key
 * 
 * When "addSecurityGroup"(see specification) is called, sks server will create a new keyStorage structure and initialize it with
 * number of keys:allocate space for past keys with keyID=0, followed by currentKey and future keys generated
 * according to security policy where keyID starts from 1. Further more, a time stamp will be recored to calculate timeToNextKeyParameter::
 * 
 * ||Key0||Key0||Key0||----Key1----||Key2||Key3||Key4||Key5||
 * |-maxPastKeyCount-|--CurrentKey--|--maxFutureKeyCount----|
 * 
 * A callback function is also set to update the sturcture regularly according to keyLifeTime,
 * it will generate a new key and delete one old key::
 * 
 * ||Key0||Key0||Key1||----Key2----||Key3||Key4||Key5||Key6||
 * |-maxPastKeyCount-|--CurrentKey--|--maxFutureKeyCount----|
 * 
 * The keyStorage sturcture will be cleaned when "removeSecurityGroup" is called
 * 
 * When "getSecurityKeys"(see specification) is called, sks server will return batch of keys according to the input.
 * Calculation of timeToNextKey:
 * timeToNextKey= keyLifeTime-(CurrentTime-firstKeyGenerationTime)%keyLifeTime
 * example: saying SKS generate key at time 0 with keyLifeTime=10,
 * A publisher come at time 2, and a subscriber come at time 3.
 * Then the timeToNextKey should be 8 and 7 for eachother
 * so they both change to next key at time 10 and synchornized with SKS server
 * 
 * 
 */

/**
 *  Allocates memory for a UA_PubSubSKSKeyStorage. On success, all members are initialized
 * with NULL or 0
 * @param storageForNewPointer pointer to a pointer which points to the new created
 * UA_PubSubSKSKeyStorage on sucess or to NULL on error
 * @return On success: UA_STATUSCODE_GOOD. On error: UA_STATUSCODE_BADOUTOFMEMORY.
 */
UA_StatusCode
UA_PubSubSKSKeyStorage_new(UA_PubSubSKSKeyStorage **storageForNewPointer);

/**
 *  Deletes a key storage. Deletes all members. Removes the key storage from the server
 * @param server server from which to remove the key storage
 * @param keyStorage keyStorage to delete
 */
void
UA_PubSubKeySKSStorage_delete(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage);

/**
 *  Deletes all member of a key storage. Removes the key storage from the server
 * @param server server from which to remove the key storage
 * @param keyStorage keyStorage to delete
 */
void
UA_PubSubKeySKSStorage_deleteMembers(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage);

/**
 * Searches for a key storage with the specified id.
 * If found, UA_PubSubKeySKSStorage_delete is called to delete this storage
 * @param server server from which to remove the key storage
 * @param securityGroupId group id of the storage which should be deleted
 */
void
UA_PubSubKeySKSStorage_deleteSingle(UA_Server *server, const UA_String *securityGroupId);

/**
 * Searches for a key storage with the specified id.
 * @param server server containing the key storage list
 * @param securityGroupId group id of the storage to find
 * @param keyStorage on success and if found, contains the storage
 * @return on success and if found, UA_STATUSCODE_GOOD, otherwise
 * UA_STATUSCODE_BADNOTFOUND
 */
UA_StatusCode
UA_getPubSubKeyStoragebySecurityGroupId(const UA_Server *server,
                                        const UA_String *securityGroupId,
                                        UA_PubSubSKSKeyStorage **keyStorage);

/**
 * Find the security policy on server according to securityPolicyUri
 * @param server publisher / subscriber server
 * @param secrutityPolicyUri the uri for the policy
 * @param policy on success contans a pointer to SeucrityPolicy
 * @return
 * UA_STATUSCODE_BADNOTFOUND if not found
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_SecurityPolicy_findPolicyBySecurityPolicyUri(const UA_Server *server,
                                                const UA_String securityPolicyUri,
                                                UA_SecurityPolicy **policy);
/**
 * InitializeSksConfig, except the pointer to the client (it should be initialized before
 * call this function)
 * @param server A Publisher/Subscriber
 * @param securityGroupId SecurityGroupID of the Reader/WriterGroup
 * @param sksconfig configuration including client,policy and channelcontext
 * @param getSecurityKeysEnabled true if use getSeucurityKeysMethod sksconfig->client MUST be set, false then use setSecurityKeysMethod
 * @return UA_STATUSCODE_GOOD on success
 * error codes of UA_getPubSubKeyStoragebySecurityGroupId, UA_importKeyToChannelContext,
 * channelModule.newContext
 * UA_STATUSCODE_BADOUTOFMEMORY if out of memory
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_initializeSksConfig(UA_Server *server, const UA_String securityGroupId,
                       UA_PubSub_SKSConfig *sksConfig,
                       const UA_Boolean getSecurityKeysEnabled);

/**
 * Remove sksconfig from a server
 * Also deletes the key storage / decrements its reference counter
 * Also deletes keyNonce and channelContext where keys are stored
 * @param server instance
 * @param sksConfig the sks config the remove
 */
void
UA_PubSubKey_deleteSKSConfig(UA_PubSub_SKSConfig *sksConfig, UA_Server *server);

/**
 * Initialize a keyStorageSturcture for a securityGroup
 * this function should be called when adding a reader/writerGroup when it cant
 * find the securityGroup in the server keyStorage list
 * After initialization, the keyStorage is added to the servers pubSubSKSKeyList
 * @param keyStorage The keyStorage to be initialized. It must be created with
 * UA_PubSubSKSKeyStorage_new before calling this function
 * @param server A Publisher/Subscriber
 * @param securityGroupId security group id which is related to the key storage
 * @param securityPolicyUri security policy uri which is related to the key storage
 * @param currentTokenId the id of the first token
 * @param currentKey the first / current key to store
 * @param futureKeys remaining keys to store
 * @param futureKeyCount the number of remaining keys to store
 * @return UA_STATUSCODE_GOOD on success
 * UA_STATUSCODE_BADNOTFOUND if the specified security policy was not found
 * UA_STATUSCODE_BADOUTOFMEMORY if out of memory
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_initializeKeyStorageWithKeys(UA_PubSubSKSKeyStorage *keyStorage, UA_Server *server,
                                const UA_String *securityGroupId,
                                const UA_String *securityPolicyUri,
                                UA_UInt32 currentTokenId, UA_ByteString *currentKey,
                                UA_ByteString *futureKeys, size_t futureKeyCount);

/**
 * Initialize a keyStorageSturcture for a securityGroup
 * this function will try ask SKS for as many keys as possible from currentKey
 * and then calles UA_initializeKeyStorageWithKeys with those keys
 * @param server A Publisher/Subscriber
 * @param securityGroupId the security group which relates to the keyStorage
 * @param sksConfig The sksConfig contining the key storage
 * @return UA_STATUSCODE_GODD on success
 * UA_STATUSCODE_BADNODATA if no keys were received
 * error codes of UA_getSecurityKeys, UA_initializeKeyStorageWithKeys,
 * addMoveToNextKeyCallback
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_initializeKeyStorageByAskingCurrentKey(UA_Server *server,
                                          const UA_String securityGroupId,
                                          UA_PubSub_SKSConfig *sksConfig);
/**
 * Set the initial value of a counter block in the channel contxt (initial vector IV)
 * A counterBlock consists of keyNonce followed by messageNonce and then 4bytes of
 * zero
 * @param policy the security policy
 * @param channelContext the channelContext
 * @param keyNonce keyNonce retrived from SKS
 * @param messageNone messageNonce is the combination of message random number and
 * sequence number
 * @return UA_STATUSCODE_GOOD on success
 * error code of channelModule.setLocalSymIv
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_setCounterBlock(UA_SecurityPolicy *policy, void *channelContext,
                   const UA_ByteString keyNonce, const UA_ByteString messageNonce);

UA_StatusCode
UA_PubSub_subscriberUpdateKeybyKeyID(const UA_UInt32 keyId,
                                     UA_PubSub_SKSConfig *sksConfig);

/**
 * An internal funtion ONLY called by UA_ReaderGroup_subscribeCallback
 * when security is enabled
 * currentNetworkMessage header MUST be decoded before calling this function
 * first, it decode and verify the signature
 * and then decrypt the payload in place
 * @param readerGroup the readerGroup
 * @param buffer buffer containing the message
 * @param currentPosition pointer to the currentPosition in the buffer, moved forward
 * within this function
 * @param currentNetworkMessage the currently decoded network message
 * @param dataToDecryptStart start position for decryption
 * @return UA_STATUSCODE_GOOD on success
 * UA_STATUSCODE_BADNOMATCH if no signature set
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_Subscriber_verifyAndDecryptNetWorkMessageBinary_Internal(
    UA_ReaderGroup *readerGroup, UA_ByteString *buffer, size_t *currentPosition,
    UA_NetworkMessage *currentNetworkMessage, UA_Byte *dataToDecryptStart);

/**
 * Client call getSecurityMethod on a SKS server
 * @param client A Publisher/Subscriber
 * @param securityGroupId SecurityGroupID of the Reader/WriterGroup
 * @param starting TokenId ID of Keys(Tokens),curret Token is requesetd by passing 0
 * @param requestedKeyCount max number of requested keys
 * @param outputsize length of the output array
 * @param getSecurityKeysOutput A pointer to a bytestring array containing one or multiple
 * keys
 * @return UA_STATUSCODE_GOOD on success
 * error codes of GetSecurityKeys fromt he SKS, UA_Client_call,
 * UA_STATUSCODE_BADARGUMENTSMISSING, UA_STATUSCODE_BADTOOMANYARGUMENTS and
 *   UA_STATUSCODE_BADTYPEMISMATCH for unexpected returns from GetSecurityKeys
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_getSecurityKeys(UA_Client *client, const UA_String securityGroupId,
                   const UA_UInt32 startingTokenId, const UA_UInt32 requestedKeyCount,
                   UA_Variant **getSecurityKeysOutput);

/** 
 * A callback function, try move currentKey to next one
 * adds a timed callback to the server for UA_moveToNextKeyCallback
 * @param server service instance
 * @param callBackFunction pointer to the callback function
 * @param functionParameter parameter provided to the callback when called
 * @param timeInMS in milliseconds until the callback is called 
 * @param callBackId pointer to the callback id
 * should be called next
 * @return UA_STATUSCODE_GOOD on success
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
addMoveToNextKeyCallback(UA_Server *server, UA_ServerCallback callBackFuntion,void* functionParameter,
                         UA_Double timeInMS, UA_UInt64* callBackId);

/**
 * Import main keys in to Signing keys, signing key and nonce into channelContext stored
 * in SKSCongfig. First get keyLength for each key according to policy, then divide main
 * key into three parts and set them for encryption and signing
 * @param key the main key received from SKS or stored in keyStorage
 * @param sksConfig container of channelcontext and policy
 * @return UA_STATUSCODE_GOOD on success
 * error codes of setLocalSymSigningKey and setLocalSymEncryptingKey
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input or when received key nonce
 * has invalid length
 */
UA_StatusCode
UA_importKeyToChannelContext(const UA_ByteString key, UA_PubSub_SKSConfig *sksConfig);

/**
 * Todo:Duplicate with UA_PubSub_findPubSubKeyByKeyID??
 * Get key from a keyList according to KeyID
 * @param keyId the key id to look for
 * @param firstItem starting point for search
 * @param keyList on success contains the keylist
 * @return UA_STATUSCODE_GOOD
 * UA_STATUSCODE_BADNOTFOUND if not found
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_PubSub_getKeyByKeyID(const UA_UInt32 keyId, UA_PubSubKeyListItem *firstItem,
                        UA_PubSubKeyListItem **keyList);

/** Update a keyStorage structure
 * There are 2 ways to use this function:
 * 1.Set currentKeyID and update ONLY currentKey
 * 2.set currentKeyID to 0 then it will try update key list by removing old keys
 * @param keyStorage the structure which should be updated
 * @param startingKeyID the firstKeyID of incoming key array
 * @param keys incoming Key
 * @param keySize size of incoming key array
 * @param currentKeyID set this value to change currentKeyID and force the update,
 * @return UA_STATUSCODE_GOOD
 * UA_STATUSCODE_BADINVALIDARGUMENT if received invalid keys
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_PubSub_updateKeyStorage(UA_PubSubSKSKeyStorage *keyStorage,
                           const UA_UInt32 startingKeyID, const UA_ByteString *keys,
                           const size_t keySize, UA_UInt32 currentKeyID);

/** A callback function, it will move current key to next one,
 * if current key is alreay the last item (newest key),
 * it will try ask for future keys from SKS, update the keystorage list 
 * and tell WriterGroup/ReaderGroup to use new current key.
 * it also set timer for next "UA_moveToNextKeyCallbackPull" according to
 * timeToNextKey(if just retrived from SKS) or keyLifeTime(if just move to next key)
 * 
 * @param server service instance
 * @param sksConfig sksConfig related to the callback
 */
void
UA_moveToNextKeyCallbackPull(UA_Server *server, UA_PubSub_SKSConfig *sksConfig);

/** 
 * A callback function when SKS is not available
 * just move key to next key and set a timer for next callback
 */
void
UA_moveToNextKeyCallbackPush(UA_Server *server, UA_PubSubSKSKeyStorage *keyStorage);

/**
 * An internal funtion ONLY called by SendNetworkMessage
 * it sets the securityHeader including messageNonce and set
 * the counterBlock used for encryption
 * it also set a buffer for signature so the binary size can be calculated correctly
 * @param writerGroup writerGroup used for this message
 * @param networkMessage the network message to encode
 * @return UA_STATUSCODE_GOOD on success
 * UA_STATUSCODE_BADOUTOFMEMORY when out of memory
 * error codes from UA_Int32_encodeBinary
 * UA_STATUSCODE_BADNOMATCH if signing failed
 * UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_Publisher_encodeSecurityHeaderBinary_Internal(UA_WriterGroup *writerGroup,
                                                 UA_NetworkMessage *networkMessage);

/*Here is a rough pic of the network message for better understanding, more detials see
 * Fig27 in OPCUA Part14, chapter 7.2.2.2.1*/
/*
   ||=====MessageHeaders=====||====Payload and SecurityFooter====||====Signature====||
   ||===================DataToSign===============================||
                             ||========DataToEncrypt============ ||

Thus, dataToEncryptEnd and dataToSignEnd can be calculated by using
bufEnd-signatureBinaryLength. However, since the some of  messageHeaders can be omitted,
it can only be retrived after encoding/decoding the headers
*/

/**
 * An internal funtion ONLY called by SendNetworkMessage when security is enabled
 * first, it encode the data as noneSecurity
 * and then make encryption and signing in place
 * @param writerGroup writerGroup used for this message
 * @param networkMessage the network message to encode
 * @param buf buffer used for encoding
 * @return UA_STATUSCODE_GOOD on success
 * error codes of UA_ByteString_encodeBinary, UA_NetworkMessage_encodeBinary,
 * encryptionAlgorithm.encrypt UA_STATUSCODE_BADINVALIDARGUMENT for NULL pointers in input
 */
UA_StatusCode
UA_Publisher_signAndEncryptNetWorkMessageBinary_Internal(
    UA_WriterGroup *writerGroup, UA_NetworkMessage *networkMessage, UA_ByteString *buf);

#endif /* UA_ENABLE_PUBSUB_SECURITY */

_UA_END_DECLS

#endif /* UA_PUBSUB_SKS_H_ */
