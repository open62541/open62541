#ifndef EXCHANGE_H
#define EXCHANGE_H

#include <stddef.h>
#include "core/nng_impl.h"
#include "nng/supplemental/nanolib/conf.h"

// Exchange MQ 
#define EXCHANGE_NAME_LEN 32
#define TOPIC_NAME_LEN    128
#define RINGBUFFER_MAX    64

typedef struct exchange_s exchange_t;
struct exchange_s {
	char name[EXCHANGE_NAME_LEN];
	char topic[TOPIC_NAME_LEN];

	ringBuffer_t *rbs[RINGBUFFER_MAX];
	unsigned int rb_count;
};

NNG_DECL int exchange_client_get_msg_by_key(void *arg, uint64_t key, nni_msg **msg);
NNG_DECL int exchange_client_get_msgs_by_key(void *arg, uint64_t key, uint32_t count, nng_msg ***list);

NNG_DECL int exchange_client_get_msgs_fuzz(void *arg, uint64_t start, uint64_t end, uint32_t *count, nng_msg ***list);

NNG_DECL int exchange_init(exchange_t **ex, char *name, char *topic,
				  unsigned int *rbsCaps, char **rbsName, uint8_t *rbsFullOp, unsigned int rbsCount);
NNG_DECL int exchange_add_rb(exchange_t *ex, ringBuffer_t *rb);
NNG_DECL int exchange_release(exchange_t *ex);
NNG_DECL int exchange_handle_msg(exchange_t *ex, uint64_t key, void *msg, nng_aio *aio);
NNG_DECL int exchange_get_ringBuffer(exchange_t *ex, char *rbName, ringBuffer_t **rb);

#endif
