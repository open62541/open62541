//
// Copyright 2024 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

// This file is for the MQTT client implementation.
// Note that while there are some similarities, MQTT is sufficiently
// different enough from SP that many of the APIs cannot be easily
// shared.
//
// At this time there is no server provided by NNG itself, although
// the nanomq project provides such a server (and is based on NNG.)
//
// About our semantics:
//
// 1. MQTT client sockets have a single implicit dialer, and cannot
//    support creation of additional dialers or listeners.
// 2. MQTT client sockets do support contexts; each context will
//    maintain its own subscriptions, and the socket will keep a
//    per-socket list of them and manage the full list.
// 3. Send sends PUBLISH messages.
// 4. Receive is used to receive published data from the server.
// 5. Most of the MQTT specific "features" are as options on the socket,
//    dialer, or even the message.  (For example message topics are set
//    as options on the message.)
// 6. Pipe events can be used to detect connect/disconnect events.
// 7. Any QoS details such as retransmit, etc. are handled under the hood.
//    This includes packet IDs.
// 8. PING and keep-alive is handled under the hood.
// 9. For publish actions, a separate method is used (not send/receive).

#ifndef NNG_MQTT_CLIENT_H
#define NNG_MQTT_CLIENT_H

#include <nng/nng.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_PROTOCOL_NAME_v31 "MQIsdp"
#define MQTT_PROTOCOL_VERSION_v31 3
#define MQTT_PROTOCOL_NAME "MQTT"
#define MQTT_PROTOCOL_VERSION_v311 4
#define MQTT_PROTOCOL_VERSION_v5 5

// Only work for MQTT Protocol layer
#define NNG_OPT_MQTT_CLIENT_PIPEID "nng-nano-pipe-id"

// Only work for MQTT Protocol layer
#define NNG_OPT_MQTT_BROKER_PIPEID "nano-broker-pipe-id"

// NNG_OPT_MQTT_EXPIRES is a 32-bit integer representing the expiration in
// seconds. This can be applied to a message.
// (TODO: What about session expiry?)
#define NNG_OPT_MQTT_EXPIRES "expires"

#define NNG_OPT_MQTT_CONNMSG "mqtt-connect-msg"

#define NNG_OPT_MQTT_CONNECT_PROPERTY "mqtt-connack-property"

#define NNG_OPT_MQTT_CONNECT_REASON "mqtt-connack-reason"

#define NNG_OPT_MQTT_RECONNECT_BACKOFF_MAX "mqtt-reconnect-backoff-max"

#define NNG_OPT_MQTT_DISCONNECT_PROPERTY "mqtt-disconnect-property"

#define NNG_OPT_MQTT_RETRY_INTERVAL "mqtt-client-retry-interval"

#define NNG_OPT_MQTT_RETRY_WAIT_TIME "mqtt-client-retry-wait-time"

#define NNG_OPT_MQTT_DISCONNECT_REASON "mqtt-disconnect-reason"

#define NNG_OPT_MQTT_SQLITE "mqtt-sqlite-option"

#define NNG_OPT_MQTT_ENABLE_SCRAM "mqtt-scram-option"

// NNG_OPT_MQTT_QOS is a byte (only lower two bits significant) representing
// the quality of service.  At this time, only level zero is supported.
// TODO: level 1 and level 2 QoS
#define NNG_OPT_MQTT_QOS "qos"

// NNG_OPT_MQTT_RETAIN indicates that the message should be retained on
// the server as the single retained message for the associated topic.
// This is a boolean.
#define NNG_OPT_MQTT_RETAIN "retain"

// NNG_OPT_MQTT_DUP indicates that the message is a duplicate. This can
// only be returned on a message -- this client will add this flag if
// sending a duplicate message (QoS 1 and 2 only).
#define NNG_OPT_MQTT_DUP "dup"

