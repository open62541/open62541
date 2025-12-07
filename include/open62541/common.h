/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2016-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016 (c) Sten Grüner
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#ifndef UA_COMMON_H_
#define UA_COMMON_H_

#include <open62541/config.h>
#include <open62541/nodeids.h>

_UA_BEGIN_DECLS

/**
 * .. _common:
 *
 * Common Definitions
 * ==================
 *
 * Common definitions for Client, Server and PubSub.
 *
 * .. _attribute-id:
 *
 * Attribute Id
 * ------------
 * Every node in an OPC UA information model contains attributes depending on
 * the node type. Possible attributes are as follows: */

typedef enum {
    UA_ATTRIBUTEID_INVALID                 = 0,
    UA_ATTRIBUTEID_NODEID                  = 1,
    UA_ATTRIBUTEID_NODECLASS               = 2,
    UA_ATTRIBUTEID_BROWSENAME              = 3,
    UA_ATTRIBUTEID_DISPLAYNAME             = 4,
    UA_ATTRIBUTEID_DESCRIPTION             = 5,
    UA_ATTRIBUTEID_WRITEMASK               = 6,
    UA_ATTRIBUTEID_USERWRITEMASK           = 7,
    UA_ATTRIBUTEID_ISABSTRACT              = 8,
    UA_ATTRIBUTEID_SYMMETRIC               = 9,
    UA_ATTRIBUTEID_INVERSENAME             = 10,
    UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
    UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
    UA_ATTRIBUTEID_VALUE                   = 13,
    UA_ATTRIBUTEID_DATATYPE                = 14,
    UA_ATTRIBUTEID_VALUERANK               = 15,
    UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16,
    UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
    UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
    UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
    UA_ATTRIBUTEID_HISTORIZING             = 20,
    UA_ATTRIBUTEID_EXECUTABLE              = 21,
    UA_ATTRIBUTEID_USEREXECUTABLE          = 22,
    UA_ATTRIBUTEID_DATATYPEDEFINITION      = 23,
    UA_ATTRIBUTEID_ROLEPERMISSIONS         = 24,
    UA_ATTRIBUTEID_USERROLEPERMISSIONS     = 25,
    UA_ATTRIBUTEID_ACCESSRESTRICTIONS      = 26,
    UA_ATTRIBUTEID_ACCESSLEVELEX           = 27
} UA_AttributeId;

/**
 * Returns a readable attribute name like "NodeId" or "Invalid" if the attribute
 * does not exist. */

UA_EXPORT const char *
UA_AttributeId_name(UA_AttributeId attrId);

/**
 * .. _access-level-mask:
 *
 * Access Level Masks
 * ------------------
 * The access level to a node is given by the following constants that are ANDed
 * with the overall access level. */

#define UA_ACCESSLEVELMASK_READ           (0x01u << 0u)
#define UA_ACCESSLEVELMASK_CURRENTREAD    (0x01u << 0u)
#define UA_ACCESSLEVELMASK_WRITE          (0x01u << 1u)
#define UA_ACCESSLEVELMASK_CURRENTWRITE   (0x01u << 1u)
#define UA_ACCESSLEVELMASK_HISTORYREAD    (0x01u << 2u)
#define UA_ACCESSLEVELMASK_HISTORYWRITE   (0x01u << 3u)
#define UA_ACCESSLEVELMASK_SEMANTICCHANGE (0x01u << 4u)
#define UA_ACCESSLEVELMASK_STATUSWRITE    (0x01u << 5u)
#define UA_ACCESSLEVELMASK_TIMESTAMPWRITE (0x01u << 6u)

/**
 * .. _write-mask:
 *
 * Write Masks
 * -----------
 * The write mask and user write mask is given by the following constants that
 * are ANDed for the overall write mask. Part 3: 5.2.7 Table 2 */

