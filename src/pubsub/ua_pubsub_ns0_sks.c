/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL

static UA_Boolean
isValidParentNode(UA_Server *server, UA_NodeId parentId) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Boolean retval = true;
    const UA_Node *parentNodeType;
    const UA_NodeId parentNodeTypeId = UA_NS0ID(SECURITYGROUPFOLDERTYPE);
    const UA_Node *parentNode = UA_NODESTORE_GET(server, &parentId);

    if(parentNode) {
        parentNodeType = getNodeType(server, &parentNode->head);
        if(parentNodeType) {
            retval = UA_NodeId_equal(&parentNodeType->head.nodeId, &parentNodeTypeId);
            UA_NODESTORE_RELEASE(server, parentNodeType);
        }
        UA_NODESTORE_RELEASE(server, parentNode);
    }
    return retval;
}

static UA_StatusCode
updateSecurityGroupProperties(UA_Server *server, UA_NodeId *securityGroupNodeId,
                              UA_SecurityGroupConfig *config) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &config->securityGroupName, &UA_TYPES[UA_TYPES_STRING]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "SecurityGroupId"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /*AddCallbackValueSource*/
    UA_Variant_setScalar(&value, &config->securityPolicyUri, &UA_TYPES[UA_TYPES_STRING]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "SecurityPolicyUri"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_setScalar(&value, &config->keyLifeTime, &UA_TYPES[UA_TYPES_DURATION]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "KeyLifetime"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_setScalar(&value, &config->maxFutureKeyCount, &UA_TYPES[UA_TYPES_UINT32]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "MaxFutureKeyCount"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_setScalar(&value, &config->maxPastKeyCount, &UA_TYPES[UA_TYPES_UINT32]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "MaxPastKeyCount"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

UA_StatusCode
addSecurityGroupRepresentation(UA_Server *server, UA_SecurityGroup *securityGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_BAD;

    UA_SecurityGroupConfig *securityGroupConfig = &securityGroup->config;
    if(!isValidParentNode(server, securityGroup->securityGroupFolderId))
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;

    if(securityGroupConfig->securityGroupName.length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_QualifiedName browseName;
    UA_QualifiedName_init(&browseName);
    browseName.name = securityGroupConfig->securityGroupName;

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName.text = securityGroupConfig->securityGroupName;
    UA_NodeId refType = UA_NS0ID(HASCOMPONENT);
    UA_NodeId nodeType = UA_NS0ID(SECURITYGROUPTYPE);
    retval = addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NULL,
                     securityGroup->securityGroupFolderId, refType,
                     browseName, nodeType, &object_attr,
                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
                     &securityGroup->securityGroupNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Add SecurityGroup failed with error: %s.",
                     UA_StatusCode_name(retval));
        return retval;
    }

    retval = updateSecurityGroupProperties(server,
                                           &securityGroup->securityGroupNodeId,
                                           securityGroupConfig);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Add SecurityGroup failed with error: %s.",
                     UA_StatusCode_name(retval));
        deleteNode(server, securityGroup->securityGroupNodeId, true);
    }
    return retval;
}

/**
 * @note The user credentials and permissions are checked in the AccessControl plugin
 * before this callback is executed.
 */
static UA_StatusCode
setSecurityKeysAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Validate the arguments */
    if(!server || !input)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(inputSize < 7)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 7 || outputSize > 0)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    /* Check whether the channel is encrypted according to specification */
    UA_Session *session = getSessionById(server, sessionId);
    if(!session || !session->channel)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(session->channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;

    /* Check for types */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) || /*SecurityGroupId*/
        !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_STRING]) || /*SecurityPolicyUri*/
        !UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_INTEGERID]) || /*CurrentTokenId*/
        !UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*CurrentKey*/
        !UA_Variant_hasArrayType(&input[4], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*FutureKeys*/
        (!UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_DURATION]) &&
        !UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_DOUBLE])) || /*TimeToNextKey*/
        (!UA_Variant_hasScalarType(&input[6], &UA_TYPES[UA_TYPES_DURATION]) &&
        !UA_Variant_hasScalarType(&input[6], &UA_TYPES[UA_TYPES_DOUBLE]))) /*TimeToNextKey*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_Duration callbackTime;
    UA_String *securityGroupId = (UA_String *)input[0].data;
    UA_String *securityPolicyUri = (UA_String *)input[1].data;
    UA_UInt32 currentKeyId = *(UA_UInt32 *)input[2].data;
    UA_ByteString *currentKey = (UA_ByteString *)input[3].data;
    UA_ByteString *futureKeys = (UA_ByteString *)input[4].data;
    size_t futureKeySize = input[4].arrayLength;
    UA_Duration msTimeToNextKey = *(UA_Duration *)input[5].data;
    UA_Duration msKeyLifeTime = *(UA_Duration *)input[6].data;

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubKeyStorage *ks =
        UA_PubSubKeyStorage_find(psm, *securityGroupId);
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!UA_String_equal(securityPolicyUri, &ks->policy->policyUri))
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_PubSubKeyListItem *current = UA_PubSubKeyStorage_getKeyByKeyId(ks, currentKeyId);
    if(!current) {
        UA_PubSubKeyStorage_clearKeyList(ks);
        retval |= (UA_PubSubKeyStorage_push(ks, currentKey, currentKeyId)) ?
            UA_STATUSCODE_GOOD : UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_PubSubKeyStorage_setCurrentKey(ks, currentKeyId);
    retval |= UA_PubSubKeyStorage_addSecurityKeys(ks, futureKeySize,
                                                  futureKeys, currentKeyId);
    ks->keyLifeTime = msKeyLifeTime;
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_PubSubKeyStorage_activateKeyToChannelContext(psm, UA_NODEID_NULL,
                                                             ks->securityGroupID);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(
            server->config.logging, UA_LOGCATEGORY_SERVER,
            "Failed to import Symmetric Keys into PubSub Channel Context with %s \n",
            UA_StatusCode_name(retval));
        return retval;
    }

    callbackTime = msKeyLifeTime;
    if(msTimeToNextKey > 0)
        callbackTime = msTimeToNextKey;

    /* Move to setSecurityKeysAction */
    return UA_PubSubKeyStorage_addKeyRolloverCallback(
        psm, ks, (UA_Callback)UA_PubSubKeyStorage_keyRolloverCallback,
        callbackTime, &ks->callBackId);
}

