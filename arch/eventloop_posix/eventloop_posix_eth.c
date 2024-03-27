/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright 2018 (c) Kontron Europe GmbH (Author: Rudolf Hoyler)
 *   Copyright 2019-2020 (c) Kalycito Infotech Private Limited
 *   Copyright 2019-2020 (c) Wind River Systems, Inc.
 *   Copyright 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_posix.h"

#if defined(UA_ARCHITECTURE_POSIX) && defined(__linux__)

#include <arpa/inet.h> /* htons */
#include <net/ethernet.h> /* ETH_P_*/
#include <linux/if_packet.h>
#include <linux/net_tstamp.h> /* txtime */

/* Configuration parameters */

#define ETH_MANAGERPARAMS 2

static UA_KeyValueRestriction ethManagerParams[ETH_MANAGERPARAMS] = {
    {{0, UA_STRING_STATIC("recv-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false}
};

#define ETH_PARAMETERSSIZE 15
#define ETH_PARAMINDEX_ADDR 0
#define ETH_PARAMINDEX_LISTEN 1
#define ETH_PARAMINDEX_IFACE 2
#define ETH_PARAMINDEX_ETHERTYPE 3
#define ETH_PARAMINDEX_VID 4
#define ETH_PARAMINDEX_PCP 5
#define ETH_PARAMINDEX_DEI 6
#define ETH_PARAMINDEX_PROMISCUOUS 7
#define ETH_PARAMINDEX_PRIORITY 8
#define ETH_PARAMINDEX_TXTIME_ENABLE 9
#define ETH_PARAMINDEX_TXTIME_FLAGS 10
#define ETH_PARAMINDEX_TXTIME 11
#define ETH_PARAMINDEX_TXTIME_PICO 12
#define ETH_PARAMINDEX_TXTIME_DROP 13
#define ETH_PARAMINDEX_VALIDATE 14

static UA_KeyValueRestriction ethConnectionParams[ETH_PARAMETERSSIZE+1] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("interface")}, &UA_TYPES[UA_TYPES_STRING], true, true, false},
    {{0, UA_STRING_STATIC("ethertype")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("vid")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("pcp")}, &UA_TYPES[UA_TYPES_BYTE], false, true, false},
    {{0, UA_STRING_STATIC("dei")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("promiscuous")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("priority")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("txtime-enable")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("txtime-flags")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("txtime")}, &UA_TYPES[UA_TYPES_DATETIME], false, true, false},
    {{0, UA_STRING_STATIC("txtime-pico")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("txtime-drop-late")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    /* Duplicated address parameter with a scalar value required. For the send-socket case. */
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], true, true, false},
};

#define UA_ETH_MAXHEADERLENGTH (2*ETHER_ADDR_LEN)+4+2+2

typedef struct {
    UA_RegisteredFD rfd;

    UA_ConnectionManager_connectionCallback applicationCB;
    void *application;
    void *context;

    struct sockaddr_ll sll;
    /* The Ethernet header to prepend for sending frames is precomputed and reused.
     * The length field (the last 2 byte) is adjusted.
     * - 2 * ETHER_ADDR_LEN: destination and source
     * - 4 byte: VLAN tagging (optional)
     * - 2 byte: EtherType (optional)
     * - 2 byte: length */
    unsigned char header[UA_ETH_MAXHEADERLENGTH];
    unsigned char headerSize;
    unsigned char lengthOffset; /* No length field if zero */

    UA_Boolean txtimeEnabled;
} ETH_FD;

/* The format of a Ethernet address is six groups of hexadecimal digits,
 * separated by hyphens (e.g. 01-23-45-67-89-ab). */
