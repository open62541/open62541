/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing_networklayers.h"

#include <pcap.h>
#include <string.h>

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

/* Replays TCP packets from a pcap file using the TestConnectionManager API.
 * After each outgoing send (and on openConnection) a UA_DelayedCallback is
 * registered with the EventLoop. The callback injects the next matching
 * inbound packet and re-schedules itself only if the application sends again
 * from within that inject. This keeps the call-stack flat regardless of
 * session length and lets tests drive the interaction through the normal
 * EventLoop without any extra machinery. */

#define PCAP_MAX_CONNECTIONS 16

typedef struct {
    uintptr_t connId;   /* 0 = proto-slot (SYN seen, pcapOpenConnection not yet called) */
    UA_ConnectionState state;
    u_int serverPort;
    u_int clientPort;
} PCAPConnState;

typedef struct {
    pcap_t *fp;
    UA_Boolean client;      /* true = replay as client (receive server->client pkts) */
    UA_Boolean dcScheduled; /* true if dc is already queued in the EventLoop */
    UA_StatusCode (*originalFree)(UA_EventSource *es);
    UA_ConnectionManager *cm;
    UA_DelayedCallback dc;
    PCAPConnState conns[PCAP_MAX_CONNECTIONS];
} PCAPContext;

static PCAPConnState *
pcapFindConn(PCAPContext *ctx, u_int srcPort, u_int dstPort) {
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        PCAPConnState *cs = &ctx->conns[i];
        if(cs->state == UA_CONNECTIONSTATE_CLOSED ||
           cs->state == UA_CONNECTIONSTATE_CLOSING)
            continue;
        if(ctx->client) {
            /* We are the client: receive packets sent by the server */
            if(srcPort == cs->serverPort && dstPort == cs->clientPort)
                return cs;
        } else {
            /* We are the server: receive packets sent by the client */
            if(srcPort == cs->clientPort && dstPort == cs->serverPort)
                return cs;
        }
    }
    return NULL;
}

static void
pcapProcessPacket(UA_ConnectionManager *cm, PCAPContext *ctx,
                  const u_char *data, size_t dataLen) {
    const struct ether_header *ethHeader = (const struct ether_header *)data;
    if(ntohs(ethHeader->ether_type) != ETHERTYPE_IP)
        return;

    const struct ip *ipHeader =
        (const struct ip *)((const u_char *)data + sizeof(struct ether_header));
    if(ipHeader->ip_p != IPPROTO_TCP)
        return;

    const struct tcphdr *tcpHeader =
        (const struct tcphdr *)((const u_char *)ipHeader + sizeof(struct ip));
    u_int sourcePort = ntohs(tcpHeader->source);
    u_int destPort   = ntohs(tcpHeader->dest);

    /* SYN (no ACK): record the port pair.
     * Two sub-cases depending on whether pcapOpenConnection has run yet:
     *   normal:     a slot exists with state=OPENING, connId set, ports=0 → assign ports
     *   interleaved: no such slot yet → create a proto-slot (connId=0, ports set) */
    if((tcpHeader->th_flags & TH_SYN) && !(tcpHeader->th_flags & TH_ACK)) {
        /* Normal: slot created by pcapOpenConnection waiting for port assignment */
        for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
            PCAPConnState *cs = &ctx->conns[i];
            if(cs->state == UA_CONNECTIONSTATE_OPENING &&
               cs->connId != 0 && cs->clientPort == 0) {
                cs->clientPort = sourcePort;
                cs->serverPort = destPort;
                return;
            }
        }
        /* Interleaved: pcapOpenConnection has not yet been called; save ports in
         * a proto-slot (connId stays 0) so pcapOpenConnection can claim it later. */
        for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
            PCAPConnState *cs = &ctx->conns[i];
            if(cs->state == UA_CONNECTIONSTATE_CLOSED && cs->connId == 0) {
                cs->state      = UA_CONNECTIONSTATE_OPENING;
                cs->clientPort = sourcePort;
                cs->serverPort = destPort;
                return;
            }
        }
        return; /* no free slot — silently drop */
    }

    PCAPConnState *cs = pcapFindConn(ctx, sourcePort, destPort);
    if(!cs)
        return;

    size_t hdrLen = sizeof(struct ether_header) + sizeof(struct ip) +
                    (size_t)(tcpHeader->doff * 4);
    UA_ByteString payload = UA_BYTESTRING_NULL;
    if(dataLen > hdrLen) {
        payload.data   = (UA_Byte *)(uintptr_t)((const u_char *)data + hdrLen);
        payload.length = dataLen - hdrLen;
    }

    /* SYN-ACK: the TCP handshake completes, signal ESTABLISHED.
     * Skip if already ESTABLISHED (pcapOpenConnection already injected it for
     * the interleaved case). */
    if((tcpHeader->th_flags & TH_SYN) && (tcpHeader->th_flags & TH_ACK)) {
        if(cs->state == UA_CONNECTIONSTATE_OPENING && cs->connId != 0) {
            cs->state = UA_CONNECTIONSTATE_ESTABLISHED;
            TestConnectionManager_inject(cm, cs->connId,
                                         UA_CONNECTIONSTATE_ESTABLISHED,
                                         &UA_KEYVALUEMAP_NULL, &UA_BYTESTRING_NULL);
        }
        return;
    }

    if(tcpHeader->th_flags & TH_FIN) {
        /* Deliver any trailing data before signalling close */
        if(payload.length > 0) {
            cs->state = UA_CONNECTIONSTATE_ESTABLISHED;
            TestConnectionManager_inject(cm, cs->connId,
                                         UA_CONNECTIONSTATE_ESTABLISHED,
                                         &UA_KEYVALUEMAP_NULL, &payload);
        }
        cs->state = UA_CONNECTIONSTATE_CLOSING;
        TestConnectionManager_inject(cm, cs->connId,
                                     UA_CONNECTIONSTATE_CLOSING,
                                     &UA_KEYVALUEMAP_NULL, &UA_BYTESTRING_NULL);
        /* TestCM clears the slot on CLOSING; mark ours closed too */
        cs->state = UA_CONNECTIONSTATE_CLOSED;
        return;
    }

    cs->state = UA_CONNECTIONSTATE_ESTABLISHED;
    TestConnectionManager_inject(cm, cs->connId,
                                 UA_CONNECTIONSTATE_ESTABLISHED,
                                 &UA_KEYVALUEMAP_NULL, &payload);
}

