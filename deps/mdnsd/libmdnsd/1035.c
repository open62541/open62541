#include "1035.h"
#include <string.h>
#include <stdio.h>

#if defined(_MSC_VER) && _MSC_VER < 1900

__inline int msnds_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int msnds_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = msnds_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#else

#define msnds_snprintf snprintf

#endif

uint16_t net2short(const unsigned char **bufp)
{
	unsigned short int i;
	memcpy(&i, *bufp, sizeof(short int));
	*bufp += 2;
	return ntohs(i);
}

uint32_t net2long(const unsigned char **bufp)
{
	uint32_t l;

    memcpy(&l, *bufp, sizeof(uint32_t));
	*bufp += 4;

	return ntohl(l);
}

void short2net(uint16_t i, unsigned char **bufp)
{
    uint16_t x = htons(i);
    memcpy(*bufp, &x, sizeof(uint16_t));
	*bufp += 2;
}

void long2net(uint32_t l, unsigned char **bufp)
{
    uint32_t x = htonl(l);
    memcpy(*bufp, &x, sizeof(uint32_t));
	*bufp += 4;
}

static unsigned short int _ldecomp(const unsigned char *ptr)
{
	unsigned short int i;

	i = (unsigned short int)(0xc0 ^ ptr[0]);
	i = (unsigned short int)(i<<8);
	i = (unsigned short int)(i | ptr[1]);

	return i;
}

static bool _label(struct message *m, const unsigned char **bufp, const unsigned char *bufEnd, char **namep)
{
	int x;
	const unsigned char *label;
	char *name;

	/* Set namep to the end of the block */
	*namep = name = (char *)m->_packet + m->_len;

	if (*bufp >= bufEnd)
	    return false;

    // forward buffer pointer until we find the first compressed label
    bool moveBufp = true;
	/* Loop storing label in the block */
	do {
	    if (moveBufp) {
            label = *bufp;
	    }

        if (label >= bufEnd) {
            break;
        }

        /* Since every domain name ends with the null label of
            the root, a domain name is terminated by a length byte of zero. */
        if (*label == 0) {
            if (moveBufp) {
                *bufp += 1;
            }
            break;
        }


		/* Skip past any compression pointers, kick out if end encountered (bad data prolly) */

		/* If a label is compressed, it has following structure of 2 bytes:
		 *    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    	 *    | 1  1|                OFFSET                   |
		 *    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
		 *
		 * The OFFSET field specifies an offset from
		 * the start of the message (i.e., the first octet of the ID field in the
		 * domain header).  A zero offset specifies the first byte of the ID field,
		 * etc.
		 **/
		if (*label & 0xc0) {
		    if (label + 2 > bufEnd)
		        return false;
            unsigned short int offset = _ldecomp(label);
            if (offset > m->_len)
                return false;
            if (m->_buf + offset >= bufEnd)
                return false;
            label = m->_buf + offset;
            // chek if label is again pointer, then abort.
            if (*label & 0xc0)
                return false;
            moveBufp = false;
            *bufp += 2;
		}


		/* Make sure we're not over the limits
		 * See https://tools.ietf.org/html/rfc1035
		 * 2.3.4. Size limits
		 * */
		const unsigned char labelLen = (unsigned char)*label;
		if (labelLen > 63)
		    // maximum label length is 63 octets
		    return false;
		long nameLen = (name + labelLen) - *namep;
		if (nameLen > 255)
            // maximum names length is 255 octets
		    return false;

		if (label + 1 + labelLen > bufEnd) {
            return false;
        }
        if ((unsigned char*)name + labelLen > m->_packet + MAX_PACKET_LEN) {
            return false;
        }
		/* Copy chars for this label */
		memcpy(name, label + 1, labelLen);
		name[labelLen] = '.';

        name += labelLen + 1;

        if (moveBufp) {
            *bufp += labelLen + 1;
        }
        else {
            label += labelLen +1;
        }

	} while (*bufp <= bufEnd);

    if ((unsigned char*)name >= m->_packet + MAX_PACKET_LEN) {
        return false;
    }

	/* Terminate name and check for cache or cache it */
	*name = '\0';
	for (x = 0; x < MAX_NUM_LABELS && m->_labels[x]; x++) {
		if (strcmp(*namep, m->_labels[x]))
			continue;

		*namep = m->_labels[x];
		return true;
	}

	/* No cache, so cache it if room */
	if (x < MAX_NUM_LABELS && m->_labels[x] == 0) {
        m->_labels[x] = *namep;
    }
    m->_len += (unsigned long)((name - *namep) + 1);

	return true;
}