#define UA_WRITEMASK_ACCESSLEVEL             (0x01u << 0u)
#define UA_WRITEMASK_ARRAYDIMENSIONS         (0x01u << 1u)
#define UA_WRITEMASK_BROWSENAME              (0x01u << 2u)
#define UA_WRITEMASK_CONTAINSNOLOOPS         (0x01u << 3u)
#define UA_WRITEMASK_DATATYPE                (0x01u << 4u)
#define UA_WRITEMASK_DESCRIPTION             (0x01u << 5u)
#define UA_WRITEMASK_DISPLAYNAME             (0x01u << 6u)
#define UA_WRITEMASK_EVENTNOTIFIER           (0x01u << 7u)
#define UA_WRITEMASK_EXECUTABLE              (0x01u << 8u)
#define UA_WRITEMASK_HISTORIZING             (0x01u << 9u)
#define UA_WRITEMASK_INVERSENAME             (0x01u << 10u)
#define UA_WRITEMASK_ISABSTRACT              (0x01u << 11u)
#define UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL (0x01u << 12u)
#define UA_WRITEMASK_NODECLASS               (0x01u << 13u)
#define UA_WRITEMASK_NODEID                  (0x01u << 14u)
#define UA_WRITEMASK_SYMMETRIC               (0x01u << 15u)
#define UA_WRITEMASK_USERACCESSLEVEL         (0x01u << 16u)
#define UA_WRITEMASK_USEREXECUTABLE          (0x01u << 17u)
#define UA_WRITEMASK_USERWRITEMASK           (0x01u << 18u)
#define UA_WRITEMASK_VALUERANK               (0x01u << 19u)
#define UA_WRITEMASK_WRITEMASK               (0x01u << 20u)
#define UA_WRITEMASK_VALUEFORVARIABLETYPE    (0x01u << 21u)
#define UA_WRITEMASK_DATATYPEDEFINITION      (0x01u << 22u)
#define UA_WRITEMASK_ROLEPERMISSIONS         (0x01u << 23u)
#define UA_WRITEMASK_ACCESSRESTRICTIONS      (0x01u << 24u)
#define UA_WRITEMASK_ACCESSLEVELEX           (0x01u << 25u)

/**
 * .. _valuerank-defines:
 *
 * ValueRank
 * ---------
 * The following are the most common ValueRanks used for Variables,
 * VariableTypes and method arguments. ValueRanks higher than 3 are valid as
 * well (but less common). */

#define UA_VALUERANK_SCALAR_OR_ONE_DIMENSION  -3
#define UA_VALUERANK_ANY                      -2
#define UA_VALUERANK_SCALAR                   -1
#define UA_VALUERANK_ONE_OR_MORE_DIMENSIONS    0
#define UA_VALUERANK_ONE_DIMENSION             1
#define UA_VALUERANK_TWO_DIMENSIONS            2
#define UA_VALUERANK_THREE_DIMENSIONS          3

/**
 * .. _eventnotifier:
 *
 * EventNotifier
 * -------------
 * The following are the available EventNotifier used for Nodes.
 * The EventNotifier Attribute is used to indicate if the Node can be used
 * to subscribe to Events or to read / write historic Events.
 * Part 3: 5.4 Table 10 */

#define UA_EVENTNOTIFIER_SUBSCRIBE_TO_EVENT (0x01u << 0u)
#define UA_EVENTNOTIFIER_HISTORY_READ       (0x01u << 2u)
#define UA_EVENTNOTIFIER_HISTORY_WRITE      (0x01u << 3u)

/**
 * .. _rule-handling:
 *
 * Rule Handling
 * -------------
 * The RuleHanding settings define how error cases that result from rules in the
 * OPC UA specification shall be handled. The rule handling can be softened,
 * e.g. to workaround misbehaving implementations or to mitigate the impact of
 * additional rules that are introduced in later versions of the OPC UA
 * specification. */
typedef enum {
    UA_RULEHANDLING_DEFAULT = 0,
    UA_RULEHANDLING_ABORT,  /* Abort the operation and return an error code */
    UA_RULEHANDLING_WARN,   /* Print a message in the logs and continue */
    UA_RULEHANDLING_ACCEPT, /* Continue and disregard the broken rule */
} UA_RuleHandling;

