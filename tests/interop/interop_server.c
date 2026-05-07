/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Standalone OPC UA server for cross-SDK interoperability tests.
 * Provides the address space required by check_interop_client and the
 * other SDK interop test suites (see tests/interop/README.md).
 *
 * Usage:
 *   interop_server <port> <server-cert.der> <private-key.der> [<trustlist.der> ...]
 *
 *   Set env INTEROP_ECC_CERT_DIR to a directory containing ECC certificates
 *   named  server_c_<curve>.cert.der / server_c_<curve>.key.der  to expose
 *   ECC security policies.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
#include <mbedtls/version.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* ------------------------------------------------------------------ */
/* File loading helper (inlined to avoid dependency on examples/)      */
/* ------------------------------------------------------------------ */

static UA_ByteString
loadFile(const char *path) {
    UA_ByteString fileContents = UA_STRING_NULL;
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0;
        return fileContents;
    }
    if(fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        errno = 0;
        return fileContents;
    }
    long length = ftell(fp);
    if(length < 0) {
        fclose(fp);
        errno = 0;
        return fileContents;
    }
    fileContents.length = (size_t)length;
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        if(fseek(fp, 0, SEEK_SET) != 0) {
            fclose(fp);
            UA_ByteString_clear(&fileContents);
            errno = 0;
            return fileContents;
        }
        size_t rd = fread(fileContents.data, sizeof(UA_Byte),
                          fileContents.length, fp);
        if(rd != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);
    return fileContents;
}

/* ------------------------------------------------------------------ */
/* Address space: users, variable, method                              */
/* ------------------------------------------------------------------ */

static UA_UsernamePasswordLogin logins[3] = {
    {UA_STRING_STATIC("peter"), UA_STRING_STATIC("peter123")},
    {UA_STRING_STATIC("paula"), UA_STRING_STATIC("paula123")},
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")}
};

static UA_StatusCode
helloWorldMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    if(inputSize < 1 || !input[0].data ||
       input[0].type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_String *inputStr = (UA_String *)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        UA_Byte *newData = (UA_Byte *)UA_realloc(tmp.data,
                                                 tmp.length + inputStr->length);
        if(!newData) {
            UA_String_clear(&tmp);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        tmp.data = newData;
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    return UA_STATUSCODE_GOOD;
}

static void
addHelloWorldMethod(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US", "Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Hello World");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;

    /* Create an InteropTests container object under ObjectsFolder.
     * Methods must use HasComponent references which some SDKs reject
     * directly under ObjectsFolder (Organizes-only). */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "InteropTests");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1000),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
                            UA_QUALIFIEDNAME(1, "InteropTests"),
                            UA_NS0ID(BASEOBJECTTYPE), oAttr, NULL, NULL);

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62541),
                            UA_NODEID_NUMERIC(1, 1000), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

static void
addVariable(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NS0ID(BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void
writeVariable(UA_Server *server) {
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");

    UA_Int32 myInteger = 43;
    UA_Variant myVar;
    UA_Variant_init(&myVar);
    UA_Variant_setScalar(&myVar, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, myIntegerNodeId, myVar);

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myIntegerNodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.status = UA_STATUSCODE_BADNOTCONNECTED;
    wv.value.hasStatus = true;
    UA_Server_write(server, &wv);

    /* Reset the variable to a good statuscode with a value */
    wv.value.hasStatus = false;
    wv.value.value = myVar;
    wv.value.hasValue = true;
    UA_Server_write(server, &wv);
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    UA_StatusCode retval = 0;

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

#ifdef UA_ENABLE_ENCRYPTION
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_UInt16 port = 0;
    if(argc >= 4) {
        port = (UA_UInt16)atoi(argv[1]);
        certificate = loadFile(argv[2]);
        privateKey = loadFile(argv[3]);
    } else {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Missing arguments. Arguments are "
                     "<port> <server-certificate.der> <private-key.der> "
                     "[<trustlist1.der>, ...]");
        return EXIT_FAILURE;
    }

    size_t trustListSize = 0;
    if(argc > 4)
        trustListSize = (size_t)argc - 4;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize + 1);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(argv[i + 4]);

    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;

    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    retval =
        UA_ServerConfig_setDefaultWithSecurityPolicies(config, port,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList,
                                                       revocationListSize);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* --- ECC security policies (optional) ---
     * ECC policy APIs are only available with OpenSSL, LibreSSL or
     * mbedTLS >= 3.0.  Guard with the same condition as the
     * implementation in ua_config_default.c. */
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) || \
    (defined(UA_ENABLE_ENCRYPTION_MBEDTLS) && defined(MBEDTLS_VERSION_NUMBER) && \
     MBEDTLS_VERSION_NUMBER >= 0x03000000)
    {
        const char *eccDir = getenv("INTEROP_ECC_CERT_DIR");
        if(eccDir) {
            struct {
                const char *curve;
                UA_StatusCode (*addPolicy)(UA_ServerConfig *,
                                          const UA_ByteString *,
                                          const UA_ByteString *);
            } eccPolicies[] = {
                {"nistP256",        UA_ServerConfig_addSecurityPolicyEccNistP256},
                {"nistP384",        UA_ServerConfig_addSecurityPolicyEccNistP384},
                {"brainpoolP256r1", UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1},
                {"brainpoolP384r1", UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1},
                {"curve25519",      UA_ServerConfig_addSecurityPolicyEccCurve25519},
                {"curve448",        UA_ServerConfig_addSecurityPolicyEccCurve448}
            };
            size_t numEcc = sizeof(eccPolicies) / sizeof(eccPolicies[0]);

            for(size_t i = 0; i < numEcc; i++) {
                char certPath[512], keyPath[512];
                snprintf(certPath, sizeof(certPath),
                         "%s/server_c_%s.cert.der", eccDir, eccPolicies[i].curve);
                snprintf(keyPath, sizeof(keyPath),
                         "%s/server_c_%s.key.der", eccDir, eccPolicies[i].curve);

                UA_ByteString eccCert = loadFile(certPath);
                UA_ByteString eccKey  = loadFile(keyPath);
                if(eccCert.length == 0 || eccKey.length == 0) {
                    UA_ByteString_clear(&eccCert);
                    UA_ByteString_clear(&eccKey);
                    continue;  /* cert not generated for this curve */
                }

                UA_StatusCode rv = eccPolicies[i].addPolicy(config, &eccCert, &eccKey);
                if(rv == UA_STATUSCODE_GOOD) {
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                                "Added ECC policy: ECC_%s", eccPolicies[i].curve);
                } else {
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                                  "Could not add ECC_%s: 0x%08x",
                                  eccPolicies[i].curve, (unsigned)rv);
                }
                UA_ByteString_clear(&eccCert);
                UA_ByteString_clear(&eccKey);
            }

            /* addSecurityPolicyEcc* only adds the policy – endpoints must
             * be created separately so that GetEndpoints returns them.
             * addAllEndpoints has built-in duplicate detection and will
             * skip the RSA endpoints that already exist. */
            UA_ServerConfig_addAllEndpoints(config);
        }
    }
#endif /* ECC available */
#endif

    /* Accept all client certificates – the interop tests validate ECC
     * crypto correctness, not certificate trust management. */
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    retval = UA_AccessControl_default(config, true,
             &config->securityPolicies[config->securityPoliciesSize-1].policyUri,
             3, logins);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    addHelloWorldMethod(server);
    addVariable(server);
    writeVariable(server);

    retval = UA_Server_runUntilInterrupt(server);

    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
 cleanup:
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
