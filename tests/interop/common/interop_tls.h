/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef INTEROP_TLS_H_
#define INTEROP_TLS_H_

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/certificategroup_default.h>

#ifdef UA_ENABLE_LWS
/* Each interop transport owns its TLS identity and its independent trust store. */
static UA_StatusCode
interopConfigureTls(UA_EventLoop *el, UA_Boolean http,
                    const UA_ByteString *certificate, const UA_ByteString *key,
                    const UA_ByteString *trusted) {
    UA_TlsConfig tls = {0};
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(certificate)
        res = UA_ByteString_copy(certificate, &tls.certificateChain);
    if(res == UA_STATUSCODE_GOOD && key) {
#ifdef UA_ENABLE_ENCRYPTION
        res = UA_CertificateUtils_decryptPrivateKey(*key, UA_STRING_NULL, &tls.privateKey);
#else
        res = UA_ByteString_copy(key, &tls.privateKey);
#endif
    }
    if(res == UA_STATUSCODE_GOOD && trusted) {
#ifdef UA_ENABLE_ENCRYPTION
        UA_TrustListDataType list;
        UA_TrustListDataType_init(&list);
        list.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
        list.trustedCertificates = (UA_ByteString*)(uintptr_t)trusted;
        list.trustedCertificatesSize = 1;
        UA_NodeId id = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTHTTPSGROUP);
        tls.verification.mode = UA_TLSVERIFY_OWNED;
        res = UA_CertificateGroup_Memorystore(&tls.verification.group.owned,
                                              &id, &list, el->logger, NULL);
#else
        res = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    if(res == UA_STATUSCODE_GOOD) {
        UA_ConnectionManager *cm = http ?
            UA_ConnectionManager_new_HTTP(UA_STRING("interop-https"), &tls) :
            UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("interop-wss"), &tls);
        if(cm) {
            res = el->registerEventSource(el, &cm->eventSource);
            if(res != UA_STATUSCODE_GOOD)
                cm->eventSource.free(&cm->eventSource);
        } else {
            res = UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
    }
    UA_TlsConfig_clear(&tls);
    return res;
}
#endif
#endif /* INTEROP_TLS_H_ */
