#ifndef NANOMQ_CONF_API_H
#define NANOMQ_CONF_API_H

#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/nanolib/cJSON.h"
#include "nng/supplemental/nanolib/file.h"
#include "nng/supplemental/nanolib/utils.h"
#include "nng/supplemental/nanolib/cvector.h"
#include "rest_api.h"

extern cJSON *get_reload_config(conf *config);
extern cJSON *get_basic_config(conf *config);
extern cJSON *get_tls_config(conf_tls *tls, bool is_server);
extern cJSON *get_auth_config(conf_auth *auth);
extern cJSON *get_auth_http_config(conf_auth_http *auth_http);
extern cJSON *get_websocket_config(conf_websocket *ws);
extern cJSON *get_http_config(conf_http_server *http);
extern cJSON *get_sqlite_config(conf_sqlite *sqlite);
extern cJSON *get_bridge_config(conf_bridge *bridge, const char *node_name);
extern void   set_bridge_node_conf(
      conf_bridge *config, cJSON *node_obj, const char *name);

extern void set_reload_config(cJSON *json, conf *config);
extern void set_basic_config(cJSON *json, conf *config);
extern void set_tls_config(
    cJSON *json, const char *conf_path, conf_tls *tls, const char *key_prefix);
extern void set_auth_config(
    cJSON *json, const char *conf_path, conf_auth *auth);
extern void set_auth_http_config(
    cJSON *json, const char *conf_path, conf_auth_http *auth);
extern void set_http_config(
    cJSON *json, const char *conf_path, conf_http_server *http);
extern void set_websocket_config(
    cJSON *json, const char *conf_path, conf_websocket *ws);
extern void set_sqlite_config(cJSON *json, const char *conf_path,
    conf_sqlite *sqlite, const char *key_prefix);

extern void reload_basic_config(conf *cur_conf, conf *new_conf);
extern void reload_sqlite_config(conf_sqlite *cur_conf, conf_sqlite *new_conf);
extern void reload_auth_config(conf_auth *cur_conf, conf_auth *new_conf);
extern void reload_log_config(conf *cur_conf, conf *new_conf);

#endif
