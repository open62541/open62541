/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_EVENTLOOP_LIBWEBSOCKETS_H_
#define UA_EVENTLOOP_LIBWEBSOCKETS_H_

/* Include the open62541 platform configuration before system headers. */
#include "eventloop_posix.h"

#include <libwebsockets.h>

/* LWS 5.x adds trailing padding to lws_context_creation_info only outside
 * strict ANSI mode. Most binary packages are built in a GNU dialect. Building
 * this adapter in strict mode would therefore make the public structure eight
 * bytes smaller than the structure expected by the library. CMake compiles the
 * LWS adapter sources as GNU99; keep other build systems from silently
 * producing an ABI mismatch. */
#if LWS_LIBRARY_VERSION_NUMBER >= 5000000 && defined(__STRICT_ANSI__)
# error "libwebsockets 5.x adapters must be compiled in a non-strict C dialect"
#endif

/* one of these is appended to each pt for our use */
struct pt_eventlibs_custom {
    UA_POSIXConnectionManager cm;
    UA_EventLoopPOSIX *io_loop;
    struct lws_context *context;
    UA_DelayedCallback forcedServiceCallback;
    UA_Boolean forcedServicePending;
};

extern const lws_plugin_evlib_t evlib_open62541;

void
UA_LWS_disableProtocolPlugins(struct lws_context_creation_info *info);

void
UA_LWS_useContextLogger(struct lws_context_creation_info *info,
                        struct lws_context *context);

void
UA_LWS_clearActiveLogger(void);

struct lws_context *
UA_LWS_acquireContext(UA_EventLoop *eventLoop);

void
UA_LWS_releaseContext(UA_EventLoop *eventLoop);

void
UA_LWS_requestWritable(struct lws *wsi);

typedef struct {
    size_t users;
    UA_TlsConfig config;
} UA_LWSTlsState;

typedef enum {
    UA_TLSCLIENTAUTH_NONE = 0,
    UA_TLSCLIENTAUTH_OPTIONAL,
    UA_TLSCLIENTAUTH_REQUIRED
} UA_LWSTlsClientAuth;

typedef struct {
    UA_Boolean verifyHostname;
    UA_LWSTlsClientAuth clientAuth;
} UA_LWSTlsOptions;

typedef struct UA_LWSTlsBinding {
    struct UA_LWSTlsBinding *next;
    struct lws_vhost *vhost;
    UA_LWSTlsState *context;
    UA_StatusCode status;
    UA_LWSTlsOptions options;
    void (*verificationResult)(struct lws *wsi, UA_StatusCode status);
    UA_Boolean listener;
} UA_LWSTlsBinding;

UA_StatusCode UA_LWS_initTlsConfig(UA_LWSTlsState *state, UA_TlsConfig *config);
UA_StatusCode UA_LWS_clearTlsConfig(UA_LWSTlsState *state);
UA_StatusCode UA_LWS_validateTls(UA_LWSTlsState *state,
                                UA_Boolean listener, UA_Boolean tls,
                                const UA_KeyValueMap *params, UA_LWSTlsOptions *options);
void UA_LWS_configureTls(struct lws_context_creation_info *info,
                        UA_LWSTlsState *context, UA_Boolean listener,
                        const UA_LWSTlsOptions *options);
void UA_LWS_bindTls(UA_EventLoop *el, UA_LWSTlsBinding *binding,
                    struct lws_vhost *vhost, UA_LWSTlsState *context,
                    UA_Boolean listener);
void UA_LWS_unbindTls(UA_EventLoop *el, UA_LWSTlsBinding *binding);
int UA_LWS_tlsCallback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len);
int UA_LWS_verifyTls(struct lws *wsi, void *storeContext, void *ssl, int preverify);

#endif /* UA_EVENTLOOP_LIBWEBSOCKETS_H_ */
