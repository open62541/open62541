/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_SERVER_CONFIG_H_
#define UA_SERVER_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_plugin_log.h"
#include "ua_plugin_network.h"
#include "ua_plugin_access_control.h"
#include "ua_plugin_securitypolicy.h"

/**
 * Server Configuration
 * ====================
 * The configuration structure is passed to the server during initialization. */

typedef struct {
    UA_UInt32 min;
    UA_UInt32 max;
} UA_UInt32Range;

typedef struct {
    UA_Double min;
    UA_Double max;
} UA_DoubleRange;

struct UA_ServerConfig {
    UA_UInt16 nThreads; /* only if multithreading is enabled */
    UA_Logger logger;

    /* Server Description */
    UA_BuildInfo buildInfo;
    UA_ApplicationDescription applicationDescription;
    UA_ByteString serverCertificate;
#ifdef UA_ENABLE_DISCOVERY
    UA_String mdnsServerName;
    size_t serverCapabilitiesSize;
    UA_String *serverCapabilities;
#endif

    /* Custom DataTypes */
    size_t customDataTypesSize;
    const UA_DataType *customDataTypes;

    /* Networking */
    size_t networkLayersSize;
    UA_ServerNetworkLayer *networkLayers;

    /* Available endpoints */
    UA_Endpoints endpoints;

    /* Global Node Lifecycle */
    UA_GlobalNodeLifecycle nodeLifecycle;

    /* Access Control */
    UA_AccessControl accessControl;

    /* Limits for SecureChannels */
    UA_UInt16 maxSecureChannels;
    UA_UInt32 maxSecurityTokenLifetime; /* in ms */

    /* Limits for Sessions */
    UA_UInt16 maxSessions;
    UA_Double maxSessionTimeout; /* in ms */

    /* Limits for Subscriptions */
    UA_DoubleRange publishingIntervalLimits;
    UA_UInt32Range lifeTimeCountLimits;
    UA_UInt32Range keepAliveCountLimits;
    UA_UInt32 maxNotificationsPerPublish;
    UA_UInt32 maxRetransmissionQueueSize; /* 0 -> unlimited size */

    /* Limits for MonitoredItems */
    UA_DoubleRange samplingIntervalLimits;
    UA_UInt32Range queueSizeLimits; /* Negotiated with the client */

    /* Discovery */
#ifdef UA_ENABLE_DISCOVERY
    /* Timeout in seconds when to automatically remove a registered server from
     * the list, if it doesn't re-register within the given time frame. A value
     * of 0 disables automatic removal. Default is 60 Minutes (60*60). Must be
     * bigger than 10 seconds, because cleanup is only triggered approximately
     * ervery 10 seconds. The server will still be removed depending on the
     * state of the semaphore file. */
    UA_UInt32 discoveryCleanupTimeout;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* UA_SERVER_CONFIG_H_ */
