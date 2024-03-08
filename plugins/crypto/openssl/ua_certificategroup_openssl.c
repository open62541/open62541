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

#include "securitypolicy_openssl_common.h"

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#include "ua_openssl_version_abstraction.h"
#include "libc_time.h"

#include <limits.h>

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

typedef struct {
    /*
     * If the folders are defined, we use them to reload the certificates during
     * runtime
     */

    UA_String             trustListFolder;
    UA_String             issuerListFolder;
    UA_String             revocationListFolder;
    /* Used with mbedTLS and UA_ENABLE_CERT_REJECTED_DIR option */
    UA_String             rejectedListFolder;

    STACK_OF(X509) *      skIssue;
    STACK_OF(X509) *      skTrusted;
    STACK_OF(X509_CRL) *  skCrls; /* Revocation list*/

    UA_CertificateGroup *certGroup;
} CertContext;

static UA_StatusCode
UA_CertContext_sk_Init (CertContext * context) {
    context->skTrusted = sk_X509_new_null();
    context->skIssue = sk_X509_new_null();
    context->skCrls = sk_X509_CRL_new_null();
    if (context->skTrusted == NULL || context->skIssue == NULL ||
        context->skCrls == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    return UA_STATUSCODE_GOOD;
}

static void
UA_CertContext_sk_free (CertContext * context) {
    sk_X509_pop_free (context->skTrusted, X509_free);
    sk_X509_pop_free (context->skIssue, X509_free);
    sk_X509_CRL_pop_free (context->skCrls, X509_CRL_free);
}

static UA_StatusCode
UA_CertContext_Init (CertContext * context, UA_CertificateGroup *certGroup) {
    (void) memset (context, 0, sizeof (CertContext));
    UA_ByteString_init (&context->trustListFolder);
    UA_ByteString_init (&context->issuerListFolder);
    UA_ByteString_init (&context->revocationListFolder);
    UA_ByteString_init (&context->rejectedListFolder);

    context->certGroup = certGroup;

    return UA_CertContext_sk_Init (context);
}

static void
UA_CertificateGroup_clear (UA_CertificateGroup *certGroup) {
    if (certGroup == NULL) {
        return;
    }
    CertContext * context = (CertContext *) certGroup->context;
    if (context == NULL) {
        return;
    }
    UA_ByteString_clear (&context->trustListFolder);
    UA_ByteString_clear (&context->issuerListFolder);
    UA_ByteString_clear (&context->revocationListFolder);
    UA_ByteString_clear (&context->rejectedListFolder);

    UA_CertContext_sk_free (context);
    context->certGroup = NULL;
    UA_free (context);

    certGroup->context = NULL;

    return;
}

static UA_StatusCode
UA_skTrusted_Cert2X509 (const UA_ByteString *   certificateTrustList,
                        size_t                  certificateTrustListSize,
                        CertContext *           ctx) {
    size_t                i;

    for (i = 0; i < certificateTrustListSize; i++) {
        X509 * x509 = UA_OpenSSL_LoadCertificate(&certificateTrustList[i]);

        if (x509 == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        sk_X509_push (ctx->skTrusted, x509);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_skIssuer_Cert2X509 (const UA_ByteString *   certificateIssuerList,
                       size_t                  certificateIssuerListSize,
                       CertContext *           ctx) {
    size_t                i;

    for (i = 0; i < certificateIssuerListSize; i++) {
        X509 * x509 = UA_OpenSSL_LoadCertificate(&certificateIssuerList[i]);

        if (x509 == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        sk_X509_push (ctx->skIssue, x509);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_skCrls_Cert2X509 (const UA_ByteString *   certificateRevocationList,
                     size_t                  certificateRevocationListSize,
                     CertContext *           ctx) {
    size_t                i;
    const unsigned char * pData;

    for (i = 0; i < certificateRevocationListSize; i++) {
        pData = certificateRevocationList[i].data;
        X509_CRL * crl = NULL;

        if (certificateRevocationList[i].length > 1 && pData[0] == 0x30 && pData[1] == 0x82) { // Magic number for DER encoded files
            crl = d2i_X509_CRL (NULL, &pData, (long) certificateRevocationList[i].length);
        } else {
            BIO* bio = NULL;
            bio = BIO_new_mem_buf((void *) certificateRevocationList[i].data,
                                  (int) certificateRevocationList[i].length);
            crl = PEM_read_bio_X509_CRL(bio, NULL, NULL, NULL);
            BIO_free(bio);
        }

        if (crl == NULL) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        sk_X509_CRL_push (ctx->skCrls, crl);
    }

    return UA_STATUSCODE_GOOD;
}

#ifdef __linux__
#include <dirent.h>

static int UA_Certificate_Filter_der_pem (const struct dirent * entry) {
    /* ignore hidden files */
    if (entry->d_name[0] == '.') return 0;

    /* check file extension */
    const char *pszFind = strrchr(entry->d_name, '.');
    if (pszFind == 0)
        return 0;
    pszFind++;
    if (strcmp (pszFind, "der") == 0 || strcmp (pszFind, "pem") == 0)
        return 1;

    return 0;
}

static int UA_Certificate_Filter_crl (const struct dirent * entry) {

    /* ignore hidden files */
    if (entry->d_name[0] == '.') return 0;

    /* check file extension */
    const char *pszFind = strrchr(entry->d_name, '.');
    if (pszFind == 0)
        return 0;
    pszFind++;
    if (strcmp (pszFind, "crl") == 0)
        return 1;

    return 0;
}

static UA_StatusCode
UA_BuildFullPath (const char * path,
                  const char * fileName,
                  size_t       fullPathBufferLength,
                  char *       fullPath) {
    size_t  pathLen = strlen (path);
    size_t  fileNameLen = strlen (fileName);
    if ((pathLen + fileNameLen + 2) > fullPathBufferLength) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    strcpy (fullPath, path);
    strcat (fullPath, "/");
    strcat (fullPath, fileName);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_loadCertFromFile (const char *     fileName,
                     UA_ByteString *  cert) {

    FILE * fp = fopen(fileName, "rb");

    if (fp == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    fseek(fp, 0, SEEK_END);
    cert->length = (size_t)  ftell(fp);
    if (UA_ByteString_allocBuffer (cert, cert->length) != UA_STATUSCODE_GOOD) {
        fclose (fp);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    fseek(fp, 0, SEEK_SET);
    size_t readLen = fread (cert->data, 1, cert->length, fp);
    if (readLen != cert->length) {
        UA_ByteString_clear (cert);
        cert->length = 0;
        fclose (fp);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    fclose (fp);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReloadCertFromFolder (CertContext * ctx) {
    UA_StatusCode    ret;
    struct dirent ** dirlist = NULL;
    int              i;
    int              numCertificates;
    char             certFile[PATH_MAX];
    UA_ByteString    strCert;
    char             folderPath[PATH_MAX];

    UA_ByteString_init (&strCert);

    if (ctx->trustListFolder.length > 0) {
        UA_LOG_INFO(ctx->certGroup->logging, UA_LOGCATEGORY_SERVER, "Reloading the trust-list");

        sk_X509_pop_free (ctx->skTrusted, X509_free);
        ctx->skTrusted = sk_X509_new_null();
        if (ctx->skTrusted == NULL) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        (void) memcpy (folderPath, ctx->trustListFolder.data,
                       ctx->trustListFolder.length);
        folderPath[ctx->trustListFolder.length] = 0;
        numCertificates = scandir(folderPath, &dirlist,
                                  UA_Certificate_Filter_der_pem,
                                  alphasort);
        for (i = 0; i < numCertificates; i++) {
            if (UA_BuildFullPath (folderPath, dirlist[i]->d_name,
                                  PATH_MAX, certFile) != UA_STATUSCODE_GOOD) {
                continue;
            }
            ret = UA_loadCertFromFile (certFile, &strCert);
            if (ret != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO(ctx->certGroup->logging, UA_LOGCATEGORY_SERVER,
                            "Failed to load the certificate file %s", certFile);
                continue;  /* continue or return ? */
            }
            if (UA_skTrusted_Cert2X509 (&strCert, 1, ctx) != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (ctx->certGroup->logging, UA_LOGCATEGORY_SERVER,
                            "Failed to decode the certificate file %s", certFile);
                UA_ByteString_clear (&strCert);
                continue;  /* continue or return ? */
            }
            UA_ByteString_clear (&strCert);
        }
    }

    if (ctx->issuerListFolder.length > 0) {
        UA_LOG_INFO(ctx->certGroup->logging, UA_LOGCATEGORY_SERVER, "Reloading the issuer-list");

        sk_X509_pop_free (ctx->skIssue, X509_free);
        ctx->skIssue = sk_X509_new_null();
        if (ctx->skIssue == NULL) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        memcpy (folderPath, ctx->issuerListFolder.data, ctx->issuerListFolder.length);
        folderPath[ctx->issuerListFolder.length] = 0;
        numCertificates = scandir(folderPath, &dirlist,
                                  UA_Certificate_Filter_der_pem,
                                  alphasort);
        for (i = 0; i < numCertificates; i++) {
            if (UA_BuildFullPath (folderPath, dirlist[i]->d_name,
                                  PATH_MAX, certFile) != UA_STATUSCODE_GOOD) {
                continue;
            }
            ret = UA_loadCertFromFile (certFile, &strCert);
            if (ret != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (ctx->certGroup->logging, UA_LOGCATEGORY_SERVER,
                            "Failed to load the certificate file %s", certFile);
                continue;  /* continue or return ? */
            }
            if (UA_skIssuer_Cert2X509 (&strCert, 1, ctx) != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (ctx->certGroup->logging, UA_LOGCATEGORY_SERVER,
                            "Failed to decode the certificate file %s", certFile);
                UA_ByteString_clear (&strCert);
                continue;  /* continue or return ? */
            }
            UA_ByteString_clear (&strCert);
        }
    }

    if (ctx->revocationListFolder.length > 0) {
        UA_LOG_INFO(ctx->certGroup->logging, UA_LOGCATEGORY_SERVER, "Reloading the revocation-list");

        sk_X509_CRL_pop_free (ctx->skCrls, X509_CRL_free);
        ctx->skCrls = sk_X509_CRL_new_null();
        if (ctx->skCrls == NULL) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        memcpy (folderPath, ctx->revocationListFolder.data, ctx->revocationListFolder.length);
        folderPath[ctx->revocationListFolder.length] = 0;
        numCertificates = scandir(folderPath, &dirlist,
                                  UA_Certificate_Filter_crl,
                                  alphasort);
        for (i = 0; i < numCertificates; i++) {
            if (UA_BuildFullPath (folderPath, dirlist[i]->d_name,
                                  PATH_MAX, certFile) != UA_STATUSCODE_GOOD) {
                continue;
            }
            ret = UA_loadCertFromFile (certFile, &strCert);
            if (ret != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (ctx->certGroup->logging, UA_LOGCATEGORY_SERVER,
                            "Failed to load the revocation file %s", certFile);
                continue;  /* continue or return ? */
            }
            if (UA_skCrls_Cert2X509 (&strCert, 1, ctx) != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (ctx->certGroup->logging, UA_LOGCATEGORY_SERVER,
                            "Failed to decode the revocation file %s", certFile);
                UA_ByteString_clear (&strCert);
                continue;  /* continue or return ? */
            }
            UA_ByteString_clear (&strCert);
        }
    }

    ret = UA_STATUSCODE_GOOD;
    return ret;
}

#endif  /* end of __linux__ */

static UA_StatusCode
UA_X509_Store_CTX_Error_To_UAError (int opensslErr) {
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
UA_CertificateGroup_Verify(UA_CertificateGroup *certGroup,
                           const UA_ByteString *certificate) {
    X509_STORE_CTX *storeCtx = NULL;
    X509_STORE *store = NULL;
    CertContext *ctx = NULL;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    if ((certGroup == NULL) || (certGroup->context == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    ctx = (CertContext *) certGroup->context;

    /* Parse the certificate */
    X509 *certificateX509 = UA_OpenSSL_LoadCertificate(certificate);
    if(!certificateX509) {
        ret = UA_STATUSCODE_BADCERTIFICATEINVALID;
        goto cleanup;
    }

    /* Reload PKI folder */
#ifdef __linux__
    ret = UA_ReloadCertFromFolder (ctx);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;
#endif

    /* Accept the certificate without verification of no trust and issuer list
     * are loaded */
    if(sk_X509_CRL_num(ctx->skCrls) == 0 &&
       sk_X509_num(ctx->skIssue) == 0 &&
       sk_X509_num(ctx->skTrusted) == 0) {
        UA_LOG_WARNING(certGroup->logging, UA_LOGCATEGORY_USERLAND,
                       "No certificate store configured. Accepting the certificate.");
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
                                          ctx->skIssue);
    if(opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
#if defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT < 0x10100000L
	(void) X509_STORE_CTX_trusted_stack (storeCtx, ctx->skTrusted);
#else
	(void) X509_STORE_CTX_set0_trusted_stack (storeCtx, ctx->skTrusted);
#endif

    /* Set crls to ctx */
    if (sk_X509_CRL_num (ctx->skCrls) > 0) {
        X509_STORE_CTX_set0_crls (storeCtx, ctx->skCrls);
    }

    /* Set flag to check if the certificate has an invalid signature */
    X509_STORE_CTX_set_flags (storeCtx, X509_V_FLAG_CHECK_SS_SIGNATURE);

    if (X509_check_issued(certificateX509,certificateX509) != X509_V_OK) {
        X509_STORE_CTX_set_flags (storeCtx, X509_V_FLAG_CRL_CHECK);
    }

    /* This condition will check whether the certificate is a User certificate or a CA certificate.
     * If the KU_KEY_CERT_SIGN and KU_CRL_SIGN of key_usage are set, then the certificate shall be
     * condidered as CA Certificate and cannot be used to establish a connection. Refer the test case
     * CTT/Security/Security Certificate Validation/029.js for more details */
     /** \todo Can the ca-parameter of X509_check_purpose can be used? */
    if(X509_check_purpose(certificateX509, X509_PURPOSE_CRL_SIGN, 0) && X509_check_ca(certificateX509)) {
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    }

    opensslRet = X509_verify_cert (storeCtx);
    if (opensslRet == 1) {
        ret = UA_STATUSCODE_GOOD;

        /* Check if the not trusted certificate has a CRL file. If there is no CRL file available for the corresponding
         * parent certificate then return status code UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN. Refer the test
         * case CTT/Security/Security Certificate Validation/002.js */
        if (X509_check_issued(certificateX509,certificateX509) != X509_V_OK) {
            /* Free X509_STORE_CTX and reuse it for certification verification */
            if (storeCtx != NULL) {
               X509_STORE_CTX_free(storeCtx);
            }

            /* Initialised X509_STORE_CTX sructure*/
            storeCtx = X509_STORE_CTX_new();

            /* Sets up X509_STORE_CTX structure for a subsequent verification operation */
            X509_STORE_set_flags(store, 0);
            X509_STORE_CTX_init (storeCtx, store, certificateX509,ctx->skIssue);

            /* Set trust list to ctx */
            (void) X509_STORE_CTX_trusted_stack (storeCtx, ctx->skTrusted);

            /* Set crls to ctx */
            X509_STORE_CTX_set0_crls (storeCtx, ctx->skCrls);

            /* Set flags for CRL check */
            X509_STORE_CTX_set_flags (storeCtx, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);

            opensslRet = X509_verify_cert (storeCtx);
            if (opensslRet != 1) {
                opensslRet = X509_STORE_CTX_get_error (storeCtx);
                if (opensslRet == X509_V_ERR_UNABLE_TO_GET_CRL) {
                    ret = UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN;
                }
            }
        }
    }
    else {
        opensslRet = X509_STORE_CTX_get_error (storeCtx);

        /* Check the issued certificate of a CA that is not trusted but available */
        if(opensslRet == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN){
            int                     trusted_cert_len = sk_X509_num(ctx->skTrusted);
            int                     cmpVal;
            X509                    *trusted_cert;
            const ASN1_OCTET_STRING *trusted_cert_keyid;
            const ASN1_OCTET_STRING *remote_cert_keyid;

            for (int i = 0; i < trusted_cert_len; i++) {
                trusted_cert = sk_X509_value(ctx->skTrusted, i);

                /* Fetch the Subject key identifier of the certificate in trust list */
                trusted_cert_keyid = X509_get0_subject_key_id(trusted_cert);

                /* Fetch the Subject key identifier of the remote certificate */
                remote_cert_keyid = X509_get0_subject_key_id(certificateX509);

                /* Check remote certificate is present in the trust list */
                cmpVal = ASN1_OCTET_STRING_cmp(trusted_cert_keyid, remote_cert_keyid);
                if (cmpVal == 0){
                    ret = UA_STATUSCODE_GOOD;
                    goto cleanup;
                }
            }
        }

        /* Return expected OPCUA error code */
        ret = UA_X509_Store_CTX_Error_To_UAError (opensslRet);
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

/* main entry */

UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateGroup *certGroup,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize) {
    UA_StatusCode ret;

    if (certGroup == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (certGroup->logging == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clear if the plugin is already initialized */
    if(certGroup->clear)
        certGroup->clear(certGroup);

    CertContext * context = (CertContext *) UA_malloc (sizeof (CertContext));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    ret = UA_CertContext_Init (context, certGroup);
    if (ret != UA_STATUSCODE_GOOD) {
        UA_free(context);
        return ret;
    }

    certGroup->context = context;
    certGroup->verifyCertificate = UA_CertificateGroup_Verify;
    certGroup->clear = UA_CertificateGroup_clear;
    certGroup->getTrustList = NULL;
    certGroup->setTrustList = NULL;
    certGroup->addToTrustList = NULL;
    certGroup->removeFromTrustList = NULL;

    if (certificateTrustListSize > 0) {
        if (UA_skTrusted_Cert2X509 (certificateTrustList, certificateTrustListSize,
                                    context) != UA_STATUSCODE_GOOD) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    if (certificateIssuerListSize > 0) {
        if (UA_skIssuer_Cert2X509 (certificateIssuerList, certificateIssuerListSize,
                                  context) != UA_STATUSCODE_GOOD) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    if (certificateRevocationListSize > 0) {
        if (UA_skCrls_Cert2X509 (certificateRevocationList, certificateRevocationListSize,
                                  context) != UA_STATUSCODE_GOOD) {
            ret = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    return UA_STATUSCODE_GOOD;

errout:
    UA_CertificateGroup_clear(certGroup);
    return ret;
}

#ifdef __linux__ /* Linux only so far */
UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateGroup *certGroup,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder) {
    UA_StatusCode ret;
    if (certGroup == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (certGroup->logging == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clear if the plugin is already initialized */
    if(certGroup->clear)
        certGroup->clear(certGroup);

    CertContext * context = (CertContext *) UA_malloc (sizeof (CertContext));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    ret = UA_CertContext_Init (context, certGroup);
    if (ret != UA_STATUSCODE_GOOD) {
        UA_free(context);
        return ret;
    }

    certGroup->context = context;
    certGroup->verifyCertificate = UA_CertificateGroup_Verify;
    certGroup->clear = UA_CertificateGroup_clear;
    certGroup->getTrustList = NULL;
    certGroup->setTrustList = NULL;
    certGroup->addToTrustList = NULL;
    certGroup->removeFromTrustList = NULL;

    /* Only set the folder paths. They will be reloaded during runtime. */
    context->trustListFolder = UA_STRING_ALLOC(trustListFolder);
    context->issuerListFolder = UA_STRING_ALLOC(issuerListFolder);
    context->revocationListFolder = UA_STRING_ALLOC(revocationListFolder);

    return UA_STATUSCODE_GOOD;
}
#endif

static int
privateKeyPasswordCallback(char *buf, int size, int rwflag, void *userdata) {
    (void) rwflag;
    UA_ByteString *pw = (UA_ByteString*)userdata;
    if(pw->length <= (size_t)size)
        memcpy(buf, pw->data, pw->length);
    return (int)pw->length;
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

#endif  /* end of defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) */
