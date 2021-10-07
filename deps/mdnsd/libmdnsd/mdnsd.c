#include "mdnsd_config.h"
#include "mdnsd.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define SPRIME 108		/* Size of query/publish hashes */
#define LPRIME 1009		/* Size of cache hash */

#define GC 86400                /* Brute force garbage cleanup
				 * frequency, rarely needed (daily
				 * default) */

#ifdef _MSC_VER
#include "ms_stdint.h" /* Includes stdint.h or workaround for older Visual Studios */

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime( &system_time );
	SystemTimeToFileTime( &system_time, &file_time );
	time =  ((uint64_t)file_time.dwLowDateTime )      ;
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
	return 0;
}
#else
#include <sys/time.h>
#endif

#if defined(__MINGW32__)
static char *my_strdup(const char *s) {
    char *p = (char *)MDNSD_malloc(strlen(s) + 1);
    if(p) { strcpy(p, s); }
    return p;
}
#define STRDUP my_strdup
#elif defined(_WIN32)
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

#ifndef _WIN32
# include <netdb.h>
#endif

/**
 * Messy, but it's the best/simplest balance I can find at the moment
 *
 * Some internal data types, and a few hashes: querys, answers, cached,
 * and records (published, unique and shared).  Each type has different
 * semantics for processing, both for timeouts, incoming, and outgoing
 * I/O.  They inter-relate too, like records affect the querys they are
 * relevant to.  Nice things about MDNS: we only publish once (and then
 * ask asked), and only query once, then just expire records we've got
 * cached
*/

struct query {
	char *name;
	int type;
	unsigned long int nexttry;
	int tries;
	int (*answer)(mdns_answer_t *, void *);
	void *arg;
	struct query *next, *list;
};

struct unicast {
	int id;
    in_addr_t to;
	unsigned short int port;
	mdns_record_t *r;
	struct unicast *next;
};

struct cached {
	struct mdns_answer rr;
	struct query *q;
	struct cached *next;
};

struct mdns_record {
	struct mdns_answer rr;
	char unique;		/* # of checks performed to ensure */
	int tries;
	void (*conflict)(char *, int, void *);
	void *arg;
	struct timeval last_sent;
	struct mdns_record *next, *list;
};

struct mdns_daemon {
	char shutdown;
	unsigned long int expireall, checkqlist;
	struct timeval now, sleep, pause, probe, publish;
	int clazz, frame;
	struct cached *cache[LPRIME];
	struct mdns_record *published[SPRIME], *probing, *a_now, *a_pause, *a_publish;
	struct unicast *uanswers;
	struct query *queries[SPRIME], *qlist;
	mdnsd_record_received_callback received_callback;
	void *received_callback_data;
};

static int _namehash(const char *s)
{
	const unsigned char *name = (const unsigned char *)s;
	unsigned long h = 0;

	while (*name) {		/* do some fancy bitwanking on the string */
		unsigned long int g;
		h = (h << 4) + (unsigned long int)(*name++);
		if ((g = (h & 0xF0000000UL)) != 0)
			h ^= (g >> 24);
		h &= ~g;
	}

	return (int)h;
}

/* Basic linked list and hash primitives */
static struct query *_q_next(mdns_daemon_t *d, struct query *q, const char *host, int type)
{
	if (q == 0)
		q = d->queries[_namehash(host) % SPRIME];
	else
		q = q->next;

	for (; q != 0; q = q->next) {
		if (q->type == type && strcmp(q->name, host) == 0)
			return q;
	}

	return 0;
}

static struct cached *_c_next(mdns_daemon_t *d, struct cached *c,const char *host, int type)
{
	if (c == 0)
		c = d->cache[_namehash(host) % LPRIME];
	else
		c = c->next;

	for (; c != 0; c = c->next) {
		if ((type == c->rr.type || type == 255) && strcmp(c->rr.name, host) == 0)
			return c;
	}

	return 0;
}

static mdns_record_t *_r_next(mdns_daemon_t *d, mdns_record_t *r, const char *host, int type)
{
	if (r == 0)
		r = d->published[_namehash(host) % SPRIME];
	else
		r = r->next;

	for (; r != 0; r = r->next) {
		if (type == r->rr.type && strcmp(r->rr.name, host) == 0)
			return r;
	}

	return 0;
}

static int _rr_len(mdns_answer_t *rr)
{
	int len = 12;		/* name is always compressed (dup of earlier), plus normal stuff */

	if (rr->rdata)
		len += rr->rdlen;
	if (rr->rdname)
		len += (int)strlen(rr->rdname); /* worst case */
	if (rr->ip.s_addr)
		len += 4;
	if (rr->type == QTYPE_PTR)
		len += 6;	/* srv record stuff */

	return len;
}