static UA_StatusCode
parseEthAddress(const UA_String *buf, UA_Byte *addr) {
    size_t curr = 0, idx = 0;
    for(; idx < ETHER_ADDR_LEN; idx++) {
        UA_UInt32 value;
        size_t progress = UA_readNumberWithBase(&buf->data[curr],
                                                buf->length - curr, &value, 16);
        if(progress == 0 || value > (long)0xff)
            return UA_STATUSCODE_BADINTERNALERROR;

        addr[idx] = (UA_Byte) value;

        curr += progress;
        if(curr == buf->length)
            break;

        if(buf->data[curr] != '-')
            return UA_STATUSCODE_BADINTERNALERROR;

        curr++; /* skip '-' */
    }

    if(idx != (ETH_ALEN-1))
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
isMulticastEthAddress(const UA_Byte *address) {
    if((address[0] & 1) == 0)
        return false; /* Unicast address */
    for(size_t i = 0; i < ETHER_ADDR_LEN; i++) {
        if(address[i] != 0xff)
            return true; /* Not broadcast address ff-ff-ff-ff-ff-ff */
    }
    return false;
}

static void
setAddrString(unsigned char addrStr[18], unsigned char addr[ETHER_ADDR_LEN]) {
    mp_snprintf((char*)addrStr, 18, "%02x-%02x-%02x-%02x-%02x-%02x",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

/* Return zero if parsing failed */
static size_t
parseETHHeader(const UA_ByteString *buf,
               unsigned char destAddr[ETHER_ADDR_LEN],
               unsigned char sourceAddr[ETHER_ADDR_LEN],
               UA_UInt16 *etherType, UA_UInt16 *vid,
               UA_Byte *pcp, UA_Boolean *dei) {
    if(buf->length < (2 * ETHER_ADDR_LEN)+2)
        return 0;

    /* Parse "normal" Ethernet header */
    memcpy(destAddr, buf->data, ETHER_ADDR_LEN);
    memcpy(sourceAddr, &buf->data[ETHER_ADDR_LEN], ETHER_ADDR_LEN);
    size_t pos = 2 * ETHER_ADDR_LEN;
    UA_UInt16 length = ntohs(*(UA_UInt16*)&buf->data[pos]);
    pos += 2;

    /* No EtherType and no VLAN */
    if(length <= 1500)
        return pos;

    /* Parse 802.1Q VLAN header */
    if(length == 0x8100) {
        if(buf->length < (2 * ETHER_ADDR_LEN)+2+4)
            return 0;
        pos += 2;
        UA_UInt16 vlan = ntohs(*(UA_UInt16*)&buf->data[pos]);
        *pcp = 0x07 & vlan;
        *dei = 0x01 & (vlan >> 3);
        *vid = vlan >> 4;
        pos += 2;
        length = ntohs(*(UA_UInt16*)&buf->data[pos]);
    }

    /* Set the EtherType if it is set */
    if(length > 1500)
        *etherType = length;

    return pos;
}

static unsigned char
setETHHeader(unsigned char *buf,
             unsigned char destAddr[ETHER_ADDR_LEN],
             unsigned char sourceAddr[ETHER_ADDR_LEN],
             UA_UInt16 etherType, UA_UInt16 vid,
             UA_Byte pcp, UA_Boolean dei, unsigned char *lengthOffset) {
    /* Set dest and source address */
    size_t pos = 0;
    memcpy(buf, destAddr, ETHER_ADDR_LEN);
    pos += ETHER_ADDR_LEN;
    memcpy(&buf[pos], destAddr, ETHER_ADDR_LEN);
    pos += ETHER_ADDR_LEN;

    /* Set the 802.1Q VLAN header */
    if(vid > 0 && vid != ETH_P_ALL) {
        *(UA_UInt16*)&buf[pos] = htons(0x8100);
        pos += 2;
        UA_UInt16 vlan = (UA_UInt16)((UA_UInt16)pcp + (((UA_UInt16)dei) << 3) + (vid << 4));
        *(UA_UInt16*)&buf[pos] = htons(vlan);
        pos += 2;
    }

    /* Set the Ethertype or store the offset for the length field */
    if(etherType == 0 || etherType == ETH_P_ALL) {
        *lengthOffset = (unsigned char)pos;
    } else {
        *(UA_UInt16*)&buf[pos] = htons(etherType);
    }
    pos += 2;
    return (unsigned char)pos;
}

static UA_StatusCode
ETH_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                       UA_ByteString *buf, size_t bufSize) {
    /* Get the ETH_FD */
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_FD fd = (UA_FD)connectionId;
    ETH_FD *erfd = (ETH_FD*)ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!erfd)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    /* Allocate the buffer with the hidden Ethernet header in front */
    UA_StatusCode res =
        UA_EventLoopPOSIX_allocNetworkBuffer(cm, connectionId, buf,
                                             bufSize + erfd->headerSize);
    if(UA_LIKELY(res == UA_STATUSCODE_GOOD)) {
        buf->data   += erfd->headerSize;
        buf->length -= erfd->headerSize;
    }
    return res;
}

static void
ETH_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    /* Get the ETH_FD */
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_FD fd = (UA_FD)connectionId;
    ETH_FD *erfd = (ETH_FD*)ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!erfd)
        return;

    /* Unhide the Ethernet header and free */
    buf->data   -= erfd->headerSize;
    buf->length += erfd->headerSize;
    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
}

