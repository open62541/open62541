#ifndef NANOMQ_RULE_H
#define NANOMQ_RULE_H

#include "nng/mqtt/mqtt_client.h"
#include "nng/supplemental/sqlite/sqlite3.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/nng.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(SUPP_RULE_ENGINE)
extern int nano_client(nng_socket *sock, repub_t *repub);
extern int nano_client_publish(nng_socket *sock, const char *topic,
    uint8_t *payload, uint32_t len, uint8_t qos, property *props);
extern int nanomq_client_sqlite(conf_rule *cr, bool init_last);
extern int nanomq_client_mysql(conf_rule *cr, bool init_last);
extern int nanomq_client_postgresql(conf_rule *cr, bool init_last);
extern int nanomq_client_timescaledb(conf_rule *cr, bool init_last);
#endif

#endif // NANOMQ_RULE_H
