//
// Copyright 2022 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef MQTT_PROTOCOL_H
#define MQTT_PROTOCOL_H

#define MQTT_PROTOCOL_NAME_v31 "MQIsdp"
#define MQTT_PROTOCOL_VERSION_v31 3
#define MQTT_PROTOCOL_NAME "MQTT"
#define MQTT_PROTOCOL_VERSION_v311 4
#define MQTT_PROTOCOL_VERSION_v5 5

/* NNG OPTs */
#define NANO_CONF "nano:conf"


/* Length defination */
// Maximum Packet Size of broker
#define NANO_MAX_RECV_PACKET_SIZE (2*1024*1024)
#define NANO_MIN_PACKET_LEN sizeof(uint8_t) * 8
#define NANO_CONNECT_PACKET_LEN sizeof(uint8_t) * 12
#define NANO_MIN_FIXED_HEADER_LEN sizeof(uint8_t) * 2
//flow control:how many QoS packet broker willing to process at same time.
#define NANO_MAX_QOS_PACKET 1024

#ifdef NANO_PACKET_SIZE
#define NNI_NANO_MAX_PACKET_SIZE sizeof(uint8_t) * NANO_PACKET_SIZE
#else
#define NNI_NANO_MAX_PACKET_SIZE sizeof(uint8_t) * 16
#endif

/* Error values */
enum err_t {
	ERR_AUTH_CONTINUE      = -4,
	ERR_NO_SUBSCRIBERS     = -3,
	ERR_SUB_EXISTS         = -2,
	ERR_CONN_PENDING       = -1,
	ERR_SUCCESS            = 0,
	ERR_NOMEM              = 1,
	ERR_PROTOCOL           = 2,
	ERR_INVAL              = 3,
	ERR_NO_CONN            = 4,
	ERR_CONN_REFUSED       = 5,
	ERR_NOT_FOUND          = 6,
	ERR_CONN_LOST          = 7,
	ERR_TLS                = 8,
	ERR_PAYLOAD_SIZE       = 9,
	ERR_NOT_SUPPORTED      = 10,
	ERR_AUTH               = 11,
	ERR_ACL_DENIED         = 12,
	ERR_UNKNOWN            = 13,
	ERR_ERRNO              = 14,
	ERR_EAI                = 15,
	ERR_PROXY              = 16,
	ERR_PLUGIN_DEFER       = 17,
	ERR_MALFORMED_UTF8     = 18,
	ERR_KEEPALIVE          = 19,
	ERR_LOOKUP             = 20,
	ERR_MALFORMED_PACKET   = 21,
	ERR_DUPLICATE_PROPERTY = 22,
	ERR_TLS_HANDSHAKE      = 23,
	ERR_QOS_NOT_SUPPORTED  = 24,
	ERR_OVERSIZE_PACKET    = 25,
	ERR_OCSP               = 26,
};

// mqtt5 macro
#define NMQ_RECEIVE_MAXIMUM_EXCEEDED 0X93
#define NMQ_PACKET_TOO_LARGE 0x95
#define NMQ_UNSEPECIFY_ERROR 0X80
#define NMQ_SERVER_UNAVAILABLE 0x88
#define NMQ_SERVER_BUSY 0x89
#define NMQ_SERVER_SHUTTING_DOWN 0x8B
#define NMQ_KEEP_ALIVE_TIMEOUT 0x8D
#define NMQ_AUTH_SUB_ERROR 0X87

// MQTT Control Packet types
typedef enum {
	RESERVED    = 0,
	CONNECT     = 1,
	CONNACK     = 2,
	PUBLISH     = 3,
	PUBACK      = 4,
	PUBREC      = 5,
	PUBREL      = 6,
	PUBCOMP     = 7,
	SUBSCRIBE   = 8,
	SUBACK      = 9,
	UNSUBSCRIBE = 10,
	UNSUBACK    = 11,
	PINGREQ     = 12,
	PINGRESP    = 13,
	DISCONNECT  = 14,
	AUTH        = 15
} mqtt_control_packet_types;

//MQTTV5
typedef enum {
	// 0 : V4 1: V5 5: V4 to V5  4: V5 to V4
	MQTTV4 = 0,
	MQTTV5 = 1,
	MQTTV4_V5 = 2,
	MQTTV5_V4 = 3,
} target_prover;

#endif