/* Test if the ConnectionManager can be stopped */
static void
ETH_checkStopped(UA_POSIXConnectionManager *pcm) {
    UA_LOCK_ASSERT(&((UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop)->elMutex, 1);

    if(pcm->fdsSize == 0 &&
       pcm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(pcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "ETH\t| All sockets closed, the EventLoop has stopped");
        pcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

/* This method must not be called from the application directly, but from within
 * the EventLoop. Otherwise we cannot be sure whether the file descriptor is
 * still used after calling close. */
static void
ETH_close(UA_POSIXConnectionManager *pcm, ETH_FD *conn) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Closing connection",
                 (unsigned)conn->rfd.fd);

    /* Deregister from the EventLoop */
    UA_EventLoopPOSIX_deregisterFD(el, &conn->rfd);

    /* Deregister internally */
    ZIP_REMOVE(UA_FDTree, &pcm->fds, &conn->rfd);
    UA_assert(pcm->fdsSize > 0);
    pcm->fdsSize--;

    /* Signal closing to the application */
    UA_UNLOCK(&el->elMutex);
    conn->applicationCB(&pcm->cm, (uintptr_t)conn->rfd.fd,
                        conn->application, &conn->context,
                        UA_CONNECTIONSTATE_CLOSING,
                        &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    UA_LOCK(&el->elMutex);

    /* Close the socket */
    int ret = UA_close(conn->rfd.fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "ETH %u\t| Socket closed", (unsigned)conn->rfd.fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "ETH %u\t| Could not close the socket (%s)",
                          (unsigned)conn->rfd.fd, errno_str));
    }

    /* Don't call free here. This might be done automatically via the delayed
     * callback that calls ETH_close. */
    /* UA_free(rfd); */

    /* Stop if the ucm is stopping and this was the last open socket */
    ETH_checkStopped(pcm);
}

static void
ETH_delayedClose(void *application, void *context) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    ETH_FD *conn = (ETH_FD*)context;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "ETH %u\t| Delayed closing of the connection",
                 (unsigned)conn->rfd.fd);
    UA_LOCK(&el->elMutex);
    ETH_close(pcm, conn);
    UA_UNLOCK(&el->elMutex);
    UA_free(conn);
}

/* Gets called when a socket receives data or closes */
static void
ETH_connectionSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                             short event) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    ETH_FD *conn = (ETH_FD*)rfd;
    if(event == UA_FDEVENT_ERR) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "ETH %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)rfd->fd, errno_str));
        ETH_close(pcm, conn);
        UA_free(rfd);
        return;
    }

    /* Use the already allocated receive-buffer */
    UA_ByteString response = pcm->rxBuffer;;

    /* Receive */
#ifndef _WIN32
    ssize_t ret = UA_recv(rfd->fd, (char*)response.data,
                          response.length, MSG_DONTWAIT);
#else
    int ret = UA_recv(rfd->fd, (char*)response.data,
                      response.length, MSG_DONTWAIT);
