/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 */

#ifndef UA_SERVER_CONFIG_H_
#define UA_SERVER_CONFIG_H_

#include "ua_server.h"
#include "ua_plugin_log.h"
#include "ua_plugin_network.h"
#include "ua_plugin_access_control.h"
#include "ua_plugin_pki.h"
#include "ua_plugin_securitypolicy.h"
#include "ua_plugin_nodestore.h"

#ifdef UA_ENABLE_PUBSUB
#include "ua_plugin_pubsub.h"
#endif

#ifdef UA_ENABLE_HISTORIZING
#include "ua_plugin_historydatabase.h"
#include "ua_plugin_network_manager.h"
#endif

_UA_BEGIN_DECLS

/**
 * .. _server-configuration:
 *
 * Server Configuration
 * --------------------

 * The configuration structure is passed to the server during initialization.
 * The server expects that the configuration is not modified during runtime.
 * Currently, only one server can use a configuration at a time. During
 * shutdown, the server will clean up the parts of the configuration that are
 * modified at runtime through the provided API.
 *
 * Examples for configurations are provided in the ``/plugins`` folder.
 * The usual usage is as follows:
 *
 * 1. Create a server configuration with default settings as a starting point
 * 2. Modifiy the configuration, e.g. by adding a server certificate
 * 3. Instantiate a server with it
 * 4. After shutdown of the server, clean up the configuration (free memory)
 *
 * The :ref:`tutorials` provide a good starting point for this. */

typedef struct {
    UA_UInt32 min;
    UA_UInt32 max;
} UA_UInt32Range;

typedef struct {
    UA_Duration min;
    UA_Duration max;
} UA_DurationRange;

struct UA_ServerConfig {
    UA_UInt16 nThreads; /* only if multithreading is enabled */
    UA_Logger logger;

    /* Server Description */
    UA_BuildInfo buildInfo;
    UA_ApplicationDescription applicationDescription;
    UA_ByteString serverCertificate;

    /* MDNS Discovery */
#ifdef UA_ENABLE_DISCOVERY
    UA_String mdnsServerName;
    size_t serverCapabilitiesSize;
    UA_String *serverCapabilities;
#endif

    /* Custom DataTypes. Attention! Custom datatypes are not cleaned up together
     * with the configuration. So it is possible to allocate them on ROM. */
    const UA_DataTypeArray *customDataTypes;

    /**
     * .. note:: See the section on :ref:`generic-types`. Examples for working
     *    with custom data types are provided in
     *    ``/examples/custom_datatype/``. */

    /* Nodestore */
    UA_Nodestore nodestore;

    /* Networking */
    /**
     * This function is called by the server to create a networkManager.
     * This enables configuration to be done on the user side in the config.
     * The delayed configuration makes sure, that initialization is done during
     * server startup. Also, only the server will have ownership of the NetworkManager.
     *
     * @param config The configuration.
     * @param networkManager The networkManager to initialize.
     */
    UA_StatusCode (*configureNetworkManager)(const UA_ServerConfig *config, UA_NetworkManager *networkManager);

    /**
     * One ore more sockets will be created for each socket config depending on the
     * createSocket function.
     */
    UA_SocketConfig *socketConfigs;
    size_t socketConfigsSize;

    /**
     * The connection config contains parameters for all connections created by the server.
     */
    UA_ConnectionConfig connectionConfig;

#ifdef UA_ENABLE_PUBSUB
    /*PubSub network layer */
    size_t pubsubTransportLayersSize;
    UA_PubSubTransportLayer *pubsubTransportLayers;
#endif

    /* Available security policies */
    size_t securityPoliciesSize;
    UA_SecurityPolicy* securityPolicies;

    /* Available endpoints */
    size_t endpointsSize;
    UA_EndpointDescription *endpoints;

    /* Node Lifecycle callbacks */
    UA_GlobalNodeLifecycle nodeLifecycle;
    /**
     * .. note:: See the section for :ref:`node lifecycle
     *    handling<node-lifecycle>`. */