/* Compares new rdata with known a, painfully */
static int _a_match(struct resource *r, mdns_answer_t *a)
{
	if (!a->name)
		return 0;
	if (strcmp(r->name, a->name) != 0 || r->type != a->type)
		return 0;

	if (r->type == QTYPE_SRV && !strcmp(r->known.srv.name, a->rdname) && a->srv.port == r->known.srv.port &&
		a->srv.weight == r->known.srv.weight && a->srv.priority == r->known.srv.priority)
		return 1;

	if ((r->type == QTYPE_PTR || r->type == QTYPE_NS || r->type == QTYPE_CNAME) && !strcmp(a->rdname, r->known.ns.name))
		return 1;

	if (r->rdlength == a->rdlen && r->rdlength == 0)
	    return 1;

	if ((r->rdlength == a->rdlen) && !memcmp(r->rdata, a->rdata, r->rdlength))
		return 1;

	return 0;
}

/* Compare time values easily */
static int _tvdiff(struct timeval old_time, struct timeval new_time)
{
	int udiff = 0;

	if (old_time.tv_sec != new_time.tv_sec)
		udiff = (int)((new_time.tv_sec - old_time.tv_sec) * 1000000);

	return (int)((new_time.tv_usec - old_time.tv_usec) + udiff);
}

static void _r_remove_list(mdns_record_t **list, mdns_record_t *r) {
	if (*list == r) {
		*list = r->list;
	} else {
		mdns_record_t *tmp = *list;
		while (tmp) {
			if (tmp->list == r) {
				tmp->list = r->list;
				break;
			}
			if (tmp == tmp->list)
				break;
			tmp = tmp->list;
		}
	}
}

static void _r_remove_lists(mdns_daemon_t *d, mdns_record_t *r, mdns_record_t **skip) {
	if (d->probing && &d->probing != skip) {
		_r_remove_list(&d->probing, r);
	}
	if (d->a_now && &d->a_now != skip) {
		_r_remove_list(&d->a_now, r);
	}
	if (d->a_pause && &d->a_pause != skip) {
		_r_remove_list(&d->a_pause, r);
	}
	if (d->a_publish && &d->a_publish != skip) {
		_r_remove_list(&d->a_publish, r);
	}
}

/* Make sure not already on the list, then insert */
static void _r_push(mdns_record_t **list, mdns_record_t *r)
{
	mdns_record_t *cur;

	for (cur = *list; cur != 0; cur = cur->list) {
		if (cur == r)
			return;
	}

	r->list = *list;
	*list = r;
}

/* Force any r out right away, if valid */
static void _r_publish(mdns_daemon_t *d, mdns_record_t *r)
{
	if (r->unique && r->unique < 5)
		return;		/* Probing already */

	r->tries = 0;
	d->publish.tv_sec = d->now.tv_sec;
	d->publish.tv_usec = d->now.tv_usec;
	_r_push(&d->a_publish, r);
}

/* send r out asap */
static void _r_send(mdns_daemon_t *d, mdns_record_t *r)
{
	/* Being published, make sure that happens soon */
	if (r->tries < 4) {
		d->publish.tv_sec = d->now.tv_sec;
		d->publish.tv_usec = d->now.tv_usec;
		return;
	}

	/* Known unique ones can be sent asap */
	if (r->unique) {

		// check if r already in other lists. If yes, remove it from there
		_r_remove_lists(d,r, &d->a_now);
		_r_push(&d->a_now, r);
		return;
	}

	/* Set d->pause.tv_usec to random 20-120 msec */
	d->pause.tv_sec = d->now.tv_sec;
	d->pause.tv_usec = d->now.tv_usec + (d->now.tv_usec % 100) + 20;
	_r_push(&d->a_pause, r);
}

/* Create generic unicast response struct */
static void _u_push(mdns_daemon_t *d, mdns_record_t *r, int id, in_addr_t to, unsigned short int port)
{
	struct unicast *u;

	u = (struct unicast *)MDNSD_calloc(1, sizeof(struct unicast));
	u->r = r;
	u->id = id;
	u->to = to;
	u->port = port;
	u->next = d->uanswers;
	d->uanswers = u;
}

static void _q_reset(mdns_daemon_t *d, struct query *q)
{
	struct cached *cur = 0;

	q->nexttry = 0;
	q->tries = 0;

	while ((cur = _c_next(d, cur, q->name, q->type))) {
		if (q->nexttry == 0 || cur->rr.ttl - 7 < q->nexttry)
			q->nexttry = cur->rr.ttl - 7;
	}

	if (q->nexttry != 0 && q->nexttry < d->checkqlist)
		d->checkqlist = q->nexttry;
}

/* No more query, update all it's cached entries, remove from lists */
static void _q_done(mdns_daemon_t *d, struct query *q)
{
	struct cached *c = 0;
	struct query *cur;
	int i = _namehash(q->name) % LPRIME;

	while ((c = _c_next(d, c, q->name, q->type)))
		c->q = 0;

	if (d->qlist == q) {
		d->qlist = q->list;
	} else {
		for (cur = d->qlist; cur->list != q; cur = cur->list)
			;
		cur->list = q->list;
	}

	if (d->queries[i] == q) {
		d->queries[i] = q->next;
	} else {
		for (cur = d->queries[i]; cur->next != q; cur = cur->next)
			;
		cur->next = q->next;
	}

	MDNSD_free(q->name);
	MDNSD_free(q);
}

