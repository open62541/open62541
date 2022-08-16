/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2019 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#include <open62541/util.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>
#include <mbedtls/version.h>

#define REMOTECERTIFICATETRUSTED 1
#define ISSUERKNOWN              2
#define DUALPARENT               3
#define PARENTFOUND              4

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

    if(internalErrorFlag) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

#endif

static UA_StatusCode
certificateVerification_allow(void *verificationContext,
                              const UA_ByteString *certificate) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
certificateVerification_verify(void *verificationContext,
                               const UA_ByteString *certificate) {
    CertInfo *ci = (CertInfo*)verificationContext;
    if(!ci)
        return UA_STATUSCODE_BADINTERNALERROR;

#ifdef __linux__ /* Reload certificates if folder paths are specified */
    UA_StatusCode certFlag = reloadCertificates(ci);
    if(certFlag != UA_STATUSCODE_GOOD) {
        return certFlag;
    }
#endif

    if(ci->trustListFolder.length == 0 &&
       ci->issuerListFolder.length == 0 &&
       ci->revocationListFolder.length == 0 &&
       ci->certificateTrustList.raw.len == 0 &&
       ci->certificateIssuerList.raw.len == 0 &&
       ci->certificateRevocationList.raw.len == 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "PKI plugin unconfigured. Accepting the certificate.");
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

    /* Flag to check if the remote certificate is trusted or not */
    int TRUSTED = 0;

    /* Check if the remoteCertificate is present in the trustList while mbedErr value is not zero */
    if(mbedErr && !(flags & MBEDTLS_X509_BADCERT_EXPIRED) && !(flags & MBEDTLS_X509_BADCERT_FUTURE)) {
        for(tempCert = &ci->certificateTrustList; tempCert != NULL; tempCert = tempCert->next) {
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
                                                       &ci->certificateIssuerList,
                                                       &ci->certificateRevocationList,
                                                       &crtProfile, NULL, &flags, NULL, NULL);

        /* Check if the parent certificate has a CRL file available */
        if(!mbedErr) {
            /* Flag value to identify if that there is an intermediate CA present */
            int dualParent = 0;

            /* Identify the topmost parent certificate for the remoteCertificate */
            for(parentCert = &ci->certificateIssuerList; parentCert != NULL; parentCert = parentCert->next ) {
                if(memcmp(remoteCertificate.issuer_raw.p, parentCert->subject_raw.p, parentCert->subject_raw.len) == 0) {
                    for(parentCert_2 = &ci->certificateTrustList; parentCert_2 != NULL; parentCert_2 = parentCert_2->next) {
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
                tempCrl = &ci->certificateRevocationList;
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
        for(parentCert = &ci->certificateTrustList; parentCert != NULL; parentCert = parentCert->next) {
            if(memcmp(remoteCertificate.issuer_raw.p, parentCert->subject_raw.p, parentCert->subject_raw.len) == 0) {
                parentFound = PARENTFOUND;
                break;
            }

        }

        /* If the parent certificate is found traverse the revocationList and identify
         * if there is any CRL file that corresponds to the parentCertificate */
        if(parentFound == PARENTFOUND &&
            memcmp(remoteCertificate.issuer_raw.p, remoteCertificate.subject_raw.p, remoteCertificate.subject_raw.len) != 0) {
            tempCrl = &ci->certificateRevocationList;
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
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
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
    if(certificateTrustListSize > 0)
        cv->verifyCertificate = certificateVerification_verify;
    else
        cv->verifyCertificate = certificateVerification_allow;
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
#endif
