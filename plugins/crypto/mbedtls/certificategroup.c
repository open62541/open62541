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

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <mbedtls/x509.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/entropy.h>
#include <mbedtls/version.h>
#include <mbedtls/sha256.h>
#if defined(MBEDTLS_USE_PSA_CRYPTO)
#include <mbedtls/psa_util.h>
#endif

#include "securitypolicy_common.h"

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

typedef struct {
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
} MemoryCertStore;

static UA_Boolean mbedtlsCheckCA(mbedtls_x509_crt *cert);

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
    /* Remove the section of the trust list that needs to be reset, while keeping the remaining parts intact */
    return UA_TrustListDataType_set(trustList, &context->trustList);
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
mbedtlsCheckCrlMatch(mbedtls_x509_crt *cert, mbedtls_x509_crl *crl) {
    char certSubject[MBEDTLS_X509_MAX_DN_NAME_SIZE];
    char crlIssuer[MBEDTLS_X509_MAX_DN_NAME_SIZE];

    mbedtls_x509_dn_gets(certSubject, sizeof(certSubject), &cert->subject);
    mbedtls_x509_dn_gets(crlIssuer, sizeof(crlIssuer), &crl->issuer);

    if(strncmp(certSubject, crlIssuer, MBEDTLS_X509_MAX_DN_NAME_SIZE) == 0)
        return UA_STATUSCODE_GOOD;

    return UA_STATUSCODE_BADNOMATCH;
}

static UA_StatusCode
mbedtlsFindCrls(UA_CertificateGroup *certGroup, const UA_ByteString *certificate,
             const UA_ByteString *crlList, const size_t crlListSize,
             UA_ByteString **crls, size_t *crlsSize) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &cert);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SECURITYPOLICY,
            "An error occurred while parsing the certificate.");
        return retval;
    }

    /* Check if the certificate is a CA certificate.
     * Only a CA certificate can have a CRL. */
    if(!mbedtlsCheckCA(&cert)) {
        UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SECURITYPOLICY,
               "The certificate is not a CA certificate and therefore does not have a CRL.");
        mbedtls_x509_crt_free(&cert);
        return UA_STATUSCODE_GOOD;
    }

    UA_Boolean foundMatch = false;
    for(size_t i = 0; i < crlListSize; i++) {
        mbedtls_x509_crl crl;
        mbedtls_x509_crl_init(&crl);
        retval = UA_mbedTLS_LoadCrl(&crlList[i], &crl);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                "An error occurred while parsing the crl.");
            mbedtls_x509_crt_free(&cert);
            return retval;
        }

        retval = mbedtlsCheckCrlMatch(&cert, &crl);
        mbedtls_x509_crl_free(&crl);
        if(retval != UA_STATUSCODE_GOOD) {
            continue;
        }

        /* Continue the search, as a certificate may be associated with multiple CRLs. */
        foundMatch = true;
        retval = UA_Array_appendCopy((void **)crls, crlsSize, &crlList[i],
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
        if(retval != UA_STATUSCODE_GOOD) {
            mbedtls_x509_crt_free(&cert);
            return retval;
        }
    }
    mbedtls_x509_crt_free(&cert);

    if(!foundMatch)
        return UA_STATUSCODE_BADNOMATCH;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
