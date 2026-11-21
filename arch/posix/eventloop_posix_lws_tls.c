/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */
#include "eventloop_posix_lws.h"
#include <limits.h>

#if defined(LWS_WITH_TLS) && !defined(LWS_WITH_MBEDTLS) && \
    !defined(USE_WOLFSSL) && !defined(LWS_WITH_BORINGSSL) && !defined(LWS_WITH_AWSLC)
# define UA_LWS_TLS_OPENSSL
# include <openssl/x509v3.h>
# include <openssl/pem.h>
# include <openssl/err.h>
#endif

#ifdef UA_LWS_TLS_OPENSSL
/* Normalize the supplied chain for libwebsockets. */
static UA_StatusCode
normalizeChain(const UA_ByteString *source, UA_ByteString *der) {
    if(!source->length || source->data[0] == 0x30)
        return UA_ByteString_copy(source, der);
    BIO *input = BIO_new_mem_buf(source->data, (int)source->length);
    BIO *output = BIO_new(BIO_s_mem());
    if(!input || !output) {
        BIO_free(input); BIO_free(output);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    char *remaining;
    long length;
    while((length = BIO_get_mem_data(input, &remaining)) > 0) {
        if(*remaining == ' ' || *remaining == '\r' ||
           *remaining == '\n' || *remaining == '\t') {
            char ignored;
            (void)BIO_read(input, &ignored, 1);
            continue;
        }
        const char begin[] = "-----BEGIN CERTIFICATE-----";
        if((size_t)length < sizeof(begin)-1 || memcmp(remaining, begin, sizeof(begin)-1)) {
            res = UA_STATUSCODE_BADCERTIFICATEINVALID;
            break;
        }
        X509 *cert = PEM_read_bio_X509(input, NULL, NULL, NULL);
        int ok = cert ? i2d_X509_bio(output, cert) : 0;
        X509_free(cert);
        if(!ok) { res = UA_STATUSCODE_BADCERTIFICATEINVALID; break; }
    }
    length = BIO_get_mem_data(output, &remaining);
    if(res == UA_STATUSCODE_GOOD) {
        UA_ByteString buffer = {(size_t)length, (UA_Byte*)remaining};
        res = length ? UA_ByteString_copy(&buffer, der) : UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    BIO_free(input); BIO_free(output);
    ERR_clear_error();
    return res;
}
#endif

UA_StatusCode
UA_LWS_initTlsConfig(UA_LWSTlsState *state, UA_TlsConfig *config) {
    if(!config)
        return UA_STATUSCODE_GOOD;
#ifndef UA_LWS_TLS_OPENSSL
    (void)state; (void)config;
    return UA_STATUSCODE_BADNOTSUPPORTED;
#else
    if(config->verification.mode > UA_TLSVERIFY_OWNED ||
       config->verification.mode < UA_TLSVERIFY_SYSTEM)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_CertificateGroup *group = NULL;
    if(config->verification.mode == UA_TLSVERIFY_BORROWED)
        group = config->verification.group.borrowed;
    else if(config->verification.mode == UA_TLSVERIFY_OWNED)
        group = &config->verification.group.owned;
    if(config->verification.mode != UA_TLSVERIFY_SYSTEM &&
       !group)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if((config->certificateChain.length == 0) != (config->privateKey.length == 0) ||
       config->certificateChain.length > INT_MAX || config->privateKey.length > INT_MAX)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString chain = UA_BYTESTRING_NULL;
    UA_StatusCode res = normalizeChain(&config->certificateChain, &chain);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(chain.length) {
        const unsigned char *p = chain.data;
        const unsigned char *end = p + chain.length;
        X509 *leaf = d2i_X509(NULL, &p, (long)chain.length);
        if(!leaf) {
            UA_ByteString_clear(&chain);
            return UA_STATUSCODE_BADCERTIFICATEINVALID;
        }
        UA_Boolean valid = true;
        while(p < end) {
            X509 *issuer = d2i_X509(NULL, &p, (long)(end - p));
            if(!issuer) { valid = false; break; }
            X509_free(issuer);
        }
        p = config->privateKey.data;
        EVP_PKEY *key = d2i_AutoPrivateKey(NULL, &p, (long)config->privateKey.length);
        valid = valid && key && p == config->privateKey.data + config->privateKey.length &&
            X509_check_private_key(leaf, key) == 1;
        EVP_PKEY_free(key);
        X509_free(leaf);
        if(!valid) {
            UA_ByteString_clear(&chain);
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        }
    }
    UA_ByteString_clear(&config->certificateChain);
    config->certificateChain = chain;
    state->config = *config;
    memset(config, 0, sizeof(*config));
    return UA_STATUSCODE_GOOD;
#endif
}

UA_StatusCode
UA_LWS_clearTlsConfig(UA_LWSTlsState *state) {
    if(state->users)
        return UA_STATUSCODE_BADINVALIDSTATE;
    UA_TlsConfig_clear(&state->config);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_LWS_validateTls(UA_LWSTlsState *state, UA_Boolean listener, UA_Boolean tls,
                   const UA_KeyValueMap *params, UA_LWSTlsOptions *options) {
    options->verifyHostname = true;
    options->clientAuth = UA_TLSCLIENTAUTH_NONE;
    const UA_Variant *host = UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "tls-verify-hostname"));
    const UA_Variant *auth = UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "tls-client-auth"));
    if(host) {
        if(listener || !UA_Variant_hasScalarType(host, &UA_TYPES[UA_TYPES_BOOLEAN]))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        options->verifyHostname = *(UA_Boolean*)host->data;
    }
    if(auth) {
        if(!listener || !tls || !UA_Variant_hasScalarType(auth, &UA_TYPES[UA_TYPES_STRING]))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        UA_String none = UA_STRING("none"), optional = UA_STRING("optional"), required = UA_STRING("required");
        if(UA_String_equal((UA_String*)auth->data, &optional))
            options->clientAuth = UA_TLSCLIENTAUTH_OPTIONAL;
        else if(UA_String_equal((UA_String*)auth->data, &required))
            options->clientAuth = UA_TLSCLIENTAUTH_REQUIRED;
        else if(!UA_String_equal((UA_String*)auth->data, &none))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
#ifndef LWS_WITH_TLS
    if(tls)
        return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    if(listener && tls && !state->config.certificateChain.length)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_STATUSCODE_GOOD;
}

