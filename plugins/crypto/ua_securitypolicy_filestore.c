/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/securitypolicy_default.h>

#include "mp_printf.h"
#include "ua_filestore_common.h"

#ifdef UA_ENABLE_ENCRYPTION

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)

/* In-Memory security policy as a base */
typedef struct {
    UA_SecurityPolicy *innerPolicy;
    UA_String storePath;
} Filestore_Context;

static bool
checkCertificateInFilestore(char *path, const UA_ByteString newCertificate) {
    /* Check if the file is already stored in CertStore */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    bool isStored = false;
    UA_ByteString fileData = UA_BYTESTRING_NULL;
    UA_DIR *dir = UA_opendir(path);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct UA_DIRENT *dirent;
    while((dirent = UA_readdir(dir)) != NULL) {
        if(dirent->d_type != UA_DT_REG)
            continue;

        /* Get filename to load */
        char filename[UA_FILENAME_MAX];
        if(mp_snprintf(filename, UA_FILENAME_MAX, "%s/%s", path, dirent->d_name) < 0)
            return false;

        /* Load data from file */
        if(fileData.length > 0)
            UA_ByteString_clear(&fileData);
        retval = readFileToByteString(filename, &fileData);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;

        /* check if new certificate is already stored */
        if(UA_ByteString_equal(&newCertificate, &fileData)) {
            isStored = true;
            goto cleanup;
        }
    }

cleanup:
    if(fileData.length > 0)
        UA_ByteString_clear(&fileData);
    UA_closedir(dir);

    return isStored;
}

static UA_StatusCode
createCertName(const UA_ByteString *certificate, char *fileNameBuf, size_t fileNameLen) {
    /* Check parameter */
    if(certificate == NULL || fileNameBuf == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_String thumbprint = UA_STRING_NULL;
    thumbprint.length = 40;
    thumbprint.data = (UA_Byte*)UA_calloc(thumbprint.length, sizeof(UA_Byte));

    UA_String subjectName = UA_STRING_NULL;

    UA_CertificateUtils_getThumbprint((UA_ByteString*)(uintptr_t)certificate, &thumbprint);
    UA_CertificateUtils_getSubjectName((UA_ByteString*)(uintptr_t)certificate, &subjectName);

    if((thumbprint.length + subjectName.length + 2) > fileNameLen) {
        UA_String_clear(&thumbprint);
        UA_String_clear(&subjectName);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    char *thumbprintBuffer = (char*)UA_calloc(1, thumbprint.length + 1);
    char *subjectNameBuffer = (char*)UA_calloc(1, subjectName.length + 1);

    memcpy(thumbprintBuffer, thumbprint.data, thumbprint.length);
    memcpy(subjectNameBuffer, subjectName.data, subjectName.length);

    char *subName = NULL;
    char *substring = "CN=";
    char *ptr = strstr(subjectNameBuffer, substring);

    if(ptr != NULL) {
        subName = ptr + 3;
    } else {
        subName = subjectNameBuffer;
    }

    if(mp_snprintf(fileNameBuf, fileNameLen, "%s[%s]", subName, thumbprintBuffer) < 0)
        retval = UA_STATUSCODE_BADINTERNALERROR;

    UA_String_clear(&thumbprint);
    UA_String_clear(&subjectName);
    UA_free(thumbprintBuffer);
    UA_free(subjectNameBuffer);

    return retval;
}


static UA_StatusCode
writeCertificateAndPrivateKeyToFilestore(const UA_String storePath,
                                         const UA_ByteString newCertificate,
                                         const UA_ByteString newPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Create the paths to the certificates and private key folders */
    char ownCertPathDir[UA_PATH_MAX];
    if(mp_snprintf(ownCertPathDir, UA_PATH_MAX, "%.*s%s", (int)storePath.length,
                   (char *)storePath.data, "/ApplCerts/own/certs") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    char ownKeyPathDir[UA_PATH_MAX];
    if(mp_snprintf(ownKeyPathDir, UA_PATH_MAX, "%.*s%s", (int)storePath.length,
                   (char *)storePath.data, "/ApplCerts/own/private") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check if certificate is already stored */
    if(checkCertificateInFilestore(ownCertPathDir, newCertificate))
        return UA_STATUSCODE_GOOD;

    /* Create filename for new certificate */
    char newFilename[UA_PATH_MAX];
    retval = createCertName(&newCertificate, newFilename, UA_PATH_MAX);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    char newCertFilename[UA_PATH_MAX];
    if(mp_snprintf(newCertFilename, UA_PATH_MAX, "%s/%s%s",
                   ownCertPathDir, newFilename, ".der") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    char newKeyFilename[UA_PATH_MAX];
    if(mp_snprintf(newKeyFilename, UA_PATH_MAX, "%s/%s%s",
                   ownKeyPathDir, newFilename, ".key") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval = writeByteStringToFile(newCertFilename, &newCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Write new private key only if it is set */
    if(newPrivateKey.length > 0)
        return writeByteStringToFile(newKeyFilename, &newPrivateKey);
    else
        return UA_STATUSCODE_GOOD;
}

/************************/
/* Asymmetric Signature */
/************************/

static UA_StatusCode
asym_verify_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                      const UA_ByteString *message, const UA_ByteString *signature) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymSignatureAlgorithm.verify(isp, channelContext, message, signature);
}

static UA_StatusCode
asym_sign_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                    const UA_ByteString *message, UA_ByteString *signature) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymSignatureAlgorithm.sign(isp, channelContext, message, signature);
}

static size_t
asym_getLocalSignatureSize_filestore(const UA_SecurityPolicy *sp,
                                     const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymSignatureAlgorithm.getLocalSignatureSize(isp, channelContext);
}

static size_t
asym_getRemoteSignatureSize_filestore(const UA_SecurityPolicy *sp,
                                     const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymSignatureAlgorithm.getRemoteSignatureSize(isp, channelContext);
}

static size_t
asymSig_getLocalKeyLength_filestore(const UA_SecurityPolicy *sp,
                                    const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymSignatureAlgorithm.getLocalKeyLength(isp, channelContext);
}

static size_t
asymSig_getRemoteKeyLength_filestore(const UA_SecurityPolicy *sp,
                                    const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymSignatureAlgorithm.getRemoteKeyLength(isp, channelContext);
}

/*************************/
/* Asymmetric Encryption */
/*************************/

static UA_StatusCode
asym_encrypt_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                       UA_ByteString *data) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymEncryptionAlgorithm.encrypt(isp, channelContext, data);
}

static UA_StatusCode
asym_decrypt_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                       UA_ByteString *data) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymEncryptionAlgorithm.decrypt(isp, channelContext, data);
}