#endif

    /* Receive has failed */
    if(ret <= 0) {
        if(UA_ERRNO == UA_INTERRUPTED)
            return;

        /* Orderly shutdown of the socket. We can immediately close as no method
         * "below" in the call stack will use the socket in this iteration of
         * the EventLoop. */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "ETH %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)rfd->fd, errno_str));
        ETH_close(pcm, conn);
        UA_free(rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Received message of size %u",
                 (unsigned)rfd->fd, (unsigned)ret);

    response.length = (size_t)ret;

    /* Parse the Ethernet header */
    unsigned char destAddr[ETHER_ADDR_LEN];
    unsigned char sourceAddr[ETHER_ADDR_LEN];
    UA_UInt16 etherType = 0;
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_Boolean dei = 0;
    size_t headerSize = parseETHHeader(&response, destAddr, sourceAddr,
                                       &etherType, &vid, &pcp, &dei);
    if(headerSize == 0)
        return;

    /* Set up the parameter arguments passed to the application */
    unsigned char destAddrBytes[18];
    unsigned char sourceAddrBytes[18];
    setAddrString(destAddrBytes, destAddr);
    setAddrString(sourceAddrBytes, sourceAddr);
    UA_String destAddrStr = {17, destAddrBytes};
    UA_String sourceAddrStr = {17, sourceAddrBytes};

    size_t paramsSize = 2;
    UA_KeyValuePair params[6];
    params[0].key = UA_QUALIFIEDNAME(0, "destination-address");
    UA_Variant_setScalar(&params[0].value, &destAddrStr, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "source-address");
    UA_Variant_setScalar(&params[1].value, &sourceAddrStr, &UA_TYPES[UA_TYPES_STRING]);

    if(etherType > 0) {
        params[2].key = UA_QUALIFIEDNAME(0, "ethertype");
        UA_Variant_setScalar(&params[1].value, &etherType, &UA_TYPES[UA_TYPES_UINT16]);
        paramsSize++;
    }

    if(vid > 0) {
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "vid");
        UA_Variant_setScalar(&params[paramsSize].value, &vid, &UA_TYPES[UA_TYPES_UINT16]);
        params[paramsSize+1].key = UA_QUALIFIEDNAME(0, "pcp");
        UA_Variant_setScalar(&params[paramsSize+1].value, &pcp, &UA_TYPES[UA_TYPES_BYTE]);
        params[paramsSize+2].key = UA_QUALIFIEDNAME(0, "dei");
        UA_Variant_setScalar(&params[paramsSize+2].value, &dei, &UA_TYPES[UA_TYPES_BOOLEAN]);
        paramsSize += 3;
    }

    /* Callback to the application layer with the Ethernet header hidden */
    UA_KeyValueMap map = {paramsSize, params};
    response.data += headerSize;
    response.length -= headerSize;
    UA_UNLOCK(&el->elMutex);
    conn->applicationCB(cm, (uintptr_t)rfd->fd, conn->application, &conn->context,
                        UA_CONNECTIONSTATE_ESTABLISHED, &map, response);
    UA_LOCK(&el->elMutex);
    response.data -= headerSize;
    response.length += headerSize;
}

