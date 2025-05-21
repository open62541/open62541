#ifndef MQTT_UNSUBSCRIBE_HANDLE_H
#define MQTT_UNSUBSCRIBE_HANDLE_H

#include <nng/nng.h>

#include "broker.h"
#include <nng/mqtt/packet.h>

int  decode_unsub_msg(nano_work *);
int  encode_unsuback_msg(nng_msg *, nano_work *);
int  unsub_ctx_handle(nano_work *);
void unsub_pkt_free(packet_unsubscribe *);

#endif // MQTT_UNSUBSCRIBE_HANDLE_H