static size_t
asym_getLocalKeyLength_filestore(const UA_SecurityPolicy *sp,
                                 const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymEncryptionAlgorithm.getLocalKeyLength(isp, channelContext);
}

static size_t
asym_getRemoteKeyLength_filestore(const UA_SecurityPolicy *sp,
                                  const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymEncryptionAlgorithm.getRemoteKeyLength(isp, channelContext);
}

static size_t
asym_getRemotePlainTextBlockSize_filestore(const UA_SecurityPolicy *sp,
                                           const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymEncryptionAlgorithm.getRemotePlainTextBlockSize(isp, channelContext);
}

static size_t
asym_getRemoteBlockSize_filestore(const UA_SecurityPolicy *sp,
                                  const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->asymEncryptionAlgorithm.getRemoteBlockSize(isp, channelContext);
}

/***********************/
/* Symmetric Signature */
/***********************/

static UA_StatusCode
sym_verify_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                     const UA_ByteString *message, const UA_ByteString *signature) {
    Filestore_Context *pc =
        (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symSignatureAlgorithm.verify(isp, channelContext, message, signature);
}

static UA_StatusCode
sym_sign_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                   const UA_ByteString *message, UA_ByteString *signature) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symSignatureAlgorithm.sign(isp, channelContext, message, signature);
}

static size_t
sym_getLocalSignatureSize_filestore(const UA_SecurityPolicy *sp,
                                    const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symSignatureAlgorithm.getLocalSignatureSize(isp, channelContext);
}

static size_t
sym_getRemoteSignatureSize_filestore(const UA_SecurityPolicy *sp,
                                     const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symSignatureAlgorithm.getRemoteSignatureSize(isp, channelContext);
}

static size_t
symSig_getLocalKeyLength_filestore(const UA_SecurityPolicy *sp,
                                    const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symSignatureAlgorithm.getLocalKeyLength(isp, channelContext);
}

