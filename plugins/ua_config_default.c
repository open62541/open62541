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
 *    Copyright 2017-2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>

#include "../deps/mp_printf.h"

#include <stdio.h>
#ifdef UA_ARCHITECTURE_WIN32
# include <winsock2.h>
#else
# include <unistd.h>
#endif

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

/* Request the private key password from stdin if no callback is defined */
#ifdef UA_ENABLE_ENCRYPTION
static UA_StatusCode
readPrivateKeyPassword(UA_ByteString *password) {
    /* Read from stdin */
    char buf[256];
    fputs("Private key requires a password. Enter and press return: ", stdout);
    char *s = fgets(buf, 256, stdin);
    if(!s)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Get rid of any trailing \n */
    size_t len = strlen(buf);
    if(len == 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(buf[len-1] == '\n')
        buf[len-1] = 0;

    *password = UA_BYTESTRING_ALLOC(buf);
    return UA_STATUSCODE_GOOD;
}
#endif

UA_Server *
UA_Server_new(void) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setDefault(&config);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;
    return UA_Server_newWithConfig(&config);
}

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) || defined(UA_ARCHITECTURE_ZEPHYR)
/* Required for the definition of SIGINT */
#include <signal.h>

struct InterruptContext {
    UA_Server *server;
    UA_Boolean running;
};

static void
shutdownServer(UA_Server *server, void *context) {
    struct InterruptContext *ic = (struct InterruptContext*)context;
    UA_ServerConfig *config = UA_Server_getConfig(ic->server);
    UA_LOG_INFO(config->logging, UA_LOGCATEGORY_USERLAND,
                "Stopping the server");
    ic->running = false;
}

static void
interruptServer(UA_InterruptManager *im, uintptr_t interruptHandle,
                void *context, const UA_KeyValueMap *parameters) {
    struct InterruptContext *ic = (struct InterruptContext*)context;
    UA_ServerConfig *config = UA_Server_getConfig(ic->server);

    if(config->shutdownDelay <= 0.0) {
        UA_LOG_INFO(config->logging, UA_LOGCATEGORY_USERLAND,
                    "Received SIGINT interrupt. Stopping the server.");
        ic->running = false;
        return;
    }

    UA_LOG_INFO(config->logging, UA_LOGCATEGORY_USERLAND,
                "Received SIGINT interrupt. Stopping the server in %.2fs.",
                config->shutdownDelay / 1000.0);

    UA_UInt32 secondsTillShutdown = (UA_UInt32)(config->shutdownDelay / 1000.0);
    UA_Variant val;
    UA_Variant_setScalar(&val, &secondsTillShutdown, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(ic->server, UA_NS0ID(SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), val);
    UA_Server_addTimedCallback(ic->server, shutdownServer, ic, UA_DateTime_nowMonotonic() +
                               (UA_DateTime)(config->shutdownDelay * UA_DATETIME_MSEC), NULL);

    /* Notify the application that the server is stopping */
    if(config->notifyLifecycleState)
        config->notifyLifecycleState(ic->server, UA_LIFECYCLESTATE_STOPPING);
}

UA_StatusCode
UA_Server_runUntilInterrupt(UA_Server *server) {
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_EventLoop *el = config->eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Get the interrupt manager */
    UA_EventSource *es = el->eventSources;
    while(es) {
        if(es->eventSourceType == UA_EVENTSOURCETYPE_INTERRUPTMANAGER)
            break;
        es = es->next;
    }
    if(!es) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_USERLAND,
                       "No Interrupt EventSource configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_InterruptManager *im = (UA_InterruptManager*)es;

    /* Register the interrupt */
    struct InterruptContext ic;
    ic.server = server;
    ic.running = true;
    UA_StatusCode retval =
        im->registerInterrupt(im, SIGINT, &UA_KEYVALUEMAP_NULL,
                              interruptServer, &ic);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_USERLAND,
                     "Could not register the interrupt with status code %s",
                     UA_StatusCode_name(retval));
        return retval;
    }

    /* Run the server */
    retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        goto deregister_interrupt;
    while(ic.running) {
        UA_Server_run_iterate(server, true);
    }

    /* Shut down the server */
    retval = UA_Server_run_shutdown(server);

    /* Deregister the interrupt */
 deregister_interrupt:
    im->deregisterInterrupt(im, SIGINT);
    return retval;
}

#endif /* defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) */

/*******************************/
/* Default Connection Settings */
/*******************************/

const UA_ConnectionConfig UA_ConnectionConfig_default = {
    0,       /* .protocolVersion */
    1 << 16, /* .sendBufferSize, 64k per chunk */
    1 << 16, /* .recvBufferSize, 64k per chunk */
    1 << 29, /* .localMaxMessageSize, 512 MB */
    1 << 29, /* .remoteMaxMessageSize, 512 MB */
    1 << 14, /* .localMaxChunkCount, 16k */
    1 << 14  /* .remoteMaxChunkCount, 16k */
};

/***************************/
/* Default Server Settings */
/***************************/

#define MANUFACTURER_NAME "open62541"
#define PRODUCT_NAME "open62541 OPC UA Server"
#define PRODUCT_URI "http://open62541.org"
#define APPLICATION_NAME "open62541-based OPC UA Application"
#define APPLICATION_URI "urn:open62541.unconfigured.application"

/* Upper bound */
#define SECURITY_POLICY_SIZE 7

#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

