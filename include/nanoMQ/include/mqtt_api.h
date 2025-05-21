#ifndef MQTT_API_H
#define MQTT_API_H

#include "nng/mqtt/mqtt_client.h"
#include "nng/protocol/mqtt/mqtt.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/tls/tls.h"
#include "nng/supplemental/util/options.h"
#include "nng/supplemental/util/platform.h"

#define INPROC_SERVER_URL "inproc://inproc_server"

int nano_listen(
    nng_socket sid, const char *addr, nng_listener *lp, int flags, conf *conf);
int init_listener_tls(nng_listener l, conf_tls *tls);

extern int decode_common_mqtt_msg(nng_msg **dest, nng_msg *src);
extern int encode_common_mqtt_msg(
    nng_msg **dest, nng_msg *src, const char *clientid, uint8_t proto_ver);

extern int log_init(conf_log *log);
extern int log_fini(conf_log *log);

extern char *nano_pipe_get_local_address(nng_pipe p);
extern uint8_t *nano_pipe_get_local_address6(nng_pipe p);
extern uint16_t nano_pipe_get_local_port(nng_pipe p);
extern uint16_t nano_pipe_get_local_port6(nng_pipe p);

#if defined(SUPP_ICEORYX)
#include "nng/iceoryx_shm/iceoryx_shm.h"
extern int nano_iceoryx_send_nng_msg(
    nng_iceoryx_puber *puber, nng_msg *msg, nng_socket *sock);
extern int nano_iceoryx_recv_nng_msg(
    nng_iceoryx_suber *suber, nng_msg *icemsg, nng_msg **msg);
extern bool nano_iceoryx_topic_filter(char *icetopic, char *topic, uint32_t topicsz);
#endif

#endif // MQTT_API_H