/* buh-bye, remove from hash and free */
static void _r_done(mdns_daemon_t *d, mdns_record_t *r)
{
	mdns_record_t *cur = 0;
	int i = _namehash(r->rr.name) % SPRIME;

	if (d->published[i] == r)
		d->published[i] = r->next;
	else {
		for (cur = d->published[i]; cur && cur->next != r; cur = cur->next) ;
		if (cur)
			cur->next = r->next;
	}
	MDNSD_free(r->rr.name);
	MDNSD_free(r->rr.rdata);
	MDNSD_free(r->rr.rdname);
	MDNSD_free(r);
}

/* Call the answer function with this cached entry */
static void _q_answer(mdns_daemon_t *d, struct cached *c)
{
	if (c->rr.ttl <= (unsigned long int)d->now.tv_sec)
		c->rr.ttl = 0;
	if (c->q->answer(&c->rr, c->q->arg) == -1)
		_q_done(d, c->q);
}

static void _conflict(mdns_daemon_t *d, mdns_record_t *r)
{
	r->conflict(r->rr.name, r->rr.type, r->arg);
	mdnsd_done(d, r);
}

/* Expire any old entries in this list */
static void _c_expire(mdns_daemon_t *d, struct cached **list)
{
	struct cached *next, *cur = *list, *last = 0;

	while (cur != 0) {
		next = cur->next;
		if ((unsigned long int)d->now.tv_sec >= cur->rr.ttl) {
			if (last)
				last->next = next;

			/* Update list pointer if the first one expired */
			if (*list == cur)
				*list = next;

			if (cur->q)
				_q_answer(d, cur);

			MDNSD_free(cur->rr.name);
			MDNSD_free(cur->rr.rdata);
			MDNSD_free(cur->rr.rdname);
			MDNSD_free(cur);
		} else {
			last = cur;
		}
		cur = next;
	}
}

/* Brute force expire any old cached records */
static void _gc(mdns_daemon_t *d)
{
	int i;

	for (i = 0; i < LPRIME; i++) {
		if (d->cache[i])
			_c_expire(d, &d->cache[i]);
	}

	d->expireall = (unsigned long int)(d->now.tv_sec + GC);
}

static int _cache(mdns_daemon_t *d, struct resource *r)
{
	struct cached *c = 0;
	int i = _namehash(r->name) % LPRIME;

	/* Cache flush for unique entries */
	if (r->clazz == 32768 + d->clazz) {
		while ((c = _c_next(d, c, r->name, r->type)))
			c->rr.ttl = 0;
		_c_expire(d, &d->cache[i]);
	}

	/* Process deletes */
	if (r->ttl == 0) {
		while ((c = _c_next(d, c, r->name, r->type))) {
			if (_a_match(r, &c->rr)) {
				c->rr.ttl = 0;
				_c_expire(d, &d->cache[i]);
				c = NULL;
			}
		}

		return 0;
	}

	/*
	 * XXX: The c->rr.ttl is a hack for now, BAD SPEC, start
	 *      retrying just after half-waypoint, then expire
	 */
	c = (struct cached *)MDNSD_calloc(1, sizeof(struct cached));
	c->rr.name = STRDUP(r->name);
	c->rr.type = r->type;
	c->rr.ttl = (unsigned int)((unsigned long)d->now.tv_sec + (r->ttl / 2) + 8);
	c->rr.rdlen = r->rdlength;
	if (r->rdlength && !r->rdata) {
		//MDNSD_LOG_ERROR("rdlength is %d but rdata is NULL for domain name %s, type: %d, ttl: %ld", r->rdlength, r->name, r->type, r->ttl);
		MDNSD_free(c->rr.name);
		MDNSD_free(c);
		return 1;
	}
	if (r->rdlength) {
		c->rr.rdata = (unsigned char *)MDNSD_malloc(r->rdlength);
		memcpy(c->rr.rdata, r->rdata, r->rdlength);
	} else {
		c->rr.rdata = NULL;
	}

	switch (r->type) {
		case QTYPE_A:
			c->rr.ip = r->known.a.ip;
			break;

		case QTYPE_NS:
		case QTYPE_CNAME:
		case QTYPE_PTR:
			c->rr.rdname = STRDUP(r->known.ns.name);
			break;

		case QTYPE_SRV:
			c->rr.rdname = STRDUP(r->known.srv.name);
			c->rr.srv.port = r->known.srv.port;
			c->rr.srv.weight = r->known.srv.weight;
			c->rr.srv.priority = r->known.srv.priority;
			break;
	}

	c->next = d->cache[i];
	d->cache[i] = c;

	if ((c->q = _q_next(d, 0, r->name, r->type)))
		_q_answer(d, c);

	return 0;
}