// NNG_OPT_MQTT_TOPIC is the message topic.  It is encoded as an "Encoded
// UTF-8 string" (uint16 length followed by UTF-8 data).  At the API, it
// is just a UTF-8 string (C style, with a terminating byte.)  Note that
// we do not support embedded NUL bytes in our UTF-8 strings.  Every
// MQTT published message must have a topic.
#define NNG_OPT_MQTT_TOPIC "topic"

// NNG_OPT_MQTT_REASON is a reason that can be conveyed with a message.
// It is a UTF-8 string.
#define NNG_OPT_MQTT_REASON "reason"

// NNG_OPT_MQTT_USER_PROPS is an array of user properties. These are
// send with the message, and used for application specific purposes.
// The properties are of the type nng_mqtt_user_props_t.
#define NNG_OPT_MQTT_USER_PROPS "user-props"

// NNG_OPT_MQTT_PAYLOAD_FORMAT is the format of the payload for a message.
// It can be 0, indicating binary data, or 1, indicating UTF-8.
#define NNG_OPT_MQTT_PAYLOAD_FORMAT "mqtt-payload-format"

// NNG_OPT_MQTT_CONTENT_TYPE is the mime type as UTF-8 for PUBLISH
// or Will messages.
#define NNG_OPT_MQTT_CONTENT_TYPE "content-type"

// The following options are reserved for MQTT v5.0 request/reply support.
#define NNG_OPT_MQTT_RESPONSE_TOPIC "response-topic"
#define NNG_OPT_MQTT_CORRELATION_DATA "correlation-data"

// NNG_OPT_MQTT_CLIENT_ID is the UTF-8 string corresponding to the client
// identification.  We automatically generate an initial value fo this,
// which is the UUID.
// TODO: Should applications be permitted to change this?
#define NNG_OPT_MQTT_CLIENT_ID "client-id" // UTF-8 string

#define NNG_OPT_MQTT_WILL_DELAY "will-delay"

// NNG_OPT_MQTT_RECEIVE_MAX is used with QoS 1 or 2 (not implemented),
// and indicates the level of concurrent receives it is willing to
// process. (TODO: Implementation note: we will need to preallocate a complete
// state machine (aio, plus any state) for each value of this > 0.
// It's not clear whether this should be tunable.)  This is read-only
// property on the socket, and records the value given from the server.
// It will be 64K if the server did not indicate a specific value.
#define NNG_OPT_MQTT_RECEIVE_MAX "mqtt-receive-max"

// NNG_OPT_MQTT_SESSION_EXPIRES is an nng_duration.
// If set to NNG_DURATION_ZERO, then the session will expire automatically
// when the connection is closed.
// If it set to NNG_DURATION_INFINITE, the session never expires.
// Otherwise it will be a whole number of seconds indicating the session
// expiry interval.
#define NNG_OPT_MQTT_SESSION_EXPIRES "session-expires"

#define NNG_OPT_MQTT_TOPIC_ALIAS_MAX "alias-max"
#define NNG_OPT_MQTT_TOPIC_ALIAS "topic-alias"
#define NNG_OPT_MQTT_MAX_QOS "max-qos"

// NNG_MAX_RECV_LMQ and NNG_MAX_SEND_LMQ define the length of waiting queue
// they are the length of nni_lmq, please be ware it affects the memory usage
// significantly while having heavy throughput
#define NNG_MAX_RECV_LMQ 64
#define NNG_MAX_SEND_LMQ 64
#define NNG_TRAN_MAX_LMQ_SIZE 128

// NNG_TLS_xxx options can be set on the client as well.
// E.g. NNG_OPT_TLS_CA_CERT, etc.

// TBD: Extended authentication.  I think we should skip it -- everyone
// should just use TLS if they need security.

// NNG_OPT_MQTT_KEEP_ALIVE is set on the client, and can be retrieved
// by the client.  This is an nng_duration but will always be zero or
// a whole number of seconds less than 65536.  If setting the value,
// it must be set before the client connects.  When retrieved, the
// server's value will be returned (if it is different from what we
// requested.)  If we reconnect, we will try again with the configured
// value rather than the value that we got from the server last time.
#define NNG_OPT_MQTT_KEEP_ALIVE "mqtt-keep-alive"

