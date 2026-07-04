/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_DRIVER_MDNS_H_
#define UA_DRIVER_MDNS_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

/**
 * Multicast DNS Discovery Driver
 * ------------------------------
 * An OPC UA application can announce / retract a server via multicast DNS. Also
 * it receives multicast-based information about other servers. This
 * functionality is implemented as a driver that integrates with open62541
 * servers.
 * Internally, the drivers use the EventLoop of the server to create and manage
 * UDP sockets. They further use the UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY
 * category for server notifications to receive relevant updates from the server
 * at runtime.
 *
 * The following configuration parameters are forwarded in a key-value map
 * during driver instantiation and/or set in the key-value map inside the
 * generic driver structure before starting the driver.
 *
 * 0:listen [bool]
 *    Receive information about OPC UA servers on the network via mDNS. Calls
 *    UA_Server_registerServerOnNetwork and UA_Server_deregisterServerOnNetwork
 *    to store the server information locally.
 *
 * 0:announce [bool]
 *    Announce registered servers over mDNS. This receives register/deregister
 *    notifications from UA_Server_registerServerOnNetwork and forwards the
 *    information.
 *
 * 0:announce-ttl [uint32]
 *    Default TTL in seconds for announced mDNS records.
 *
 * 0:query-presence [bool]
 *    Send an mDNS PTR query for _opcua-tcp._tcp.local. on startup. Defaults to
 *    false.
 *
 * 0:query-details [bool]
 *    Send targeted SRV/TXT follow-up queries for partially received OPC UA
 *    service instances. Defaults to false.
 *
 * 0:query-interval [uint32]
 *    Interval in seconds for repeated presence queries. A value of zero
 *    disables repeated queries. Defaults to zero.
 *
 * 0:interface [string]
 *    Network interface for listening or sending. Can be either the IP address
 *    of the network interface or the interface name (e.g. 'eth0'). Cf. the UDP
 *    ConnectionManager parameters. */

typedef struct UA_MdnsDriver UA_MdnsDriver;

struct UA_MdnsDriver {
    UA_Driver drv; /* Must be the first member */

    /* Manually publish/remove address records. This is separate from the
     * ServerOnNetwork announcement path as PTR/SRV/TXT records are always
     * derived from the ServerOnNetwork data. The address strings are numeric
     * IPv4/IPv6 literals. */
    UA_StatusCode (*addARecord)(UA_MdnsDriver *mdns, UA_String hostname,
                                UA_String address, UA_UInt32 ttl);
    UA_StatusCode (*removeARecord)(UA_MdnsDriver *mdns, UA_String hostname,
                                   UA_String address);
    UA_StatusCode (*addAAAARecord)(UA_MdnsDriver *mdns, UA_String hostname,
                                   UA_String address, UA_UInt32 ttl);
    UA_StatusCode (*removeAAAARecord)(UA_MdnsDriver *mdns, UA_String hostname,
                                      UA_String address);
};

/* Based on the libmdnsd dependency */
UA_EXPORT UA_MdnsDriver *
UA_MdnsDriver_Mdnsd(const UA_KeyValueMap params);

/* Based on the libavahi dependency */
UA_EXPORT UA_MdnsDriver *
UA_MdnsDriver_Avahi(const UA_KeyValueMap params);

_UA_END_DECLS

#endif /* UA_DRIVER_MDNS_H_ */