/* Internal label matching */
static int _lmatch(struct message *m, const char *l1, const char *l2)
{
	int len;

	/* Always ensure we get called w/o a pointer */
	if (*l1 & 0xc0)
		return _lmatch(m, (char*)m->_buf + _ldecomp((const unsigned char*)l1), l2);
	if (*l2 & 0xc0)
		return _lmatch(m, l1, (char*)m->_buf + _ldecomp((const unsigned char*) l2));

	/* Same already? */
	if (l1 == l2)
		return 1;

	/* Compare all label characters */
	if (*l1 != *l2)
		return 0;
	for (len = 1; len <= *l1; len++) {
		if (l1[len] != l2[len])
			return 0;
	}

	/* Get new labels */
	l1 += *l1 + 1;
	l2 += *l2 + 1;

	/* At the end, all matched */
	if (*l1 == 0 && *l2 == 0)
		return 1;

	/* Try next labels */
	return _lmatch(m, l1, l2);
}

/* Nasty, convert host into label using compression */
static int _host(struct message *m, unsigned char **bufp, char *name)
{
	char label[256], *l;
	int len = 0, x = 1, y = 0, last = 0;

	if (name == 0)
		return 0;

	/* Make our label */
	while (name[y]) {
		if (name[y] == '.') {
			if (!name[y + 1])
				break;
			label[last] = (char)(x - (last + 1));
			last = x;
		} else {
			label[x] = name[y];
		}

		if (x++ == 255)
			return 0;

		y++;
	}

	label[last] = (char)(x - (last + 1));
	if (x == 1)
		x--;		/* Special case, bad names, but handle correctly */
	len = x + 1;
	label[x] = 0;		/* Always terminate w/ a 0 */

	/* Double-loop checking each label against all m->_labels for match */
	for (x = 0; label[x]; x += label[x] + 1) {
		for (y = 0; y < MAX_NUM_LABELS && m->_labels[y]; y++) {
			if (_lmatch(m, label + x, m->_labels[y])) {
				/* Matching label, set up pointer */
				l = label + x;
				short2net((unsigned short)((unsigned char *)m->_labels[y] - m->_packet), (unsigned char **)&l);
				label[x] = (char)(label[x] | 0xc0);
				len = x + 2;
				break;
			}
		}
	
		if (label[x] & 0xc0)
			break;
	}

	/* Copy into buffer, point there now */
	memcpy(*bufp, label, (size_t)len);
	l = (char *)*bufp;
	*bufp += len;

	/* For each new label, store it's location for future compression */
	for (x = 0; l[x] && m->_label < MAX_NUM_LABELS; x += l[x] + 1) {
		if (l[x] & 0xc0)
			break;

		m->_labels[m->_label++] = l + x;
	}

	return len;
}

