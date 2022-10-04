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
#include "eventloop_common.h"

#include <arpa/inet.h> /* htons */
#include <net/ethernet.h> /* ETH_P_*/
#include <linux/if_packet.h>

/* Configuration parameters */
#define ETH_PARAMETERSSIZE 8

#define ETH_PARAMINDEX_ADDR 0
#define ETH_PARAMINDEX_LISTEN 1
#define ETH_PARAMINDEX_IFACE 2
#define ETH_PARAMINDEX_ETHERTYPE 3
#define ETH_PARAMINDEX_VID 4
#define ETH_PARAMINDEX_PCP 5
#define ETH_PARAMINDEX_DEI 6
#define ETH_PARAMINDEX_PROMISCUOUS 7

static UA_KeyValueRestriction ETHConfigParameters[ETH_PARAMETERSSIZE+1] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("interface")}, &UA_TYPES[UA_TYPES_STRING], true, true, false},
    {{0, UA_STRING_STATIC("ethertype")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("vid")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("pcp")}, &UA_TYPES[UA_TYPES_BYTE], false, true, false},
    {{0, UA_STRING_STATIC("dei")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("promiscuous")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    /* Duplicated address parameter with a scalar value required. For the send-socket case. */
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], true, true, false},
};

#define UA_ETH_MAXHEADERLENGTH (2*ETHER_ADDR_LEN)+4+2+2

typedef struct {
    UA_RegisteredFD fd;
    UA_ConnectionManager_connectionCallback connectionCallback;
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
} ETH_FD;

typedef struct {
    UA_ConnectionManager cm;

    size_t fdsSize;
    LIST_HEAD(, UA_RegisteredFD) fds;
} ETHConnectionManager;

static UA_StatusCode
ETHConnectionManager_register(ETHConnectionManager *ecm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ecm->cm.eventSource.eventLoop;

    /* Set the EventSource */
    rfd->es = &ecm->cm.eventSource;

    /* Register in the EventLoop */
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, rfd);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Add to the linked list in the ETH EventSource */
    LIST_INSERT_HEAD(&ecm->fds, rfd, es_pointers);
    ecm->fdsSize++;
    return UA_STATUSCODE_GOOD;
}

static void
ETHConnectionManager_deregister(ETHConnectionManager *ecm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ecm->cm.eventSource.eventLoop;
    UA_EventLoopPOSIX_deregisterFD(el, rfd);
    LIST_REMOVE(rfd, es_pointers);
    UA_assert(ecm->fdsSize > 0);
    ecm->fdsSize--;
}

static UA_RegisteredFD *
ETHConnectionManager_findRegisteredFD(ETHConnectionManager *ecm, uintptr_t connectionId) {
    UA_RegisteredFD *rfd;
    LIST_FOREACH(rfd, &ecm->fds, es_pointers) {
        if(rfd->fd == (UA_FD)connectionId)
            return rfd;
    }
    return NULL;
}

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
    snprintf((char*)addrStr, 18, "%02x-%02x-%02x-%02x-%02x-%02x",
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
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_RegisteredFD *rfd = ETHConnectionManager_findRegisteredFD(ecm, connectionId);
    ETH_FD *erfd = (ETH_FD*)rfd;
    if(!erfd)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    /* Allocate the buffer with the hidden Ethernet header in front */
    UA_StatusCode res = UA_ByteString_allocBuffer(buf, bufSize+erfd->headerSize);
    buf->data   += erfd->headerSize;
    buf->length -= erfd->headerSize;
    return res;
}

static void
ETH_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_RegisteredFD *rfd = ETHConnectionManager_findRegisteredFD(ecm, connectionId);
    ETH_FD *erfd = (ETH_FD*)rfd;
    if(!erfd)
        return;
    /* Unhide the Ethernet header */
    buf->data   -= erfd->headerSize;
    buf->length += erfd->headerSize;
    UA_ByteString_clear(buf);
}

