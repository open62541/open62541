/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Daniel Feist, Precitec GmbH & Co. KG
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 */

#include "ua_config_default.h"
#include "ua_client_config.h"
#include "ua_log_stdout.h"
#include "ua_network_tcp.h"
#include "ua_accesscontrol_default.h"
#include "ua_pki_certificate.h"
#include "ua_nodestore_default.h"
#include "ua_securitypolicies.h"
#include "ua_plugin_securitypolicy.h"

/* Struct initialization works across ANSI C/C99/C++ if it is done when the
 * variable is first declared. Assigning values to existing structs is
 * heterogeneous across the three. */
static UA_INLINE UA_UInt32Range
UA_UINT32RANGE(UA_UInt32 min, UA_UInt32 max) {
    UA_UInt32Range range = {min, max};
    return range;
}

static UA_INLINE UA_DurationRange
UA_DURATIONRANGE(UA_Duration min, UA_Duration max) {
    UA_DurationRange range = {min, max};
    return range;
}

/*******************************/
/* Default Connection Settings */
/*******************************/

const UA_ConnectionConfig UA_ConnectionConfig_default = {
    0, /* .protocolVersion */
    65535, /* .sendBufferSize, 64k per chunk */
    65535, /* .recvBufferSize, 64k per chunk */
    0, /* .maxMessageSize, 0 -> unlimited */
    0 /* .maxChunkCount, 0 -> unlimited */
};

/***************************/
/* Default Server Settings */
/***************************/

#define MANUFACTURER_NAME "open62541"
#define PRODUCT_NAME "open62541 OPC UA Server"
#define PRODUCT_URI "http://open62541.org"
#define APPLICATION_NAME "open62541-based OPC UA Application"
#define APPLICATION_URI "urn:unconfigured:application"

#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

static UA_StatusCode
createEndpoint(UA_ServerConfig *conf, UA_EndpointDescription *endpoint,
               const UA_SecurityPolicy *securityPolicy,
               UA_MessageSecurityMode securityMode) {
    UA_EndpointDescription_init(endpoint);

    endpoint->securityMode = securityMode;
    UA_String_copy(&securityPolicy->policyUri, &endpoint->securityPolicyUri);
    endpoint->transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    /* Enable all login mechanisms from the access control plugin  */
    UA_StatusCode retval = UA_Array_copy(conf->accessControl.userTokenPolicies,
                                         conf->accessControl.userTokenPoliciesSize,
                                         (void **)&endpoint->userIdentityTokens,
                                         &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    endpoint->userIdentityTokensSize =
        conf->accessControl.userTokenPoliciesSize;

    UA_String_copy(&securityPolicy->localCertificate, &endpoint->serverCertificate);
    UA_ApplicationDescription_copy(&conf->applicationDescription,
                                   &endpoint->server);

    return UA_STATUSCODE_GOOD;
}

void
UA_ServerConfig_set_customHostname(UA_ServerConfig *config, const UA_String customHostname) {
    if(!config)
        return;
    UA_String_deleteMembers(&config->customHostname);
    UA_String_copy(&customHostname, &config->customHostname);
}

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

static UA_ServerConfig *
createDefaultConfig(void) {
    UA_ServerConfig *conf = (UA_ServerConfig *)UA_malloc(sizeof(UA_ServerConfig));
    if(!conf)
        return NULL;

    /* Zero out.. All members have a valid initial value */
    memset(conf, 0, sizeof(UA_ServerConfig));

    /* --> Start setting the default static config <-- */
    conf->nThreads = 1;
    conf->logger = UA_Log_Stdout_;

    /* Server Description */
    conf->buildInfo.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    conf->buildInfo.manufacturerName = UA_STRING_ALLOC(MANUFACTURER_NAME);
    conf->buildInfo.productName = UA_STRING_ALLOC(PRODUCT_NAME);
    conf->buildInfo.softwareVersion =
        UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));
    #ifdef UA_PACK_DEBIAN
    conf->buildInfo.buildNumber = UA_STRING_ALLOC("deb");
	#else
    conf->buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);
	#endif
    conf->buildInfo.buildDate = 0;

    conf->applicationDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);
    conf->applicationDescription.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    conf->applicationDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("en", APPLICATION_NAME);
    conf->applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;
    /* conf->applicationDescription.gatewayServerUri = UA_STRING_NULL; */
    /* conf->applicationDescription.discoveryProfileUri = UA_STRING_NULL; */
    /* conf->applicationDescription.discoveryUrlsSize = 0; */
    /* conf->applicationDescription.discoveryUrls = NULL; */

