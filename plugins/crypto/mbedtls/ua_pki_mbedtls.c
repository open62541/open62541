/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2019, 2024 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#include <open62541/util.h>
#include <open62541/plugin/pki_default.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_mbedtls_common.h"

#include <mbedtls/x509.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/entropy.h>
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
        return NULL;
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
    UA_CertificateVerification *cv;

    /* If the folders are defined, we use them to reload the certificates during
     * runtime */
    UA_String trustListFolder;
    UA_String issuerListFolder;
    UA_String revocationListFolder;
    UA_String rejectedListFolder;

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
reloadCertificates(const UA_CertificateVerification *cv, CertInfo *ci) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    int err = 0;
    int internalErrorFlag = 0;

    /* Load the trustlists */
    if(ci->trustListFolder.length > 0) {
        UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER, "Reloading the trust-list");
        mbedtls_x509_crt_free(&ci->certificateTrustList);
        mbedtls_x509_crt_init(&ci->certificateTrustList);

        char f[PATH_MAX];
        memcpy(f, ci->trustListFolder.data, ci->trustListFolder.length);
        f[ci->trustListFolder.length] = 0;
        err = mbedtls_x509_crt_parse_path(&ci->certificateTrustList, f);
        if(err == 0) {
            UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER,
                        "Loaded certificate from %s", f);
        } else {
            char errBuff[300];
            mbedtls_strerror(err, errBuff, 300);
            UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER,
                        "Failed to load certificate from %s, mbedTLS error: %s (error code: %d)", f, errBuff, err);
            internalErrorFlag = 1;
        }
    }

    /* Load the revocationlists */
    if(ci->revocationListFolder.length > 0) {
        UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER, "Reloading the revocation-list");
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
                UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER,
                            "Loaded certificate from %.*s",
                            (int)paths[i].length, paths[i].data);
            } else {
                char errBuff[300];
                mbedtls_strerror(err, errBuff, 300);
                UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER,
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
        UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER, "Reloading the issuer-list");
        mbedtls_x509_crt_free(&ci->certificateIssuerList);
        mbedtls_x509_crt_init(&ci->certificateIssuerList);
        char f[PATH_MAX];
        memcpy(f, ci->issuerListFolder.data, ci->issuerListFolder.length);
        f[ci->issuerListFolder.length] = 0;
        err = mbedtls_x509_crt_parse_path(&ci->certificateIssuerList, f);
        if(err == 0) {
            UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER,
                        "Loaded certificate from %s", f);
        } else {
            char errBuff[300];
            mbedtls_strerror(err, errBuff, 300);
            UA_LOG_INFO(cv->logging, UA_LOGCATEGORY_SERVER,
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

static UA_StatusCode
mbedtlsCheckRevoked(CertInfo *ci, mbedtls_x509_crt *cert) {
    /* Parse the Issuer Name */
    char inbuf[UA_MBEDTLS_MAX_DN_LENGTH];
    int nameLen = mbedtls_x509_dn_gets(inbuf, UA_MBEDTLS_MAX_DN_LENGTH, &cert->issuer);
    if(nameLen < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_String issuerName = {(size_t)nameLen, (UA_Byte*)inbuf};

    if(ci->certificateRevocationList.raw.len == 0) {
        UA_LOG_WARNING(ci->cv->logging, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Zero revocation lists have been loaded. "
                       "This seems intentional - omitting the check.");
        return UA_STATUSCODE_GOOD;
    }

    /* Loop over the crl and match the Issuer Name */
    UA_StatusCode res = UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN;
    for(mbedtls_x509_crl *crl = &ci->certificateRevocationList; crl; crl = crl->next) {
        /* Is the CRL for certificates from the cert issuer?
         * Is the serial number of the certificate contained in the CRL? */
        if(mbedtlsSameName(issuerName, &crl->issuer)) {
            if(mbedtls_x509_crt_is_revoked(cert, crl) != 0)
                return UA_STATUSCODE_BADCERTIFICATEREVOKED;
            res = UA_STATUSCODE_GOOD; /* There was at least one crl that did not revoke (so far) */
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
         * untrusted).
         *
         * Break here as we have reached the end of the chain. Omit the
         * Revocation Check for self-signed certificates. */
        if(issuer == cert || mbedtlsSameBuf(&cert->tbs, &issuer->tbs)) {
            ret = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
            break;
        }

        /* Verification Step: Revocation Check */
        ret = mbedtlsCheckRevoked(ci, cert);
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
certificateVerification_verify(const UA_CertificateVerification *cv,
                               const UA_ByteString *certificate) {
    if(!cv || !certificate)
        return UA_STATUSCODE_BADINTERNALERROR;

    CertInfo *ci = (CertInfo*)cv->context;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    mbedtls_x509_crt *old_issuers[UA_MBEDTLS_MAX_CHAIN_LENGTH];

#ifdef __linux__ /* Reload certificates if folder paths are specified */
    ret = reloadCertificates(cv, ci);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
#endif

    /* Verification Step: Certificate Structure
     * This parses the entire certificate chain contained in the bytestring. */
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data,
                                         certificate->length);
    if(mbedErr) {
        ret = UA_STATUSCODE_BADCERTIFICATEINVALID;
        goto errout;
    }

    /* Verification Step: Certificate Usage
     * Check whether the certificate is a User certificate or a CA certificate.
     * Refer the test case CTT/Security/Security Certificate Validation/029.js
     * for more details. */
    if(mbedtlsCheckCA(&cert)) {
        ret = UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
        goto errout;
    }

    /* These steps are performed outside of this method.
     * Because we need the server or client context.
     * - Security Policy
     * - Host Name
     * - URI */

    /* Verification Step: Build Certificate Chain
     * We perform the checks for each certificate inside. */
    ret = mbedtlsVerifyChain(ci, &cert, old_issuers, &cert, 0);

 errout:
    mbedtls_x509_crt_free(&cert);

#ifdef UA_ENABLE_CERT_REJECTED_DIR
    if(ret != UA_STATUSCODE_GOOD &&
       ci->rejectedListFolder.length > 0) {
            char rejectedFileName[256] = {0};
            UA_ByteString thumbprint;
            UA_ByteString_allocBuffer(&thumbprint, UA_SHA1_LENGTH);
            if(mbedtls_thumbprint_sha1(certificate, &thumbprint) == UA_STATUSCODE_GOOD) {
                static const char hex2char[] = "0123456789ABCDEF";
                for(size_t pos = 0, namePos = 0; pos < thumbprint.length; pos++) {
                    rejectedFileName[namePos++] = hex2char[(thumbprint.data[pos] & 0xf0) >> 4];
                    rejectedFileName[namePos++] = hex2char[thumbprint.data[pos] & 0x0f];
                }
                strcat(rejectedFileName, ".der");
            } else {
                UA_UInt64 dt = (UA_UInt64) UA_DateTime_now();
                sprintf(rejectedFileName, "cert_%" PRIu64 ".der", dt);
            }
            UA_ByteString_clear(&thumbprint);
            char *rejectedFullFileName = (char *)
                calloc(ci->rejectedListFolder.length + 1 /* '/' */ + strlen(rejectedFileName) + 1, sizeof(char));
            if(!rejectedFullFileName)
                return ret;
            memcpy(rejectedFullFileName, ci->rejectedListFolder.data, ci->rejectedListFolder.length);
            rejectedFullFileName[ci->rejectedListFolder.length] = '/';
            memcpy(&rejectedFullFileName[ci->rejectedListFolder.length + 1], rejectedFileName, strlen(rejectedFileName));
            FILE * fp_rejectedFile = fopen(rejectedFullFileName, "wb");
            if(fp_rejectedFile) {
                fwrite(certificate->data, sizeof(certificate->data[0]), certificate->length, fp_rejectedFile);
                fclose(fp_rejectedFile);
            }
            free(rejectedFullFileName);
    }
#endif

    return ret;
}

static UA_StatusCode
certificateVerification_verifyApplicationURI(const UA_CertificateVerification *cv,
                                             const UA_ByteString *certificate,
                                             const UA_String *applicationURI) {
    CertInfo *ci;
    if(!cv)
        return UA_STATUSCODE_BADINTERNALERROR;
    ci = (CertInfo*)cv->context;
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
    UA_String_clear(&ci->rejectedListFolder);
    UA_free(ci);
    cv->context = NULL;
}

static UA_StatusCode
getCertificate_ExpirationDate(UA_DateTime *expiryDateTime, 
                              UA_ByteString *certificate) {
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

static UA_StatusCode
getCertificate_SubjectName(UA_String *subjectName,
                           UA_ByteString *certificate) {
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
UA_CertificateVerification_Trustlist(UA_CertificateVerification *cv,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize) {

    if(cv == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clear if the plugin is already initialized */
    if(cv->clear)
        cv->clear(cv);

    CertInfo *ci = (CertInfo*)UA_malloc(sizeof(CertInfo));
    if(!ci)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(ci, 0, sizeof(CertInfo));
    ci->cv = cv;
    mbedtls_x509_crt_init(&ci->certificateTrustList);
    mbedtls_x509_crl_init(&ci->certificateRevocationList);
    mbedtls_x509_crt_init(&ci->certificateIssuerList);

    cv->context = (void*)ci;
    cv->verifyCertificate = certificateVerification_verify;
    cv->clear = certificateVerification_clear;
    cv->verifyApplicationURI = certificateVerification_verifyApplicationURI;
    cv->getExpirationDate = getCertificate_ExpirationDate;
    cv->getSubjectName = getCertificate_SubjectName;

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

#ifdef UA_ENABLE_CERT_REJECTED_DIR
UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder,
                                       const char *rejectedListFolder) {
#else
UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder) {
#endif
    UA_StatusCode ret;
    if(cv == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(cv->logging == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clear if the plugin is already initialized */
    if(cv->clear)
        cv->clear(cv);

    CertInfo *ci = (CertInfo*)UA_malloc(sizeof(CertInfo));
    if(!ci)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(ci, 0, sizeof(CertInfo));
    ci->cv = cv;
    mbedtls_x509_crt_init(&ci->certificateTrustList);
    mbedtls_x509_crl_init(&ci->certificateRevocationList);
    mbedtls_x509_crt_init(&ci->certificateIssuerList);

    /* Only set the folder paths. They will be reloaded during runtime.
     * TODO: Add a more efficient reloading of only the changes */
    ci->trustListFolder = UA_STRING_ALLOC(trustListFolder);
    ci->issuerListFolder = UA_STRING_ALLOC(issuerListFolder);
    ci->revocationListFolder = UA_STRING_ALLOC(revocationListFolder);
#ifdef UA_ENABLE_CERT_REJECTED_DIR
    ci->rejectedListFolder = UA_STRING_ALLOC(rejectedListFolder);
#endif

    cv->context = (void*)ci;
    cv->verifyCertificate = certificateVerification_verify;
    cv->clear = certificateVerification_clear;
    cv->verifyApplicationURI = certificateVerification_verifyApplicationURI;

    ret = reloadCertificates(cv, ci);

    return ret;
}

#endif

UA_StatusCode
UA_PKI_decryptPrivateKey(const UA_ByteString privateKey,
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

#endif /* UA_ENABLE_ENCRYPTION_MBEDTLS */