static UA_StatusCode
addEndpoint(UA_ServerConfig *conf,
            const UA_SecurityPolicy *securityPolicy,
            UA_MessageSecurityMode securityMode) {
    /* Test if the endpoint already exists */
    for(size_t i = 0; i < conf->endpointsSize; i++) {
        UA_EndpointDescription *ep = &conf->endpoints[i];
        if(!UA_String_equal(&securityPolicy->policyUri, &ep->securityPolicyUri))
            continue;
        if(ep->securityMode != securityMode)
            continue;
        return UA_STATUSCODE_GOOD;
    }

    /* Reallocate the array size */
    UA_EndpointDescription *tmp = (UA_EndpointDescription *)
        UA_realloc(conf->endpoints,
                   sizeof(UA_EndpointDescription) * (1 + conf->endpointsSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    conf->endpoints = tmp;

    /* The following fields are overwritten internally with up-to-date
     * information from the server config:
     *
     * UserTokenPolicies
     * ApplicationDescription (server)
     * ServerCertificate
     * EndpointURL */
    UA_EndpointDescription *endpoint = &conf->endpoints[conf->endpointsSize];
    UA_EndpointDescription_init(endpoint);
    endpoint->transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");
    endpoint->securityMode = securityMode;
    endpoint->securityLevel = securityPolicy->securityLevel;

    UA_StatusCode retval = UA_String_copy(&securityPolicy->policyUri,
                                          &endpoint->securityPolicyUri);

    if(retval == UA_STATUSCODE_GOOD) {
        conf->endpointsSize++;
    } else {
        UA_EndpointDescription_clear(endpoint);
        if(conf->endpointsSize == 0) {
            UA_free(conf->endpoints);
            conf->endpoints = NULL;
        }
    }

    return retval;
}

static UA_StatusCode
setDefaultConfig(UA_ServerConfig *conf, UA_UInt16 portNumber) {
    if(!conf)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* NodeStore */
    if(!conf->nodestore)
        conf->nodestore = UA_Nodestore_ZipTree();

    /* Logging */
    if(conf->logging == NULL)
        conf->logging = UA_Log_Stdout_new(UA_LOGLEVEL_INFO);

    /* EventLoop */
    if(conf->eventLoop == NULL) {
#if defined(UA_ARCHITECTURE_ZEPHYR)
        conf->eventLoop = UA_EventLoop_new_Zephyr(conf->logging);
#elif defined(UA_ARCHITECTURE_LWIP)
        conf->eventLoop = UA_EventLoop_new_LWIP(conf->logging, NULL);
#else
        conf->eventLoop = UA_EventLoop_new_POSIX(conf->logging);
#endif
        if(conf->eventLoop == NULL)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        conf->externalEventLoop = false;

        /* Add the TCP connection manager */
#if defined(UA_ARCHITECTURE_ZEPHYR)
        UA_ConnectionManager *tcpCM =
            UA_ConnectionManager_new_Zephyr_TCP(UA_STRING("tcp connection manager"));
#elif defined(UA_ARCHITECTURE_LWIP)
        UA_ConnectionManager *tcpCM =
            UA_ConnectionManager_new_LWIP_TCP(UA_STRING("tcp connection manager"));
#else
        UA_ConnectionManager *tcpCM =
            UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcp connection manager"));
#endif
        if(tcpCM)
            conf->eventLoop->registerEventSource(conf->eventLoop, (UA_EventSource *)tcpCM);

        /* Add the UDP connection manager */
#if defined(UA_ARCHITECTURE_LWIP)
        UA_ConnectionManager *udpCM =
            UA_ConnectionManager_new_LWIP_UDP(UA_STRING("udp connection manager"));
        if(udpCM)
            conf->eventLoop->registerEventSource(conf->eventLoop, (UA_EventSource *)udpCM);
#elif !defined(UA_ARCHITECTURE_ZEPHYR)
        UA_ConnectionManager *udpCM =
            UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp connection manager"));
        if(udpCM)
            conf->eventLoop->registerEventSource(conf->eventLoop, (UA_EventSource *)udpCM);
#endif

        /* Add the Ethernet connection manager */
#if !defined(UA_ARCHITECTURE_ZEPHYR) && !defined(UA_ARCHITECTURE_LWIP) && defined(UA_ARCHITECTURE_POSIX) && (defined(__linux__))
        UA_ConnectionManager *ethCM =
            UA_ConnectionManager_new_POSIX_Ethernet(UA_STRING("eth connection manager"));
        if(ethCM)
            conf->eventLoop->registerEventSource(conf->eventLoop, (UA_EventSource *)ethCM);
#endif

#if !defined(UA_ARCHITECTURE_ZEPHYR) && !defined(UA_ARCHITECTURE_LWIP)
        /* Add the interrupt manager */
        UA_InterruptManager *im = UA_InterruptManager_new_POSIX(UA_STRING("interrupt manager"));
        if(im) {
            conf->eventLoop->registerEventSource(conf->eventLoop, &im->eventSource);
        } else {
            UA_LOG_ERROR(conf->logging, UA_LOGCATEGORY_USERLAND,
                         "Cannot create the Interrupt Manager (only relevant if used)");
        }
#endif
#ifdef UA_ENABLE_MQTT
        /* Add the MQTT connection manager */
        UA_ConnectionManager *mqttCM =
            UA_ConnectionManager_new_MQTT(UA_STRING("mqtt connection manager"));
        if(mqttCM)
            conf->eventLoop->registerEventSource(conf->eventLoop, (UA_EventSource *)mqttCM);
#endif
    }
    if(conf->eventLoop != NULL) {
        if(conf->eventLoop->state != UA_EVENTLOOPSTATE_STARTED) {
            UA_StatusCode statusCode = conf->eventLoop->start(conf->eventLoop);
            if(statusCode != UA_STATUSCODE_GOOD) {
                return statusCode;
            }
        }
    }

    /* If a second server is started later it can "steal" the port.
     * Having port reuse enabled is important for development.
     * Otherwise a long TCP TIME_WAIT is required before the port can be used again. */
    conf->tcpReuseAddr = true;

    /* --> Start setting the default static config <-- */

    conf->shutdownDelay = 0.0;

    /* Server Description */
    UA_BuildInfo_clear(&conf->buildInfo);
    conf->buildInfo.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    conf->buildInfo.manufacturerName = UA_STRING_ALLOC(MANUFACTURER_NAME);
    conf->buildInfo.productName = UA_STRING_ALLOC(PRODUCT_NAME);
    conf->buildInfo.softwareVersion =
        UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));
    conf->buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);
    conf->buildInfo.buildDate = UA_DateTime_now();

    UA_ApplicationDescription_clear(&conf->applicationDescription);
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
    UA_MdnsDiscoveryConfiguration_clear(&conf->mdnsConfig);
# ifdef UA_ENABLE_DISCOVERY_MULTICAST_MDNSD
    conf->mdnsInterfaceIP = UA_STRING_NULL;
#  if !defined(UA_HAS_GETIFADDR)
    conf->mdnsIpAddressList = NULL;
    conf->mdnsIpAddressListSize = 0;