MemoryCertStore_getCertificateCrls(UA_CertificateGroup *certGroup, const UA_ByteString *certificate,
                                   const UA_Boolean isTrusted, UA_ByteString **crls,
                                   size_t *crlsSize) {
    /* Check parameter */
    if(certGroup == NULL || certificate == NULL || crls == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;

    if(isTrusted) {
        return mbedtlsFindCrls(certGroup, certificate,
                               context->trustList.trustedCrls,
                               context->trustList.trustedCrlsSize, crls,
                               crlsSize);
    }
    return mbedtlsFindCrls(certGroup, certificate,
                           context->trustList.issuerCrls,
                           context->trustList.issuerCrlsSize, crls,
                           crlsSize);
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

#define UA_MBEDTLS_MAX_CHAIN_LENGTH 10
#define UA_MBEDTLS_MAX_DN_LENGTH 256

/* We need to access some private fields below */
#ifndef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(x) x
#endif

/* Is the certificate a CA? */
static UA_Boolean
mbedtlsCheckCA(mbedtls_x509_crt *cert) {
    /* The Basic Constraints extension must be set and the cert acts as CA */
    if(!(cert->MBEDTLS_PRIVATE(ext_types) & MBEDTLS_X509_EXT_BASIC_CONSTRAINTS) ||
       !cert->MBEDTLS_PRIVATE(ca_istrue))
        return false;

    /* The Key Usage extension must be set to cert signing and CRL issuing */
    if(!(cert->MBEDTLS_PRIVATE(ext_types) & MBEDTLS_X509_EXT_KEY_USAGE) ||
       mbedtls_x509_crt_check_key_usage(cert, MBEDTLS_X509_KU_KEY_CERT_SIGN) != 0 ||
       mbedtls_x509_crt_check_key_usage(cert, MBEDTLS_X509_KU_CRL_SIGN) != 0)
        return false;

    return true;
}

static UA_Boolean
mbedtlsSameName(UA_String name, const mbedtls_x509_name *name2) {
    char buf[UA_MBEDTLS_MAX_DN_LENGTH];
    int len = mbedtls_x509_dn_gets(buf, UA_MBEDTLS_MAX_DN_LENGTH, name2);
    if(len < 0)
        return false;
    UA_String nameString = {(size_t)len, (UA_Byte*)buf};
    return UA_String_equal(&name, &nameString);
}

static UA_Boolean
mbedtlsSameBuf(mbedtls_x509_buf *a, mbedtls_x509_buf *b) {
    if(a->len != b->len)
        return false;
    return (memcmp(a->p, b->p, a->len) == 0);
}

/* Return the first matching issuer candidate AFTER prev.
 * This can return the cert itself if self-signed. */
static mbedtls_x509_crt *
mbedtlsFindNextIssuer(MemoryCertStore *ctx, mbedtls_x509_crt *stack,
                      mbedtls_x509_crt *cert, mbedtls_x509_crt *prev) {
    char inbuf[UA_MBEDTLS_MAX_DN_LENGTH];
    int nameLen = mbedtls_x509_dn_gets(inbuf, UA_MBEDTLS_MAX_DN_LENGTH, &cert->issuer);
    if(nameLen < 0)
        return NULL;
    UA_String issuerName = {(size_t)nameLen, (UA_Byte*)inbuf};
    do {
        for(mbedtls_x509_crt *i = stack; i; i = i->next) {
            if(prev) {
                if(prev == i)
                    prev = NULL; /* This was the last issuer we tried to verify */
                continue;
            }
            /* Compare issuer name and subject name.
             * Skip when the key does not match the signature. */
            if(mbedtlsSameName(issuerName, &i->subject) &&
               mbedtls_pk_can_do(&i->pk, cert->MBEDTLS_PRIVATE(sig_pk)))
                return i;
        }

        /* Switch from the stack that came with the cert to the issuer list and
         * then to the trust list. */
        if(stack == &ctx->trustedCertificates)
            stack = NULL;
        else if(stack == &ctx->issuerCertificates)
            stack = &ctx->trustedCertificates;
        else
            stack = &ctx->issuerCertificates;
    } while(stack);
    return NULL;
}

static UA_StatusCode
mbedtlsCheckRevoked(UA_CertificateGroup *cg, MemoryCertStore *ctx, mbedtls_x509_crt *cert) {
    /* Parse the Issuer Name */
    char inbuf[UA_MBEDTLS_MAX_DN_LENGTH];
    int nameLen = mbedtls_x509_dn_gets(inbuf, UA_MBEDTLS_MAX_DN_LENGTH, &cert->issuer);
    if(nameLen < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_String issuerName = {(size_t)nameLen, (UA_Byte*)inbuf};

    if(ctx->trustedCrls.raw.len == 0 && ctx->issuerCrls.raw.len == 0) {
        UA_LOG_WARNING(cg->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Zero revocation lists have been loaded. "
                       "This seems intentional - omitting the check.");
        return UA_STATUSCODE_GOOD;
    }

    /* Loop over the crl and match the Issuer Name */
    UA_StatusCode res = UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN;
    for(mbedtls_x509_crl *crl = &ctx->trustedCrls; crl; crl = crl->next) {
        /* Is the CRL for certificates from the cert issuer?
         * Is the serial number of the certificate contained in the CRL? */
        if(mbedtlsSameName(issuerName, &crl->issuer)) {
            if(mbedtls_x509_crt_is_revoked(cert, crl) != 0)
                return UA_STATUSCODE_BADCERTIFICATEREVOKED;
            res = UA_STATUSCODE_GOOD; /* There was at least one crl that did not revoke (so far) */
        }
    }

    /* Loop over the issuer crls separately */
    for(mbedtls_x509_crl *crl = &ctx->issuerCrls; crl; crl = crl->next) {
        if(mbedtlsSameName(issuerName, &crl->issuer)) {
            if(mbedtls_x509_crt_is_revoked(cert, crl) != 0)
                return UA_STATUSCODE_BADCERTIFICATEREVOKED;
            res = UA_STATUSCODE_GOOD;
        }
    }

    return res;
}

/* Verify that the public key of the issuer was used to sign the certificate */
static UA_Boolean
mbedtlsCheckSignature(const mbedtls_x509_crt *cert, mbedtls_x509_crt *issuer) {
    size_t hash_len;
    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    mbedtls_md_type_t md = cert->MBEDTLS_PRIVATE(sig_md);
#if !defined(MBEDTLS_USE_PSA_CRYPTO)
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md);
    hash_len = mbedtls_md_get_size(md_info);
    if(mbedtls_md(md_info, cert->tbs.p, cert->tbs.len, hash) != 0)
        return false;
#else
    if(psa_hash_compute(mbedtls_md_psa_alg_from_type(md), cert->tbs.p,
                        cert->tbs.len, hash, sizeof(hash), &hash_len) != PSA_SUCCESS)
        return false;
#endif
    const mbedtls_x509_buf *sig = &cert->MBEDTLS_PRIVATE(sig);
    void *sig_opts = cert->MBEDTLS_PRIVATE(sig_opts);
    mbedtls_pk_type_t pktype = cert->MBEDTLS_PRIVATE(sig_pk);
    return (mbedtls_pk_verify_ext(pktype, sig_opts, &issuer->pk, md,
                                  hash, hash_len, sig->p, sig->len) == 0);
}

static UA_StatusCode
mbedtlsVerifyChain(UA_CertificateGroup *cg, MemoryCertStore *ctx, mbedtls_x509_crt *stack,
                   mbedtls_x509_crt **old_issuers, mbedtls_x509_crt *cert, int depth) {
    /* Maxiumum chain length */
    if(depth == UA_MBEDTLS_MAX_CHAIN_LENGTH)
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;

    /* Verification Step: Validity Period */
    if(mbedtls_x509_time_is_future(&cert->valid_from) ||
       mbedtls_x509_time_is_past(&cert->valid_to))
        return (depth == 0) ? UA_STATUSCODE_BADCERTIFICATETIMEINVALID :
            UA_STATUSCODE_BADCERTIFICATEISSUERTIMEINVALID;

    /* Return the most specific error code. BADCERTIFICATECHAININCOMPLETE is
     * returned only if all possible chains are incomplete. */
    mbedtls_x509_crt *issuer = NULL;
    UA_StatusCode ret = UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    while(ret != UA_STATUSCODE_GOOD) {
        /* Find the issuer. This can return the same certificate if it is
         * self-signed (subject == issuer). We come back here to try a different
         * "path" if a subsequent verification fails. */
        issuer = mbedtlsFindNextIssuer(ctx, stack, cert, issuer);
        if(!issuer)
            break;

        /* Verification Step: Certificate Usage
         * Can the issuer act as CA? Omit for self-signed leaf certificates. */
        if((depth > 0 || issuer != cert) && !mbedtlsCheckCA(issuer)) {
            ret = UA_STATUSCODE_BADCERTIFICATEISSUERUSENOTALLOWED;
            continue;
        }

        /* Verification Step: Signature */
        if(!mbedtlsCheckSignature(cert, issuer)) {
            ret = UA_STATUSCODE_BADCERTIFICATEINVALID;  /* Wrong issuer, try again */
            continue;
        }

        /* The certificate is self-signed. We have arrived at the top of the
         * chain. We check whether the certificate is trusted below. This is the
         * only place where we return UA_STATUSCODE_BADCERTIFICATEUNTRUSTED.
         * This signals that the chain is complete (but can be still
         * untrusted).
         *
         * Break here as we have reached the end of the chain. Omit the
         * Revocation Check for self-signed certificates. */
        if(issuer == cert || mbedtlsSameBuf(&cert->tbs, &issuer->tbs)) {
            ret = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
            break;
        }

        /* Verification Step: Revocation Check */
        ret = mbedtlsCheckRevoked(cg, ctx, cert);
        if(depth > 0) {
            if(ret == UA_STATUSCODE_BADCERTIFICATEREVOKED)
                ret = UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED;
            if(ret == UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN)
                ret = UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN;
        }
        if(ret != UA_STATUSCODE_GOOD)
            continue;

        /* Detect (endless) loops of issuers */
        for(int i = 0; i < depth; i++) {
            if(old_issuers[i] == issuer)
                return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
        }
        old_issuers[depth] = issuer;

        /* We have found the issuer certificate used for the signature. Recurse
         * to the next certificate in the chain (verify the current issuer). */
        ret = mbedtlsVerifyChain(cg, ctx, stack, old_issuers, issuer, depth + 1);
    }

    /* The chain is complete, but we haven't yet identified a trusted
     * certificate "on the way down". Can we trust this certificate? */
    if(ret == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED) {
        for(mbedtls_x509_crt *t = &ctx->trustedCertificates; t; t = t->next) {
            if(mbedtlsSameBuf(&cert->tbs, &t->tbs))
                return UA_STATUSCODE_GOOD;
        }
    }

    return ret;
}

/* This follows Part 6, 6.1.3 Determining if a Certificate is trusted.
 * It defines a sequence of steps for certificate verification. */
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

    /* Verification Step: Certificate Structure
     * This parses the entire certificate chain contained in the bytestring. */
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data,
                                         certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    /* Verification Step: Certificate Usage
     * Check whether the certificate is a User certificate or a CA certificate.
     * Refer the test case CTT/Security/Security Certificate Validation/029.js
     * for more details. */
    if(mbedtlsCheckCA(&cert)) {
        mbedtls_x509_crt_free(&cert);
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }

    /* These steps are performed outside of this method.
     * Because we need the server or client context.
     * - Security Policy
     * - Host Name
     * - URI */

    /* Verification Step: Build Certificate Chain
     * We perform the checks for each certificate inside. */
    mbedtls_x509_crt *old_issuers[UA_MBEDTLS_MAX_CHAIN_LENGTH];
    UA_StatusCode ret = mbedtlsVerifyChain(certGroup, context, &cert, old_issuers, &cert, 0);
    mbedtls_x509_crt_free(&cert);
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
    if(retval != UA_STATUSCODE_GOOD) {
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
    certGroup->getCertificateCrls = MemoryCertStore_getCertificateCrls;
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

#if !defined(mbedtls_x509_subject_alternative_name)

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

#endif

UA_StatusCode
UA_CertificateUtils_verifyApplicationURI(UA_RuleHandling ruleHandling,
                                         const UA_ByteString *certificate,
                                         const UA_String *applicationURI,
                                         UA_Logger *logger) {
    /* Parse the certificate */
    mbedtls_x509_crt remoteCertificate;
    mbedtls_x509_crt_init(&remoteCertificate);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

#if defined(mbedtls_x509_subject_alternative_name)
    /* Get the Subject Alternative Name and compate */
    mbedtls_x509_subject_alternative_name san;
    mbedtls_x509_sequence *cur = &remoteCertificate.subject_alt_names;
    retval = UA_STATUSCODE_BADCERTIFICATEURIINVALID;
    for(; cur; cur = cur->next) {
        int res = mbedtls_x509_parse_subject_alt_name(&cur->buf, &san);
        if(res != 0)
            continue;
        if(san.type != MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER) {
            mbedtls_x509_free_subject_alt_name(&san);
            continue;
        }

        UA_String uri = {san.san.unstructured_name.len, san.san.unstructured_name.p};
        UA_Boolean found = UA_String_equal(&uri, applicationURI);
        if(found) {
            retval = UA_STATUSCODE_GOOD;
        } else if(ruleHandling != UA_RULEHANDLING_ACCEPT) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                           "The certificate's Subject Alternative Name URI (%S) "
                           "does not match the ApplicationURI (%S)",
                           uri, *applicationURI);
        }
        mbedtls_x509_free_subject_alt_name(&san);
        break;
    }

    if(!cur && ruleHandling != UA_RULEHANDLING_ACCEPT) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "The certificate has no Subject Alternative Name URI defined");
    }
