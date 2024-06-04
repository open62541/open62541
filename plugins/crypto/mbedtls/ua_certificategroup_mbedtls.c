/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2019 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/util.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/entropy.h>
#include <mbedtls/version.h>

#include "securitypolicy_mbedtls_common.h"

#define REMOTECERTIFICATETRUSTED 1
#define ISSUERKNOWN              2
#define DUALPARENT               3
#define PARENTFOUND              4

/* Configuration parameters */

#define MEMORYCERTSTORE_PARAMETERSSIZE 2
#define MEMORYCERTSTORE_PARAMINDEX_MAXTRUSTLISTSIZE 0
#define MEMORYCERTSTORE_PARAMINDEX_MAXREJECTEDLISTSIZE 1

static const struct {
    UA_QualifiedName name;
    const UA_DataType *type;
    UA_Boolean required;
} MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("max-trust-listsize")}, &UA_TYPES[UA_TYPES_UINT16], false},
    {{0, UA_STRING_STATIC("max-rejected-listsize")}, &UA_TYPES[UA_TYPES_STRING], false}
};

struct MemoryCertStore;
typedef struct MemoryCertStore MemoryCertStore;

struct MemoryCertStore {
    UA_TrustListDataType trustList;
    size_t rejectedCertificatesSize;
    UA_ByteString *rejectedCertificates;

    UA_UInt32 maxTrustListSize;
    UA_UInt32 maxRejectedListSize;

    UA_Boolean reloadRequired;

    mbedtls_x509_crt trustedCertificates;
    mbedtls_x509_crt issuerCertificates;
    mbedtls_x509_crl trustedCrls;
    mbedtls_x509_crl issuerCrls;
};

static UA_StatusCode
MemoryCertStore_removeFromTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    context->reloadRequired = true;
    return UA_TrustListDataType_remove(trustList, &context->trustList);
}

static UA_StatusCode
MemoryCertStore_getTrustList(UA_CertificateGroup *certGroup, UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    return UA_TrustListDataType_copy(&context->trustList, trustList);
}

static UA_StatusCode
MemoryCertStore_setTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    if(context->maxTrustListSize != 0 && UA_TrustListDataType_getSize(trustList) > context->maxTrustListSize) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    context->reloadRequired = true;
    UA_TrustListDataType_clear(&context->trustList);
    return UA_TrustListDataType_add(trustList, &context->trustList);
}

static UA_StatusCode
MemoryCertStore_addToTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(certGroup == NULL || trustList == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    if(context->maxTrustListSize != 0 && UA_TrustListDataType_getSize(&context->trustList) + UA_TrustListDataType_getSize(trustList) > context->maxTrustListSize) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    context->reloadRequired = true;
    return UA_TrustListDataType_add(trustList, &context->trustList);
}

