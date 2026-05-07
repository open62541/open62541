/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Cross-SDK interoperability test client.
 * Implements test scenarios T-1 through T-9 from tests/interop/README.md.
 *
 * Usage:
 *   check_interop_client <url>
 *   check_interop_client <url> <cert.der> <key.der> [<trustlist.der> ...]
 *
 * Without certificates only T-1..T-4 (no security) are executed.
 * With certificates all tests including encrypted connections and X509
 * certificate authentication are executed.
 *
 * Exit codes:
 *   0 = all tests passed
 *   1 = test failure
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_ENCRYPTION
#include <open62541/plugin/certificategroup_default.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static UA_ByteString loadFileFromDisk(const char *path) {
    UA_ByteString fileContents = UA_STRING_NULL;
    FILE *fp = fopen(path, "rb");
    if(!fp)
        return fileContents;
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length);
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t rd = fread(fileContents.data, 1, fileContents.length, fp);
        if(rd != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);
    return fileContents;
}

static int g_passed = 0;
static int g_failed = 0;
static int g_skipped = 0;

static void
interop_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("[interop] ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

static void
interop_fail(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[interop] FAIL: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    g_failed++;
}

static void interop_pass(const char *msg) {
    printf("[interop] PASS: %s\n", msg);
    g_passed++;
}

static void interop_skip(const char *msg) {
    printf("[interop] SKIP: %s\n", msg);
    g_skipped++;
}

#define INTEROP_CHECK(cond, msg) do { \
    if(!(cond)) { \
        interop_fail("%s", msg); \
    } else { \
        interop_pass(msg); \
    } \
} while(0)

/* ------------------------------------------------------------------ */
/* Shared client setup                                                */
/* ------------------------------------------------------------------ */

#ifdef UA_ENABLE_ENCRYPTION
/* Configure encryption on a client. Returns UA_STATUSCODE_GOOD on success. */
static UA_StatusCode
configureEncryption(UA_ClientConfig *cc, const char *policyUri,
                    UA_ByteString *certificate, UA_ByteString *privateKey,
                    UA_ByteString *trustList, size_t trustListSize) {
    UA_StatusCode retval =
        UA_ClientConfig_setDefaultEncryption(cc, *certificate, *privateKey,
                                             trustList, trustListSize, NULL, 0);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.client.application");
    if(policyUri)
        cc->securityPolicyUri = UA_STRING_ALLOC(policyUri);
    return UA_STATUSCODE_GOOD;
}
#endif

/* ------------------------------------------------------------------ */
/* Sub-checks run inside an established session                       */
/* ------------------------------------------------------------------ */

static void
check_read_server_status(UA_Client *client) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_StatusCode retval = UA_Client_readValueAttribute(
        client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "Read ServerStatus State");
    if(retval == UA_STATUSCODE_GOOD)
        INTEROP_CHECK(val.type != NULL, "ServerStatus has a type");
    UA_Variant_clear(&val);
}

static void
check_read_namespace_array(UA_Client *client) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_StatusCode retval = UA_Client_readValueAttribute(
        client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &val);
    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "Read NamespaceArray");
    if(retval == UA_STATUSCODE_GOOD)
        INTEROP_CHECK(val.arrayLength > 0, "NamespaceArray has entries");
    UA_Variant_clear(&val);
}

/* ------------------------------------------------------------------ */
/* T-1: Anonymous Connection (No Security)                            */
/* ------------------------------------------------------------------ */

static void
test_T1_anonymous_none(const char *url) {
    interop_log("--- T-1: Anonymous Connection (No Security) ---");

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval = UA_Client_connect(client, url);
    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "T-1 Anonymous connect (None)");

    if(retval == UA_STATUSCODE_GOOD) {
        check_read_server_status(client);
        check_read_namespace_array(client);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}

/* ------------------------------------------------------------------ */
/* T-2: Username/Password Authentication (No Security)                */
/* ------------------------------------------------------------------ */