#ifdef UA_ENABLE_DISCOVERY
    /* conf->mdnsServerName = UA_STRING_NULL; */
    /* conf->serverCapabilitiesSize = 0; */
    /* conf->serverCapabilities = NULL; */
#endif

    /* Custom DataTypes */
    /* conf->customDataTypesSize = 0; */
    /* conf->customDataTypes = NULL; */

    /* Networking */
    /* conf->networkLayersSize = 0; */
    /* conf->networkLayers = NULL; */
    /* conf->customHostname = UA_STRING_NULL; */

    /* Endpoints */
    /* conf->endpoints = {0, NULL}; */

    /* Certificate Verification that accepts every certificate. Can be
     * overwritten when the policy is specialized. */
    UA_CertificateVerification_AcceptAll(&conf->certificateVerification);

    /* Global Node Lifecycle */
    conf->nodeLifecycle.constructor = NULL;
    conf->nodeLifecycle.destructor = NULL;

    if (UA_AccessControl_default(&conf->accessControl, true, usernamePasswordsSize,
    		usernamePasswords) != UA_STATUSCODE_GOOD) {
    	UA_ServerConfig_delete(conf);
    	return NULL;
    }

    /* Relax constraints for the InformationModel */
    conf->relaxEmptyValueConstraint = true; /* Allow empty values */

    /* Limits for SecureChannels */
    conf->maxSecureChannels = 40;
    conf->maxSecurityTokenLifetime = 10 * 60 * 1000; /* 10 minutes */

    /* Limits for Sessions */
    conf->maxSessions = 100;
    conf->maxSessionTimeout = 60.0 * 60.0 * 1000.0; /* 1h */

    /* Limits for Subscriptions */
    conf->publishingIntervalLimits = UA_DURATIONRANGE(100.0, 3600.0 * 1000.0);
    conf->lifeTimeCountLimits = UA_UINT32RANGE(3, 15000);
    conf->keepAliveCountLimits = UA_UINT32RANGE(1, 100);
    conf->maxNotificationsPerPublish = 1000;
    conf->maxRetransmissionQueueSize = 0; /* unlimited */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    conf->maxEventsPerNode = 0; /* unlimited */
#endif

    /* Limits for MonitoredItems */
    conf->samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
    conf->queueSizeLimits = UA_UINT32RANGE(1, 100);

#ifdef UA_ENABLE_DISCOVERY
    conf->discoveryCleanupTimeout = 60 * 60;
#endif

#ifdef UA_ENABLE_HISTORIZING
    /* conf->accessHistoryDataCapability = UA_FALSE; */
    /* conf->maxReturnDataValues = 0; */

    /* conf->accessHistoryEventsCapability = UA_FALSE; */
    /* conf->maxReturnEventValues = 0; */

    /* conf->insertDataCapability = UA_FALSE; */
    /* conf->insertEventCapability = UA_FALSE; */
    /* conf->insertAnnotationsCapability = UA_FALSE; */

    /* conf->replaceDataCapability = UA_FALSE; */
    /* conf->replaceEventCapability = UA_FALSE; */

    /* conf->updateDataCapability = UA_FALSE; */
    /* conf->updateEventCapability = UA_FALSE; */

    /* conf->deleteRawCapability = UA_FALSE; */
    /* conf->deleteEventCapability = UA_FALSE; */
    /* conf->deleteAtTimeDataCapability = UA_FALSE; */
#endif

    /* --> Finish setting the default static config <-- */

    return conf;
}

static UA_StatusCode
addDefaultNetworkLayers(UA_ServerConfig *conf, UA_UInt16 portNumber, UA_UInt32 sendBufferSize, UA_UInt32 recvBufferSize) {
    /* Add a network layer */
    conf->networkLayers = (UA_ServerNetworkLayer *)
        UA_malloc(sizeof(UA_ServerNetworkLayer));
    if(!conf->networkLayers)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_ConnectionConfig config = UA_ConnectionConfig_default;
    if (sendBufferSize > 0)
        config.sendBufferSize = sendBufferSize;
    if (recvBufferSize > 0)
        config.recvBufferSize = recvBufferSize;

    conf->networkLayers[0] =
        UA_ServerNetworkLayerTCP(config, portNumber, &conf->logger);
    if (!conf->networkLayers[0].handle)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    conf->networkLayersSize = 1;

    return UA_STATUSCODE_GOOD;
}