static UA_StatusCode
ETH_openListenConnection(UA_EventLoopPOSIX *el, ETH_FD *conn,
                         const UA_KeyValueMap *params,
                         int ifindex, UA_UInt16 etherType,
                         UA_Boolean validate) {
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Bind the socket to interface and EtherType. Don't receive anything else. */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(struct sockaddr_ll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(etherType);
    sll.sll_ifindex = ifindex;
    if(!validate && bind(conn->rfd.fd, (struct sockaddr*)&sll, sizeof(sll)) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Immediately register for listen events. Don't have to wait for a
     * connection to open. */
    conn->rfd.listenEvents = UA_FDEVENT_IN;

    /* Set receiving to promiscuous (all target host addresses) */
    const UA_Boolean *promiscuous = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_PROMISCUOUS].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(promiscuous && *promiscuous) {
        struct packet_mreq mreq;
        memset(&mreq, 0, sizeof(struct packet_mreq));
        mreq.mr_ifindex = ifindex;
        mreq.mr_type = PACKET_MR_PROMISC;
        int ret = setsockopt(conn->rfd.fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                             &mreq, sizeof(mreq));
        if(ret < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                            "ETH %u\t| Could not set raw socket to promiscuous mode %s",
                            (unsigned)conn->rfd.fd, errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        } else {
            UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "ETH %u\t| The socket was set to promiscuous mode",
                        (unsigned)conn->rfd.fd);
        }
    }

    /* Register for multicast if an address is defined */
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_ADDR].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(address) {
        UA_Byte addr[ETHER_ADDR_LEN];
        UA_StatusCode res = parseEthAddress(address, addr);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                         "ETH\t| Address for listening cannot be parsed");
            return res;
        }

        if(!isMulticastEthAddress(addr)) {
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                         "ETH\t| Address for listening is not a multicast address. Ignoring.");
            return UA_STATUSCODE_GOOD;
        }

        struct packet_mreq mreq;
        memset(&mreq, 0, sizeof(struct packet_mreq));
        mreq.mr_ifindex = ifindex;
        mreq.mr_type = PACKET_MR_MULTICAST;
        mreq.mr_alen = ETH_ALEN;
        memcpy(mreq.mr_address, addr, ETHER_ADDR_LEN);
        if(!validate && UA_setsockopt(conn->rfd.fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                                      (char *)&mreq, sizeof(mreq)) < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                            "ETH\t| Registering for multicast failed with error %s",
                            errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "ETH %u\t| Opened an Ethernet listen socket",
                (unsigned)conn->rfd.fd);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ETH_openSendConnection(UA_EventLoopPOSIX *el, ETH_FD *conn, const UA_KeyValueMap *params,
                       UA_Byte source[ETHER_ADDR_LEN], int ifindex, UA_UInt16 etherType) {
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Parse the target address (has to exist) */
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_ADDR].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    UA_Byte dest[ETHER_ADDR_LEN];
    UA_StatusCode res = parseEthAddress(address, dest);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "ETH\t| Could not parse the Ethernet address \"%.*s\"",
                     (int)address->length, (char*)address->data);
        return res;
    }

    /* Get the VLAN config */
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_Boolean eid = false;

    const UA_UInt16 *vidp = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_VID].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(vidp)
        vid = *vidp;

    const UA_Byte *pcpp = (const UA_Byte*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_PCP].name,
                                 &UA_TYPES[UA_TYPES_BYTE]);
    if(pcpp)
        pcp = *pcpp;

    const UA_Boolean *eidp = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_DEI].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(eidp)
        eid = *eidp;

    /* Store the structure for sendto */
    conn->sll.sll_ifindex = ifindex;
	conn->sll.sll_halen = ETH_ALEN;
    memcpy(conn->sll.sll_addr, dest, ETHER_ADDR_LEN);

    /* Generate the Ethernet header */
    conn->headerSize = setETHHeader(conn->header, dest, source, etherType,
                                    vid, pcp, eid, &conn->lengthOffset);

    /* Set the send priority if defined */
    const UA_Int32 *soPriority = (const UA_Int32*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_PRIORITY].name,
                                 &UA_TYPES[UA_TYPES_INT32]);
    if(soPriority) {
        int prioRes = setsockopt(conn->rfd.fd, SOL_SOCKET, SO_PRIORITY,
                                 soPriority, sizeof(int));
        if(prioRes != 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                            "setsockopt SO_PRIORITY failed with error %s", errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* Enable txtime sending */
    const UA_Boolean *txtime_enable = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_TXTIME_ENABLE].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_UInt32 *txtime_flags = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_TXTIME_FLAGS].name,
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(txtime_enable && *txtime_enable) {
#ifndef SO_TXTIME
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "ETH %u\t| txtime feature not supported",
                       (unsigned)conn->rfd.fd);
#else
        struct sock_txtime so_txtime_val;
        memset(&so_txtime_val, 0, sizeof(struct sock_txtime));
        so_txtime_val.clockid = el->clockSourceMonotonic;
        so_txtime_val.flags = SOF_TXTIME_REPORT_ERRORS;
        if(txtime_flags)
            so_txtime_val.flags = *txtime_flags;
        if(setsockopt(conn->rfd.fd, SOL_SOCKET, SO_TXTIME,
                      &so_txtime_val, sizeof(so_txtime_val)) == 0) {
            conn->txtimeEnabled = true;
        } else {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                              "ETH %u\t| Could not enable txtime (%s)",
                              (unsigned)conn->rfd.fd, errno_str));
        }