// NNG_OPT_MQTT_MAX_PACKET_SIZE is the maximum packet size that can
// be used.  It needs to be set before the client dials.
#define NNG_OPT_MQTT_MAX_PACKET_SIZE "mqtt-max-packet-size"
#define NNG_OPT_MQTT_USERNAME "username"
#define NNG_OPT_MQTT_PASSWORD "password"

// Message handling.  Note that topic aliases are handled by the library
// automatically on behalf of the consumer.

typedef enum {
	nng_mqtt_msg_format_binary = 0,
	nng_mqtt_msg_format_utf8   = 1,
} nng_mqtt_msg_format_t;

/* Message types & flags */
#define CMD_UNKNOWN 0x00
#define CMD_HTTPREQ 0x01
#define CMD_CONNECT 0x10
#define CMD_CONNACK 0x20
#define CMD_PUBLISH 0x30	// indicates PUBLISH packet & MQTTV4 pub packet
#define CMD_PUBLISH_V5 0x31 // this is the flag for differing MQTTV5 from V4 V3
#define CMD_PUBACK 0x40
#define CMD_PUBREC 0x50
#define CMD_PUBREL 0x60
#define CMD_PUBCOMP 0x70
#define CMD_SUBSCRIBE 0x80
#define CMD_SUBACK 0x90
#define CMD_UNSUBSCRIBE 0xA0
#define CMD_UNSUBACK 0xB0
#define CMD_PINGREQ 0xC0
#define CMD_PINGRESP 0xD0
#define CMD_DISCONNECT 0xE0
#define CMD_AUTH_V5 0xF0
#define CMD_DISCONNECT_EV 0xE2
#define CMD_LASTWILL 0XE3

typedef enum {
	NNG_MQTT_CONNECT     = 0x01,
	NNG_MQTT_CONNACK     = 0x02,
	NNG_MQTT_PUBLISH     = 0x03,
	NNG_MQTT_PUBACK      = 0x04,
	NNG_MQTT_PUBREC      = 0x05,
	NNG_MQTT_PUBREL      = 0x06,
	NNG_MQTT_PUBCOMP     = 0x07,
	NNG_MQTT_SUBSCRIBE   = 0x08,
	NNG_MQTT_SUBACK      = 0x09,
	NNG_MQTT_UNSUBSCRIBE = 0x0A,
	NNG_MQTT_UNSUBACK    = 0x0B,
	NNG_MQTT_PINGREQ     = 0x0C,
	NNG_MQTT_PINGRESP    = 0x0D,
	NNG_MQTT_DISCONNECT  = 0x0E,
	NNG_MQTT_AUTH        = 0x0F
} nng_mqtt_packet_type;