#else
    /* Poor man's ApplicationUri verification. mbedTLS does not parse all fields
     * of the Alternative Subject Name. Instead test whether the URI-string is
     * present in the v3_ext field in general. */
    if(UA_Bstrstr(remoteCertificate.v3_ext.p, remoteCertificate.v3_ext.len,
                  applicationURI->data, applicationURI->length) == NULL)
        retval = UA_STATUSCODE_BADCERTIFICATEURIINVALID;

    if(retval != UA_STATUSCODE_GOOD && ruleHandling != UA_RULEHANDLING_ACCEPT) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "The certificate's application URI could not be verified. StatusCode %s",
                       UA_StatusCode_name(retval));
    }
#endif

    if(ruleHandling != UA_RULEHANDLING_ABORT)
        retval = UA_STATUSCODE_GOOD;

    mbedtls_x509_crt_free(&remoteCertificate);
    return retval;
}

UA_StatusCode
UA_CertificateUtils_getExpirationDate(UA_ByteString *certificate,
                                      UA_DateTime *expiryDateTime) {
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &publicKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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

    mbedtls_x509_crl crl;
    mbedtls_x509_crl_init(&crl);

    char buf[1024];
    int res = 0;
    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &publicKey);
    if(retval == UA_STATUSCODE_GOOD) {
        res = mbedtls_x509_dn_gets(buf, 1024, &publicKey.subject);
        mbedtls_x509_crt_free(&publicKey);
    } else {
        retval = UA_mbedTLS_LoadCrl(certificate, &crl);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        res = mbedtls_x509_dn_gets(buf, 1024, &crl.issuer);
        mbedtls_x509_crl_free(&crl);
    }

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

    // prepare temporary to hold the binary thumbprint
    UA_Byte buf[UA_SHA1_LENGTH];
    UA_ByteString thumbpr = {
        /*.length =*/ sizeof(buf),
        /*.data =*/ buf
    };

    retval = mbedtls_thumbprint_sha1(certificate, &thumbpr);

    // convert to hexadecimal string representation
    size_t t = 0u;
    for (size_t i = 0u; i < thumbpr.length; i++) {
        UA_Byte shift = 4u;
        // byte consists of two nibbles: AAAABBBB
        const UA_Byte curByte = thumbpr.data[i];
        // convert AAAA first then BBBB
        for(size_t n = 0u; n < 2u; n++) {
            UA_Byte curNibble = (curByte >> shift) & 0x0Fu;
            if(curNibble >= 10u)
                thumbprint->data[t++] = (65u + (curNibble - 10u));  // 65 == 'A'
            else
                thumbprint->data[t++] = (48u + curNibble);          // 48 == '0'
            shift -= 4u;
        }
    }

    return retval;
}