static size_t
symSig_getRemoteKeyLength_filestore(const UA_SecurityPolicy *sp,
                                    const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symSignatureAlgorithm.getRemoteKeyLength(isp, channelContext);
}

/*************************/
/* Asymmetric Encryption */
/*************************/

static UA_StatusCode
sym_encrypt_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                      UA_ByteString *data) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symEncryptionAlgorithm.encrypt(isp, channelContext, data);
}

static UA_StatusCode
sym_decrypt_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                      UA_ByteString *data) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symEncryptionAlgorithm.decrypt(isp, channelContext, data);
}

static size_t
sym_getLocalKeyLength_filestore(const UA_SecurityPolicy *sp,
                                const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symEncryptionAlgorithm.getLocalKeyLength(isp, channelContext);
}

static size_t
sym_getRemoteKeyLength_filestore(const UA_SecurityPolicy *sp,
                                 const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symEncryptionAlgorithm.getRemoteKeyLength(isp, channelContext);
}

static size_t
sym_getRemotePlainTextBlockSize_filestore(const UA_SecurityPolicy *sp,
                                          const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symEncryptionAlgorithm.getRemotePlainTextBlockSize(isp, channelContext);
}

static size_t
sym_getRemoteBlockSize_filestore(const UA_SecurityPolicy *sp,
                                 const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->symEncryptionAlgorithm.getRemoteBlockSize(isp, channelContext);
}

/*************************/
/* Certificate Signature */
/*************************/

static UA_StatusCode
cert_verify_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                      const UA_ByteString *message, const UA_ByteString *signature) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->certSignatureAlgorithm.verify(isp, channelContext, message, signature);
}

static UA_StatusCode
cert_sign_filestore(const UA_SecurityPolicy *sp, void *channelContext,
                    const UA_ByteString *message, UA_ByteString *signature) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->certSignatureAlgorithm.sign(isp, channelContext, message, signature);
}

static size_t
cert_getLocalSignatureSize_filestore(const UA_SecurityPolicy *sp,
                                     const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->certSignatureAlgorithm.getLocalSignatureSize(isp, channelContext);
}

static size_t
cert_getRemoteSignatureSize_filestore(const UA_SecurityPolicy *sp,
                                     const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->certSignatureAlgorithm.getRemoteSignatureSize(isp, channelContext);
}


static size_t
cert_getLocalKeyLength_filestore(const UA_SecurityPolicy *sp,
                                 const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->certSignatureAlgorithm.getLocalKeyLength(isp, channelContext);
}

static size_t
cert_getRemoteKeyLength_filestore(const UA_SecurityPolicy *sp,
                                  const void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->certSignatureAlgorithm.getRemoteKeyLength(isp, channelContext);
}

/********************/
/* Direct Callbacks */
/********************/

static UA_StatusCode
newChannelContext_filestore(const UA_SecurityPolicy *sp,
                            const UA_ByteString *remoteCertificate,
                            void **pp_contextData) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->newChannelContext(isp, remoteCertificate, pp_contextData);
}

static void
deleteChannelContext_filestore(const UA_SecurityPolicy *sp, void *channelContext) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    isp->deleteChannelContext(isp, channelContext);
}

static UA_StatusCode
setLocalSymSigningKey_filestore(const UA_SecurityPolicy *sp,
                                   void *channelContext, const UA_ByteString *key) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->setLocalSymSigningKey(isp, channelContext, key);
}

static UA_StatusCode
setLocalSymEncryptingKey_filestore(const UA_SecurityPolicy *sp,
                                      void *channelContext, const UA_ByteString *key) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->setLocalSymEncryptingKey(isp, channelContext, key);
}

static UA_StatusCode
setLocalSymIv_filestore(const UA_SecurityPolicy *sp,
                           void *channelContext, const UA_ByteString *iv) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->setLocalSymIv(isp, channelContext, iv);
}

static UA_StatusCode
setRemoteSymSigningKey_filestore(const UA_SecurityPolicy *sp,
                                    void *channelContext, const UA_ByteString *key) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->setRemoteSymSigningKey(isp, channelContext, key);
}

static UA_StatusCode
setRemoteSymEncryptingKey_filestore(const UA_SecurityPolicy *sp,
                                       void *channelContext, const UA_ByteString *key) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->setRemoteSymEncryptingKey(isp, channelContext, key);
}

