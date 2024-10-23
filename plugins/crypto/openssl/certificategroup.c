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

#include "libc_time.h"
#include "securitypolicy_common.h"

#define SHA1_DIGEST_LENGTH 20

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
openSSLCheckCrlMatch(X509 *cert, X509_CRL *crl) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Check if the certificate is a CA certificate.
     * Only a CA certificate can have a CRL. */
    BASIC_CONSTRAINTS *bs = (BASIC_CONSTRAINTS*)X509_get_ext_d2i(cert, NID_basic_constraints, NULL, NULL);
    if(!bs || bs->ca == 0) {
        /* The certificate is not a CA or the extension is missing */
        BASIC_CONSTRAINTS_free(bs);
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }
    BASIC_CONSTRAINTS_free(bs);

    X509_NAME *certSubject = X509_get_subject_name(cert);
    if(!certSubject)
        return UA_STATUSCODE_BADINTERNALERROR;

    X509_NAME *crlIssuer = X509_CRL_get_issuer(crl);
    if(!crlIssuer) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(X509_NAME_cmp(certSubject, crlIssuer) == 0) {
        retval = UA_STATUSCODE_GOOD;
    } else {
        retval = UA_STATUSCODE_BADNOMATCH;
    }

    return retval;
}

static UA_StatusCode
openSSLFindCrls(UA_CertificateGroup *certGroup, const UA_ByteString *certificate,
                const UA_ByteString *crlList, const size_t crlListSize,
                UA_ByteString **crls, size_t *crlsSize) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    X509 *cert = UA_OpenSSL_LoadCertificate(certificate);
    if(!cert)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Boolean foundMatch = false;
    for(size_t i = 0; i < crlListSize; i++) {
        X509_CRL *crl = UA_OpenSSL_LoadCrl(&crlList[i]);
        if(!crl) {
            X509_free(cert);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        retval = openSSLCheckCrlMatch(cert, crl);
        X509_CRL_free(crl);
        if(retval == UA_STATUSCODE_BADNOMATCH) {
            continue;
        }
        /* If it is not a CA certificate, there is no crl list. */
        if(retval == UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED) {
            UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SERVER,
                "The certificate is not a CA certificate and therefore does not have a CRL.");
            X509_free(cert);
            return retval;
        }
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_SERVER,
                "An error occurred while determining the appropriate CRL.");
            X509_free(cert);
            return retval;
        }
        /* Continue the search, as a certificate may be associated with multiple CRLs. */
        foundMatch = true;
        retval = UA_Array_appendCopy((void **)crls, crlsSize, &crlList[i],
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
        if(retval != UA_STATUSCODE_GOOD) {
            X509_free(cert);
            return retval;
        }
    }
    X509_free(cert);
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
        return openSSLFindCrls(certGroup, certificate,
                               context->trustList.trustedCrls,
                               context->trustList.trustedCrlsSize, crls,
                               crlsSize);
    }
    return openSSLFindCrls(certGroup, certificate,
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

static const unsigned char openssl_PEM_PRE[28] = "-----BEGIN CERTIFICATE-----";

/* Extract the leaf certificate from a bytestring that may contain an entire chain */
static X509 *
openSSLLoadLeafCertificate(UA_ByteString cert, size_t *offset) {
    if(cert.length <= *offset)
        return NULL;
    cert.length -= *offset;
    cert.data += *offset;

    /* Detect DER encoding. Extract the encoding length and cut. */
    if(cert.length >= 4 && cert.data[0] == 0x30 && cert.data[1] == 0x82) {
        /* The certificate length is encoded after the magic bytes */
        size_t certLen = 4; /* Magic numbers + length bytes */
        certLen += (size_t)(((uint16_t)cert.data[2]) << 8);
        certLen += cert.data[3];
        if(certLen > cert.length)
            return NULL;
        cert.length = certLen;
        *offset += certLen;
        const UA_Byte *dataPtr = cert.data;
        return d2i_X509(NULL, &dataPtr, (long)cert.length);
    }

    /* Assume PEM encoding. Detect multiple certificates and cut. */
    if(cert.length > 27 * 4) {
        const unsigned char *match =
            UA_Bstrstr(openssl_PEM_PRE, 27, &cert.data[27*2], cert.length - (27*2));
        if(match)
            cert.length = (uintptr_t)(match - cert.data);
    }
    *offset += cert.length;

    BIO *bio = BIO_new_mem_buf((void *) cert.data, (int)cert.length);
    X509 *result = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return result;
}

/* The bytestring might contain an entire certificate chain. The first
 * stack-element is the leaf certificate itself. The remaining ones are
 * potential issuer certificates. */
static STACK_OF(X509) *
openSSLLoadCertificateStack(const UA_ByteString cert) {
    size_t offset = 0;
    X509 *x509 = NULL;
    STACK_OF(X509) *result = sk_X509_new_null();
    if(!result)
        return NULL;
    while((x509 = openSSLLoadLeafCertificate(cert, &offset))) {
        sk_X509_push(result, x509);
    }
    return result;
}

/* Return the first matching issuer candidate AFTER prev */
static X509 *
openSSLFindNextIssuer(MemoryCertStore *ctx, STACK_OF(X509) *stack, X509 *x509, X509 *prev) {
    /* First check issuers from the stack - provided in the same bytestring as
     * the certificate. This can also return x509 itself. */
    do {
        int size = sk_X509_num(stack);
        for(int i = 0; i < size; i++) {
            X509 *candidate = sk_X509_value(stack, i);
            if(prev) {
                if(prev == candidate)
                    prev = NULL; /* This was the last issuer we tried to verify */
                continue;
            }
            /* This checks subject/issuer name and the key usage of the issuer.
             * It does not verify the validity period and if the issuer key was
             * used for the signature. We check that afterwards. */
            if(X509_check_issued(candidate, x509) == 0)
                return candidate;
        }
        /* Switch to search in the ctx->skIssue list */
        stack = (stack != ctx->issuerCertificates) ? ctx->issuerCertificates : NULL;
    } while(stack);
    return NULL;
}

static UA_Boolean
openSSLCheckRevoked(MemoryCertStore *ctx, X509 *cert) {
    const ASN1_INTEGER *sn = X509_get0_serialNumber(cert);
    const X509_NAME *in = X509_get_issuer_name(cert);
    int size = sk_X509_CRL_num(ctx->crls);
    for(int i = 0; i < size; i++) {
        /* The crl contains a list of serial numbers from the same issuer */
        X509_CRL *crl = sk_X509_CRL_value(ctx->crls, i);
        if(X509_NAME_cmp(in, X509_CRL_get_issuer(crl)) != 0)
            continue;
        STACK_OF(X509_REVOKED) *rs = X509_CRL_get_REVOKED(crl);
        int rsize = sk_X509_REVOKED_num(rs);
        for(int j = 0; j < rsize; j++) {
            X509_REVOKED *r = sk_X509_REVOKED_value(rs, j);
            if(ASN1_INTEGER_cmp(sn, X509_REVOKED_get0_serialNumber(r)) == 0)
                return true;
        }
    }
    return false;
}

#define UA_OPENSSL_MAX_CHAIN_LENGTH 10

static UA_StatusCode
openSSL_verifyChain(MemoryCertStore *ctx, STACK_OF(X509) *stack, X509 **old_issuers,
                    X509 *cert, int depth) {
    /* Maxiumum chain length */
    if(depth == UA_OPENSSL_MAX_CHAIN_LENGTH)
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;

    /* Verification Step: Validity Period */
    ASN1_TIME *notBefore = X509_get_notBefore(cert);
    ASN1_TIME *notAfter = X509_get_notAfter(cert);
    if(X509_cmp_current_time(notBefore) != -1 || X509_cmp_current_time(notAfter) != 1)
        return (depth == 0) ? UA_STATUSCODE_BADCERTIFICATETIMEINVALID :
            UA_STATUSCODE_BADCERTIFICATEISSUERTIMEINVALID;

    /* Verification Step: Revocation Check */
    if(openSSLCheckRevoked(ctx, cert))
        return (depth == 0) ? UA_STATUSCODE_BADCERTIFICATEREVOKED :
            UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED;

    /* Return the most specific error code. BADCERTIFICATECHAININCOMPLETE is
     * returned only if all possible chains are incomplete. */
    X509 *issuer = NULL;
    UA_StatusCode ret = UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    while(ret != UA_STATUSCODE_GOOD) {
        /* Find the issuer. We jump back here to find a different path if a
         * subsequent check fails. */
        issuer = openSSLFindNextIssuer(ctx, stack, cert, issuer);
        if(!issuer)
            break;

        /* Verification Step: Certificate Usage
         * Can the issuer act as CA? Omit for self-signed leaf certificates. */
        if((depth > 0 || issuer != cert) && !X509_check_ca(issuer)) {
            ret = UA_STATUSCODE_BADCERTIFICATEISSUERUSENOTALLOWED;
            continue;
        }

        /* Verification Step: Signature */
        int opensslRet = X509_verify(cert, X509_get0_pubkey(issuer));
        if(opensslRet == -1) {
            return UA_STATUSCODE_BADCERTIFICATEINVALID; /* Ill-formed signature */
        } else if(opensslRet == 0) {
            ret = UA_STATUSCODE_BADCERTIFICATEINVALID;  /* Wrong issuer, try again */
            continue;
        }

        /* The certificate is self-signed. We have arrived at the top of the
         * chain. We check whether the certificate is trusted below. This is the
         * only place where we return UA_STATUSCODE_BADCERTIFICATEUNTRUSTED.
         * This signals that the chain is complete (but can be still
         * untrusted). */
        if(cert == issuer || X509_cmp(cert, issuer) == 0) {
            ret = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
            continue;
        }

        /* Detect (endless) loops of issuers. The last one can be skipped by the
         * check for self-signed just before. */
        for(int i = 0; i < depth; i++) {
            if(old_issuers[i] == issuer)
                return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
        }
        old_issuers[depth] = issuer;

        /* We have found the issuer certificate used for the signature. Recurse
         * to the next certificate in the chain (verify the current issuer). */
        ret = openSSL_verifyChain(ctx, stack, old_issuers, issuer, depth + 1);
    }

    /* Is the certificate in the trust list? If yes, then we are done. */
    if(ret == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED) {
        for(int i = 0; i < sk_X509_num(ctx->trustedCertificates); i++) {
            if(X509_cmp(cert, sk_X509_value(ctx->trustedCertificates, i)) == 0)
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

    /* Verification Step: Certificate Structure */
    STACK_OF(X509) *stack = openSSLLoadCertificateStack(*certificate);
    if(!stack || sk_X509_num(stack) < 1) {
        if(stack)
            sk_X509_pop_free(stack, X509_free);
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

    /* Verification Step: Certificate Usage
     * Check whether the certificate is a User certificate or a CA certificate.
     * Refer the test case CTT/Security/Security Certificate Validation/029.js
     * for more details. */
    X509 *leaf = sk_X509_value(stack, 0);
    if(X509_check_ca(leaf)) {
        sk_X509_pop_free(stack, X509_free);
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }

    /* These steps are performed outside of this method.
     * Because we need the server or client context.
     * - Security Policy
     * - Host Name
     * - URI */

    /* Verification Step: Build Certificate Chain
     * We perform the checks for each certificate inside. */
    X509 *old_issuers[UA_OPENSSL_MAX_CHAIN_LENGTH];
    UA_StatusCode ret = openSSL_verifyChain(context, stack, old_issuers, leaf, 0);
    sk_X509_pop_free(stack, X509_free);
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

UA_StatusCode
UA_CertificateUtils_verifyApplicationURI(UA_RuleHandling ruleHandling,
                                         const UA_ByteString *certificate,
                                         const UA_String *applicationURI) {
    const unsigned char * pData;
    X509 *                certificateX509;
    UA_String             subjectURI = UA_STRING_NULL;
    GENERAL_NAMES *       pNames;
    int                   i;
    UA_StatusCode         ret;

    pData = certificate->data;
    if (pData == NULL) {
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    certificateX509 = UA_OpenSSL_LoadCertificate(certificate);
    if (certificateX509 == NULL) {
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    pNames = (GENERAL_NAMES *) X509_get_ext_d2i(certificateX509, NID_subject_alt_name,
                                                NULL, NULL);
    if (pNames == NULL) {
        X509_free (certificateX509);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    UA_String_init(&subjectURI);

    for (i = 0; i < sk_GENERAL_NAME_num (pNames); i++) {
        GENERAL_NAME * value = sk_GENERAL_NAME_value (pNames, i);
        if (value->type == GEN_URI) {
            subjectURI.length = (size_t) (value->d.ia5->length);
            subjectURI.data = (UA_Byte *) UA_malloc (subjectURI.length);
            if (subjectURI.data == NULL) {
                X509_free (certificateX509);
                sk_GENERAL_NAME_pop_free(pNames, GENERAL_NAME_free);
                return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
            }
            (void) memcpy (subjectURI.data, value->d.ia5->data, subjectURI.length);
            break;
        }

    }

    ret = UA_STATUSCODE_GOOD;
    if (UA_Bstrstr (subjectURI.data, subjectURI.length,
                    applicationURI->data, applicationURI->length) == NULL) {
        ret = UA_STATUSCODE_BADCERTIFICATEURIINVALID;
    }

    if(ret != UA_STATUSCODE_GOOD && ruleHandling == UA_RULEHANDLING_DEFAULT) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "The certificate's application URI could not be verified. StatusCode %s",
                       UA_StatusCode_name(ret));
        ret = UA_STATUSCODE_GOOD;
    }

    X509_free (certificateX509);
    sk_GENERAL_NAME_pop_free(pNames, GENERAL_NAME_free);
    UA_String_clear (&subjectURI);
    return ret;
}

UA_StatusCode
UA_CertificateUtils_getExpirationDate(UA_ByteString *certificate,
                                      UA_DateTime *expiryDateTime) {
    const unsigned char *pData = certificate->data;
    X509 * x509 = d2i_X509 (NULL, &pData, (long)certificate->length);
    if (x509 == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the certificate Expiry date */
    ASN1_TIME *not_after = X509_get_notAfter(x509);

    struct tm dtTime;
    ASN1_TIME_to_tm(not_after, &dtTime);
    X509_free(x509);

    struct mytm dateTime;
    memset(&dateTime, 0, sizeof(struct mytm));
    dateTime.tm_year = dtTime.tm_year;
    dateTime.tm_mon = dtTime.tm_mon;
    dateTime.tm_mday = dtTime.tm_mday;
    dateTime.tm_hour = dtTime.tm_hour;
    dateTime.tm_min = dtTime.tm_min;
    dateTime.tm_sec = dtTime.tm_sec;

    long long sec_epoch = __tm_to_secs(&dateTime);
    *expiryDateTime = UA_DATETIME_UNIX_EPOCH;
    *expiryDateTime += sec_epoch * UA_DATETIME_SEC;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CertificateUtils_getSubjectName(UA_ByteString *certificate,
                                   UA_String *subjectName) {
    const unsigned char *pData = certificate->data;
    X509 *x509 = d2i_X509 (NULL, &pData, (long)certificate->length);
    if(!x509)
        return UA_STATUSCODE_BADINTERNALERROR;
    X509_NAME *sn = X509_get_subject_name(x509);
    char buf[1024];
    *subjectName = UA_STRING_ALLOC(X509_NAME_oneline(sn, buf, 1024));
    X509_free(x509);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CertificateUtils_getThumbprint(UA_ByteString *certificate,
                                  UA_String *thumbprint) {
    if(certificate == NULL || thumbprint->length != (SHA1_DIGEST_LENGTH * 2))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    BIO *certBio = BIO_new_mem_buf(certificate->data, certificate->length);
    if(!certBio)
        return UA_STATUSCODE_BADINTERNALERROR;

    X509 *cert = NULL;
    /* Try to read the certificate as PEM first */
    cert = PEM_read_bio_X509(certBio, NULL, 0, NULL);
    if(!cert) {
        /* If PEM read fails, reset BIO and try reading as DER */
        BIO_reset(certBio);
        cert = d2i_X509_bio(certBio, NULL);
    }
    if(!cert) {
        BIO_free(certBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    unsigned char digest[SHA1_DIGEST_LENGTH];
    unsigned int digestLen;
    if(X509_digest(cert, EVP_sha1(), digest, &digestLen) != 1) {
        X509_free(cert);
        BIO_free(certBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_String thumb = UA_STRING_NULL;
    thumb.length = (SHA1_DIGEST_LENGTH * 2) + 1;
    thumb.data = (UA_Byte*)malloc(sizeof(UA_Byte) * thumb.length);

    /* Create a string containing a hex representation */
    char *p = (char*)thumb.data;
    for(size_t i = 0; i < digestLen; i++) {
        p += sprintf(p, "%.2X", digest[i]);
    }

    memcpy(thumbprint->data, thumb.data, thumbprint->length);

    X509_free(cert);
    BIO_free(certBio);
    free(thumb.data);

    return retval;
}

UA_StatusCode
UA_CertificateUtils_getKeySize(UA_ByteString *certificate,
                               size_t *keySize){
    if(certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    BIO *certBio = BIO_new_mem_buf(certificate->data, certificate->length);
    if(!certBio)
        return UA_STATUSCODE_BADINTERNALERROR;

    X509 *cert = NULL;
    /* Try to read the certificate as PEM first */
    cert = PEM_read_bio_X509(certBio, NULL, 0, NULL);
    if(!cert) {
        /* If PEM read fails, reset BIO and try reading as DER */
        BIO_reset(certBio);
        cert = d2i_X509_bio(certBio, NULL);
    }
    if(!cert) {
        BIO_free(certBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    EVP_PKEY *pkey = X509_get_pubkey(cert);
    if(!pkey) {
        X509_free(cert);
        BIO_free(certBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(EVP_PKEY_base_id(pkey) == EVP_PKEY_RSA) {
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
        *keySize = EVP_PKEY_get_size(pkey) * 8;
#else
        *keySize =  RSA_size(get_pkey_rsa(pkey)) * 8;
#endif
    } else {
        EVP_PKEY_free(pkey);
        X509_free(cert);
        BIO_free(certBio);
        return UA_STATUSCODE_BADINTERNALERROR; /* Unsupported key type */
    }

    EVP_PKEY_free(pkey);
    X509_free(cert);
    BIO_free(certBio);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_CertificateUtils_comparePublicKeys(const UA_ByteString *certificate1,
                                      const UA_ByteString *certificate2) {
    if(certificate1 == NULL || certificate2 == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    BIO *dataBio1 = BIO_new_mem_buf(certificate1->data, certificate1->length);
    if(!dataBio1)
        return UA_STATUSCODE_BADINTERNALERROR;

    BIO *dataBio2 = BIO_new_mem_buf(certificate2->data, certificate2->length);
    if(!dataBio2) {
        BIO_free(dataBio1);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    X509 *cert1 = NULL;
    X509 *cert2 = NULL;
    X509_REQ *csr1 = NULL;
    X509_REQ *csr2 = NULL;
    /* Try to read the certificate as PEM first */
    cert1 = PEM_read_bio_X509(dataBio1, NULL, 0, NULL);
    if(!cert1) {
        /* If PEM read fails, reset BIO and try reading as DER */
        BIO_reset(dataBio1);
        cert1 = d2i_X509_bio(dataBio1, NULL);
    }
    /* Try to read as a csr */
    if(!cert1) {
        BIO_reset(dataBio1);
        csr1 = PEM_read_bio_X509_REQ(dataBio1, NULL, 0, NULL);
        if(!csr1) {
            BIO_reset(dataBio1);
            csr1 = d2i_X509_REQ_bio(dataBio1, NULL);
        }
    }
    if(!cert1 && !csr1) {
        BIO_free(dataBio1);
        BIO_free(dataBio2);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    cert2 = PEM_read_bio_X509(dataBio2, NULL, 0, NULL);
    if(!cert2) {
        /* If PEM read fails, reset BIO and try reading as DER */
        BIO_reset(dataBio2);
        cert2 = d2i_X509_bio(dataBio2, NULL);
    }
    /* Try to load as a csr */
    if(!cert2) {
        BIO_reset(dataBio2);
        csr2 = PEM_read_bio_X509_REQ(dataBio2, NULL, 0, NULL);
        if(!csr2) {
            BIO_reset(dataBio2);
            csr2 = d2i_X509_REQ_bio(dataBio2, NULL);
        }
    }
    if(!cert2 && !csr2) {
        X509_free(cert1);
        X509_REQ_free(csr1);
        BIO_free(dataBio1);
        BIO_free(dataBio2);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    BIO_free(dataBio1);
    BIO_free(dataBio2);

    EVP_PKEY *pkey1 = cert1 ? X509_get_pubkey(cert1) : X509_REQ_get_pubkey(csr1);
    EVP_PKEY *pkey2 = cert2 ? X509_get_pubkey(cert2) : X509_REQ_get_pubkey(csr2);

    X509_free(cert1);
    X509_free(cert2);
    X509_REQ_free(csr1);
    X509_REQ_free(csr2);

    if(!pkey1 || !pkey2) {
        EVP_PKEY_free(pkey1);
        EVP_PKEY_free(pkey2);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    int isEqual = EVP_PKEY_eq(pkey1, pkey2);
    if(isEqual == 0)
        retval = UA_STATUSCODE_BADNOMATCH;
    if(isEqual < 0)
        retval = UA_STATUSCODE_BADINTERNALERROR;
#else
    int isEqual = EVP_PKEY_cmp(pkey1, pkey2);
    if(isEqual == 0)
        retval = UA_STATUSCODE_BADNOMATCH;
    if(isEqual < 0)
        retval = UA_STATUSCODE_BADINTERNALERROR;
#endif

    EVP_PKEY_free(pkey1);
    EVP_PKEY_free(pkey2);

    return retval;
}

UA_StatusCode
UA_CertificateUtils_checkKeyPair(const UA_ByteString *certificate,
                                 const UA_ByteString *privateKey) {
    if(certificate == NULL || privateKey == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    X509 *cert = NULL;
    EVP_PKEY *pkey = NULL;

    BIO *certBio = BIO_new_mem_buf(certificate->data, certificate->length);
    if (!certBio) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    BIO *pkeyBio = BIO_new_mem_buf(privateKey->data, privateKey->length);
    if (!pkeyBio) {
        BIO_free(certBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Try to read the certificate as PEM first */
    cert = PEM_read_bio_X509(certBio, NULL, 0, NULL);
    if(!cert) {
        /* If PEM read fails, reset BIO and try reading as DER */
        BIO_reset(certBio);
        cert = d2i_X509_bio(certBio, NULL);
    }
    if(!cert) {
        BIO_free(certBio);
        BIO_free(pkeyBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Try to read the privateKey as PEM first */
    pkey = PEM_read_bio_PrivateKey(pkeyBio, NULL, NULL, NULL);
    if(!pkey) {
        /* If PEM read fails, reset BIO and try reading as DER */
        BIO_reset(pkeyBio);
        pkey = d2i_PrivateKey_bio(pkeyBio, NULL);
    }
    if(!pkey) {
        BIO_free(certBio);
        X509_free(cert);
        BIO_free(pkeyBio);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Verify if the private key matches the public key in the certificate */
    if(X509_check_private_key(cert, pkey) != 1) {
        BIO_free(certBio);
        X509_free(cert);
        BIO_free(pkeyBio);
        EVP_PKEY_free(pkey);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    X509_free(cert);
    EVP_PKEY_free(pkey);
    BIO_free(certBio);
    BIO_free(pkeyBio);

    return UA_STATUSCODE_GOOD;
}

static int
privateKeyPasswordCallback(char *buf, int size, int rwflag, void *userdata) {
    (void) rwflag;
    UA_ByteString *pw = (UA_ByteString*)userdata;
    if(pw->length <= (size_t)size)
        memcpy(buf, pw->data, pw->length);
    return (int)pw->length;
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

    /* Decrypt */
    BIO *bio = BIO_new_mem_buf((void*)privateKey.data, (int)privateKey.length);
    EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL,
                                             privateKeyPasswordCallback,
                                             (void*)(uintptr_t)&password);
    BIO_free(bio);
    if(!pkey)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Write DER encoded, allocates the new memory */
    unsigned char *data = NULL;
    const int numBytes = i2d_PrivateKey(pkey, &data);
    EVP_PKEY_free(pkey);
    if(!data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy to the data to outDerKey
     * Passing the data pointer directly causes a heap corruption on Windows
     * when outDerKey is cleared.
     */
    UA_ByteString temp = UA_BYTESTRING_NULL;
    temp.data = data;
    temp.length = (size_t)numBytes;
    const UA_StatusCode success = UA_ByteString_copy(&temp, outDerKey);
    /* OPENSSL_clear_free() is not supported by the LibreSSL version in the CI */
    OPENSSL_cleanse(data, numBytes);
    OPENSSL_free(data);
    return success;
}

#endif
