#ifndef NANOLIB_H
#define NANOLIB_H

#include "hash_table.h"
#include "mqtt_db.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "nng/supplemental/nanolib/conf.h"

extern void conf_tls_init(conf_tls *tls);
extern void conf_tls_destroy(conf_tls *tls);
extern void conf_tls_parse(
    conf_tls *tls, const char *path, const char *prefix1, const char *prefix2);

extern void conf_http_server_init(conf_http_server *http, uint16_t port);
extern void conf_http_server_destroy(conf_http_server *http);

extern void conf_session_node_init(conf_session_node *node);
extern void conf_bridge_node_init(conf_bridge_node *node);
extern void conf_bridge_sub_properties_init(conf_bridge_sub_properties *prop);
extern void conf_bridge_conn_properties_init(
    conf_bridge_conn_properties *prop);
extern void conf_bridge_conn_will_properties_init(
    conf_bridge_conn_will_properties *prop);

#endif