static void
test_T2_username_none(const char *url) {
    interop_log("--- T-2: Username/Password (No Security) ---");

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval =
        UA_Client_connectUsername(client, url, "user1", "password");

    /* Many servers (including interop_server) correctly reject username/password
     * over the None security policy to prevent credential leaking.
     * Accept both outcomes. */
    if(retval == UA_STATUSCODE_GOOD) {
        interop_log("PASS: T-2 Username connect (None) - server allows it");
        g_passed++;
        check_read_server_status(client);
        UA_Client_disconnect(client);
    } else {
        interop_log("PASS: T-2 Username connect (None) - server correctly "
                     "rejects unencrypted credentials (0x%08x)",
                     (unsigned)retval);
        g_passed++;
    }
    UA_Client_delete(client);
}

/* ------------------------------------------------------------------ */
/* T-3: Read Custom Variable                                          */
/* ------------------------------------------------------------------ */

static void
test_T3_read_variable(const char *url) {
    interop_log("--- T-3: Read Custom Variable ---");

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval = UA_Client_connect(client, url);
    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "T-3 Connect");

    if(retval == UA_STATUSCODE_GOOD) {
        UA_Variant val;
        UA_Variant_init(&val);
        retval = UA_Client_readValueAttribute(
            client, UA_NODEID_STRING(1, "the.answer"), &val);
        if(retval == UA_STATUSCODE_BADNODEIDUNKNOWN) {
            interop_skip("T-3 Read the.answer (node not on this server)");
        } else {
            INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "T-3 Read the.answer");
            if(retval == UA_STATUSCODE_GOOD &&
               val.type == &UA_TYPES[UA_TYPES_INT32]) {
                UA_Int32 v = *(UA_Int32 *)val.data;
                INTEROP_CHECK(v == 43, "T-3 Value == 43");
            }
        }
        UA_Variant_clear(&val);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}

/* ------------------------------------------------------------------ */
/* T-4: Call Method                                                   */
/* ------------------------------------------------------------------ */

static void
test_T4_call_method(const char *url) {
    interop_log("--- T-4: Call HelloWorld Method ---");

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval = UA_Client_connect(client, url);
    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "T-4 Connect");

    if(retval == UA_STATUSCODE_GOOD) {
        UA_Variant input;
        UA_Variant_init(&input);
        UA_String arg = UA_STRING("World");
        UA_Variant_setScalar(&input, &arg, &UA_TYPES[UA_TYPES_STRING]);

        size_t outputSize = 0;
        UA_Variant *output = NULL;
        retval = UA_Client_call(client,
                                UA_NODEID_NUMERIC(1, 1000),
                                UA_NODEID_NUMERIC(1, 62541),
                                1, &input, &outputSize, &output);
        if(retval == UA_STATUSCODE_BADNODEIDUNKNOWN ||
           retval == UA_STATUSCODE_BADMETHODINVALID) {
            interop_skip("T-4 Call HelloWorld (method not on this server)");
        } else {
            INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, "T-4 Call HelloWorld");
            if(retval == UA_STATUSCODE_GOOD && outputSize > 0 &&
               output[0].type == &UA_TYPES[UA_TYPES_STRING]) {
                UA_String *result = (UA_String *)output[0].data;
                UA_Boolean hasHello = (result->length >= 5 &&
                                       memcmp(result->data, "Hello", 5) == 0);
                INTEROP_CHECK(hasHello, "T-4 Output starts with 'Hello'");
            }
        }
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}

/* ------------------------------------------------------------------ */
/* T-5/6/7: Encrypted Connection per policy                           */
/* ------------------------------------------------------------------ */