static UA_StatusCode
getSecurityKeysAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Validate the arguments */
    if(!server || !input)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(inputSize < 3 || outputSize < 5)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 3 || outputSize > 5)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    /* Check whether the channel is encrypted according to specification */
    UA_Session *session = getSessionById(server, sessionId);
    if(!session || !session->channel)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(session->channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;

    /* Check for types */
    if(!UA_Variant_hasScalarType(&input[0],
                                 &UA_TYPES[UA_TYPES_STRING]) || /*SecurityGroupId*/
       !UA_Variant_hasScalarType(&input[1],
                                 &UA_TYPES[UA_TYPES_INTEGERID]) || /*StartingTokenId*/
       !UA_Variant_hasScalarType(&input[2],
                                 &UA_TYPES[UA_TYPES_UINT32])) /*RequestedKeyCount*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_UInt32 currentKeyCount = 1;

    /* Input */
    UA_String *securityGroupId = (UA_String *)input[0].data;
    UA_UInt32 startingTokenId = *(UA_UInt32 *)input[1].data;
    UA_UInt32 requestedKeyCount = *(UA_UInt32 *)input[2].data;

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_find(psm, *securityGroupId);
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_Boolean executable = false;
    UA_SecurityGroup *sg = UA_SecurityGroup_findByName(psm, *securityGroupId);
    void *sgNodeCtx;
    getNodeContext(server, sg->securityGroupNodeId, (void **)&sgNodeCtx);
    executable = server->config.accessControl.getUserExecutableOnObject(
        server, &server->config.accessControl, sessionId, sessionContext, methodId,
        methodContext, &sg->securityGroupNodeId, sgNodeCtx);

    if(!executable)
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    /* If the caller requests a number larger than the Security Key Service
     * permits, then the SKS shall return the maximum it allows.*/
    if(requestedKeyCount > sg->config.maxFutureKeyCount)
        requestedKeyCount =(UA_UInt32) sg->keyStorage->keyListSize;
    else
        requestedKeyCount = requestedKeyCount + currentKeyCount; /* Add Current keyCount */

    /* The current token is requested by passing 0. */
    UA_PubSubKeyListItem *startingItem = NULL;
    if(startingTokenId == 0) {
        /* currentItem is always set by the server when a security group is added */
        UA_assert(sg->keyStorage->currentItem != NULL);
        startingItem = sg->keyStorage->currentItem;
    } else {
        startingItem = UA_PubSubKeyStorage_getKeyByKeyId(sg->keyStorage, startingTokenId);
        /* If the StartingTokenId is unknown, the oldest (firstItem) available
         * tokens are returned. */
        if(!startingItem)
            startingItem = TAILQ_FIRST(&sg->keyStorage->keyList);
    }

    /* SecurityPolicyUri */
    retval = UA_Variant_setScalarCopy(&output[0], &(sg->keyStorage->policy->policyUri),
                         &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* FirstTokenId */
    retval = UA_Variant_setScalarCopy(&output[1], &startingItem->keyID,
                                      &UA_TYPES[UA_TYPES_INTEGERID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* TimeToNextKey */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime baseTime = sg->baseTime;
    UA_DateTime currentTime = el->dateTime_nowMonotonic(el);
    UA_Duration interval = sg->config.keyLifeTime;
    UA_Duration timeToNextKey =
        (UA_Duration)((currentTime - baseTime) / UA_DATETIME_MSEC);
    timeToNextKey = interval - timeToNextKey;
    retval = UA_Variant_setScalarCopy(&output[3], &timeToNextKey,
                                      &UA_TYPES[UA_TYPES_DURATION]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* KeyLifeTime */
    retval = UA_Variant_setScalarCopy(&output[4], &sg->config.keyLifeTime,
                         &UA_TYPES[UA_TYPES_DURATION]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Keys */
    UA_PubSubKeyListItem *iterator = startingItem;
    output[2].data = (UA_ByteString *)
        UA_calloc(requestedKeyCount, startingItem->key.length);
    if(!output[2].data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_ByteString *requestedKeys = (UA_ByteString *)output[2].data;
    UA_UInt32 retkeyCount = 0;
    for(size_t i = 0; i < requestedKeyCount; i++) {
        UA_ByteString_copy(&iterator->key, &requestedKeys[i]);
        ++retkeyCount;
        iterator = TAILQ_NEXT(iterator, keyListEntry);
        if(!iterator) {
            requestedKeyCount = retkeyCount;
            break;
        }
    }

    UA_Variant_setArray(&output[2], requestedKeys, requestedKeyCount,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
    return retval;
}

UA_StatusCode
initPubSubNS0_SKS(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    
    if(server->config.pubSubConfig.enableInformationModelMethods) {
        /* Set SKS method callbacks */
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_SETSECURITYKEYS), setSecurityKeysAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_GETSECURITYKEYS), getSecurityKeysAction);
    }

    return retVal;
}

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL*/
#endif /* UA_ENABLE_PUBSUB_SKS */