static UA_StatusCode
setRemoteSymIv_filestore(const UA_SecurityPolicy *sp,
                            void *channelContext, const UA_ByteString *iv) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->setRemoteSymIv(isp, channelContext, iv);
}

static UA_StatusCode
generateKey_filestore(const UA_SecurityPolicy *sp,
                         void *channelContext, const UA_ByteString *secret,
                         const UA_ByteString *seed, UA_ByteString *out) {
    Filestore_Context *pc = (Filestore_Context *)sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->generateKey(isp, channelContext, secret, seed, out);
}

static UA_StatusCode
generateNonce_filestore(const UA_SecurityPolicy *sp,
                           void *channelContext, UA_ByteString *out) {
    Filestore_Context *pc = (Filestore_Context *)sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->generateNonce(isp, channelContext, out);
}

static UA_StatusCode
compareCertificate_filestore(const UA_SecurityPolicy *sp,
                                const void *channelContext,
                                const UA_ByteString *certificate) {
    Filestore_Context *pc = (Filestore_Context *)sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->compareCertificate(isp, channelContext, certificate);
}

static UA_StatusCode
updateCertificate_filestore(UA_SecurityPolicy *sp,
                            const UA_ByteString newCertificate,
                            const UA_ByteString newPrivateKey) {
    /* Temporarily save the old certificate file so that it can be removed from
     * CertStore */
    UA_ByteString localCertificateTmp;
    UA_ByteString_copy(&sp->localCertificate, &localCertificateTmp);

    Filestore_Context *pc =
        (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;

    UA_StatusCode retval = isp->updateCertificate(isp, newCertificate, newPrivateKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    sp->localCertificate = isp->localCertificate;

    retval = writeCertificateAndPrivateKeyToFilestore(pc->storePath,
                                                      newCertificate, newPrivateKey);
    UA_ByteString_clear(&localCertificateTmp);

    return retval;
}

static UA_StatusCode
createSigningRequest_filestore(UA_SecurityPolicy *sp,
                               const UA_String *subjectName,
                               const UA_ByteString *nonce,
                               const UA_KeyValueMap *params,
                               UA_ByteString *csr,
                               UA_ByteString *newPrivateKey) {
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->createSigningRequest(isp, subjectName, nonce, params, csr, newPrivateKey);
}

static UA_StatusCode
makeThumbprint_filestore(const UA_SecurityPolicy *securityPolicy,
                         const UA_ByteString *certificate,
                         UA_ByteString *thumbprint) {
    Filestore_Context *pc = (Filestore_Context *) securityPolicy->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->makeCertThumbprint(isp, certificate, thumbprint);
}

static UA_StatusCode
compareCertificateThumbprint_filestore(const UA_SecurityPolicy *securityPolicy,
                                       const UA_ByteString *certThumbprint) {
    Filestore_Context *pc = (Filestore_Context *) securityPolicy->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    return isp->compareCertThumbprint(isp, certThumbprint);
}

static void
clear_filestore(UA_SecurityPolicy *sp) {
    if(!sp)
        return;
    if(!sp->policyContext)
        return;

    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;

    pc->innerPolicy->clear(pc->innerPolicy);
    UA_String_clear(&pc->storePath);
    UA_free(pc->innerPolicy);
    UA_free(pc);
    sp->policyContext = NULL;
}

static UA_StatusCode
newContext_filestore(UA_SecurityPolicy *policy, const UA_String storePath) {
    if(!policy)
        return UA_STATUSCODE_BADINTERNALERROR;

    Filestore_Context *pc = (Filestore_Context *)
        UA_calloc(1, sizeof(Filestore_Context));
    if(!pc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_String_copy(&storePath, &pc->storePath);
    policy->policyContext = (void *)pc;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecurityPolicy_Filestore(UA_SecurityPolicy *sp,
                            UA_SecurityPolicy *innerPolicy,
                            const UA_String storePath) {
    if(!sp || !innerPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_SecurityPolicy *isp = innerPolicy;

    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = isp->logger;
    sp->policyUri = isp->policyUri;
    sp->certificateGroupId = isp->certificateGroupId;
    sp->certificateTypeId = isp->certificateTypeId;
    sp->securityLevel = isp->securityLevel;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = isp->asymSignatureAlgorithm.uri;
    asymSig->verify = asym_verify_filestore;
    asymSig->sign = asym_sign_filestore;
    asymSig->getLocalSignatureSize = asym_getLocalSignatureSize_filestore;
    asymSig->getRemoteSignatureSize = asym_getRemoteSignatureSize_filestore;
    asymSig->getLocalKeyLength = asymSig_getLocalKeyLength_filestore;
    asymSig->getRemoteKeyLength = asymSig_getRemoteKeyLength_filestore;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = isp->asymEncryptionAlgorithm.uri;
    asymEnc->encrypt = asym_encrypt_filestore; 
    asymEnc->decrypt = asym_decrypt_filestore;
    asymEnc->getLocalKeyLength = asym_getLocalKeyLength_filestore;
    asymEnc->getRemoteKeyLength = asym_getRemoteKeyLength_filestore;
    asymEnc->getRemoteBlockSize = asym_getRemoteBlockSize_filestore;
    asymEnc->getRemotePlainTextBlockSize = asym_getRemotePlainTextBlockSize_filestore;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = isp->symSignatureAlgorithm.uri;
    symSig->verify = sym_verify_filestore;
    symSig->sign = sym_sign_filestore;
    symSig->getLocalSignatureSize = sym_getLocalSignatureSize_filestore;
    symSig->getRemoteSignatureSize = sym_getRemoteSignatureSize_filestore;
    symSig->getLocalKeyLength = symSig_getLocalKeyLength_filestore;
    symSig->getRemoteKeyLength = symSig_getRemoteKeyLength_filestore;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = isp->symEncryptionAlgorithm.uri;
    symEnc->encrypt = sym_encrypt_filestore;
    symEnc->decrypt = sym_decrypt_filestore;
    symEnc->getLocalKeyLength = sym_getLocalKeyLength_filestore;
    symEnc->getRemoteKeyLength = sym_getRemoteKeyLength_filestore;
    symEnc->getRemoteBlockSize = sym_getRemoteBlockSize_filestore;
    symEnc->getRemotePlainTextBlockSize = sym_getRemotePlainTextBlockSize_filestore;

    /* Certificate Signing */
    UA_SecurityPolicySignatureAlgorithm *certSig = &sp->certSignatureAlgorithm;;
    certSig->uri = isp->certSignatureAlgorithm.uri;
    certSig->verify = cert_verify_filestore;
    certSig->sign = cert_sign_filestore;
    certSig->getLocalSignatureSize = cert_getLocalSignatureSize_filestore;
    certSig->getRemoteSignatureSize = cert_getRemoteSignatureSize_filestore;
    certSig->getLocalKeyLength = cert_getLocalKeyLength_filestore;
    certSig->getRemoteKeyLength = cert_getRemoteKeyLength_filestore;

    /* Direct Method Pointers */
    sp->newChannelContext = newChannelContext_filestore;
    sp->deleteChannelContext = deleteChannelContext_filestore;
    sp->setLocalSymEncryptingKey = setLocalSymEncryptingKey_filestore;
    sp->setLocalSymSigningKey = setLocalSymSigningKey_filestore;
    sp->setLocalSymIv = setLocalSymIv_filestore;
    sp->setRemoteSymEncryptingKey = setRemoteSymEncryptingKey_filestore;
    sp->setRemoteSymSigningKey = setRemoteSymSigningKey_filestore;
    sp->setRemoteSymIv = setRemoteSymIv_filestore;
    sp->compareCertificate = compareCertificate_filestore;
    sp->generateKey = generateKey_filestore;
    sp->generateNonce = generateNonce_filestore;
    sp->nonceLength = isp->nonceLength;
    sp->makeCertThumbprint = makeThumbprint_filestore;
    sp->compareCertThumbprint = compareCertificateThumbprint_filestore;
    sp->updateCertificate = updateCertificate_filestore;
    sp->createSigningRequest = createSigningRequest_filestore;
    sp->clear = clear_filestore;

    /* Set the state / context */
    sp->localCertificate = isp->localCertificate;
    UA_StatusCode retval = newContext_filestore(sp, storePath);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the inner policy */
    Filestore_Context *pc = (Filestore_Context *) sp->policyContext;
    pc->innerPolicy = innerPolicy;

    return UA_STATUSCODE_GOOD;
}

#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

#endif /* UA_ENABLE_ENCRYPTION */