static void pcapScheduleNext(PCAPContext *ctx);

/* Delayed callback: inject the next relevant inbound packet, then stop.
 * If the application sends again from within the inject callback,
 * pcapSendWithConnection will re-schedule, preserving flat stack depth. */
static void
pcapNextPacketCB(void *application, void *context) {
    (void)application;
    PCAPContext *ctx = (PCAPContext *)context;
    ctx->dcScheduled = UA_FALSE; /* callback is executing — no longer queued */
    UA_ConnectionManager *cm = ctx->cm;

    const u_char *data;
    struct pcap_pkthdr *pkthdr;
    while(pcap_next_ex(ctx->fp, &pkthdr, &data) == 1 && data) {
        if(pkthdr->len < sizeof(struct ether_header) + sizeof(struct ip))
            continue;
        const struct ether_header *eh = (const struct ether_header *)data;
        if(ntohs(eh->ether_type) != ETHERTYPE_IP)
            continue;
        const struct ip *ih =
            (const struct ip *)((const u_char *)data + sizeof(struct ether_header));
        if(ih->ip_p != IPPROTO_TCP)
            continue;
        pcapProcessPacket(cm, ctx, data, (size_t)pkthdr->len);
        /* Always advance: if the application already scheduled via
         * pcapSendWithConnection, this is a no-op (dcScheduled==true). */
        pcapScheduleNext(ctx);
        return;
    }
    /* EOF — pcap exhausted, nothing more to schedule */
}

static void
pcapScheduleNext(PCAPContext *ctx) {
    if(ctx->dcScheduled)
        return; /* already queued, avoid double-adding the dc struct */
    UA_EventLoop *el = ctx->cm->eventSource.eventLoop;
    if(!el)
        return;
    ctx->dc.callback    = pcapNextPacketCB;
    ctx->dc.application = NULL;
    ctx->dc.context     = ctx;
    ctx->dcScheduled    = UA_TRUE;
    el->addDelayedCallback(el, &ctx->dc);
}