/* Copy the data bits only */
static void _a_copy(struct message *m, mdns_answer_t *a)
{
	if (a->rdata) {
		message_rdata_raw(m, a->rdata, a->rdlen);
		return;
	}

	if (a->ip.s_addr)
		message_rdata_long(m, a->ip);
	if (a->type == QTYPE_SRV)
		message_rdata_srv(m, a->srv.priority, a->srv.weight, a->srv.port, a->rdname);
	else if (a->rdname)
		message_rdata_name(m, a->rdname);
}

/* Copy a published record into an outgoing message */
static int _r_out(mdns_daemon_t *d, struct message *m, mdns_record_t **list)
{
	mdns_record_t *r;
	int ret = 0;

	while ((r = *list) != 0 && message_packet_len(m) + _rr_len(&r->rr) < d->frame) {
		if (r != r->list)
			*list = r->list;
		else
			*list = NULL;
		ret++;

		if (r->unique)
			message_an(m, r->rr.name, r->rr.type, (unsigned short int)(d->clazz + 32768), r->rr.ttl);
		else
			message_an(m, r->rr.name, r->rr.type,  (unsigned short int)d->clazz, r->rr.ttl);
		r->last_sent = d->now;

		_a_copy(m, &r->rr);
		if (r->rr.ttl == 0) {

			// also remove from other lists, because record may be in multiple lists at the same time
			_r_remove_lists(d, r, list);

			_r_done(d, r);

		}
	}

	return ret;
}


mdns_daemon_t *mdnsd_new(int clazz, int frame)
{
	mdns_daemon_t *d;

	d = (mdns_daemon_t *)MDNSD_calloc(1, sizeof(struct mdns_daemon));
	gettimeofday(&d->now, 0);
	d->expireall = (unsigned long int)(d->now.tv_sec + GC);
	d->clazz = clazz;
	d->frame = frame;
	d->received_callback = NULL;

	return d;
}

/* Shutting down, zero out ttl and push out all records */
void mdnsd_shutdown(mdns_daemon_t *d)
{
	int i;
	mdns_record_t *cur, *next;

	d->a_now = 0;
	for (i = 0; i < SPRIME; i++) {
		for (cur = d->published[i]; cur != 0;) {
			next = cur->next;
			cur->rr.ttl = 0;
			cur->list = d->a_now;
			d->a_now = cur;
			cur = next;
		}
	}

	d->shutdown = 1;
}

void mdnsd_flush(mdns_daemon_t *d)
{
	(void)d;
	/* - Set all querys to 0 tries
	 * - Free whole cache
	 * - Set all mdns_record_t *to probing
	 * - Reset all answer lists
	 */
}

void mdnsd_free(mdns_daemon_t *d)
{
	size_t i;
	for (i = 0; i< LPRIME; i++) {
		struct cached* cur = d->cache[i];
		while (cur) {
			struct cached* next = cur->next;
			MDNSD_free(cur->rr.name);
			MDNSD_free(cur->rr.rdata);
			MDNSD_free(cur->rr.rdname);
			MDNSD_free(cur);
			cur = next;
		}
	}

	for (i = 0; i< SPRIME; i++) {
		struct mdns_record* cur = d->published[i];
		struct query* curq = NULL;
		while (cur) {
			struct mdns_record* next = cur->next;
			MDNSD_free(cur->rr.name);
			MDNSD_free(cur->rr.rdata);
			MDNSD_free(cur->rr.rdname);
			MDNSD_free(cur);
			cur = next;
		}


		curq = d->queries[i];
		while (curq) {
			struct query* next = curq->next;
			MDNSD_free(curq->name);
			MDNSD_free(curq);
			curq = next;
		}

	}

	{
		struct unicast *u = d->uanswers;
		while (u) {
			struct unicast *next = u->next;
			MDNSD_free(u);
			u=next;
		}
	}

	MDNSD_free(d);
}


void mdnsd_register_receive_callback(mdns_daemon_t *d, mdnsd_record_received_callback cb, void* data) {
	d->received_callback = cb;
	d->received_callback_data = data;
}

