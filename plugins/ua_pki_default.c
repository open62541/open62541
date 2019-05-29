/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#include <open62541/plugin/pki_default.h>

#ifdef UA_ENABLE_ENCRYPTION
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#endif

/************/
/* AllowAll */
/************/

static UA_StatusCode
verifyCertificateAllowAll(void *verificationContext,
               const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
verifyApplicationURIAllowAll(void *verificationContext,
                             const UA_ByteString *certificate,
                             const UA_String *applicationURI) {
    return UA_STATUSCODE_GOOD;
}

static void
deleteVerifyAllowAll(UA_CertificateVerification *cv) {

}

void UA_CertificateVerification_AcceptAll(UA_CertificateVerification *cv) {
    cv->verifyCertificate = verifyCertificateAllowAll;
    cv->verifyApplicationURI = verifyApplicationURIAllowAll;
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

    /* This condition will check whether the certificate is a User certificate
     * or a CA certificate. If the MBEDTLS_X509_KU_KEY_CERT_SIGN and
     * MBEDTLS_X509_KU_CRL_SIGN of key_usage are set, then the certificate
     * shall be condidered as CA Certificate and cannot be used to establish a
     * connection. Refer the test case CTT/Security/Security Certificate Validation/029.js
     * for more details */
    if((remoteCertificate.key_usage & MBEDTLS_X509_KU_KEY_CERT_SIGN) &&
       (remoteCertificate.key_usage & MBEDTLS_X509_KU_CRL_SIGN)) {
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(mbedErr) {
        /* char buff[100]; */
        /* mbedtls_x509_crt_verify_info(buff, 100, "", flags); */
        /* UA_LOG_ERROR(channelContextData->policyContext->securityPolicy->logger, */
        /*              UA_LOGCATEGORY_SECURITYPOLICY, */
        /*              "Verifying the certificate failed with error: %s", buff); */

        if(flags & (uint32_t)MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
            retval = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
        } else if(flags & (uint32_t)MBEDTLS_X509_BADCERT_FUTURE ||
                  flags & (uint32_t)MBEDTLS_X509_BADCERT_EXPIRED) {
            retval = UA_STATUSCODE_BADCERTIFICATETIMEINVALID;
        } else if(flags & (uint32_t)MBEDTLS_X509_BADCERT_REVOKED ||
                  flags & (uint32_t)MBEDTLS_X509_BADCRL_EXPIRED) {
            retval = UA_STATUSCODE_BADCERTIFICATEREVOKED;
        } else {
            retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        }
    }

    mbedtls_x509_crt_free(&remoteCertificate);
    return retval;
}

/* Find binary substring. Taken and adjusted from
 * http://tungchingkai.blogspot.com/2011/07/binary-strstr.html */

static const unsigned char *
bstrchr(const unsigned char *s, const unsigned char ch, size_t l) {
    /* find first occurrence of c in char s[] for length l*/
    /* handle special case */
    if(l == 0)
        return (NULL);

    for(; *s != ch; ++s, --l)
        if(l == 0)
            return (NULL);
    return s;
}

static const unsigned char *
bstrstr(const unsigned char *s1, size_t l1, const unsigned char *s2, size_t l2) {
    /* find first occurrence of s2[] in s1[] for length l1*/
    const unsigned char *ss1 = s1;
    const unsigned char *ss2 = s2;
    /* handle special case */
    if(l1 == 0)
        return (NULL);
    if(l2 == 0)
        return s1;

    /* match prefix */
    for (; (s1 = bstrchr(s1, *s2, (uintptr_t)ss1-(uintptr_t)s1+(uintptr_t)l1)) != NULL &&
             (uintptr_t)ss1-(uintptr_t)s1+(uintptr_t)l1 != 0; ++s1) {

        /* match rest of prefix */
        const unsigned char *sc1, *sc2;
        for (sc1 = s1, sc2 = s2; ;)
            if (++sc2 >= ss2+l2)
                return s1;
            else if (*++sc1 != *sc2)
                break;
    }
    return NULL;
}

static UA_StatusCode
certificateVerification_verifyApplicationURI(void *verificationContext,
                                             const UA_ByteString *certificate,
                                             const UA_String *applicationURI) {
    CertInfo *ci = (CertInfo*)verificationContext;
    if(!ci)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    mbedtls_x509_crt remoteCertificate;
    mbedtls_x509_crt_init(&remoteCertificate);
    int mbedErr = mbedtls_x509_crt_parse(&remoteCertificate, certificate->data,
                                         certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Poor man's ApplicationUri verification. mbedTLS does not parse all fields
     * of the Alternative Subject Name. Instead test whether the URI-string is
     * present in the v3_ext field in general.
     *
     * TODO: Improve parsing of the Alternative Subject Name */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(bstrstr(remoteCertificate.v3_ext.p, remoteCertificate.v3_ext.len,
               applicationURI->data, applicationURI->length) == NULL)
        retval = UA_STATUSCODE_BADCERTIFICATEURIINVALID;

    mbedtls_x509_crt_free(&remoteCertificate);
    return retval;
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
    CertInfo *ci = (CertInfo*)UA_malloc(sizeof(CertInfo));
    if(!ci)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mbedtls_x509_crt_init(&ci->certificateTrustList);
    mbedtls_x509_crl_init(&ci->certificateRevocationList);

    cv->context = (void*)ci;
    if(certificateTrustListSize > 0)
        cv->verifyCertificate = certificateVerification_verify;
    else
        cv->verifyCertificate = verifyCertificateAllowAll;
    cv->deleteMembers = certificateVerification_deleteMembers;
    cv->verifyApplicationURI = certificateVerification_verifyApplicationURI;

    int err = 0;
    for(size_t i = 0; i < certificateTrustListSize; i++) {
        err = mbedtls_x509_crt_parse(&ci->certificateTrustList,
                                     certificateTrustList[i].data,
                                     certificateTrustList[i].length);
        if(err)
            goto error;
    }
    for(size_t i = 0; i < certificateRevocationListSize; i++) {
        err = mbedtls_x509_crl_parse(&ci->certificateRevocationList,
                                     certificateRevocationList[i].data,
                                     certificateRevocationList[i].length);
        if(err)
            goto error;
    }

    return UA_STATUSCODE_GOOD;
error:
    certificateVerification_deleteMembers(cv);
    return UA_STATUSCODE_BADINTERNALERROR;
}

#endif
