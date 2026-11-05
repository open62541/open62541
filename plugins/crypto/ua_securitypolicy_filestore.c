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

    /* The subject name is fully attacker-controlled (it comes from the
     * certificate presented to ServerConfiguration.UpdateCertificate) and
     * is used verbatim as part of a filename below. Replace path
     * separators and control characters so the subject cannot break out
     * of the certificate / private-key directories the filename is
     * written into. This is independent of how a given crypto backend
     * renders the subject string, e.g. mbedTLS preserves '/' in the CN
     * while OpenSSL escapes it. */
    for(char *p = subName; *p != '\0'; p++) {
        if(*p == '/' || *p == '\\' || (unsigned char)*p < 0x20)
            *p = '_';
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

/* Resolve the CertificateTypeId to its BrowseName for naming staged keys. */
static const struct {
    UA_UInt32 id;
    const char *name;
} stagedKeyCertificateTypes[] = {
    {UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE, "RsaMinApplicationCertificateType"},
    {UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE, "RsaSha256ApplicationCertificateType"},
    {UA_NS0ID_ECCNISTP256APPLICATIONCERTIFICATETYPE, "EccNistP256ApplicationCertificateType"},
    {UA_NS0ID_ECCNISTP384APPLICATIONCERTIFICATETYPE, "EccNistP384ApplicationCertificateType"},
    {UA_NS0ID_ECCBRAINPOOLP256R1APPLICATIONCERTIFICATETYPE,
     "EccBrainpoolP256r1ApplicationCertificateType"},
    {UA_NS0ID_ECCBRAINPOOLP384R1APPLICATIONCERTIFICATETYPE,
     "EccBrainpoolP384r1ApplicationCertificateType"},
    {UA_NS0ID_ECCCURVE25519APPLICATIONCERTIFICATETYPE, "EccCurve25519ApplicationCertificateType"},
    {UA_NS0ID_ECCCURVE448APPLICATIONCERTIFICATETYPE, "EccCurve448ApplicationCertificateType"}
};

static const char *
getStagedKeyCertificateTypeName(const UA_NodeId *certificateTypeId) {
    if(certificateTypeId->namespaceIndex != 0 ||
       certificateTypeId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return NULL;
    size_t typesSize =
        sizeof(stagedKeyCertificateTypes) / sizeof(stagedKeyCertificateTypes[0]);
    for(size_t i = 0; i < typesSize; i++) {
        if(stagedKeyCertificateTypes[i].id == certificateTypeId->identifier.numeric)
            return stagedKeyCertificateTypes[i].name;
    }
    return NULL;
}

static UA_StatusCode
createStagedKeyFilename(const UA_String storePath, const UA_NodeId *certificateTypeId,
                        char *path, size_t pathLen) {
    const char *typeName = getStagedKeyCertificateTypeName(certificateTypeId);
    if(!typeName)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    int len = mp_snprintf(path, pathLen, "%.*s/ApplCerts/own/private/staged_%s.key",
                          (int)storePath.length, (char *)storePath.data, typeName);
    if(len < 0 || (size_t)len >= pathLen)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeStagedPrivateKeyFromFilestore(const UA_String storePath,
                                    const UA_NodeId *certificateTypeId) {
    /* No key is staged for a CertificateType without a filename */
    if(!getStagedKeyCertificateTypeName(certificateTypeId))
        return UA_STATUSCODE_GOOD;

    char path[UA_PATH_MAX];
    UA_StatusCode retval =
        createStagedKeyFilename(storePath, certificateTypeId, path, UA_PATH_MAX);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* The file does not exist if no key is staged */
    if(UA_remove(path) != 0 && errno != ENOENT)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeStagedPrivateKeyToFilestore(const UA_String storePath,
                                 const UA_NodeId *certificateTypeId,
                                 const UA_ByteString stagedPrivateKey) {
    char path[UA_PATH_MAX];
    UA_StatusCode retval =
        createStagedKeyFilename(storePath, certificateTypeId, path, UA_PATH_MAX);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Do not leave a partially written key behind */
    retval = writeByteStringToFile(path, &stagedPrivateKey);
    if(retval != UA_STATUSCODE_GOOD)
        (void)UA_remove(path);
    return retval;
}

/* Read the staged key from the inner SecurityPolicy. */
static void
readStagedPrivateKeyFromPolicy(UA_SecurityPolicy *isp, UA_ByteString *stagedKey) {
    UA_ByteString_init(stagedKey);
    if(!isp->createSigningRequest)
        return;
    UA_ByteString csr = UA_BYTESTRING_NULL;
    if(UA_ByteString_allocBuffer(&csr, 1) != UA_STATUSCODE_GOOD)
        return;
    csr.data[0] = 0;

    UA_Boolean readStagedKey = true;
    UA_KeyValuePair param;
    param.key = UA_QUALIFIEDNAME(0, "staged-private-key");
    UA_Variant_setScalar(&param.value, &readStagedKey, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap params = {1, &param};

    UA_StatusCode res = isp->createSigningRequest(isp, NULL, NULL, &params, &csr, stagedKey);
    UA_ByteString_clear(&csr);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_memZero(stagedKey);
        UA_ByteString_clear(stagedKey);
    }
}

/* The staged private key file is read once when the SecurityPolicy is created.
 * While the server runs, the SecurityPolicy holds the staged key in memory. */
static void
loadStagedPrivateKeyFromFilestore(UA_SecurityPolicy *sp) {
    Filestore_Context *pc = (Filestore_Context *)sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;
    if(!isp->createSigningRequest ||
       !getStagedKeyCertificateTypeName(&sp->certificateTypeId))
        return;

    char path[UA_PATH_MAX];
    if(createStagedKeyFilename(pc->storePath, &sp->certificateTypeId,
                               path, UA_PATH_MAX) != UA_STATUSCODE_GOOD)
        return;

    /* No key is staged if the file does not exist */
    UA_ByteString stagedKey = UA_BYTESTRING_NULL;
    UA_StatusCode res = readFileToByteString(path, &stagedKey);
    if(res == UA_STATUSCODE_BADNOTFOUND)
        return;
    if(res == UA_STATUSCODE_GOOD && stagedKey.length == 0)
        res = UA_STATUSCODE_BADINTERNALERROR;

    /* An existing key in newPrivateKey becomes the staged key of the inner
     * SecurityPolicy. No CSR is created. */
    if(res == UA_STATUSCODE_GOOD) {
        UA_ByteString csr = UA_BYTESTRING_NULL;
        res = isp->createSigningRequest(isp, NULL, NULL, &UA_KEYVALUEMAP_NULL,
                                        &csr, &stagedKey);
        UA_ByteString_clear(&csr);
    }
    UA_ByteString_memZero(&stagedKey);
    UA_ByteString_clear(&stagedKey);

    /* The server starts without the staged key. A certificate for it is
     * rejected, and the CertificateManager has to create a new CSR. */
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                       "Could not load the staged private key %s (%s)",
                       path, UA_StatusCode_name(res));
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
    Filestore_Context *pc =
        (Filestore_Context *) sp->policyContext;
    UA_SecurityPolicy *isp = pc->innerPolicy;

    /* A certificate without a PrivateKey can activate the staged key of the
     * last CSR. During execution, the inner SecurityPolicy holds the staged
     * key. */
    UA_ByteString activatedKey = UA_BYTESTRING_NULL;
    if(newPrivateKey.length == 0 &&
       getStagedKeyCertificateTypeName(&sp->certificateTypeId)) {
        readStagedPrivateKeyFromPolicy(isp, &activatedKey);
        if(activatedKey.length > 0 &&
           UA_CertificateUtils_checkKeyPair(&newCertificate, &activatedKey) !=
           UA_STATUSCODE_GOOD) {
            UA_ByteString_memZero(&activatedKey);
            UA_ByteString_clear(&activatedKey);
        }
    }

    UA_StatusCode retval = isp->updateCertificate(isp, newCertificate, newPrivateKey);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    sp->localCertificate = isp->localCertificate;

    retval = writeCertificateAndPrivateKeyToFilestore(
        pc->storePath, newCertificate,
        (newPrivateKey.length > 0) ? newPrivateKey : activatedKey);

    /* The replaced certificate discards the staged key (Part 12, 7.10.10).
     * The inner SecurityPolicy has discarded it in memory already. */
    UA_StatusCode removeRes =
        removeStagedPrivateKeyFromFilestore(pc->storePath, &sp->certificateTypeId);
    if(retval == UA_STATUSCODE_GOOD)
        retval = removeRes;

cleanup:
    UA_ByteString_memZero(&activatedKey);
    UA_ByteString_clear(&activatedKey);
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

    /* Reading the staged key doesn't modify anything. */
    const UA_Boolean *readStagedKey = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "staged-private-key"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(readStagedKey && *readStagedKey)
        return isp->createSigningRequest(isp, subjectName, nonce, params, csr,
                                         newPrivateKey);

    UA_StatusCode retval =
        isp->createSigningRequest(isp, subjectName, nonce, params, csr, newPrivateKey);

    /* A failed CSR or a CSR without a new key discards the key of an earlier CSR. */
    if(retval != UA_STATUSCODE_GOOD) {
        (void)removeStagedPrivateKeyFromFilestore(pc->storePath, &sp->certificateTypeId);
        return retval;
    }
    if(!newPrivateKey || newPrivateKey->length == 0)
        return removeStagedPrivateKeyFromFilestore(pc->storePath, &sp->certificateTypeId);

    /* The CSR is not returned if its key is not persisted */
    return writeStagedPrivateKeyToFilestore(pc->storePath, &sp->certificateTypeId,
                                            *newPrivateKey);
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
    sp->policyType = isp->policyType;

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
    UA_SecurityPolicySignatureAlgorithm *certSig = &sp->certSignatureAlgorithm;
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

    /* Restore the staged key of a CSR from before a restart */
    loadStagedPrivateKeyFromFilestore(sp);

    return UA_STATUSCODE_GOOD;
}

#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

#endif /* UA_ENABLE_ENCRYPTION */
