#ifndef NANOMQ_BRIDGE_H
#define NANOMQ_BRIDGE_H

#include "nng/mqtt/mqtt_client.h"
#include "nng/nng.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/util/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include "broker.h"
#include "pub_handler.h"

typedef struct {
	nng_socket       *sock;
	conf_bridge_node *config;		// bridge conf file
	nng_mqtt_client  *client;
	nng_msg          *connmsg;
	conf             *conf;			//parent conf file
	nng_mtx          *switch_mtx;
	nng_cv           *switch_cv;
	nng_mtx          *exec_mtx;
	nng_cv           *exec_cv;
	nng_duration     cancel_timeout;
} bridge_param;

extern bool topic_filter(const char *origin, const char *input);
extern bool bridge_sub_handler(nano_work *work);
extern int  bridge_client(
     nng_socket *sock, conf *config, conf_bridge_node *bridge_conf);
extern int hybrid_bridge_client(
    nng_socket *sock, conf *config, conf_bridge_node *node);
extern void bridge_handle_topic_reflection(nano_work *work, conf_bridge *bridge);
extern nng_msg *bridge_publish_msg(const char *topic, uint8_t *payload,
    uint32_t len, bool dup, uint8_t qos, bool retain, property *props);

extern int  bridge_reload(nng_socket *sock, conf *config, conf_bridge_node *node);

extern int bridge_subscribe(nng_socket *sock, conf_bridge_node *node,
    nng_mqtt_topic_qos *topic_qos, size_t sub_count, property *properties);
extern int bridge_unsubscribe(nng_socket *sock, conf_bridge_node *node,
    nng_mqtt_topic *topic, size_t unsub_count, property *properties);

#endif // NANOMQ_BRIDGE_H
