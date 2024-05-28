/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/util.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/log_stdout.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#include "ua_openssl_version_abstraction.h"
#include "securitypolicy_openssl_common.h"

/* Configuration parameters */

#define MEMORYCERTSTORE_PARAMETERSSIZE 2
#define MEMORYCERTSTORE_PARAMINDEX_MAXTRUSTLISTSIZE 0
#define MEMORYCERTSTORE_PARAMINDEX_MAXREJECTEDLISTSIZE 1

static const struct {
    UA_QualifiedName name;
    const UA_DataType *type;
    UA_Boolean required;
} MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("maxTrustListSize")}, &UA_TYPES[UA_TYPES_UINT16], false},
    {{0, UA_STRING_STATIC("maxRejectedListSize")}, &UA_TYPES[UA_TYPES_STRING], false}
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

    STACK_OF(X509) *trustedCertificates;
    STACK_OF(X509) *issuerCertificates;
    STACK_OF(X509_CRL) *crls;
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

        sk_X509_pop_free (context->trustedCertificates, X509_free);
        sk_X509_pop_free (context->issuerCertificates, X509_free);
        sk_X509_CRL_pop_free (context->crls, X509_CRL_free);

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

    sk_X509_pop_free(context->trustedCertificates, X509_free);
    context->trustedCertificates = sk_X509_new_null();
    if(context->trustedCertificates == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    for(size_t i = 0; i < context->trustList.trustedCertificatesSize; i++) {
        X509 *cert = UA_OpenSSL_LoadCertificate(&context->trustList.trustedCertificates[i]);
        if(cert == NULL)
            return UA_STATUSCODE_BADINTERNALERROR;
        sk_X509_push(context->trustedCertificates, cert);
    }

    sk_X509_pop_free(context->issuerCertificates, X509_free);
    context->issuerCertificates = sk_X509_new_null();
    if(context->issuerCertificates == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    for(size_t i = 0; i < context->trustList.issuerCertificatesSize; i++) {
        X509 *cert = UA_OpenSSL_LoadCertificate(&context->trustList.issuerCertificates[i]);
        if(cert == NULL)
            return UA_STATUSCODE_BADINTERNALERROR;
        sk_X509_push(context->issuerCertificates, cert);
    }

    sk_X509_CRL_pop_free(context->crls, X509_CRL_free);
    context->crls = sk_X509_CRL_new_null();
    if(context->crls == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    for(size_t i = 0; i < context->trustList.trustedCrlsSize; i++) {
        const unsigned char *pData = context->trustList.trustedCrls[i].data;
        X509_CRL * crl = NULL;

        if(context->trustList.trustedCrls[i].length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
            crl = d2i_X509_CRL (NULL, &pData, (long)context->trustList.trustedCrls[i].length);
        } else {
            BIO* bio = NULL;
            bio = BIO_new_mem_buf((void *)context->trustList.trustedCrls[i].data,
                                  (int)context->trustList.trustedCrls[i].length);
            crl = PEM_read_bio_X509_CRL(bio, NULL, NULL, NULL);
            BIO_free(bio);
        }

        if (crl == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        sk_X509_CRL_push(context->crls, crl);
    }
    for(size_t i = 0; i < context->trustList.issuerCrlsSize; i++) {
        const unsigned char *pData = context->trustList.issuerCrls[i].data;
        X509_CRL * crl = NULL;

        if(context->trustList.issuerCrls[i].length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
            crl = d2i_X509_CRL (NULL, &pData, (long)context->trustList.issuerCrls[i].length);
        } else {
            BIO* bio = NULL;
            bio = BIO_new_mem_buf((void *)context->trustList.issuerCrls[i].data,
                                  (int)context->trustList.issuerCrls[i].length);
            crl = PEM_read_bio_X509_CRL(bio, NULL, NULL, NULL);
            BIO_free(bio);
        }

        if (crl == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        sk_X509_CRL_push(context->crls, crl);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_X509_Store_CTX_Error_To_UAError(int opensslErr) {
    UA_StatusCode ret;

    switch (opensslErr) {
        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_CRL_NOT_YET_VALID:
        case X509_V_ERR_CRL_HAS_EXPIRED:
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
        case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
            ret = UA_STATUSCODE_BADCERTIFICATETIMEINVALID;
        break;
        case X509_V_ERR_CERT_REVOKED:
            ret = UA_STATUSCODE_BADCERTIFICATEREVOKED;
        break;
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            ret = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
        break;
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            ret = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        break;
        case X509_V_ERR_UNABLE_TO_GET_CRL:
            ret = UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN;
        break;
        default:
            ret = UA_STATUSCODE_BADCERTIFICATEINVALID;
        break;
    }

    return ret;
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
    if(sk_X509_CRL_num(context->crls) == 0 &&
       sk_X509_num(context->issuerCertificates) == 0 &&
       sk_X509_num(context->trustedCertificates) == 0) {
        UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_USERLAND,
                       "No certificate store configured. Accepting the certificate.");
        return UA_STATUSCODE_GOOD;
    }

    X509_STORE_CTX *storeCtx = NULL;
    X509_STORE *store = NULL;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    /* Parse the certificate */
    X509 *certificateX509 = UA_OpenSSL_LoadCertificate(certificate);
    if(!certificateX509) {
        ret = UA_STATUSCODE_BADCERTIFICATEINVALID;
        goto cleanup;
    }

    store = X509_STORE_new();
    storeCtx = X509_STORE_CTX_new();
    if(store == NULL || storeCtx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    X509_STORE_set_flags(store, 0);
    int opensslRet = X509_STORE_CTX_init(storeCtx, store, certificateX509,
                                         context->issuerCertificates);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
#if defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT < 0x10100000L
	(void) X509_STORE_CTX_trusted_stack(storeCtx, context->trustedCertificates);
#else
	(void) X509_STORE_CTX_set0_trusted_stack(storeCtx, context->trustedCertificates);
#endif

    /* Set crls to ctx */
    if(sk_X509_CRL_num(context->crls) > 0) {
        X509_STORE_CTX_set0_crls(storeCtx, context->crls);
    }

    /* Set flag to check if the certificate has an invalid signature */
    X509_STORE_CTX_set_flags(storeCtx, X509_V_FLAG_CHECK_SS_SIGNATURE);

    if(X509_check_issued(certificateX509, certificateX509) != X509_V_OK) {
        X509_STORE_CTX_set_flags(storeCtx, X509_V_FLAG_CRL_CHECK);
    }

    /* This condition will check whether the certificate is a User certificate or a CA certificate.
     * If the KU_KEY_CERT_SIGN and KU_CRL_SIGN of key_usage are set, then the certificate shall be
     * condidered as CA Certificate and cannot be used to establish a connection. Refer the test case
     * CTT/Security/Security Certificate Validation/029.js for more details */
     /** \todo Can the ca-parameter of X509_check_purpose can be used? */
    if(X509_check_purpose(certificateX509, X509_PURPOSE_CRL_SIGN, 0) && X509_check_ca(certificateX509)) {
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }

    opensslRet = X509_verify_cert(storeCtx);
    if(opensslRet == 1) {
        ret = UA_STATUSCODE_GOOD;

        /* Check if the not trusted certificate has a CRL file. If there is no CRL file available for the corresponding
         * parent certificate then return status code UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN. Refer the test
         * case CTT/Security/Security Certificate Validation/002.js */
        if(X509_check_issued(certificateX509, certificateX509) != X509_V_OK) {
            /* Free X509_STORE_CTX and reuse it for certification verification */
            if(storeCtx != NULL) {
               X509_STORE_CTX_free(storeCtx);
            }

            /* Initialised X509_STORE_CTX sructure*/
            storeCtx = X509_STORE_CTX_new();

            /* Sets up X509_STORE_CTX structure for a subsequent verification operation */
            X509_STORE_set_flags(store, 0);
            X509_STORE_CTX_init(storeCtx, store, certificateX509, context->issuerCertificates);

            /* Set trust list to ctx */
            (void) X509_STORE_CTX_trusted_stack(storeCtx, context->trustedCertificates);

            /* Set crls to ctx */
            X509_STORE_CTX_set0_crls(storeCtx, context->crls);

            /* Set flags for CRL check */
            X509_STORE_CTX_set_flags(storeCtx, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);

            opensslRet = X509_verify_cert(storeCtx);
            if(opensslRet != 1) {
                opensslRet = X509_STORE_CTX_get_error(storeCtx);
                if(opensslRet == X509_V_ERR_UNABLE_TO_GET_CRL) {
                    ret = UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN;
                }
            }
        }
    }
    else {
        opensslRet = X509_STORE_CTX_get_error(storeCtx);

        /* Check the issued certificate of a CA that is not trusted but available */
        if(opensslRet == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN){
            int trusted_cert_len = sk_X509_num(context->trustedCertificates);
            int cmpVal;
            X509 *trusted_cert;
            const ASN1_OCTET_STRING *trusted_cert_keyid;
            const ASN1_OCTET_STRING *remote_cert_keyid;

            for(int i = 0; i < trusted_cert_len; i++) {
                trusted_cert = sk_X509_value(context->trustedCertificates, i);

                /* Fetch the Subject key identifier of the certificate in trust list */
                trusted_cert_keyid = X509_get0_subject_key_id(trusted_cert);

                /* Fetch the Subject key identifier of the remote certificate */
                remote_cert_keyid = X509_get0_subject_key_id(certificateX509);

                /* Check remote certificate is present in the trust list */
                cmpVal = ASN1_OCTET_STRING_cmp(trusted_cert_keyid, remote_cert_keyid);
                if(cmpVal == 0) {
                    ret = UA_STATUSCODE_GOOD;
                    goto cleanup;
                }
            }
        }

        /* Return expected OPCUA error code */
        ret = UA_X509_Store_CTX_Error_To_UAError(opensslRet);
    }

cleanup:
    if(store)
        X509_STORE_free(store);
    if(storeCtx)
        X509_STORE_CTX_free(storeCtx);
    if(certificateX509)
        X509_free(certificateX509);
    return ret;
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

#endif