//TODO Enum is not fitting here
typedef enum {
	SUCCESS                                = 0,
	NORMAL_DISCONNECTION                   = 0,
	GRANTED_QOS_0                          = 0,
	GRANTED_QOS_1                          = 1,
	GRANTED_QOS_2                          = 2,
	DISCONNECT_WITH_WILL_MESSAGE           = 4,
	NO_MATCHING_SUBSCRIBERS                = 16,
	NO_SUBSCRIPTION_EXISTED                = 17,
	CONTINUE_AUTHENTICATION                = 24,
	RE_AUTHENTICATE                        = 25,
	UNSPECIFIED_ERROR                      = 128,
	MALFORMED_PACKET                       = 129,
	PROTOCOL_ERROR                         = 130,
	IMPLEMENTATION_SPECIFIC_ERROR          = 131,
	UNSUPPORTED_PROTOCOL_VERSION           = 132,
	CLIENT_IDENTIFIER_NOT_VALID            = 133,
	BAD_USER_NAME_OR_PASSWORD              = 134,
	NOT_AUTHORIZED                         = 135,
	SERVER_UNAVAILABLE                     = 136,
	SERVER_BUSY                            = 137,
	BANNED                                 = 138,
	SERVER_SHUTTING_DOWN                   = 139,
	BAD_AUTHENTICATION_METHOD              = 140,
	KEEP_ALIVE_TIMEOUT                     = 141,
	SESSION_TAKEN_OVER                     = 142,
	TOPIC_FILTER_INVALID                   = 143,
	TOPIC_NAME_INVALID                     = 144,
	PACKET_IDENTIFIER_IN_USE               = 145,
	PACKET_IDENTIFIER_NOT_FOUND            = 146,
	RECEIVE_MAXIMUM_EXCEEDED               = 147,
	TOPIC_ALIAS_INVALID                    = 148,
	PACKET_TOO_LARGE                       = 149,
	MESSAGE_RATE_TOO_HIGH                  = 150,
	QUOTA_EXCEEDED                         = 151,
	ADMINISTRATIVE_ACTION                  = 152,
	PAYLOAD_FORMAT_INVALID                 = 153,
	RETAIN_NOT_SUPPORTED                   = 154,
	QOS_NOT_SUPPORTED                      = 155,
	USE_ANOTHER_SERVER                     = 156,
	SERVER_MOVED                           = 157,
	SHARED_SUBSCRIPTIONS_NOT_SUPPORTED     = 158,
	CONNECTION_RATE_EXCEEDED               = 159,
	MAXIMUM_CONNECT_TIME                   = 160,
	SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 161,
	WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED   = 162

} reason_code;

typedef enum {
	PAYLOAD_FORMAT_INDICATOR          = 1,
	MESSAGE_EXPIRY_INTERVAL           = 2,
	CONTENT_TYPE                      = 3,
	RESPONSE_TOPIC                    = 8,
	CORRELATION_DATA                  = 9,
	SUBSCRIPTION_IDENTIFIER           = 11,
	SESSION_EXPIRY_INTERVAL           = 17,
	ASSIGNED_CLIENT_IDENTIFIER        = 18,
	SERVER_KEEP_ALIVE                 = 19,
	AUTHENTICATION_METHOD             = 21,
	AUTHENTICATION_DATA               = 22,
	REQUEST_PROBLEM_INFORMATION       = 23,
	WILL_DELAY_INTERVAL               = 24,
	REQUEST_RESPONSE_INFORMATION      = 25,
	RESPONSE_INFORMATION              = 26,
	SERVER_REFERENCE                  = 28,
	REASON_STRING                     = 31,
	RECEIVE_MAXIMUM                   = 33,
	TOPIC_ALIAS_MAXIMUM               = 34,
	TOPIC_ALIAS                       = 35,
	PUBLISH_MAXIMUM_QOS               = 36,
	RETAIN_AVAILABLE                  = 37,
	USER_PROPERTY                     = 38,
	MAXIMUM_PACKET_SIZE               = 39,
	WILDCARD_SUBSCRIPTION_AVAILABLE   = 40,
	SUBSCRIPTION_IDENTIFIER_AVAILABLE = 41,
	SHARED_SUBSCRIPTION_AVAILABLE     = 42
} properties_type;

struct mqtt_buf_t {
	uint32_t length;
	uint8_t *buf;
};

typedef struct mqtt_buf_t mqtt_buf;
typedef struct mqtt_buf_t nng_mqtt_buffer;
typedef struct mqtt_buf_t nng_mqtt_topic;

struct mqtt_kv_t {
	mqtt_buf key;
	mqtt_buf value;
};
typedef struct mqtt_kv_t mqtt_kv;

typedef struct mqtt_topic_qos_t {
	nng_mqtt_topic topic;
	uint8_t        qos;
	uint8_t        nolocal;
	uint8_t        rap;
	uint8_t        retain_handling;
} mqtt_topic_qos;

typedef struct mqtt_topic_qos_t nng_mqtt_topic_qos;

extern uint16_t nni_msg_get_pub_pid(nng_msg *m);
struct mqtt_string {
	char *   body;
	uint32_t len;
};
typedef struct mqtt_string mqtt_string;

