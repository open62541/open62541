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

typedef struct {
    UA_RegisteredFD rfd;

    UA_ConnectionManager_connectionCallback applicationCB;
    void *application;
    void *context;
}LWS_FD;

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
        eventfd.events = LWS_POLLIN;
        eventfd.revents =  LWS_POLLIN;
        lws_service_fd((struct lws_context*)conn->context, &eventfd);
    }
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
    newConn->applicationCB = NULL;
    newConn->application = NULL;
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

    conn->rfd.listenEvents = 0;
    if(flags & LWS_EV_START) {
        if(flags & LWS_EV_WRITE)
            conn->rfd.listenEvents |= UA_FDEVENT_OUT;
        if(flags & LWS_EV_READ)
            conn->rfd.listenEvents |= UA_FDEVENT_IN;
    } else {
        if(flags & LWS_EV_WRITE)
            conn->rfd.listenEvents |= UA_FDEVENT_IN;
        if(flags & LWS_EV_READ)
            conn->rfd.listenEvents |= UA_FDEVENT_IN;
    }

    UA_EventLoopPOSIX_modifyFD(el, &conn->rfd);
    UA_UNLOCK(&el->elMutex);
}

static void
delayedClose(void *application, void *context) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)application;
    LWS_FD *conn = (LWS_FD*)context;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "HTTP %u\t| Delayed closing of the connection",
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
