/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */
#include <open62541/plugin/eventloop.h>
#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include "testing-plugins/testing_tls.h"

typedef struct {
    unsigned verified, cleared;
    UA_StatusCode result;
    const UA_ByteString *expectedChain;
} GroupState;

static UA_StatusCode
verify(UA_CertificateGroup *group, const UA_ByteString *chain) {
    GroupState *state = (GroupState*)group->context;
    ck_assert_ptr_nonnull(chain);
    ck_assert_uint_gt(chain->length, 0);
    ck_assert_uint_eq(chain->data[0], 0x30); /* DER sequence */
    if(state->expectedChain)
        ck_assert(UA_ByteString_equal(chain, state->expectedChain));
    state->verified++;
    return state->result;
}
static void
clearGroup(UA_CertificateGroup *group) {
    ((GroupState*)group->context)->cleared++;
    memset(group, 0, sizeof(*group));
}
static UA_CertificateGroup
groupFor(GroupState *state) {
    UA_CertificateGroup group;
    memset(&group, 0, sizeof(group));
    group.context = state;
    group.verifyCertificate = verify;
    group.clear = clearGroup;
    return group;
}
static UA_ByteString
readFile(const char *name) {
    FILE *f = fopen(name, "rb");
    ck_assert_ptr_nonnull(f);
    ck_assert_int_eq(fseek(f, 0, SEEK_END), 0);
    long length = ftell(f);
    ck_assert_int_gt(length, 0);
    rewind(f);
    UA_ByteString data;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&data, (size_t)length), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fread(data.data, 1, data.length, f), data.length);
    fclose(f);
    return data;
}
static UA_ConnectionManager *
newManager(UA_EventLoop *el, UA_Boolean http, UA_TlsConfig *config) {
    UA_ConnectionManager *cm = http ? UA_ConnectionManager_new_HTTP(UA_STRING("http"), config) :
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("websocket"), config);
    ck_assert_ptr_nonnull(cm);
    ck_assert_uint_eq(el->registerEventSource(el, &cm->eventSource), UA_STATUSCODE_GOOD);
    return cm;
}
static void
stopLoop(UA_EventLoop *el) {
    el->stop(el);
    for(unsigned i = 0; i < 1000 && el->state != UA_EVENTLOOPSTATE_STOPPED; i++)
        el->run(el, 1);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
}

typedef struct {
    UA_Boolean http, server, completed, success;
    uintptr_t id;
    UA_UInt16 port;
    UA_StatusCode status;
    const char *clientAuth;
    UA_Boolean ignoreHostname;
} Peer;

