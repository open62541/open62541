/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server_config.h>

void
UA_ServerConfig_clean(UA_ServerConfig *config) {
    if(!config)
        return;

    /* Server Description */
    UA_BuildInfo_deleteMembers(&config->buildInfo);
    UA_ApplicationDescription_deleteMembers(&config->applicationDescription);
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_MdnsDiscoveryConfiguration_clear(&config->discovery.mdns);
#endif

    /* Custom DataTypes */
    /* nothing to do */

    /* Networking */
    for(size_t i = 0; i < config->networkLayersSize; ++i)
        config->networkLayers[i].deleteMembers(&config->networkLayers[i]);
    UA_free(config->networkLayers);
    config->networkLayers = NULL;
    config->networkLayersSize = 0;
    UA_String_deleteMembers(&config->customHostname);
    config->customHostname = UA_STRING_NULL;

    for(size_t i = 0; i < config->securityPoliciesSize; ++i) {
        UA_SecurityPolicy *policy = &config->securityPolicies[i];
        policy->deleteMembers(policy);
    }
    UA_free(config->securityPolicies);
    config->securityPolicies = NULL;
    config->securityPoliciesSize = 0;

    for(size_t i = 0; i < config->endpointsSize; ++i)
        UA_EndpointDescription_deleteMembers(&config->endpoints[i]);

    UA_free(config->endpoints);
    config->endpoints = NULL;
    config->endpointsSize = 0;

    /* Certificate Validation */
    if(config->certificateVerification.deleteMembers)
        config->certificateVerification.deleteMembers(&config->certificateVerification);

    /* Access Control */
    if(config->accessControl.deleteMembers)
        config->accessControl.deleteMembers(&config->accessControl);

    /* Historical data */
#ifdef UA_ENABLE_HISTORIZING
    if(config->historyDatabase.deleteMembers)
        config->historyDatabase.deleteMembers(&config->historyDatabase);
#endif

    /* Logger */
    if(config->logger.clear)
        config->logger.clear(config->logger.context);
}

void
UA_ServerConfig_setCustomHostname(UA_ServerConfig *config,
                                  const UA_String customHostname) {
    if(!config)
        return;
    UA_String_deleteMembers(&config->customHostname);
    UA_String_copy(&customHostname, &config->customHostname);
}

#ifdef UA_ENABLE_PUBSUB
/* Add a pubsubTransportLayer to the configuration. Memory is reallocated on
 * demand. */
UA_StatusCode
UA_ServerConfig_addPubSubTransportLayer(UA_ServerConfig *config,
        UA_PubSubTransportLayer *pubsubTransportLayer) {

    if(config->pubsubTransportLayersSize == 0) {
        config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
                UA_malloc(sizeof(UA_PubSubTransportLayer));
    } else {
        config->pubsubTransportLayers = (UA_PubSubTransportLayer*)
                UA_realloc(config->pubsubTransportLayers,
                sizeof(UA_PubSubTransportLayer) * (config->pubsubTransportLayersSize + 1));
    }

    if(config->pubsubTransportLayers == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    memcpy(&config->pubsubTransportLayers[config->pubsubTransportLayersSize],
            pubsubTransportLayer, sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayersSize++;

    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_PUBSUB */