struct mqtt_string_node {
	struct mqtt_string_node *next;
	mqtt_string *            it;
};
typedef struct mqtt_string_node mqtt_string_node;

struct mqtt_binary {
	uint8_t *body;
	uint32_t len;
};
typedef struct mqtt_binary mqtt_binary;

struct mqtt_str_pair {
	char *   key; // key
	uint32_t len_key;
	char *   val; // value
	uint32_t len_val;
};
typedef struct mqtt_str_pair mqtt_str_pair;

union Property_type {
	uint8_t  u8;
	uint16_t u16;
	uint32_t u32;
	uint32_t varint;
	mqtt_buf binary;
	mqtt_buf str;
	mqtt_kv  strpair;
};

typedef enum {
	U8,
	U16,
	U32,
	VARINT,
	BINARY,
	STR,
	STR_PAIR,
	UNKNOWN
} property_type_enum;

struct property_data {
	property_type_enum  p_type;
	union Property_type p_value;
	bool                is_copy;
};

typedef struct property_data property_data;

struct property {
	uint8_t          id;
	property_data    data;
	struct property *next;
};
typedef struct property property;

NNG_DECL int  nng_mqtt_msg_alloc(nng_msg **, size_t);
NNG_DECL int  nng_mqtt_msg_proto_data_alloc(nng_msg *);
NNG_DECL void nng_mqtt_msg_proto_data_free(nng_msg *);
NNG_DECL int  nng_mqtt_msg_encode(nng_msg *);
NNG_DECL int  nng_mqtt_msg_decode(nng_msg *);
NNG_DECL int  nng_mqttv5_msg_encode(nng_msg *);
NNG_DECL int  nng_mqttv5_msg_decode(nng_msg *);
NNG_DECL int  nng_mqtt_msg_validate(nng_msg *, uint8_t);
NNG_DECL void nng_mqtt_msg_set_packet_type(nng_msg *, nng_mqtt_packet_type);
NNG_DECL nng_mqtt_packet_type nng_mqtt_msg_get_packet_type(nng_msg *);
NNG_DECL void nng_mqtt_msg_set_bridge_bool(nng_msg *msg, bool bridged);
NNG_DECL bool nng_mqtt_msg_get_bridge_bool(nng_msg *msg);
NNG_DECL void nng_mqtt_msg_set_sub_retain_bool(nng_msg *msg, bool retain);
NNG_DECL bool nng_mqtt_msg_get_sub_retain_bool(nng_msg *msg);
NNG_DECL void nng_mqtt_msg_set_connect_proto_version(nng_msg *, uint8_t);
NNG_DECL void nng_mqtt_msg_set_connect_keep_alive(nng_msg *, uint16_t);
NNG_DECL void nng_mqtt_msg_set_connect_client_id(nng_msg *, const char *);
NNG_DECL void nng_mqtt_msg_set_connect_user_name(nng_msg *, const char *);
NNG_DECL void nng_mqtt_msg_set_connect_password(nng_msg *, const char *);
NNG_DECL void nng_mqtt_msg_set_connect_clean_session(nng_msg *, bool);
NNG_DECL void nng_mqtt_msg_set_connect_will_topic(nng_msg *, const char *);
NNG_DECL void nng_mqtt_msg_set_connect_will_msg(
    nng_msg *, uint8_t *, uint32_t);
