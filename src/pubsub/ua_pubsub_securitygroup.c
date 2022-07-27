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
    UA_NodeId_clear(&securityGroup->securityGroupFolderId);
    UA_NodeId_clear(&securityGroup->securityGroupNodeId);
}

void
UA_SecurityGroup_delete(UA_SecurityGroup *securityGroup) {
    UA_SecurityGroup_clear(securityGroup);
    UA_free(securityGroup);
}

UA_StatusCode
removeSecurityGroup(UA_Server *server, UA_NodeId securityGroup) {
    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(server, securityGroup);
    if(!sg)
        return UA_STATUSCODE_BADNOTFOUND;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_removeSecurityGroupRepresentation(server, sg);
#endif
    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.securityGroups, sg, listEntry);
    server->pubSubManager.securityGroupsSize--;
    if(sg->callbackId > 0)
        removeCallback(server, sg->callbackId);

    UA_PubSubKeyStorage_removeKeyStorage(server, sg->keyStorage);

    UA_SecurityGroup_delete(sg);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeSecurityGroup(UA_Server *server, const UA_NodeId securityGroup) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = removeSecurityGroup(server, securityGroup);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

#endif