#  endif
# endif
#endif

    /* Custom DataTypes */
    /* conf->customDataTypesSize = 0; */
    /* conf->customDataTypes = NULL; */

    /* Networking */
    /* Set up the local ServerUrls. They are used during startup to initialize
     * the server sockets. */
    UA_String serverUrls[1];
    size_t serverUrlsSize = 0;
    char serverUrlBuffer[1][512];

    if(portNumber == 0) {
        UA_LOG_WARNING(conf->logging, UA_LOGCATEGORY_USERLAND,
                       "Dynamic port assignment will be used.");
    }

    if(conf->serverUrlsSize > 0) {
        UA_LOG_WARNING(conf->logging, UA_LOGCATEGORY_USERLAND,
                       "ServerUrls already set. Overriding.");
        UA_Array_delete(conf->serverUrls, conf->serverUrlsSize,
                        &UA_TYPES[UA_TYPES_STRING]);
        conf->serverUrls = NULL;
        conf->serverUrlsSize = 0;
    }

    /* Listen on all interfaces (also external). This must be the first
     * entry if this is desired. Otherwise some interfaces may be blocked
     * (already in use) with a hostname that is only locally reachable.*/
    mp_snprintf(serverUrlBuffer[0], sizeof(serverUrlBuffer[0]),
                "opc.tcp://:%u", portNumber);
    serverUrls[serverUrlsSize] = UA_STRING(serverUrlBuffer[0]);
    serverUrlsSize++;

    /* Add to the config */
    UA_StatusCode retval =
        UA_Array_copy(serverUrls, serverUrlsSize,
                      (void**)&conf->serverUrls, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    conf->serverUrlsSize = serverUrlsSize;

    /* Endpoints */
    /* conf->endpoints = {0, NULL}; */

    /* Set Logger for Certificate Verification */
    if(!conf->secureChannelPKI.logging)
        conf->secureChannelPKI.logging = conf->logging;
    if(!conf->sessionPKI.logging)
        conf->sessionPKI.logging = conf->logging;

#ifdef UA_ENABLE_ENCRYPTION
    /* Limits for TrustList */
    conf->maxTrustListSize = 0;
    conf->maxRejectedListSize = 0;
#endif

    /* Certificate Verification that accepts every certificate. Can be
     * overwritten when the policy is specialized. */
    UA_CertificateGroup_AcceptAll(&conf->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&conf->sessionPKI);

    /* * Global Node Lifecycle * */
    /* conf->nodeLifecycle.constructor = NULL; */
    /* conf->nodeLifecycle.destructor = NULL; */
    /* conf->nodeLifecycle.createOptionalChild = NULL; */
    /* conf->nodeLifecycle.generateChildNodeId = NULL; */
    conf->modellingRulesOnInstances = true;

    /* Limits for SecureChannels */
    conf->maxSecureChannels = 100;
    conf->maxSecurityTokenLifetime = 10 * 60 * 1000; /* 10 minutes */

    /* Limits for Sessions */
    conf->maxSessions = 100;
    conf->maxSessionTimeout = 60.0 * 60.0 * 1000.0; /* 1h */

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Limits for Subscriptions */
    conf->publishingIntervalLimits = UA_DURATIONRANGE(100.0, 3600.0 * 1000.0);
    conf->lifeTimeCountLimits = UA_UINT32RANGE(3, 15000);
    conf->keepAliveCountLimits = UA_UINT32RANGE(1, 100);
    conf->maxNotificationsPerPublish = 1000;
    conf->enableRetransmissionQueue = true;
    conf->maxRetransmissionQueueSize = 0; /* unlimited */
# ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    conf->maxEventsPerNode = 0; /* unlimited */
# endif

    /* Limits for MonitoredItems */
    conf->samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
    conf->queueSizeLimits = UA_UINT32RANGE(1, 100);
#endif

#ifdef UA_ENABLE_DISCOVERY
    conf->discoveryCleanupTimeout = 60 * 60;
#endif

#ifdef UA_ENABLE_HISTORIZING
    /* conf->accessHistoryDataCapability = false; */
    /* conf->maxReturnDataValues = 0; */

    /* conf->accessHistoryEventsCapability = false; */
    /* conf->maxReturnEventValues = 0; */

    /* conf->insertDataCapability = false; */
    /* conf->insertEventCapability = false; */
    /* conf->insertAnnotationsCapability = false; */

    /* conf->replaceDataCapability = false; */
    /* conf->replaceEventCapability = false; */

    /* conf->updateDataCapability = false; */
    /* conf->updateEventCapability = false; */

    /* conf->deleteRawCapability = false; */
    /* conf->deleteEventCapability = false; */
    /* conf->deleteAtTimeDataCapability = false; */
#endif

#if UA_MULTITHREADING >= 100
    conf->maxAsyncOperationQueueSize = 0;
    conf->asyncOperationTimeout = 120000; /* Async Operation Timeout in ms (2 minutes) */
#endif

#ifdef UA_ENABLE_PUBSUB
    conf->pubsubEnabled = true;
    conf->pubSubConfig.enableDeltaFrames = true;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    conf->pubSubConfig.enableInformationModelMethods = true;
#endif
#endif

    /* --> Finish setting the default static config <-- */

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setBasics(UA_ServerConfig* conf) {
    return UA_ServerConfig_setBasics_withPort(conf, 4840);
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setBasics_withPort(UA_ServerConfig* conf, UA_UInt16 portNumber) {
    return setDefaultConfig(conf, portNumber);
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyNone(UA_ServerConfig *config,
                                      const UA_ByteString *certificate) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;

    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    UA_StatusCode retval =
        UA_SecurityPolicy_None(&config->securityPolicies[config->securityPoliciesSize],
                               localCertificate, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addEndpoint(UA_ServerConfig *config, const UA_String securityPolicyUri,
                            UA_MessageSecurityMode securityMode) {
    /* Lookup the security policy */
    const UA_SecurityPolicy *policy = NULL;
    for (size_t i = 0; i < config->securityPoliciesSize; ++i) {
        if (UA_String_equal(&securityPolicyUri, &config->securityPolicies[i].policyUri)) {
            policy = &config->securityPolicies[i];
            break;
        }
    }
    if(!policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Populate the endpoint */
    return addEndpoint(config, policy, securityMode);
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addAllEndpoints(UA_ServerConfig *config) {
    /* Populate the endpoints */
    for(size_t i = 0; i < config->securityPoliciesSize; ++i) {
        if(UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &config->securityPolicies[i].policyUri)) {
            UA_StatusCode retval =
                addEndpoint(config, &config->securityPolicies[i], UA_MESSAGESECURITYMODE_NONE);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        } else {
            UA_StatusCode retval =
                addEndpoint(config, &config->securityPolicies[i], UA_MESSAGESECURITYMODE_SIGN);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
            retval = addEndpoint(config, &config->securityPolicies[i],
                                 UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addAllSecureEndpoints(UA_ServerConfig *config) {

    /* Delete all predefined endpoints. */
    if(config->endpointsSize > 0) {
        for(size_t i = 0; i < config->endpointsSize; ++i)
            UA_EndpointDescription_clear(&config->endpoints[i]);

        UA_free(config->endpoints);
        config->endpoints = NULL;
        config->endpointsSize = 0;
    }

    /* Populate the endpoints */
    for(size_t i = 0; i < config->securityPoliciesSize; ++i) {
        /* Skip the None and all deprecated policies */
        if(config->securityPolicies[i].securityLevel == 0)
            continue;
        UA_StatusCode retval =
            addEndpoint(config, &config->securityPolicies[i], UA_MESSAGESECURITYMODE_SIGN);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = addEndpoint(config, &config->securityPolicies[i],
                             UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setMinimalCustomBuffer(UA_ServerConfig *config, UA_UInt16 portNumber,
                                       const UA_ByteString *certificate,
                                       UA_UInt32 sendBufferSize,
                                       UA_UInt32 recvBufferSize) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = setDefaultConfig(config, portNumber);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(config);
        return retval;
    }

    /* Set the TCP settings */
    config->tcpBufSize = recvBufferSize;

    /* Allocate the SecurityPolicies */
    retval = UA_ServerConfig_addSecurityPolicyNone(config, certificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(config);
        return retval;
    }

    /* Initialize the Access Control plugin */
    retval = UA_AccessControl_default(config, true, NULL, 0, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(config);
        return retval;
    }

    /* Allocate the endpoint */
    retval = UA_ServerConfig_addEndpoint(config, UA_SECURITY_POLICY_NONE_URI,
                                         UA_MESSAGESECURITYMODE_NONE);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(config);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_ENCRYPTION

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic128Rsa15(UA_ServerConfig *config,
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
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
    UA_StatusCode retval =
        UA_SecurityPolicy_Basic128Rsa15(&config->securityPolicies[config->securityPoliciesSize],
                                        localCertificate, localPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic256(UA_ServerConfig *config,
                                          const UA_ByteString *certificate,
                                          const UA_ByteString *privateKey) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
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
    UA_StatusCode retval =
        UA_SecurityPolicy_Basic256(&config->securityPolicies[config->securityPoliciesSize],
                                   localCertificate, localPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic256Sha256(UA_ServerConfig *config,
                                                const UA_ByteString *certificate,
                                                const UA_ByteString *privateKey) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
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
    UA_StatusCode retval =
        UA_SecurityPolicy_Basic256Sha256(&config->securityPolicies[config->securityPoliciesSize],
                                         localCertificate, localPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(UA_ServerConfig *config,
                                                const UA_ByteString *certificate,
                                                const UA_ByteString *privateKey) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
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
    UA_StatusCode retval =
        UA_SecurityPolicy_Aes128Sha256RsaOaep(&config->securityPolicies[config->securityPoliciesSize],
                                              localCertificate, localPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(UA_ServerConfig *config,
                                                     const UA_ByteString *certificate,
                                                     const UA_ByteString *privateKey) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
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
    UA_StatusCode retval =
        UA_SecurityPolicy_Aes256Sha256RsaPss(&config->securityPolicies[config->securityPoliciesSize],
                                              localCertificate, localPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyEccNistP256(UA_ServerConfig *config,
                                                     const UA_ByteString *certificate,
                                                     const UA_ByteString *privateKey) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
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
    UA_StatusCode retval =
        UA_SecurityPolicy_EccNistP256(&config->securityPolicies[config->securityPoliciesSize],
                                              UA_APPLICATIONTYPE_SERVER, localCertificate,
                                              localPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}
#endif


/* This assumes that the sp array has enough space for up to SECURITY_POLICY_SIZE policies.
 * Policies that have been added are not cleaned up after an error.
 * Certificate and/or pricateKey can be NULL and will not be used then.
 * Logs a warning if policies could not be added. */
static void
addAllSecurityPolicies(UA_SecurityPolicy *sp, size_t *length,
                       const UA_ByteString certificate, const UA_ByteString privateKey,
                       UA_Boolean onlySecure, UA_ApplicationType applicationType,
                       UA_Logger *logging) {
    /* Basic256Sha256 */
    UA_StatusCode retval = UA_SecurityPolicy_Basic256Sha256(sp + *length, certificate, privateKey, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Aes256Sha256RsaPss */
    retval = UA_SecurityPolicy_Aes256Sha256RsaPss(sp + *length, certificate, privateKey, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Aes128Sha256RsaOaep */
    retval = UA_SecurityPolicy_Aes128Sha256RsaOaep(sp + *length, certificate, privateKey, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                       UA_StatusCode_name(retval));
    }

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
    /* EccNistP256 */
    retval = UA_SecurityPolicy_EccNistP256(sp + *length, applicationType,
                                           certificate, privateKey, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#EccNistP256 with error code %s",
                       UA_StatusCode_name(retval));
    }
#endif

    /* Don't add "unsecure" SecurityPolicies */
    if(onlySecure)
        return;

    /* None */
    retval = UA_SecurityPolicy_None(sp + *length, certificate, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#None with error code %s",
                       UA_StatusCode_name(retval));
    }

#ifdef UA_INCLUDE_INSECURE_POLICIES
    /* Basic128Rsa15 should no longer be used */
    retval = UA_SecurityPolicy_Basic128Rsa15(sp + *length, certificate, privateKey, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic128Rsa15 with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Basic256 should no longer be used */
    retval = UA_SecurityPolicy_Basic256(sp + *length, certificate, privateKey, logging);
    *length += (retval == UA_STATUSCODE_GOOD) ? 1 : 0;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic256 with error code %s",
                       UA_StatusCode_name(retval));
    }
#endif
}

static UA_StatusCode
addAllServerSecurityPolicies(UA_ServerConfig *config,
                             const UA_ByteString *certificate, const UA_ByteString *privateKey,
                             UA_Boolean onlySecure) {
    /* Populate the cert variables */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;
    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
       localPrivateKey = *privateKey;

    /* Load the private key and convert to the DER format. Use an empty password
     * on the first try -- maybe the key does not require a password. */
    UA_ByteString decryptedPrivateKey = UA_BYTESTRING_NULL;
    UA_ByteString keyPassword = UA_BYTESTRING_NULL;
    UA_StatusCode keySuccess = UA_STATUSCODE_GOOD;
    if(privateKey && privateKey->length > 0)
        keySuccess = UA_CertificateUtils_decryptPrivateKey(localPrivateKey, keyPassword,
                                                           &decryptedPrivateKey);

    /* Get the password and decrypt. An application might want to loop / retry
     * here to allow users to correct their entry. */
    if(keySuccess != UA_STATUSCODE_GOOD) {
        if(config->privateKeyPasswordCallback)
            keySuccess = config->privateKeyPasswordCallback(config, &keyPassword);
        else
            keySuccess = readPrivateKeyPassword(&keyPassword);
        if(keySuccess != UA_STATUSCODE_GOOD)
            return keySuccess;
        keySuccess = UA_CertificateUtils_decryptPrivateKey(localPrivateKey, keyPassword,
                                                           &decryptedPrivateKey);
        UA_ByteString_memZero(&keyPassword);
        UA_ByteString_clear(&keyPassword);
    }
    if(keySuccess != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_USERLAND,
                     "Could not decrypt the private key with status code %s",
                     UA_StatusCode_name(keySuccess));
        return keySuccess;
    }

    /* Clear up the old SecurityPolicies */
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        config->securityPolicies[i].clear(&config->securityPolicies[i]);
    }

    /* Allocate memory for the additional policies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy*)
        UA_realloc(config->securityPolicies, sizeof(UA_SecurityPolicy) * SECURITY_POLICY_SIZE);
    if(!tmp) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    config->securityPolicies = tmp;
    config->securityPoliciesSize = 0;

    /* Do the generic addition of certificates */
    addAllSecurityPolicies(config->securityPolicies, &config->securityPoliciesSize,
                           localCertificate, localPrivateKey, onlySecure,
                           UA_APPLICATIONTYPE_SERVER, config->logging);

    if(config->securityPoliciesSize == 0) {
        UA_free(config->securityPolicies);
        config->securityPolicies = NULL;
    }

    UA_ByteString_memZero(&decryptedPrivateKey);
    UA_ByteString_clear(&decryptedPrivateKey);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ServerConfig_addAllSecurityPolicies(UA_ServerConfig *config,
                                       const UA_ByteString *certificate,
                                       const UA_ByteString *privateKey) {
    return addAllServerSecurityPolicies(config, certificate, privateKey, false);
}

/* Always returns UA_STATUSCODE_GOOD. Logs a warning if policies could not be added. */
UA_StatusCode
UA_ServerConfig_addAllSecureSecurityPolicies(UA_ServerConfig *config,
                                             const UA_ByteString *certificate,
                                             const UA_ByteString *privateKey) {
    return addAllServerSecurityPolicies(config, certificate, privateKey, true);
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setDefaultWithSecurityPolicies(UA_ServerConfig *conf,
                                               UA_UInt16 portNumber,
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey,
                                               const UA_ByteString *trustList,
                                               size_t trustListSize,
                                               const UA_ByteString *issuerList,
                                               size_t issuerListSize,
                                               const UA_ByteString *revocationList,
                                               size_t revocationListSize) {
    UA_StatusCode retval = setDefaultConfig(conf, portNumber);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(conf);
        return retval;
    }

    if(trustListSize > 0) {
        UA_TrustListDataType list;
        UA_TrustListDataType_init(&list);
        list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
        retval = UA_Array_copy(trustList, trustListSize,
                               (void**)&list.trustedCertificates,
                               &UA_TYPES[UA_TYPES_BYTESTRING]);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        list.trustedCertificatesSize = trustListSize;

        if(issuerListSize > 0) {
            list.specifiedLists |= UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
            retval = UA_Array_copy(issuerList, issuerListSize,
                                   (void**)&list.issuerCertificates,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_TrustListDataType_clear(&list);
                return retval;
            }
            list.issuerCertificatesSize = issuerListSize;
        }

        if(revocationListSize > 0) {
            list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCRLS;
            retval = UA_Array_copy(revocationList, revocationListSize,
                                   (void**)&list.trustedCrls,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_TrustListDataType_clear(&list);
                return retval;
            }
            list.trustedCrlsSize = revocationListSize;
        }

        /* Set up the parameters */
        UA_KeyValuePair params[2];
        size_t paramsSize = 2;

        params[0].key = UA_QUALIFIEDNAME(0, "max-trust-listsize");
        UA_Variant_setScalar(&params[0].value, &conf->maxTrustListSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
        params[1].key = UA_QUALIFIEDNAME(0, "max-rejected-listsize");
        UA_Variant_setScalar(&params[1].value, &conf->maxRejectedListSize,
                             &UA_TYPES[UA_TYPES_UINT32]);

        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        UA_NodeId defaultApplicationGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
        retval = UA_CertificateGroup_Memorystore(&conf->secureChannelPKI,
                                                 &defaultApplicationGroup, &list,
                                                 conf->logging, &paramsMap);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_TrustListDataType_clear(&list);
            return retval;
        }

        UA_NodeId defaultUserTokenGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
        retval = UA_CertificateGroup_Memorystore(&conf->sessionPKI,
                                                 &defaultUserTokenGroup, &list,
                                                 conf->logging, &paramsMap);
        UA_TrustListDataType_clear(&list);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    } else {
        UA_LOG_ERROR(conf->logging, UA_LOGCATEGORY_USERLAND,
                     "Empty trustlist passed. Leaving the previously "
                     "configured certificate verification in place");
    }

    retval = UA_ServerConfig_addAllSecurityPolicies(conf, certificate, privateKey);

    /* Reinitialize AccessControl to take the changed SecurityPolicies into account
     * TODO: This looses username/pw during the reinitialization */
    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_AccessControl_default(conf, true, NULL, 0, NULL);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(conf);
        return retval;
    }

    retval = UA_ServerConfig_addAllEndpoints(conf);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(conf);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setDefaultWithSecureSecurityPolicies(UA_ServerConfig *conf,
                                               UA_UInt16 portNumber,
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey,
                                               const UA_ByteString *trustList,
                                               size_t trustListSize,
                                               const UA_ByteString *issuerList,
                                               size_t issuerListSize,
                                               const UA_ByteString *revocationList,
                                               size_t revocationListSize) {
    UA_StatusCode retval = setDefaultConfig(conf, portNumber);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(conf);
        return retval;
    }

    if(trustListSize > 0) {
        UA_TrustListDataType list;
        UA_TrustListDataType_init(&list);
        list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
        retval = UA_Array_copy(trustList, trustListSize,
                               (void**)&list.trustedCertificates,
                               &UA_TYPES[UA_TYPES_BYTESTRING]);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        list.trustedCertificatesSize = trustListSize;

        if(issuerListSize > 0) {
            list.specifiedLists |= UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
            retval = UA_Array_copy(issuerList, issuerListSize,
                                   (void**)&list.issuerCertificates,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_TrustListDataType_clear(&list);
                return retval;
            }
            list.issuerCertificatesSize = issuerListSize;
        }

        if(revocationListSize > 0) {
            list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCRLS;
            retval = UA_Array_copy(revocationList, revocationListSize,
                                   (void**)&list.trustedCrls,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_TrustListDataType_clear(&list);
                return retval;
            }
            list.trustedCrlsSize = revocationListSize;
        }

        /* Set up the parameters */
        UA_KeyValuePair params[2];
        size_t paramsSize = 2;

        params[0].key = UA_QUALIFIEDNAME(0, "max-trust-listsize");
        UA_Variant_setScalar(&params[0].value, &conf->maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);
        params[1].key = UA_QUALIFIEDNAME(0, "max-rejected-listsize");
        UA_Variant_setScalar(&params[1].value, &conf->maxRejectedListSize, &UA_TYPES[UA_TYPES_UINT32]);

        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        UA_NodeId defaultApplicationGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
        retval = UA_CertificateGroup_Memorystore(&conf->secureChannelPKI, &defaultApplicationGroup,
                                                 &list, conf->logging, &paramsMap);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_TrustListDataType_clear(&list);
            return retval;
        }

        UA_NodeId defaultUserTokenGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
        retval = UA_CertificateGroup_Memorystore(&conf->sessionPKI, &defaultUserTokenGroup,
                                                 &list, conf->logging, &paramsMap);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_TrustListDataType_clear(&list);
            return retval;
        }
        UA_TrustListDataType_clear(&list);
    } else {
        UA_LOG_ERROR(conf->logging, UA_LOGCATEGORY_USERLAND,
                     "Empty trustlist passed. Leaving the previously "
                     "configured certificate verification in place");
    }

    retval = UA_ServerConfig_addAllSecureSecurityPolicies(conf, certificate, privateKey);

    if(retval == UA_STATUSCODE_GOOD) {
        retval = UA_AccessControl_default(conf, false, NULL, 0, NULL);
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(conf);
        return retval;
    }

    retval = UA_ServerConfig_addAllSecureEndpoints(conf);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(conf);
        return retval;
    }

    return UA_STATUSCODE_GOOD;
}

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)

UA_StatusCode
UA_ServerConfig_addSecurityPolicy_Filestore(UA_ServerConfig *config,
                                            UA_SecurityPolicy *innerPolicy,
                                            const UA_String storePath) {
    /* Allocate the SecurityPolicies */
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * (1 + config->securityPoliciesSize));
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = tmp;

    UA_StatusCode retval =
        UA_SecurityPolicy_Filestore(&config->securityPolicies[config->securityPoliciesSize],
                                    innerPolicy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        if(config->securityPoliciesSize == 0) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
        }
        return retval;
    }

    config->securityPoliciesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ServerConfig_addSecurityPolicies_Filestore(UA_ServerConfig *config,
                                              const UA_ByteString *certificate,
                                              const UA_ByteString *privateKey,
                                              const UA_String storePath) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Boolean onlySecure = false;
    UA_Boolean onlyNone = false;

    /* Populate the SecurityPolicies */
    UA_ByteString localCertificate = UA_BYTESTRING_NULL;
    UA_ByteString localPrivateKey  = UA_BYTESTRING_NULL;

    if(certificate)
        localCertificate = *certificate;
    if(privateKey)
        localPrivateKey = *privateKey;

    if(certificate && privateKey) {
        size_t certificateKeyLength = 0;
        retval = UA_CertificateUtils_getKeySize((UA_ByteString*)(uintptr_t)certificate, &certificateKeyLength);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        if(certificateKeyLength > 2048)
            onlySecure = true;
    } else {
        onlyNone = true;
    }

    /* Load the private key and convert to the DER format. Use an empty password
     * on the first try -- maybe the key does not require a password. */
    UA_ByteString decryptedPrivateKey = UA_BYTESTRING_NULL;
    UA_ByteString keyPassword = UA_BYTESTRING_NULL;
    UA_StatusCode keySuccess = UA_STATUSCODE_GOOD;

    if(privateKey && privateKey->length > 0)
        keySuccess = UA_CertificateUtils_decryptPrivateKey(localPrivateKey, keyPassword,
                                              &decryptedPrivateKey);

    /* Get the password and decrypt. An application might want to loop / retry
     * here to allow users to correct their entry. */
    if(keySuccess != UA_STATUSCODE_GOOD) {
        if(config->privateKeyPasswordCallback)
            keySuccess = config->privateKeyPasswordCallback(config, &keyPassword);
        else
            keySuccess = readPrivateKeyPassword(&keyPassword);
        if(keySuccess != UA_STATUSCODE_GOOD)
            return keySuccess;
        keySuccess = UA_CertificateUtils_decryptPrivateKey(localPrivateKey, keyPassword,
                                              &decryptedPrivateKey);
        UA_ByteString_memZero(&keyPassword);
        UA_ByteString_clear(&keyPassword);
    }
    if(keySuccess != UA_STATUSCODE_GOOD)
        return keySuccess;

    if(onlyNone) {
        /* None */
        UA_SecurityPolicy *nonePolicy =
            (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
        if(!nonePolicy) {
            UA_ByteString_memZero(&decryptedPrivateKey);
            UA_ByteString_clear(&decryptedPrivateKey);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        retval = UA_SecurityPolicy_None(nonePolicy, localCertificate, config->logging);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                           "Could not add SecurityPolicy#None with error code %s",
                           UA_StatusCode_name(retval));
            nonePolicy->clear(nonePolicy);
            UA_free(nonePolicy);
            nonePolicy = NULL;
        }
        retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, nonePolicy, storePath);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                           "Could not add SecurityPolicy#None with error code %s",
                           UA_StatusCode_name(retval));
        }
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return retval;
    }

    /* Basic256Sha256 */
    UA_SecurityPolicy *basic256Sha256Policy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!basic256Sha256Policy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_Basic256Sha256(basic256Sha256Policy, localCertificate,
                                              decryptedPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                       UA_StatusCode_name(retval));
        basic256Sha256Policy->clear(basic256Sha256Policy);
        UA_free(basic256Sha256Policy);
        basic256Sha256Policy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, basic256Sha256Policy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Aes256Sha256RsaPss */
    UA_SecurityPolicy *aes256Sha256RsaPssPolicy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!aes256Sha256RsaPssPolicy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_Aes256Sha256RsaPss(aes256Sha256RsaPssPolicy, localCertificate,
                                                  decryptedPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                       UA_StatusCode_name(retval));
        aes256Sha256RsaPssPolicy->clear(aes256Sha256RsaPssPolicy);
        UA_free(aes256Sha256RsaPssPolicy);
        aes256Sha256RsaPssPolicy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, aes256Sha256RsaPssPolicy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Aes128Sha256RsaOaep */
    UA_SecurityPolicy *aes128Sha256RsaOaepPolicy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!aes128Sha256RsaOaepPolicy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_Aes128Sha256RsaOaep(aes128Sha256RsaOaepPolicy, localCertificate,
                                                   decryptedPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                       UA_StatusCode_name(retval));
        aes128Sha256RsaOaepPolicy->clear(aes128Sha256RsaOaepPolicy);
        UA_free(aes128Sha256RsaOaepPolicy);
        aes128Sha256RsaOaepPolicy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, aes128Sha256RsaOaepPolicy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                       UA_StatusCode_name(retval));
    }

    if(onlySecure) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_GOOD;
    }

    /* None */
    UA_SecurityPolicy *nonePolicy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!nonePolicy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_None(nonePolicy, localCertificate, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#None with error code %s",
                       UA_StatusCode_name(retval));
        nonePolicy->clear(nonePolicy);
        UA_free(nonePolicy);
        nonePolicy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, nonePolicy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#None with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Basic128Rsa15 */
    UA_SecurityPolicy *basic128Rsa15Policy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!basic128Rsa15Policy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_Basic128Rsa15(basic128Rsa15Policy, localCertificate,
                                             decryptedPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic128Rsa15 with error code %s",
                       UA_StatusCode_name(retval));
        basic128Rsa15Policy->clear(basic128Rsa15Policy);
        UA_free(basic128Rsa15Policy);
        basic128Rsa15Policy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, basic128Rsa15Policy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic128Rsa15 with error code %s",
                       UA_StatusCode_name(retval));
    }

    /* Basic256 */
    UA_SecurityPolicy *basic256Policy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!basic256Policy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_Basic256(basic256Policy, localCertificate,
                                        decryptedPrivateKey, config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic256 with error code %s",
                       UA_StatusCode_name(retval));
        basic256Policy->clear(basic256Policy);
        UA_free(basic256Policy);
        basic256Policy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, basic256Policy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#Basic256 with error code %s",
                       UA_StatusCode_name(retval));
    }

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
    /* EccNistP256 */
    UA_SecurityPolicy *eccnistp256Policy =
        (UA_SecurityPolicy*)UA_calloc(1, sizeof(UA_SecurityPolicy));
    if(!eccnistp256Policy) {
        UA_ByteString_memZero(&decryptedPrivateKey);
        UA_ByteString_clear(&decryptedPrivateKey);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval = UA_SecurityPolicy_EccNistP256(eccnistp256Policy, UA_APPLICATIONTYPE_SERVER,
                                        localCertificate, decryptedPrivateKey,
                                        config->logging);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#ECC_nistP256 with error code %s",
                       UA_StatusCode_name(retval));
        eccnistp256Policy->clear(eccnistp256Policy);
        UA_free(eccnistp256Policy);
        eccnistp256Policy = NULL;
    }
    retval = UA_ServerConfig_addSecurityPolicy_Filestore(config, eccnistp256Policy, storePath);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                       "Could not add SecurityPolicy#ECC_nistP256 with error code %s",
                       UA_StatusCode_name(retval));
    }