NNG_DECL void nng_mqtt_msg_set_connect_will_retain(nng_msg *, bool);
NNG_DECL void nng_mqtt_msg_set_connect_will_qos(nng_msg *, uint8_t);
NNG_DECL void nng_mqtt_msg_set_connect_property(nng_msg *, property *);
NNG_DECL property *nng_mqtt_msg_get_connect_will_property(nng_msg *);
NNG_DECL void nng_mqtt_msg_set_connect_will_property(nng_msg *, property *);
NNG_DECL const char *nng_mqtt_msg_get_connect_user_name(nng_msg *);
NNG_DECL const char *nng_mqtt_msg_get_connect_password(nng_msg *);
NNG_DECL bool        nng_mqtt_msg_get_connect_clean_session(nng_msg *);
NNG_DECL uint8_t     nng_mqtt_msg_get_connect_proto_version(nng_msg *);
NNG_DECL uint16_t    nng_mqtt_msg_get_connect_keep_alive(nng_msg *);
NNG_DECL const char *nng_mqtt_msg_get_connect_client_id(nng_msg *);
NNG_DECL const char *nng_mqtt_msg_get_connect_will_topic(nng_msg *);
NNG_DECL uint8_t *nng_mqtt_msg_get_connect_will_msg(nng_msg *, uint32_t *);
NNG_DECL bool     nng_mqtt_msg_get_connect_will_retain(nng_msg *);
NNG_DECL uint8_t  nng_mqtt_msg_get_connect_will_qos(nng_msg *);
NNG_DECL property *nng_mqtt_msg_get_connect_property(nng_msg *);

NNG_DECL void        nng_mqtt_msg_set_connack_return_code(nng_msg *, uint8_t);
NNG_DECL void        nng_mqtt_msg_set_connack_flags(nng_msg *, uint8_t);
NNG_DECL void        nng_mqtt_msg_set_connack_property(nng_msg *, property *);
NNG_DECL uint8_t     nng_mqtt_msg_get_connack_return_code(nng_msg *);
NNG_DECL uint8_t     nng_mqtt_msg_get_connack_flags(nng_msg *);
NNG_DECL property   *nng_mqtt_msg_get_connack_property(nng_msg *);

NNG_DECL void        nng_mqtt_msg_set_publish_proto_version(nng_msg *, uint8_t);
NNG_DECL uint8_t     nng_mqtt_msg_get_publish_proto_version(nng_msg *);
NNG_DECL void        nng_mqtt_msg_set_publish_qos(nng_msg *, uint8_t);
NNG_DECL uint8_t     nng_mqtt_msg_get_publish_qos(nng_msg *);
NNG_DECL void        nng_mqtt_msg_set_publish_retain(nng_msg *, bool);
NNG_DECL bool        nng_mqtt_msg_get_publish_retain(nng_msg *);
NNG_DECL void        nng_mqtt_msg_set_publish_dup(nng_msg *, bool);
NNG_DECL bool        nng_mqtt_msg_get_publish_dup(nng_msg *);
NNG_DECL int         nng_mqtt_msg_set_publish_topic(nng_msg *, const char *);
NNG_DECL int         nng_mqtt_msg_set_publish_topic_len(nng_msg *msg, uint32_t len);

NNG_DECL const char *nng_mqtt_msg_get_publish_topic(nng_msg *, uint32_t *);
NNG_DECL int         nng_mqtt_msg_set_publish_payload(nng_msg *, uint8_t *, uint32_t);
NNG_DECL uint8_t    *nng_mqtt_msg_get_publish_payload(nng_msg *, uint32_t *);
NNG_DECL property   *nng_mqtt_msg_get_publish_property(nng_msg *);
NNG_DECL void        nng_mqtt_msg_set_publish_property(nng_msg *, property *);

NNG_DECL uint16_t nng_mqtt_msg_get_puback_packet_id(nng_msg *);
NNG_DECL property *nng_mqtt_msg_get_puback_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_puback_property(nng_msg *, property *);

NNG_DECL uint16_t nng_mqtt_msg_get_pubrec_packet_id(nng_msg *);
NNG_DECL property *nng_mqtt_msg_get_pubrec_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_pubrec_property(nng_msg *, property *);

NNG_DECL uint16_t nng_mqtt_msg_get_pubrel_packet_id(nng_msg *);
NNG_DECL property *nng_mqtt_msg_get_pubrel_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_pubrel_property(nng_msg *, property *);

NNG_DECL uint16_t nng_mqtt_msg_get_pubcomp_packet_id(nng_msg *);
NNG_DECL property *nng_mqtt_msg_get_pubcomp_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_pubcomp_property(nng_msg *, property *);