static UA_StatusCode
MemoryCertStore_getRejectedList(UA_CertificateGroup *certGroup, UA_ByteString **rejectedList, size_t *rejectedListSize) {
    /* Check parameter */
    if(certGroup == NULL || rejectedList == NULL || rejectedListSize == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    UA_StatusCode retval = UA_Array_copy(context->rejectedCertificates, context->rejectedCertificatesSize,
                                         (void**)rejectedList, &UA_TYPES[UA_TYPES_BYTESTRING]);

    if(retval == UA_STATUSCODE_GOOD)
        *rejectedListSize = context->rejectedCertificatesSize;

    return retval;
}

static UA_StatusCode
MemoryCertStore_addToRejectedList(UA_CertificateGroup *certGroup, const UA_ByteString *certificate) {
    /* Check parameter */
    if(certGroup == NULL || certificate == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;

    /* check duplicate certificate */
    for(size_t i = 0; i < context->rejectedCertificatesSize; i++) {
        if(UA_ByteString_equal(certificate, &context->rejectedCertificates[i]))
            return UA_STATUSCODE_GOOD; /* certificate already exist */
    }

    /* Store rejected certificate */
    if(context->maxRejectedListSize == 0 || context->rejectedCertificatesSize < context->maxRejectedListSize) {
        return UA_Array_appendCopy((void**)&context->rejectedCertificates, &context->rejectedCertificatesSize,
                                   certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    }
    UA_Array_delete(context->rejectedCertificates, context->rejectedCertificatesSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    context->rejectedCertificates = NULL;
    context->rejectedCertificatesSize = 0;
    return UA_Array_appendCopy((void**)&context->rejectedCertificates, &context->rejectedCertificatesSize,
                               certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
}

static void
MemoryCertStore_clear(UA_CertificateGroup *certGroup) {
    /* check parameter */
    if(certGroup == NULL) {
        return;
    }

    UA_NodeId_clear(&certGroup->certificateGroupId);

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    if(context) {
        UA_TrustListDataType_clear(&context->trustList);

        UA_Array_delete(context->rejectedCertificates, context->rejectedCertificatesSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
        context->rejectedCertificates = NULL;
        context->rejectedCertificatesSize = 0;

        mbedtls_x509_crt_free(&context->trustedCertificates);
        mbedtls_x509_crt_free(&context->issuerCertificates);
        mbedtls_x509_crl_free(&context->trustedCrls);
        mbedtls_x509_crl_free(&context->issuerCrls);

        UA_free(context);
        certGroup->context = NULL;
    }
}

static UA_StatusCode
reloadCertificates(UA_CertificateGroup *certGroup) {
    /* Check parameter */
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    UA_ByteString data;
    UA_ByteString_init(&data);
    int err = 0;

    mbedtls_x509_crt_free(&context->trustedCertificates);
    mbedtls_x509_crt_init(&context->trustedCertificates);
    for(size_t i = 0; i < context->trustList.trustedCertificatesSize; ++i) {
        data = UA_mbedTLS_CopyDataFormatAware(&context->trustList.trustedCertificates[i]);
        err = mbedtls_x509_crt_parse(&context->trustedCertificates, data.data, data.length);
        UA_ByteString_clear(&data);
        if(err)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    mbedtls_x509_crt_free(&context->issuerCertificates);
    mbedtls_x509_crt_init(&context->issuerCertificates);
    for(size_t i = 0; i < context->trustList.issuerCertificatesSize; ++i) {
        data = UA_mbedTLS_CopyDataFormatAware(&context->trustList.issuerCertificates[i]);
        err = mbedtls_x509_crt_parse(&context->issuerCertificates, data.data, data.length);
        UA_ByteString_clear(&data);
        if(err)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    mbedtls_x509_crl_free(&context->trustedCrls);
    mbedtls_x509_crl_init(&context->trustedCrls);
    for(size_t i = 0; i < context->trustList.trustedCrlsSize; i++) {
        data = UA_mbedTLS_CopyDataFormatAware(&context->trustList.trustedCrls[i]);
        err = mbedtls_x509_crl_parse(&context->trustedCrls, data.data, data.length);
        UA_ByteString_clear(&data);
        if(err)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    mbedtls_x509_crl_free(&context->issuerCrls);
    mbedtls_x509_crl_init(&context->issuerCrls);
    for(size_t i = 0; i < context->trustList.issuerCrlsSize; i++) {
        data = UA_mbedTLS_CopyDataFormatAware(&context->trustList.issuerCrls[i]);
        err = mbedtls_x509_crl_parse(&context->issuerCrls, data.data, data.length);
        UA_ByteString_clear(&data);
        if(err)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
verifyCertificate(UA_CertificateGroup *certGroup, const UA_ByteString *certificate) {
    /* Check parameter */
    if (certGroup == NULL || certGroup->context == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    if(context->reloadRequired) {
        UA_StatusCode retval = reloadCertificates(certGroup);
        if(retval != UA_STATUSCODE_GOOD) {
            return retval;
        }
        context->reloadRequired = false;
    }


    /* Accept the certificate if the store is empty */
    if(context->trustedCertificates.raw.len == 0 &&
       context->issuerCertificates.raw.len == 0 &&
       context->trustedCrls.raw.len == 0 &&
       context->issuerCrls.raw.len == 0) {
        UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_USERLAND,
                       "No certificate store configured. Accepting the certificate.");
        return UA_STATUSCODE_GOOD;
    }

    /* Parse the certificate */
    mbedtls_x509_crt remoteCertificate;

    /* Temporary Object to parse the trustList */
    mbedtls_x509_crt *tempCert = NULL;

    /* Temporary Object to parse the revocationList */
    mbedtls_x509_crl *tempCrl = NULL;

    /* Temporary Object to identify the parent CA when there is no intermediate CA */
    mbedtls_x509_crt *parentCert = NULL;

    /* Temporary Object to identify the parent CA when there is intermediate CA */
    mbedtls_x509_crt *parentCert_2 = NULL;

    /* Flag value to identify if the issuer certificate is found */
    int issuerKnown = 0;

    /* Flag value to identify if the parent certificate found */
    int parentFound = 0;

    mbedtls_x509_crt_init(&remoteCertificate);
    int mbedErr = mbedtls_x509_crt_parse(&remoteCertificate, certificate->data,
                                         certificate->length);
    if(mbedErr) {
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    /* Verify */
    mbedtls_x509_crt_profile crtProfile = {
            MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1) | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA256),
            0xFFFFFF, 0x000000, 128 * 8 // in bits
    }; // TODO: remove magic numbers

    uint32_t flags = 0;
    mbedErr = mbedtls_x509_crt_verify_with_profile(&remoteCertificate,
                                                   &context->trustedCertificates,
                                                   &context->trustedCrls,
                                                   &crtProfile, NULL, &flags, NULL, NULL);

    /* Flag to check if the remote certificate is trusted or not */
    int TRUSTED = 0;

    /* Check if the remoteCertificate is present in the trustList while mbedErr value is not zero */
    if(mbedErr && !(flags & MBEDTLS_X509_BADCERT_EXPIRED) && !(flags & MBEDTLS_X509_BADCERT_FUTURE)) {
        for(tempCert = &context->trustedCertificates; tempCert != NULL; tempCert = tempCert->next) {
            if(remoteCertificate.raw.len == tempCert->raw.len &&
               memcmp(remoteCertificate.raw.p, tempCert->raw.p, remoteCertificate.raw.len) == 0) {
                TRUSTED = REMOTECERTIFICATETRUSTED;
                break;
            }
        }
    }

    /* If the remote certificate is present in the trustList then check if the issuer certificate
     * of remoteCertificate is present in issuerList */
    if(TRUSTED && mbedErr) {
        mbedErr = mbedtls_x509_crt_verify_with_profile(&remoteCertificate,
                                                       &context->issuerCertificates,
                                                       &context->issuerCrls,
                                                       &crtProfile, NULL, &flags, NULL, NULL);

        /* Check if the parent certificate has a CRL file available */
        if(!mbedErr) {
            /* Flag value to identify if that there is an intermediate CA present */
            int dualParent = 0;

            /* Identify the topmost parent certificate for the remoteCertificate */
            for(parentCert = &context->issuerCertificates; parentCert != NULL; parentCert = parentCert->next ) {
                if(memcmp(remoteCertificate.issuer_raw.p, parentCert->subject_raw.p, parentCert->subject_raw.len) == 0) {
                    for(parentCert_2 = &context->trustedCertificates; parentCert_2 != NULL; parentCert_2 = parentCert_2->next) {
                        if(memcmp(parentCert->issuer_raw.p, parentCert_2->subject_raw.p, parentCert_2->subject_raw.len) == 0) {
                            dualParent = DUALPARENT;
                            break;
                        }
                    }
                    parentFound = PARENTFOUND;
                }

                if(parentFound == PARENTFOUND)
                    break;
            }

            /* Check if there is an intermediate certificate between the topmost parent
             * certificate and child certificate
             * If yes the topmost parent certificate is to be checked whether it has a
             * CRL file avaiable */
            if(dualParent == DUALPARENT && parentFound == PARENTFOUND) {
                parentCert = parentCert_2;
            }

            /* If a parent certificate is found traverse the revocationList and identify
             * if there is any CRL file that corresponds to the parentCertificate */
            if(parentFound == PARENTFOUND) {
                tempCrl = &context->trustedCrls;
                while(tempCrl != NULL) {
                    if(tempCrl->version != 0 &&
                       tempCrl->issuer_raw.len == parentCert->subject_raw.len &&
                       memcmp(tempCrl->issuer_raw.p,
                              parentCert->subject_raw.p,
                              tempCrl->issuer_raw.len) == 0) {
                        issuerKnown = ISSUERKNOWN;
                        break;
                    }

                    tempCrl = tempCrl->next;
                }

                /* If the CRL file corresponding to the parent certificate is not present
                 * then return UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN */
                if(!issuerKnown) {
                    return UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN;
                }
            }
        }
    }
    else if(!mbedErr && !TRUSTED) {
        /* This else if section is to identify if the parent certificate which is present in trustList
         * has CRL file corresponding to it */

        /* Identify the parent certificate of the remoteCertificate */
        for(parentCert = &context->trustedCertificates; parentCert != NULL; parentCert = parentCert->next) {
            if(memcmp(remoteCertificate.issuer_raw.p, parentCert->subject_raw.p, parentCert->subject_raw.len) == 0) {
                parentFound = PARENTFOUND;
                break;
            }
        }

        /* If the parent certificate is found traverse the revocationList and identify
         * if there is any CRL file that corresponds to the parentCertificate */
        if(parentFound == PARENTFOUND &&
           memcmp(remoteCertificate.issuer_raw.p, remoteCertificate.subject_raw.p, remoteCertificate.subject_raw.len) != 0) {
            tempCrl = &context->trustedCrls;
            while(tempCrl != NULL) {
                if(tempCrl->version != 0 &&
                   tempCrl->issuer_raw.len == parentCert->subject_raw.len &&
                   memcmp(tempCrl->issuer_raw.p,
                          parentCert->subject_raw.p,
                          tempCrl->issuer_raw.len) == 0) {
                    issuerKnown = ISSUERKNOWN;
                    break;
                }

                tempCrl = tempCrl->next;
            }

            /* If the CRL file corresponding to the parent certificate is not present
             * then return UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN */
            if(!issuerKnown) {
                return UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN;
            }

        }

    }

    // TODO: Extend verification

    /* This condition will check whether the certificate is a User certificate
     * or a CA certificate. If the MBEDTLS_X509_KU_KEY_CERT_SIGN and
     * MBEDTLS_X509_KU_CRL_SIGN of key_usage are set, then the certificate
     * shall be condidered as CA Certificate and cannot be used to establish a
     * connection. Refer the test case CTT/Security/Security Certificate Validation/029.js
     * for more details */
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    if((remoteCertificate.key_usage & MBEDTLS_X509_KU_KEY_CERT_SIGN) &&
       (remoteCertificate.key_usage & MBEDTLS_X509_KU_CRL_SIGN)) {
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }
#else
    if((remoteCertificate.private_key_usage & MBEDTLS_X509_KU_KEY_CERT_SIGN) &&
       (remoteCertificate.private_key_usage & MBEDTLS_X509_KU_CRL_SIGN)) {
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }
#endif

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(mbedErr) {
#if UA_LOGLEVEL <= 400
        char buff[100];
        int len = mbedtls_x509_crt_verify_info(buff, 100, "", flags);
        UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Verifying the certificate failed with error: %.*s", len-1, buff);
#endif
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

static UA_StatusCode
MemoryCertStore_verifyCertificate(UA_CertificateGroup *certGroup,
                                  const UA_ByteString *certificate) {
    /* Check parameter */
    if(certGroup == NULL || certificate == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = verifyCertificate(certGroup, certificate);
    if(retval == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED ||
        retval == UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED ||
        retval == UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN ||
        retval == UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN) {
        if(MemoryCertStore_addToRejectedList(certGroup, certificate) != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                           "Could not append certificate to rejected list");
        }
    }
    return retval;
}

UA_StatusCode
UA_CertificateGroup_Memorystore(UA_CertificateGroup *certGroup,
                                UA_NodeId *certificateGroupId,
                                const UA_TrustListDataType *trustList,
                                const UA_Logger *logger,
                                const UA_KeyValueMap *params) {

    if(certGroup == NULL || certificateGroupId == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Clear if the plugin is already initialized */
    if(certGroup->clear)
        certGroup->clear(certGroup);

    UA_NodeId_copy(certificateGroupId, &certGroup->certificateGroupId);
    certGroup->logging = logger;

    certGroup->getTrustList = MemoryCertStore_getTrustList;
    certGroup->setTrustList = MemoryCertStore_setTrustList;
    certGroup->addToTrustList = MemoryCertStore_addToTrustList;
    certGroup->removeFromTrustList = MemoryCertStore_removeFromTrustList;
    certGroup->getRejectedList = MemoryCertStore_getRejectedList;
    certGroup->verifyCertificate = MemoryCertStore_verifyCertificate;
    certGroup->clear = MemoryCertStore_clear;

    /* Set PKI Store context data */
    MemoryCertStore *context = (MemoryCertStore *)UA_calloc(1, sizeof(MemoryCertStore));
    if(!context) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    certGroup->context = context;
    /* Default values */
    context->maxTrustListSize = 65535;
    context->maxRejectedListSize = 100;

    if(params) {
        const UA_UInt32 *maxTrustListSize = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params, MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMINDEX_MAXTRUSTLISTSIZE].name,
                                 &UA_TYPES[UA_TYPES_UINT32]);

        const UA_UInt32 *maxRejectedListSize = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params, MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMINDEX_MAXREJECTEDLISTSIZE].name,
                                 &UA_TYPES[UA_TYPES_UINT32]);

        if(maxTrustListSize) {
            context->maxTrustListSize = *maxTrustListSize;
        }

        if(maxRejectedListSize) {
            context->maxRejectedListSize = *maxRejectedListSize;
        }
    }

    UA_TrustListDataType_add(trustList, &context->trustList);
    reloadCertificates(certGroup);

    return UA_STATUSCODE_GOOD;

cleanup:
    certGroup->clear(certGroup);
    return retval;
}

/* Find binary substring. Taken and adjusted from
 * http://tungchingkai.blogspot.com/2011/07/binary-strstr.html */

static const unsigned char *
bstrchr(const unsigned char *s, const unsigned char ch, size_t l) {
    /* find first occurrence of c in char s[] for length l*/
    for(; l > 0; ++s, --l) {
        if(*s == ch)
            return s;
    }
    return NULL;
}

static const unsigned char *
UA_Bstrstr(const unsigned char *s1, size_t l1, const unsigned char *s2, size_t l2) {
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

UA_StatusCode
UA_CertificateUtils_verifyApplicationURI(UA_RuleHandling ruleHandling,
                                         const UA_ByteString *certificate,
                                         const UA_String *applicationURI) {
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
    if(UA_Bstrstr(remoteCertificate.v3_ext.p, remoteCertificate.v3_ext.len,
                  applicationURI->data, applicationURI->length) == NULL)
        retval = UA_STATUSCODE_BADCERTIFICATEURIINVALID;

    if(retval != UA_STATUSCODE_GOOD && ruleHandling == UA_RULEHANDLING_DEFAULT) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "The certificate's application URI could not be verified. StatusCode %s",
                       UA_StatusCode_name(retval));
        retval = UA_STATUSCODE_GOOD;
    }
    mbedtls_x509_crt_free(&remoteCertificate);
    return retval;
}

UA_StatusCode
UA_CertificateUtils_getExpirationDate(UA_ByteString *certificate,
                                      UA_DateTime *expiryDateTime) {
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);
    int mbedErr = mbedtls_x509_crt_parse(&publicKey, certificate->data, certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_DateTimeStruct ts;
    ts.year = (UA_Int16)publicKey.valid_to.year;
    ts.month = (UA_UInt16)publicKey.valid_to.mon;
    ts.day = (UA_UInt16)publicKey.valid_to.day;
    ts.hour = (UA_UInt16)publicKey.valid_to.hour;
    ts.min = (UA_UInt16)publicKey.valid_to.min;
    ts.sec = (UA_UInt16)publicKey.valid_to.sec;
    ts.milliSec = 0;
    ts.microSec = 0;
    ts.nanoSec = 0;
    *expiryDateTime = UA_DateTime_fromStruct(ts);
    mbedtls_x509_crt_free(&publicKey);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CertificateUtils_getSubjectName(UA_ByteString *certificate,
                                   UA_String *subjectName) {
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);
    int mbedErr = mbedtls_x509_crt_parse(&publicKey, certificate->data, certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    char buf[1024];
    int res = mbedtls_x509_dn_gets(buf, 1024, &publicKey.subject);
    mbedtls_x509_crt_free(&publicKey);
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_String tmp = {(size_t)res, (UA_Byte*)buf};
    return UA_String_copy(&tmp, subjectName);
}

UA_StatusCode
UA_CertificateUtils_getThumbprint(UA_ByteString *certificate,
                                  UA_String *thumbprint){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(certificate == NULL || thumbprint->length != (UA_SHA1_LENGTH * 2))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString thumbpr = UA_BYTESTRING_NULL;
    UA_ByteString_allocBuffer(&thumbpr, UA_SHA1_LENGTH);

    retval = mbedtls_thumbprint_sha1(certificate, &thumbpr);

    UA_String thumb = UA_STRING_NULL;
    thumb.length = (UA_SHA1_LENGTH * 2) + 1;
    thumb.data = (UA_Byte*)malloc(sizeof(UA_Byte)*thumb.length);

    /* Create a string containing a hex representation */
    char *p = (char*)thumb.data;
    for (size_t i = 0; i < thumbpr.length; i++) {
        p += sprintf(p, "%.2X", thumbpr.data[i]);
    }

    memcpy(thumbprint->data, thumb.data, thumbprint->length);

    UA_ByteString_clear(&thumbpr);
    UA_ByteString_clear(&thumb);

    return retval;
}

UA_StatusCode
UA_CertificateUtils_getKeySize(UA_ByteString *certificate,
                               size_t *keySize){
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);
    int mbedErr = mbedtls_x509_crt_parse(&publicKey, certificate->data, certificate->length);
    if(mbedErr) {
        mbedtls_x509_crt_free(&publicKey);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(publicKey.pk);

#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    *keySize = rsa->len * 8;
#else
    *keySize = mbedtls_rsa_get_len(rsa) * 8;
#endif
    mbedtls_x509_crt_free(&publicKey);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CertificateUtils_decryptPrivateKey(const UA_ByteString privateKey,
                                      const UA_ByteString password,
                                      UA_ByteString *outDerKey) {
    if(!outDerKey)
        return UA_STATUSCODE_BADINTERNALERROR;

    if (privateKey.length == 0) {
        *outDerKey = UA_BYTESTRING_NULL;
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Already in DER format -> return verbatim */
    if(privateKey.length > 1 && privateKey.data[0] == 0x30 && privateKey.data[1] == 0x82)
        return UA_ByteString_copy(&privateKey, outDerKey);

    /* Create a null-terminated string */
    UA_ByteString nullTerminatedKey = UA_mbedTLS_CopyDataFormatAware(&privateKey);
    if(nullTerminatedKey.length != privateKey.length + 1)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Create the private-key context */
    mbedtls_pk_context ctx;
    mbedtls_pk_init(&ctx);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    int err = mbedtls_pk_parse_key(&ctx, nullTerminatedKey.data,
                                   nullTerminatedKey.length,
                                   password.data, password.length);
#else
    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);
    int err = mbedtls_pk_parse_key(&ctx, nullTerminatedKey.data,
                                   nullTerminatedKey.length,
                                   password.data, password.length,
                                   mbedtls_entropy_func, &entropy);
    mbedtls_entropy_free(&entropy);
#endif
    UA_ByteString_clear(&nullTerminatedKey);
    if(err != 0) {
        mbedtls_pk_free(&ctx);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    /* Write the DER-encoded key into a local buffer */
    unsigned char buf[2 << 13];
    size_t pos = (size_t)mbedtls_pk_write_key_der(&ctx, buf, sizeof(buf));

    /* Allocate memory */
    UA_StatusCode res = UA_ByteString_allocBuffer(outDerKey, pos);
    if(res != UA_STATUSCODE_GOOD) {
        mbedtls_pk_free(&ctx);
        return res;
    }

    /* Copy to the output */
    memcpy(outDerKey->data, &buf[sizeof(buf) - pos], pos);
    mbedtls_pk_free(&ctx);
    return UA_STATUSCODE_GOOD;
}

#endif
