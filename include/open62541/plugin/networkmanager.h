/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef OPEN62541_NETWORKMANAGER_H
#define OPEN62541_NETWORKMANAGER_H

#include <open62541/plugin/socket.h>

_UA_BEGIN_DECLS

typedef UA_StatusCode (*UA_SocketCreationFunction)(const UA_SocketConfig *parameters,
                                                   const UA_SocketCallbackFunction creationCallback);

struct UA_NetworkManager {
    /**
     * Creates a new socket using the creation function in the supplied config.
     *
     * On successful creation, the socket is kept in the network manager, until it is closed.
     * The network manager will free the socket and the socket will call its free callback.
     * The free callback can be used to clean up all references to the socket, that were created
     * with the create callback.
     *
     * \param networkManager The network manager to perform the operation on.
     * \param socketParameters The parameters of the socket, including the creation function.
     * \param creationCallback The callback that is called after a socket has been created.
     */
    UA_StatusCode (*createSocket)(UA_NetworkManager *networkManager, UA_SocketConfig *const socketParameters,
                                  const UA_SocketCallbackFunction creationCallback);

    /**
     * Processes all registered sockets.
     *
     * If a socket has pending data on it, the sockets activity function is called.
     * The activity function will perform internal processing specific to the socket.
     * When the socket has data that is ready to be processed, the dataCallback will
     * be called.
     *
     * \param networkManager The NetworkManager to perform the operation on.
     * \param timeout The process function will wait for timeout milliseconds or until
     *                one of the registered sockets is active.
     */
    UA_StatusCode (*process)(UA_NetworkManager *networkManager, UA_UInt16 timeout);

    /**
     * Checks if the supplied socket has pending activity and calls the activity callback chain
     * if there is activity.
     *
     * \param networkManager The NetworkManager to perform the operation on.
     * \param timeout The processSocket function will wait for timeout milliseconds or
     *                until the socket is active.
     * \return
     */
    UA_StatusCode (*processSocket)(UA_NetworkManager *networkManager, UA_UInt32 timeout, UA_Socket *sock);

    /**
     * Gets all known discovery urls of listener sockets registered with the network manager.
     * This function will allocate an array of strings, which needs to be freed by the caller.
     *
     * \param networkManager the network manager to perform the operation on.
     * \param discoveryUrls the newly allocated array of discoveryUrls.
     * \param discoveryUrlsSize the size of the discoveryUrls array.
     */
    UA_StatusCode (*getDiscoveryUrls)(const UA_NetworkManager *networkManager, UA_String *discoveryUrls[],
                                      size_t *discoveryUrlsSize);

    /**
     * Starts the network manager.
     * Performs initial setup and needs to be called before using the network manager.
     * \param networkManager The NetworkManager to perform the operation on.
     */
    UA_StatusCode (*start)(UA_NetworkManager *networkManager);

    /**
     * Shuts down the NetworkManager. This will shut down and free all registered sockets.
     *
     * \param networkManager The NetworkManager to perform the operation on.
     */
    UA_StatusCode (*shutdown)(UA_NetworkManager *networkManager);

    /**
     * Cleans up all internally allocated data in the NetworkManager and then frees it.
     *
     * \param networkManager The NetworkManager to perform the operation on.
     */
    UA_StatusCode (*free)(UA_NetworkManager *networkManager);

    const UA_Logger *logger;
};

_UA_END_DECLS

#endif //OPEN62541_NETWORKMANAGER_H