void
UA_LWS_configureTls(struct lws_context_creation_info *info,
                     UA_LWSTlsState *context, UA_Boolean listener,
                     const UA_LWSTlsOptions *options) {
#ifdef LWS_WITH_TLS
    if(!context)
        return;
    UA_TlsConfig *config = &context->config;
    if(context->config.certificateChain.length) {
        if(listener) {
            info->server_ssl_cert_mem = context->config.certificateChain.data;
            info->server_ssl_cert_mem_len = (unsigned int)context->config.certificateChain.length;
            info->server_ssl_private_key_mem = config->privateKey.data;
            info->server_ssl_private_key_mem_len = (unsigned int)config->privateKey.length;
        } else {
            info->client_ssl_cert_mem = context->config.certificateChain.data;
            info->client_ssl_cert_mem_len = (unsigned int)context->config.certificateChain.length;
            info->client_ssl_key_mem = config->privateKey.data;
            info->client_ssl_key_mem_len = (unsigned int)config->privateKey.length;
        }
    }
    if(listener && options->clientAuth != UA_TLSCLIENTAUTH_NONE) {
        info->options |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
        if(options->clientAuth == UA_TLSCLIENTAUTH_OPTIONAL)
            info->options |= LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;
    }
#else
    (void)info; (void)context; (void)listener; (void)options;
#endif
}

void
UA_LWS_bindTls(UA_EventLoop *el, UA_LWSTlsBinding *binding, struct lws_vhost *vhost,
               UA_LWSTlsState *context, UA_Boolean listener) {
    UA_EventLoopPOSIX *loop = (UA_EventLoopPOSIX*)el;
    binding->context = context;
    binding->listener = listener;
    binding->vhost = vhost;
    binding->next = (UA_LWSTlsBinding*)loop->lwsTlsBindings;
    loop->lwsTlsBindings = binding;
    if(context)
        context->users++;
}

