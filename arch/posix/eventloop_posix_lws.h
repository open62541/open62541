/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_EVENTLOOP_LIBWEBSOCKETS_H_
#define UA_EVENTLOOP_LIBWEBSOCKETS_H_

/* Include the open62541 platform configuration before system headers. */
#include "eventloop_posix.h"
#include <open62541/plugin/certificategroup.h>

#include <libwebsockets.h>

/* one of these is appended to each pt for our use */
struct pt_eventlibs_custom {
    UA_POSIXConnectionManager cm;
    UA_EventLoopPOSIX *io_loop;
    struct lws_context *context;
};

extern const lws_plugin_evlib_t evlib_open62541;
extern lws_log_cx_t open62541_log_cx;

void
UA_LWS_disableProtocolPlugins(struct lws_context_creation_info *info);

struct lws_context *
UA_LWS_acquireContext(UA_EventLoop *eventLoop);

void
UA_LWS_releaseContext(UA_EventLoop *eventLoop);

void
UA_LWS_requestWritable(struct lws *wsi);

static UA_INLINE void
UA_LWS_clearCertificateGroup(UA_ConnectionManager *cm) {
    if(!cm->certificateGroupOwned || !cm->certificateGroup)
        return;
    if(cm->certificateGroup->clear)
        cm->certificateGroup->clear(cm->certificateGroup);
    UA_free(cm->certificateGroup);
    cm->certificateGroup = NULL;
    cm->certificateGroupOwned = false;
}

static UA_INLINE UA_StatusCode
UA_LWS_verifyPeerCertificate(UA_ConnectionManager *cm, struct lws *wsi) {
    UA_CertificateGroup *cg = cm->certificateGroup;
    if(!cg)
        return UA_STATUSCODE_GOOD;
    if(!cg->verifyCertificate)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    union lws_tls_cert_info_results small;
    memset(&small, 0, sizeof(small));
    int res = lws_tls_peer_cert_info(wsi, LWS_TLS_CERT_INFO_DER_RAW,
                                     &small, sizeof(small.ns.name));
    size_t length = (small.ns.len > 0) ? (size_t)small.ns.len : 0;
    if(!res)
        length = (size_t)small.ns.len;
    if(length == 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    size_t allocSize = offsetof(union lws_tls_cert_info_results, ns.name) + length;
    union lws_tls_cert_info_results *der =
        (union lws_tls_cert_info_results*)UA_malloc(allocSize);
    if(!der)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(der, 0, allocSize);
    res = lws_tls_peer_cert_info(wsi, LWS_TLS_CERT_INFO_DER_RAW, der, length);
    UA_ByteString certificate = {length, (UA_Byte*)der->ns.name};
    UA_StatusCode retval = res ? UA_STATUSCODE_BADCERTIFICATEINVALID :
        cg->verifyCertificate(cg, &certificate);
    UA_free(der);
    return retval;
}

#endif /* UA_EVENTLOOP_LIBWEBSOCKETS_H_ */