int mdnsd_in(mdns_daemon_t *d, struct message *m, in_addr_t ip, unsigned short int port)
{
	int i;
	mdns_record_t *r = 0;

	if (d->shutdown)
		return 1;

	gettimeofday(&d->now, 0);

	if (m->header.qr == 0) {
		/* Process each query */
		for (i = 0; i < m->qdcount; i++) {
			mdns_record_t *r_start, *r_next = NULL;
			bool hasConflict = false;
			if (m->qd[i].clazz != d->clazz || (r = _r_next(d, 0, m->qd[i].name, m->qd[i].type)) == 0)
				continue;
			r_start = r;


			/* Check all of our potential answers */
			for (; r != 0; r = r_next) {

				MDNSD_LOG_TRACE("Got Query: Name: %s, Type: %d", r->rr.name, r->rr.type);

				// do this here, because _conflict deletes r and thus next is not valid anymore
				r_next = _r_next(d, r, m->qd[i].name, m->qd[i].type);
				/* probing state, check for conflicts */
				if (r->unique && r->unique < 5) {
					/* Check all to-be answers against our own */
					int j;
					for (j = 0; j < m->nscount; j++) {
						if (m->qd[i].type != m->an[j].type || strcmp(m->qd[i].name, m->an[j].name))
							continue;

						/* This answer isn't ours, conflict! */
						if (!_a_match(&m->an[j], &r->rr)) {
							_conflict(d, r);
							hasConflict = true;
							break;
						}
					}
					continue;
				}

				/* Check the known answers for this question */
				{
					int j;
					for (j = 0; j < m->ancount; j++) {
						if (m->qd[i].type != m->an[j].type || strcmp(m->qd[i].name, m->an[j].name))
							continue;

						if (d->received_callback) {
							d->received_callback(&m->an[j], d->received_callback_data);
						}

						/* Do they already have this answer? */
						if (_a_match(&m->an[j], &r->rr))
							break;
					}
					if (j == m->ancount)
						_r_send(d, r);
				}

			}

			/* Send the matching unicast reply */
			if (!hasConflict && port != 5353)
				_u_push(d, r_start, m->id, ip, port);
		}

		return 0;
	}

	/* Process each answer, check for a conflict, and cache */
	for (i = 0; i < m->ancount; i++) {
		if (m->an[i].name == NULL) {
			MDNSD_LOG_ERROR("Got answer with NULL name at %p. Type: %d, TTL: %ld\n", (void*)&m->an[i], m->an[i].type, m->an[i].ttl);
			return 3;
		}

		MDNSD_LOG_TRACE("Got Answer: Name: %s, Type: %d", m->an[i].name, m->an[i].type);
		if ((r = _r_next(d, 0, m->an[i].name, m->an[i].type)) != 0 &&
			r->unique && _a_match(&m->an[i], &r->rr) == 0)
			_conflict(d, r);

		if (d->received_callback) {
			d->received_callback(&m->an[i], d->received_callback_data);
		}
		if (_cache(d, &m->an[i]) != 0)
			return 2;
	}
	return 0;
}