UA_StatusCode
UA_CertificateUtils_getKeySize(UA_ByteString *certificate,
                               size_t *keySize){
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &publicKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
UA_CertificateUtils_comparePublicKeys(const UA_ByteString *certificate1,
                                      const UA_ByteString *certificate2) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    mbedtls_x509_crt cert1;
    mbedtls_x509_crt cert2;
    mbedtls_x509_csr csr1;
    mbedtls_x509_csr csr2;
    mbedtls_mpi N1, E1;
    mbedtls_mpi N2, E2;

    UA_ByteString data1 = UA_mbedTLS_CopyDataFormatAware(certificate1);
    UA_ByteString data2 = UA_mbedTLS_CopyDataFormatAware(certificate2);

    mbedtls_x509_crt_init(&cert1);
    mbedtls_x509_crt_init(&cert2);
    mbedtls_x509_csr_init(&csr1);
    mbedtls_x509_csr_init(&csr2);
    mbedtls_mpi_init(&N1);
    mbedtls_mpi_init(&E1);
    mbedtls_mpi_init(&N2);
    mbedtls_mpi_init(&E2);

    int mbedErr = mbedtls_x509_crt_parse(&cert1, data1.data, data1.length);
    if(mbedErr) {
        /* Try to load as a csr */
        mbedErr = mbedtls_x509_csr_parse(&csr1, data1.data, data1.length);
        if(mbedErr) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
    }

    mbedErr = mbedtls_x509_crt_parse(&cert2, data2.data, data2.length);
    if(mbedErr) {
        /* Try to load as a csr */
        mbedErr = mbedtls_x509_csr_parse(&csr2, data2.data, data2.length);
        if(mbedErr) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
    }

#if MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_pk_context pk1 = cert1.pk.pk_info ? cert1.pk : csr1.pk;
    mbedtls_pk_context pk2 = cert2.pk.pk_info ? cert2.pk : csr2.pk;
#else
    mbedtls_pk_context pk1 = cert1.pk_raw.p ? cert1.pk : csr1.pk;
    mbedtls_pk_context pk2 = cert2.pk_raw.p ? cert2.pk : csr2.pk;
#endif

    if(!mbedtls_pk_rsa(pk1) || !mbedtls_pk_rsa(pk2)) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(!mbedtls_pk_can_do(&pk1, MBEDTLS_PK_RSA) &&
       !mbedtls_pk_can_do(&pk2, MBEDTLS_PK_RSA)) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

#if MBEDTLS_VERSION_NUMBER < 0x02070000
    N1 = mbedtls_pk_rsa(pk1)->N;
    E1 = mbedtls_pk_rsa(pk1)->E;
    N2 = mbedtls_pk_rsa(pk2)->N;
    E2 = mbedtls_pk_rsa(pk2)->E;
#else
    if(mbedtls_rsa_export(mbedtls_pk_rsa(pk1), &N1, NULL, NULL, NULL, &E1) != 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    if(mbedtls_rsa_export(mbedtls_pk_rsa(pk2), &N2, NULL, NULL, NULL, &E2) != 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
#endif

    if(mbedtls_mpi_cmp_mpi(&N1, &N2) || mbedtls_mpi_cmp_mpi(&E1, &E2))
        retval = UA_STATUSCODE_BADNOMATCH;

cleanup:
    mbedtls_mpi_free(&N1);
    mbedtls_mpi_free(&E1);
    mbedtls_mpi_free(&N2);
    mbedtls_mpi_free(&E2);
    mbedtls_x509_crt_free(&cert1);
    mbedtls_x509_crt_free(&cert2);
    mbedtls_x509_csr_free(&csr1);
    mbedtls_x509_csr_free(&csr2);
    UA_ByteString_clear(&data1);
    UA_ByteString_clear(&data2);

    return retval;
}

UA_StatusCode
UA_CertificateUtils_checkKeyPair(const UA_ByteString *certificate,
                                 const UA_ByteString *privateKey) {
    mbedtls_x509_crt cert;
    mbedtls_pk_context pk;

    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&pk);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &cert);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_mbedTLS_LoadPrivateKey(privateKey, &pk, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Verify the private key matches the public key in the certificate */
    if(!mbedtls_pk_can_do(&pk, mbedtls_pk_get_type(&cert.pk))) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto cleanup;
    }

    /* Check if the public key from the certificate matches the private key */
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    if(mbedtls_pk_check_pair(&cert.pk, &pk) != 0) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
#else
    if(mbedtls_pk_check_pair(&cert.pk, &pk, mbedtls_entropy_func, NULL) != 0) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
#endif

cleanup:
    mbedtls_pk_free(&pk);
    mbedtls_x509_crt_free(&cert);

    return retval;
}

UA_StatusCode
UA_CertificateUtils_checkCA(const UA_ByteString *certificate) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &cert);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = mbedtlsCheckCA(&cert) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOMATCH;

cleanup:
    mbedtls_x509_crt_free(&cert);

    return retval;
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
    unsigned char buf[1 << 14];
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
