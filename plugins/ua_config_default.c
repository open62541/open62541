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
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#include <open62541/client_config_default.h>
#include <open62541/network_tcp.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>

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

    /* Add security level value for the corresponding message security mode */
    endpoint->securityLevel = (UA_Byte) securityMode;

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

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

static UA_StatusCode
setDefaultConfig(UA_ServerConfig *conf) {
    if (!conf)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Zero out.. All members have a valid initial value */
    UA_ServerConfig_clean(conf);
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

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_MdnsDiscoveryConfiguration_init(&conf->discovery.mdns);
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

    UA_StatusCode retval = UA_AccessControl_default(&conf->accessControl, true,
                                                    usernamePasswordsSize,
                                                    usernamePasswords);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(conf);
        return retval;
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
    conf->enableRetransmissionQueue = true;
    conf->maxRetransmissionQueueSize = 0; /* unlimited */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    conf->maxEventsPerNode = 0; /* unlimited */
#endif

    /* Limits for MonitoredItems */
    conf->samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
    conf->queueSizeLimits = UA_UINT32RANGE(1, 100);

#ifdef UA_ENABLE_DISCOVERY
    conf->discovery.cleanupTimeout = 60 * 60;
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

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setBasics(UA_ServerConfig* conf) {
    return setDefaultConfig(conf);
}

static UA_StatusCode
addDefaultNetworkLayers(UA_ServerConfig *conf, UA_UInt16 portNumber,
                        UA_UInt32 sendBufferSize, UA_UInt32 recvBufferSize) {
    return UA_ServerConfig_addNetworkLayerTCP(conf, portNumber, sendBufferSize, recvBufferSize);
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addNetworkLayerTCP(UA_ServerConfig *conf, UA_UInt16 portNumber,
                                   UA_UInt32 sendBufferSize, UA_UInt32 recvBufferSize) {
    /* Add a network layer */
    UA_ServerNetworkLayer *tmp = (UA_ServerNetworkLayer *)
        UA_realloc(conf->networkLayers, sizeof(UA_ServerNetworkLayer) * (1 + conf->networkLayersSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    conf->networkLayers = tmp;

    UA_ConnectionConfig config = UA_ConnectionConfig_default;
    if (sendBufferSize > 0)
        config.sendBufferSize = sendBufferSize;
    if (recvBufferSize > 0)
        config.recvBufferSize = recvBufferSize;

    conf->networkLayers[conf->networkLayersSize] =
        UA_ServerNetworkLayerTCP(config, portNumber, &conf->logger);
    if (!conf->networkLayers[conf->networkLayersSize].handle)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    conf->networkLayersSize++;

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyNone(UA_ServerConfig *config, 
                                      const UA_ByteString *certificate) {
    UA_StatusCode retval;

    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;
    
    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    retval = UA_SecurityPolicy_None(&config->securityPolicies[config->securityPoliciesSize], NULL,
                                    localCertificate, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addEndpoint(UA_ServerConfig *config, const UA_String securityPolicyUri, 
                            UA_MessageSecurityMode securityMode)
{
    UA_StatusCode retval;

    /* Allocate the endpoint */
    UA_EndpointDescription * tmp = (UA_EndpointDescription *)
        UA_realloc(config->endpoints, sizeof(UA_EndpointDescription) * (1 + config->endpointsSize));
    if(!tmp) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    config->endpoints = tmp;

    /* Lookup the security policy */
    const UA_SecurityPolicy *policy = NULL;
    for (size_t i = 0; i < config->securityPoliciesSize; ++i) {
        if (UA_String_equal(&securityPolicyUri, &config->securityPolicies[i].policyUri)) {
            policy = &config->securityPolicies[i];
            break;
        }
    }
    if (!policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Populate the endpoint */
    retval = createEndpoint(config, &config->endpoints[config->endpointsSize],
                            policy, securityMode);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->endpointsSize++;

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addAllEndpoints(UA_ServerConfig *config) {
    UA_StatusCode retval;

    /* Allocate the endpoints */
    UA_EndpointDescription * tmp = (UA_EndpointDescription *)
        UA_realloc(config->endpoints, sizeof(UA_EndpointDescription) * (2 * config->securityPoliciesSize + config->endpointsSize));
    if(!tmp) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    config->endpoints = tmp;

    /* Populate the endpoints */
    for (size_t i = 0; i < config->securityPoliciesSize; ++i) {
        if (UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &config->securityPolicies[i].policyUri)) {
            retval = createEndpoint(config, &config->endpoints[config->endpointsSize],
                                    &config->securityPolicies[i], UA_MESSAGESECURITYMODE_NONE);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
            config->endpointsSize++;
        } else {
            retval = createEndpoint(config, &config->endpoints[config->endpointsSize],
                                    &config->securityPolicies[i], UA_MESSAGESECURITYMODE_SIGN);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
            config->endpointsSize++;

            retval = createEndpoint(config, &config->endpoints[config->endpointsSize],
                                    &config->securityPolicies[i], UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
            config->endpointsSize++;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setMinimalCustomBuffer(UA_ServerConfig *config, UA_UInt16 portNumber,
                                       const UA_ByteString *certificate,
                                       UA_UInt32 sendBufferSize,
                                       UA_UInt32 recvBufferSize) {
    if (!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = setDefaultConfig(config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    retval = addDefaultNetworkLayers(config, portNumber, sendBufferSize, recvBufferSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    /* Allocate the SecurityPolicies */
    retval = UA_ServerConfig_addSecurityPolicyNone(config, certificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    /* Allocate the endpoint */
    retval = UA_ServerConfig_addEndpoint(config, UA_SECURITY_POLICY_NONE_URI, UA_MESSAGESECURITYMODE_NONE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(config);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_ENCRYPTION

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic128Rsa15(UA_ServerConfig *config, 
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey) {
    UA_StatusCode retval;

    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;
    
    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;
    retval = UA_SecurityPolicy_Basic128Rsa15(&config->securityPolicies[config->securityPoliciesSize],
                                             &config->certificateVerification,
                                             localCertificate, localPrivateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic256(UA_ServerConfig *config, 
                                          const UA_ByteString *certificate,
                                          const UA_ByteString *privateKey) {
    UA_StatusCode retval;

    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;
    
    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;
    retval = UA_SecurityPolicy_Basic256(&config->securityPolicies[config->securityPoliciesSize],
                                        &config->certificateVerification,
                                        localCertificate, localPrivateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic256Sha256(UA_ServerConfig *config, 
                                                const UA_ByteString *certificate,
                                                const UA_ByteString *privateKey) {
    UA_StatusCode retval;

    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;
    
    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;
    retval = UA_SecurityPolicy_Basic256Sha256(&config->securityPolicies[config->securityPoliciesSize],
                                              &config->certificateVerification,
                                              localCertificate, localPrivateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addAllSecurityPolicies(UA_ServerConfig *config,
                                       const UA_ByteString *certificate,
                                       const UA_ByteString *privateKey) {
    UA_StatusCode retval;

    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * (4 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;
    
    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;

    retval = UA_SecurityPolicy_None(&config->securityPolicies[config->securityPoliciesSize], NULL,
                                    localCertificate, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    retval = UA_SecurityPolicy_Basic128Rsa15(&config->securityPolicies[config->securityPoliciesSize],
                                             &config->certificateVerification,
                                             localCertificate, localPrivateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    retval = UA_SecurityPolicy_Basic256(&config->securityPolicies[config->securityPoliciesSize],
                                        &config->certificateVerification,
                                        localCertificate, localPrivateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    retval = UA_SecurityPolicy_Basic256Sha256(&config->securityPolicies[config->securityPoliciesSize],
                                              &config->certificateVerification,
                                              localCertificate, localPrivateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    config->securityPoliciesSize++;

    return retval;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setDefaultWithSecurityPolicies(UA_ServerConfig *conf,
                                               UA_UInt16 portNumber,
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey,
                                               const UA_ByteString *trustList,
                                               size_t trustListSize,
                                               const UA_ByteString *revocationList,
                                               size_t revocationListSize) {
    UA_StatusCode retval = setDefaultConfig(conf);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(conf);
        return retval;
    }

    retval = UA_CertificateVerification_Trustlist(&conf->certificateVerification,
                                                  trustList, trustListSize,
                                                  revocationList, revocationListSize);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    if(trustListSize == 0)
        UA_LOG_WARNING(&conf->logger, UA_LOGCATEGORY_USERLAND,
                       "No CA trust-list provided. "
                       "Any remote certificate will be accepted.");

    retval = addDefaultNetworkLayers(conf, portNumber, 0, 0);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(conf);
        return retval;
    }

    retval = UA_ServerConfig_addAllSecurityPolicies(conf, certificate, privateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(conf);
        return retval;
    }

    retval = UA_ServerConfig_addAllEndpoints(conf);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(conf);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

#endif

/***************************/
/* Default Client Settings */
/***************************/

static UA_INLINE void
UA_ClientConnectionTCP_poll_callback(UA_Client *client, void *data) {
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

    /* With encryption enabled, the applicationUri needs to match the URI from
     * the certificate */
    config->clientDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);
    config->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;

    if(config->securityPoliciesSize > 0) {
        UA_LOG_ERROR(&config->logger, UA_LOGCATEGORY_NETWORK,
                     "Could not initialize a config that already has SecurityPolicies");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    config->securityPolicies = (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy));
    if(!config->securityPolicies)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_SecurityPolicy_None(config->securityPolicies, NULL,
                                                  UA_BYTESTRING_NULL, &config->logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(config->securityPolicies);
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
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * 4);
    if(!sp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = sp;

    retval = UA_SecurityPolicy_Basic128Rsa15(&config->securityPolicies[1],
                                             &config->certificateVerification,
                                             localCertificate, privateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    ++config->securityPoliciesSize;

    retval = UA_SecurityPolicy_Basic256(&config->securityPolicies[2],
                                        &config->certificateVerification,
                                        localCertificate, privateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    ++config->securityPoliciesSize;

    retval = UA_SecurityPolicy_Basic256Sha256(&config->securityPolicies[3],
                                              &config->certificateVerification,
                                              localCertificate, privateKey, &config->logger);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    ++config->securityPoliciesSize;

    return UA_STATUSCODE_GOOD;
}
#endif