#endif

    UA_ByteString_memZero(&decryptedPrivateKey);
    UA_ByteString_clear(&decryptedPrivateKey);
    return UA_STATUSCODE_GOOD;
}

UA_EXPORT UA_StatusCode
UA_ServerConfig_setDefaultWithFilestore(UA_ServerConfig *conf,
                                        UA_UInt16 portNumber,
                                        const UA_ByteString *certificate,
                                        const UA_ByteString *privateKey,
                                        const UA_String storePath) {
    UA_StatusCode retval = setDefaultConfig(conf, portNumber);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    if(!storePath.data) {
        UA_LOG_ERROR(conf->logging, UA_LOGCATEGORY_USERLAND,
                     "The path to a PKI folder has not been specified");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Set up the parameters */
    UA_KeyValuePair params[2];
    size_t paramsSize = 2;

    params[0].key = UA_QUALIFIEDNAME(0, "max-trust-listsize");
    UA_Variant_setScalar(&params[0].value, &conf->maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);
    params[1].key = UA_QUALIFIEDNAME(0, "max-rejected-listsize");
    UA_Variant_setScalar(&params[1].value, &conf->maxRejectedListSize, &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = paramsSize;

    UA_NodeId defaultApplicationGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    retval = UA_CertificateGroup_Filestore(&conf->secureChannelPKI, &defaultApplicationGroup,
                                           storePath, conf->logging, &paramsMap);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_NodeId defaultUserTokenGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    retval = UA_CertificateGroup_Filestore(&conf->sessionPKI, &defaultUserTokenGroup,
                                           storePath, conf->logging, &paramsMap);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_ServerConfig_addSecurityPolicies_Filestore(conf, certificate, privateKey, storePath);

    if(retval == UA_STATUSCODE_GOOD) {
        retval = UA_AccessControl_default(conf, true, NULL, 0, NULL);
    }

    if(retval == UA_STATUSCODE_GOOD) {
        retval = UA_ServerConfig_addAllEndpoints(conf);
    }

    return retval;
}

#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

#endif /* UA_ENABLE_ENCRYPTION */

/***************************/
/* Default Client Settings */
/***************************/

UA_Client * UA_Client_new(void) {
    UA_ClientConfig config;
    memset(&config, 0, sizeof(UA_ClientConfig));
    /* Set up basic usable config including logger and event loop */
    UA_StatusCode res = UA_ClientConfig_setDefault(&config);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;
    return UA_Client_newWithConfig(&config);
}

UA_StatusCode
UA_ClientConfig_setDefault(UA_ClientConfig *config) {
    /* The following fields are untouched and OK to leave as NULL or 0:
     *  clientContext
     *  userIdentityToken
     *  securityMode
     *  securityPolicyUri
     *  endpoint
     *  userTokenPolicy
     *  customDataTypes
     *  connectivityCheckInterval
     *  stateCallback
     *  inactivityCallback
     *  outStandingPublishRequests
     *  subscriptionInactivityCallback
     *  sessionLocaleIds
     *  sessionLocaleIdsSize */

    if(config->timeout == 0)
        config->timeout = 5 * 1000; /* 5 seconds */
    if(config->secureChannelLifeTime == 0)
        config->secureChannelLifeTime = 10 * 60 * 1000; /* 10 minutes */

    if(config->logging == NULL)
        config->logging = UA_Log_Stdout_new(UA_LOGLEVEL_INFO);

    /* EventLoop */
    if(config->eventLoop == NULL) {
#if defined(UA_ARCHITECTURE_ZEPHYR)
        config->eventLoop = UA_EventLoop_new_Zephyr(config->logging);
#elif defined(UA_ARCHITECTURE_LWIP)
        config->eventLoop = UA_EventLoop_new_LWIP(config->logging, NULL);
#else
        config->eventLoop = UA_EventLoop_new_POSIX(config->logging);
#endif
        config->externalEventLoop = false;

        /* Add the TCP connection manager */
#if defined(UA_ARCHITECTURE_ZEPHYR)
        UA_ConnectionManager *tcpCM =
            UA_ConnectionManager_new_Zephyr_TCP(UA_STRING("tcp connection manager"));
#elif defined(UA_ARCHITECTURE_LWIP)
        UA_ConnectionManager *tcpCM =
            UA_ConnectionManager_new_LWIP_TCP(UA_STRING("tcp connection manager"));
#else
        UA_ConnectionManager *tcpCM =
            UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcp connection manager"));
#endif
        config->eventLoop->registerEventSource(config->eventLoop, (UA_EventSource *)tcpCM);

#if defined(UA_ARCHITECTURE_LWIP)
        UA_ConnectionManager *udpCM =
            UA_ConnectionManager_new_LWIP_UDP(UA_STRING("udp connection manager"));
        if(udpCM)
            config->eventLoop->registerEventSource(config->eventLoop, (UA_EventSource *)udpCM);
#elif !defined(UA_ARCHITECTURE_ZEPHYR)
        /* Add the UDP connection manager */
        UA_ConnectionManager *udpCM =
            UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp connection manager"));
        config->eventLoop->registerEventSource(config->eventLoop, (UA_EventSource *)udpCM);
#endif
    }

    if(config->localConnectionConfig.recvBufferSize == 0)
        config->localConnectionConfig = UA_ConnectionConfig_default;

    if(!config->certificateVerification.logging) {
        config->certificateVerification.logging = config->logging;
    }

#ifdef UA_ENABLE_ENCRYPTION
    /* Limits for TrustList */
    config->maxTrustListSize = 0;
    config->maxRejectedListSize = 0;
#endif

    if(!config->certificateVerification.verifyCertificate) {
        /* Certificate Verification that accepts every certificate. Can be
         * overwritten when the policy is specialized. */
        UA_CertificateGroup_AcceptAll(&config->certificateVerification);
    }

    /* With encryption enabled, the applicationUri needs to match the URI from
     * the certificate */
    if(!config->clientDescription.applicationUri.data)
        config->clientDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);
    if(config->clientDescription.applicationType == 0)
        config->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;

    if(config->securityPoliciesSize == 0) {
        config->securityPolicies = (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy));
        if(!config->securityPolicies)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        UA_StatusCode retval = UA_SecurityPolicy_None(config->securityPolicies,
                                                      UA_BYTESTRING_NULL, config->logging);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_free(config->securityPolicies);
            config->securityPolicies = NULL;
            return retval;
        }
        config->securityPoliciesSize = 1;
    }

    if(config->requestedSessionTimeout == 0)
        config->requestedSessionTimeout = 1200000;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    if(config->outStandingPublishRequests == 0)
        config->outStandingPublishRequests = 10;
