/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

/* Suppress the warning 'redefinition of typedef' in the libwebsockets library.
 * Fixed with the following commit and included in version 4.4.0 and later (https://github.com/warmcat/libwebsockets/commit/48e09ddf51ea014a60d666f8eb780cd801c0d4b5). */
#pragma GCC diagnostic ignored "-Wpedantic"

#include "eventloop_posix_lws.h"

#include <open62541/plugin/log_stdout.h>

#if defined(LWS_WITH_TLS) && !defined(LWS_WITH_MBEDTLS) && \
    !defined(USE_WOLFSSL) && !defined(LWS_WITH_BORINGSSL) && \
    !defined(LWS_WITH_AWSLC)
# define UA_LWS_USE_OPENSSL
#endif

/* Creating and destroying an lws context accesses process-global state in
 * libwebsockets. Initialize the recursive mutex once at runtime because a
 * portable static recursive-mutex initializer is not available. */
static pthread_once_t lwsLifecycleMutexOnce = PTHREAD_ONCE_INIT;
static UA_Lock lwsLifecycleMutex;
static size_t lwsContextCount;

#ifdef UA_LWS_USE_OPENSSL
/* OpenSSL cannot be initialized again after OPENSSL_cleanup(). Keep one client
 * SSL context alive for the process as a lifetime safeguard. */
static SSL_CTX *lwsClientSslContext;
#endif

static void
initializeLwsLifecycleMutex(void) {
    UA_LOCK_INIT(&lwsLifecycleMutex);
}

static void
lockLwsLifecycle(void) {
    int res = pthread_once(&lwsLifecycleMutexOnce, initializeLwsLifecycleMutex);
    UA_assert(res == 0);
    UA_LOCK(&lwsLifecycleMutex);
}

static void
unlockLwsLifecycle(void) {
    UA_UNLOCK(&lwsLifecycleMutex);
}

#ifdef UA_LWS_USE_OPENSSL
static UA_Boolean
initializeLwsClientSslContext(void) {
    if(lwsClientSslContext)
        return true;

#if defined(LWS_HAVE_TLS_CLIENT_METHOD)
    const SSL_METHOD *method = TLS_client_method();
#else
    const SSL_METHOD *method = SSLv23_client_method();
#endif
    if(!method)
        return false;

    lwsClientSslContext = SSL_CTX_new(method);
    if(!lwsClientSslContext)
        return false;

#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(lwsClientSslContext, SSL_OP_NO_COMPRESSION);
#endif
    SSL_CTX_set_options(lwsClientSslContext, SSL_OP_CIPHER_SERVER_PREFERENCE);
    SSL_CTX_set_mode(lwsClientSslContext, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                     SSL_MODE_RELEASE_BUFFERS);
    SSL_CTX_set_default_verify_paths(lwsClientSslContext);
    return true;
}
#endif

typedef struct {
    UA_RegisteredFD rfd;
    void *context;
}LWS_FD;

static void
io_custom(struct lws *wsi, unsigned int flags);

static void
connectionCallback(UA_ConnectionManager *cm, LWS_FD *conn, short event) {
    struct lws_pollfd eventfd;
    eventfd.fd = conn->rfd.fd;
    eventfd.events = 0;
    eventfd.revents = 0;

    if(event == UA_FDEVENT_ERR) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_EVENTLOOP,
                     "Error in Connection Callback for FD %d", conn->rfd.fd);
    }
    if(event == UA_FDEVENT_OUT) {
        eventfd.events = LWS_POLLOUT;
        eventfd.revents =  LWS_POLLOUT;
        lws_service_fd((struct lws_context*)conn->context, &eventfd);
    }
    if(event == UA_FDEVENT_IN) {
        eventfd.events = LWS_POLLIN;
        eventfd.revents =  LWS_POLLIN;
        lws_service_fd((struct lws_context*)conn->context, &eventfd);
    }
    /* Check if the connection requires forced service */
    while(!lws_service_adjust_timeout((struct lws_context*)conn->context, 1, 0)) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_EVENTLOOP,
                     "Process connection with pending work.");
        /* A timeout of -1 services only logical connections that LWS has
         * marked for forced service. This includes multiplexed MQTT streams,
         * which do not own a socket and therefore cannot be serviced by
         * synthesizing an event for the network fd. */
        lws_service_tsi((struct lws_context*)conn->context, -1, 0);
    }
}

