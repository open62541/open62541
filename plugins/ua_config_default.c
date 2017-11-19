/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_config_default.h"
#include "ua_log_stdout.h"
#include "ua_network_tcp.h"
#include "ua_accesscontrol_default.h"
#include "ua_nodestore_default.h"
#include "ua_types_generated.h"
#include "ua_securitypolicy_none.h"
#include "ua_types.h"
#include "ua_types_generated_handling.h"
#include "ua_client_highlevel.h"

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

/* Struct initialization works across ANSI C/C99/C++ if it is done when the
 * variable is first declared. Assigning values to existing structs is
 * heterogeneous across the three. */
static UA_INLINE UA_UInt32Range
UA_UINT32RANGE(UA_UInt32 min, UA_UInt32 max) {
    UA_UInt32Range range = {min, max};
    return range;
}

static UA_INLINE UA_DurationRange
UA_DURATIONRANGE(UA_Double min, UA_Double max) {
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
createSecurityPolicyNoneEndpoint(UA_ServerConfig *conf, UA_Endpoint *endpoint,
                                 const UA_ByteString localCertificate) {
    UA_EndpointDescription_init(&endpoint->endpointDescription);

    UA_SecurityPolicy_None(&endpoint->securityPolicy, localCertificate, conf->logger);
    endpoint->endpointDescription.securityMode = UA_MESSAGESECURITYMODE_NONE;
    endpoint->endpointDescription.securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
    endpoint->endpointDescription.transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    /* enable anonymous and username/password */
    size_t policies = 2;
    endpoint->endpointDescription.userIdentityTokens = (UA_UserTokenPolicy*)
        UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(!endpoint->endpointDescription.userIdentityTokens)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    endpoint->endpointDescription.userIdentityTokensSize = policies;

    endpoint->endpointDescription.userIdentityTokens[0].tokenType =
        UA_USERTOKENTYPE_ANONYMOUS;
    endpoint->endpointDescription.userIdentityTokens[0].policyId =
        UA_STRING_ALLOC(ANONYMOUS_POLICY);

    endpoint->endpointDescription.userIdentityTokens[1].tokenType =
        UA_USERTOKENTYPE_USERNAME;
    endpoint->endpointDescription.userIdentityTokens[1].policyId =
        UA_STRING_ALLOC(USERNAME_POLICY);

    UA_String_copy(&localCertificate, &endpoint->endpointDescription.serverCertificate);

    UA_ApplicationDescription_copy(&conf->applicationDescription,
                                   &endpoint->endpointDescription.server);

    return UA_STATUSCODE_GOOD;
}

void
UA_ServerConfig_set_customHostname(UA_ServerConfig *config, const UA_String customHostname){
    if(!config)
        return;
    UA_String_deleteMembers(&config->customHostname);
    UA_String_copy(&customHostname, &config->customHostname);
}

UA_ServerConfig *
UA_ServerConfig_new_minimal(UA_UInt16 portNumber,
                            const UA_ByteString *certificate) {
    UA_ServerConfig *conf = (UA_ServerConfig*)UA_malloc(sizeof(UA_ServerConfig));
    if(!conf)
        return NULL;

    /* Zero out.. All members have a valid initial value */
    memset(conf, 0, sizeof(UA_ServerConfig));

    /* --> Start setting the default static config <-- */
    conf->nThreads = 1;
    conf->logger = UA_Log_Stdout;

    /* Server Description */
    conf->buildInfo.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    conf->buildInfo.manufacturerName = UA_STRING_ALLOC(MANUFACTURER_NAME);
    conf->buildInfo.productName = UA_STRING_ALLOC(PRODUCT_NAME);
    conf->buildInfo.softwareVersion =
        UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));
    conf->buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);
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

    /* Global Node Lifecycle */
    conf->nodeLifecycle.constructor = NULL;
    conf->nodeLifecycle.destructor = NULL;

    /* Access Control */
    conf->accessControl.enableAnonymousLogin = true;
    conf->accessControl.enableUsernamePasswordLogin = true;
    conf->accessControl.activateSession = activateSession_default;
    conf->accessControl.closeSession = closeSession_default;
    conf->accessControl.getUserRightsMask = getUserRightsMask_default;
    conf->accessControl.getUserAccessLevel = getUserAccessLevel_default;
    conf->accessControl.getUserExecutable = getUserExecutable_default;
    conf->accessControl.getUserExecutableOnObject = getUserExecutableOnObject_default;
    conf->accessControl.allowAddNode = allowAddNode_default;
    conf->accessControl.allowAddReference = allowAddReference_default;
    conf->accessControl.allowDeleteNode = allowDeleteNode_default;
    conf->accessControl.allowDeleteReference = allowDeleteReference_default;

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

    /* Limits for MonitoredItems */
    conf->samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
    conf->queueSizeLimits = UA_UINT32RANGE(1, 100);

