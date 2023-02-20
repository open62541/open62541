#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <sys/types.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libmdnsd/mdnsd.h>

#ifdef _WIN32
# define CLOSESOCKET(S) closesocket((SOCKET)S)
# define ssize_t int
#else
# include <unistd.h> // read, write, close
# define CLOSESOCKET(S) close(S)
#endif

/* Print an answer */
int ans(mdns_answer_t *a, void *arg)
{
	int now;

	if (a->ttl == 0)
		now = 0;
	else
		now = (int)(a->ttl - (unsigned long)time(0));

	switch (a->type) {
	case QTYPE_A:
		printf("A %s for %d seconds to ip %s\n", a->name, now, inet_ntoa(a->ip));
		break;

	case QTYPE_PTR:
		printf("PTR %s for %d seconds to %s\n", a->name, now, a->rdname);
		break;

	case QTYPE_SRV:
		printf("SRV %s for %d seconds to %s:%hu\n", a->name, now, a->rdname, a->srv.port);
		break;

	default:
		printf("%hu %s for %d seconds with %hu data\n", a->type, a->name, now, a->rdlen);
	}

	return 0;
}


static void socket_set_nonblocking(int sockfd) {
#ifdef _WIN32
	u_long iMode = 1;
    ioctlsocket(sockfd, FIONBIO, &iMode);
#else
	int opts = fcntl(sockfd, F_GETFL);
	fcntl(sockfd, F_SETFL, opts|O_NONBLOCK);
#endif
}


/* Create multicast 224.0.0.251:5353 socket */
int msock(void)
{
	int s, flag = 1, ittl = 255;
	struct sockaddr_in in;
	struct ip_mreq mc;
	char ttl = (char) 255;

	memset(&in, 0, sizeof(in));
	in.sin_family = AF_INET;
	in.sin_port = htons(5353);
	in.sin_addr.s_addr = 0;

	if ((s = (int)socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return 0;

#ifdef SO_REUSEPORT
	setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
	if (bind(s, (struct sockaddr *)&in, sizeof(in))) {
		CLOSESOCKET(s);
		return 0;
	}

	mc.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
	mc.imr_interface.s_addr = htonl(INADDR_ANY);
	setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mc, sizeof(mc));
	setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ttl, sizeof(ttl));
	setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ittl, sizeof(ittl));

	socket_set_nonblocking(s);


	return s;
}

int main(int argc, char *argv[])
{
	mdns_daemon_t *d;
	struct message m;
	struct in_addr ip;
	unsigned short int port;
	ssize_t bsize;
	socklen_t ssize;
	unsigned char buf[MAX_PACKET_LEN];
	struct sockaddr_in from, to;
	fd_set fds;
	int s;

	if (argc != 3) {
		printf("usage: mquery 12 _http._tcp.local.\n");
		return 1;
	}

	d = mdnsd_new(1, 1000);
	if ((s = msock()) == 0) {
		printf("can't create socket: %s\n", strerror(errno));
		return 1;
	}

	mdnsd_query(d, argv[2], atoi(argv[1]), ans, 0);

	while (1) {
		struct timeval *tv = mdnsd_sleep(d);
		FD_ZERO(&fds);
		FD_SET(s, &fds);
		select(s + 1, &fds, 0, 0, tv);

		if (FD_ISSET(s, &fds)) {
			ssize = sizeof(struct sockaddr_in);
			while ((bsize = recvfrom(s, buf, MAX_PACKET_LEN, 0, (struct sockaddr *)&from, &ssize)) > 0) {
				memset(&m, 0, sizeof(struct message));
				if (!message_parse(&m, buf, bsize))
                    continue;
				mdnsd_in(d, &m, (unsigned long int)from.sin_addr.s_addr, from.sin_port);
			}
			if (bsize < 0 && errno != EAGAIN) {
				printf("can't read from socket %d: %s\n", errno, strerror(errno));
				return 1;
			}
		}

		while (mdnsd_out(d, &m, &ip, &port)) {
			memset(&to, 0, sizeof(to));
			to.sin_family = AF_INET;
			to.sin_port = port;
			to.sin_addr.s_addr = ip.s_addr;
			if (sendto(s, message_packet(&m), message_packet_len(&m), 0, (struct sockaddr *)&to,
				   sizeof(struct sockaddr_in)) != message_packet_len(&m)) {
				printf("can't write to socket: %s\n", strerror(errno));
				return 1;
			}
		}
	}

	mdnsd_shutdown(d);
	mdnsd_free(d);

	return 0;
}
