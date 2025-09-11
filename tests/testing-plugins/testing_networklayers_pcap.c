/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_networklayers.h"
#include "../../arch/posix/eventloop_posix.h"

#include <pcap.h>

#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

/* This assumes the POSIX EventLoop. It replays TCP packets from a pcap file. It
 * registers a unix socket for self-triggering in the EventLoop. The TCP
 * connections are handled internally and not registered in the EventLoop. */

#define PCAP_MAX_CONNECTIONS 16

typedef struct {
    unsigned id;
    UA_ConnectionState state;

    /* For now it suffices to compare the ports...
    char serverIP[INET_ADDRSTRLEN];
    char clientIP[INET_ADDRSTRLEN]; */
    u_int serverPort;
    u_int clientPort;

    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback connectionCallback;
} PCAPConnection;

typedef struct {
    UA_ConnectionManager cm;
    pcap_t *fp;
    UA_RegisteredFD rfd; /* Self-pipe to trigger activity on connections */
    int pipe_fd;
    UA_Boolean client; /* Are we client or server? */
    PCAPConnection connections[PCAP_MAX_CONNECTIONS];
    unsigned fdCount;
} PCAPConnectionManager;

static void
flushPipe(int fd) {
    char buf[128];
    ssize_t i;
    do {
        i = read(fd, buf, 128);
    } while(i > 0);
}

static void
pcapCloseConnection(PCAPConnectionManager *pcm, PCAPConnection *c) {
    c->connectionCallback(&pcm->cm, (uintptr_t)c->clientPort, c->application,
                            &c->context, UA_CONNECTIONSTATE_CLOSING,
                            NULL, UA_BYTESTRING_NULL);
    c->state = UA_CONNECTIONSTATE_CLOSED;
}

static void
processPacket(PCAPConnectionManager *pcm, const u_char *data, size_t dataLen) {
    const struct ether_header *ethHeader = (const struct ether_header*)data;
    if(ntohs(ethHeader->ether_type) != ETHERTYPE_IP)
        return;

    const struct ip *ipHeader = (const struct ip*)(data + sizeof(struct ether_header));
    if(ipHeader->ip_p != IPPROTO_TCP)
        return;

    const struct tcphdr *tcpHeader =
        (const struct tcphdr*)((const u_char*)ipHeader + sizeof(struct ip));
    u_int sourcePort = ntohs(tcpHeader->source);
    u_int destPort = ntohs(tcpHeader->dest);

    /* A connection is fully opening. Store IP+port and notify the application.
     * ATTENTION! This assumes that only one connection is _OPENING at a time. */
    if(tcpHeader->th_flags & TH_SYN &&
       !(tcpHeader->th_flags & TH_ACK)) {
        for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
            PCAPConnection *c = &pcm->connections[i];
            if(c->state == UA_CONNECTIONSTATE_OPENING) {
                c->clientPort = sourcePort;
                c->serverPort = destPort;
                break;
            }
        }
    }

    /* Find the connection which matches the ports.
     * This relies on the client-side ports being randomized (unique). */
    PCAPConnection *c = NULL;
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        PCAPConnection *tmp = &pcm->connections[i];
        if(tmp->state == UA_CONNECTIONSTATE_CLOSED ||
           tmp->state == UA_CONNECTIONSTATE_CLOSING)
            continue;
        if(pcm->client) {
            if(sourcePort != tmp->serverPort || destPort != tmp->clientPort)
                continue;
        } else {
            if(sourcePort != tmp->clientPort || destPort != tmp->serverPort)
                continue;
        }
        c = tmp;
        break;
    }

    if(!c)
        return;

    c->state = UA_CONNECTIONSTATE_ESTABLISHED;

    /* Use the packet */
    UA_ByteString payload;
    payload.data = ((u_char*)(uintptr_t)tcpHeader) + (tcpHeader->doff * 4);
    payload.length =
        dataLen - (sizeof(struct ether_header) + sizeof(struct ip) + (tcpHeader->doff * 4));

    /* Process the packet */
    c->connectionCallback(&pcm->cm, (uintptr_t)c->id, c->application, &c->context,
                          UA_CONNECTIONSTATE_ESTABLISHED, NULL, payload);

    /* Close the connection when FIN is received */
    if(tcpHeader->th_flags & TH_FIN)
        c->state = UA_CONNECTIONSTATE_CLOSING;
}

