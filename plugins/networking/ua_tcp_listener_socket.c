/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_sockets.h"
#include "ua_types.h"

UA_StatusCode
UA_TCP_ListenerSocketFromAddrinfo(struct addrinfo addrinfo,
                                  UA_DataSocketFactory *dataSocketFactory,
                                  UA_Logger *logger,
                                  UA_Socket **p_socket) {
    UA_Socket *socket = (UA_Socket *)UA_malloc(sizeof(UA_Socket));
    if(socket == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    
    return 0;
}


UA_StatusCode
UA_TCP_ListenerSockets(UA_UInt32 port,
                       UA_DataSocketFactory *dataSocketFactory,
                       UA_Logger *logger,
                       UA_Socket *sockets[]) {
    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    char discoveryUrlBuffer[256];
    char hostnameBuffer[256];
    UA_ByteString *customHostname = NULL;
    if(customHostname->length) {
        du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "opc.tcp://%.*s:%d/",
                                        (int)customHostname->length,
                                        customHostname->data,
                                        port);
        du.data = (UA_Byte *)discoveryUrlBuffer;
    } else {
        if(UA_gethostname(hostnameBuffer, 255) == 0) {
            du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "opc.tcp://%s:%d/",
                                            hostnameBuffer, port);
            du.data = (UA_Byte *)discoveryUrlBuffer;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK, "Could not get the hostname");
        }
    }
    // UA_String_copy(&du, &nl->discoveryUrl);

    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK, "%s", du.data);

    return UA_STATUSCODE_GOOD;
}
