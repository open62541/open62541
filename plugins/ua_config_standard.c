/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_config_standard.h"
#include "ua_log_stdout.h"
#include "ua_network_tcp.h"
#include "ua_accesscontrol_default.h"
#include "ua_types_generated.h"
#include "ua_securitypolicy_none.h"
#include "ua_types.h"

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

 /*******************************/
 /* Default Connection Settings */
 /*******************************/

const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard =
{
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

#define UA_STRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)s}
#define UA_STRING_STATIC_NULL {0, NULL}
#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

const UA_EXPORT UA_ServerConfig UA_ServerConfig_standard = {
    1, /* .nThreads */
    UA_Log_Stdout, /* .logger */

    /* Server Description */
    {
        UA_STRING_STATIC(PRODUCT_URI),
        UA_STRING_STATIC(MANUFACTURER_NAME),
        UA_STRING_STATIC(PRODUCT_NAME),
        UA_STRING_STATIC(VERSION(
                UA_OPEN62541_VER_MAJOR,
                UA_OPEN62541_VER_MINOR,
                UA_OPEN62541_VER_PATCH,
                UA_OPEN62541_VER_LABEL)
        ),
        UA_STRING_STATIC(__DATE__ " " __TIME__),
        0
    }, /* .buildInfo */

    {
        UA_STRING_STATIC(APPLICATION_URI),
        UA_STRING_STATIC(PRODUCT_URI),
        {
            UA_STRING_STATIC("en"),
            UA_STRING_STATIC(APPLICATION_NAME)
        },
        UA_APPLICATIONTYPE_SERVER,
        UA_STRING_STATIC_NULL,
        UA_STRING_STATIC_NULL,
        0, NULL
    }, /* .applicationDescription */

#ifdef UA_ENABLE_DISCOVERY
    UA_STRING_STATIC_NULL, /* mdnsServerName */
    0, /* serverCapabilitiesSize */
    NULL, /* serverCapabilities */
#endif

    /* Custom DataTypes */
    0, /* .customDataTypesSize */
    NULL, /* .customDataTypes */

    /* Networking */
    0, /* .networkLayersSize */
    NULL, /* .networkLayers */

    /* Endpoints */
    0,
    NULL,

    /* Access Control */
    {true, true,
     activateSession_default, closeSession_default,
     getUserRightsMask_default, getUserAccessLevel_default,
     getUserExecutable_default, getUserExecutableOnObject_default,
     allowAddNode_default, allowAddReference_default,
     allowDeleteNode_default, allowDeleteReference_default},

    /* Limits for SecureChannels */
    40, /* .maxSecureChannels */
    10 * 60 * 1000, /* .maxSecurityTokenLifetime, 10 minutes */

    /* Limits for Sessions */
    100, /* .maxSessions */
    60.0 * 60.0 * 1000.0, /* .maxSessionTimeout, 1h */

    /* Limits for Subscriptions */
    {
        100.0,
        3600.0 * 1000.0
    }, /* .publishingIntervalLimits */

    {
        3,
        15000
    }, /* .lifeTimeCountLimits */

    {
        1,
        100
    }, /* .keepAliveCountLimits */

    1000, /* .maxNotificationsPerPublish */
    0, /* .maxRetransmissionQueueSize, unlimited */

    /* Limits for MonitoredItems */
    {
        50.0,
        24.0 * 3600.0 * 1000.0
    }, /* .samplingIntervalLimits */

    {
        1,
        100
    } /* .queueSizeLimits */

#ifdef UA_ENABLE_DISCOVERY
    , 60 * 60 /* .discoveryCleanupTimeout */
#endif
};

/***************************/
/* Default Client Settings */
/***************************/

const UA_EXPORT UA_ClientConfig UA_ClientConfig_standard = {
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

const UA_SubscriptionSettings UA_SubscriptionSettings_standard = {
    500.0, /* .requestedPublishingInterval */
    10000, /* .requestedLifetimeCount */
    1, /* .requestedMaxKeepAliveCount */
    10, /* .maxNotificationsPerPublish */
    true, /* .publishingEnabled */
    0 /* .priority */
};

#endif

static UA_StatusCode createSecurityPolicyNoneEndpoint(UA_ServerConfig *const conf,
                                                      const UA_ByteString *const cert,
                                                      size_t slot) {
    UA_Endpoint *endpoint_sp_none = &conf->endpoints[slot];

    UA_EndpointDescription_init(&endpoint_sp_none->endpointDescription);

    endpoint_sp_none->securityPolicy = &UA_SecurityPolicy_None;
    endpoint_sp_none->endpointDescription.securityMode = UA_MESSAGESECURITYMODE_NONE;
    UA_ByteString_copy(&endpoint_sp_none->securityPolicy->policyUri,
                       &endpoint_sp_none->endpointDescription.securityPolicyUri);
    endpoint_sp_none->endpointDescription.transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    size_t policies = 0;
    if(conf->accessControl.enableAnonymousLogin)
        ++policies;
    if(conf->accessControl.enableUsernamePasswordLogin)
        ++policies;
    endpoint_sp_none->endpointDescription.userIdentityTokensSize = policies;
    endpoint_sp_none->endpointDescription.userIdentityTokens =
        (UA_UserTokenPolicy *)UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

    size_t currentIndex = 0;
    if(conf->accessControl.enableAnonymousLogin) {
        UA_UserTokenPolicy_init(&endpoint_sp_none->endpointDescription.userIdentityTokens[currentIndex]);
        endpoint_sp_none->endpointDescription.userIdentityTokens[currentIndex].tokenType =
            UA_USERTOKENTYPE_ANONYMOUS;
        endpoint_sp_none->endpointDescription.userIdentityTokens[currentIndex].policyId =
            UA_STRING_ALLOC(ANONYMOUS_POLICY);
        ++currentIndex;
    }
    if(conf->accessControl.enableUsernamePasswordLogin) {
        UA_UserTokenPolicy_init(&endpoint_sp_none->endpointDescription.userIdentityTokens[currentIndex]);
        endpoint_sp_none->endpointDescription.userIdentityTokens[currentIndex].tokenType =
            UA_USERTOKENTYPE_USERNAME;
        endpoint_sp_none->endpointDescription.userIdentityTokens[currentIndex].policyId =
            UA_STRING_ALLOC(USERNAME_POLICY);
    }

    if(cert != NULL)
        UA_String_copy(cert, &endpoint_sp_none->endpointDescription.serverCertificate);

    UA_ApplicationDescription_copy(&conf->applicationDescription,
                                   &endpoint_sp_none->endpointDescription.server);

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_ServerConfig *UA_ServerConfig_standard_parametrized_new(UA_UInt16 portNumber,
                                                                     const UA_ByteString *certificate) {

    UA_ServerConfig *conf = (UA_ServerConfig*)UA_malloc(sizeof(UA_ServerConfig));
    if(conf == NULL)
        return NULL;

    *conf = UA_ServerConfig_standard;

    conf->networkLayersSize = 1;
    conf->networkLayers = (UA_ServerNetworkLayer*)UA_malloc(sizeof(UA_ServerNetworkLayer) * conf->networkLayersSize);
    conf->networkLayers[0] = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);

    conf->endpointsSize = 1;
    conf->endpoints = (UA_Endpoint*)UA_malloc(sizeof(UA_Endpoint) * conf->endpointsSize);
    if(conf->endpoints == NULL) {
        UA_free(conf);
        return NULL;
    }

    createSecurityPolicyNoneEndpoint(conf, certificate, 0);

    // Initialize policy contexts
    for(size_t i = 0; i < conf->endpointsSize; ++i) {
        UA_SecurityPolicy *const policy = conf->endpoints[i].securityPolicy;
        policy->logger = conf->logger;

        policy->endpointContext.newContext(policy, NULL, &conf->endpoints[i].securityContext);
    }

    return conf;
}

UA_EXPORT UA_ServerConfig *UA_ServerConfig_standard_new(void) {
    return UA_ServerConfig_standard_parametrized_new(4840, NULL);
}

UA_EXPORT void UA_ServerConfig_standard_deleteMembers(UA_ServerConfig *config) {

    if(config == NULL)
        return;

    for(size_t i = 0; i < config->endpointsSize; ++i) {
        UA_SecurityPolicy *const policy = config->endpoints[i].securityPolicy;

        policy->endpointContext.deleteContext(config->endpoints[i].securityContext);

        UA_EndpointDescription_deleteMembers(&config->endpoints[i].endpointDescription);
    }

    for(size_t i = 0; i < config->networkLayersSize; ++i) {
        config->networkLayers[i].deleteMembers(&config->networkLayers[i]);
    }

    UA_free(config->endpoints);
    UA_free(config->networkLayers);

    UA_free(config);
}