/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 */

/*
modification history
--------------------
21feb20,lan  written
*/

#include <open62541/server_config.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>

typedef X509 *  PX509;
typedef X509_CRL *  PX509_CRL;
typedef struct {
    /* 
     * If the folders are defined, we use them to reload the certificates during
     * runtime 
     */

    UA_String           trustListFolder;
    UA_String           issuerListFolder;
    UA_String           revocationListFolder;
    PX509 *             trustList; /* trust cert */
    size_t              trustListSize; /* size of X509*  */
    PX509 *             issueList; /* trust cert */
    size_t              issueListSize; /* size of X509*  */
    PX509_CRL *         revocationList;
    size_t              revocationListSize; /* size of X509_CRL*  */
} CertContext;

static UA_StatusCode 
UA_CertContext_Init (CertContext * context) {
    (void) memset (context, 0, sizeof (CertContext));
    UA_ByteString_init (&context->trustListFolder);
    UA_ByteString_init (&context->issuerListFolder);
    UA_ByteString_init (&context->revocationListFolder);
    return UA_STATUSCODE_GOOD;
}

static void 
UA_CertList_clear (PX509 *  lst,
                   size_t * lstLen) {
    size_t  i;                       
    for (i = 0; i < *lstLen; i++) {
        X509_free (lst[i]);
    }
    *lstLen = 0;
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
    UA_ByteString_deleteMembers (&context->trustListFolder);
    UA_ByteString_deleteMembers (&context->issuerListFolder);
    UA_ByteString_deleteMembers (&context->revocationListFolder);

    if (context->trustList != NULL) {
        UA_CertList_clear (context->trustList, &context->trustListSize);
        UA_free (context->trustList);
    }
    if (context->issueList != NULL){
        UA_CertList_clear (context->issueList, &context->issueListSize);
        UA_free (context->issueList);        
    }         
    if (context->revocationList != NULL) {
        size_t  i;
        for (i = 0; i < context->revocationListSize; i++) {
            X509_CRL_free (context->revocationList[i]);
        }
        UA_free (context->revocationList);
    }

    return;
}