void
UA_LWS_unbindTls(UA_EventLoop *el, UA_LWSTlsBinding *binding) {
    UA_EventLoopPOSIX *loop = (UA_EventLoopPOSIX*)el;
    UA_LWSTlsBinding **entry = (UA_LWSTlsBinding**)&loop->lwsTlsBindings;
    while(*entry && *entry != binding)
        entry = &(*entry)->next;
    if(!*entry)
        return;
    *entry = binding->next;
    if(binding->context)
        binding->context->users--;
    memset(binding, 0, sizeof(*binding));
}

#ifdef UA_LWS_TLS_OPENSSL
typedef struct { UA_StatusCode status; } TlsDecision;
static CRYPTO_ONCE decisionOnce = CRYPTO_ONCE_STATIC_INIT;
static int decisionIndex = -1;
static void freeDecision(void *parent, void *ptr, CRYPTO_EX_DATA *ad,
                         int idx, long argl, void *argp) {
    (void)parent; (void)ad; (void)idx; (void)argl; (void)argp;
    UA_free(ptr);
}
static void createDecisionIndex(void) {
    decisionIndex = SSL_get_ex_new_index(0, NULL, NULL, NULL, freeDecision);
}

static UA_StatusCode
verifyGroup(UA_CertificateGroup *group, X509_STORE_CTX *store, SSL *ssl,
            UA_Boolean listener, UA_Boolean ignoreHostname) {
    X509 *leaf = X509_STORE_CTX_get0_cert(store);
    STACK_OF(X509) *chain = X509_STORE_CTX_get0_untrusted(store);
    if(!leaf)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    /* The untrusted stack is the peer-supplied chain, not OpenSSL's incomplete
     * chain built against system roots. The group supplies its own issuers. */
    BIO *bio = BIO_new(BIO_s_mem());
    if(!bio)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    int ok = i2d_X509_bio(bio, leaf);
    for(int i = 0; ok && chain && i < sk_X509_num(chain); i++) {
        X509 *cert = sk_X509_value(chain, i);
        if(X509_cmp(cert, leaf) != 0)
            ok = i2d_X509_bio(bio, cert);
    }
    char *data = NULL;
    long length = BIO_get_mem_data(bio, &data);
    UA_ByteString der = {(length > 0) ? (size_t)length : 0, (UA_Byte*)data};
    UA_StatusCode status = (ok && length > 0) ?
        (group->verifyCertificate ? group->verifyCertificate(group, &der) :
         UA_STATUSCODE_GOOD) : UA_STATUSCODE_BADCERTIFICATEINVALID;
    BIO_free(bio);
    if(status != UA_STATUSCODE_GOOD)
        return status;
    if(X509_check_purpose(leaf, listener ? X509_PURPOSE_SSL_CLIENT :
                         X509_PURPOSE_SSL_SERVER, 0) != 1)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    if(listener || ignoreHostname)
        return UA_STATUSCODE_GOOD;
    X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
    const char *host = X509_VERIFY_PARAM_get0_host(param, 0);
    char *ip = X509_VERIFY_PARAM_get1_ip_asc(param);
    int matches = ip ? X509_check_ip_asc(leaf, ip, 0) :
        (host ? X509_check_host(leaf, host, 0, X509_CHECK_FLAG_NEVER_CHECK_SUBJECT, NULL) : 0);
    OPENSSL_free(ip);
    return matches == 1 ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADCERTIFICATEHOSTNAMEINVALID;
}
#endif

static UA_LWSTlsBinding *
findBinding(struct lws *wsi, UA_Boolean initializing) {
    if(!wsi)
        return NULL;
    UA_EventLoopPOSIX *loop = (UA_EventLoopPOSIX*)lws_context_user(lws_get_context(wsi));
    if(!loop)
        return NULL;
    for(UA_LWSTlsBinding *b = (UA_LWSTlsBinding*)loop->lwsTlsBindings; b; b = b->next) {
        if(b->vhost == lws_get_vhost(wsi))
            return b;
        /* Vhost construction is synchronous and serialized on the event loop.
         * Bind before creation so initialization callbacks see the TLS context. */
        if(initializing && !b->vhost) {
            b->vhost = lws_get_vhost(wsi);
            return b;
        }
    }
    return NULL;
}

