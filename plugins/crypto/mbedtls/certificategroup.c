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
#include <mbedtls/psa_util.h>
#include <mbedtls/platform_util.h>

#include "securitypolicy_common.h"
#include "securitypolicy_mbedtls_compat.h"

/* Configuration parameters */

#define MEMORYCERTSTORE_PARAMETERSSIZE 2
#define MEMORYCERTSTORE_PARAMINDEX_MAXTRUSTLISTSIZE 0
#define MEMORYCERTSTORE_PARAMINDEX_MAXREJECTEDLISTSIZE 1

static const struct {
    UA_QualifiedName name;
    const UA_DataType *type;
    UA_Boolean required;
} MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("max-trust-listsize")}, &UA_TYPES[UA_TYPES_UINT32], false},
    {{0, UA_STRING_STATIC("max-rejected-listsize")}, &UA_TYPES[UA_TYPES_UINT32], false}
};

typedef struct {
    UA_TrustListDataType trustList;
    size_t rejectedCertificatesSize;
    UA_ByteString *rejectedCertificates;

    UA_UInt32 maxTrustListSize;
    UA_UInt32 maxRejectedListSize;

    mbedtls_x509_crt trustedCertificates;
    mbedtls_x509_crt issuerCertificates;
    mbedtls_x509_crl trustedCrls;
    mbedtls_x509_crl issuerCrls;
} MemoryCertStore;

static UA_Boolean mbedtlsCheckCA(mbedtls_x509_crt *cert);

static UA_Boolean
certificateGroupValidByteString(const UA_ByteString *value) {
    return value && (value->length == 0 || value->data);
}

typedef UA_StatusCode
(*TrustListMutation)(const UA_TrustListDataType *src,
                     UA_TrustListDataType *dst);

static UA_StatusCode
MemoryCertStore_updateTrustList(UA_CertificateGroup *certGroup,
                                const UA_TrustListDataType *trustList,
                                TrustListMutation mutation);

static UA_StatusCode
MemoryCertStore_removeFromTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(!certGroup || !certGroup->context || !trustList)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return MemoryCertStore_updateTrustList(
        certGroup, trustList, UA_TrustListDataType_remove);
}

static UA_StatusCode
MemoryCertStore_getTrustList(UA_CertificateGroup *certGroup, UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(!certGroup || !certGroup->context || !trustList)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    return UA_TrustListDataType_copy(&context->trustList, trustList);
}

static UA_StatusCode
MemoryCertStore_setTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(!certGroup || !certGroup->context || !trustList)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return MemoryCertStore_updateTrustList(
        certGroup, trustList, UA_TrustListDataType_set);
}

static UA_StatusCode
MemoryCertStore_addToTrustList(UA_CertificateGroup *certGroup, const UA_TrustListDataType *trustList) {
    /* Check parameter */
    if(!certGroup || !certGroup->context || !trustList)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return MemoryCertStore_updateTrustList(
        certGroup, trustList, UA_TrustListDataType_add);
}

