#if defined(__MINGW32__) && (!defined(WINVER) || WINVER < 0x501)
/* Assume the target is newer than Windows XP */
# undef WINVER
# undef _WIN32_WINDOWS
# undef _WIN32_WINNT
# define WINVER _WIN32_WINNT_VISTA
# define _WIN32_WINDOWS _WIN32_WINNT_VISTA
# define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <sys/types.h>

#ifdef _WIN32

# define _WINSOCK_DEPRECATED_NO_WARNINGS /* inet_ntoa is deprecated on MSVC but used for compatibility */
# include <winsock2.h>
# include <ws2tcpip.h>

static int my_inet_pton(int af, const char *src, void *dst)
{
  struct sockaddr_storage ss;
  int size = sizeof(ss);
  char src_copy[INET6_ADDRSTRLEN+1];

  ZeroMemory(&ss, sizeof(ss));
  /* stupid non-const API */
  strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
  src_copy[INET6_ADDRSTRLEN] = 0;

  if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
    switch(af) {
      case AF_INET:
    *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
    return 1;
      case AF_INET6:
    *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
    return 1;
    }
  }
  return 0;
}

#define INET_PTON my_inet_pton
#else
# include <arpa/inet.h>
# include <netinet/in.h>
# include <sys/select.h>
# include <sys/ioctl.h>
#define INET_PTON inet_pton
#endif

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
# define CLOSESOCKET(S) closesocket((SOCKET)S)
# define ssize_t int
#else
# include <unistd.h> // read, write, close
# define CLOSESOCKET(S) close(S)
#endif

#include <libmdnsd/mdnsd.h>
#include <libmdnsd/sdtxt.h>

int _shutdown = 0;
mdns_daemon_t *_d;
int daemon_socket = 0;

void conflict(char *name, int type, void *arg)
{
	printf("conflicting name detected %s for type %d\n", name, type);
	exit(1);
}

void record_received(const struct resource* r, void* data) {
	#ifndef _WIN32
	char ipinput[INET_ADDRSTRLEN];
	#endif
	switch(r->type) {
		case QTYPE_A:
			#ifndef _WIN32
			inet_ntop(AF_INET, &(r->known.a.ip), ipinput, INET_ADDRSTRLEN);
			printf("Got %s: A %s->%s\n", r->name,r->known.a.name, ipinput);
			#else
			printf("Got %s: A %s\n", r->name,r->known.a.name);
			#endif
			break;
		case QTYPE_NS:
			printf("Got %s: NS %s\n", r->name,r->known.ns.name);
			break;
		case QTYPE_CNAME:
			printf("Got %s: CNAME %s\n", r->name,r->known.cname.name);
			break;
		case QTYPE_PTR:
			printf("Got %s: PTR %s\n", r->name,r->known.ptr.name);
			break;
		case QTYPE_TXT:
			printf("Got %s: TXT %s\n", r->name,r->rdata);
			break;
		case QTYPE_SRV:
			printf("Got %s: SRV %d %d %d %s\n", r->name,r->known.srv.priority,r->known.srv.weight,r->known.srv.port,r->known.srv.name);
			break;
		default:
			printf("Got %s: unknown\n", r->name);

	}

}

void done(int sig)
{
	_shutdown = 1;
	mdnsd_shutdown(_d);
	// wake up select
	if (write(daemon_socket, "\0", 1) == -1) {
		printf("Could not write zero byte to socket\n");
	}
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
	unsigned char ttl = 255; // send to any reachable net, not only the subnet

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
	mdns_record_t *r;
	struct in_addr ip;
	unsigned short int port;
	fd_set fds;
	int s;
	char hlocal[256], nlocal[256];
	unsigned char *packet;
	int len = 0;
	xht_t *h;
	char *path = NULL;

	if (argc < 4) {
		printf("usage: mhttp 'unique name' 12.34.56.78 80 '/optionalpath'\n");
		return 1;
	}

	INET_PTON(AF_INET,argv[2], &ip);
	port = atoi(argv[3]);
	if (argc == 5)
		path = argv[4];
	printf("Announcing .local site named '%s' to %s:%d and extra path '%s'\n", argv[1], inet_ntoa(ip), port, argv[4]);

	signal(SIGINT, done);
	#ifdef SIGHUP
	signal(SIGHUP, done);
	#endif
	#ifdef SIGQUIT
	signal(SIGQUIT, done);
	#endif
	signal(SIGTERM, done);
	_d = d = mdnsd_new(QCLASS_IN, 1000);
	if ((s = msock()) == 0) {
		printf("can't create socket: %s\n", strerror(errno));
		return 1;
	}


	mdnsd_register_receive_callback(d, record_received, NULL);


	sprintf(hlocal, "%s._http._tcp.local.", argv[1]);
	sprintf(nlocal, "http-%s.local.", argv[1]);

	// Announce that we have a _http._tcp service
	r = mdnsd_shared(d, "_services._dns-sd._udp.local.", QTYPE_PTR, 120);
	mdnsd_set_host(d, r, "_http._tcp.local.");

	r = mdnsd_shared(d, "_http._tcp.local.", QTYPE_PTR, 120);
	mdnsd_set_host(d, r, hlocal);
	r = mdnsd_unique(d, hlocal, QTYPE_SRV, 600, conflict, 0);
	mdnsd_set_srv(d, r, 0, 0, port, nlocal);
	r = mdnsd_unique(d, nlocal, QTYPE_A, 600, conflict, 0);
	mdnsd_set_raw(d, r, (char *)&ip.s_addr, 4);
	r = mdnsd_unique(d, hlocal, QTYPE_TXT, 600, conflict, 0);
	h = xht_new(11);
	if (path && strlen(path))
		xht_set(h, "path", path);
	packet = sd2txt(h, &len);
	xht_free(h);
	mdnsd_set_raw(d, r, (char *)packet, len);
	MDNSD_free(packet);

	// example for getting a previously published record:
	{
		mdns_record_t *get_r = mdnsd_get_published(d, "_http._tcp.local.");
		while(get_r) {
			const mdns_answer_t *data = mdnsd_record_data(get_r);
			printf("Found record of type %d\n", data->type);
			get_r = mdnsd_record_next(get_r);
		}
	}

	{
		struct timeval next_sleep;
		next_sleep.tv_sec = 0;
		next_sleep.tv_usec = 0;
		while (1) {

			FD_ZERO(&fds);
			FD_SET(s, &fds);
			select(s + 1, &fds, 0, 0, &next_sleep);

			if (_shutdown)
				break;

			{
				unsigned short retVal = mdnsd_step(d, s, FD_ISSET(s, &fds), true, &next_sleep);
				if (retVal == 1) {
					printf("can't read from socket %d: %s\n", errno, strerror(errno));
					break;
				} else if (retVal == 2) {
					printf("can't write to socket: %s\n", strerror(errno));
					break;
				}
			}

			if (_shutdown)
				break;
		}
	}

	mdnsd_shutdown(d);
	mdnsd_free(d);

	return 0;
}
