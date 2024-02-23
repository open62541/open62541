/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/plugin/certificategroup_default.h>

static UA_StatusCode
verifyCertificateAllowAll(UA_CertificateGroup *certGroup,
                          const UA_ByteString *certificate) {
    UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_USERLAND,
                   "No certificate store configured. Accepting the certificate.");
    return UA_STATUSCODE_GOOD;
}

static void
clearVerifyAllowAll(UA_CertificateGroup *certGroup) {

}

void UA_CertificateVerification_AcceptAll(UA_CertificateGroup *certGroup) {
    /* Clear the structure, as it may have already been initialized. */
    if(certGroup->clear)
        certGroup->clear(certGroup);
    certGroup->verifyCertificate = verifyCertificateAllowAll;
    certGroup->clear = clearVerifyAllowAll;
    certGroup->getTrustList = NULL;
    certGroup->setTrustList = NULL;
    certGroup->addToTrustList = NULL;
    certGroup->removeFromTrustList = NULL;
}

#ifndef UA_ENABLE_ENCRYPTION
UA_StatusCode
UA_CertificateUtils_verifyApplicationURI(UA_RuleHandling ruleHandling,
                                         const UA_ByteString *certificate,
                                         const UA_String *applicationURI){
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CertificateUtils_getExpirationDate(UA_ByteString *certificate,
                                      UA_DateTime *expiryDateTime){
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

UA_StatusCode
UA_CertificateUtils_getSubjectName(UA_ByteString *certificate,
                                   UA_String *subjectName){
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

UA_StatusCode
UA_CertificateUtils_getThumbprint(UA_ByteString *certificate,
                                  UA_String *thumbprint){
    return UA_STATUSCODE_BADNOTSUPPORTED;
}
#endif