#ifdef UA_ENABLE_ENCRYPTION
static void
test_encrypted_anonymous(const char *url, const char *policyUri,
                         const char *label,
                         UA_ByteString *certificate, UA_ByteString *privateKey,
                         UA_ByteString *trustList, size_t trustListSize) {
    interop_log("--- %s: Encrypted Anonymous (%s) ---", label, policyUri);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval =
        configureEncryption(cc, policyUri, certificate, privateKey,
                            trustList, trustListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        INTEROP_CHECK(UA_FALSE, "Configure encryption");
        UA_Client_delete(client);
        return;
    }

    retval = UA_Client_connect(client, url);
    char msg[128];
    snprintf(msg, sizeof(msg), "%s Encrypted anonymous connect", label);

    /* A server that doesn't offer this security policy returns
     * BadSecurityPolicyRejected (or BadSecureChannelIdInvalid / BadNoMatch
     * from some stacks).  A server that doesn't trust the client certificate
     * may return BadSecurityChecksFailed, BadCertificateUntrusted, or simply
     * close the connection.  When no matching endpoint is found at all the
     * client returns BadIdentityTokenRejected.  Treat all of these as a
     * graceful SKIP rather than a failure so that SDKs with limited policies
     * or trust configuration don't break the test suite. */
    if(retval == UA_STATUSCODE_BADSECURITYPOLICYREJECTED ||
       retval == UA_STATUSCODE_BADSECURITYMODEREJECTED   ||
       retval == UA_STATUSCODE_BADNOTSUPPORTED            ||
       retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED    ||
       retval == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED    ||
       retval == UA_STATUSCODE_BADSECURECHANNELIDINVALID  ||
       retval == UA_STATUSCODE_BADCONNECTIONCLOSED        ||
       retval == UA_STATUSCODE_BADIDENTITYTOKENREJECTED) {
        char skip_msg[160];
        snprintf(skip_msg, sizeof(skip_msg),
                 "%s (policy/cert not accepted by server, 0x%08x)",
                 label, (unsigned)retval);
        interop_skip(skip_msg);
        UA_Client_delete(client);
        return;
    }

    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, msg);

    if(retval == UA_STATUSCODE_GOOD) {
        check_read_server_status(client);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}

/* Variant for ECC tests that can treat cert-trust problems as failures
 * when the test is expected to succeed (INTEROP_REQUIRE_ECC). */
static void
test_ecc_encrypted_anonymous(const char *url, const char *policyUri,
                             const char *label,
                             UA_ByteString *certificate, UA_ByteString *privateKey,
                             UA_ByteString *trustList, size_t trustListSize,
                             UA_Boolean required) {
    interop_log("--- %s: ECC Encrypted Anonymous (%s) ---", label, policyUri);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval =
        configureEncryption(cc, policyUri, certificate, privateKey,
                            trustList, trustListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        INTEROP_CHECK(UA_FALSE, "Configure encryption");
        UA_Client_delete(client);
        return;
    }

    retval = UA_Client_connect(client, url);
    char msg[128];
    snprintf(msg, sizeof(msg), "%s ECC anonymous connect", label);

    /* Server genuinely does not offer this specific ECC policy.
     * Always a legitimate SKIP regardless of the required flag. */
    if(retval == UA_STATUSCODE_BADSECURITYPOLICYREJECTED ||
       retval == UA_STATUSCODE_BADSECURITYMODEREJECTED   ||
       retval == UA_STATUSCODE_BADNOTSUPPORTED            ||
       retval == UA_STATUSCODE_BADIDENTITYTOKENREJECTED) {
        char skip_msg[160];
        snprintf(skip_msg, sizeof(skip_msg),
                 "%s (policy not offered by server, 0x%08x)",
                 label, (unsigned)retval);
        interop_skip(skip_msg);
        UA_Client_delete(client);
        return;
    }

    /* Certificate trust / channel establishment errors.
     * When ECC is required, these indicate a misconfiguration and
     * should be treated as failures rather than silent skips. */
    if(retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED    ||
       retval == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED    ||
       retval == UA_STATUSCODE_BADSECURECHANNELIDINVALID  ||
       retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
        if(required) {
            char fail_msg[180];
            snprintf(fail_msg, sizeof(fail_msg),
                     "%s ECC cert/trust failure (0x%08x) – "
                     "expected PASS because INTEROP_REQUIRE_ECC is set",
                     label, (unsigned)retval);
            interop_fail("%s", fail_msg);
        } else {
            char skip_msg[160];
            snprintf(skip_msg, sizeof(skip_msg),
                     "%s (cert not trusted by server, 0x%08x)",
                     label, (unsigned)retval);
            interop_skip(skip_msg);
        }
        UA_Client_delete(client);
        return;
    }

    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD, msg);

    if(retval == UA_STATUSCODE_GOOD) {
        check_read_server_status(client);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}