int mdnsd_out(mdns_daemon_t *d, struct message *m, struct in_addr *ip, unsigned short int *port)
{
	mdns_record_t *r;
	int ret = 0;

	gettimeofday(&d->now, 0);
	memset(m, 0, sizeof(struct message));

	/* Defaults, multicast */
	*port = htons(5353);
	ip->s_addr = inet_addr("224.0.0.251");
	m->header.qr = 1;
	m->header.aa = 1;

	/* Send out individual unicast answers */
	if (d->uanswers) {
		struct unicast *u = d->uanswers;

		MDNSD_LOG_TRACE("Send Unicast Answer: Name: %s, Type: %d", u->r->rr.name, u->r->rr.type);

		d->uanswers = u->next;
		*port = u->port;
		ip->s_addr = u->to;
		m->id = (unsigned short int)u->id;
		message_qd(m, u->r->rr.name, u->r->rr.type, (unsigned short int)d->clazz);
		message_an(m, u->r->rr.name, u->r->rr.type, (unsigned short int)d->clazz, u->r->rr.ttl);
		u->r->last_sent = d->now;
		_a_copy(m, &u->r->rr);
		MDNSD_free(u);

		return 1;
	}

//	printf("OUT: probing %X now %X pause %X publish %X\n",d->probing,d->a_now,d->a_pause,d->a_publish);

	/* Accumulate any immediate responses */
	if (d->a_now)
		ret += _r_out(d, m, &d->a_now);

	/* Check if it's time to send the publish retries (unlink if done) */
	if (d->a_publish && _tvdiff(d->now, d->publish) <= 0) {

		mdns_record_t *next, *cur = d->a_publish, *last = NULL;

		while (cur && message_packet_len(m) + _rr_len(&cur->rr) < d->frame) {

			if (cur->rr.type == QTYPE_PTR) {
				MDNSD_LOG_TRACE("Send Publish PTR: Name: %s, rdlen: %d, rdata: %.*s, rdname: %s",
				        cur->rr.name,cur->rr.rdlen, cur->rr.rdlen, cur->rr.rdata, cur->rr.rdname == NULL ? "" : cur->rr.rdname);
			} else if (cur->rr.type == QTYPE_SRV) {
				MDNSD_LOG_TRACE("Send Publish SRV: Name: %s, rdlen: %d, rdata: %.*s, rdname: %s, port: %d, prio: %d, weight: %d",
				        cur->rr.name,cur->rr.rdlen, cur->rr.rdlen, cur->rr.rdata, cur->rr.rdname == NULL ? "" : cur->rr.rdname,
				        cur->rr.srv.port, cur->rr.srv.priority, cur->rr.srv.weight);
			} else {
				MDNSD_LOG_TRACE("Send Publish: Name: %s, Type: %d", cur->rr.name, cur->rr.type);
			}
			next = cur->list;
			ret++;
			cur->tries++;

			if (cur->unique)
				message_an(m, cur->rr.name, cur->rr.type, (unsigned short int)(d->clazz + 32768), cur->rr.ttl);
			else
				message_an(m, cur->rr.name, cur->rr.type, (unsigned short int)d->clazz, cur->rr.ttl);
			_a_copy(m, &cur->rr);
			cur->last_sent = d->now;

			if (cur->rr.ttl != 0 && cur->tries < 4) {
				last = cur;
				cur = next;
				continue;
			}

			if (d->a_publish == cur)
				d->a_publish = next;
			if (last)
				last->list = next;
			if (cur->rr.ttl == 0)
				_r_done(d, cur);
			cur = next;
		}

		if (d->a_publish) {
			d->publish.tv_sec = d->now.tv_sec + 2;
			d->publish.tv_usec = d->now.tv_usec;
		}
	}

	/* If we're in shutdown, we're done */
	if (d->shutdown)
		return ret;

	/* Check if a_pause is ready */
	if (d->a_pause && _tvdiff(d->now, d->pause) <= 0)
		ret += _r_out(d, m, &d->a_pause);

	/* Now process questions */
	if (ret > 0)
		return ret;

	m->header.qr = 0;
	m->header.aa = 0;

	if (d->probing && _tvdiff(d->now, d->probe) <= 0) {
		mdns_record_t *last = 0;

		/* Scan probe list to ask questions and process published */
		for (r = d->probing; r != 0;) {
			/* Done probing, publish */
			if (r->unique == 4) {
				mdns_record_t *next = r->list;

				if (d->probing == r)
					d->probing = r->list;
				else
					last->list = r->list;

				r->list = 0;
				r->unique = 5;
				_r_publish(d, r);
				r = next;
				continue;
			}

			MDNSD_LOG_TRACE("Send Probing: Name: %s, Type: %d", r->rr.name, r->rr.type);

			message_qd(m, r->rr.name, r->rr.type, (unsigned short int)d->clazz);
			r->last_sent = d->now;
			last = r;
			r = r->list;
		}

		/* Scan probe list again to append our to-be answers */
		for (r = d->probing; r != 0; r = r->list) {
			r->unique++;

			MDNSD_LOG_TRACE("Send Answer in Probe: Name: %s, Type: %d", r->rr.name, r->rr.type);
			message_ns(m, r->rr.name, r->rr.type, (unsigned short int)d->clazz, r->rr.ttl);
			_a_copy(m, &r->rr);
			r->last_sent = d->now;
			ret++;
		}

		/* Process probes again in the future */
		if (ret) {
			d->probe.tv_sec = d->now.tv_sec;
			d->probe.tv_usec = d->now.tv_usec + 250000;
			return ret;
		}
	}

	/* Process qlist for retries or expirations */
	if (d->checkqlist && (unsigned long int)d->now.tv_sec >= d->checkqlist) {
		struct query *q;
		struct cached *c;
		unsigned long int nextbest = 0;

		/* Ask questions first, track nextbest time */
		for (q = d->qlist; q != 0; q = q->list) {
			if (q->nexttry > 0 && q->nexttry <= (unsigned long int)d->now.tv_sec && q->tries < 3)
				message_qd(m, q->name, (unsigned short int)q->type, (unsigned short int)d->clazz);
			else if (q->nexttry > 0 && (nextbest == 0 || q->nexttry < nextbest))
				nextbest = q->nexttry;
		}

		/* Include known answers, update questions */
		for (q = d->qlist; q != 0; q = q->list) {
			if (q->nexttry == 0 || q->nexttry > (unsigned long int)d->now.tv_sec)
				continue;

			/* Done retrying, expire and reset */
			if (q->tries == 3) {
				_c_expire(d, &d->cache[_namehash(q->name) % LPRIME]);
				_q_reset(d, q);
				continue;
			}

			ret++;
			q->nexttry = (unsigned long int)(d->now.tv_sec + ++q->tries);
			if (nextbest == 0 || q->nexttry < nextbest)
				nextbest = q->nexttry;

			/* If room, add all known good entries */
			c = 0;
			while ((c = _c_next(d, c, q->name, q->type)) != 0 && c->rr.ttl > (unsigned long int)d->now.tv_sec + 8 &&
				   message_packet_len(m) + _rr_len(&c->rr) < d->frame) {

				MDNSD_LOG_TRACE("Add known answer: Name: %s, Type: %d", c->rr.name, c->rr.type);
				message_an(m, q->name, (unsigned short int)q->type, (unsigned short int)d->clazz, c->rr.ttl - (unsigned long int)d->now.tv_sec);
				_a_copy(m, &c->rr);
			}
		}
		d->checkqlist = nextbest;
	}

	if ((unsigned long int)d->now.tv_sec > d->expireall)
		_gc(d);

	return ret;
}


