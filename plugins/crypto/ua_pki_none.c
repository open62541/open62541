/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 */

#include <open62541/plugin/pki_default.h>

static UA_StatusCode
verifyCertificateAllowAll(UA_CertificateManager *certificateManager,
                          UA_PKIStore *pkiStore,
                          const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
verifyApplicationURIAllowAll(UA_CertificateManager *verificationContext,
                             UA_PKIStore *pkiStore,
                             const UA_ByteString *certificate,
                             const UA_String *applicationURI) {
    return UA_STATUSCODE_GOOD;
}

static void
clearVerifyAllowAll(UA_CertificateManager *cv) {

}

void
UA_CertificateManager_AcceptAll(UA_CertificateManager *cv) {
    /* Clear the structure, as it may have already been initialized. */
    if(cv->clear) {
        cv->clear(cv);
    }

    cv->verifyCertificate = verifyCertificateAllowAll;
    cv->verifyApplicationURI = verifyApplicationURIAllowAll;
    cv->clear = clearVerifyAllowAll;
}