static void
pcapActivityCallback(UA_EventSource *es, UA_RegisteredFD *rfd, short event) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)es;

    /* Re-arm the self-pipe by reading all waiting data */
    flushPipe(rfd->fd);

    /* Close connections in the _CLOSING state */
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        if(pcm->connections[i].state == UA_CONNECTIONSTATE_CLOSING)
            pcapCloseConnection(pcm, &pcm->connections[i]);
    }

    /* Get the next packet */
    const u_char *data;
    struct pcap_pkthdr *pkthdr;
    int res = pcap_next_ex(pcm->fp, &pkthdr, &data);
    if(res != 1 || data == NULL)
        return;

    /* Process the packet */
    processPacket(pcm, data, pkthdr->len);

    /* Retrigger the EventLoop to process the next packet */
    write(pcm->pipe_fd, ".", 1);
}

/* Only allow a single TCP connection */
static UA_StatusCode
pcapOpenConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)cm;

    /* Find a unused connection slot */
    PCAPConnection *c = NULL;
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        if(pcm->connections[i].state == UA_CONNECTIONSTATE_CLOSED) {
            c = &pcm->connections[i];
            break;
        }
    }

    if(!c)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Store the context for the connection */
    memset(c, 0, sizeof(PCAPConnection));
    c->id = ++pcm->fdCount;
    c->application = application;
    c->context = context;
    c->connectionCallback = connectionCallback;
    c->state = UA_CONNECTIONSTATE_OPENING;

    /* Signal that the connection is opening */
    connectionCallback(cm, (uintptr_t)c->id, application, &c->context,
                       UA_CONNECTIONSTATE_OPENING, NULL, UA_BYTESTRING_NULL);

    /* Trigger the EventLoop to process the next packet */
    write(pcm->pipe_fd, ".", 1);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pcapSendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params,
                       UA_ByteString *buf) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)cm;

    UA_ByteString_clear(buf);

    /* Find the connection  */
    PCAPConnection *c = NULL;
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        if(pcm->connections[i].id == (unsigned)connectionId) {
            c = &pcm->connections[i];
            break;
        }
    }

    if(!c)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* Retrigger the EventLoop to process the next packet */
    write(pcm->pipe_fd, ".", 1);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pcapShutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)cm;
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        if(pcm->connections[i].id == (unsigned)connectionId)
            pcm->connections[i].state = UA_CONNECTIONSTATE_CLOSING;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pcapAllocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                        UA_ByteString *buf, size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
pcapFreeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

static UA_StatusCode
pcapStart(UA_EventSource *es) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)es;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;

    int sd[2];
    UA_EventLoopPOSIX_pipe(sd);
    pcm->rfd.fd = sd[0];
    pcm->pipe_fd= sd[1];

    pcm->rfd.es = &pcm->cm.eventSource;
    pcm->rfd.eventSourceCB = pcapActivityCallback;
    pcm->rfd.listenEvents = UA_FDEVENT_IN;
    UA_EventLoopPOSIX_registerFD(el, &pcm->rfd);

    es->state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
pcapStop(UA_EventSource *es) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)es;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;

    /* Close all connections that remain open */
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        if(pcm->connections[i].state != UA_CONNECTIONSTATE_CLOSED)
            pcapCloseConnection(pcm, &pcm->connections[i]);
    }

    UA_EventLoopPOSIX_deregisterFD(el, &pcm->rfd);
    close(pcm->rfd.fd);
    close(pcm->pipe_fd);
    es->state = UA_EVENTSOURCESTATE_STOPPING;
    pcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static UA_StatusCode
pcapFree(UA_EventSource *es) {
    PCAPConnectionManager *pcm = (PCAPConnectionManager*)es;
    pcap_close(pcm->fp);
    UA_free(es);
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
ConnectionManage_replayPCAP(const char *pcap_file, UA_Boolean client) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *fp = pcap_open_offline(pcap_file, errbuf);
    if(!fp)
        return NULL;

    int lt = pcap_datalink(fp);
    if(lt != DLT_EN10MB) {
        pcap_close(fp);
        return NULL;
    }

    PCAPConnectionManager *pcm = (PCAPConnectionManager*)
        UA_calloc(1, sizeof(PCAPConnectionManager));
    pcm->cm.protocol = UA_STRING("tcp");
    pcm->cm.openConnection = pcapOpenConnection;
    pcm->cm.sendWithConnection = pcapSendWithConnection;
    pcm->cm.closeConnection = pcapShutdownConnection;
    pcm->cm.allocNetworkBuffer = pcapAllocNetworkBuffer;
    pcm->cm.freeNetworkBuffer = pcapFreeNetworkBuffer;

    pcm->cm.eventSource.start = pcapStart;
    pcm->cm.eventSource.stop = pcapStop;
    pcm->cm.eventSource.free = pcapFree;

    pcm->fp = fp;
    pcm->client = client;

    return &pcm->cm;
}
