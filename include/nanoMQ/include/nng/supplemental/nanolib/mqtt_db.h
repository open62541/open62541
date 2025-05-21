#ifndef MQTT_DB_H
#define MQTT_DB_H

#include "cvector.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nng/nng.h"


typedef enum {
	MQTT_VERSION_V311 = 4,
	MQTT_VERSION_V5   = 5,
} mqtt_version_t;

typedef struct dbtree            dbtree;

typedef struct {
	char * topic;
	char **clients;
	int    cld_cnt;
} dbtree_info;

/**
 * @brief dbtree_create - Create a dbtree.
 * @param dbtree - dbtree
 * @return void
 */
NNG_DECL void dbtree_create(dbtree **db);

/**
 * @brief dbtree_destory - Destory dbtree tree
 * @param dbtree - dbtree
 * @return void
 */
NNG_DECL void dbtree_destory(dbtree *db);

/**
 * @brief dbtree_print - Print dbtree for debug.
 * @param dbtree - dbtree
 * @return void
 */
NNG_DECL void dbtree_print(dbtree *db);

/**
 * @brief dbtree_insert_client - check if this
 * topic and pipe id is exist on the tree, if
 * there is not exist, this func will insert node
 * recursively until find all topic then insert
 * client on the node.
 * @param dbtree - dbtree_node
 * @param topic - topic
 * @param pipe_id - pipe id
 * @return
 */
NNG_DECL void *dbtree_insert_client(
    dbtree *db, char *topic, uint32_t pipe_id);

/**
 * @brief dbtree_find_client - check if this
 * topic and pipe id is exist on the tree, if
 * there is not exist, return it.
 * @param dbtree - dbtree_node
 * @param topic - topic
 * @param ctxt - data related with pipe_id
 * @param pipe_id - pipe id
 * @return
 */
// void *dbtree_find_client(dbtree *db, char *topic, uint32_t pipe_id);

/**
 * @brief dbtree_delete_client - This function will
 * be called when disconnection and cleansession = 1.
 * check if this topic and client id is exist on the
 * tree, if there is exist, this func will delete
 * related node and client on the tree
 * @param dbtree - dbtree
 * @param topic - topic
 * @param pipe_id - pipe id
 * @return
 */
NNG_DECL void *dbtree_delete_client(
    dbtree *db, char *topic, uint32_t pipe_id);

/**
 * @brief dbtree_find_clients_and_cache_msg - Get all
 * subscribers online to this topic
 * @param dbtree - dbtree
 * @param topic - topic
 * @return pipe id array
 */
NNG_DECL uint32_t *dbtree_find_clients(dbtree *db, char *topic);

/**
 * @brief dbtree_insert_retain - Insert retain message to this topic.
 * @param db - dbtree
 * @param topic - topic
 * @param ret_msg - dbtree_retain_msg
 * @return
 */
NNG_DECL nng_msg *dbtree_insert_retain(
    dbtree *db, char *topic, nng_msg *ret_msg);

/**
 * @brief dbtree_delete_retain - Delete all retain message to this topic.
 * @param db - dbtree
 * @param topic - topic
 * @return ctxt or NULL, if client can be delete or not
 */
NNG_DECL nng_msg *dbtree_delete_retain(dbtree *db, char *topic);

/**
 * @brief dbtree_find_retain - Get all retain message to this topic.
 * @param db - dbtree
 * @param topic - topic
 * @return dbtree_retain_msg pointer vector
 */
NNG_DECL nng_msg **dbtree_find_retain(dbtree *db, char *topic);

/**
 * @brief dbtree_find_shared_clients - This function
 * will Find shared subscribe client.
 * @param dbtree - dbtree
 * @param topic - topic
 * @return pipe id array
 */
NNG_DECL uint32_t *dbtree_find_shared_clients(dbtree *db, char *topic);

/**
 * @brief dbtree_get_tree - This function will
 * get all info about this tree.
 * @param dbtree - dbtree
 * @param cb - a callback function
 * @return all info about this tree
 */
NNG_DECL void ***dbtree_get_tree(dbtree *db, void *(*cb)(uint32_t pipe_id));

#endif