UA_ServerConfig *
UA_ServerConfig_new_customBuffer(UA_UInt16 portNumber,
                                 const UA_ByteString *certificate,
                                 UA_UInt32 sendBufferSize,
                                 UA_UInt32 recvBufferSize) {
    UA_ServerConfig *conf = createDefaultConfig();
    if (!conf) {
        return NULL;
    }

    UA_StatusCode retval = UA_Nodestore_default_new(&conf->nodestore);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(addDefaultNetworkLayers(conf, portNumber, sendBufferSize, recvBufferSize) != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Allocate the SecurityPolicies */
    conf->securityPolicies = (UA_SecurityPolicy *)UA_malloc(sizeof(UA_SecurityPolicy));
    if(!conf->securityPolicies) {
       UA_ServerConfig_delete(conf);
       return NULL;
    }
    conf->securityPoliciesSize = 1;

    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    retval =
        UA_SecurityPolicy_None(&conf->securityPolicies[0], NULL, localCertificate, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Allocate the endpoint */
    conf->endpoints = (UA_EndpointDescription *)UA_malloc(sizeof(UA_EndpointDescription));
    if(!conf->endpoints) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }
    conf->endpointsSize = 1;

    /* Populate the endpoint */
    retval =
        createEndpoint(conf, &conf->endpoints[0], &conf->securityPolicies[0],
                       UA_MESSAGESECURITYMODE_NONE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    return conf;
}

#ifdef UA_ENABLE_ENCRYPTION

UA_ServerConfig *
UA_ServerConfig_new_basic128rsa15(UA_UInt16 portNumber,
                                  const UA_ByteString *certificate,
                                  const UA_ByteString *privateKey,
                                  const UA_ByteString *trustList,
                                  size_t trustListSize,
                                  const UA_ByteString *revocationList,
                                  size_t revocationListSize) {
    UA_ServerConfig *conf = createDefaultConfig();

    UA_StatusCode retval = UA_CertificateVerification_Trustlist(&conf->certificateVerification,
                                                                trustList, trustListSize,
                                                                revocationList, revocationListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = UA_Nodestore_default_new(&conf->nodestore);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(addDefaultNetworkLayers(conf, portNumber, 0, 0) != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(trustListSize == 0)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "No CA trust-list provided. Any remote certificate will be accepted.");

    /* Allocate the SecurityPolicies */
    conf->securityPoliciesSize = 0;
    conf->securityPolicies = (UA_SecurityPolicy *)UA_malloc(sizeof(UA_SecurityPolicy) * 2);
    if(!conf->securityPolicies) {
       UA_ServerConfig_delete(conf);
       return NULL;
    }

    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;

    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_None(&conf->securityPolicies[0], NULL, localCertificate, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }
    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_Basic128Rsa15(&conf->securityPolicies[1], &conf->certificateVerification,
                                        localCertificate, localPrivateKey, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Allocate the endpoints */
    conf->endpointsSize = 0;
    conf->endpoints = (UA_EndpointDescription *)UA_malloc(sizeof(UA_EndpointDescription) * 3);
    if(!conf->endpoints) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Populate the endpoints */
    ++conf->endpointsSize;
    retval = createEndpoint(conf, &conf->endpoints[0], &conf->securityPolicies[0],
                            UA_MESSAGESECURITYMODE_NONE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    ++conf->endpointsSize;
    retval = createEndpoint(conf, &conf->endpoints[1], &conf->securityPolicies[1],
                            UA_MESSAGESECURITYMODE_SIGN);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    ++conf->endpointsSize;
    retval = createEndpoint(conf, &conf->endpoints[2], &conf->securityPolicies[1],
                            UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    return conf;
}

UA_ServerConfig *
UA_ServerConfig_new_basic256sha256(UA_UInt16 portNumber,
                                   const UA_ByteString *certificate,
                                   const UA_ByteString *privateKey,
                                   const UA_ByteString *trustList,
                                   size_t trustListSize,
                                   const UA_ByteString *revocationList,
                                   size_t revocationListSize) {
    UA_ServerConfig *conf = createDefaultConfig();

    UA_StatusCode retval = UA_CertificateVerification_Trustlist(&conf->certificateVerification,
                                                                trustList, trustListSize,
                                                                revocationList, revocationListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = UA_Nodestore_default_new(&conf->nodestore);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(addDefaultNetworkLayers(conf, portNumber, 0, 0) != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(trustListSize == 0)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "No CA trust-list provided. Any remote certificate will be accepted.");

    /* Allocate the SecurityPolicies */
    conf->securityPoliciesSize = 0;
    conf->securityPolicies = (UA_SecurityPolicy *)UA_malloc(sizeof(UA_SecurityPolicy) * 2);
    if(!conf->securityPolicies) {
       UA_ServerConfig_delete(conf);
       return NULL;
    }

    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;
    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_None(&conf->securityPolicies[0], NULL, localCertificate, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }
    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_Basic256Sha256(&conf->securityPolicies[1], &conf->certificateVerification,
                                         localCertificate, localPrivateKey, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Allocate the endpoints */
    conf->endpointsSize = 0;
    conf->endpoints = (UA_EndpointDescription *)UA_malloc(sizeof(UA_EndpointDescription) * 3);
    if(!conf->endpoints) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Populate the endpoints */
    ++conf->endpointsSize;
    retval = createEndpoint(conf, &conf->endpoints[0], &conf->securityPolicies[0],
                            UA_MESSAGESECURITYMODE_NONE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    ++conf->endpointsSize;
    retval = createEndpoint(conf, &conf->endpoints[1], &conf->securityPolicies[1],
                            UA_MESSAGESECURITYMODE_SIGN);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    ++conf->endpointsSize;
    retval = createEndpoint(conf, &conf->endpoints[2], &conf->securityPolicies[1],
                            UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    return conf;
}

UA_ServerConfig *
UA_ServerConfig_new_allSecurityPolicies(UA_UInt16 portNumber,
                                        const UA_ByteString *certificate,
                                        const UA_ByteString *privateKey,
                                        const UA_ByteString *trustList,
                                        size_t trustListSize,
                                        const UA_ByteString *revocationList,
                                        size_t revocationListSize) {
    UA_ServerConfig *conf = createDefaultConfig();

    UA_StatusCode retval = UA_CertificateVerification_Trustlist(&conf->certificateVerification,
                                                                trustList, trustListSize,
                                                                revocationList, revocationListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = UA_Nodestore_default_new(&conf->nodestore);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(addDefaultNetworkLayers(conf, portNumber, 0, 0) != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    if(trustListSize == 0)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "No CA trust-list provided. Any remote certificate will be accepted.");

    /* Allocate the SecurityPolicies */
    conf->securityPoliciesSize = 0;
    conf->securityPolicies = (UA_SecurityPolicy *)UA_malloc(sizeof(UA_SecurityPolicy) * 3);
    if(!conf->securityPolicies) {
       UA_ServerConfig_delete(conf);
       return NULL;
    }

    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;
    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_None(&conf->securityPolicies[0], NULL, localCertificate, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }
    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_Basic128Rsa15(&conf->securityPolicies[1], &conf->certificateVerification,
                                        localCertificate, localPrivateKey, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }
    ++conf->securityPoliciesSize;
    retval =
        UA_SecurityPolicy_Basic256Sha256(&conf->securityPolicies[2], &conf->certificateVerification,
                                         localCertificate, localPrivateKey, &conf->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Allocate the endpoints */
    conf->endpointsSize = 0;
    conf->endpoints = (UA_EndpointDescription *)UA_malloc(sizeof(UA_EndpointDescription) * 5);
    if(!conf->endpoints) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Populate the endpoints */
    retval = createEndpoint(conf, &conf->endpoints[conf->endpointsSize], &conf->securityPolicies[0],
                            UA_MESSAGESECURITYMODE_NONE);
    ++conf->endpointsSize;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = createEndpoint(conf, &conf->endpoints[conf->endpointsSize], &conf->securityPolicies[1],
                            UA_MESSAGESECURITYMODE_SIGN);
    ++conf->endpointsSize;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = createEndpoint(conf, &conf->endpoints[conf->endpointsSize], &conf->securityPolicies[1],
                            UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ++conf->endpointsSize;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = createEndpoint(conf, &conf->endpoints[conf->endpointsSize], &conf->securityPolicies[2],
                            UA_MESSAGESECURITYMODE_SIGN);
    ++conf->endpointsSize;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    retval = createEndpoint(conf, &conf->endpoints[conf->endpointsSize], &conf->securityPolicies[2],
                            UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    ++conf->endpointsSize;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    return conf;
}


#endif

void
UA_ServerConfig_delete(UA_ServerConfig *config) {
    if(!config)
        return;

    /* Server Description */
    UA_BuildInfo_deleteMembers(&config->buildInfo);
    UA_ApplicationDescription_deleteMembers(&config->applicationDescription);
#ifdef UA_ENABLE_DISCOVERY
    UA_String_deleteMembers(&config->mdnsServerName);
    UA_Array_delete(config->serverCapabilities, config->serverCapabilitiesSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverCapabilities = NULL;
    config->serverCapabilitiesSize = 0;
#endif

    /* Nodestore */
    if(config->nodestore.deleteNodestore)
        config->nodestore.deleteNodestore(config->nodestore.context);

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
    config->certificateVerification.deleteMembers(&config->certificateVerification);

    /* Access Control */
    config->accessControl.deleteMembers(&config->accessControl);

    /* Historical data */
#ifdef UA_ENABLE_HISTORIZING
    if(config->historyDatabase.deleteMembers)
        config->historyDatabase.deleteMembers(&config->historyDatabase);
#endif

    /* Logger */
    if(config->logger.clear)
        config->logger.clear(config->logger.context);

    UA_free(config);
}


#ifdef UA_ENABLE_PUBSUB /* conditional compilation */
/**
 * Add a pubsubTransportLayer to the configuration.
 * Memory is reallocated on demand */
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

    if (config->pubsubTransportLayers == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    memcpy(&config->pubsubTransportLayers[config->pubsubTransportLayersSize],
            pubsubTransportLayer, sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayersSize++;

    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_PUBSUB */


/***************************/
/* Default Client Settings */
/***************************/

static UA_INLINE void UA_ClientConnectionTCP_poll_callback(UA_Client *client, void *data) {
    UA_ClientConnectionTCP_poll(client, data);
}

UA_StatusCode
UA_ClientConfig_setDefault(UA_ClientConfig *config) {
    config->timeout = 5000;
    config->secureChannelLifeTime = 10 * 60 * 1000; /* 10 minutes */

    config->logger.log = UA_Log_Stdout_log;
    config->logger.context = NULL;
    config->logger.clear = UA_Log_Stdout_clear;

    config->localConnectionConfig = UA_ConnectionConfig_default;

    /* Certificate Verification that accepts every certificate. Can be
     * overwritten when the policy is specialized. */
    UA_CertificateVerification_AcceptAll(&config->certificateVerification);

    if(config->securityPoliciesSize > 0) {
        UA_LOG_ERROR(&config->logger, UA_LOGCATEGORY_NETWORK,
                     "Could not initialize a config that already has SecurityPolicies");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    config->securityPolicies = (UA_SecurityPolicy*)malloc(sizeof(UA_SecurityPolicy));
    if(!config->securityPolicies)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_SecurityPolicy_None(config->securityPolicies, NULL,
                                                  UA_BYTESTRING_NULL, &config->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        free(config->securityPolicies);
        config->securityPolicies = NULL;
        return retval;
    }
    config->securityPoliciesSize = 1;

    config->connectionFunc = UA_ClientConnectionTCP;
    config->initConnectionFunc = UA_ClientConnectionTCP_init; /* for async client */
    config->pollConnectionFunc = UA_ClientConnectionTCP_poll_callback; /* for async connection */

    config->customDataTypes = NULL;
    config->stateCallback = NULL;
    config->connectivityCheckInterval = 0;

    config->requestedSessionTimeout = 1200000; /* requestedSessionTimeout */

    config->inactivityCallback = NULL;
    config->clientContext = NULL;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    config->outStandingPublishRequests = 10;
    config->subscriptionInactivityCallback = NULL;
#endif

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_ENCRYPTION
UA_StatusCode
UA_ClientConfig_setDefaultEncryption(UA_ClientConfig *config,
                                     UA_ByteString localCertificate, UA_ByteString privateKey,
                                     const UA_ByteString *trustList, size_t trustListSize,
                                     const UA_ByteString *revocationList, size_t revocationListSize) {
    UA_StatusCode retval = UA_ClientConfig_setDefault(config);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_CertificateVerification_Trustlist(&config->certificateVerification,
                                                  trustList, trustListSize,
                                                  revocationList, revocationListSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Populate SecurityPolicies */
    UA_SecurityPolicy *sp = (UA_SecurityPolicy*)
        realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * 3);
    if(!sp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = sp;

    retval = UA_SecurityPolicy_Basic128Rsa15(&config->securityPolicies[1],
                                             &config->certificateVerification,
                                             localCertificate, privateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    ++config->securityPoliciesSize;

    retval = UA_SecurityPolicy_Basic256Sha256(&config->securityPolicies[2],
                                              &config->certificateVerification,
                                              localCertificate, privateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    ++config->securityPoliciesSize;

    return UA_STATUSCODE_GOOD;
}
#endif
