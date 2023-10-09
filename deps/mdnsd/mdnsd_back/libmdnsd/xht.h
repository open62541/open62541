/*  Simple string->void* hashtable, very static and bare minimal, but efficient */
#ifndef MDNS_XHT_H_
#define MDNS_XHT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mdnsd_config.h"
typedef struct xht xht_t;

/**
 * must pass a prime#
 */
xht_t MDNSD_EXPORT *xht_new(int prime);

/**
 * caller responsible for key storage, no copies made
 *
 * set val to NULL to clear an entry, memory is reused but never free'd
 * (# of keys only grows to peak usage)
 *
 * Note: don't free it b4 xht_free()!
 */
void MDNSD_EXPORT xht_set(xht_t *h, char *key, void *val);

/**
 * Unlike xht_set() where key/val is in caller's mem, here they are
 * copied into xht and free'd when val is 0 or xht_free()
 */
void MDNSD_EXPORT xht_store(xht_t *h, char *key, int klen, void *val, int vlen);

/**
 * returns value of val if found, or NULL
 */
void MDNSD_EXPORT *xht_get(xht_t *h, char *key);

/**
 * free the hashtable and all entries
 */
void MDNSD_EXPORT xht_free(xht_t *h);

/**
 * pass a function that is called for every key that has a value set
 */
typedef void (*xht_walker)(xht_t *h, char *key, void *val, void *arg);
void xht_walk(xht_t *h, xht_walker w, void *arg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* MDNS_XHT_H_ */
