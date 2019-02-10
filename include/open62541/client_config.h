/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 */

#ifndef UA_CLIENT_CONFIG_H
#define UA_CLIENT_CONFIG_H

#include <open62541/config.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/network.h>
#include <open62541/plugin/securitypolicy.h>

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
    /* Basic client configuration */
    void *clientContext; /* User-defined data attached to the client */
    UA_Logger logger;   /* Logger used by the client */
    UA_UInt32 timeout;  /* Response timeout in ms */
    UA_ApplicationDescription clientDescription;

    /* Basic connection configuration */
    UA_ExtensionObject userIdentityToken; /* Configured User-Identity Token */
    UA_MessageSecurityMode securityMode;  /* None, Sign, SignAndEncrypt. The
                                           * default is invalid. This indicates
                                           * the client to select any matching
                                           * endpoint. */
    UA_String securityPolicyUri; /* SecurityPolicy for the SecureChannel. An
                                  * empty string indicates the client to select
                                  * any matching SecurityPolicy. */

    /* Advanced connection configuration
     *
     * If either endpoint or userTokenPolicy has been set (at least one non-zero
     * byte in either structure), then the selected Endpoint and UserTokenPolicy
     * overwrite the settings in the basic connection configuration. The
     * userTokenPolicy array in the EndpointDescription is ignored. The selected
     * userTokenPolicy is set in the dedicated configuration field.
     *
     * If the advanced configuration is not set, the client will write to it the
     * selected Endpoint and UserTokenPolicy during GetEndpoints.
     *
     * The information in the advanced configuration is used during reconnect
     * when the SecureChannel was broken. */
    UA_EndpointDescription endpoint;
    UA_UserTokenPolicy userTokenPolicy;

    /* Advanced client configuration */

    UA_UInt32 secureChannelLifeTime; /* Lifetime in ms (then the channel needs
                                        to be renewed) */
    UA_UInt32 requestedSessionTimeout; /* Session timeout in ms */
    UA_ConnectionConfig localConnectionConfig;
    UA_UInt32 connectivityCheckInterval;     /* Connectivity check interval in ms.
                                              * 0 = background task disabled */
    const UA_DataTypeArray *customDataTypes; /* Custom DataTypes. Attention!
                                              * Custom datatypes are not cleaned
                                              * up together with the
                                              * configuration. So it is possible
                                              * to allocate them on ROM. */

    /* Available SecurityPolicies */
    size_t securityPoliciesSize;
    UA_SecurityPolicy *securityPolicies;

    /* Certificate Verification Plugin */
    UA_CertificateVerification certificateVerification;

    /* Callbacks for async connection handshakes */
    UA_ConnectClientConnection connectionFunc;
    UA_ConnectClientConnection initConnectionFunc;
    void (*pollConnectionFunc)(UA_Client *client, void *context);

    /* Callback for state changes */
    void (*stateCallback)(UA_Client *client, UA_ClientState clientState);

    /* When connectivityCheckInterval is greater than 0, every
     * connectivityCheckInterval (in ms), a async read request is performed on
     * the server. inactivityCallback is called when the client receive no
     * response for this read request The connection can be closed, this in an
     * attempt to recreate a healthy connection. */
    void (*inactivityCallback)(UA_Client *client);

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
} UA_ClientConfig;

_UA_END_DECLS

#endif /* UA_CLIENT_CONFIG_H */
