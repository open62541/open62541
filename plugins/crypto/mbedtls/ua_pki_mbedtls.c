/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2019, 2024 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#include <open62541/util.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <mbedtls/x509.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <string.h>

#define UA_MBEDTLS_MAX_CHAIN_LENGTH 10
#define UA_MBEDTLS_MAX_DN_LENGTH 256

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

// mbedTLS expects PEM data to be null terminated
// The data length parameter must include the null terminator
static UA_ByteString copyDataFormatAware(const UA_ByteString *data)
{
    UA_ByteString result;
    UA_ByteString_init(&result);

    if (!data->length)
        return result;

    if (data->length && data->data[0] == '-') {
        UA_ByteString_allocBuffer(&result, data->length + 1);
        memcpy(result.data, data->data, data->length);
        result.data[data->length] = '\0';
    } else {
        UA_ByteString_copy(data, &result);
    }

    return result;
}

typedef struct {
    /* If the folders are defined, we use them to reload the certificates during
     * runtime */
    UA_String trustListFolder;
    UA_String issuerListFolder;
    UA_String revocationListFolder;

    mbedtls_x509_crt certificateTrustList;
    mbedtls_x509_crt certificateIssuerList;
    mbedtls_x509_crl certificateRevocationList;
} CertInfo;

#ifdef __linux__ /* Linux only so far */

#include <dirent.h>
#include <limits.h>

static UA_StatusCode
fileNamesFromFolder(const UA_String *folder, size_t *pathsSize, UA_String **paths) {
    char buf[PATH_MAX + 1];
    if(folder->length > PATH_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;

    memcpy(buf, folder->data, folder->length);
    buf[folder->length] = 0;
    
    DIR *dir = opendir(buf);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    *paths = (UA_String*)UA_Array_new(256, &UA_TYPES[UA_TYPES_STRING]);
    if(*paths == NULL) {
        closedir(dir);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    struct dirent *ent;
    char buf2[PATH_MAX + 1];
    char *res = realpath(buf, buf2);
    if(!res) {
        closedir(dir);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    size_t pathlen = strlen(buf2);
    *pathsSize = 0;
    while((ent = readdir (dir)) != NULL && *pathsSize < 256) {
        if(ent->d_type != DT_REG)
            continue;
        buf2[pathlen] = '/';
        buf2[pathlen+1] = 0;
        strcat(buf2, ent->d_name);
        (*paths)[*pathsSize] = UA_STRING_ALLOC(buf2);
        *pathsSize += 1;
    }
    closedir(dir);

    if(*pathsSize == 0) {
        UA_free(*paths);
        *paths = NULL;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
reloadCertificates(CertInfo *ci) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    int err = 0;
    int internalErrorFlag = 0;

    /* Load the trustlists */
    if(ci->trustListFolder.length > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Reloading the trust-list");
        mbedtls_x509_crt_free(&ci->certificateTrustList);
        mbedtls_x509_crt_init(&ci->certificateTrustList);

        char f[PATH_MAX];
        memcpy(f, ci->trustListFolder.data, ci->trustListFolder.length);
        f[ci->trustListFolder.length] = 0;
        err = mbedtls_x509_crt_parse_path(&ci->certificateTrustList, f);
        if(err == 0) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "Loaded certificate from %s", f);
        } else {
            char errBuff[300];
            mbedtls_strerror(err, errBuff, 300);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "Failed to load certificate from %s, mbedTLS error: %s (error code: %d)", f, errBuff, err);
            internalErrorFlag = 1;
        }
    }

    /* Load the revocationlists */
    if(ci->revocationListFolder.length > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Reloading the revocation-list");
        size_t pathsSize = 0;
        UA_String *paths = NULL;
        retval = fileNamesFromFolder(&ci->revocationListFolder, &pathsSize, &paths);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        mbedtls_x509_crl_free(&ci->certificateRevocationList);
        mbedtls_x509_crl_init(&ci->certificateRevocationList);
        for(size_t i = 0; i < pathsSize; i++) {
            char f[PATH_MAX];
            memcpy(f, paths[i].data, paths[i].length);
            f[paths[i].length] = 0;
            err = mbedtls_x509_crl_parse_file(&ci->certificateRevocationList, f);
            if(err == 0) {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Loaded certificate from %.*s",
                            (int)paths[i].length, paths[i].data);
            } else {
                char errBuff[300];
                mbedtls_strerror(err, errBuff, 300);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to load certificate from %.*s, mbedTLS error: %s (error code: %d)",
                            (int)paths[i].length, paths[i].data, errBuff, err);
                internalErrorFlag = 1;
            }
        }
        UA_Array_delete(paths, pathsSize, &UA_TYPES[UA_TYPES_STRING]);
        paths = NULL;
        pathsSize = 0;
    }

    /* Load the issuerlists */
    if(ci->issuerListFolder.length > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Reloading the issuer-list");
        mbedtls_x509_crt_free(&ci->certificateIssuerList);
        mbedtls_x509_crt_init(&ci->certificateIssuerList);
        char f[PATH_MAX];
        memcpy(f, ci->issuerListFolder.data, ci->issuerListFolder.length);
        f[ci->issuerListFolder.length] = 0;
        err = mbedtls_x509_crt_parse_path(&ci->certificateIssuerList, f);
        if(err == 0) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "Loaded certificate from %s", f);
        } else {
            char errBuff[300];
            mbedtls_strerror(err, errBuff, 300);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "Failed to load certificate from %s, mbedTLS error: %s (error code: %d)", 
                        f, errBuff, err);
            internalErrorFlag = 1;
        }
    }

    return (internalErrorFlag) ? UA_STATUSCODE_BADINTERNALERROR : retval;
}