void
UA_LWS_requestWritable(struct lws *wsi) {
    lws_callback_on_writable(wsi);

    /* LWS does not notify a custom event library when a multiplexed stream
     * requests a writable callback. Explicitly enable write events on the
     * underlying network WSI. */
    struct lws *networkWsi = lws_get_network_wsi(wsi);
    io_custom(networkWsi, LWS_EV_START | LWS_EV_WRITE);
}

/*
 * These is the custom "event library" interface layer between lws event lib
 * support and the custom loop implementation above.  We only need to support
 * a few key apis.
 *
 * We are user code, so all the internal lws objects are opaque.  But there are
 * enough public helpers to get everything done.
 *
 * During lws context creation, we get called with the foreign loop pointer
 * that was passed in the creation info struct.  Stash it in our private part
 * of the pt, so we can reference it in the other callbacks subsequently.
 */

static int
init_pt_custom(struct lws_context *cx, void *_loop, int tsi) {
    struct pt_eventlibs_custom *priv = (struct pt_eventlibs_custom *)
        lws_evlib_tsi_to_evlib_pt(cx, tsi);

    /* store the loop we are bound to in our private part of the pt */
    priv->io_loop = ( UA_EventLoopPOSIX *)_loop;
    priv->context = cx;
    return 0;
}

static int
sock_accept_custom(struct lws *wsi) {
    struct pt_eventlibs_custom *priv = (struct pt_eventlibs_custom *)
        lws_evlib_wsi_to_evlib_pt(wsi);
    UA_EventLoopPOSIX *el = priv->io_loop;
    UA_LOCK(&el->elMutex);

    lws_sockfd_type fd =  lws_get_socket_fd(wsi);
    LWS_FD *conn = (LWS_FD*)ZIP_FIND(UA_FDTree, &priv->cm.fds, &fd);
    if(conn) { /* File descriptor may already be registered. */
        UA_UNLOCK(&el->elMutex);
        return 0;
    }
    /* Allocate the UA_RegisteredFD */
    LWS_FD *newConn = (LWS_FD*)UA_calloc(1, sizeof(LWS_FD));
    if(!newConn) {
        UA_LOG_WARNING(priv->io_loop->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "LWS %u\t| Error allocating memory for the socket",
                       (unsigned)fd);
        UA_UNLOCK(&el->elMutex);
        return (int)UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newConn->rfd.fd = fd;
    newConn->rfd.listenEvents = UA_FDEVENT_IN;
    newConn->rfd.es = NULL;
    newConn->rfd.eventSourceCB = (UA_FDCallback)connectionCallback;
    newConn->context = (void*)priv->context;

    UA_StatusCode retval = UA_EventLoopPOSIX_registerFD(el, &newConn->rfd);
    if(retval == UA_STATUSCODE_GOOD) {
        ZIP_INSERT(UA_FDTree, &priv->cm.fds, &newConn->rfd);
        priv->cm.fdsSize++;
    } else {
        UA_free(newConn);
    }

    UA_UNLOCK(&el->elMutex);
    return (int)retval;
}

static void
io_custom(struct lws *wsi, unsigned int flags) {
    struct pt_eventlibs_custom *priv = (struct pt_eventlibs_custom *)
        lws_evlib_wsi_to_evlib_pt(wsi);
    UA_EventLoopPOSIX *el = priv->io_loop;
    UA_LOCK(&el->elMutex);

    lws_sockfd_type fd =  lws_get_socket_fd(wsi);
    LWS_FD *conn = (LWS_FD*)ZIP_FIND(UA_FDTree, &priv->cm.fds, &fd);
    if(!conn) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    if(flags & LWS_EV_START) {
        if(flags & LWS_EV_WRITE)
            conn->rfd.listenEvents |= UA_FDEVENT_OUT;
        if(flags & LWS_EV_READ)
            conn->rfd.listenEvents |= UA_FDEVENT_IN;
    } else {
        if(flags & LWS_EV_WRITE)
            conn->rfd.listenEvents &= (short)~UA_FDEVENT_OUT;
        if(flags & LWS_EV_READ)
            conn->rfd.listenEvents &= (short)~UA_FDEVENT_IN;
    }

    UA_EventLoopPOSIX_modifyFD(el, &conn->rfd);
    UA_UNLOCK(&el->elMutex);
}

static void
delayedClose(void *application, void *context) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)application;
    LWS_FD *conn = (LWS_FD*)context;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "LWS %u\t| Delayed closing of the connection",
                 (unsigned)conn->rfd.fd);
    UA_free(conn);
    conn = NULL;
}

