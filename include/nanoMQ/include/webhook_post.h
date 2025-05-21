#ifndef WEBHOOK_POST_H
#define WEBHOOK_POST_H

#include "webhook_inproc.h"
#include "broker.h"

extern int webhook_msg_publish(nng_socket *sock, conf_web_hook *hook_conf,
    pub_packet_struct *pub_packet, const char *username,
    const char *client_id);
extern int webhook_client_connack(nng_socket *sock, conf_web_hook *hook_conf,
    uint8_t proto_ver, uint16_t keepalive, uint8_t reason,
    const char *username, const char *client_id);
extern int webhook_client_disconnect(nng_socket *sock,
    conf_web_hook *hook_conf, uint8_t proto_ver, uint16_t keepalive,
    uint8_t reason, const char *username, const char *client_id);
extern int hook_entry(nano_work *work, uint8_t reason);
extern int hook_exchange_init(conf *nanomq_conf, uint64_t num_ctx);
extern int hook_exchange_sender_init(conf *nanomq_conf, struct work **works, uint64_t num_ctx);

#endif