#endif
    }

    /* Done creating the socket */
    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "ETH %u\t| Opened an Ethernet send socket",
                (unsigned)conn->rfd.fd);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ETH_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)cm->eventSource.eventLoop;

    UA_LOCK(&el->elMutex);

    /* Listen or send connection? */
    const UA_Boolean *listen = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_LISTEN].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    size_t ethParamRestrictions = ETH_PARAMETERSSIZE;
    if(!listen || !*listen)
        ethParamRestrictions++; /* Use the last restriction only for send connections */

    /* Validate the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "ETH", ethConnectionParams,
                                        ethParamRestrictions, params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return res;
    }

    /* Only validate the parameters? */
    UA_Boolean validate = false;
    const UA_Boolean *validateParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_VALIDATE].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validateParam)
        validate = *validateParam;

    /* Get the EtherType parameter */
    UA_UInt16 etherType = ETH_P_ALL;
    const UA_UInt16 *etParam =  (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_ETHERTYPE].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(etParam)
        etherType = *etParam;

    /* Get the interface index */
    const UA_String *interface = (const UA_String*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_IFACE].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(interface->length >= 128) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    char ifname[128];
    memcpy(ifname, interface->data, interface->length);
    ifname[interface->length] = 0;
    int ifindex = (int)if_nametoindex(ifname);
    if(ifindex == 0) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "ETH\t| Could not find the interface %s", ifname);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create the socket and add the basic configuration */
    ETH_FD *conn = NULL;
    UA_FD sockfd;
    if(listen && *listen)
        sockfd = socket(PF_PACKET, SOCK_RAW, htons(etherType));
    else
        sockfd = socket(PF_PACKET, SOCK_RAW, 0); /* Don't receive */
    if(sockfd == -1) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "ETH\t| Could not create a raw Ethernet socket (are you root?)");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    res |= UA_EventLoopPOSIX_setReusable(sockfd);
    res |= UA_EventLoopPOSIX_setNonBlocking(sockfd);
    res |= UA_EventLoopPOSIX_setNoSigPipe(sockfd);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Create the FD object */
    conn = (ETH_FD*)UA_calloc(1, sizeof(ETH_FD));
    if(!conn) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    conn->rfd.fd = sockfd;
    conn->rfd.es = &pcm->cm.eventSource;
    conn->rfd.eventSourceCB = (UA_FDCallback)ETH_connectionSocketCallback;
    conn->context = context;
    conn->application = application;
    conn->applicationCB = connectionCallback;

    /* Configure a listen or a send connection */
    if(!listen || !*listen) {
        /* Get the source address for the interface */
        struct ifreq ifr;
        memcpy(ifr.ifr_name, ifname, interface->length);
        ifr.ifr_name[interface->length] = 0;
        int result = ioctl(conn->rfd.fd, SIOCGIFHWADDR, &ifr);
        if(result == -1) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                            "ETH %u\t| Cannot get the source address, %s",
                            (unsigned)conn->rfd.fd, errno_str));
            res = UA_STATUSCODE_BADCONNECTIONREJECTED;
            goto cleanup;
        }
        res = ETH_openSendConnection(el, conn, params,
                                     (unsigned char*)ifr.ifr_hwaddr.sa_data,
                                     ifindex, etherType);
    } else {
        res = ETH_openListenConnection(el, conn, params, ifindex, etherType, validate);
    }

    /* Don't actually open or shut down */
    if(validate || res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Register in the EventLoop */
    res = UA_EventLoopPOSIX_registerFD(el, &conn->rfd);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Register locally */
    ZIP_INSERT(UA_FDTree, &pcm->fds, &conn->rfd);
    pcm->fdsSize++;

    /* Register the listen socket in the application */
    UA_UNLOCK(&el->elMutex);
    connectionCallback(cm, (uintptr_t)sockfd, application, &conn->context,
                       UA_CONNECTIONSTATE_ESTABLISHED, &UA_KEYVALUEMAP_NULL,
                       UA_BYTESTRING_NULL);
    return UA_STATUSCODE_GOOD;

 cleanup:
    UA_close(sockfd);
    UA_free(conn);
    UA_UNLOCK(&el->elMutex);
    return res;
}

static void
ETH_shutdown(UA_POSIXConnectionManager *pcm, ETH_FD *conn) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&((UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop)->elMutex, 1);

    UA_DelayedCallback *dc = &conn->rfd.dc;
    if(dc->callback) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "ETH %u\t| Cannot close - already closing",
                    (unsigned)conn->rfd.fd);
        return;
    }

    /* Shutdown the socket to cancel the current select/epoll */
    shutdown(conn->rfd.fd, UA_SHUT_RDWR);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Shutdown called", (unsigned)conn->rfd.fd);

    dc->callback = ETH_delayedClose;
    dc->application = pcm;
    dc->context = conn;

    /* Don't use the "public" el->addDelayedCallback. It takes a lock. */
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
}

