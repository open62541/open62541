/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/plugin/socket.h>

/**
 * This is a private header for the lws plugin.
 * Do not include it directly, as it is only needed by the lws plugin source files to share some common data.
 */

typedef UA_StatusCode (*WSS_CreateCallback)(UA_Socket *listenSock, UA_Socket *newSock);

typedef struct {
    UA_Socket socket;
    UA_Boolean closeOnNextCallback;
} UA_WSS_Socket;

typedef struct {
    UA_UInt64 fd;
    void *lwsContext;
    UA_Socket *listenerSocket;
    WSS_CreateCallback createCallback;
    void *wsi;
} UA_WSS_DataSocket_AcceptFrom_AdditionalParameters;

UA_StatusCode
UA_WSS_DSock_handleWritable(UA_WSS_Socket *socket);