static void
callback(UA_ConnectionManager *cm, uintptr_t id, void *application,
         void **context, UA_ConnectionState state,
         const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)context;
    Peer *peer = (Peer*)application;
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(port) { peer->port = *port; peer->id = id; return; }
    if(state == UA_CONNECTIONSTATE_OPENING)
        peer->id = id;
    if(!peer->server && !peer->http && state == UA_CONNECTIONSTATE_ESTABLISHED) {
        peer->success = peer->completed = true;
        peer->id = id;
    }
    if(peer->server && peer->http && state == UA_CONNECTIONSTATE_ESTABLISHED) {
        const UA_Variant *method = UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "method"));
        if(method) {
            UA_ByteString body = UA_BYTESTRING_ALLOC("ok");
            ck_assert_uint_eq(cm->sendWithConnection(cm, id, &UA_KEYVALUEMAP_NULL, &body),
                              UA_STATUSCODE_GOOD);
        }
    }
    if(!peer->server && peer->http && msg.length) {
        peer->success = peer->completed = true;
    }
    const UA_StatusCode *status = (const UA_StatusCode*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, peer->http ? "request-status" : "status"),
        &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(status && *status != UA_STATUSCODE_GOOD) {
        peer->status = *status;
        peer->completed = true;
    }
    if(!peer->server && state == UA_CONNECTIONSTATE_CLOSING)
        peer->completed = true;
}
static UA_StatusCode
openPeer(UA_ConnectionManager *cm, Peer *peer, const char *host) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_nonnull(map);
    UA_Boolean yes = true;
    UA_String address = UA_STRING((char*)(uintptr_t)host);
    UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "tls"), &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "port"), &peer->port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "address"), &address, &UA_TYPES[UA_TYPES_STRING]);
    if(peer->server)
        UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "listen"), &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(peer->clientAuth) {
        UA_String auth = UA_STRING((char*)(uintptr_t)peer->clientAuth);
        UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "tls-client-auth"), &auth, &UA_TYPES[UA_TYPES_STRING]);
    }
    if(!peer->server) {
        UA_Boolean verifyHostname = !peer->ignoreHostname;
        UA_KeyValueMap_setScalar(map, UA_QUALIFIEDNAME(0, "tls-verify-hostname"), &verifyHostname, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
    UA_StatusCode result = cm->openConnection(cm, map, peer, NULL, callback);
    UA_KeyValueMap_delete(map);
    if(result == UA_STATUSCODE_GOOD && peer->http && !peer->server)
        result = cm->sendWithConnection(cm, peer->id, &UA_KEYVALUEMAP_NULL, NULL);
    return result;
}
static void
waitPeer(UA_EventLoop *el, Peer *peer) {
    for(unsigned i = 0; i < 1000 && !peer->completed; i++)
        el->run(el, 5);
    ck_assert(peer->completed);
}

START_TEST(configOwnership) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    newManager(el, _i == 0, NULL);
    GroupState owned = {0}, borrowed = {0};
    UA_CertificateGroup group = groupFor(&borrowed);
    UA_TlsConfig config = {0};
    config.verification.mode = UA_TLSVERIFY_BORROWED;
    UA_ConnectionManager *cm = _i == 0 ? UA_ConnectionManager_new_HTTP(UA_STRING("invalid"), &config) :
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("invalid"), &config);
    ck_assert_ptr_null(cm);
    ck_assert_int_eq(config.verification.mode, UA_TLSVERIFY_BORROWED);
    config.verification.group.borrowed = &group;
    newManager(el, _i == 0, &config);
    UA_TlsConfig empty = {0};
    ck_assert_int_eq(memcmp(&config, &empty, sizeof(config)), 0);
    config.verification.mode = UA_TLSVERIFY_OWNED;
    config.verification.group.owned = groupFor(&owned);
    config.privateKey = UA_BYTESTRING_ALLOC("invalid");
    cm = _i == 0 ? UA_ConnectionManager_new_HTTP(UA_STRING("invalid"), &config) :
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("invalid"), &config);
    ck_assert_ptr_null(cm);
    ck_assert_ptr_nonnull(config.privateKey.data);
    ck_assert_uint_eq(owned.cleared, 0);
    UA_TlsConfig_clear(&config);
    ck_assert_uint_eq(owned.cleared, 1);
    config = testTlsConfig(NULL, NULL, NULL);
    config.verification.mode = UA_TLSVERIFY_OWNED;
    config.verification.group.owned = groupFor(&owned);
    config.certificateChain = readFile("server_cert.der");
    config.privateKey = UA_BYTESTRING_ALLOC("not a DER key");
    UA_Byte *originalCertificate = config.certificateChain.data;
    cm = _i == 0 ? UA_ConnectionManager_new_HTTP(UA_STRING("invalid-key"), &config) :
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("invalid-key"), &config);
    ck_assert_ptr_null(cm);
    ck_assert_ptr_eq(config.certificateChain.data, originalCertificate);
    ck_assert_uint_eq(owned.cleared, 1);
    UA_TlsConfig_clear(&config);
    ck_assert_uint_eq(owned.cleared, 2);

    config.verification.mode = UA_TLSVERIFY_OWNED;
    config.verification.group.owned = groupFor(&owned);
    newManager(el, _i == 0, &config);
    ck_assert_int_eq(memcmp(&config, &empty, sizeof(config)), 0);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(owned.cleared, 3);
    ck_assert_uint_eq(borrowed.cleared, 0);
    group.clear(&group);
    ck_assert_uint_eq(borrowed.cleared, 1);
} END_TEST

static GroupState *borrowedGroupState;
static UA_StatusCode (*originalManagerFree)(UA_EventSource *es);

static UA_StatusCode
freeManagerWithLiveGroup(UA_EventSource *es) {
    ck_assert_uint_eq(borrowedGroupState->cleared, 0);
    return originalManagerFree(es);
}

/* Explicit managers may borrow the final object's channel verification group.
 * Its address must stay valid until the owned EventLoop frees those managers. */
