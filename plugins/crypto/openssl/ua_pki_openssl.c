/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH

 */

#include <open62541/util.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/log_stdout.h>

#include "securitypolicy_openssl_common.h"

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>

#include "ua_openssl_version_abstraction.h"

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

    STACK_OF(X509) *      skIssue;
    STACK_OF(X509) *      skTrusted;
    STACK_OF(X509_CRL) *  skCrls; /* Revocation list*/
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
UA_CertContext_Init (CertContext * context) {
    (void) memset (context, 0, sizeof (CertContext));
    UA_ByteString_init (&context->trustListFolder);
    UA_ByteString_init (&context->issuerListFolder);
    UA_ByteString_init (&context->revocationListFolder);
    return UA_CertContext_sk_Init (context);
}

static void
UA_CertificateVerification_clear (UA_CertificateVerification * cv) {
    if (cv == NULL) {
        return ;
    }
    CertContext * context = (CertContext *) cv->context;    
    if (context == NULL) {
        return;
    }
    UA_ByteString_clear (&context->trustListFolder);
    UA_ByteString_clear (&context->issuerListFolder);
    UA_ByteString_clear (&context->revocationListFolder);

    UA_CertContext_sk_free (context);
    UA_free (context);

    memset(cv, 0, sizeof(UA_CertificateVerification));
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
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Reloading the trust-list"); 

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
                UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to load the certificate file %s", certFile); 
                continue;  /* continue or return ? */
            }
            if (UA_skTrusted_Cert2X509 (&strCert, 1, ctx) != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to decode the certificate file %s", certFile);
                UA_ByteString_clear (&strCert);
                continue;  /* continue or return ? */
            }
            UA_ByteString_clear (&strCert);
        }
        for (i = 0; i < numCertificates; i++) {
            free(dirlist[i]);
        }
        free(dirlist);
    }

    if (ctx->issuerListFolder.length > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Reloading the issuer-list");    

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
                UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to load the certificate file %s", certFile); 
                continue;  /* continue or return ? */
            }
            if (UA_skIssuer_Cert2X509 (&strCert, 1, ctx) != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to decode the certificate file %s", certFile);                 
                UA_ByteString_clear (&strCert);
                continue;  /* continue or return ? */
            }
            UA_ByteString_clear (&strCert);
        }
        for (i = 0; i < numCertificates; i++) {
            free(dirlist[i]);
        }
        free(dirlist);
    }

    if (ctx->revocationListFolder.length > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Reloading the revocation-list");   

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
                UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to load the revocation file %s", certFile);                 
                continue;  /* continue or return ? */
            }
            if (UA_skCrls_Cert2X509 (&strCert, 1, ctx) != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                            "Failed to decode the revocation file %s", certFile);                                 
                UA_ByteString_clear (&strCert);
                continue;  /* continue or return ? */
            }
            UA_ByteString_clear (&strCert);
        }
        for (i = 0; i < numCertificates; i++) {
            free(dirlist[i]);
        }
        free(dirlist);
    }

    ret = UA_STATUSCODE_GOOD;
    return ret;
}

#endif  /* end of __linux__ */

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
openSSLFindNextIssuer(CertContext *ctx, STACK_OF(X509) *stack, X509 *x509, X509 *prev) {
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
        /* Switch from the stack that came with the cert to the issuer list and
         * then to the trust list. */
        if(stack == ctx->skTrusted)
            stack = NULL;
        else if(stack == ctx->skIssue)
            stack = ctx->skTrusted;
        else
            stack = ctx->skIssue;
    } while(stack);
    return NULL;
}

/* Is the certificate a CA? */
static UA_Boolean
openSSLCheckCA(X509 *cert) {
    uint32_t flags = X509_get_extension_flags(cert);
    /* The basic constraints must be set with the CA flag true */
    if(!(flags & EXFLAG_CA))
        return false;

    /* The Key Usage extension must be set */
    if(!(flags & EXFLAG_KUSAGE))
        return false;

    /* The Key Usage must include cert signing and CRL issuing */
    uint32_t usage = X509_get_key_usage(cert);
    if(!(usage & KU_KEY_CERT_SIGN) || !(usage & KU_CRL_SIGN))
        return false;

    return true;
}

static UA_StatusCode
openSSLCheckRevoked(CertContext *ctx, X509 *cert) {
    const ASN1_INTEGER *sn = X509_get0_serialNumber(cert);
    const X509_NAME *in = X509_get_issuer_name(cert);
    int size = sk_X509_CRL_num(ctx->skCrls);

    if(size == 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Zero revocation lists have been loaded. "
                       "This seems intentional - omitting the check.");
        return UA_STATUSCODE_GOOD;
    }

    /* Loop over the crl and match the Issuer Name */
    UA_StatusCode res = UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN;
    for(int i = 0; i < size; i++) {
        /* The crl contains a list of serial numbers from the same issuer */
        X509_CRL *crl = sk_X509_CRL_value(ctx->skCrls, i);
        if(X509_NAME_cmp(in, X509_CRL_get_issuer(crl)) != 0)
            continue;
        STACK_OF(X509_REVOKED) *rs = X509_CRL_get_REVOKED(crl);
        int rsize = sk_X509_REVOKED_num(rs);
        for(int j = 0; j < rsize; j++) {
            X509_REVOKED *r = sk_X509_REVOKED_value(rs, j);
            if(ASN1_INTEGER_cmp(sn, X509_REVOKED_get0_serialNumber(r)) == 0)
                return UA_STATUSCODE_BADCERTIFICATEREVOKED;
        }
        res = UA_STATUSCODE_GOOD; /* There was at least one crl that did not revoke (so far) */
    }
    return res;
}