#define RET					\
	while (d->sleep.tv_usec > 1000000) {	\
		d->sleep.tv_sec++;		\
		d->sleep.tv_usec -= 1000000;	\
	}					\
	return &d->sleep;

struct timeval *mdnsd_sleep(mdns_daemon_t *d)
{
	int usec, minExpire;

	d->sleep.tv_sec = d->sleep.tv_usec = 0;

	/* First check for any immediate items to handle */
	if (d->uanswers || d->a_now)
		return &d->sleep;

	gettimeofday(&d->now, 0);

	/* Then check for paused answers or nearly expired records */
	if (d->a_pause) {
		if ((usec = _tvdiff(d->now, d->pause)) > 0)
			d->sleep.tv_usec = usec;
		RET;
	}

	/* Now check for probe retries */
	if (d->probing) {
		if ((usec = _tvdiff(d->now, d->probe)) > 0)
			d->sleep.tv_usec = usec;
		RET;
	}

	/* Now check for publish retries */
	if (d->a_publish) {
		if ((usec = _tvdiff(d->now, d->publish)) > 0)
			d->sleep.tv_usec = usec;
		RET;
	}

	/* Also check for queries with known answer expiration/retry */
	if (d->checkqlist) {
		int sec;
		if ((sec = (int)(d->checkqlist - (unsigned long int)d->now.tv_sec)) > 0)
			d->sleep.tv_sec = sec;
		RET;
	}

	/* Resend published records before TTL expires */
	// latest expire is garbage collection
	minExpire = (int)(d->expireall - (unsigned long int)d->now.tv_sec);
	if (minExpire < 0)
		return &d->sleep;

	{
		size_t i;
		for (i=0; i<SPRIME; i++) {
			int expire;
			if (!d->published[i])
				continue;
			expire = (int)((d->published[i]->last_sent.tv_sec + (long int)d->published[i]->rr.ttl) - d->now.tv_sec);
			if (expire < minExpire)
				d->a_pause = NULL;
			minExpire = expire < minExpire ? expire : minExpire;
			_r_push(&d->a_pause, d->published[i]);
		}
	}
	// publish 2 seconds before expire.
	d->sleep.tv_sec = minExpire > 2 ? minExpire-2 : 0;
	d->pause.tv_sec = d->now.tv_sec + d->sleep.tv_sec;
	RET;
}

void mdnsd_query(mdns_daemon_t *d, const char *host, int type, int (*answer)(mdns_answer_t *a, void *arg), void *arg)
{
	struct query *q;
	int i = _namehash(host) % SPRIME;

	if (!(q = _q_next(d, 0, host, type))) {
		if (!answer)
			return;

		q = (struct query *)MDNSD_calloc(1, sizeof(struct query));
		q->name = STRDUP(host);
		q->type = type;
		q->next = d->queries[i];
		q->list = d->qlist;
		d->qlist = d->queries[i] = q;

		/* Any cached entries should be associated */
		{
			struct cached *cur = 0;
			while ((cur = _c_next(d, cur, q->name, q->type)))
				cur->q = q;
		}
		_q_reset(d, q);

		/* New question, immediately send out */
		q->nexttry = d->checkqlist = (unsigned long int)d->now.tv_sec;
	}

	/* No answer means we don't care anymore */
	if (!answer) {
		_q_done(d, q);
		return;
	}

	q->answer = answer;
	q->arg = arg;
}

mdns_answer_t *mdnsd_list(mdns_daemon_t *d,const char *host, int type, mdns_answer_t *last)
{
	return (mdns_answer_t *)_c_next(d, (struct cached *)last, host, type);
}

mdns_record_t *mdnsd_record_next(const mdns_record_t* r) {
	return r ? r->next : NULL;
}

const mdns_answer_t *mdnsd_record_data(const mdns_record_t* r) {
	return &r->rr;
}

mdns_record_t *mdnsd_shared(mdns_daemon_t *d, const char *host, unsigned short int type, unsigned long int ttl)
{
	int i = _namehash(host) % SPRIME;
	mdns_record_t *r;

	r = (struct mdns_record *)MDNSD_calloc(1, sizeof(struct mdns_record));
	r->rr.name = STRDUP(host);
	r->rr.type = type;
	r->rr.ttl = ttl;
	r->next = d->published[i];
	d->published[i] = r;

	return r;
}

mdns_record_t *mdnsd_unique(mdns_daemon_t *d, const char *host, unsigned short int type, unsigned long int ttl, void (*conflict)(char *host, int type, void *arg), void *arg)
{
	mdns_record_t *r;

	r = mdnsd_shared(d, host, type, ttl);
	r->conflict = conflict;
	r->arg = arg;
	r->unique = 1;
	_r_push(&d->probing, r);
	d->probe.tv_sec = d->now.tv_sec;
	d->probe.tv_usec = d->now.tv_usec;

	return r;
}

mdns_record_t * mdnsd_get_published(const mdns_daemon_t *d, const char *host) {
	return d->published[_namehash(host) % SPRIME];
}