static bool _rrparse(struct message *m, struct resource *rr, int count, const unsigned char **bufp, const unsigned char* bufferEnd)
{
	int i;
    const unsigned char *addr_bytes = NULL;

    if (count == 0) {
        return true;
    }

    if (*bufp >= m->_bufEnd) {
        return false;
    }

	for (i = 0; i < count; i++) {
	    if (*bufp >= bufferEnd) {
            return false;
        }

		if (!_label(m, bufp, bufferEnd, &(rr[i].name))) {
            return false;
        }
		if (*bufp + 10 > bufferEnd) {
            return false;
        }
		rr[i].type     = net2short(bufp);
		rr[i].clazz    = net2short(bufp);
		rr[i].ttl      = net2long(bufp);
		rr[i].rdlength = net2short(bufp);
//		fprintf(stderr, "Record type %d class 0x%2x ttl %lu len %d\n", rr[i].type, rr[i].clazz, rr[i].ttl, rr[i].rdlength);

		/* For the following records the rdata will be parsed later. So don't set it here:
		 * NS, CNAME, PTR, DNAME, SOA, MX, AFSDB, RT, KX, RP, PX, SRV, NSEC
		 * See 18.14 of https://tools.ietf.org/html/rfc6762#page-47 */
		if (rr[i].type == QTYPE_NS || rr[i].type == QTYPE_CNAME || rr[i].type == QTYPE_PTR || rr[i].type == QTYPE_SRV) {
			rr[i].rdlength = 0;
		} else {
            /* If not going to overflow, make copy of source rdata */
            if (*bufp + rr[i].rdlength > bufferEnd) {
                return false;
            }

            if (m->_len + rr[i].rdlength > MAX_PACKET_LEN) {
                return false;
            }
			rr[i].rdata = m->_packet + m->_len;
			m->_len += rr[i].rdlength;
			memcpy(rr[i].rdata, *bufp, rr[i].rdlength);
		}


		/* Parse commonly known ones */
		switch (rr[i].type) {
		case QTYPE_A:
			if (m->_len + 16 > MAX_PACKET_LEN) {
                return false;
            }
			rr[i].known.a.name = (char *)m->_packet + m->_len;
			m->_len += 16;
            if (*bufp + 4 > bufferEnd) {
                return false;
            }
			msnds_snprintf(rr[i].known.a.name,16, "%d.%d.%d.%d", (*bufp)[0], (*bufp)[1], (*bufp)[2], (*bufp)[3]);
			addr_bytes = (const unsigned char *) *bufp;
			rr[i].known.a.ip.s_addr = (in_addr_t) (
					((in_addr_t) addr_bytes[0]) |
					(((in_addr_t) addr_bytes[1]) << 8) |
					(((in_addr_t) addr_bytes[2]) << 16) |
					(((in_addr_t) addr_bytes[3]) << 24));
            *bufp += 4;
			break;

		case QTYPE_NS:
			if (!_label(m, bufp, bufferEnd, &(rr[i].known.ns.name))) {
                return false;
            }
			break;

		case QTYPE_CNAME:
			if (!_label(m, bufp, bufferEnd, &(rr[i].known.cname.name))) {
                return false;
            }
			break;

		case QTYPE_PTR:
			if (!_label(m, bufp, bufferEnd, &(rr[i].known.ptr.name))) {
                return false;
            }
			break;

		case QTYPE_SRV:
            if (*bufp + 6 > bufferEnd) {
                return false;
            }
			rr[i].known.srv.priority = net2short(bufp);
			rr[i].known.srv.weight = net2short(bufp);
			rr[i].known.srv.port = net2short(bufp);
			if (!_label(m, bufp, bufferEnd, &(rr[i].known.srv.name))) {
                return false;
            }
			break;

		case QTYPE_TXT:
		default:
			*bufp += rr[i].rdlength;
		}
	}

	return true;
}

/* Keep all our mem in one (aligned) block for easy freeing */
#define my(x,y, cast)				\
	while (m->_len & 7)			\
		m->_len++;			\
		                        \
    if (m->_len + y > MAX_PACKET_LEN) { return false; } \
	x = (cast)(void *)(m->_packet + m->_len);	\
	m->_len += y;

bool message_parse(struct message *m, unsigned char *packet, size_t packetLen)
{
	int i;
	const unsigned char *buf;
	m->_bufEnd = packet + packetLen;

	/* Message format: https://tools.ietf.org/html/rfc1035

	+---------------------+
    |        Header       |
    +---------------------+
    |       Question      | the question for the name server
    +---------------------+
    |        Answer       | RRs answering the question
    +---------------------+
    |      Authority      | RRs pointing toward an authority
    +---------------------+
    |      Additional     | RRs holding additional information
    +---------------------+
    */

	if (packet == 0 || m == 0)
		return false;

	/* See https://tools.ietf.org/html/rfc1035
	                                1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	 */

	/* The header always needs to be present. Size = 12 byte */
	if (packetLen < 12)
	    return false;

	/* Header stuff bit crap */
    buf = m->_buf = packet;
	m->id = net2short(&buf);
	if (buf[0] & 0x80)
		m->header.qr = 1;
	m->header.opcode = (unsigned short)(((buf[0] & 0x78) >> 3) & 15);
	if (buf[0] & 0x04)
		m->header.aa = 1;
	if (buf[0] & 0x02)
		m->header.tc = 1;
	if (buf[0] & 0x01)
		m->header.rd = 1;
	if (buf[1] & 0x80)
		m->header.ra = 1;
	m->header.z = (unsigned short)(((buf[1] & 0x70) >> 4) & 7);
	m->header.rcode = (unsigned short)(buf[1] & 0x0F);
	buf += 2;

	m->qdcount = net2short(&buf);
	m->ancount = net2short(&buf);
    m->nscount = net2short(&buf);
    m->arcount = net2short(&buf);

    // check if the message has the correct size, i.e. the count matches the number of bytes

	/* Process questions */
	my(m->qd, (sizeof(struct question) * m->qdcount), struct question *);
	for (i = 0; i < m->qdcount; i++) {
		if (!_label(m, &buf, m->_bufEnd, &(m->qd[i].name))) {
            return false;
        }
		if (buf + 4 > m->_bufEnd) {
            return false;
        }
		m->qd[i].type  = net2short(&buf);
		m->qd[i].clazz = net2short(&buf);
	}
    if (buf > m->_bufEnd) {
        return false;
    }

	/* Process rrs */
	my(m->an, (sizeof(struct resource) * m->ancount), struct resource *);
	my(m->ns, (sizeof(struct resource) * m->nscount), struct resource *);
	my(m->ar, (sizeof(struct resource) * m->arcount), struct resource *);
	if (!_rrparse(m, m->an, m->ancount, &buf, m->_bufEnd))
		return false;
	if (!_rrparse(m, m->ns, m->nscount, &buf, m->_bufEnd))
		return false;
	if (!_rrparse(m, m->ar, m->arcount, &buf, m->_bufEnd))
		return false;
	return true;
}