NNG_DECL nng_mqtt_topic_qos *nng_mqtt_msg_get_subscribe_topics(
    nng_msg *, uint32_t *);
NNG_DECL void nng_mqtt_msg_set_subscribe_topics(
    nng_msg *, nng_mqtt_topic_qos *, uint32_t);
NNG_DECL property *nng_mqtt_msg_get_subscribe_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_subscribe_property(nng_msg *, property *);

NNG_DECL void nng_mqtt_msg_set_suback_return_codes(
    nng_msg *, uint8_t *, uint32_t);
NNG_DECL uint8_t *nng_mqtt_msg_get_suback_return_codes(nng_msg *, uint32_t *);
NNG_DECL property *nng_mqtt_msg_get_suback_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_suback_property(nng_msg *, property *);

NNG_DECL void     nng_mqtt_msg_set_unsubscribe_topics(
        nng_msg *, nng_mqtt_topic *, uint32_t);
NNG_DECL nng_mqtt_topic *nng_mqtt_msg_get_unsubscribe_topics(
    nng_msg *, uint32_t *);
NNG_DECL property *nng_mqtt_msg_get_unsubscribe_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_unsubscribe_property(nng_msg *, property *);

NNG_DECL void nng_mqtt_msg_set_unsuback_return_codes(
    nng_msg *, uint8_t *, uint32_t);
NNG_DECL uint8_t *nng_mqtt_msg_get_unsuback_return_codes(nng_msg *, uint32_t *);
NNG_DECL property *nng_mqtt_msg_get_unsuback_property(nng_msg *);
NNG_DECL void      nng_mqtt_msg_set_unsuback_property(nng_msg *, property *);

NNG_DECL property   *nng_mqtt_msg_get_disconnect_property(nng_msg *);
NNG_DECL void        nng_mqtt_msg_set_disconnect_property(nng_msg *, property *);

NNG_DECL nng_mqtt_topic *nng_mqtt_topic_array_create(size_t);
NNG_DECL void nng_mqtt_topic_array_set(nng_mqtt_topic *, size_t, const char *);
NNG_DECL void nng_mqtt_topic_array_free(nng_mqtt_topic *, size_t);
NNG_DECL nng_mqtt_topic_qos *nng_mqtt_topic_qos_array_create(size_t);
NNG_DECL void nng_mqtt_topic_qos_array_set(nng_mqtt_topic_qos *, size_t,
           const char *, uint8_t, uint8_t, uint8_t, uint8_t);
NNG_DECL void nng_mqtt_topic_qos_array_free(nng_mqtt_topic_qos *, size_t);
NNG_DECL int  nng_mqtt_set_connect_cb(nng_socket, nng_pipe_cb, void *);
NNG_DECL int  nng_mqtt_set_disconnect_cb(nng_socket, nng_pipe_cb, void *);
NNG_DECL void nng_mqtt_msg_dump(nng_msg *, uint8_t *, uint32_t, bool);

NNG_DECL conn_param *nng_get_conn_param_from_msg(nng_msg *);
NNG_DECL void nng_msg_proto_set_property(nng_msg *msg, void *p);
NNG_DECL void nng_mqtt_msg_set_disconnect_reason_code(nng_msg *msg, uint8_t reason_code);

NNG_DECL uint32_t get_mqtt_properties_len(property *prop);
NNG_DECL int      mqtt_property_free(property *prop);
NNG_DECL void      mqtt_property_foreach(property *prop, void (*cb)(property *));
NNG_DECL int       mqtt_property_dup(property **dup, const property *src);
NNG_DECL property *mqtt_property_pub_by_will(property *will_prop);
NNG_DECL int mqtt_property_value_copy(property *dst, const property *src);

