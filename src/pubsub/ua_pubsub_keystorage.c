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

#endif
