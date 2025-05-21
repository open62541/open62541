/**
 * Created by Alvin on 2020/7/25.
 */

#ifndef NANOMQ_PUB_HANDLER_H
#define NANOMQ_PUB_HANDLER_H

#include "broker.h"
#include <nng/mqtt/packet.h>
#include <nng/nng.h>
#include <nng/protocol/mqtt/mqtt.h>

typedef uint32_t variable_integer;

// MQTT Fixed header
struct fixed_header {
	// flag_bits
	uint8_t retain : 1;
	uint8_t qos : 2;
	uint8_t dup : 1;
	// packet_types
	uint8_t packet_type : 4;
	// remaining length
	uint32_t remain_len;
};

// MQTT Variable header
union variable_header {
	struct {
		uint16_t           packet_id;
		struct mqtt_string topic_name;
		property           *properties;
		uint32_t           prop_len;
	} publish;

	struct {
		uint16_t    packet_id;
		reason_code reason_code;
		property    *properties;
		uint32_t    prop_len;
	} pub_arrc, puback, pubrec, pubrel, pubcomp;
};

struct mqtt_payload {
	uint8_t *data;
	uint32_t len;
};

struct pub_packet_struct {
	struct fixed_header   fixed_header;
	union variable_header var_header;
	struct mqtt_payload   payload;
};

struct pipe_content {
	mqtt_msg_info *msg_infos;
};

bool encode_pub_message(
    nng_msg *dest_msg, nano_work *work, mqtt_control_packet_types cmd);
reason_code decode_pub_message(nano_work *work, uint8_t proto);
void free_pub_packet(struct pub_packet_struct *pub_packet);
void free_msg_infos(mqtt_msg_info *msg_infos);
void init_pipe_content(struct pipe_content *pipe_ct);
void init_pub_packet_property(struct pub_packet_struct *pub_packet);
bool check_msg_exp(nng_msg *msg, property *prop);

reason_code handle_pub(nano_work *work, struct pipe_content *pipe_ct,
    uint8_t proto, bool is_event);

#endif // NNG_PUB_HANDLER_H