static UA_StatusCode
ETH_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_LOCK(&el->elMutex);

    /* Get the ETH_FD */
    UA_FD fd = (UA_FD)connectionId;
    UA_RegisteredFD *rfd = ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!rfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "ETH\t| Cannot close Ethernet connection %u - not found",
                       (unsigned)connectionId);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    ETH_shutdown(pcm, (ETH_FD*)rfd);
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

#ifdef SO_TXTIME
static ssize_t
send_txtime(UA_EventLoopPOSIX *el, ETH_FD *conn, const UA_KeyValueMap *params,
            UA_DateTime txtime, const char *bytes, size_t bytesSize) {
    /* Get additiona parameters */
    const UA_UInt16 *txtime_pico = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_TXTIME_PICO].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    const UA_Boolean *txtime_drop = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params,
                                 ethConnectionParams[ETH_PARAMINDEX_TXTIME_DROP].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
#ifndef SCM_DROP_IF_LATE
    if(txtime_drop) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "ETH %u\t| txtime drop_if_late not supported on the current system",
                     (unsigned)conn->rfd.fd);
        return 0;
    }
#endif


    /* Transform from 100ns since 1601 to ns since the Unix Epoch */
    UA_UInt64 transmission_time = (UA_UInt64)
        (txtime - UA_DATETIME_UNIX_EPOCH) * 100;
    if(txtime_pico)
        transmission_time += (*txtime_pico) / 1000;

    /* Structure for scattering or gathering of input/output */
    struct iovec inputOutputVec;
    inputOutputVec.iov_base = (void*)(uintptr_t)bytes;
    inputOutputVec.iov_len  = bytesSize;

    /* Specify the transmission time in the CMSG. */
    char dataPacket[CMSG_SPACE(sizeof(uint64_t))
#ifdef SCM_DROP_IF_LATE
                    + CMSG_SPACE(sizeof(uint8_t))
#endif
                    ];
    struct msghdr message;
    memset(&message, 0, sizeof(struct msghdr));
    message.msg_control    = dataPacket;
    message.msg_controllen = sizeof(dataPacket);
    message.msg_name       = (struct sockaddr*)&conn->sll;
    message.msg_namelen    = sizeof(conn->sll);
    message.msg_iov        = &inputOutputVec;
    message.msg_iovlen     = 1;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&message);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_TXTIME;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(__u64));
    *((__u64*)CMSG_DATA(cmsg)) = transmission_time;

#ifdef SCM_DROP_IF_LATE
    cmsg = CMSG_NXTHDR(&message, cmsg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_DROP_IF_LATE;
    cmsg->cmsg_len = CMSG_LEN(sizeof(uint8_t));
    *((uint8_t*)CMSG_DATA(cmsg)) = (!txtime_drop || *txtime_drop) ? 1: 0;
#endif

    /* Send */
    return sendmsg(conn->rfd.fd, &message, 0);
}
#endif

static UA_StatusCode
ETH_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params, UA_ByteString *buf) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;

    UA_LOCK(&el->elMutex);

    /* Get the ETH_FD */
    UA_FD fd = (UA_FD)connectionId;
    ETH_FD *conn = (ETH_FD*)ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!conn) {
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    /* Uncover and set the Ethernet header */
    buf->data -= conn->headerSize;
    buf->length += conn->headerSize;
    memcpy(buf->data, conn->header, conn->headerSize);
    if(conn->lengthOffset) {
        UA_UInt16 *ethLength =  (UA_UInt16*)&buf->data[conn->lengthOffset];
        *ethLength = htons((UA_UInt16)(buf->length - conn->headerSize));
    }

    /* Was a txtime configured? */
    const UA_DateTime *txtime = (const UA_DateTime*)
        UA_KeyValueMap_getScalar(params, ethConnectionParams[ETH_PARAMINDEX_TXTIME].name,
                                 &UA_TYPES[UA_TYPES_DATETIME]);
    if(txtime && !conn->txtimeEnabled) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "ETH %u\t| txtime was not configured for the connection",
                     (unsigned)connectionId);
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Prevent OS signals when sending to a closed socket */
    int flags = MSG_NOSIGNAL;

    struct pollfd tmp_poll_fd;
    tmp_poll_fd.fd = (UA_FD)connectionId;
    tmp_poll_fd.events = UA_POLLOUT;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                         "ETH %u\t| Attempting to send", (unsigned)connectionId);
            size_t bytes_to_send = buf->length - nWritten;