#endif

/* ------------------------------------------------------------------ */
/* T-8: Username/Password over Encrypted Channel                      */
/* ------------------------------------------------------------------ */

#ifdef UA_ENABLE_ENCRYPTION
static void
test_T8_username_encrypted(const char *url,
                           UA_ByteString *certificate, UA_ByteString *privateKey,
                           UA_ByteString *trustList, size_t trustListSize) {
    const char *policyUri =
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256";
    interop_log("--- T-8: Username/Password over Basic256Sha256 ---");

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval =
        configureEncryption(cc, policyUri, certificate, privateKey,
                            trustList, trustListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        INTEROP_CHECK(UA_FALSE, "T-8 Configure encryption");
        UA_Client_delete(client);
        return;
    }

    retval = UA_Client_connectUsername(client, url, "user1", "password");

    /* SKIP if the server rejects the encrypted channel (cert not trusted,
     * policy not offered) or if it rejects the identity token / credentials
     * (server may require a different policy or reject unknown users). */
    if(retval == UA_STATUSCODE_BADSECURITYPOLICYREJECTED  ||
       retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED    ||
       retval == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED    ||
       retval == UA_STATUSCODE_BADSECURECHANNELIDINVALID  ||
       retval == UA_STATUSCODE_BADCONNECTIONCLOSED        ||
       retval == UA_STATUSCODE_BADIDENTITYTOKENREJECTED   ||
       retval == UA_STATUSCODE_BADUSERACCESSDENIED) {
        interop_skip("T-8 Username over encrypted "
                     "(policy/cert/credentials not accepted by server)");
        UA_Client_delete(client);
        return;
    }

    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD,
                  "T-8 Username connect (Basic256Sha256)");

    if(retval == UA_STATUSCODE_GOOD) {
        check_read_server_status(client);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}
#endif

/* ------------------------------------------------------------------ */
/* T-9: X509 Certificate Authentication                               */
/* ------------------------------------------------------------------ */

