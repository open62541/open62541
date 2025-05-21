#ifndef NANOMQ_BROKER_H
#define NANOMQ_BROKER_H

#define HTTP_CTX_NUM 4

#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/nanolib/nanolib.h"
#include "nng/nng.h"
#include "nng/protocol/mqtt/mqtt.h"
#include "nng/supplemental/util/platform.h"
#include "nng/mqtt/packet.h"
#include "hashmap.h"

#define PROTO_MQTT_BROKER 0x00
#define PROTO_MQTT_BRIDGE 0x01
#define PROTO_AWS_BRIDGE 0x02
#define PROTO_HTTP_SERVER 0x03
#define PROTO_ICEORYX_BRIDGE 0x04

#define STATISTICS

#if defined(ENABLE_NANOMQ_TESTS)
	#undef STATISTICS
#endif

typedef struct work nano_work;
struct work {
	enum {
		INIT,
		RECV,
		WAIT,
		SEND, // Actions after sending msg
		HOOK, // Rule Engine
		END,  // Clear state and cache before disconnect
		CLOSE // sending disconnect packet and err code
	} state;
	uint8_t     proto;		  // logic proto
	uint8_t     proto_ver;   // MQTT version cache
	uint8_t     flag;        // flag for webhook & rule_engine
	nng_aio *   aio;
	nng_msg *   msg;
	nng_msg **  msg_ret;
	nng_ctx     ctx;        // ctx for mqtt broker
	nng_ctx     extra_ctx; //  ctx for bridging/http post
	nng_pipe    pid;
	dbtree *    db;
	dbtree *    db_ret;
	conf *      config;

	conf_bridge_node *node;	// only works for bridge ctx
	reason_code 	  code; // MQTT reason code

	nng_socket hook_sock;

	struct pipe_content *     pipe_ct;
	conn_param *              cparam;
	struct pub_packet_struct *pub_packet;
	packet_subscribe *        sub_pkt;
	packet_unsubscribe *      unsub_pkt;

	void *sqlite_db;
#if defined(SUPP_POSTGRESQL)
	void *pgconn;
#endif
#if defined(SUPP_TIMESCALEDB)
	void *tsconn;
#endif

#if defined(SUPP_PLUGIN)
	property *user_property;
#endif
#if defined(SUPP_ICEORYX)
	void *iceoryx_suber;
	void *iceoryx_puber;
	nng_socket iceoryx_sock;
#endif
};

struct client_ctx {
	nng_pipe pid;
#ifdef STATISTICS
	nng_atomic_u64 *recv_cnt;
#endif
	conn_param *cparam;
	uint32_t    prop_len;
	property   *properties;
	uint8_t     proto_ver;
};

static int broker_start_rc;

typedef struct client_ctx client_ctx;

extern int  broker_start(int argc, char **argv);
extern int  broker_stop(int argc, char **argv);
extern int  broker_restart(int argc, char **argv);
extern int  broker_reload(int argc, char **argv);
extern int  broker_dflt(int argc, char **argv);
extern void bridge_send_cb(void *arg);
extern void *broker_start_with_conf(void *nmq_conf);

#ifdef STATISTICS
extern uint64_t nanomq_get_message_in(void);
extern uint64_t nanomq_get_message_out(void);
extern uint64_t nanomq_get_message_drop(void);
#endif
extern dbtree *          get_broker_db(void);
extern struct hashmap_s *get_hashmap(void);
extern int               rule_engine_insert_sql(nano_work *work);

#endif