/* Test if the ConnectionManager can be stopped */
static void
ETH_checkStopped(ETHConnectionManager *ecm) {
    if(ecm->fdsSize == 0 && ecm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(ecm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "ETH\t| All sockets closed, the EventLoop has stopped");
        ecm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

/* This method must not be called from the application directly, but from within
 * the EventLoop. Otherwise we cannot be sure whether the file descriptor is
 * still used after calling close. */
static void
ETH_close(ETHConnectionManager *ecm, UA_RegisteredFD *rfd) {
    ETH_FD *erfd = (ETH_FD*)rfd;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ecm->cm.eventSource.eventLoop;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Closing connection", (unsigned)rfd->fd);

    /* Deregister from the EventLoop */
    ETHConnectionManager_deregister(ecm, rfd);

    /* Signal closing to the application */
    erfd->connectionCallback(&ecm->cm, (uintptr_t)rfd->fd,
                              rfd->application, &rfd->context,
                              UA_CONNECTIONSTATE_CLOSING,
                              0, NULL, UA_BYTESTRING_NULL);

    /* Close the socket */
    int ret = UA_close(rfd->fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "ETH %u\t| Socket closed", (unsigned)rfd->fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "ETH %u\t| Could not close the socket (%s)",
                          (unsigned)rfd->fd, errno_str));
    }

    /* Don't call free here. This might be done automatically via the delayed
     * callback that calls ETH_close. */
    /* UA_free(rfd); */

    /* Stop if the ucm is stopping and this was the last open socket */
    ETH_checkStopped(ecm);
}

static void
ETH_delayedClose(void *application, void *context) {
    ETHConnectionManager *ecm = (ETHConnectionManager*)application;
    UA_ConnectionManager *cm = &ecm->cm;
    UA_RegisteredFD* rfd = (UA_RegisteredFD *)context;
    UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "ETH %u\t| Delayed closing of the connection", (unsigned)rfd->fd);
    ETH_close(ecm, rfd);
    UA_free(rfd);
}

/* Gets called when a socket receives data or closes */
static void
ETH_connectionSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                             short event) {
    ETH_FD *efd = (ETH_FD*)rfd;
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    if(event == UA_FDEVENT_ERR) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "ETH %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)rfd->fd, errno_str));

        /* A new socket has opened. Signal it to the application. */
        efd->connectionCallback(cm, (uintptr_t)rfd->fd,
                                rfd->application, &rfd->context,
                                UA_CONNECTIONSTATE_CLOSING,
                                0, NULL, UA_BYTESTRING_NULL);
        ETH_close(ecm, rfd);
        UA_free(rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Allocate receive buffer", (unsigned)rfd->fd);

    /* Get the number of available bytes */
    int bytes_available = 0;
    UA_ioctl(rfd->fd, FIONREAD, &bytes_available);
    if(bytes_available <= 0)
        return;

    UA_ByteString response;
    UA_StatusCode res = UA_ByteString_allocBuffer(&response, (size_t)bytes_available);
    if(res != UA_STATUSCODE_GOOD)
        return;

    /* Receive */
#ifndef _WIN32
    ssize_t ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#else
    int ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#endif

    /* Receive has failed */
    if(ret <= 0) {
        if(UA_ERRNO == UA_INTERRUPTED)
            goto finish;

        /* Orderly shutdown of the socket. We can immediately close as no method
         * "below" in the call stack will use the socket in this iteration of
         * the EventLoop. */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "ETH %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)rfd->fd, errno_str));
        ETH_close(ecm, rfd);
        UA_free(rfd);
        goto finish;
    }

    /* Set the length of the received buffer */
    response.length = (size_t)ret;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Received message of size %u",
                 (unsigned)rfd->fd, (unsigned)ret);

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
        goto finish;

    /* Set up the parameter arguments */
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
    response.data += headerSize;
    response.length -= headerSize;
    efd->connectionCallback(cm, (uintptr_t)rfd->fd, rfd->application, &rfd->context,
                              UA_CONNECTIONSTATE_ESTABLISHED, paramsSize, params, response);
    response.data -= headerSize;
    response.length += headerSize;

 finish:
    UA_ByteString_clear(&response);
}

