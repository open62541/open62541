#ifndef MQTT_SUBSCRIBE_HANDLE_H
#define MQTT_SUBSCRIBE_HANDLE_H

#include <nng/nng.h>
#include <nng/mqtt/packet.h>

#include "broker.h"

typedef struct {
	uint32_t pid;
	dbtree *db;
}  sub_destroy_info;

/*
 * Use to decode sub msg.
 */
int decode_sub_msg(nano_work *);

/*
 * Use to encode an ack for sub msg
 */
int encode_suback_msg(nng_msg *, nano_work *);

int sub_ctx_handle(nano_work *);

/*
 * Delete a client ctx from topic node in dbtree
 */
int sub_ctx_del(void *db, char *topic, uint32_t pid);

/*
 * Free the client ctx
 */
void sub_ctx_free(client_ctx *);

/*
 * A wrap for sub ctx free
 */
void * wrap_sub_ctx_free_cb(void *arg);

/*
 * Free a packet_subscribe.
 */
void sub_pkt_free(packet_subscribe *);

/*
 * Delete all refs in dbtree about client ctx
 */
void destroy_sub_client(uint32_t pid, dbtree * db);

#endif