START_TEST(configuredGroupOutlivesManager) {
    UA_Client *client = NULL;
    UA_Server *server = NULL;
    UA_EventLoop *el;
    UA_CertificateGroup *group;
    if(_i < 2) {
        client = UA_Client_new();
        ck_assert_ptr_nonnull(client);
        el = UA_Client_getConfig(client)->eventLoop;
        group = &UA_Client_getConfig(client)->certificateVerification;
    } else {
        server = UA_Server_new();
        ck_assert_ptr_nonnull(server);
        el = UA_Server_getConfig(server)->eventLoop;
        group = &UA_Server_getConfig(server)->secureChannelPKI;
    }
    GroupState state = {0};
    if(group->clear)
        group->clear(group);
    *group = groupFor(&state);
    UA_TlsConfig config = {0};
    config.verification.mode = UA_TLSVERIFY_BORROWED;
    config.verification.group.borrowed = group;
    UA_ConnectionManager *cm = newManager(el, (_i % 2) == 0, &config);
    borrowedGroupState = &state;
    originalManagerFree = cm->eventSource.free;
    cm->eventSource.free = freeManagerWithLiveGroup;
    if(client)
        UA_Client_delete(client);
    else
        UA_Server_delete(server);
    ck_assert_uint_eq(state.cleared, 1);
} END_TEST

