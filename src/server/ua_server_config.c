/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include <open62541/server.h>

void
UA_ServerConfig_clean(UA_ServerConfig *config) {
    if(!config)
        return;

    /* Server Description */
    UA_BuildInfo_clear(&config->buildInfo);
    UA_ApplicationDescription_clear(&config->applicationDescription);
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_MdnsDiscoveryConfiguration_clear(&config->mdnsConfig);
    UA_String_clear(&config->mdnsInterfaceIP);
# if !defined(UA_HAS_GETIFADDR)
    if (config->mdnsIpAddressListSize) {
        UA_free(config->mdnsIpAddressList);
    }
# endif
#endif

    /* Custom DataTypes */
    /* nothing to do */

    /* Networking */
    for(size_t i = 0; i < config->networkLayersSize; ++i)
        config->networkLayers[i].clear(&config->networkLayers[i]);
    UA_free(config->networkLayers);
    config->networkLayers = NULL;
    config->networkLayersSize = 0;
    UA_String_clear(&config->customHostname);
    config->customHostname = UA_STRING_NULL;

    for(size_t i = 0; i < config->securityPoliciesSize; ++i) {
        UA_SecurityPolicy *policy = &config->securityPolicies[i];
        policy->clear(policy);
    }
    UA_free(config->securityPolicies);
    config->securityPolicies = NULL;
    config->securityPoliciesSize = 0;

    for(size_t i = 0; i < config->endpointsSize; ++i)
        UA_EndpointDescription_clear(&config->endpoints[i]);

    UA_free(config->endpoints);
    config->endpoints = NULL;
    config->endpointsSize = 0;

    /* Nodestore */
    if(config->nodestore.context && config->nodestore.clear) {
        config->nodestore.clear(config->nodestore.context);
        config->nodestore.context = NULL;
    }

    /* Certificate Validation */
    if(config->certificateVerification.clear)
        config->certificateVerification.clear(&config->certificateVerification);

    /* Access Control */
    if(config->accessControl.clear)
        config->accessControl.clear(&config->accessControl);

    /* Historical data */
#ifdef UA_ENABLE_HISTORIZING
    if(config->historyDatabase.clear)
        config->historyDatabase.clear(&config->historyDatabase);
#endif

    /* Logger */
    if(config->logger.clear)
        config->logger.clear(config->logger.context);
    config->logger.log = NULL;
    config->logger.clear = NULL;

#ifdef UA_ENABLE_PUBSUB
    /* PubSub configuration */
    if (config->pubsubConfiguration != NULL) {
        UA_free(config->pubsubConfiguration);
        config->pubsubConfiguration = 0;
    }
#endif /* UA_ENABLE_PUBSUB */
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