/**
 * Order
 * -----
 * The Order enum is used to establish an absolute ordering between elements.
 */

typedef enum {
    UA_ORDER_LESS = -1,
    UA_ORDER_EQ = 0,
    UA_ORDER_MORE = 1
} UA_Order;

/**
 * .. _application-notification:
 *
 * Application Notification
 * ------------------------
 *
 * The ApplicationNotification enum indicates the type of notification for the
 * server/client in which the corresponding callback is configured.
 *
 * The notification comes with a key-value map for the payload. Future
 * additional payload members are added to the end of the payload. So that the
 * names, type and also index of the payload members is stable. */

typedef enum {
    /* Lifetime notifications, no payload */
    UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STARTED,
    UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_SHUTDOWN, /* preparing shutdown */
    UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPING, /* shutdown begins now */
    UA_APPLICATIONNOTIFICATIONTYPE_LIFECYCLE_STOPPED,

    /* (Server only) Give background information after a SecureChannel is opened
     * or closed.
     *
     * 0:securechannel-id [UInt32]
     *    Identifier of the SecureChannel to which the Session is connected.
     * 0:connection-manager-name [String]
     *    Name of the ConnectionManager (configured in the EventLoop) from which
     *    the connction was opened.
     * 0:connection-id [UInt64]
     *    Identifier of the connection in the context of the EventLoop. This is
     *    often the socket identifier, but that is not necessarily the case.
     * 0:remote-addresss [String]
     *   Address (hostname or IP that opened the SecureChannel.
     *
     * 0:protocol-version [UInt32]
     *   The version of the UACP protocol requested by the Client.
     * 0:recv-buffer-size [UInt32]
     *   The largest buffer (chunk size) we can receive over the channel.
     * 0:recv-max-message-size [UInt32]
     *   The maximum size of received messages.
     * 0:recv-max-chunk-count [UInt32]
     *   The maximum number of chunks for received messages.
     * 0:send-buffer-size [UInt32]
     *   The largest buffer (chunk size) we can send over the channel.
     * 0:send-max-message-size [UInt32]
     *   The maximum size of sent messages.
     * 0:send-max-chunk-count [UInt32]
     *   The maximum number of chunks for sent messages.
     * 0:endpoint-url
     *   The target EndpointUri (for the server) indicated by the client.
     *
     * 0:security-mode [MessageSecurityMode]
     *   The SecurityChannel can be unsigned, signed or signed+encrypted.
     * 0:security-policy-uri [String]
     *   Uri of the SecurityPolicy for this SecyrityChannel.
     * 0:remote-certificate [ByteString]
     *   Certificate used by the remote side during OpenSecureChannel. */
    UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED,
    UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_CLOSED,

    /* (Server only) Give background information for Sessions. The _DEACTIVATE
     * notification occurs when a Sesssion is unbound from its original
     * SecureChannel. Either because the SecureChannel is closed or because the
     * session is activated on another SecureChannel.
     *
     * 0:session-id [NodeId]
     *   Identifier of the Session.
     * 0:securechannel-id [UInt32]
     *   Identifier of the SecureChannel on which the Session is activated.
     *   Zero if the Session is not bound to any SecureChannel.
     * 0:session-name [String]
     *   Name of the Session as defined by the client.
     * 0:client-description [ApplicationDescription]
     *   Name of the Session as defined by the client.
     * 0:client-user-id [String]
     *   User identifier used to activate the session. This is extracted from
     *   the UserIdentityToken (e.g. username but not the password).
     * 0:locale-ids [Array of String]
     *   List of preferred languages.
     *
     * Any additional attributes set via UA_Server_setSessionAttribute are
     * appended to the above list. */
    UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CREATED,
    UA_APPLICATIONNOTIFICATIONTYPE_SESSION_ACTIVATED,
    UA_APPLICATIONNOTIFICATIONTYPE_SESSION_DEACTIVATED,
    UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED,

    /* Processing of a service request or response. The server-side processing
     * of a request can be asynchronous. The existence of a yet-unfinished async
     * operation from the request is signaled with the _SERVICE_ASYNC enum. The
     * _SERVICE_END enum is signalled eventually, once all async operations from
     * the service request are completed.
     *
     * 0:securechannel-id [UInt32]
     *    Identifier of the SecureChannel to which the Session is connected.
     * 0:session-id [NodeId]
     *    Identifier of the Session for/from which the Service is requested.
     *    This is the ns=0;i=0 NodeId if no Session is bound to the receiving
     *    SecureChannel.
     * 0:request-id [UInt32]
     *    Identifier of the RequestId for the Request/Response pair.
     * 0:service-type [NodeId]
     *    DataType identifier for the Request (server) or Response (client). */
    UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_BEGIN,
    UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_ASYNC,
    UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_END,

    /* (Server only) Signals the creation or modification of a Subscription.
     *
     * 0:session-id [NodeId]
     *    Identifier of the Session for which the Subscription is created.
     *    If the subscription is not bound to any Session, then the NodeId
     *    ns=0;i=0 is returned.
     * 0:subscription-id [UInt32]
     *    Identifier of the Subscription (unique for the Session).
     * 0:publishing-interval [Double]
     *    Frequence at which accumulated notifications are sent out.
     * 0:lifetime-count [UInt32]
     *    Number of consecutive publishing interval with a missing
     *    PublishRequest before the Subscription is starved (deleted).
     * 0:max-keepalive-count [UInt32]
     *    Number of consecutive publishing intervals without a PublishResponse
     *    before a keepalive is sent.
     * 0:max-notifications-per-publish [UInt32]
     *    Number of notifications that can be in a PublishResponse.
     * 0:priority [Byte]
     *    Higher-priority subscriptions send out PublishResponses first.
     * 0:publishing-enabled [Boolean]
     *    What the name says. */
    UA_APPLICATIONNOTIFICATIONTYPE_SUBSCRIPTION_CREATED,
    UA_APPLICATIONNOTIFICATIONTYPE_SUBSCRIPTION_MODIFIED,
    UA_APPLICATIONNOTIFICATIONTYPE_SUBSCRIPTION_PUBLISHINGMODE,
    UA_APPLICATIONNOTIFICATIONTYPE_SUBSCRIPTION_TRANSFERRED,
    UA_APPLICATIONNOTIFICATIONTYPE_SUBSCRIPTION_DELETED,

    /* (Server only) Signals the creation or modification of a MonitoredItem.
     *
     * 0:session-id [NodeId]
     *    Identifier of the Session for which the Subscription is created.
     *    If the subscription is not bound to any Session, then the NodeId
     *    ns=0;i=0 is returned.
     * 0:subscription-id [UInt32]
     *    Identifier of the Subscription (unique for the Session).
     * 0:monitoreditem-id [UInt32]
     *    Identifier of the MonitoredItem (unique for the Subscription).
     * 0:target-node [NodeId]
     *    Identifier of the Node that is monitored.
     * 0:attribute-id [UInt32]
     *    Node-attribute that is being monitored.
     *    Conforms to the values from the UA_AttributeId enum.
     * 0:index-range [String]
     *    Defines if only part of an array value is monitored.
     * 0:timestamps-to-return [TimestampsToReturn]
     *    Enum with the options SOURCE | SERVER | BOTH | NEITHER.
     * 0:monitorimg-mode [MonitoringMode]
     *    Enum with the options DISABLED | SAMPLING | REPORTING.
     * 0:client-handle [UInt32]
     *   Client-supplied identifier of the MonitoredItem.
     * 0:samping-interval [Double]
     *    Interval to evaluate the MonitoredItem in milliseconds.
     * 0:filter [Empty | DataChangeFilter | EventFilter | AggregateFilter]
     *    The filter used to emit notifications.
     * 0:queue-size [UInt32]
     *    Maximum number of notifications waiting to be published.
     * 0:discard-oldest [Boolean]
     *    When the queue overflows, delete the newest or the oldest
     *    notification. */
    UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_CREATED,
    UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_MODIFIED,
    UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_MONITORINGMODE,
    UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_DELETE
} UA_ApplicationNotificationType;

