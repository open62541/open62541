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

    /* Stop and delete the EventLoop */
    UA_EventLoop *el = config->eventLoop;
    if(el && !config->externalEventLoop) {
        if(el->state != UA_EVENTLOOPSTATE_FRESH &&
           el->state != UA_EVENTLOOPSTATE_STOPPED) {
            el->stop(el);
            while(el->state != UA_EVENTLOOPSTATE_STOPPED) {
                el->run(el, 100);
            }
        }
        el->free(el);
        config->eventLoop = NULL;
    }

    /* Networking */
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = NULL;
    config->serverUrlsSize = 0;

    /* Security Policies */
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
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(config->pubSubConfig.securityPolicies != NULL) {
        for(size_t i = 0; i < config->pubSubConfig.securityPoliciesSize; i++) {
            config->pubSubConfig.securityPolicies[i].clear(&config->pubSubConfig.securityPolicies[i]);
        }
        UA_free(config->pubSubConfig.securityPolicies);
        config->pubSubConfig.securityPolicies = NULL;
        config->pubSubConfig.securityPoliciesSize = 0;
    }
#endif
#endif /* UA_ENABLE_PUBSUB */
}

#ifdef UA_ENABLE_PUBSUB
/* Add a pubsubTransportLayer to the configuration. Memory is reallocated on
 * demand. */
UA_StatusCode
UA_ServerConfig_addPubSubTransportLayer(UA_ServerConfig *config,
                                        UA_PubSubTransportLayer pubsubTransportLayer) {
    UA_PubSubTransportLayer *tmpLayers = (UA_PubSubTransportLayer*)
        UA_realloc(config->pubSubConfig.transportLayers,
                   sizeof(UA_PubSubTransportLayer) *
                   (config->pubSubConfig.transportLayersSize + 1));
    if(tmpLayers == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    config->pubSubConfig.transportLayers = tmpLayers;
    config->pubSubConfig.transportLayers[config->pubSubConfig.transportLayersSize] = pubsubTransportLayer;
    config->pubSubConfig.transportLayersSize++;
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_PUBSUB */