static UA_StatusCode
ETH_openListenConnection(UA_EventLoop *el, ETH_FD *fd,
                         size_t paramsSize, const UA_KeyValuePair *params,
                         int ifindex, UA_UInt16 etherType) {
    /* Bind the socket to interface and EtherType. Don't receive anything else. */
    struct sockaddr_ll sll = {0};
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(etherType);
    sll.sll_ifindex = ifindex;
    if(UA_bind(fd->fd.fd, (struct sockaddr*)&sll, sizeof(sll)) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Immediately register for listen events. Don't have to wait for a
     * connection to open. */
    fd->fd.listenEvents = UA_FDEVENT_IN;

    /* Set receiving to promiscuous (all target host addresses) */
    const UA_Boolean *promiscuous = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_PROMISCUOUS].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(promiscuous && *promiscuous) {
        struct packet_mreq mreq = {0};
        mreq.mr_ifindex = ifindex;
        mreq.mr_type = PACKET_MR_PROMISC;
        int ret = setsockopt(fd->fd.fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if(ret < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                            "ETH %u\t| Could not set raw socket to promiscuous mode %s",
                            (unsigned)fd->fd.fd, errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        } else {
            UA_LOG_INFO(el->logger, UA_LOGCATEGORY_NETWORK,
                        "ETH %u\t| The socket was set to promiscuous mode", (unsigned)fd->fd.fd);
        }
    }

    /* Register for multicast if an address is defined */
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_ADDR].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(address) {
        UA_Byte addr[ETHER_ADDR_LEN];
        UA_StatusCode res = parseEthAddress(address, addr);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                         "ETH\t| Address for listening cannot be parsed");
            return res;
        }

        if(!isMulticastEthAddress(addr)) {
            UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                         "ETH\t| Address for listening is not a multicast address. Ignoring.");
            return UA_STATUSCODE_GOOD;
        }

        struct packet_mreq mreq;
        mreq.mr_ifindex = ifindex;
        mreq.mr_type = PACKET_MR_MULTICAST;
        mreq.mr_alen = ETH_ALEN;
        memcpy(mreq.mr_address, addr, ETHER_ADDR_LEN);
        int ret = UA_setsockopt(fd->fd.fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
                                (char *)&mreq, sizeof(mreq));
        if(ret < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                            "ETH\t| Registering for multicast failed with error %s", errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    } 

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ETH_openSendConnection(UA_EventLoop *el, ETH_FD *fd,
                       size_t paramsSize, const UA_KeyValuePair *params,
                       UA_Byte source[ETHER_ADDR_LEN], int ifindex, UA_UInt16 etherType) {
    /* Parse the target address (has to exist) */
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_ADDR].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    UA_Byte dest[ETHER_ADDR_LEN];
    UA_StatusCode res = parseEthAddress(address, dest);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                     "ETH\t| Could not parse the Ethernet address \"%.*s\"",
                     (int)address->length, (char*)address->data);
        return res;
    }

    /* Get the VLAN config */
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_Boolean eid = false;

    const UA_UInt16 *vidp = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_VID].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(vidp)
        vid = *vidp;

    const UA_Byte *pcpp = (const UA_Byte*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_PCP].name,
                                 &UA_TYPES[UA_TYPES_BYTE]);
    if(pcpp)
        pcp = *pcpp;

    const UA_Boolean *eidp = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_DEI].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(eidp)
        eid = *eidp;

    /* Store the structure for sendto */
    fd->sll.sll_ifindex = ifindex;
	fd->sll.sll_halen = ETH_ALEN;
    memcpy(fd->sll.sll_addr, dest, ETHER_ADDR_LEN);

    /* Generate the Ethernet header */
    fd->headerSize = setETHHeader(fd->header, dest, source, etherType,
                                  vid, pcp, eid, &fd->lengthOffset);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ETH_openConnection(UA_ConnectionManager *cm, size_t paramsSize, const UA_KeyValuePair *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_EventLoop *el = cm->eventSource.eventLoop;

    /* Listen or send connection? */
    const UA_Boolean *listen = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_LISTEN].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    size_t ethParams = ETH_PARAMETERSSIZE;
    if(!listen || !*listen)
        ethParams++; /* Require the address parameter to exist */

    /* Validate the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(ETHConfigParameters, ETH_PARAMETERSSIZE,
                                        params, paramsSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Get the EtherType parameter */
    UA_UInt16 etherType = ETH_P_ALL;
    const UA_UInt16 *etParam =  (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_ETHERTYPE].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(etParam)
        etherType = *etParam;

    /* Get the interface index */
    const UA_String *interface = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 ETHConfigParameters[ETH_PARAMINDEX_IFACE].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(interface->length >= 128)
        return UA_STATUSCODE_BADINTERNALERROR;
    char ifname[128];
    memcpy(ifname, interface->data, interface->length);
    ifname[interface->length] = 0;
    int ifindex = (int)if_nametoindex(ifname);
    if(ifindex == 0) {
        UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                     "ETH \t| Could not find the interface %s", ifname);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create the socket and add the basic configuration */
    ETH_FD *fd = NULL;
    UA_FD sockfd;
    if(listen && *listen)
        sockfd = UA_socket(PF_PACKET, SOCK_RAW, htons(etherType));
    else
        sockfd = UA_socket(PF_PACKET, SOCK_RAW, 0); /* Don't receive */
    if(sockfd == -1) {
        UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                     "ETH \t| Could not create a raw Ethernet socket (are you root?)");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    res |= UA_EventLoopPOSIX_setReusable(sockfd);
    res |= UA_EventLoopPOSIX_setNonBlocking(sockfd);
    res |= UA_EventLoopPOSIX_setNoSigPipe(sockfd);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Create the FD object */
    fd = (ETH_FD*)UA_calloc(1, sizeof(ETH_FD));
    if(!fd) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    fd->fd.context = context;
    fd->fd.application = application;
    fd->fd.callback = (UA_FDCallback)ETH_connectionSocketCallback;
    fd->fd.fd = sockfd;
    fd->connectionCallback = connectionCallback;

    /* Configure a listen or a send connection */
    if(!listen || !*listen) {
        /* Get the source address for the interface */
        struct ifreq ifr;
        memcpy(ifr.ifr_name, ifname, interface->length);
        ifr.ifr_name[interface->length] = 0;
        int result = ioctl(fd->fd.fd, SIOCGIFHWADDR, &ifr);
        if(result == -1) {
            UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                            "ETH %u\t| Cannot get the source address, %s",
                            (unsigned)fd->fd.fd, errno_str));
            res = UA_STATUSCODE_BADCONNECTIONREJECTED;
            goto cleanup;
        }
        res = ETH_openSendConnection(el, fd, paramsSize, params,
                                     (unsigned char*)ifr.ifr_hwaddr.sa_data,
                                     ifindex, etherType);
    } else {
        res = ETH_openListenConnection(el, fd, paramsSize, params, ifindex, etherType);
    }
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    res = ETHConnectionManager_register(ecm, &fd->fd);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Register the listen socket in the application */
    connectionCallback(cm, (uintptr_t)sockfd, application, &fd->fd.context,
                       UA_CONNECTIONSTATE_ESTABLISHED, 0, NULL, UA_BYTESTRING_NULL);
    return UA_STATUSCODE_GOOD;

 cleanup:
    UA_close(sockfd);
    UA_free(fd);
    return res;
}