#endif

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
mbedtlsFindNextIssuer(CertInfo *ci, mbedtls_x509_crt *stack,
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
        if(stack == &ci->certificateTrustList)
            stack = NULL;
        else if(stack == &ci->certificateIssuerList)
            stack = &ci->certificateTrustList;
        else
            stack = &ci->certificateIssuerList;
    } while(stack);
    return NULL;
}

static UA_Boolean
mbedtlsCheckRevoked(CertInfo *ci, mbedtls_x509_crt *cert) {
    char inbuf[UA_MBEDTLS_MAX_DN_LENGTH];
    int nameLen = mbedtls_x509_dn_gets(inbuf, UA_MBEDTLS_MAX_DN_LENGTH, &cert->issuer);
    if(nameLen < 0)
        return true;
    UA_String issuerName = {(size_t)nameLen, (UA_Byte*)inbuf};
    for(mbedtls_x509_crl *crl = &ci->certificateRevocationList; crl; crl = crl->next) {
        /* Is the CRL for certificates from the cert issuer?
         * Is the serial number of the certificate contained in the CRL? */
        if(mbedtlsSameName(issuerName, &crl->issuer) &&
           mbedtls_x509_crt_is_revoked(cert, crl) != 0)
            return true;
    }
    return false;
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
mbedtlsVerifyChain(CertInfo *ci, mbedtls_x509_crt *stack, mbedtls_x509_crt **old_issuers,
                   mbedtls_x509_crt *cert, int depth) {
    /* Maxiumum chain length */
    if(depth == UA_MBEDTLS_MAX_CHAIN_LENGTH)
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;

    /* Verification Step: Validity Period */
    if(mbedtls_x509_time_is_future(&cert->valid_from) ||
       mbedtls_x509_time_is_past(&cert->valid_to))
        return (depth == 0) ? UA_STATUSCODE_BADCERTIFICATETIMEINVALID :
            UA_STATUSCODE_BADCERTIFICATEISSUERTIMEINVALID;

    /* Verification Step: Revocation Check */
    if(mbedtlsCheckRevoked(ci, cert))
        return (depth == 0) ? UA_STATUSCODE_BADCERTIFICATEREVOKED :
            UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED;

    /* Return the most specific error code. BADCERTIFICATECHAININCOMPLETE is
     * returned only if all possible chains are incomplete. */
    mbedtls_x509_crt *issuer = NULL;
    UA_StatusCode ret = UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    while(ret != UA_STATUSCODE_GOOD) {
        /* Find the issuer. This can return the same certificate if it is
         * self-signed (subject == issuer). We come back here to try a different
         * "path" if a subsequent verification fails. */
        issuer = mbedtlsFindNextIssuer(ci, stack, cert, issuer);
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
         * untrusted). */
        if(issuer == cert || mbedtlsSameBuf(&cert->tbs, &issuer->tbs)) {
            ret = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
            continue;
        }

        /* Detect (endless) loops of issuers. The last one can be skipped by the
         * check for self-signed just before. */
        for(int i = 0; i < depth - 1; i++) {
            if(old_issuers[i] == issuer)
                return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
        }
        old_issuers[depth] = issuer;

        /* We have found the issuer certificate used for the signature. Recurse
         * to the next certificate in the chain (verify the current issuer). */
        ret = mbedtlsVerifyChain(ci, stack, old_issuers, issuer, depth + 1);
    }

    /* The chain is complete, but we haven't yet identified a trusted
     * certificate "on the way down". Can we trust this certificate? */
    if(ret == UA_STATUSCODE_BADCERTIFICATEUNTRUSTED) {
        for(mbedtls_x509_crt *t = &ci->certificateTrustList; t; t = t->next) {
            if(mbedtlsSameBuf(&cert->tbs, &t->tbs))
                return UA_STATUSCODE_GOOD;
        }
    }

    return ret;
}

