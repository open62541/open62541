/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_pki_certificate.h"

#ifdef UA_ENABLE_ENCRYPTION
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#endif

/************/
/* AllowAll */
/************/

static UA_StatusCode
verifyAllowAll(void *verificationContext, const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static void
deleteVerifyAllowAll(UA_CertificateVerification *cv) {

}

void UA_CertificateVerification_AcceptAll(UA_CertificateVerification *cv) {
    cv->verifyCertificate = verifyAllowAll;
    cv->deleteMembers = deleteVerifyAllowAll;
}

#ifdef UA_ENABLE_ENCRYPTION

typedef struct {
    mbedtls_x509_crt certificateTrustList;
    mbedtls_x509_crl certificateRevocationList;
} CertInfo;

static UA_StatusCode
certificateVerification_verify(void *verificationContext,
                               const UA_ByteString *certificate) {
    CertInfo *ci = (CertInfo*)verificationContext;
    if(!ci)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    mbedtls_x509_crt remoteCertificate;
    mbedtls_x509_crt_init(&remoteCertificate);
    int mbedErr = mbedtls_x509_crt_parse(&remoteCertificate, certificate->data,
                                         certificate->length);
    if(mbedErr) {
        /* char errBuff[300]; */
        /* mbedtls_strerror(mbedErr, errBuff, 300); */
        /* UA_LOG_WARNING(data->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY, */
        /*                "Could not parse the remote certificate with error: %s", errBuff); */
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    /* Verify */
    mbedtls_x509_crt_profile crtProfile = {
        MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1) | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA256),
        0xFFFFFF, 0x000000, 128 * 8 // in bits
    }; // TODO: remove magic numbers

    uint32_t flags = 0;
    mbedErr = mbedtls_x509_crt_verify_with_profile(&remoteCertificate,
                                                   &ci->certificateTrustList,
                                                   &ci->certificateRevocationList,
                                                   &crtProfile, NULL, &flags, NULL, NULL);

    // TODO: Extend verification
    if(mbedErr) {
        /* char buff[100]; */
        /* mbedtls_x509_crt_verify_info(buff, 100, "", flags); */
        /* UA_LOG_ERROR(channelContextData->policyContext->securityPolicy->logger, */
        /*              UA_LOGCATEGORY_SECURITYPOLICY, */
        /*              "Verifying the certificate failed with error: %s", buff); */

        mbedtls_x509_crt_free(&remoteCertificate);
        if(flags & MBEDTLS_X509_BADCERT_NOT_TRUSTED)
            return UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;

        if(flags & MBEDTLS_X509_BADCERT_FUTURE ||
           flags & MBEDTLS_X509_BADCERT_EXPIRED)
            return UA_STATUSCODE_BADCERTIFICATETIMEINVALID;

        if(flags & MBEDTLS_X509_BADCERT_REVOKED ||
           flags & MBEDTLS_X509_BADCRL_EXPIRED)
            return UA_STATUSCODE_BADCERTIFICATEREVOKED;

        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    mbedtls_x509_crt_free(&remoteCertificate);
    return UA_STATUSCODE_GOOD;
}

static void
certificateVerification_deleteMembers(UA_CertificateVerification *cv) {
    CertInfo *ci = (CertInfo*)cv->context;
    if(!ci)
        return;
    mbedtls_x509_crt_free(&ci->certificateTrustList);
    mbedtls_x509_crl_free(&ci->certificateRevocationList);
    UA_free(ci);
    cv->context = NULL;
}

UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateVerification *cv,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize) {
    CertInfo *ci = (CertInfo*)malloc(sizeof(CertInfo));
    if(!ci)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mbedtls_x509_crt_init(&ci->certificateTrustList);
    mbedtls_x509_crl_init(&ci->certificateRevocationList);

    cv->context = (void*)ci;
    if(certificateTrustListSize > 0)
        cv->verifyCertificate = certificateVerification_verify;
    else
        cv->verifyCertificate = verifyAllowAll;
    cv->deleteMembers = certificateVerification_deleteMembers;

    int err = 0;
    for(size_t i = 0; i < certificateTrustListSize; i++) {
        err |= mbedtls_x509_crt_parse(&ci->certificateTrustList,
                                      certificateTrustList[i].data,
                                      certificateTrustList[i].length);
    }
    for(size_t i = 0; i < certificateRevocationListSize; i++) {
        err |= mbedtls_x509_crl_parse(&ci->certificateRevocationList,
                                      certificateRevocationList[i].data,
                                      certificateRevocationList[i].length);
    }

    if(err) {
        certificateVerification_deleteMembers(cv);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

#endif