/**
 * Connection State
 * ---------------- */

typedef enum {
    UA_CONNECTIONSTATE_CLOSED,     /* The socket has been closed and the connection
                                    * will be deleted */
    UA_CONNECTIONSTATE_OPENING,    /* The socket is open, but the connection not yet
                                      fully established */
    UA_CONNECTIONSTATE_ESTABLISHED,/* The socket is open and the connection
                                    * configured */
    UA_CONNECTIONSTATE_CLOSING,    /* The socket is closing down */
    UA_CONNECTIONSTATE_BLOCKING,   /* Listening disabled (e.g. max connections reached) */
    UA_CONNECTIONSTATE_REOPENING   /* Listening resumed after being blocked */
} UA_ConnectionState;

typedef enum {
    UA_SECURECHANNELSTATE_CLOSED = 0,
    UA_SECURECHANNELSTATE_REVERSE_LISTENING,
    UA_SECURECHANNELSTATE_CONNECTING,
    UA_SECURECHANNELSTATE_CONNECTED,
    UA_SECURECHANNELSTATE_REVERSE_CONNECTED,
    UA_SECURECHANNELSTATE_RHE_SENT,
    UA_SECURECHANNELSTATE_HEL_SENT,
    UA_SECURECHANNELSTATE_HEL_RECEIVED,
    UA_SECURECHANNELSTATE_ACK_SENT,
    UA_SECURECHANNELSTATE_ACK_RECEIVED,
    UA_SECURECHANNELSTATE_OPN_SENT,
    UA_SECURECHANNELSTATE_OPEN,
    UA_SECURECHANNELSTATE_CLOSING,
} UA_SecureChannelState;

