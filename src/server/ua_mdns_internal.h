/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_MDNS_INTERNAL_H
#define UA_MDNS_INTERNAL_H

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

/**
 * TXT record:
 * [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
 *
 * A/AAAA record for all ip addresses:
 * [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
 * [hostname]. A [ip].
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "mdnsd/libmdnsd/mdnsd.h"

void mdns_record_received(const struct resource *r, void *data);

void mdns_create_txt(UA_Server *server, const char *fullServiceDomain,
                     const char *path, const UA_String *capabilites,
                     const size_t *capabilitiesSize,
                     void (*conflict)(char *host, int type, void *arg));

void mdns_set_address_record(UA_Server *server, const char *fullServiceDomain,
                             const char *localDomain);

mdns_record_t *
mdns_find_record(mdns_daemon_t *mdnsDaemon, unsigned short type,
                 const char *host, const char *rdname);

void
UA_Discovery_update_MdnsForDiscoveryUrl(UA_Server *server, const UA_String *serverName,
                                        const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                        const UA_String *discoveryUrl,
                                        UA_Boolean isOnline, UA_Boolean updateTxt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UA_ENABLE_DISCOVERY_MULTICAST

#endif //UA_MDNS_INTERNAL_H
