/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_plugin_network_manager.h"

#ifndef OPEN62541_UA_NETWORKMANAGERS_H
#define OPEN62541_UA_NETWORKMANAGERS_H

UA_StatusCode
UA_SelectBasedNetworkManager(UA_Logger *logger, UA_NetworkManager **p_networkManager);

#endif //OPEN62541_UA_NETWORKMANAGERS_H
