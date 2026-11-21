/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef TESTING_TLS_H_
#define TESTING_TLS_H_
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/eventloop.h>
#include <check.h>
static UA_TlsConfig
testTlsConfig(const UA_ByteString *cert,
               const UA_ByteString *key, const UA_ByteString *trusted) {
    UA_TlsConfig config;
    memset(&config, 0, sizeof(config));
    if(cert) ck_assert_uint_eq(UA_ByteString_copy(cert, &config.certificateChain), UA_STATUSCODE_GOOD);
    if(key) ck_assert_uint_eq(UA_ByteString_copy(key, &config.privateKey), UA_STATUSCODE_GOOD);
    if(trusted) {
#ifdef UA_ENABLE_ENCRYPTION
        UA_TrustListDataType list;
        UA_TrustListDataType_init(&list);
        list.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
        list.trustedCertificates = (UA_ByteString*)(uintptr_t)trusted;
        list.trustedCertificatesSize = 1;
        UA_NodeId id = UA_NODEID_NUMERIC(0, 1);
        config.verification.mode = UA_TLSVERIFY_OWNED;
        ck_assert_uint_eq(UA_CertificateGroup_Memorystore(&config.verification.group.owned,
            &id, &list, UA_Log_Stdout, NULL), UA_STATUSCODE_GOOD);
#else
        ck_abort_msg("TLS group tests require a certificate verification provider");
#endif
    }
    return config;
}
#endif