static UA_StatusCode
UA_CertificateVerification_Verify (void *                verificationContext,
                                   const UA_ByteString * certificate) {
    STACK_OF(X509)*       skissue;
    STACK_OF(X509)*       sktrusted;
    STACK_OF(X509_CRL) *  skcrls;
    X509_STORE_CTX*       storeCtx;
    X509_STORE*           store;
    CertContext *         ctx;
    UA_StatusCode         ret;
    size_t                i;
    int                   opensslRet;
    const unsigned char * pData; 
    X509 *                certificateX509 = NULL;

    if (verificationContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    ctx = (CertContext *) verificationContext;

    sktrusted = sk_X509_new_null();
    skissue = sk_X509_new_null();
    skcrls = sk_X509_CRL_new_null();    
    store = X509_STORE_new();
    storeCtx = X509_STORE_CTX_new();
    
    if (sktrusted == NULL || skissue == NULL || skcrls == NULL ||
        store == NULL || storeCtx == NULL) {
        ret = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    
    for (i = 0; i < ctx->trustListSize; i++) {
        (void) sk_X509_push (sktrusted, ctx->trustList[i]);
    }
    for (i = 0; i < ctx->issueListSize; i++) {
        (void) sk_X509_push (skissue, ctx->issueList[i]);
    }
    for (i = 0; i < ctx->revocationListSize; i++) {
        (void) sk_X509_CRL_push (skcrls, ctx->revocationList[i]);
    }

    pData = certificate->data;
    certificateX509 = d2i_X509 (NULL, &pData, (long) certificate->length);
    if (certificateX509 == NULL) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    } 

    opensslRet = X509_STORE_CTX_init (storeCtx, store, certificateX509, skissue);
    if (opensslRet != 1) {
        ret = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }
    (void) X509_STORE_CTX_trusted_stack (storeCtx, sktrusted);

    /* Set crls to ctx */
    if (sk_X509_CRL_num (skcrls) > 0) {
        X509_STORE_CTX_set0_crls (storeCtx, skcrls);
        X509_STORE_CTX_set_flags (storeCtx, X509_V_FLAG_CRL_CHECK);
    }

    opensslRet = X509_verify_cert (storeCtx);
    if (opensslRet == 1) {
        ret = UA_STATUSCODE_GOOD;
    }
    else {
        ret = UA_STATUSCODE_BADINTERNALERROR;
    }
cleanup:
    if (sktrusted != NULL) {
        sk_X509_free (sktrusted);
    }
    if (skissue != NULL) {
        sk_X509_free (skissue);
    }    
    if (skcrls != NULL) {
        sk_X509_CRL_free (skcrls);
    }  
    if (store != NULL) {
        X509_STORE_free (store);
    }
    if (storeCtx != NULL) {
        X509_STORE_CTX_free (storeCtx);
    }
    if (certificateX509 != NULL) {
        X509_free (certificateX509);
    }
    return ret;                                       
}

static UA_StatusCode
UA_VerifyCertificateAllowAll (void *                verificationContext,
                              const UA_ByteString * certificate) {
    return UA_STATUSCODE_GOOD;  
}

static UA_StatusCode 
UA_CertificateVerification_VerifyApplicationURI (void *                verificationContext,
                                                 const UA_ByteString * certificate,
                                                 const UA_String *     applicationURI) {
    const unsigned char * pData; 
    X509 *                certificateX509;
    UA_String             subjectURI;
    GENERAL_NAMES *       pNames;
    int                   i;
    UA_StatusCode         ret;

    pData = certificate->data;
    certificateX509 = d2i_X509 (NULL, &pData, (long) certificate->length);
    if (certificateX509 == NULL) {
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    } 

    pNames = (GENERAL_NAMES *) X509_get_ext_d2i(certificateX509, NID_subject_alt_name, 
                                                NULL, NULL);
    if (pNames == NULL) {
        X509_free (certificateX509);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
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

    ret = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    if (UA_Bstrstr (subjectURI.data, subjectURI.length, 
                    applicationURI->data, applicationURI->length) != NULL) {
        ret = UA_STATUSCODE_GOOD;
    }

    X509_free (certificateX509);
    sk_GENERAL_NAME_pop_free(pNames, GENERAL_NAME_free);
    UA_String_clear (&subjectURI);    
    return ret;
}

static UA_StatusCode
UA_Certificate2X509 (const UA_ByteString *  certList,
                     size_t                 certListLen,
                     PX509 *                x509List,
                     size_t *               x509ListLen) {
    size_t                i;        
    const unsigned char * pData;          

    *x509ListLen = 0;
    for (i = 0; i < certListLen; i++) {
        pData = certList[i].data;
        x509List[i] = d2i_X509 (NULL, &pData, (long) certList[i].length);
        if (x509List[i] == NULL) {
            UA_CertList_clear (x509List, x509ListLen);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        (*x509ListLen)++;
    }

    return UA_STATUSCODE_GOOD;
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
     size_t       i;   

    if (cv == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }   

    CertContext * context = (CertContext *) UA_malloc (sizeof (CertContext));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_CertContext_Init (context);

    cv->verifyApplicationURI = UA_CertificateVerification_VerifyApplicationURI;
    cv->clear = UA_CertificateVerification_clear;
    cv->context = context;
    if (certificateTrustListSize > 0)
        cv->verifyCertificate = UA_CertificateVerification_Verify;
    else
        cv->verifyCertificate = UA_VerifyCertificateAllowAll;
    
    if (certificateTrustListSize > 0) {
        context->trustList = (PX509 *) UA_malloc (sizeof (PX509) * certificateTrustListSize);
        if (context->trustList == NULL) {
            ret = UA_STATUSCODE_BADOUTOFMEMORY;
            goto errout;
        }
        ret = UA_Certificate2X509 (certificateTrustList, certificateTrustListSize,
              context->trustList, &context->trustListSize);
        if (ret != UA_STATUSCODE_GOOD) {
            goto errout;
        }
    }

    if (certificateIssuerListSize > 0) {
        context->issueList = (PX509 *) UA_malloc (sizeof (PX509) * certificateIssuerListSize);
        if (context->issueList == NULL) {
            ret = UA_STATUSCODE_BADOUTOFMEMORY;
            goto errout;
        }
        ret = UA_Certificate2X509 (certificateIssuerList, certificateIssuerListSize,
              context->issueList, &context->issueListSize);
        if (ret != UA_STATUSCODE_GOOD) {
            goto errout;
        }
    }

    if (certificateRevocationListSize > 0){
        context->revocationList = (PX509_CRL *) UA_malloc (sizeof (PX509_CRL) * 
                              certificateRevocationListSize);
        if (context->revocationList == NULL) {
            ret = UA_STATUSCODE_BADOUTOFMEMORY;
            goto errout;
        } 
        const unsigned char * pData; 
        context->revocationListSize = 0;
        for (i = 0; i < certificateRevocationListSize; i++) {
            pData = certificateRevocationList[i].data;
            context->revocationList[i] = d2i_X509_CRL (NULL, &pData, 
                 (long) certificateRevocationList[i].length);
            if (context->revocationList[i] == NULL) {
                ret = UA_STATUSCODE_BADINTERNALERROR;
                goto errout;
                }
            context->revocationListSize++;
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
    return UA_STATUSCODE_BADNOTSUPPORTED; /* TODO: implement later */ 
}
#endif

#endif  /* end of UA_ENABLE_ENCRYPTION_OPENSSL */