NNG_DECL property *mqtt_property_alloc(void);
NNG_DECL property *mqtt_property_set_value_u8(uint8_t prop_id, uint8_t value);
NNG_DECL property *mqtt_property_set_value_u16(uint8_t prop_id, uint16_t value);
NNG_DECL property *mqtt_property_set_value_u32(uint8_t prop_id, uint32_t value);
NNG_DECL property *mqtt_property_set_value_varint(uint8_t prop_id, uint32_t value);
NNG_DECL property *mqtt_property_set_value_binary(uint8_t prop_id, uint8_t *value, uint32_t len, bool copy_value);
NNG_DECL property *mqtt_property_set_value_str( uint8_t prop_id, const char *value, uint32_t len, bool copy_value);
NNG_DECL property *mqtt_property_set_value_strpair(uint8_t prop_id, const char *key, uint32_t key_len, const char *value, uint32_t value_len, bool copy_value);

NNG_DECL property_type_enum mqtt_property_get_value_type(uint8_t prop_id);
NNG_DECL property_data *mqtt_property_get_value(property *prop, uint8_t prop_id);
NNG_DECL void      mqtt_property_append(property *prop_list, property *last);

// Note that MQTT sockets can be connected to at most a single server.
// Creating the client does not connect it.
NNG_DECL int nng_mqtt_client_open(nng_socket *);
NNG_DECL int nng_mqttv5_client_open(nng_socket *);

// Note that there is a single implicit dialer for the client,
// and options may be set on the socket to configure dial options.
// Those options should be set before doing nng_dial().

// close done via nng_close().

// Question: session resumption.  Should we resume sessions under the hood
// as part of reconnection, or do we want to expose this to the API user?
// My inclination is not to expose.

// nng_dial or nng_dialer_create can be used, but this protocol only
// allows a single dialer to be created on the socket.

// Subscriptions are normally run synchronously from the view of the
// caller.  Because there is a round-trip message involved, we use
// a separate method instead of merely relying upon socket options.
// TODO: shared subscriptions.  Subscription options (retain, QoS)
typedef struct nng_mqtt_client nng_mqtt_client;
typedef void(nng_mqtt_send_cb)(nng_mqtt_client *client, nng_msg *msg, void *);
struct nng_mqtt_client{
	nng_socket sock;
	nng_aio   *send_aio;
	nng_aio   *recv_aio;
	void      *msgq;
	void      *obj; // user defined callback obj
	bool       async;

	nng_mqtt_send_cb *cb;
};


NNG_DECL nng_mqtt_client *nng_mqtt_client_alloc(nng_socket, nng_mqtt_send_cb, bool);
NNG_DECL void nng_mqtt_client_free(nng_mqtt_client*, bool);
NNG_DECL int nng_mqtt_subscribe(nng_socket, nng_mqtt_topic_qos *, uint32_t, property *);
NNG_DECL int nng_mqtt_subscribe_async(nng_mqtt_client *, nng_mqtt_topic_qos *, size_t, property *);
NNG_DECL int nng_mqtt_unsubscribe(nng_socket, nng_mqtt_topic *, size_t, property *);
NNG_DECL int nng_mqtt_unsubscribe_async(nng_mqtt_client *, nng_mqtt_topic *, size_t, property *);
// as with other ctx based methods, we use the aio form exclusively
NNG_DECL int nng_mqtt_ctx_subscribe(nng_ctx *, const char *, nng_aio *, ...);
NNG_DECL int nng_mqtt_disconnect(nng_socket *, uint8_t, property *);

typedef struct nng_mqtt_sqlite_option nng_mqtt_sqlite_option;

#if defined(NNG_SUPP_SQLITE)
NNG_DECL int  nng_mqtt_alloc_sqlite_opt(nng_mqtt_sqlite_option **);
NNG_DECL int  nng_mqtt_free_sqlite_opt(nng_mqtt_sqlite_option *);
NNG_DECL void nng_mqtt_set_sqlite_conf(
    nng_mqtt_sqlite_option *sqlite, void *config);
NNG_DECL void nng_mqtt_sqlite_db_init(nng_mqtt_sqlite_option *, const char *);
NNG_DECL void nng_mqtt_sqlite_db_fini(nng_mqtt_sqlite_option *);
#endif

#ifdef __cplusplus
}
#endif

#endif // NNG_MQTT_CLIENT_H