START_TEST(configParameterValidation) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    UA_ConnectionManager *cm = newManager(el, _i == 0, NULL);
    UA_KeyValueMap *params = UA_KeyValueMap_new();
    UA_Boolean yes = true;
    UA_UInt16 port = 4840;
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "validate"), &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "tls"), &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "port"), &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "address"), &address, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "listen"), &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_KeyValueMap_remove(params, UA_QUALIFIEDNAME(0, "listen"));
    const char *removed[] = {"useSSL", "tls-context", "certificate", "private-key",
                             "private-key-password", "ca-certificate"};
    for(unsigned i = 0; i < sizeof(removed)/sizeof(removed[0]); i++) {
        UA_QualifiedName key = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)removed[i]);
        UA_KeyValueMap_setScalar(params, key, &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
        ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
        UA_KeyValueMap_remove(params, key);
    }
    UA_ByteString cert = readFile("server_cert.der"), key = readFile("server_key.der");
    UA_TlsConfig config = testTlsConfig(&cert, &key, NULL);
    cm = newManager(el, _i == 0, &config);
    UA_QualifiedName hostKey = UA_QUALIFIEDNAME(0, "tls-verify-hostname");
    UA_QualifiedName authKey = UA_QUALIFIEDNAME(0, "tls-client-auth");
    UA_QualifiedName listenKey = UA_QUALIFIEDNAME(0, "listen");
    UA_QualifiedName tlsKey = UA_QUALIFIEDNAME(0, "tls");
    UA_String required = UA_STRING("required"), invalid = UA_STRING("unknown");
    /* Client-only Boolean hostname policy, including plaintext HTTP redirects. */
    UA_KeyValueMap_setScalar(params, hostKey, &required, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINTERNALERROR);
    UA_KeyValueMap_setScalar(params, hostKey, &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_setScalar(params, listenKey, &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_KeyValueMap_remove(params, hostKey);
    /* TLS listener-only string policy; all three allowed values are exercised
     * in hostnameAndClientAuthentication, including certificate-less clients. */
    UA_KeyValueMap_setScalar(params, authKey, &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINTERNALERROR);
    UA_KeyValueMap_setScalar(params, authKey, &invalid, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_KeyValueMap_setScalar(params, authKey, &required, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_remove(params, tlsKey);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_KeyValueMap_remove(params, listenKey);
    UA_KeyValueMap_setScalar(params, tlsKey, &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_KeyValueMap_remove(params, authKey);
    UA_KeyValueMap_setScalar(params, tlsKey, &required, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->openConnection(cm, params, NULL, NULL, NULL), UA_STATUSCODE_BADINTERNALERROR);
    UA_ByteString_clear(&cert); UA_ByteString_clear(&key);
    UA_KeyValueMap_delete(params);
    el->free(el);
} END_TEST

/* Managers share lws infrastructure, but each has exactly one independent
 * trust configuration. Exercise both orders of HTTP/WebSocket initialization. */
START_TEST(sharedManagerVerification) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    UA_ConnectionManager *servers[2], *clients[2];
    GroupState states[2] = {{0}};
    UA_CertificateGroup groups[2] = {groupFor(&states[0]), groupFor(&states[1])};
    Peer listeners[2] = {{0}};
    UA_ByteString cert = readFile("server_cert.der"), key = readFile("server_key.der");
    for(unsigned i = 0; i < 2; i++) {
        UA_TlsConfig config = testTlsConfig(&cert, &key, NULL);
        servers[i] = newManager(el, i == (unsigned)_i, &config);
        config.verification.mode = UA_TLSVERIFY_BORROWED;
        config.verification.group.borrowed = &groups[i];
        clients[i] = newManager(el, i == (unsigned)_i, &config);
        listeners[i].http = i == (unsigned)_i;
        listeners[i].server = true;
    }
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    for(unsigned i = 0; i < 2; i++)
        ck_assert_uint_eq(openPeer(servers[i], &listeners[i], "127.0.0.1"), UA_STATUSCODE_GOOD);
    for(unsigned i = 0; i < 2; i++) {
        GroupState *state = &states[i];
        for(unsigned attempt = 0; attempt < 3; attempt++) {
            state->result = attempt == 1 ? UA_STATUSCODE_BADCERTIFICATEREVOKED : UA_STATUSCODE_GOOD;
            Peer peer = {0}; peer.port = listeners[i].port; peer.http = listeners[i].http;
            ck_assert_uint_eq(openPeer(clients[i], &peer, "127.0.0.1"), UA_STATUSCODE_GOOD);
            waitPeer(el, &peer);
            ck_assert_int_eq(peer.success, attempt != 1);
            ck_assert_uint_eq(state->verified, attempt + 1);
            if(attempt == 1)
                ck_assert_uint_eq(peer.status, UA_STATUSCODE_BADCERTIFICATEREVOKED);
            clients[i]->closeConnection(clients[i], peer.id);
            for(unsigned n = 0; n < 10; n++) el->run(el, 1);
        }
        ck_assert_uint_eq(state->cleared, 0);
        UA_ConnectionManager *nativeCM = newManager(el, listeners[i].http, NULL);
        Peer native = {0}; native.port = listeners[i].port; native.http = listeners[i].http;
        ck_assert_uint_eq(openPeer(nativeCM, &native, "127.0.0.1"), UA_STATUSCODE_GOOD);
        waitPeer(el, &native);
        ck_assert(!native.success);
        nativeCM->closeConnection(nativeCM, native.id);
        for(unsigned n = 0; n < 10; n++) el->run(el, 1);
    }
    stopLoop(el);
    el->free(el);
    for(unsigned i = 0; i < 2; i++)
        groups[i].clear(&groups[i]);
    UA_ByteString_clear(&cert); UA_ByteString_clear(&key);
} END_TEST

START_TEST(hostnameAndClientAuthentication) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    UA_ByteString cert = readFile("server_cert.der"), key = readFile("server_key.der");
    GroupState state = {0};
    UA_CertificateGroup group = groupFor(&state);
    UA_TlsConfig serverConfig = testTlsConfig(&cert, &key, &cert);
    UA_ConnectionManager *serverCM = newManager(el, _i == 0, &serverConfig);
    const char *authNames[] = {"none", "optional", "required"};
    for(unsigned auth = 0; auth < 3; auth++) {
        Peer server = {0}; server.http = _i == 0; server.server = true;
        server.clientAuth = authNames[auth];
        ck_assert_uint_eq(openPeer(serverCM, &server, "127.0.0.1"), UA_STATUSCODE_GOOD);
        for(unsigned identity = 0; identity < 2; identity++) {
            UA_TlsConfig config = {0};
            config.verification.mode = UA_TLSVERIFY_BORROWED;
            config.verification.group.borrowed = &group;
            if(identity) {
                UA_ByteString_copy(&cert, &config.certificateChain);
                UA_ByteString_copy(&key, &config.privateKey);
            }
            UA_ConnectionManager *clientCM = newManager(el, _i == 0, &config);
            Peer client = {0}; client.http = server.http; client.port = server.port;
            ck_assert_uint_eq(openPeer(clientCM, &client, "127.0.0.1"), UA_STATUSCODE_GOOD);
            waitPeer(el, &client);
            ck_assert_int_eq(client.success, identity || auth != 2);
            clientCM->closeConnection(clientCM, client.id);
            for(unsigned n = 0; n < 20; n++) el->run(el, 1);
        }
        serverCM->closeConnection(serverCM, server.id);
        for(unsigned n = 0; n < 20; n++) el->run(el, 1);
    }
    /* Use a certificate without an IP SAN so the numeric loopback address
     * mismatches. This keeps the test independent of localhost resolution and
     * IPv6-to-IPv4 connection fallback. */
    const unsigned char *data = cert.data;
    X509 *leaf = d2i_X509(NULL, &data, (long)cert.length);
    ck_assert_ptr_nonnull(leaf);
    int index = X509_get_ext_by_NID(leaf, NID_subject_alt_name, -1);
    ck_assert_int_ge(index, 0);
    X509_EXTENSION_free(X509_delete_ext(leaf, index));
    X509_EXTENSION *san = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name,
                                            "DNS:localhost");
    ck_assert_ptr_nonnull(san);
    ck_assert_int_eq(X509_add_ext(leaf, san, -1), 1);
    X509_EXTENSION_free(san);
    data = key.data;
    EVP_PKEY *signer = d2i_AutoPrivateKey(NULL, &data, (long)key.length);
    ck_assert_ptr_nonnull(signer);
    ck_assert_int_gt(X509_sign(leaf, signer, EVP_sha256()), 0);
    EVP_PKEY_free(signer);
    BIO *der = BIO_new(BIO_s_mem());
    ck_assert_ptr_nonnull(der);
    ck_assert_int_eq(i2d_X509_bio(der, leaf), 1);
    X509_free(leaf);
    char *encoded;
    long length = BIO_get_mem_data(der, &encoded);
    ck_assert_int_gt(length, 0);
    UA_ByteString mismatch = {(size_t)length, (UA_Byte*)encoded};
    serverConfig = testTlsConfig(&mismatch, &key, NULL);
    serverCM = newManager(el, _i == 0, &serverConfig);
    BIO_free(der);

    /* An explicit AcceptAll group still checks the hostname. */
    UA_CertificateGroup acceptAll = {0};
    UA_CertificateGroup_AcceptAll(&acceptAll);
    Peer server = {0}; server.http = _i == 0; server.server = true;
    ck_assert_uint_eq(openPeer(serverCM, &server, "127.0.0.1"), UA_STATUSCODE_GOOD);
    UA_TlsConfig config = {0};
    config.verification.mode = UA_TLSVERIFY_BORROWED;
    config.verification.group.borrowed = &acceptAll;
    UA_ConnectionManager *clientCM = newManager(el, _i == 0, &config);
    /* Concurrent connections share identity/trust, but not hostname behavior. */
    Peer clients[2] = {{0}};
    for(unsigned ignore = 0; ignore < 2; ignore++) {
        clients[ignore].http = server.http;
        clients[ignore].port = server.port;
        clients[ignore].ignoreHostname = (UA_Boolean)ignore;
        ck_assert_uint_eq(openPeer(clientCM, &clients[ignore], "127.0.0.1"),
                          UA_STATUSCODE_GOOD);
    }
    for(unsigned i = 0; i < 2; i++) {
        waitPeer(el, &clients[i]);
        ck_assert_int_eq(clients[i].success, i);
        if(!i) ck_assert_uint_eq(clients[i].status, UA_STATUSCODE_BADCERTIFICATEHOSTNAMEINVALID);
        clientCM->closeConnection(clientCM, clients[i].id);
    }
    stopLoop(el); el->free(el);
    ck_assert_uint_eq(state.cleared, 0);
    group.clear(&group); acceptAll.clear(&acceptAll);
    UA_ByteString_clear(&cert); UA_ByteString_clear(&key);
} END_TEST

/* Send a real two-certificate chain in both directions. The callback checks
 * the complete peer chain, including the issuer supplied by lws. */
START_TEST(certificateChains) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    UA_ConnectionManager *managers[2];
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    UA_ByteString cert = readFile("server_cert.der"), key = readFile("server_key.der");
    const unsigned char *p = cert.data;
    X509 *leaf = d2i_X509(NULL, &p, (long)cert.length);
    ck_assert_ptr_nonnull(leaf);
    X509 *issuer = X509_dup(leaf);
    ck_assert_ptr_nonnull(issuer);
    p = key.data;
    EVP_PKEY *signer = d2i_AutoPrivateKey(NULL, &p, (long)key.length);
    ck_assert_ptr_nonnull(signer);
    X509_NAME *name = X509_NAME_new();
    ck_assert_ptr_nonnull(name);
    ck_assert_int_eq(X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
        (const unsigned char*)"TLS test issuer", -1, -1, 0), 1);
    ck_assert_int_eq(X509_set_subject_name(issuer, name), 1);
    ck_assert_int_eq(X509_set_issuer_name(issuer, name), 1);
    ck_assert_int_eq(X509_set_issuer_name(leaf, name), 1);
    X509_NAME_free(name);
    ck_assert_int_eq(ASN1_INTEGER_set(X509_get_serialNumber(issuer), 2), 1);
    int nids[] = {NID_basic_constraints, NID_key_usage};
    char *values[] = {"critical,CA:TRUE", "critical,keyCertSign,cRLSign"};
    for(unsigned i = 0; i < 2; i++) {
        int index = X509_get_ext_by_NID(issuer, nids[i], -1);
        if(index >= 0) X509_EXTENSION_free(X509_delete_ext(issuer, index));
        X509_EXTENSION *ext = X509V3_EXT_conf_nid(NULL, NULL, nids[i], values[i]);
        ck_assert_ptr_nonnull(ext);
        ck_assert_int_eq(X509_add_ext(issuer, ext, -1), 1);
        X509_EXTENSION_free(ext);
    }
    ck_assert_int_gt(X509_sign(issuer, signer, EVP_sha256()), 0);
    ck_assert_int_gt(X509_sign(leaf, signer, EVP_sha256()), 0);
    EVP_PKEY_free(signer);
    BIO *der = BIO_new(BIO_s_mem()), *pem = BIO_new(BIO_s_mem());
    ck_assert_ptr_nonnull(der); ck_assert_ptr_nonnull(pem);
    ck_assert_int_eq(i2d_X509_bio(der, leaf), 1);
    ck_assert_int_eq(i2d_X509_bio(der, issuer), 1);
    ck_assert_int_eq(PEM_write_bio_X509(pem, leaf), 1);
    ck_assert_int_eq(PEM_write_bio_X509(pem, issuer), 1);
    X509_free(leaf); X509_free(issuer);
    char *data;
    long length = BIO_get_mem_data(der, &data);
    UA_ByteString chain = {(size_t)length, (UA_Byte*)data};
    GroupState state = {0}; state.expectedChain = &chain;
    UA_CertificateGroup group = groupFor(&state);
    for(unsigned format = 0; format < 2; format++) {
        length = BIO_get_mem_data(format ? pem : der, &data);
        UA_ByteString identity = {(size_t)length, (UA_Byte*)data};
        Peer peers[2] = {{0}};
        for(unsigned side = 0; side < 2; side++) {
            UA_TlsConfig config = {0};
            config.verification.mode = UA_TLSVERIFY_BORROWED;
            config.verification.group.borrowed = &group;
            UA_ByteString_copy(&identity, &config.certificateChain);
            UA_ByteString_copy(&key, &config.privateKey);
            managers[side] = newManager(el, _i == 0, &config);
            peers[side].http = _i == 0;
            peers[side].server = side == 0;
            peers[side].clientAuth = side ? NULL : "required";
            peers[side].port = side ? peers[0].port : 0;
            ck_assert_uint_eq(openPeer(managers[side], &peers[side], "127.0.0.1"), UA_STATUSCODE_GOOD);
        }
        waitPeer(el, &peers[1]);
        ck_assert(peers[1].success);
        ck_assert_uint_eq(state.verified, 2 * (format + 1));
        managers[1]->closeConnection(managers[1], peers[1].id);
        managers[0]->closeConnection(managers[0], peers[0].id);
        for(unsigned n = 0; n < 20; n++) el->run(el, 1);
    }
    stopLoop(el); el->free(el);
    group.clear(&group);
    BIO_free(der); BIO_free(pem);
    UA_ByteString_clear(&cert); UA_ByteString_clear(&key);
} END_TEST

int main(void) {
    Suite *suite = suite_create("Manager TLS configuration");
    TCase *tc = tcase_create("ownership and verification");
    tcase_set_timeout(tc, 60);
    tcase_add_loop_test(tc, configParameterValidation, 0, 2);
    tcase_add_loop_test(tc, certificateChains, 0, 2);
    tcase_add_loop_test(tc, configOwnership, 0, 2);
    tcase_add_loop_test(tc, configuredGroupOutlivesManager, 0, 4);
    tcase_add_loop_test(tc, sharedManagerVerification, 0, 2);
    tcase_add_loop_test(tc, hostnameAndClientAuthentication, 0, 2);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