/* Close the connection via a delayed callback */
static void
ETH_shutdown(UA_ConnectionManager *cm, UA_RegisteredFD *rfd) {
    UA_EventLoop *el = cm->eventSource.eventLoop;
    if(rfd->dc.callback) {
        UA_LOG_INFO(el->logger, UA_LOGCATEGORY_NETWORK,
                    "ETH %u\t| Cannot close - already closing",
                    (unsigned)rfd->fd);
        return;
    }

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                 "ETH %u\t| Shutdown called", (unsigned)rfd->fd);

    UA_DelayedCallback *dc = &rfd->dc;
    dc->callback = ETH_delayedClose;
    dc->application = cm;
    dc->context = rfd;
    el->addDelayedCallback(el, dc);
}

static UA_StatusCode
ETH_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_EventLoop *el = cm->eventSource.eventLoop;
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_RegisteredFD *rfd = ETHConnectionManager_findRegisteredFD(ecm, connectionId);
    if(!rfd) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                       "ETH\t| Cannot close Ethernet connection %u - not found",
                       (unsigned)connectionId);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    ETH_shutdown(cm, rfd);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ETH_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       size_t paramsSize, const UA_KeyValuePair *params,
                       UA_ByteString *buf) {
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_EventLoop *el = cm->eventSource.eventLoop;
    UA_RegisteredFD *rfd = ETHConnectionManager_findRegisteredFD(ecm, connectionId);
    ETH_FD *efd = (ETH_FD*)rfd;
    if(!efd)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    /* Uncover and set the Ethernet header */
    buf->data -= efd->headerSize;
    buf->length += efd->headerSize;
    memcpy(buf->data, efd->header, efd->headerSize);
    if(efd->lengthOffset) {
        UA_UInt16 *ethLength =  (UA_UInt16*)&buf->data[efd->lengthOffset];
        *ethLength = htons((UA_UInt16)(buf->length - efd->headerSize));
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
            UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                         "ETH %u\t| Attempting to send", (unsigned)connectionId);
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_sendto(rfd->fd, (const char*)buf->data + nWritten, bytes_to_send,
                          flags, (struct sockaddr*)&efd->sll, sizeof(efd->sll));
            if(n < 0) {
                /* An error we cannot recover from? */
                if(UA_ERRNO != UA_INTERRUPTED &&
                   UA_ERRNO != UA_WOULDBLOCK &&
                   UA_ERRNO != UA_AGAIN) {
                    UA_LOG_SOCKET_ERRNO_WRAP(
                       UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                                    "ETH %u\t| Send failed with error %s",
                                    (unsigned)connectionId, errno_str));
                    ETH_shutdownConnection(cm, connectionId);
                    UA_ByteString_clear(buf);
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }

                /* Poll for the socket resources to become available and retry
                 * (blocking) */
                int poll_ret;
                do {
                    poll_ret = UA_poll(&tmp_poll_fd, 1, 100);
                    if(poll_ret < 0 && UA_ERRNO != UA_INTERRUPTED) {
                        UA_LOG_SOCKET_ERRNO_WRAP(
                           UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                                        "ETH %u\t| Send failed with error %s",
                                        (unsigned)connectionId, errno_str));
                        ETH_shutdownConnection(cm, connectionId);
                        UA_ByteString_clear(buf);
                        return UA_STATUSCODE_BADCONNECTIONCLOSED;
                    }
                } while(poll_ret <= 0);
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