#ifdef SO_TXTIME
            if(txtime) {
                n = send_txtime(el, conn, params, *txtime,
                                (const char*)buf->data + nWritten, bytes_to_send);
            } else
#endif
            {
                n = UA_sendto(conn->rfd.fd,
                              (const char*)buf->data + nWritten, bytes_to_send,
                              flags, (struct sockaddr*)&conn->sll, sizeof(conn->sll));
            }
            if(n < 0) {
                /* An error we cannot recover from? */
                if(UA_ERRNO != UA_INTERRUPTED &&
                   UA_ERRNO != UA_WOULDBLOCK &&
                   UA_ERRNO != UA_AGAIN) {
                    UA_LOG_SOCKET_ERRNO_WRAP(
                       UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                                    "ETH %u\t| Send failed with error %s",
                                    (unsigned)connectionId, errno_str));
                    ETH_shutdown(pcm, conn);
                    UA_UNLOCK(&el->elMutex);
                    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }

                /* Poll for the socket resources to become available and retry
                 * (blocking) */
                int poll_ret;
                do {
                    poll_ret = UA_poll(&tmp_poll_fd, 1, 100);
                    if(poll_ret < 0 && UA_ERRNO != UA_INTERRUPTED) {
                        UA_LOG_SOCKET_ERRNO_WRAP(
                           UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                                        "ETH %u\t| Send failed with error %s",
                                        (unsigned)connectionId, errno_str));
                        ETH_shutdown(pcm, conn);
                        UA_UNLOCK(&el->elMutex);
                        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
                        return UA_STATUSCODE_BADCONNECTIONCLOSED;
                    }
                } while(poll_ret <= 0);
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_UNLOCK(&el->elMutex);
    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ETH_eventSourceStart(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK(&el->elMutex);

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "To start the Ethernet ConnectionManager, "
                     "it has to be registered in an EventLoop and not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "ETH",
                                        ethManagerParams, ETH_MANAGERPARAMS,
                                        &cm->eventSource.params);
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    /* Allocate the rx buffer */
    res = UA_EventLoopPOSIX_allocateStaticBuffers(pcm);
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;

 finish:
    UA_UNLOCK(&el->elMutex);
    return res;
}

static void *
ETH_shutdownCB(void *application, UA_RegisteredFD *rfd) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    ETH_shutdown(pcm, (ETH_FD*)rfd);
    return NULL;
}

static void
ETH_eventSourceStop(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK(&el->elMutex);

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "ETH\t| Shutting down the ConnectionManager");

    /* Prevent new connections to open */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shutdown all existing connection */
    ZIP_ITER(UA_FDTree, &pcm->fds, ETH_shutdownCB, cm);

    /* Check if stopped once more (also checking inside ETH_close, but there we
     * don't check if there is no rfd at all) */
    ETH_checkStopped(pcm);

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
ETH_eventSourceDelete(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "ETH\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_ByteString_clear(&pcm->rxBuffer);
    UA_ByteString_clear(&pcm->txBuffer);
    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

static const char *ethName = "eth";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_Ethernet(const UA_String eventSourceName) {
    UA_POSIXConnectionManager *cm = (UA_POSIXConnectionManager*)
        UA_calloc(1, sizeof(UA_POSIXConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *))ETH_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))ETH_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))ETH_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char*)(uintptr_t)ethName);
    cm->cm.openConnection = ETH_openConnection;
    cm->cm.allocNetworkBuffer = ETH_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = ETH_freeNetworkBuffer;
    cm->cm.sendWithConnection = ETH_sendWithConnection;
    cm->cm.closeConnection = ETH_shutdownConnection;
    return &cm->cm;
}

#endif /* defined(UA_ARCHITECTURE_POSIX) && defined(__linux__) */

