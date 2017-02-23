/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_MDNS_INTERNAL_H
#define UA_MDNS_INTERNAL_H


#ifdef UA_ENABLE_DISCOVERY_MULTICAST

#ifdef __cplusplus
extern "C" {
#endif

#include "mdnsd/libmdnsd/mdnsd.h"

int mdns_hash_record(const char *s);

struct serverOnNetwork_list_entry *
mdns_record_add_or_get(UA_Server *server, const char *record, const char *serverName,
                       size_t serverNameLen, UA_Boolean createNew);

void mdns_record_remove(UA_Server *server, const char *record,
                        struct serverOnNetwork_list_entry *entry);

void mdns_append_path_to_url(UA_String *url, const char *path);

void mdns_record_received(const struct resource *r, void *data);

// TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
void mdns_create_txt(UA_Server *server, const char *fullServiceDomain, const char *path,
                     const UA_String *capabilites, const size_t *capabilitiesSize,
                     void (*conflict)(char *host, int type, void *arg));


// A/AAAA record for all ip addresses.
// [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
// [hostname]. A [ip].
void mdns_set_address_record(UA_Server *server, const char *fullServiceDomain, const char *localDomain);

mdns_record_t *mdns_find_record(const mdns_daemon_t *mdnsDaemon, unsigned short type,
                      const char *host, const char *rdname);

#ifdef __cplusplus
} // extern "C"
#endif


#endif // UA_ENABLE_DISCOVERY_MULTICAST

#endif //UA_MDNS_INTERNAL_H