static UA_StatusCode
pcapOpenConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback cb) {
    PCAPContext *ctx = (PCAPContext *)TestConnectionManager_getContext(cm);

    /* Find a free internal port-tracking slot */
    PCAPConnState *cs = NULL;
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        if(ctx->conns[i].state == UA_CONNECTIONSTATE_CLOSED) {
            cs = &ctx->conns[i];
            break;
        }
    }
    if(!cs)
        return UA_STATUSCODE_BADINTERNALERROR;

    uintptr_t connId;
    UA_StatusCode rv =
        TestConnectionManager_createConnection(cm, application, context,
                                               cb, &connId);
    if(rv != UA_STATUSCODE_GOOD)
        return rv;

    memset(cs, 0, sizeof(PCAPConnState));
    cs->connId = connId;
    cs->state  = UA_CONNECTIONSTATE_OPENING;

    /* Look for a proto-slot: a SYN arrived before this call (interleaved pcap),
     * so the TCP handshake packets have already been consumed from the iterator.
     * Claim the proto-slot and jump straight to ESTABLISHED. */
    for(size_t i = 0; i < PCAP_MAX_CONNECTIONS; i++) {
        PCAPConnState *ps = &ctx->conns[i];
        if(ps->state == UA_CONNECTIONSTATE_OPENING &&
           ps->connId == 0 && ps->clientPort != 0) {
            cs->clientPort = ps->clientPort;
            cs->serverPort = ps->serverPort;
            cs->state      = UA_CONNECTIONSTATE_ESTABLISHED;
            memset(ps, 0, sizeof(PCAPConnState)); /* release the proto-slot */
            TestConnectionManager_inject(cm, connId, UA_CONNECTIONSTATE_ESTABLISHED,
                                         &UA_KEYVALUEMAP_NULL, &UA_BYTESTRING_NULL);
            pcapScheduleNext(ctx);
            return UA_STATUSCODE_GOOD;
        }
    }

    /* Normal case: SYN/SYN-ACK not yet seen; inject OPENING and wait. */
    TestConnectionManager_inject(cm, connId, UA_CONNECTIONSTATE_OPENING,
                                 &UA_KEYVALUEMAP_NULL, &UA_BYTESTRING_NULL);

    /* Schedule the first inbound packet via the EventLoop */
    pcapScheduleNext(ctx);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pcapSendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)connectionId;
    (void)params;
    UA_ByteString_clear(buf);
    /* Schedule the next inbound packet for the following EventLoop cycle */
    PCAPContext *ctx = (PCAPContext *)TestConnectionManager_getContext(cm);
    pcapScheduleNext(ctx);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pcapCloseConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    /* For pcap replay the CLOSING callback is delivered when the server's FIN
     * packet arrives in the capture file.  Doing nothing here keeps the TestCM
     * slot intact so the FIN can be matched and the callback fired in the
     * correct order.  inject(CLOSING) auto-clears the slot when that happens. */
    (void)cm;
    (void)connectionId;
    return UA_STATUSCODE_GOOD;
}

static const TestConnectionManager_CallbackOverloads pcapOverloads = {
    pcapOpenConnection,
    pcapSendWithConnection,
    pcapCloseConnection
};

static UA_StatusCode
pcapFree(UA_EventSource *es) {
    UA_ConnectionManager *cm = (UA_ConnectionManager *)es;
    PCAPContext *ctx = (PCAPContext *)TestConnectionManager_getContext(cm);
    /* Save the original free before ctx is released */
    UA_StatusCode (*origFree)(UA_EventSource *) = ctx->originalFree;
    pcap_close(ctx->fp);
    UA_free(ctx);
    TestConnectionManager_setContext(cm, NULL);
    return origFree(es);
}

UA_ConnectionManager *
ConnectionManage_replayPCAP(const char *pcap_file, UA_Boolean client) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *fp = pcap_open_offline(pcap_file, errbuf);
    if(!fp)
        return NULL;

    if(pcap_datalink(fp) != DLT_EN10MB) {
        pcap_close(fp);
        return NULL;
    }

    UA_ConnectionManager *cm = TestConnectionManager_new("tcp", &pcapOverloads);
    UA_assert(cm);

    PCAPContext *ctx = (PCAPContext *)UA_calloc(1, sizeof(PCAPContext));
    UA_assert(ctx);
    ctx->fp     = fp;
    ctx->client = client;
    ctx->cm              = cm;
    ctx->originalFree    = cm->eventSource.free;

    TestConnectionManager_setContext(cm, ctx);
    cm->eventSource.free = pcapFree;

    return cm;
}