/* Asynchronously register the listenSocket */
static UA_StatusCode
ETH_eventSourceStart(UA_ConnectionManager *cm) {
    /* Check the state */
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "To start the Ethernet ConnectionManager, "
                     "it has to be registered in an EventLoop and not started");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
ETH_eventSourceStop(UA_ConnectionManager *cm) {
    ETHConnectionManager *ecm = (ETHConnectionManager*)cm;
    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                "ETH\t| Shutting down the ConnectionManager");

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shut down all registered fd. The cm is set to "stopped" when the last fd
     * is closed (from within ETH_close). */
    UA_RegisteredFD *rfd;
    LIST_FOREACH(rfd, &ecm->fds, es_pointers) {
        ETH_shutdown(cm, rfd);
    }

    /* Check if stopped once more (also checking inside ETH_close, but there we
     * don't check if there is no rfd at all) */
    ETH_checkStopped(ecm);
}

static UA_StatusCode
ETH_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "ETH\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Delete the parameters */
    UA_Array_delete(cm->eventSource.params,
                    cm->eventSource.paramsSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    cm->eventSource.params = NULL;
    cm->eventSource.paramsSize = 0;

    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

static const char *ethName = "eth";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_Ethernet(const UA_String eventSourceName) {
    ETHConnectionManager *cm = (ETHConnectionManager*)
        UA_calloc(1, sizeof(ETHConnectionManager));
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