/* This follows Part 6, 6.1.3 Determining if a Certificate is trusted.
 * It defines a sequence of steps for certificate verification. */
static UA_StatusCode
certificateVerification_verify(void *verificationContext,
                                const UA_ByteString *certificate) {
    if(!verificationContext || !certificate)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    CertInfo *ci = (CertInfo*)verificationContext;

#ifdef __linux__ /* Reload certificates if folder paths are specified */
    ret = reloadCertificates(ci);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
#endif

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
    ret = mbedtlsVerifyChain(ci, &cert, old_issuers, &cert, 0);
    mbedtls_x509_crt_free(&cert);
    return ret;
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
    if(UA_Bstrstr(remoteCertificate.v3_ext.p, remoteCertificate.v3_ext.len,
               applicationURI->data, applicationURI->length) == NULL)
        retval = UA_STATUSCODE_BADCERTIFICATEURIINVALID;

    mbedtls_x509_crt_free(&remoteCertificate);
    return retval;
}

static void
certificateVerification_clear(UA_CertificateVerification *cv) {
    CertInfo *ci = (CertInfo*)cv->context;
    if(!ci)
        return;
    mbedtls_x509_crt_free(&ci->certificateTrustList);
    mbedtls_x509_crl_free(&ci->certificateRevocationList);
    mbedtls_x509_crt_free(&ci->certificateIssuerList);
    UA_String_clear(&ci->trustListFolder);
    UA_String_clear(&ci->issuerListFolder);
    UA_String_clear(&ci->revocationListFolder);
    UA_free(ci);
    cv->context = NULL;
}

UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateVerification *cv,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize) {
    CertInfo *ci = (CertInfo*)UA_malloc(sizeof(CertInfo));
    if(!ci)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(ci, 0, sizeof(CertInfo));
    mbedtls_x509_crt_init(&ci->certificateTrustList);
    mbedtls_x509_crl_init(&ci->certificateRevocationList);
    mbedtls_x509_crt_init(&ci->certificateIssuerList);

    cv->context = (void*)ci;
    cv->verifyCertificate = certificateVerification_verify;
    cv->clear = certificateVerification_clear;
    cv->verifyApplicationURI = certificateVerification_verifyApplicationURI;

    int err;
    UA_ByteString data;
    UA_ByteString_init(&data);

    for(size_t i = 0; i < certificateTrustListSize; i++) {
        data = copyDataFormatAware(&certificateTrustList[i]);
        err = mbedtls_x509_crt_parse(&ci->certificateTrustList,
                                     data.data,
                                     data.length);
        UA_ByteString_clear(&data);
        if(err)
            goto error;
    }
    for(size_t i = 0; i < certificateIssuerListSize; i++) {
        data = copyDataFormatAware(&certificateIssuerList[i]);
        err = mbedtls_x509_crt_parse(&ci->certificateIssuerList,
                                     data.data,
                                     data.length);
        UA_ByteString_clear(&data);
        if(err)
            goto error;
    }
    for(size_t i = 0; i < certificateRevocationListSize; i++) {
        data = copyDataFormatAware(&certificateRevocationList[i]);
        err = mbedtls_x509_crl_parse(&ci->certificateRevocationList,
                                     data.data,
                                     data.length);
        UA_ByteString_clear(&data);
        if(err)
            goto error;
    }

    return UA_STATUSCODE_GOOD;
error:
    certificateVerification_clear(cv);
    return UA_STATUSCODE_BADINTERNALERROR;
}

#ifdef __linux__ /* Linux only so far */

UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder) {
    CertInfo *ci = (CertInfo*)UA_malloc(sizeof(CertInfo));
    if(!ci)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(ci, 0, sizeof(CertInfo));
    mbedtls_x509_crt_init(&ci->certificateTrustList);
    mbedtls_x509_crl_init(&ci->certificateRevocationList);
    mbedtls_x509_crt_init(&ci->certificateIssuerList);

    /* Only set the folder paths. They will be reloaded during runtime.
     * TODO: Add a more efficient reloading of only the changes */
    ci->trustListFolder = UA_STRING_ALLOC(trustListFolder);
    ci->issuerListFolder = UA_STRING_ALLOC(issuerListFolder);
    ci->revocationListFolder = UA_STRING_ALLOC(revocationListFolder);

    reloadCertificates(ci);

    cv->context = (void*)ci;
    cv->verifyCertificate = certificateVerification_verify;
    cv->clear = certificateVerification_clear;
    cv->verifyApplicationURI = certificateVerification_verifyApplicationURI;

    return UA_STATUSCODE_GOOD;
}

#endif

#endif /* UA_ENABLE_ENCRYPTION_MBEDTLS */