#endif

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_ENCRYPTION

static UA_StatusCode
clientConfig_setAuthenticationSecurityPolicies(UA_ClientConfig *config,
                                               UA_ByteString certificateAuth,
                                               UA_ByteString privateKeyAuth) {
    for(size_t i = 0; i < config->authSecurityPoliciesSize; i++) {
        config->authSecurityPolicies[i].clear(&config->authSecurityPolicies[i]);
    }

    UA_SecurityPolicy *sp = (UA_SecurityPolicy*)
        UA_realloc(config->authSecurityPolicies, sizeof(UA_SecurityPolicy) * SECURITY_POLICY_SIZE);
    if(!sp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->authSecurityPolicies = sp;
    config->authSecurityPoliciesSize = 0;

    addAllSecurityPolicies(sp, &config->authSecurityPoliciesSize,
                           certificateAuth, privateKeyAuth, false,
                           UA_APPLICATIONTYPE_CLIENT, config->logging);

    if(config->authSecurityPoliciesSize == 0) {
        UA_free(config->authSecurityPolicies);
        config->authSecurityPolicies = NULL;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
clientConfig_setSecurityPolicies(UA_ClientConfig *config,
                                 UA_ByteString certificateAuth,
                                 UA_ByteString privateKeyAuth) {
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        config->securityPolicies[i].clear(&config->securityPolicies[i]);
    }

    UA_SecurityPolicy *sp = (UA_SecurityPolicy*)
        UA_realloc(config->securityPolicies,
                   sizeof(UA_SecurityPolicy) * SECURITY_POLICY_SIZE);
    if(!sp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    config->securityPolicies = sp;
    config->securityPoliciesSize = 0;

    addAllSecurityPolicies(sp, &config->securityPoliciesSize,
                           certificateAuth, privateKeyAuth, false,
                           UA_APPLICATIONTYPE_CLIENT, config->logging);

    if(config->securityPoliciesSize == 0) {
        UA_free(config->securityPolicies);
        config->securityPolicies = NULL;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ClientConfig_setDefaultEncryption(UA_ClientConfig *config,
                                     UA_ByteString localCertificate, UA_ByteString privateKey,
                                     const UA_ByteString *trustList, size_t trustListSize,
                                     const UA_ByteString *revocationList, size_t revocationListSize) {
    UA_StatusCode retval = UA_ClientConfig_setDefault(config);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(trustListSize > 0) {
        UA_TrustListDataType list;
        UA_TrustListDataType_init(&list);
        list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
        retval = UA_Array_copy(trustList, trustListSize,
                               (void**)&list.trustedCertificates,
                               &UA_TYPES[UA_TYPES_BYTESTRING]);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        list.trustedCertificatesSize = trustListSize;

        if(revocationListSize > 0) {
            list.specifiedLists |= UA_TRUSTLISTMASKS_TRUSTEDCRLS;
            retval = UA_Array_copy(revocationList, revocationListSize,
                                   (void**)&list.trustedCrls,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_TrustListDataType_clear(&list);
                return retval;
            }
            list.trustedCrlsSize = revocationListSize;
        }

        /* Set up the parameters */
        UA_KeyValuePair params[2];
        size_t paramsSize = 2;

        params[0].key = UA_QUALIFIEDNAME(0, "max-trust-listsize");
        UA_Variant_setScalar(&params[0].value, &config->maxTrustListSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
        params[1].key = UA_QUALIFIEDNAME(0, "max-rejected-listsize");
        UA_Variant_setScalar(&params[1].value, &config->maxRejectedListSize,
                             &UA_TYPES[UA_TYPES_UINT32]);

        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        if(config->certificateVerification.clear)
            config->certificateVerification.clear(&config->certificateVerification);
        UA_NodeId defaultApplicationGroup =
            UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
        retval = UA_CertificateGroup_Memorystore(&config->certificateVerification,
                                                 &defaultApplicationGroup, &list,
                                                 config->logging, &paramsMap);
        UA_TrustListDataType_clear(&list);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    } else {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Empty trustlist passed. Leaving the previously "
                     "configured certificate verification in place");
    }

    /* Load the private key and convert to the DER format. Use an empty password
     * on the first try -- maybe the key does not require a password. */
    UA_ByteString decryptedPrivateKey = UA_BYTESTRING_NULL;
    UA_ByteString keyPassword = UA_BYTESTRING_NULL;
    UA_StatusCode keySuccess = UA_STATUSCODE_GOOD;

    if(privateKey.length > 0)
        keySuccess = UA_CertificateUtils_decryptPrivateKey(privateKey, keyPassword,
                                                           &decryptedPrivateKey);

    /* Get the password and decrypt. An application might want to loop / retry
     * here to allow users to correct their entry. */
    if(keySuccess != UA_STATUSCODE_GOOD) {
        if(config->privateKeyPasswordCallback)
            keySuccess = config->privateKeyPasswordCallback(config, &keyPassword);
        else
            keySuccess = readPrivateKeyPassword(&keyPassword);
        if(keySuccess != UA_STATUSCODE_GOOD)
            return keySuccess;
        keySuccess = UA_CertificateUtils_decryptPrivateKey(privateKey, keyPassword, &decryptedPrivateKey);
        UA_ByteString_memZero(&keyPassword);
        UA_ByteString_clear(&keyPassword);
    }
    if(keySuccess != UA_STATUSCODE_GOOD)
        return keySuccess;

    clientConfig_setSecurityPolicies(config, localCertificate, decryptedPrivateKey);
    clientConfig_setAuthenticationSecurityPolicies(config, localCertificate, decryptedPrivateKey);

    UA_ByteString_memZero(&decryptedPrivateKey);
    UA_ByteString_clear(&decryptedPrivateKey);

    return UA_STATUSCODE_GOOD;
}
#endif

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
UA_StatusCode
UA_ClientConfig_setAuthenticationCert(UA_ClientConfig *config,
                                      UA_ByteString certificateAuth,
                                      UA_ByteString privateKeyAuth) {
#ifdef UA_ENABLE_ENCRYPTION_LIBRESSL
    UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_USERLAND,
                 "Certificate authentication with LibreSSL as crypto backend is not supported.");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
#endif

    /* Create UserIdentityToken */
    UA_X509IdentityToken* identityToken = UA_X509IdentityToken_new();
    if(!identityToken)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    /* Don't set identityToken->policyId. This is taken from the appropriate
     * endpoint at runtime. */
    UA_StatusCode retval = UA_ByteString_copy(&certificateAuth, &identityToken->certificateData);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_ExtensionObject_clear(&config->userIdentityToken);
    config->userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    config->userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN];
    config->userIdentityToken.content.decoded.data = identityToken;

    /* Populate SecurityPolicies */
    return clientConfig_setAuthenticationSecurityPolicies(config, certificateAuth, privateKeyAuth);
}
#endif
