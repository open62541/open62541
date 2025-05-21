// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
//
// The Struct to store mqtt_packet.

#ifndef NNG_MQTT_PACKET_H
#define NNG_MQTT_PACKET_H

#include "nng/nng.h"
#include "nng/mqtt/mqtt_client.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define UPDATE_FIELD_INT(field, new_obj, old_obj) \
	do {                                      \
		new_obj->field = old_obj->field;  \
	} while (0)

#define UPDATE_FIELD_MQTT_STRING(field, sub_field, new_obj, old_obj)   \
	do {                                                           \
		if (new_obj->field.sub_field == NULL &&                \
		    old_obj->field.sub_field != NULL) {                \
			new_obj->field = old_obj->field;               \
			new_obj->field.sub_field =                     \
			    strdup((char *) old_obj->field.sub_field); \
		}                                                      \
	} while (0)

#define UPDATE_FIELD_MQTT_STRING_PAIR(                                  \
    field, sub_field1, sub_field2, new_obj, old_obj)                    \
	do {                                                            \
		if ((new_obj->field.sub_field1 == NULL &&               \
		        old_obj->field.sub_field1 != NULL) ||           \
		    (new_obj->field.sub_field2 == NULL &&               \
		        old_obj->field.sub_field2 != NULL)) {           \
			new_obj->field = old_obj->field;                \
			new_obj->field.sub_field1 =                     \
			    strdup((char *) old_obj->field.sub_field1); \
			new_obj->field.sub_field2 =                     \
			    strdup((char *) old_obj->field.sub_field2); \
		}                                                       \
	} while (0)

struct mqtt_msg_info {
	uint32_t pipe;
};
typedef struct mqtt_msg_info mqtt_msg_info;

struct topic_node {
	uint8_t qos : 2;
	uint8_t no_local : 1;
	uint8_t rap : 1;
	uint8_t retain_handling : 2;

	struct {
		char *body;
		int   len;
	} topic;

	uint8_t reason_code;

	struct topic_node *next;
};
typedef struct topic_node topic_node;

struct packet_subscribe {
	uint16_t    packet_id;
	uint32_t    prop_len;
	property   *properties;
	topic_node *node; // stored topic and option
};
typedef struct packet_subscribe packet_subscribe;

struct packet_unsubscribe {
	uint16_t    packet_id;
	uint32_t    prop_len;
	property   *properties;
	topic_node *node; // stored topic and option
};
typedef struct packet_unsubscribe packet_unsubscribe;

#endif