int mdnsd_has_query(const mdns_daemon_t *d, const char *host) {
	return d->queries[_namehash(host) % SPRIME]!=NULL;
}

void mdnsd_done(mdns_daemon_t *d, mdns_record_t *r)
{
	mdns_record_t *cur;

	if (r->unique && r->unique < 5) {
		/* Probing yet, zap from that list first! */
		if (d->probing == r) {
			d->probing = r->list;
		} else {
			for (cur = d->probing; cur->list != r; cur = cur->list)
				;
			cur->list = r->list;
		}

		_r_done(d, r);
		return;
	}

	r->rr.ttl = 0;
	_r_send(d, r);
}

void mdnsd_set_raw(mdns_daemon_t *d, mdns_record_t *r, const char *data, unsigned short int len)
{
	MDNSD_free(r->rr.rdata);
	r->rr.rdata = (unsigned char *)MDNSD_malloc(len);
	memcpy(r->rr.rdata, data, len);
	r->rr.rdlen = len;
	_r_publish(d, r);
}

void mdnsd_set_host(mdns_daemon_t *d, mdns_record_t *r, const char *name)
{
	MDNSD_free(r->rr.rdname);
	r->rr.rdname = STRDUP(name);
	_r_publish(d, r);
}

void mdnsd_set_ip(mdns_daemon_t *d, mdns_record_t *r, struct in_addr ip)
{
	r->rr.ip = ip;
	_r_publish(d, r);
}

void mdnsd_set_srv(mdns_daemon_t *d, mdns_record_t *r, unsigned short int priority, unsigned short int weight, unsigned short int port, char *name)
{
	r->rr.srv.priority = priority;
	r->rr.srv.weight = weight;
	r->rr.srv.port = port;
	mdnsd_set_host(d, r, name);
}

#if MDNSD_LOGLEVEL <= 100
#include <ctype.h>
static void dump_hex_pkg(char* buffer, int bufferLen) {
	char ascii[17];
	memset(ascii,0,17);
	for (int i = 0; i < bufferLen; i++)
	{
		if (i%16 == 0)
			printf("%s\n%06x ", ascii, i);
		if (isprint((int)(buffer[i])))
			ascii[i%16] = buffer[i];
		else
			ascii[i%16] = '.';
		printf("%02X ", (unsigned char)buffer[i]);
	}
	printf("%s\n%06x ", ascii, bufferLen);
	printf("\n");
}
#endif

unsigned short int mdnsd_step(mdns_daemon_t *d, int mdns_socket, bool processIn, bool processOut, struct timeval *nextSleep) {

	struct message m;

	if (processIn) {
		int bsize;
		socklen_t ssize = sizeof(struct sockaddr_in);
		unsigned char buf[MAX_PACKET_LEN];
		struct sockaddr_in from;

		while ((bsize = (int)recvfrom(mdns_socket, (char*)buf, MAX_PACKET_LEN, 0, (struct sockaddr *)&from, &ssize)) > 0) {
			memset(&m, 0, sizeof(struct message));
#if MDNSD_LOGLEVEL <= 100
			MDNSD_LOG_TRACE("Got Data:");
			dump_hex_pkg((char*)buf, bsize);
#endif
#ifdef MDNSD_DEBUG_DUMP_PKGS_FILE
            mdnsd_debug_dumpCompleteChunk(d, (char*)buf, (size_t) bsize);
#endif
			if (!message_parse(&m, buf, (size_t)bsize))
			    continue;
			if (mdnsd_in(d, &m, from.sin_addr.s_addr, from.sin_port)!=0)
				return 2;
		}
#ifdef _WIN32
		if (bsize < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else
		if (bsize < 0 && errno != EAGAIN)
#endif
		{
			return 1;
		}
	}

	if (processOut) {
		struct sockaddr_in to;
		struct in_addr ip;
		unsigned short int port;
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif
		while (mdnsd_out(d, &m, &ip, &port)) {
#ifdef __clang__
#pragma clang diagnostic pop
#endif	
			int len = message_packet_len(&m);
			char* buf = (char*)message_packet(&m);
			memset(&to, 0, sizeof(to));
			to.sin_family = AF_INET;
			to.sin_port = port;
			to.sin_addr = ip;
#if MDNSD_LOGLEVEL <= 100
			MDNSD_LOG_TRACE("Send Data:");
			dump_hex_pkg(buf, (int)len);
#endif

            #ifdef MDNSD_DEBUG_DUMP_PKGS_FILE
            mdnsd_debug_dumpCompleteChunk(d, buf, (size_t) len);
            #endif
			if (sendto(mdns_socket, buf, (unsigned int)len, 0, (struct sockaddr *)&to,
							sizeof(struct sockaddr_in)) != len) {
				return 2;
			}
		}
	}

	if (nextSleep) {
		struct timeval *tv = mdnsd_sleep(d);
		nextSleep->tv_sec = tv->tv_sec;
		nextSleep->tv_usec = tv->tv_usec;
	}

	return 0;
}