#define UA_OPENSSL_MAX_CHAIN_LENGTH 10

static UA_StatusCode
openSSL_verifyChain(CertContext *ctx, STACK_OF(X509) *stack, X509 **old_issuers,
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
        if((depth > 0 || issuer != cert) && !openSSLCheckCA(issuer)) {
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
         * untrusted).
         *
         * Break here as we have reached the end of the chain. Omit the
         * Revocation Check for self-signed certificates. */
        if(cert == issuer || X509_cmp(cert, issuer) == 0) {
            ret = UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
            break;
        }

        /* Verification Step: Revocation Check */
        ret = openSSLCheckRevoked(ctx, cert);
        if(depth > 0) {
            if(ret == UA_STATUSCODE_BADCERTIFICATEREVOKED)
                ret = UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED;
            if(ret == UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN)
                ret = UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN;
        }
        if(ret != UA_STATUSCODE_GOOD)
            continue;

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
        for(int i = 0; i < sk_X509_num(ctx->skTrusted); i++) {
            if(X509_cmp(cert, sk_X509_value(ctx->skTrusted, i)) == 0)
                return UA_STATUSCODE_GOOD;
        }
    }

    return ret;
}

/* This follows Part 6, 6.1.3 Determining if a Certificate is trusted.
 * It defines a sequence of steps for certificate verification. */
static UA_StatusCode
UA_CertificateVerification_Verify(void *verificationContext,
                                  const UA_ByteString *certificate) {
    if(!verificationContext || !certificate)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    CertContext *ctx = (CertContext *)verificationContext;

#ifdef __linux__ 
    ret = UA_ReloadCertFromFolder(ctx);
    if(ret != UA_STATUSCODE_GOOD)
        return ret;
#endif

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
    if(openSSLCheckCA(leaf)) {
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
    ret = openSSL_verifyChain(ctx, stack, old_issuers, leaf, 0);
    sk_X509_pop_free(stack, X509_free);
    return ret;
}

static UA_StatusCode
UA_CertificateVerification_VerifyApplicationURI (void *                verificationContext,
                                                 const UA_ByteString * certificate,
                                                 const UA_String *     applicationURI) {
    (void) verificationContext;

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

    X509_free (certificateX509);
    sk_GENERAL_NAME_pop_free(pNames, GENERAL_NAME_free);
    UA_String_clear (&subjectURI);
    return ret;
}

/* main entry */

UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateVerification * cv,
                                     const UA_ByteString *        certificateTrustList,
                                     size_t                       certificateTrustListSize,
                                     const UA_ByteString *        certificateIssuerList,
                                     size_t                       certificateIssuerListSize,
                                     const UA_ByteString *        certificateRevocationList,
                                     size_t                       certificateRevocationListSize) {
    UA_StatusCode ret;

    if (cv == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    CertContext * context = (CertContext *) UA_malloc (sizeof (CertContext));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    ret = UA_CertContext_Init (context);
    if (ret != UA_STATUSCODE_GOOD) {
        return ret;
    }

    cv->verifyApplicationURI = UA_CertificateVerification_VerifyApplicationURI;
    cv->clear = UA_CertificateVerification_clear;
    cv->context = context;
    cv->verifyCertificate = UA_CertificateVerification_Verify;
    
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
    UA_CertificateVerification_clear (cv);
    return ret;
}

#ifdef __linux__ /* Linux only so far */
UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification * cv,
                                       const char *                 trustListFolder,
                                       const char *                 issuerListFolder,
                                       const char *                 revocationListFolder) {
    UA_StatusCode ret;
    if (cv == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }   

    CertContext * context = (CertContext *) UA_malloc (sizeof (CertContext));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    ret = UA_CertContext_Init (context);
    if (ret != UA_STATUSCODE_GOOD) {
        return ret;
    }

    cv->verifyApplicationURI = UA_CertificateVerification_VerifyApplicationURI;
    cv->clear = UA_CertificateVerification_clear;
    cv->context = context;
    cv->verifyCertificate = UA_CertificateVerification_Verify;

    /* Only set the folder paths. They will be reloaded during runtime. */

    context->trustListFolder = UA_STRING_ALLOC(trustListFolder);
    context->issuerListFolder = UA_STRING_ALLOC(issuerListFolder);
    context->revocationListFolder = UA_STRING_ALLOC(revocationListFolder);

    return UA_STATUSCODE_GOOD;
}
#endif

#endif  /* end of defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) */
