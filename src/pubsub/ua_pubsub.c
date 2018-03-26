/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_pubsub.h"
#include "ua_pubsub_manager.h"

/**********************************************/
/*               Connection                   */
/**********************************************/
UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src, UA_PubSubConnectionConfig *dst) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    retVal |= UA_Variant_copy(&src->address, &dst->address);
    retVal |= UA_String_copy(&src->transportProfileUri, &dst->transportProfileUri);
    retVal |= UA_Variant_copy(&src->connectionTransportSettings, &dst->connectionTransportSettings);
    if(src->connectionPropertiesSize > 0){
        dst->connectionProperties = (UA_KeyValuePair *) UA_calloc(src->connectionPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->connectionProperties){
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(size_t i = 0; i < src->connectionPropertiesSize; i++){
            retVal |= UA_QualifiedName_copy(&src->connectionProperties[i].key, &dst->connectionProperties[i].key);
            retVal |= UA_Variant_copy(&src->connectionProperties[i].value, &dst->connectionProperties[i].value);
        }
    }
    return retVal;
}

/**
 * Get the current config of the Connection.
 *
 * @param server
 * @param connectionIdentifier
 * @param config
 * @return UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_PubSubConnection_getConfig(UA_Server *server, UA_NodeId connectionIdentifier,
                              UA_PubSubConnectionConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubConnection *currentPubSubConnection = UA_PubSubManager_findConnectionbyId(server, connectionIdentifier);
    if(!currentPubSubConnection)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_PubSubConnectionConfig tmpPubSubConnectionConfig;
    //deep copy of the actual config
    UA_PubSubConnectionConfig_copy(currentPubSubConnection->config, &tmpPubSubConnectionConfig);
    *config = tmpPubSubConnectionConfig;
    return UA_STATUSCODE_GOOD;
}

void
UA_PubSubConnectionConfig_deleteMembers(UA_PubSubConnectionConfig *connectionConfig) {
    UA_String_deleteMembers(&connectionConfig->name);
    UA_String_deleteMembers(&connectionConfig->transportProfileUri);
    UA_Variant_deleteMembers(&connectionConfig->connectionTransportSettings);
    UA_Variant_deleteMembers(&connectionConfig->address);
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        UA_QualifiedName_deleteMembers(&connectionConfig->connectionProperties[i].key);
        UA_Variant_deleteMembers(&connectionConfig->connectionProperties[i].value);
    }
    UA_free(connectionConfig->connectionProperties);
}

void
UA_PubSubConnection_delete(UA_PubSubConnection *connection) {
    //delete connection config
    UA_PubSubConnectionConfig_deleteMembers(connection->config);
    UA_NodeId_deleteMembers(&connection->identifier);
    if(connection->channel){
        connection->channel->close(connection->channel);
    }
    UA_free(connection->config);
}
