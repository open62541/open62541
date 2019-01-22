/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 */

#ifndef UA_CLIENT_CONFIG_H
#define UA_CLIENT_CONFIG_H

#include "ua_config.h"
#include "ua_plugin_network.h"
#include "ua_plugin_network_manager.h"

_UA_BEGIN_DECLS

struct UA_Client;
typedef struct UA_Client UA_Client;

/**
 * .. _client-config:
 *
 * Client Configuration
 * --------------------
 *
 * The client configuration is used for setting connection parameters and
 * additional settings used by the client.
 * The configuration should not be modified after it is passed to a client.
 * Currently, only one client can use a configuration at a time.
 *
 * Examples for configurations are provided in the ``/plugins`` folder.
 * The usual usage is as follows:
 *
 * 1. Create a client configuration with default settings as a starting point
 * 2. Modifiy the configuration, e.g. modifying the timeout
 * 3. Instantiate a client with it
 * 4. After shutdown of the client, clean up the configuration (free memory)
 *
 * The :ref:`tutorials` provide a good starting point for this. */

typedef enum {
    UA_CLIENTSTATE_DISCONNECTED,         /* The client is disconnected */
    UA_CLIENTSTATE_WAITING_FOR_ACK,      /* The Client has sent HEL and waiting */
    UA_CLIENTSTATE_CONNECTED,            /* A TCP connection to the server is open */
    UA_CLIENTSTATE_SECURECHANNEL,        /* A SecureChannel to the server is open */
    UA_CLIENTSTATE_SESSION,              /* A session with the server is open */
    UA_CLIENTSTATE_SESSION_DISCONNECTED, /* Disconnected vs renewed? */
    UA_CLIENTSTATE_SESSION_RENEWED       /* A session with the server is open (renewed) */
} UA_ClientState;

typedef struct {
    UA_SocketConfig socketConfig;

    const char *endpointUrl;
    UA_UInt32 timeout;
    UA_SocketHook openHook;
} UA_ClientSocketConfig;

typedef struct UA_ClientConfig UA_ClientConfig;

struct UA_ClientConfig {
    UA_UInt32 timeout;               /* ASync + Sync response timeout in ms */
    UA_UInt32 secureChannelLifeTime; /* Lifetime in ms (then the channel needs
                                        to be renewed) */
    UA_Logger logger;
    UA_ConnectionConfig localConnectionConfig;

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
    UA_StatusCode (*configureNetworkManager)(const UA_ClientConfig *config, UA_NetworkManager *networkManager);
    UA_ClientSocketConfig clientSocketConfig;

    /* Custom DataTypes. Attention! Custom datatypes are not cleaned up together
     * with the configuration. So it is possible to allocate them on ROM. */
    const UA_DataTypeArray *customDataTypes;

    /* Callback for state changes */
    void (*stateCallback)(UA_Client *client, UA_ClientState clientState);

   /* Connectivity check interval in ms.
    * 0 = background task disabled */
    UA_UInt32 connectivityCheckInterval;

    /* session timeout in ms */
    UA_UInt32 requestedSessionTimeout;

    /* When connectivityCheckInterval is greater than 0, every
     * connectivityCheckInterval (in ms), a async read request is performed on
     * the server. inactivityCallback is called when the client receive no
     * response for this read request The connection can be closed, this in an
     * attempt to recreate a healthy connection. */
    void (*inactivityCallback)(UA_Client *client);

    void *clientContext;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Number of PublishResponse queued up in the server */
    UA_UInt16 outStandingPublishRequests;

    /* If the client does not receive a PublishResponse after the defined delay
     * of ``(sub->publishingInterval * sub->maxKeepAliveCount) +
     * client->config.timeout)``, then subscriptionInactivityCallback is called
     * for the subscription.. */
    void (*subscriptionInactivityCallback)(UA_Client *client,
                                           UA_UInt32 subscriptionId,
                                           void *subContext);
#endif
};

_UA_END_DECLS

#endif /* UA_CLIENT_CONFIG_H */