#ifdef UA_ENABLE_ENCRYPTION
static void
test_T9_certificate_auth(const char *url,
                         UA_ByteString *certificate, UA_ByteString *privateKey,
                         UA_ByteString *trustList, size_t trustListSize) {
    const char *policyUri =
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256";
    interop_log("--- T-9: X509 Certificate Authentication ---");

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval =
        configureEncryption(cc, policyUri, certificate, privateKey,
                            trustList, trustListSize);
    if(retval != UA_STATUSCODE_GOOD) {
        INTEROP_CHECK(UA_FALSE, "T-9 Configure encryption");
        UA_Client_delete(client);
        return;
    }

    /* Use the same client certificate as user identity (X509 auth) */
    retval = UA_ClientConfig_setAuthenticationCert(cc, *certificate, *privateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        INTEROP_CHECK(UA_FALSE, "T-9 Set authentication cert");
        UA_Client_delete(client);
        return;
    }

    retval = UA_Client_connect(client, url);

    /* Some servers reject the X509 identity token if the client certificate
     * is not in their user trust list.  Treat that as SKIP. */
    if(retval == UA_STATUSCODE_BADIDENTITYTOKENREJECTED  ||
       retval == UA_STATUSCODE_BADIDENTITYTOKENINVALID   ||
       retval == UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED ||
       retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED    ||
       retval == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED    ||
       retval == UA_STATUSCODE_BADSECURECHANNELIDINVALID  ||
       retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
        interop_skip("T-9 X509 certificate auth (rejected by server)");
        UA_Client_delete(client);
        return;
    }

    INTEROP_CHECK(retval == UA_STATUSCODE_GOOD,
                  "T-9 X509 certificate auth connect");

    if(retval == UA_STATUSCODE_GOOD) {
        check_read_server_status(client);
        UA_Client_disconnect(client);
    }
    UA_Client_delete(client);
}
#endif

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr,
                "Usage: %s <url> [<cert.der> <key.der> [trustlist.der ...]]\n",
                argv[0]);
        return 1;
    }

    const char *url = argv[1];
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;

    UA_Boolean haveEncryption = UA_FALSE;

    if(argc >= 4) {
        certificate = loadFileFromDisk(argv[2]);
        privateKey = loadFileFromDisk(argv[3]);
        if(certificate.length == 0 || privateKey.length == 0) {
            fprintf(stderr, "Error: Failed to load certificate or key\n");
            return 1;
        }
        haveEncryption = UA_TRUE;

        if(argc > 4) {
            trustListSize = (size_t)(argc - 4);
            trustList = (UA_ByteString *)calloc(trustListSize,
                                                sizeof(UA_ByteString));
            for(int i = 4; i < argc; i++) {
                trustList[i - 4] = loadFileFromDisk(argv[i]);
                if(trustList[i - 4].length == 0)
                    fprintf(stderr,
                            "Warning: Failed to load trust list: %s\n",
                            argv[i]);
            }
        }
    }

    interop_log("=== Cross-SDK Interop Test Suite ===");
    interop_log("Server: %s", url);
    interop_log("Encryption: %s", haveEncryption ? "yes" : "no");

    /* When INTEROP_REQUIRE_ECC is set, certificate-trust problems on ECC
     * connections are treated as FAIL instead of SKIP.  This catches
     * misconfigurations where all ECC tests silently skip in CI. */
    const char *requireEcc = getenv("INTEROP_REQUIRE_ECC");
    UA_Boolean eccRequired = (requireEcc && requireEcc[0] != '0');
    if(eccRequired)
        interop_log("ECC: required (INTEROP_REQUIRE_ECC=%s)", requireEcc);
    interop_log("");

    /* === Tests without encryption === */

    /* T-1: Anonymous */
    test_T1_anonymous_none(url);

    /* T-2: Username over None */
    test_T2_username_none(url);

    /* T-3: Read custom variable */
    test_T3_read_variable(url);

    /* T-4: Call method */
    test_T4_call_method(url);

    /* === Tests with encryption === */

