/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#include <open62541/plugin/pki_default.h>

static UA_StatusCode
verifyCertificateAllowAll(const UA_CertificateVerification *cv,
                          const UA_ByteString *certificate) {
    UA_LOG_WARNING(cv->logging, UA_LOGCATEGORY_USERLAND,
                   "No certificate store configured. Accepting the certificate.");
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
verifyApplicationURIAllowAll(const UA_CertificateVerification *cv,
                             const UA_ByteString *certificate,
                             const UA_String *applicationURI) {
    return UA_STATUSCODE_GOOD;
}

static void
clearVerifyAllowAll(UA_CertificateVerification *cv) {

}

void UA_CertificateVerification_AcceptAll(UA_CertificateVerification *cv) {
    /* Clear the structure, as it may have already been initialized. */
    if(cv->clear)
        cv->clear(cv);
    cv->verifyCertificate = verifyCertificateAllowAll;
    cv->verifyApplicationURI = verifyApplicationURIAllowAll;
    cv->clear = clearVerifyAllowAll;
}
