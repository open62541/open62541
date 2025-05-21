#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "cvector.h"
#include <stdbool.h>
#include <stdint.h>
#include "nng/nng.h"
#include "nng/mqtt/packet.h"

struct topic_queue {
	uint8_t             qos;
	char	       *topic;
	struct topic_queue *next;
};

typedef struct topic_queue topic_queue;

struct msg_queue {
	char *            msg;
	struct msg_queue *next;
};

typedef struct msg_queue msg_queue;

// atpair is alias topic pair
typedef struct dbhash_atpair_s dbhash_atpair_t;

struct dbhash_atpair_s {
	uint32_t alias;
	char *   topic;
};

typedef struct dbhash_ptpair_s dbhash_ptpair_t;

// ptpair is pipe topic pair
struct dbhash_ptpair_s {
	uint32_t pipe;
	char *   topic;
};

/**
 * @brief alias_cmp - A callback to compare different alias
 * @param x - normally x is pointer of dbhash_atpair_t
 * @param y - normally y is pointer of alias
 * @return 0, minus or plus
 */
static inline int
alias_cmp(void *x_, void *y_)
{
	uint32_t *       alias = (uint32_t *) y_;
	dbhash_atpair_t *ele_x = (dbhash_atpair_t *) x_;
	return *alias - ele_x->alias;
}

NNG_DECL void dbhash_init_alias_table(void);

NNG_DECL void dbhash_destroy_alias_table(void);
// This function do not verify value of alias and topic,
// therefore you should make sure alias and topic is
// not illegal.
NNG_DECL void dbhash_insert_atpair(uint32_t pipe_id, uint32_t alias, const char *topic);

NNG_DECL const char *dbhash_find_atpair(uint32_t pipe_id, uint32_t alias);

NNG_DECL void dbhash_del_atpair_queue(uint32_t pipe_id);

NNG_DECL void dbhash_init_pipe_table(void);

NNG_DECL void dbhash_destroy_pipe_table(void);

NNG_DECL void dbhash_insert_topic(uint32_t id, char *val, uint8_t qos);

NNG_DECL bool dbhash_check_topic(uint32_t id, char *val);

NNG_DECL char *dbhash_get_first_topic(uint32_t id);

NNG_DECL topic_queue *topic_queue_init(char *topic, int topic_len);

NNG_DECL void topic_queue_release(topic_queue *tq);

NNG_DECL topic_queue *init_topic_queue_with_topic_node(topic_node *tn);

NNG_DECL struct topic_queue *dbhash_get_topic_queue(uint32_t id);

NNG_DECL struct topic_queue *dbhash_copy_topic_queue(uint32_t id);

NNG_DECL void dbhash_del_topic(uint32_t id, char *topic);

NNG_DECL void *dbhash_del_topic_queue(
     uint32_t id, void *(*cb)(void *, char *), void *args);

NNG_DECL bool dbhash_check_id(uint32_t id);

NNG_DECL void *dbhash_check_id_and_do(uint32_t id, void *(*cb)(void *), void *arg);

NNG_DECL void dbhash_print_topic_queue(uint32_t id);

NNG_DECL topic_queue **dbhash_get_topic_queue_all(size_t *sz);

NNG_DECL dbhash_ptpair_t *dbhash_ptpair_alloc(uint32_t p, char *t);

NNG_DECL void dbhash_ptpair_free(dbhash_ptpair_t *pt);

NNG_DECL dbhash_ptpair_t **dbhash_get_ptpair_all(void);

NNG_DECL size_t dbhash_get_pipe_cnt(void);

NNG_DECL void dbhash_init_cached_table(void);
NNG_DECL void dbhash_destroy_cached_table(void);

NNG_DECL void dbhash_cache_topic_all(uint32_t pid, uint32_t cid);

NNG_DECL void dbhash_restore_topic_all(uint32_t cid, uint32_t pid);

NNG_DECL struct topic_queue *dbhash_get_cached_topic(uint32_t cid);

NNG_DECL void dbhash_del_cached_topic_all(uint32_t key);

NNG_DECL bool dbhash_cached_check_id(uint32_t key);

#endif
