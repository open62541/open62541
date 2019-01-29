/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_plugin_network_manager.h"

#ifndef OPEN62541_UA_NETWORKMANAGERS_H
#define OPEN62541_UA_NETWORKMANAGERS_H

_UA_BEGIN_DECLS

/**
 * Initializes a network manager that uses select for processing.
 *
 * \param logger The logger to be used by the networkManager
 * \param networkManager pointer to an empty networkManager struct, that will be initialized.
 */
UA_StatusCode
UA_SelectBasedNetworkManager(const UA_Logger *logger, UA_NetworkManager *networkManager);

_UA_END_DECLS

#endif //OPEN62541_UA_NETWORKMANAGERS_H