static UA_StatusCode
MemoryCertStore_getRejectedList(UA_CertificateGroup *certGroup, UA_ByteString **rejectedList, size_t *rejectedListSize) {
    /* Check parameter */
    if(!certGroup || !certGroup->context || !rejectedList || !rejectedListSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    UA_StatusCode retval = UA_Array_copy(context->rejectedCertificates, context->rejectedCertificatesSize,
                                         (void**)rejectedList, &UA_TYPES[UA_TYPES_BYTESTRING]);

    if(retval == UA_STATUSCODE_GOOD) {
        *rejectedListSize = context->rejectedCertificatesSize;
    } else {
        *rejectedList = NULL;
        *rejectedListSize = 0;
    }

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
        mbedtls_x509_crt_free(&cert);
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
            mbedtls_x509_crl_free(&crl);
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
    if(!certGroup || !certGroup->context || !certificate || !crls || !crlsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

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
    if(!certGroup || !certGroup->context || !certificate)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

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
    /* Evict only the oldest entry instead of dropping the entire history. */
    UA_ByteString_clear(&context->rejectedCertificates[0]);
    if(context->rejectedCertificatesSize > 1) {
        memmove(&context->rejectedCertificates[0],
                &context->rejectedCertificates[1],
                (context->rejectedCertificatesSize - 1) * sizeof(UA_ByteString));
    }
    context->rejectedCertificatesSize--;
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
parseCertificates(const UA_ByteString *certificates, size_t certificatesSize,
                  mbedtls_x509_crt *target) {
    if(certificatesSize > 0 && !certificates)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    for(size_t i = 0; i < certificatesSize; i++) {
        UA_ByteString data = UA_BYTESTRING_NULL;
        UA_StatusCode retval =
            UA_mbedTLS_CopyDataFormatAware(&certificates[i], &data);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        int err = mbedtls_x509_crt_parse(target, data.data, data.length);
        UA_ByteString_clear(&data);
        if(err)
            return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
parseCrls(const UA_ByteString *crls, size_t crlsSize,
          mbedtls_x509_crl *target) {
    if(crlsSize > 0 && !crls)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    for(size_t i = 0; i < crlsSize; i++) {
        UA_ByteString data = UA_BYTESTRING_NULL;
        UA_StatusCode retval = UA_mbedTLS_CopyDataFormatAware(&crls[i], &data);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        int err = mbedtls_x509_crl_parse(target, data.data, data.length);
        UA_ByteString_clear(&data);
        if(err)
            return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

typedef struct {
    mbedtls_x509_crt trustedCertificates;
    mbedtls_x509_crt issuerCertificates;
    mbedtls_x509_crl trustedCrls;
    mbedtls_x509_crl issuerCrls;
} ParsedCertStore;

static void
ParsedCertStore_init(ParsedCertStore *store) {
    mbedtls_x509_crt_init(&store->trustedCertificates);
    mbedtls_x509_crt_init(&store->issuerCertificates);
    mbedtls_x509_crl_init(&store->trustedCrls);
    mbedtls_x509_crl_init(&store->issuerCrls);
}

static void
ParsedCertStore_clear(ParsedCertStore *store) {
    mbedtls_x509_crt_free(&store->trustedCertificates);
    mbedtls_x509_crt_free(&store->issuerCertificates);
    mbedtls_x509_crl_free(&store->trustedCrls);
    mbedtls_x509_crl_free(&store->issuerCrls);
}

static UA_StatusCode
ParsedCertStore_load(const UA_TrustListDataType *trustList,
                     ParsedCertStore *store) {
    if(!trustList || !store)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval =
        parseCertificates(trustList->trustedCertificates,
                          trustList->trustedCertificatesSize,
                          &store->trustedCertificates);
    if(retval == UA_STATUSCODE_GOOD)
        retval = parseCertificates(trustList->issuerCertificates,
                                   trustList->issuerCertificatesSize,
                                   &store->issuerCertificates);
    if(retval == UA_STATUSCODE_GOOD)
        retval = parseCrls(trustList->trustedCrls,
                           trustList->trustedCrlsSize, &store->trustedCrls);
    if(retval == UA_STATUSCODE_GOOD)
        retval = parseCrls(trustList->issuerCrls,
                           trustList->issuerCrlsSize, &store->issuerCrls);
    return retval;
}

static void
ParsedCertStore_commit(MemoryCertStore *context, ParsedCertStore *store) {
    mbedtls_x509_crt_free(&context->trustedCertificates);
    mbedtls_x509_crt_free(&context->issuerCertificates);
    mbedtls_x509_crl_free(&context->trustedCrls);
    mbedtls_x509_crl_free(&context->issuerCrls);
    context->trustedCertificates = store->trustedCertificates;
    context->issuerCertificates = store->issuerCertificates;
    context->trustedCrls = store->trustedCrls;
    context->issuerCrls = store->issuerCrls;
    ParsedCertStore_init(store);
}

static UA_StatusCode
MemoryCertStore_updateTrustList(UA_CertificateGroup *certGroup,
                                const UA_TrustListDataType *trustList,
                                TrustListMutation mutation) {
    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;
    UA_TrustListDataType candidate;
    UA_TrustListDataType_init(&candidate);
    UA_StatusCode retval =
        UA_TrustListDataType_copy(&context->trustList, &candidate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = mutation(trustList, &candidate);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanupCandidate;
    if(context->maxTrustListSize != 0 &&
       UA_TrustListDataType_getSize(&candidate) > context->maxTrustListSize) {
        retval = UA_STATUSCODE_BADOUTOFRANGE;
        goto cleanupCandidate;
    }

    ParsedCertStore parsed;
    ParsedCertStore_init(&parsed);
    retval = ParsedCertStore_load(&candidate, &parsed);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanupParsed;

    UA_TrustListDataType_clear(&context->trustList);
    context->trustList = candidate;
    UA_TrustListDataType_init(&candidate);
    ParsedCertStore_commit(context, &parsed);

cleanupParsed:
    ParsedCertStore_clear(&parsed);
cleanupCandidate:
    UA_TrustListDataType_clear(&candidate);
    return retval;
}

#define UA_MBEDTLS_MAX_CHAIN_LENGTH 10
#define UA_MBEDTLS_MAX_DN_LENGTH 256

/* Is the certificate a CA? */
static UA_Boolean
mbedtlsCheckCA(mbedtls_x509_crt *cert) {
    /* The Basic Constraints extension must be set and the cert acts as CA */
    if(!mbedtls_x509_crt_has_ext_type(cert, MBEDTLS_X509_EXT_BASIC_CONSTRAINTS) ||
       !mbedtls_x509_crt_get_ca_istrue(cert))
        return false;

    /* The Key Usage extension must be set to cert signing and CRL issuing */
    if(!mbedtls_x509_crt_has_ext_type(cert, MBEDTLS_X509_EXT_KEY_USAGE) ||
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
            /* Compare issuer name and subject name. Signature verification
             * below rejects candidates with an incompatible key. */
            if(mbedtlsSameName(issuerName, &i->subject))
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
static UA_StatusCode
mbedtlsVerifyChain(UA_CertificateGroup *cg, MemoryCertStore *ctx, mbedtls_x509_crt *stack,
                   mbedtls_x509_crt **old_issuers, mbedtls_x509_crt *cert, int depth) {
    /* Maxiumum chain length */
    if(depth == UA_MBEDTLS_MAX_CHAIN_LENGTH)
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;


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
        if(!UA_mbedTLS_compat_verifyCertificateSignature(cert, issuer)) {
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
            if(mbedtlsSameBuf(&cert->tbs, &t->tbs)) {
                ret = UA_STATUSCODE_GOOD;
                break;
            }
        }
    }

    if(ret == UA_STATUSCODE_GOOD) {
        /* Verification Step: Validity Period */
        if(mbedtls_x509_time_is_future(&cert->valid_from) ||
        mbedtls_x509_time_is_past(&cert->valid_to))
            return (depth == 0) ? UA_STATUSCODE_BADCERTIFICATETIMEINVALID :
                UA_STATUSCODE_BADCERTIFICATEISSUERTIMEINVALID;
    }

    return ret;
}

/* This follows Part 6, 6.1.3 Determining if a Certificate is trusted.
 * It defines a sequence of steps for certificate verification. */
static UA_StatusCode
verifyCertificate(UA_CertificateGroup *certGroup, const UA_ByteString *certificate) {
    /* Check parameter */
    if(!certGroup || !certGroup->context || !certificate ||
       (certificate->length > 0 && !certificate->data))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    MemoryCertStore *context = (MemoryCertStore *)certGroup->context;

    /* Verification Step: Certificate Structure
     * This parses the entire certificate chain contained in the bytestring. */
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data,
                                         certificate->length);
    if(mbedErr) {
        mbedtls_x509_crt_free(&cert);
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

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
    if(!certGroup || !certGroup->context || !certificate) {
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

    if(!certGroup || !certificateGroupId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_mbedTLS_PSA_Init();
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Clear if the plugin is already initialized */
    if(certGroup->clear)
        certGroup->clear(certGroup);

    certGroup->logging = logger;

    certGroup->getTrustList = MemoryCertStore_getTrustList;
    certGroup->setTrustList = MemoryCertStore_setTrustList;
    certGroup->addToTrustList = MemoryCertStore_addToTrustList;
    certGroup->removeFromTrustList = MemoryCertStore_removeFromTrustList;
    certGroup->getRejectedList = MemoryCertStore_getRejectedList;
    certGroup->getCertificateCrls = MemoryCertStore_getCertificateCrls;
    certGroup->verifyCertificate = MemoryCertStore_verifyCertificate;
    certGroup->clear = MemoryCertStore_clear;

    retval = UA_NodeId_copy(certificateGroupId, &certGroup->certificateGroupId);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Set PKI Store context data */
    MemoryCertStore *context = (MemoryCertStore *)UA_calloc(1, sizeof(MemoryCertStore));
    if(!context) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    certGroup->context = context;
    mbedtls_x509_crt_init(&context->trustedCertificates);
    mbedtls_x509_crt_init(&context->issuerCertificates);
    mbedtls_x509_crl_init(&context->trustedCrls);
    mbedtls_x509_crl_init(&context->issuerCrls);
    /* Default values */
    context->maxTrustListSize = 65535;
    context->maxRejectedListSize = 100;

    if(params) {
        const UA_UInt32 *maxTrustListSize = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params, MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMINDEX_MAXTRUSTLISTSIZE].name,
                                 MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMINDEX_MAXTRUSTLISTSIZE].type);

        const UA_UInt32 *maxRejectedListSize = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params, MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMINDEX_MAXREJECTEDLISTSIZE].name,
                                 MemoryCertStoreParameters[MEMORYCERTSTORE_PARAMINDEX_MAXREJECTEDLISTSIZE].type);

        if(maxTrustListSize) {
            context->maxTrustListSize = *maxTrustListSize;
        }

        if(maxRejectedListSize) {
            context->maxRejectedListSize = *maxRejectedListSize;
        }
    }

    if(trustList) {
        retval = MemoryCertStore_updateTrustList(
            certGroup, trustList, UA_TrustListDataType_add);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    return UA_STATUSCODE_GOOD;

cleanup:
    certGroup->clear(certGroup);
    return retval;
}

UA_StatusCode
UA_CertificateUtils_verifyApplicationUri(const UA_ByteString *certificate,
                                         const UA_String *applicationURI) {
    if(!certificateGroupValidByteString(certificate) || !certificateGroupValidByteString(applicationURI))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    /* Parse the certificate */
    mbedtls_x509_crt remoteCertificate;
    mbedtls_x509_crt_init(&remoteCertificate);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_x509_crt_free(&remoteCertificate);
        return retval;
    }

    /* Get the Subject Alternative Name and compare */
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
        mbedtls_x509_free_subject_alt_name(&san);
        if(found) {
            retval = UA_STATUSCODE_GOOD;
            break;
        }
    }

    mbedtls_x509_crt_free(&remoteCertificate);
    return retval;
}

UA_StatusCode
UA_CertificateUtils_getExpirationDate(UA_ByteString *certificate,
                                      UA_DateTime *expiryDateTime) {
    if(!certificateGroupValidByteString(certificate) || !expiryDateTime)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &publicKey);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_x509_crt_free(&publicKey);
        return retval;
    }

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
    if(!certificateGroupValidByteString(certificate) || !subjectName)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String_init(subjectName);
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
        mbedtls_x509_crt_free(&publicKey);
        retval = UA_mbedTLS_LoadCrl(certificate, &crl);
        if(retval != UA_STATUSCODE_GOOD) {
            mbedtls_x509_crl_free(&crl);
            return retval;
        }
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
    if(!certificateGroupValidByteString(certificate) || !thumbprint || !thumbprint->data ||
       thumbprint->length != (UA_SHA1_LENGTH * 2))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    // prepare temporary to hold the binary thumbprint
    UA_Byte buf[UA_SHA1_LENGTH];
    UA_ByteString thumbpr = {
        /*.length =*/ sizeof(buf),
        /*.data =*/ buf
    };

    UA_StatusCode retval = UA_mbedTLS_thumbprintSha1(certificate, &thumbpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
    if(!certificateGroupValidByteString(certificate) || !keySize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *keySize = 0;
    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &publicKey);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_x509_crt_free(&publicKey);
        return retval;
    }

    *keySize = mbedtls_pk_get_bitlen(&publicKey.pk);
    mbedtls_x509_crt_free(&publicKey);
    return (*keySize > 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTSUPPORTED;
}

UA_StatusCode
UA_CertificateUtils_comparePublicKeys(const UA_ByteString *certificate1,
                                      const UA_ByteString *certificate2) {
    if(!certificateGroupValidByteString(certificate1) || !certificateGroupValidByteString(certificate2))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval;

    mbedtls_x509_crt cert1;
    mbedtls_x509_crt cert2;
    mbedtls_x509_csr csr1;
    mbedtls_x509_csr csr2;

    UA_ByteString data1 = UA_BYTESTRING_NULL;
    UA_ByteString data2 = UA_BYTESTRING_NULL;

    mbedtls_x509_crt_init(&cert1);
    mbedtls_x509_crt_init(&cert2);
    mbedtls_x509_csr_init(&csr1);
    mbedtls_x509_csr_init(&csr2);

    retval = UA_mbedTLS_CopyDataFormatAware(certificate1, &data1);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    retval = UA_mbedTLS_CopyDataFormatAware(certificate2, &data2);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    int mbedErr = mbedtls_x509_crt_parse(&cert1, data1.data, data1.length);
    if(mbedErr) {
        /* Try to load as a csr */
        mbedErr = mbedtls_x509_csr_parse(&csr1, data1.data, data1.length);
        if(mbedErr) {
            retval = UA_STATUSCODE_BADCERTIFICATEINVALID;
            goto cleanup;
        }
    }

    retval = UA_STATUSCODE_GOOD;

    mbedErr = mbedtls_x509_crt_parse(&cert2, data2.data, data2.length);
    if(mbedErr) {
        /* Try to load as a csr */
        mbedErr = mbedtls_x509_csr_parse(&csr2, data2.data, data2.length);
        if(mbedErr) {
            retval = UA_STATUSCODE_BADCERTIFICATEINVALID;
            goto cleanup;
        }
    }

    mbedtls_pk_context *pk1 = cert1.pk_raw.p ? &cert1.pk : &csr1.pk;
    mbedtls_pk_context *pk2 = cert2.pk_raw.p ? &cert2.pk : &csr2.pk;
    unsigned char pub1[4096];
    unsigned char pub2[4096];
    int len1 = mbedtls_pk_write_pubkey_der(pk1, pub1, sizeof(pub1));
    int len2 = mbedtls_pk_write_pubkey_der(pk2, pub2, sizeof(pub2));
    if(len1 <= 0 || len2 <= 0) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    if(len1 != len2 ||
       memcmp(pub1 + sizeof(pub1) - (size_t)len1,
              pub2 + sizeof(pub2) - (size_t)len2, (size_t)len1) != 0)
        retval = UA_STATUSCODE_BADNOMATCH;

cleanup:
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
    if(!certificateGroupValidByteString(certificate) || !certificateGroupValidByteString(privateKey))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pk;

    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&pk);

    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &cert);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = (UA_mbedTLS_LoadPrivateKey(privateKey, &pk) ?
              UA_STATUSCODE_BADSECURITYCHECKSFAILED : UA_STATUSCODE_GOOD);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Compare the encoded public keys. This avoids the version-specific
     * mbedtls_pk_check_pair API and works for both RSA and ECC keys. */
    unsigned char certPublicKey[4096];
    unsigned char privatePublicKey[4096];
    int certPublicKeySize = mbedtls_pk_write_pubkey_der(&cert.pk, certPublicKey,
                                                        sizeof(certPublicKey));
    int privatePublicKeySize = mbedtls_pk_write_pubkey_der(&pk, privatePublicKey,
                                                           sizeof(privatePublicKey));
    if(certPublicKeySize <= 0 || privatePublicKeySize <= 0 ||
       certPublicKeySize != privatePublicKeySize ||
       memcmp(certPublicKey + sizeof(certPublicKey) - (size_t)certPublicKeySize,
              privatePublicKey + sizeof(privatePublicKey) - (size_t)privatePublicKeySize,
              (size_t)certPublicKeySize) != 0)
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;

cleanup:
    mbedtls_pk_free(&pk);
    mbedtls_x509_crt_free(&cert);

    return retval;
}

UA_StatusCode
UA_CertificateUtils_checkCA(const UA_ByteString *certificate) {
    if(!certificateGroupValidByteString(certificate))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
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
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString_init(outDerKey);
    UA_StatusCode retval = UA_mbedTLS_PSA_Init();
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(privateKey.length == 0 || !privateKey.data ||
       (password.length > 0 && !password.data))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Already in DER format -> return verbatim.
     * DER-encoded keys start with ASN.1 SEQUENCE tag (0x30). PEM-encoded keys
     * start with "-----BEGIN" (0x2D). Check only the tag byte to handle both
     * short-form (< 128 bytes) and long-form length encodings. */
    if(privateKey.length > 1 && privateKey.data[0] == 0x30)
        return UA_ByteString_copy(&privateKey, outDerKey);

    /* Create a null-terminated string */
    UA_ByteString nullTerminatedKey = UA_BYTESTRING_NULL;
    retval = UA_mbedTLS_CopyDataFormatAware(&privateKey, &nullTerminatedKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Create the private-key context */
    mbedtls_pk_context ctx;
    mbedtls_pk_init(&ctx);
    unsigned char buf[1 << 14] = {0};
    int err = UA_mbedTLS_compat_parsePrivateKey(
        &ctx, nullTerminatedKey.data, nullTerminatedKey.length,
        password.data, password.length);
    UA_mbedTLS_clearSensitiveByteString(&nullTerminatedKey);
    if(err != 0) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto cleanup;
    }

    /* Write the DER-encoded key into a local buffer */
    int written = mbedtls_pk_write_key_der(&ctx, buf, sizeof(buf));
    if(written <= 0) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto cleanup;
    }
    size_t pos = (size_t)written;

    /* Allocate memory */
    retval = UA_ByteString_allocBuffer(outDerKey, pos);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Copy to the output */
    memcpy(outDerKey->data, &buf[sizeof(buf) - pos], pos);
    retval = UA_STATUSCODE_GOOD;

cleanup:
    mbedtls_platform_zeroize(buf, sizeof(buf));
    UA_mbedTLS_clearSensitiveByteString(&nullTerminatedKey);
    mbedtls_pk_free(&ctx);
    if(retval != UA_STATUSCODE_GOOD)
        UA_mbedTLS_clearSensitiveByteString(outDerKey);
    return retval;
}

UA_StatusCode
UA_CertificateUtils_getCertCommonName(const UA_ByteString *certificate, UA_String *commonName) {
    if(!certificateGroupValidByteString(certificate) || certificate->length == 0 || !commonName)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_String_init(commonName);

    mbedtls_x509_crt publicKey;
    mbedtls_x509_crt_init(&publicKey);

    UA_StatusCode retval =
        UA_mbedTLS_LoadCertificate((UA_ByteString*)(uintptr_t)certificate,
                                   &publicKey);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_x509_crt_free(&publicKey);
        return retval;
    }

    for(mbedtls_x509_name *name = &publicKey.subject;
        name != NULL;
        name = name->next) {
        if(MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &name->oid) == 0) {
            UA_String tmp = {
                (size_t)name->val.len,
                (UA_Byte*)name->val.p
            };
            retval = UA_String_copy(&tmp, commonName);

            break;
        }
    }

    mbedtls_x509_crt_free(&publicKey);
    return retval;
}
#endif