int
UA_LWS_verifyTls(struct lws *wsi, void *storeContext, void *sslPtr, int preverify) {
#ifdef UA_LWS_TLS_OPENSSL
    X509_STORE_CTX *store = (X509_STORE_CTX*)storeContext;
    SSL *ssl = (SSL*)sslPtr;
    if(!wsi || !store || !ssl)
        return 1;
    UA_EventLoopPOSIX *loop = (UA_EventLoopPOSIX*)lws_context_user(lws_get_context(wsi));
    UA_LWSTlsBinding *binding = findBinding(wsi, false);
    UA_TlsConfig *config = (binding && binding->context) ? &binding->context->config : NULL;
    if(!config || config->verification.mode == UA_TLSVERIFY_SYSTEM) {
        if(binding && !binding->options.verifyHostname &&
           (X509_STORE_CTX_get_error(store) == X509_V_ERR_HOSTNAME_MISMATCH ||
            X509_STORE_CTX_get_error(store) == X509_V_ERR_IP_ADDRESS_MISMATCH)) {
            X509_STORE_CTX_set_error(store, X509_V_OK);
            return 0;
        }
        return preverify ? 0 : 1;
    }
    if(CRYPTO_THREAD_run_once(&decisionOnce, createDecisionIndex) != 1 || decisionIndex < 0)
        return 1;
    TlsDecision *decision = (TlsDecision*)SSL_get_ex_data(ssl, decisionIndex);
    if(!decision) {
        decision = (TlsDecision*)UA_malloc(sizeof(*decision));
        if(!decision)
            return 1;
        UA_CertificateGroup *group = config->verification.mode == UA_TLSVERIFY_BORROWED ?
            config->verification.group.borrowed : &config->verification.group.owned;
        decision->status = verifyGroup(group, store, ssl, binding->listener,
                                      !binding->options.verifyHostname);
        if(!SSL_set_ex_data(ssl, decisionIndex, decision)) {
            UA_free(decision);
            return 1;
        }
    }
    if(binding->verificationResult)
        binding->verificationResult(wsi, decision->status);
    else
        binding->status = decision->status;
    if(decision->status != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(loop->eventLoop.logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "TLS peer verification failed: %s", UA_StatusCode_name(decision->status));
        return 1;
    }
    /* Trust is decided by CertificateGroup, not a second system trust store. */
    X509_STORE_CTX_set_error(store, X509_V_OK);
    return 0;
#else
    (void)wsi; (void)storeContext; (void)sslPtr;
    return preverify ? 0 : 1;
#endif
}

int
UA_LWS_tlsCallback(struct lws *wsi, enum lws_callback_reasons reason,
                    void *user, void *in, size_t len) {
    if(reason == LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION ||
       reason == LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION)
        return UA_LWS_verifyTls(wsi, user, in, (int)len);
#ifdef UA_LWS_TLS_OPENSSL
    if(reason == LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS ||
       reason == LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS) {
        /* Each new TLS connection must consult the current CertificateGroup.
         * Resumption can omit certificate verification entirely. */
        SSL_CTX *ctx = (SSL_CTX*)user;
        if(ctx) {
            SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
            SSL_CTX_set_options(ctx, SSL_OP_NO_TICKET);
            UA_LWSTlsBinding *binding = findBinding(wsi, true);
            if(binding && binding->context) {
                UA_TlsConfig *config = &binding->context->config;
                if(binding->listener && binding->options.clientAuth != UA_TLSCLIENTAUTH_NONE &&
                   config->verification.mode == UA_TLSVERIFY_SYSTEM &&
                   SSL_CTX_set_default_verify_paths(ctx) != 1) {
                    binding->status = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
                    return 1;
                }
                const UA_ByteString *chain = &binding->context->config.certificateChain;
                const unsigned char *p = chain->data;
                if(chain->length) {
                    const unsigned char *end = p + chain->length;
                    X509 *leaf = d2i_X509(NULL, &p, (long)chain->length);
                    X509_free(leaf); /* lws has already installed the leaf. */
                    SSL_CTX_clear_chain_certs(ctx);
                    while(p < end) {
                        X509 *issuer = d2i_X509(NULL, &p, (long)(end - p));
                        int ok = issuer ? SSL_CTX_add1_chain_cert(ctx, issuer) : 0;
                        X509_free(issuer);
                        if(!ok) {
                            binding->status = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
                            return 1;
                        }
                    }
                }
            }
        }
        return 0;
    }
#endif
    return -2; /* Not a TLS callback */
}