static void
deregisterLWSFD(struct pt_eventlibs_custom *priv, LWS_FD *conn) {
    UA_EventLoopPOSIX *el = priv->io_loop;
    UA_EventLoopPOSIX_deregisterFD(el, &conn->rfd);

    UA_DelayedCallback *dc = &conn->rfd.dc;
    dc->callback = delayedClose;
    dc->application = el;
    dc->context = conn;

    /* Adding a delayed callback does not take a lock */
    UA_EventLoopPOSIX_addDelayedCallback((UA_EventLoop*)el, dc);
}

static int
wsi_logical_close_custom(struct lws *wsi) {
    struct pt_eventlibs_custom *priv = (struct pt_eventlibs_custom *)
        lws_evlib_wsi_to_evlib_pt(wsi);
    UA_EventLoopPOSIX *el = priv->io_loop;
    UA_LOCK(&el->elMutex);
    lws_sockfd_type fd =  lws_get_socket_fd(wsi);
    LWS_FD *conn = (LWS_FD*)ZIP_FIND(UA_FDTree, &priv->cm.fds, &fd);
    if(!conn) {
        UA_UNLOCK(&el->elMutex);
        return 0;
    }

    deregisterLWSFD(priv, conn);
    ZIP_REMOVE(UA_FDTree, &priv->cm.fds, &conn->rfd);
    priv->cm.fdsSize--;

    UA_UNLOCK(&el->elMutex);
    return (int)UA_STATUSCODE_GOOD;
}

static void *
destroyLWSFD(void *context, UA_RegisteredFD *rfd) {
    struct pt_eventlibs_custom *priv = (struct pt_eventlibs_custom*)context;
    deregisterLWSFD(priv, (LWS_FD*)rfd);
    return NULL;
}

static void
destroy_pt_custom(struct lws_context *context, int tsi) {
    struct pt_eventlibs_custom *priv = (struct pt_eventlibs_custom *)
        lws_evlib_tsi_to_evlib_pt(context, tsi);
    UA_EventLoopPOSIX *el = priv->io_loop;
    UA_LOCK(&el->elMutex);

    /* The internal event pipe does not receive a wsi_logical_close callback. */
    ZIP_ITER(UA_FDTree, &priv->cm.fds, destroyLWSFD, priv);
    ZIP_INIT(&priv->cm.fds);
    priv->cm.fdsSize = 0;

    UA_UNLOCK(&el->elMutex);
}

static const struct lws_event_loop_ops event_loop_ops_custom = {
    .name = "custom",
    .init_pt = init_pt_custom,
    .init_vhost_listen_wsi = sock_accept_custom,
    .sock_accept = sock_accept_custom,
    .io = io_custom,
    .wsi_logical_close = wsi_logical_close_custom,
    .destroy_pt = destroy_pt_custom,
    .evlib_size_pt = sizeof(struct pt_eventlibs_custom)
};

