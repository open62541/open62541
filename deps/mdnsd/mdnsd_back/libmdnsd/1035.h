/* Familiarize yourself with RFC1035 if you want to know what all the
 * variable names mean.  This file hides most of the dirty work all of
 * this code depends on the buffer space a packet is in being 4096 and
 * zero'd before the packet is copied in also conveniently decodes srv
 * rr's, type 33, see RFC2782
 */
#ifndef MDNS_1035_H_
#define MDNS_1035_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mdnsd_config.h"

#ifdef _WIN32

/* Backup definition of SLIST_ENTRY on mingw winnt.h */
#ifdef SLIST_ENTRY
# pragma push_macro("SLIST_ENTRY")
# undef SLIST_ENTRY
# define POP_SLIST_ENTRY
#endif

/* winnt.h redefines SLIST_ENTRY */
# define _WINSOCK_DEPRECATED_NO_WARNINGS /* inet_ntoa is deprecated on MSVC but used for compatibility */
# include <winsock2.h>
# include <ws2tcpip.h>

#ifndef in_addr_t
#define in_addr_t unsigned __int32
#endif

/* restore definition */
#ifdef POP_SLIST_ENTRY
# undef SLIST_ENTRY
# undef POP_SLIST_ENTRY
# pragma pop_macro("SLIST_ENTRY")
#endif


#else
# include <arpa/inet.h>
#endif

#if !defined(__bool_true_false_are_defined) && defined(_MSC_VER) && _MSC_VER < 1600
// VS 2008 has no stdbool.h
#define bool	short
#define true	1
#define false	0
#else
#include <stdbool.h>
#endif

#ifdef _MSC_VER
#include "ms_stdint.h" /* Includes stdint.h or workaround for older Visual Studios */
#else
#include <stdint.h>
#endif

/* Should be reasonably large, for UDP */
#define MAX_PACKET_LEN 10000
#define MAX_NUM_LABELS 20

struct question {
	char *name;
	unsigned short type, clazz;
};

#define QTYPE_A      1
#define QTYPE_NS     2
#define QTYPE_CNAME  5
#define QTYPE_PTR    12
#define QTYPE_TXT    16
#define QTYPE_SRV    33

struct resource {
	char *name;
	unsigned short type, clazz;
	unsigned long int ttl;
	unsigned short int rdlength;
	unsigned char *rdata;
	union {
		struct {
			struct in_addr ip;
			//cppcheck-suppress unusedStructMember
			char *name;
		} a;
		struct {
			//cppcheck-suppress unusedStructMember
			char *name;
		} ns;
		struct {
			//cppcheck-suppress unusedStructMember
			char *name;
		} cname;
		struct {
			//cppcheck-suppress unusedStructMember
			char *name;
		} ptr;
		struct {
			//cppcheck-suppress unusedStructMember
			unsigned short int priority, weight, port;
			//cppcheck-suppress unusedStructMember
			char *name;
		} srv;
	} known;
};

struct message {
	/* External data */
	unsigned short int id;
	struct {
		//it would be better to use unsigned short, but gcc < 5 complaints when using pedantic
		//cppcheck-suppress unusedStructMember
		unsigned int qr:1, opcode:4, aa:1, tc:1, rd:1, ra:1, z:3, rcode:4;
	} header;
	unsigned short int qdcount, ancount, nscount, arcount;
	struct question *qd;
	struct resource *an, *ns, *ar;

	/* Internal variables */
	unsigned char *_buf;
	unsigned char *_bufEnd;
	char *_labels[MAX_NUM_LABELS];
	size_t _len;
	int _label;

	/* Packet acts as padding, easier mem management */
	unsigned char _packet[MAX_PACKET_LEN];
};

/**
 * Returns the next short/long off the buffer (and advances it)
 */
uint16_t net2short(const unsigned char **bufp);
uint32_t  net2long (const unsigned char **bufp);

/**
 * copies the short/long into the buffer (and advances it)
 */
void MDNSD_EXPORT short2net(uint16_t i, unsigned char **bufp);
void MDNSD_EXPORT long2net (uint32_t  l, unsigned char **bufp);

/**
 * parse packet into message, packet must be at least MAX_PACKET_LEN and
 * message must be zero'd for safety
 */
bool MDNSD_EXPORT message_parse(struct message *m, unsigned char *packet, size_t packetLen);

/**
 * create a message for sending out on the wire
 */
struct message *message_wire(void);

/**
 * append a question to the wire message
 */
void MDNSD_EXPORT message_qd(struct message *m, char *name, unsigned short int type, unsigned short int clazz);

/**
 * append a resource record to the message, all called in order!
 */
void MDNSD_EXPORT message_an(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl);
void MDNSD_EXPORT message_ns(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl);
void MDNSD_EXPORT message_ar(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl);

/**
 * Append various special types of resource data blocks
 */
void MDNSD_EXPORT message_rdata_long (struct message *m, struct in_addr l);
void MDNSD_EXPORT message_rdata_name (struct message *m, char *name);
void MDNSD_EXPORT message_rdata_srv  (struct message *m, unsigned short int priority, unsigned short int weight,
			 unsigned short int port, char *name);
void MDNSD_EXPORT message_rdata_raw  (struct message *m, unsigned char *rdata, unsigned short int rdlength);

/**
 * Return the wire format (and length) of the message, just free message
 * when done
 */
unsigned char MDNSD_EXPORT * message_packet     (struct message *m);
int   MDNSD_EXPORT           message_packet_len (struct message *m);

#ifdef __cplusplus
} // extern "C"
#endif

#endif	/* MDNS_1035_H_ */
