/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_PLUGIN_NETWORK_MANAGER_H
#define OPEN62541_UA_PLUGIN_NETWORK_MANAGER_H

#include "ua_plugin_socket.h"

typedef struct UA_NetworkManager UA_NetworkManager;

struct UA_NetworkManager {
    UA_StatusCode (*registerSocket)(UA_NetworkManager *networkManager, UA_Socket *socket);

    UA_StatusCode (*unregisterSocket)(UA_NetworkManager *networkManager, UA_Socket *socket);

    UA_StatusCode (*process)(UA_NetworkManager *networkManager, UA_UInt16 timeout);

    /**
     * Checks if the supplied socket has pending activity and calls the activity callback chain
     * if so.
     * @param networkManager
     * @param timeout
     * @return
     */
    UA_StatusCode (*processSocket)(UA_NetworkManager *networkManager, UA_UInt32 timeout, UA_Socket *sock);

    /**
     * Gets all known discovery urls of listener sockets registered with the network manager.
     * This function will allocate an array of strings, which needs to be freed by the caller.
     *
     * @param networkManager the network manager to perform the operation on.
     * @param discoveryUrls the newly allocated array of discoveryUrls.
     * @param discoveryUrlsSize the size of the discoveryUrls array.
     */
    UA_StatusCode (*getDiscoveryUrls)(const UA_NetworkManager *networkManager, UA_String *discoveryUrls[],
                                      size_t *discoveryUrlsSize);

    UA_StatusCode (*shutdown)(UA_NetworkManager *networkManager);

    UA_StatusCode (*deleteMembers)(UA_NetworkManager *networkManager);

    void *internalData;
};

#endif //OPEN62541_UA_PLUGIN_NETWORK_MANAGER_H
