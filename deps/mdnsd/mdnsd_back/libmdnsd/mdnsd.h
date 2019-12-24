#ifndef MDNSD_H_
#define MDNSD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mdnsd_config.h"
#include "1035.h"
#if !defined(__bool_true_false_are_defined) && defined(_MSC_VER) && _MSC_VER < 1600
// VS 2008 has no stdbool.h
#define bool	short
#define true	1
#define false	0
#else
#include <stdbool.h>
#endif
#include <stdio.h>

#define QCLASS_IN (1)

#if MDNSD_LOGLEVEL <= 100
#define MDNSD_LOG_TRACE(...) do { \
		printf("mdnsd: TRACE - "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define MDNSD_LOG_TRACE(...) do {} while(0)
#endif

#if MDNSD_LOGLEVEL <= 200
#define MDNSD_LOG_DEBUG(...) do { \
		printf("mdnsd: DEBUG - "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define MDNSD_LOG_DEBUG(...) do {} while(0)
#endif

#if MDNSD_LOGLEVEL <= 300
#define MDNSD_LOG_INFO(...) do { \
		printf("mdnsd: INFO  - "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define MDNSD_LOG_INFO(...) do {} while(0)
#endif

#if MDNSD_LOGLEVEL <= 400
#define MDNSD_LOG_WARNING(...) do { \
		printf("mdnsd: WARN  - "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define MDNSD_LOG_WARNING(...) do {} while(0)
#endif

#if MDNSD_LOGLEVEL <= 500
#define MDNSD_LOG_ERROR(...) do { \
		printf("mdnsd: ERROR - "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define MDNSD_LOG_ERROR(...) do {} while(0)
#endif

#if MDNSD_LOGLEVEL <= 600
#define MDNSD_LOG_FATAL(...) do { \
		printf("mdnsd: FATAL - "); printf(__VA_ARGS__); printf("\n"); } while(0)
#else
#define MDNSD_LOG_FATAL(...) do {} while(0)
#endif


/* Main daemon data */
typedef struct mdns_daemon mdns_daemon_t;
/* Record entry */
typedef struct mdns_record mdns_record_t;

/* Callback for received record. Data is passed from the register call */
typedef void (*mdnsd_record_received_callback)(const struct resource* r, void* data);

/* Answer data */
typedef struct mdns_answer {
	char *name;
	unsigned short type;
	unsigned long ttl;
	unsigned short rdlen;
	unsigned char *rdata;
	struct in_addr ip;	/* A */
	char *rdname;		/* NS/CNAME/PTR/SRV */
	struct {
		//cppcheck-suppress unusedStructMember
		unsigned short int priority, weight, port;
	} srv;			/* SRV */
} mdns_answer_t;

/**
 * Global functions
 */

/**
 * Create a new mdns daemon for the given class of names (usually 1) and
 * maximum frame size
 */
mdns_daemon_t MDNSD_EXPORT * mdnsd_new(int clazz, int frame);

/**
 * Gracefully shutdown the daemon, use mdnsd_out() to get the last
 * packets
 */
void MDNSD_EXPORT mdnsd_shutdown(mdns_daemon_t *d);

/**
 * Flush all cached records (network/interface changed)
 */
void MDNSD_EXPORT mdnsd_flush(mdns_daemon_t *d);

/**
 * Free given mdns_daemon_t *(should have used mdnsd_shutdown() first!)
 */
void MDNSD_EXPORT mdnsd_free(mdns_daemon_t *d);

/**
 * Register callback which is called when a record is received. The data parameter is passed to the callback.
 * Calling this multiple times overwrites the previous register.
 */
void MDNSD_EXPORT mdnsd_register_receive_callback(mdns_daemon_t *d, mdnsd_record_received_callback cb, void* data);

/**
 * I/O functions
 */

/**
 * Oncoming message from host (to be cached/processed)
 */
int MDNSD_EXPORT mdnsd_in(mdns_daemon_t *d, struct message *m, in_addr_t ip, unsigned short int port);

/**
 * Outgoing messge to be delivered to host, returns >0 if one was
 * returned and m/ip/port set
 */
int MDNSD_EXPORT mdnsd_out(mdns_daemon_t *d, struct message *m, struct in_addr *ip, unsigned short int *port);

/**
 * returns the max wait-time until mdnsd_out() needs to be called again 
 */
struct timeval MDNSD_EXPORT * mdnsd_sleep(mdns_daemon_t *d);

/**
 * Q/A functions
 */

/**
 * Register a new query
 *
 * The answer() callback is called whenever one is found/changes/expires
 * (immediate or anytime after, mdns_answer_t valid until ->ttl==0)
 * either answer returns -1, or another mdnsd_query() with a %NULL answer
 * will remove/unregister this query
 */
void MDNSD_EXPORT mdnsd_query(mdns_daemon_t *d, const char *host, int type, int (*answer)(mdns_answer_t *a, void *arg), void *arg);

/**
 * Returns the first (if last == NULL) or next answer after last from
 * the cache mdns_answer_t only valid until an I/O function is called
 */
mdns_answer_t MDNSD_EXPORT *mdnsd_list(mdns_daemon_t *d, const char *host, int type, mdns_answer_t *last);

/**
 * Returns the next record of the given record, i.e. the value of next field.
 * @param r the base record
 * @return r->next
 */
mdns_record_t MDNSD_EXPORT *mdnsd_record_next(const mdns_record_t* r) ;

/**
 * Gets the record data
 */
const mdns_answer_t MDNSD_EXPORT *mdnsd_record_data(const mdns_record_t* r) ;


/**
 * Publishing functions
 */

/**
 * Create a new unique record
 *
 * Call mdnsd_list() first to make sure the record is not used yet.
 *
 * The conflict() callback is called at any point when one is detected
 * and unable to recover after the first data is set_*(), any future
 * changes effectively expire the old one and attempt to create a new
 * unique record
 */
mdns_record_t MDNSD_EXPORT * mdnsd_unique(mdns_daemon_t *d, const char *host, unsigned short int type, unsigned long int ttl, void (*conflict)(char *host, int type, void *arg), void *arg);

/** 
 * Create a new shared record
 */
mdns_record_t MDNSD_EXPORT * mdnsd_shared(mdns_daemon_t *d, const char *host, unsigned short int type, unsigned long int ttl);

/**
 * Get a previously created record based on the host name. NULL if not found. Does not return records for other hosts.
 * If multiple records are found, use record->next to iterate over all the results.
 */
mdns_record_t MDNSD_EXPORT * mdnsd_get_published(const mdns_daemon_t *d, const char *host);

/**
 * Check if there is already a query for the given host
 */
int MDNSD_EXPORT mdnsd_has_query(const mdns_daemon_t *d, const char *host);

/**
 * de-list the given record
 */
void MDNSD_EXPORT mdnsd_done(mdns_daemon_t *d, mdns_record_t *r);

/**
 * These all set/update the data for the given record, nothing is
 * published until they are called
 */
void MDNSD_EXPORT mdnsd_set_raw(mdns_daemon_t *d, mdns_record_t *r, const char *data, unsigned short int len);
void MDNSD_EXPORT mdnsd_set_host(mdns_daemon_t *d, mdns_record_t *r, const char *name);
void MDNSD_EXPORT mdnsd_set_ip(mdns_daemon_t *d, mdns_record_t *r, struct in_addr ip);
void MDNSD_EXPORT mdnsd_set_srv(mdns_daemon_t *d, mdns_record_t *r, unsigned short int priority, unsigned short int weight, unsigned short int port, char *name);

/**
 * Process input queue and output queue. Should be called at least the time which is returned in nextSleep.
 * Returns 0 on success, 1 on read error, 2 on write error
 */
unsigned short int MDNSD_EXPORT mdnsd_step(mdns_daemon_t *d, int mdns_socket, bool processIn, bool processOut, struct timeval *nextSleep);


#ifdef MDNSD_DEBUG_DUMP_PKGS_FILE
void mdnsd_debug_dumpCompleteChunk(mdns_daemon_t *d, const char* data, size_t len);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* MDNSD_H_ */