void message_qd(struct message *m, char *name, unsigned short int type, unsigned short int clazz)
{
	m->qdcount++;
    if (m->_buf == 0) {
        m->_buf = m->_packet + 12;
        m->_bufEnd = m->_packet + sizeof(m->_packet);
    }
	_host(m, &(m->_buf), name);
	short2net(type, &(m->_buf));
	short2net(clazz, &(m->_buf));
}

static void _rrappend(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl)
{
	if (m->_buf == 0) {
        m->_buf = m->_packet + 12;
        m->_bufEnd = m->_packet + sizeof(m->_packet);
    }
	_host(m, &(m->_buf), name);
	short2net(type, &(m->_buf));
	short2net(clazz, &(m->_buf));
	long2net((uint32_t)ttl, &(m->_buf));
}

void message_an(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl)
{
	m->ancount++;
	_rrappend(m, name, type, clazz, ttl);
}

void message_ns(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl)
{
	m->nscount++;
	_rrappend(m, name, type, clazz, ttl);
}

void message_ar(struct message *m, char *name, unsigned short int type, unsigned short int clazz, unsigned long ttl)
{
	m->arcount++;
	_rrappend(m, name, type, clazz, ttl);
}

void message_rdata_long(struct message *m, struct in_addr l)
{
	short2net(4, &(m->_buf));
	long2net(l.s_addr, &(m->_buf));
}

void message_rdata_name(struct message *m, char *name)
{
	unsigned char *mybuf = m->_buf;

	m->_buf += 2;
	short2net((unsigned short)_host(m, &(m->_buf), name), &mybuf);
}

void message_rdata_srv(struct message *m, unsigned short int priority, unsigned short int weight, unsigned short int port, char *name)
{
	unsigned char *mybuf = m->_buf;

	m->_buf += 2;
	short2net(priority, &(m->_buf));
	short2net(weight, &(m->_buf));
	short2net(port, &(m->_buf));
	short2net((unsigned short)(_host(m, &(m->_buf), name) + 6), &mybuf);
}

void message_rdata_raw(struct message *m, unsigned char *rdata, unsigned short int rdlength)
{
	if (((unsigned char *)m->_buf - m->_packet) + rdlength > 4096)
		rdlength = 0;
	short2net(rdlength, &(m->_buf));
	memcpy(m->_buf, rdata, rdlength);
	m->_buf += rdlength;
}

unsigned char *message_packet(struct message *m)
{
	unsigned char c, *buf = m->_buf, *bufEnd = m->_bufEnd;

	m->_buf = m->_packet;
    m->_bufEnd = m->_packet + sizeof(m->_packet);
	short2net(m->id, &(m->_buf));

	if (m->header.qr)
		m->_buf[0] |= 0x80;
	if ((c = (unsigned char)m->header.opcode))
		m->_buf[0] |= (unsigned char)(c << 3);
	if (m->header.aa)
		m->_buf[0] |= 0x04;
	if (m->header.tc)
		m->_buf[0] |= 0x02;
	if (m->header.rd)
		m->_buf[0] |= 0x01;
	if (m->header.ra)
		m->_buf[1] |= 0x80;
	if ((c = (unsigned char)m->header.z))
		m->_buf[1] |= (unsigned char)(c << 4);
	if (m->header.rcode)
		m->_buf[1] = (unsigned char)(m->_buf[1] | m->header.rcode);

	m->_buf += 2;
	short2net(m->qdcount, &(m->_buf));
	short2net(m->ancount, &(m->_buf));
	short2net(m->nscount, &(m->_buf));
	short2net(m->arcount, &(m->_buf));
	m->_buf = buf;		/* Restore, so packet_len works */
	m->_bufEnd = bufEnd;

	return m->_packet;
}

int message_packet_len(struct message *m)
{
	if (m->_buf == 0)
		return 12;

	return (int)((unsigned char *)m->_buf - m->_packet);
}