typedef enum {
    UA_SESSIONSTATE_CLOSED = 0,
    UA_SESSIONSTATE_CREATE_REQUESTED,
    UA_SESSIONSTATE_CREATED,
    UA_SESSIONSTATE_ACTIVATE_REQUESTED,
    UA_SESSIONSTATE_ACTIVATED,
    UA_SESSIONSTATE_CLOSING
} UA_SessionState;

/**
 * Statistic Counters
 * ------------------
 * The stack manages statistic counters for SecureChannels and Sessions.
 *
 * The Session layer counters are matching the counters of the
 * ServerDiagnosticsSummaryDataType that are defined in the OPC UA Part 5
 * specification. The SecureChannel counters are not defined in the OPC UA spec,
 * but are harmonized with the Session layer counters if possible. */

typedef enum {
    UA_SHUTDOWNREASON_CLOSE = 0,
    UA_SHUTDOWNREASON_REJECT,
    UA_SHUTDOWNREASON_SECURITYREJECT,
    UA_SHUTDOWNREASON_TIMEOUT,
    UA_SHUTDOWNREASON_ABORT,
    UA_SHUTDOWNREASON_PURGE
} UA_ShutdownReason;

typedef struct {
    size_t currentChannelCount;
    size_t cumulatedChannelCount;
    size_t rejectedChannelCount;
    size_t channelTimeoutCount; /* only used by servers */
    size_t channelAbortCount;
    size_t channelPurgeCount;   /* only used by servers */
} UA_SecureChannelStatistics;

typedef struct {
    size_t currentSessionCount;
    size_t cumulatedSessionCount;
    size_t securityRejectedSessionCount; /* only used by servers */
    size_t rejectedSessionCount;
    size_t sessionTimeoutCount;          /* only used by servers */
    size_t sessionAbortCount;            /* only used by servers */
} UA_SessionStatistics;

/**
 * Lifecycle States
 * ----------------
 * Generic lifecycle states. The STOPPING state indicates that the lifecycle is
 * being terminated. But it might take time to (asynchronously) perform a
 * graceful shutdown. */

typedef enum {
    UA_LIFECYCLESTATE_STOPPED = 0,
    UA_LIFECYCLESTATE_STARTED,
    UA_LIFECYCLESTATE_STOPPING
} UA_LifecycleState;

_UA_END_DECLS

#endif /* UA_COMMON_H_ */
