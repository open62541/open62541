/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_config_standard.h"
#include "ua_log_stdout.h"
#include "ua_network_tcp.h"
#include "ua_accesscontrol_default.h"

/*******************************/
/* Default Connection Settings */
/*******************************/

const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard = {
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

/* Access Control. The following definitions are defined as "extern" in
   ua_accesscontrol_default.h */
#define ENABLEANONYMOUSLOGIN true
#define ENABLEUSERNAMEPASSWORDLOGIN true
const UA_Boolean enableAnonymousLogin = ENABLEANONYMOUSLOGIN;
const UA_Boolean enableUsernamePasswordLogin = ENABLEUSERNAMEPASSWORDLOGIN;
const size_t usernamePasswordsSize = 2;

UA_UsernamePasswordLogin UsernamePasswordLogin[2] = {
    { UA_STRING_STATIC("user1"), UA_STRING_STATIC("password") },
    { UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1") } };
const UA_UsernamePasswordLogin *usernamePasswords = UsernamePasswordLogin;

const UA_EXPORT UA_ServerConfig UA_ServerConfig_standard = {
	1, /* .nThreads */
    UA_Log_Stdout, /* .logger */

    /* Server Description */
    {UA_STRING_STATIC(PRODUCT_URI),
     UA_STRING_STATIC(MANUFACTURER_NAME),
     UA_STRING_STATIC(PRODUCT_NAME),
     UA_STRING_STATIC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                              UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL)),
     UA_STRING_STATIC(__DATE__ " " __TIME__), 0 }, /* .buildInfo */

    {UA_STRING_STATIC(APPLICATION_URI),
     UA_STRING_STATIC(PRODUCT_URI),
     {UA_STRING_STATIC("en"),UA_STRING_STATIC(APPLICATION_NAME) },
      UA_APPLICATIONTYPE_SERVER,
      UA_STRING_STATIC_NULL,
      UA_STRING_STATIC_NULL,
      0, NULL }, /* .applicationDescription */
#ifdef UA_ENABLE_DISCOVERY
	  NULL, /* serverCapabilities */
	  0, /* serverCapabilitiesSize */
	  UA_STRING_STATIC_NULL, /* mdnsServerName */
#endif
    UA_STRING_STATIC_NULL, /* .serverCertificate */

	/* Custom DataTypes */
	0, /* .customDataTypesSize */
	NULL, /* .customDataTypes */

    /* Networking */
    0, /* .networkLayersSize */
    NULL, /* .networkLayers */

    /* Access Control */
    {ENABLEANONYMOUSLOGIN, ENABLEUSERNAMEPASSWORDLOGIN,
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
    {100.0,3600.0 * 1000.0 }, /* .publishingIntervalLimits */
    {3, 15000 }, /* .lifeTimeCountLimits */
    {1,100}, /* .keepAliveCountLimits */
    1000, /* .maxNotificationsPerPublish */
    0, /* .maxRetransmissionQueueSize, unlimited */

    /* Limits for MonitoredItems */
    {50.0, 24.0 * 3600.0 * 1000.0 }, /* .samplingIntervalLimits */
    {1,100} /* .queueSizeLimits */

#ifdef UA_ENABLE_DISCOVERY
	, 60*60 /* .discoveryCleanupTimeout */
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
     0 }, /* .maxChunkCount, 0 -> unlimited */
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
