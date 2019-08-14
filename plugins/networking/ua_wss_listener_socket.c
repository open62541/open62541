/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/plugin/socket.h>
#include <libwebsockets.h>
#include <open62541/plugin/networkmanager.h>
#include <open62541/types_generated_handling.h>
#include <open62541/plugin/networking/sockets.h>
#include "open62541_queue.h"
#include "wss_private.h"

typedef struct Opening_WSS_Sock_entry {
    UA_WSS_Socket *socket;
    LIST_ENTRY(Opening_WSS_Sock_entry) pointers;
} Opening_WSS_Sock_entry;

typedef struct Opening_WSS_Sock_List {
    LIST_HEAD(, Opening_WSS_Sock_entry) list;
} Opening_WSS_Sock_List;

typedef struct {
    UA_WSS_Socket socket;

    UA_UInt32 recvBufferSize;
    UA_UInt32 sendBufferSize;
    UA_SocketCallbackFunction onAccept;
    struct lws_context *lwsContext;
    Opening_WSS_Sock_List openingSockets;
    UA_Boolean closeOnNextActivity;
} UA_Socket_wssListener;

static UA_StatusCode
wss_open(UA_Socket *sock) {
    if(sock->socketState != UA_SOCKETSTATE_NEW) {
        UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Calling open on already open socket not supported");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    sock->socketState = UA_SOCKETSTATE_OPEN;

    sock->waitForWriteActivity = true;
    sock->waitForReadActivity = true;

    return UA_SocketCallback_call(sock->openCallback, sock);
}

static UA_StatusCode
wss_close(UA_Socket *socket) {
    UA_LOG_DEBUG(socket->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Closing wss listener socket with id %lu", socket->id);
    UA_Socket_wssListener *const internalSock = (UA_Socket_wssListener *const)socket;
    internalSock->closeOnNextActivity = true;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
wss_mayDelete(UA_Socket *socket) {
    return socket->socketState == UA_SOCKETSTATE_CLOSED;
}

static UA_StatusCode
wss_free(UA_Socket *socket) {
    UA_LOG_DEBUG(socket->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Deleting wss listener socket with id %lu", socket->id);

    UA_String_deleteMembers(&socket->discoveryUrl);

    UA_StatusCode retval = UA_SocketCallback_call(socket->freeCallback, socket);
    UA_Socket_wssListener *const internalSocket = (UA_Socket_wssListener *const)socket;
    lws_context_destroy(internalSocket->lwsContext);
    UA_free(socket);

    return retval;
}

static UA_StatusCode
wss_activity(UA_Socket *sock, UA_Boolean readActivity, UA_Boolean writeActivity) {
    if(sock == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Socket_wssListener *const internalSock = (UA_Socket_wssListener *const)sock;
    struct lws_pollfd pollfd;
    pollfd.fd = (int)sock->id;
    pollfd.events =
        (short)((sock->waitForReadActivity ? LWS_POLLIN : 0) | (sock->waitForWriteActivity ? LWS_POLLOUT : 0));
    pollfd.revents = (short)((readActivity ? LWS_POLLIN : 0) | (writeActivity ? LWS_POLLOUT : 0));

    lws_service_fd(internalSock->lwsContext, &pollfd);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
wss_send(UA_Socket *socket, UA_ByteString *buffer) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
wss_acquireSendBuffer(UA_Socket *socket, size_t bufferSize, UA_ByteString **p_buffer) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
wss_releaseSendBuffer(UA_Socket *socket, UA_ByteString *buffer) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
wss_create_callback(UA_Socket_wssListener *sock, UA_WSS_Socket *newSock) {
    Opening_WSS_Sock_entry *const newEntry = (Opening_WSS_Sock_entry *const)UA_malloc(sizeof(Opening_WSS_Sock_entry));
    if(newEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    newEntry->socket = newSock;
    LIST_INSERT_HEAD(&sock->openingSockets.list, newEntry, pointers);
    return UA_STATUSCODE_GOOD;
}

static UA_WSS_Socket *
wss_get_opening_sock_by_id(UA_Socket_wssListener *sock, UA_UInt64 id) {
    Opening_WSS_Sock_entry *entry = NULL;
    LIST_FOREACH(entry, &sock->openingSockets.list, pointers) {
        if(entry->socket->socket.id == id)
            return entry->socket;
    }

    return NULL;
}

static void
wss_remove_opening_sock(UA_Socket_wssListener *sock, UA_WSS_Socket *sockToRemove) {
    Opening_WSS_Sock_entry *entry = NULL, *tmp = NULL;
    LIST_FOREACH_SAFE(entry, &sock->openingSockets.list, pointers, tmp) {
        if(entry->socket == sockToRemove)
            LIST_REMOVE(entry, pointers);
    }
}

static int
callback_opcua(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    UA_Socket_wssListener *listenerSock = (UA_Socket_wssListener *)lws_get_vhost_user(
        (struct lws_vhost *)lws_get_vhost(wsi));
    UA_WSS_Socket *currentSocket = NULL;
    UA_UInt64 id = (UA_UInt64)lws_get_socket_fd(wsi);
    if(id == listenerSock->socket.socket.id) {
        currentSocket = (UA_WSS_Socket *)listenerSock;
    } else if(user == NULL) {
        currentSocket = wss_get_opening_sock_by_id(listenerSock, id);
    } else {
        currentSocket = *(UA_WSS_Socket **)user;
    }

    switch(reason) {
    case LWS_CALLBACK_ADD_POLL_FD: {
        UA_UInt64 socketFD = (UA_UInt64)lws_get_socket_fd(wsi);
        if(listenerSock->socket.socket.socketState == UA_SOCKETSTATE_NEW && listenerSock->socket.socket.id == 0)
            listenerSock->socket.socket.id = socketFD;
        if(listenerSock->socket.socket.socketState == UA_SOCKETSTATE_OPEN &&
           listenerSock->socket.socket.id != socketFD) {
            UA_WSS_DataSocket_AcceptFrom_AdditionalParameters additionalParameters;
            additionalParameters.fd = socketFD;
            additionalParameters.lwsContext = listenerSock->lwsContext;
            additionalParameters.listenerSocket = (UA_Socket *)listenerSock;
            additionalParameters.createCallback = (WSS_CreateCallback)wss_create_callback;
            additionalParameters.wsi = wsi;

            UA_SocketConfig socketConfig;
            socketConfig.recvBufferSize = listenerSock->recvBufferSize;
            socketConfig.sendBufferSize = listenerSock->sendBufferSize;
            socketConfig.networkManager = listenerSock->socket.socket.networkManager;
            socketConfig.createSocket = UA_WSS_DataSocket_AcceptFrom;
            socketConfig.additionalParameters = &additionalParameters;
            socketConfig.application = listenerSock->socket.socket.application;

            UA_StatusCode retval = socketConfig.networkManager->createSocket(listenerSock->socket.socket.networkManager,
                                                                             &socketConfig,
                                                                             listenerSock->onAccept);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(listenerSock->socket.socket.networkManager->logger, UA_LOGCATEGORY_NETWORK,
                             "Error while accepting new socket connection: %s",
                             UA_StatusCode_name(retval));
            }
        }
        break;
    }

    case LWS_CALLBACK_DEL_POLL_FD: {
        if(currentSocket == NULL) {
            UA_LOG_ERROR(listenerSock->socket.socket.networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Unexpected error");
            return -1;
        }
        currentSocket->socket.close((UA_Socket *)currentSocket);
        break;
    }

    case LWS_CALLBACK_CHANGE_MODE_POLL_FD: {
        struct lws_pollargs *pollargs = (struct lws_pollargs *)in;

        if(currentSocket != NULL) {
            currentSocket->socket.waitForWriteActivity = (pollargs->events & LWS_POLLOUT) ? true : false;
            currentSocket->socket.waitForReadActivity = (pollargs->events & LWS_POLLIN) ? true : false;
        }
        break;
    }

    case LWS_CALLBACK_ESTABLISHED: {
        // TODO: call open on new data socket to signal that the socket is now ready to send and receive
        if(currentSocket == NULL) {

            *(UA_WSS_Socket **)user = wss_get_opening_sock_by_id(listenerSock, id);
            currentSocket = *(UA_WSS_Socket **)user;
            wss_remove_opening_sock(listenerSock, currentSocket);
        }
        if(currentSocket == NULL) {
            UA_LOG_ERROR(listenerSock->socket.socket.networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Unexpected error");
            return -1;
        }
        break;
    }

    case LWS_CALLBACK_CLOSED: {
        currentSocket->socket.socketState = UA_SOCKETSTATE_CLOSED;
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE: {
        if(currentSocket != NULL && currentSocket != (UA_WSS_Socket *)listenerSock) {
            if(UA_WSS_DSock_handleWritable(currentSocket) != UA_STATUSCODE_GOOD)
                return -1;
        }
        break;
    }

    case LWS_CALLBACK_RECEIVE: {
        if(currentSocket == NULL || currentSocket->socket.isListener)
            break;

        UA_ByteString message = {len, (UA_Byte *)in};
        currentSocket->socket.dataCallback(&message, (UA_Socket *)currentSocket);
        break;
    }

    default:return 0;
    }

    return 0;
}

static int
callback_opcua_wrap(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    int retval = callback_opcua(wsi, reason, user, in, len);
    UA_Socket_wssListener *listenerSock = (UA_Socket_wssListener *)lws_get_vhost_user(
        (struct lws_vhost *)lws_get_vhost(wsi));
    UA_WSS_Socket *currentSocket = NULL;
    UA_UInt64 id = (UA_UInt64)lws_get_socket_fd(wsi);
    if(id == listenerSock->socket.socket.id) {
        currentSocket = (UA_WSS_Socket *)listenerSock;
    } else if(user == NULL) {
        currentSocket = wss_get_opening_sock_by_id(listenerSock, id);
        if(currentSocket != NULL && currentSocket->closeOnNextCallback) {
            currentSocket->closeOnNextCallback = false;
            currentSocket->socket.socketState = UA_SOCKETSTATE_CLOSED;
            wss_remove_opening_sock(listenerSock, currentSocket);
            return -1;
        }
    } else {
        currentSocket = *(UA_WSS_Socket **)user;
    }
    if(currentSocket != NULL && currentSocket->closeOnNextCallback) {
        currentSocket->closeOnNextCallback = false;
        return -1;
    }

    return retval;
}

static struct lws_protocols protocols[] = {
    {"opcua", callback_opcua_wrap, sizeof(UA_Socket *), 0, 0, NULL, 0},
    {NULL, NULL,                   0,                   0, 0, NULL, 0}
};

const struct lws_protocol_vhost_options pvo = {NULL,
                                               NULL,
                                               "opcua",
                                               ""};

UA_StatusCode
UA_WSS_ListenerSocket(const UA_SocketConfig *socketConfig, UA_SocketCallbackFunction const creationCallback) {
    UA_Socket_wssListener *const sock = (UA_Socket_wssListener *const)UA_malloc(sizeof(UA_Socket_wssListener));
    memset(sock, 0, sizeof(UA_Socket_wssListener));

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    sock->socket.socket.open = wss_open;
    sock->socket.socket.close = wss_close;
    sock->socket.socket.mayDelete = wss_mayDelete;
    sock->socket.socket.free = wss_free;
    sock->socket.socket.activity = wss_activity;
    sock->socket.socket.send = wss_send;
    sock->socket.socket.acquireSendBuffer = wss_acquireSendBuffer;
    sock->socket.socket.releaseSendBuffer = wss_releaseSendBuffer;

    sock->socket.socket.networkManager = socketConfig->networkManager;
    sock->socket.socket.waitForWriteActivity = false;
    sock->socket.socket.waitForReadActivity = false;
    sock->socket.socket.isListener = true;
    sock->socket.socket.application = socketConfig->application;

    sock->recvBufferSize = socketConfig->recvBufferSize;
    sock->sendBufferSize = socketConfig->sendBufferSize;
    sock->onAccept = ((const UA_ListenerSocketConfig *)socketConfig)->onAccept;

    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    char discoveryUrlBuffer[256];
    if(socketConfig->customHostname.length) {
        du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "ws://%.*s:%d/",
                                        (int)socketConfig->customHostname.length,
                                        socketConfig->customHostname.data,
                                        socketConfig->port);
        du.data = (UA_Byte *)discoveryUrlBuffer;
    } else {
        char hostnameBuffer[256];
        if(UA_gethostname(hostnameBuffer, 255) == 0) {
            du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "ws://%s:%d/",
                                            hostnameBuffer, socketConfig->port);
            du.data = (UA_Byte *)discoveryUrlBuffer;
        } else {
            UA_LOG_ERROR(socketConfig->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Could not get the hostname");
        }
    }
    // null terminate
    du.length += 1;
    UA_String_copy(&du, &sock->socket.socket.discoveryUrl);
    // remove null byte from ua string. memory still has null byte
    sock->socket.socket.discoveryUrl.length -= 1;

    UA_LOG_INFO(socketConfig->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                "Websocket network layer listening on %.*s",
                (int)sock->socket.socket.discoveryUrl.length,
                sock->socket.socket.discoveryUrl.data);

    struct lws_context_creation_info info;
    int logLevel = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG;
    lws_set_log_level(logLevel, NULL);
    memset(&info, 0, sizeof(info));
    info.port = socketConfig->port;
    info.protocols = protocols;
    info.vhost_name = (char *)sock->socket.socket.discoveryUrl.data;
    info.ws_ping_pong_interval = 10;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    info.pvo = &pvo;
    info.user = sock;

    struct lws_context *context = lws_create_context(&info);
    if(!context) {
        UA_LOG_ERROR(socketConfig->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "lws init failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    sock->lwsContext = context;

    struct lws_vhost *vhost;
    vhost = lws_create_vhost(sock->lwsContext, &info);
    if(!vhost) {
        UA_LOG_ERROR(sock->socket.socket.networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "vhost creation failed\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    retval = UA_SocketCallback_call(socketConfig->networkManagerCallback, (UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(socketConfig->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Error calling socket callback %s",
                     UA_StatusCode_name(retval));
    return UA_SocketCallback_call(creationCallback, (UA_Socket *)sock);
}