#ifdef UA_ENABLE_DISCOVERY
    conf->discoveryCleanupTimeout = 60 * 60;
#endif

    /* --> Finish setting the default static config <-- */

    UA_StatusCode retval = UA_Nodestore_default_new(&conf->nodestore);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Add a network layer */
    conf->networkLayers = (UA_ServerNetworkLayer*)
        UA_malloc(sizeof(UA_ServerNetworkLayer));
    if(!conf->networkLayers) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }
    conf->networkLayers[0] =
        UA_ServerNetworkLayerTCP(UA_ConnectionConfig_default, portNumber);
    conf->networkLayersSize = 1;

    /* Allocate the endpoint */
    conf->endpointsSize = 1;
    conf->endpoints = (UA_Endpoint*)UA_malloc(sizeof(UA_Endpoint));
    if(!conf->endpoints) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    /* Populate the endpoint */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    retval =
        createSecurityPolicyNoneEndpoint(conf, &conf->endpoints[0], localCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(conf);
        return NULL;
    }

    return conf;
}

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
    for(size_t i = 0; i < config->customDataTypesSize; ++i)
        UA_free(config->customDataTypes[i].members);
    UA_free(config->customDataTypes);
    config->customDataTypes = NULL;
    config->customDataTypesSize = 0;

    /* Networking */
    for(size_t i = 0; i < config->networkLayersSize; ++i)
        config->networkLayers[i].deleteMembers(&config->networkLayers[i]);
    UA_free(config->networkLayers);
    config->networkLayers = NULL;
    config->networkLayersSize = 0;
    UA_String_deleteMembers(&config->customHostname);
    config->customHostname = UA_STRING_NULL;

    for(size_t i = 0; i < config->endpointsSize; ++i) {
        UA_SecurityPolicy *policy = &config->endpoints[i].securityPolicy;
        policy->deleteMembers(policy);
        UA_EndpointDescription_deleteMembers(&config->endpoints[i].endpointDescription);
    }
    UA_free(config->endpoints);
    config->endpoints = NULL;
    config->endpointsSize = 0;

    UA_free(config);
}

/***************************/
/* Default Client Settings */
/***************************/

const UA_ClientConfig UA_ClientConfig_default = {
    5000, /* .timeout, 5 seconds */
    10 * 60 * 1000, /* .secureChannelLifeTime, 10 minutes */
    UA_Log_Stdout, /* .logger */
    /* .localConnectionConfig */
    {0, /* .protocolVersion */
        65535, /* .sendBufferSize, 64k per chunk */
        65535, /* .recvBufferSize, 64k per chunk */
        0, /* .maxMessageSize, 0 -> unlimited */
        0}, /* .maxChunkCount, 0 -> unlimited */
    UA_ClientConnectionTCP, /* .connectionFunc */

    0, /* .customDataTypesSize */
    NULL /*.customDataTypes */
};

/****************************************/
/* Default Client Subscription Settings */
/****************************************/

#ifdef UA_ENABLE_SUBSCRIPTIONS

const UA_SubscriptionSettings UA_SubscriptionSettings_default = {
    500.0, /* .requestedPublishingInterval */
    10000, /* .requestedLifetimeCount */
    1, /* .requestedMaxKeepAliveCount */
    10, /* .maxNotificationsPerPublish */
    true, /* .publishingEnabled */
    0 /* .priority */
};

#endif