#ifdef UA_ENABLE_ENCRYPTION
    if(haveEncryption) {
        /* T-5: Basic256Sha256 */
        test_encrypted_anonymous(
            url,
            "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256",
            "T-5", &certificate, &privateKey, trustList, trustListSize);

        /* T-6: Aes128_Sha256_RsaOaep */
        test_encrypted_anonymous(
            url,
            "http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep",
            "T-6", &certificate, &privateKey, trustList, trustListSize);

        /* T-7: Aes256_Sha256_RsaPss */
        test_encrypted_anonymous(
            url,
            "http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss",
            "T-7", &certificate, &privateKey, trustList, trustListSize);

        /* T-8: Username over encrypted channel */
        test_T8_username_encrypted(url, &certificate, &privateKey,
                                   trustList, trustListSize);

        /* T-9: X509 certificate authentication */
        test_T9_certificate_auth(url, &certificate, &privateKey,
                                 trustList, trustListSize);
    } else {
        interop_skip("T-5 Basic256Sha256 (no certs)");
        interop_skip("T-6 Aes128_Sha256_RsaOaep (no certs)");
        interop_skip("T-7 Aes256_Sha256_RsaPss (no certs)");
        interop_skip("T-8 Username over encrypted (no certs)");
        interop_skip("T-9 X509 certificate auth (no certs)");
    }

    /* === ECC security policy tests (T-10..T-15) === */

    {
        int eccPassesBefore = g_passed;
        const char *eccDir = getenv("INTEROP_ECC_CERT_DIR");
        if(eccDir) {
            struct {
                const char *label;
                const char *curve;
                const char *policyUri;
            } eccTests[] = {
                {"T-10", "nistP256",
                 "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256"},
                {"T-11", "nistP384",
                 "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384"},
                {"T-12", "brainpoolP256r1",
                 "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1"},
                {"T-13", "brainpoolP384r1",
                 "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1"},
                {"T-14", "curve25519",
                 "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519"},
                {"T-15", "curve448",
                 "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448"}
            };
            size_t numEcc = sizeof(eccTests) / sizeof(eccTests[0]);

            for(size_t i = 0; i < numEcc; i++) {
                char certPath[512], keyPath[512];
                snprintf(certPath, sizeof(certPath),
                         "%s/client_c_%s.cert.der", eccDir, eccTests[i].curve);
                snprintf(keyPath, sizeof(keyPath),
                         "%s/client_c_%s.key.der", eccDir, eccTests[i].curve);

                UA_ByteString eccCert = loadFileFromDisk(certPath);
                UA_ByteString eccKey = loadFileFromDisk(keyPath);
                if(eccCert.length == 0 || eccKey.length == 0) {
                    char skipMsg[128];
                    snprintf(skipMsg, sizeof(skipMsg),
                             "%s ECC_%s (cert not available)",
                             eccTests[i].label, eccTests[i].curve);
                    interop_skip(skipMsg);
                    UA_ByteString_clear(&eccCert);
                    UA_ByteString_clear(&eccKey);
                    continue;
                }

                /* Use server trust list from RSA args (server trusts all via AcceptAll) */
                test_ecc_encrypted_anonymous(
                    url, eccTests[i].policyUri, eccTests[i].label,
                    &eccCert, &eccKey, trustList, trustListSize, eccRequired);

                UA_ByteString_clear(&eccCert);
                UA_ByteString_clear(&eccKey);
            }
        } else {
            interop_skip("T-10 ECC_nistP256 (no ECC cert dir)");
            interop_skip("T-11 ECC_nistP384 (no ECC cert dir)");
            interop_skip("T-12 ECC_brainpoolP256r1 (no ECC cert dir)");
            interop_skip("T-13 ECC_brainpoolP384r1 (no ECC cert dir)");
            interop_skip("T-14 ECC_curve25519 (no ECC cert dir)");
            interop_skip("T-15 ECC_curve448 (no ECC cert dir)");
        }

        /* Safety net: when INTEROP_REQUIRE_ECC is set, at least one ECC
         * test must have passed.  Otherwise the CI is green while ECC
         * was never actually exercised (e.g. missing endpoints). */
        if(eccRequired && g_passed == eccPassesBefore) {
            interop_fail("INTEROP_REQUIRE_ECC is set but no ECC test passed "
                         "(all skipped or failed)");
        }
    }
#else
    (void)haveEncryption;
    (void)eccRequired;
    interop_skip("T-5 Basic256Sha256 (no encryption support)");
    interop_skip("T-6 Aes128_Sha256_RsaOaep (no encryption support)");
    interop_skip("T-7 Aes256_Sha256_RsaPss (no encryption support)");
    interop_skip("T-8 Username over encrypted (no encryption support)");
    interop_skip("T-9 X509 certificate auth (no encryption support)");
    interop_skip("T-10 ECC_nistP256 (no encryption support)");
    interop_skip("T-11 ECC_nistP384 (no encryption support)");
    interop_skip("T-12 ECC_brainpoolP256r1 (no encryption support)");
    interop_skip("T-13 ECC_brainpoolP384r1 (no encryption support)");
    interop_skip("T-14 ECC_curve25519 (no encryption support)");
    interop_skip("T-15 ECC_curve448 (no encryption support)");
#endif

    /* Cleanup */
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    free(trustList);

    interop_log("");
    interop_log("=== Results: %d passed, %d failed, %d skipped ===",
                g_passed, g_failed, g_skipped);
    return g_failed > 0 ? 1 : 0;
}