    /* Access Control */
    UA_AccessControl accessControl;
    /**
     * .. note:: See the section for :ref:`access-control
     *    handling<access-control>`. */

    /* Certificate Verification */
    UA_CertificateVerification certificateVerification;

    /* Relax constraints for the InformationModel */
    UA_Boolean relaxEmptyValueConstraint; /* Nominally, only variables with data
                                           * type BaseDataType can have an empty
                                           * value. */

    /* Limits for SecureChannels */
    UA_UInt16 maxSecureChannels;
    UA_UInt32 maxSecurityTokenLifetime; /* in ms */

    /* Limits for Sessions */
    UA_UInt16 maxSessions;
    UA_Double maxSessionTimeout; /* in ms */

    /* Operation limits */
    UA_UInt32 maxNodesPerRead;
    UA_UInt32 maxNodesPerWrite;
    UA_UInt32 maxNodesPerMethodCall;
    UA_UInt32 maxNodesPerBrowse;
    UA_UInt32 maxNodesPerRegisterNodes;
    UA_UInt32 maxNodesPerTranslateBrowsePathsToNodeIds;
    UA_UInt32 maxNodesPerNodeManagement;
    UA_UInt32 maxMonitoredItemsPerCall;

    /* Limits for Requests */
    UA_UInt32 maxReferencesPerNode;

    /* Limits for Subscriptions */
    UA_UInt32 maxSubscriptionsPerSession;
    UA_DurationRange publishingIntervalLimits; /* in ms (must not be less than 5) */
    UA_UInt32Range lifeTimeCountLimits;
    UA_UInt32Range keepAliveCountLimits;
    UA_UInt32 maxNotificationsPerPublish;
    UA_UInt32 maxRetransmissionQueueSize; /* 0 -> unlimited size */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_UInt32 maxEventsPerNode; /* 0 -> unlimited size */
#endif

    /* Limits for MonitoredItems */
    UA_UInt32 maxMonitoredItemsPerSubscription;
    UA_DurationRange samplingIntervalLimits; /* in ms (must not be less than 5) */
    UA_UInt32Range queueSizeLimits; /* Negotiated with the client */

    /* Limits for PublishRequests */
    UA_UInt32 maxPublishReqPerSession;

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

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Register MonitoredItem in Userland
     *
     * @param server Allows the access to the server object
     * @param sessionId The session id, represented as an node id
     * @param sessionContext An optional pointer to user-defined data for the specific data source
     * @param nodeid Id of the node in question
     * @param nodeidContext An optional pointer to user-defined data, associated
     *        with the node in the nodestore. Note that, if the node has already been removed,
     *        this value contains a NULL pointer.
     * @param attributeId Identifies which attribute (value, data type etc.) is monitored
     * @param removed Determines if the MonitoredItem was removed or created. */
    void (*monitoredItemRegisterCallback)(UA_Server *server,
                                          const UA_NodeId *sessionId, void *sessionContext,
                                          const UA_NodeId *nodeId, void *nodeContext,
                                          UA_UInt32 attibuteId, UA_Boolean removed);
#endif

    /* Historical Access */
#ifdef UA_ENABLE_HISTORIZING
    UA_HistoryDatabase historyDatabase;
    
    UA_Boolean accessHistoryDataCapability;
    UA_UInt32  maxReturnDataValues; /* 0 -> unlimited size */
    
    UA_Boolean accessHistoryEventsCapability;
    UA_UInt32  maxReturnEventValues; /* 0 -> unlimited size */

    UA_Boolean insertDataCapability;
    UA_Boolean insertEventCapability;
    UA_Boolean insertAnnotationsCapability;

    UA_Boolean replaceDataCapability;
    UA_Boolean replaceEventCapability;
    
    UA_Boolean updateDataCapability;
    UA_Boolean updateEventCapability;
    
    UA_Boolean deleteRawCapability;
    UA_Boolean deleteEventCapability;
    UA_Boolean deleteAtTimeDataCapability;
#endif
};

_UA_END_DECLS

#endif /* UA_SERVER_CONFIG_H_ */
