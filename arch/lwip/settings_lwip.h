/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_SETTINGS_LWIP_H
#define UA_SETTINGS_LWIP_H

#define HOSTNAME "Hostname"

#define LWIP_PORT_INIT_IPADDR(addr)   IP4_ADDR(&addr, 192, 168, 0, 200);
#define LWIP_PORT_INIT_GW(addr)       IP4_ADDR(&addr, 192, 168, 0, 1);
#define LWIP_PORT_INIT_NETMASK(addr)  IP4_ADDR(&addr, 255, 255, 255, 0);

#endif /* UA_SETTINGS_LWIP_H */