const lws_plugin_evlib_t evlib_open62541 = {
    .hdr = {
        "custom event loop",
        "lws_evlib_plugin",
        LWS_BUILD_HASH,
        LWS_PLUGIN_API_MAGIC
    },
    .ops = &event_loop_ops_custom
};

#if defined(LWS_WITH_PLUGINS) && !defined(LWS_WITH_PLUGINS_BUILTIN)

/* libwebsockets reserves eight local entries for plugin directories: seven
 * directories followed by NULL. Filling all directory entries prevents it
 * from appending and scanning its compiled-in LWS_PLUGIN_DIR. open62541 does
 * not use dynamically loaded lws protocol plugins. */
# define UA_LWS_PLUGIN_DIR_SLOTS 7
static const char *const noPluginDirs[UA_LWS_PLUGIN_DIR_SLOTS + 1] = {
    "/dev/null", "/dev/null", "/dev/null", "/dev/null",
    "/dev/null", "/dev/null", "/dev/null", NULL
};

void
UA_LWS_disableProtocolPlugins(struct lws_context_creation_info *info) {
    info->plugin_dirs = noPluginDirs;
}

#else

void
UA_LWS_disableProtocolPlugins(struct lws_context_creation_info *info) {
    (void)info;
}

#endif

/* To integrate libwebsockets logging with open62541 logging. */
static void
open62541_log_emit_cx(struct lws_log_cx *cx, int level, const char *line,
                     size_t len) {

    const UA_Logger *logger = UA_Log_Stdout;

    if(level == LLL_NOTICE)
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK, "%s", line);
    else if(level == LLL_USER)
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_USERLAND, "%s", line);
    else if(level == LLL_WARN)
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK, "%s", line);
    else if(level == LLL_ERR)
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK, "%s", line);
    else
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK, "%s", line);
}

lws_log_cx_t open62541_log_cx = {
    .lll_flags = LLLF_LOG_CONTEXT_AWARE | LLL_ERR |
                 LLL_WARN | LLL_NOTICE | LLL_USER,
    .u.emit_cx = open62541_log_emit_cx,
};

struct lws_context *
UA_LWS_acquireContext(UA_EventLoop *eventLoop) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)eventLoop;

    lockLwsLifecycle();
    if(!el->lwsContext) {
        struct lws_context_creation_info info;
        memset(&info, 0, sizeof(info));
        info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
#ifdef UA_LWS_USE_OPENSSL
        if(!initializeLwsClientSslContext()) {
            unlockLwsLifecycle();
            return NULL;
        }
        info.provided_client_ssl_ctx = lwsClientSslContext;
#else
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.log_cx = &open62541_log_cx;
        info.event_lib_custom = &evlib_open62541;
        el->lwsForeignLoop = &el->eventLoop;
        info.foreign_loops = (void**)&el->lwsForeignLoop;
        UA_LWS_disableProtocolPlugins(&info);

        el->lwsContext = lws_create_context(&info);
        if(el->lwsContext)
            ++lwsContextCount;
    }

    if(el->lwsContext)
        ++el->lwsContextUsers;
    struct lws_context *context = (struct lws_context*)el->lwsContext;
    unlockLwsLifecycle();
    return context;
}

void
UA_LWS_releaseContext(UA_EventLoop *eventLoop) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)eventLoop;

    lockLwsLifecycle();
    UA_assert(el->lwsContextUsers > 0);
    --el->lwsContextUsers;
    if(el->lwsContextUsers == 0) {
        struct lws_context *context = (struct lws_context*)el->lwsContext;
        el->lwsContext = NULL;
        lws_context_destroy(context);
        UA_assert(lwsContextCount > 0);
        --lwsContextCount;
    }
    unlockLwsLifecycle();
}
