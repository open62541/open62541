/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 */

/*
modification history
--------------------
21feb20,lan  written
*/

#include <open62541/server_config.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_ENCRYPTION_OPENSSL

UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateVerification *cv,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize) {
    return UA_STATUSCODE_GOOD; /* TODO: implement later */ 
}

UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder) {
    return UA_STATUSCODE_GOOD; /* TODO: implement later */ 
}

#endif  /* end of UA_ENABLE_ENCRYPTION_OPENSSL */