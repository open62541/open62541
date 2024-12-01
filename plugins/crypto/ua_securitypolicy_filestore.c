/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/plugin/securitypolicy_default.h>

#include "mp_printf.h"
#include "ua_filestore_common.h"

#ifdef UA_ENABLE_ENCRYPTION

#ifdef __linux__ /* Linux only so far */

#include <stdio.h>
#include <dirent.h>

#ifndef __ANDROID__
#include <bits/stdio_lim.h>
#endif  // !__ANDROID__

typedef struct {
    /* In-Memory security policy as a base */
    UA_SecurityPolicy *innerPolicy;
    UA_String storePath;
} SecurityPolicy_FilestoreContext;

static bool
checkCertificateInFilestore(char *path, const UA_ByteString newCertificate) {
    /* Check if the file is already stored in CertStore */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    bool isStored = false;
    UA_ByteString fileData = UA_BYTESTRING_NULL;
    DIR *dir = opendir(path);
    if(!dir)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct dirent *dirent;
    while((dirent = readdir(dir)) != NULL) {
        if(dirent->d_type != DT_REG)
            continue;

        /* Get filename to load */
        char filename[FILENAME_MAX];
        if(mp_snprintf(filename, FILENAME_MAX, "%s/%s", path, dirent->d_name) < 0)
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
    closedir(dir);

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
    thumbprint.data = (UA_Byte*)UA_malloc(sizeof(UA_Byte)*thumbprint.length);

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
    char ownCertPathDir[PATH_MAX];
    if(mp_snprintf(ownCertPathDir, PATH_MAX, "%.*s%s", (int)storePath.length,
                   (char *)storePath.data, "/ApplCerts/own/certs") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    char ownKeyPathDir[PATH_MAX];
    if(mp_snprintf(ownKeyPathDir, PATH_MAX, "%.*s%s", (int)storePath.length,
                   (char *)storePath.data, "/ApplCerts/own/private") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check if certificate is already stored */
    if(checkCertificateInFilestore(ownCertPathDir, newCertificate))
        return UA_STATUSCODE_GOOD;

    /* Create filename for new certificate */
    char newFilename[PATH_MAX];
    retval = createCertName(&newCertificate, newFilename, PATH_MAX);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    char newCertFilename[PATH_MAX];
    if(mp_snprintf(newCertFilename, PATH_MAX, "%s/%s%s", ownCertPathDir, newFilename, ".der") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    char newKeyFilename[PATH_MAX];
    if(mp_snprintf(newKeyFilename, PATH_MAX, "%s/%s%s", ownKeyPathDir, newFilename, ".key") < 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval = writeByteStringToFile(newCertFilename, &newCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return writeByteStringToFile(newKeyFilename, &newPrivateKey);
}

/********************/
/* AsymmetricModule */
/********************/

static UA_StatusCode
asym_makeThumbprint_sp_filestore(const UA_SecurityPolicy *securityPolicy,
                                 const UA_ByteString *certificate,
                                 UA_ByteString *thumbprint) {
    SecurityPolicy_FilestoreContext *pc =
        (SecurityPolicy_FilestoreContext *) securityPolicy->policyContext;
    return pc->innerPolicy->asymmetricModule.makeCertificateThumbprint(pc->innerPolicy, certificate, thumbprint);
}

static UA_StatusCode
asym_compareCertificateThumbprint_sp_filestore(const UA_SecurityPolicy *securityPolicy,
                                                          const UA_ByteString *certificateThumbprint) {
    SecurityPolicy_FilestoreContext *pc =
        (SecurityPolicy_FilestoreContext *) securityPolicy->policyContext;
    return pc->innerPolicy->asymmetricModule.compareCertificateThumbprint(pc->innerPolicy, certificateThumbprint);
}

/*******************/
/* SymmetricModule */
/*******************/

static UA_StatusCode
sym_generateKey_sp_filestore(void *policyContext, const UA_ByteString *secret,
                                  const UA_ByteString *seed, UA_ByteString *out) {
    SecurityPolicy_FilestoreContext *pc =
        (SecurityPolicy_FilestoreContext *) policyContext;
    return pc->innerPolicy->symmetricModule.generateKey(pc->innerPolicy->policyContext, secret, seed, out);
}

static UA_StatusCode
sym_generateNonce_sp_filestore(void *policyContext, UA_ByteString *out) {
    SecurityPolicy_FilestoreContext *pc =
    (SecurityPolicy_FilestoreContext *) policyContext;
    return pc->innerPolicy->symmetricModule.generateNonce(pc->innerPolicy->policyContext, out);
}

/*****************/
/* ChannelModule */
/*****************/

static UA_StatusCode
channelContext_newContext_sp_filestore(const UA_SecurityPolicy *securityPolicy,
                                       const UA_ByteString *remoteCertificate,
                                       void **pp_contextData) {
    SecurityPolicy_FilestoreContext *pc =
        (SecurityPolicy_FilestoreContext *) securityPolicy->policyContext;
    return pc->innerPolicy->channelModule.newContext(pc->innerPolicy, remoteCertificate, pp_contextData);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_filestore(UA_SecurityPolicy *securityPolicy,
                                            const UA_ByteString newCertificate,
                                            const UA_ByteString newPrivateKey) {
    /* Temporarily save the old certificate file so that it can be removed from CertStore */
    UA_ByteString localCertificateTmp;
    UA_ByteString_copy(&securityPolicy->localCertificate, &localCertificateTmp);

    SecurityPolicy_FilestoreContext *pc =
        (SecurityPolicy_FilestoreContext *) securityPolicy->policyContext;

    UA_StatusCode retval =
        pc->innerPolicy->updateCertificateAndPrivateKey(pc->innerPolicy, newCertificate, newPrivateKey);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    securityPolicy->localCertificate = pc->innerPolicy->localCertificate;

    retval =
        writeCertificateAndPrivateKeyToFilestore(pc->storePath, newCertificate, newPrivateKey);
    UA_ByteString_clear(&localCertificateTmp);

    return retval;
}

static UA_StatusCode
createSigningRequest_sp_filestore(UA_SecurityPolicy *securityPolicy,
                                  const UA_String *subjectName,
                                  const UA_ByteString *nonce,
                                  const UA_KeyValueMap *params,
                                  UA_ByteString *csr,
                                  UA_ByteString *newPrivateKey) {
    SecurityPolicy_FilestoreContext *pc =
        (SecurityPolicy_FilestoreContext *) securityPolicy->policyContext;
    return pc->innerPolicy->createSigningRequest(pc->innerPolicy, subjectName, nonce, params, csr, newPrivateKey);
}

static void
clear_sp_filestore(UA_SecurityPolicy *securityPolicy) {
    if(!securityPolicy)
        return;

    if(!securityPolicy->policyContext)
        return;

    SecurityPolicy_FilestoreContext *pc =
            (SecurityPolicy_FilestoreContext *) securityPolicy->policyContext;

    pc->innerPolicy->clear(pc->innerPolicy);
    UA_String_clear(&pc->storePath);

    UA_free(pc->innerPolicy);
    UA_free(pc);
    securityPolicy->policyContext = NULL;
}


static void
init_sp_filestore(UA_SecurityPolicy *policy) {
    SecurityPolicy_FilestoreContext *pc =
            (SecurityPolicy_FilestoreContext *) policy->policyContext;

    policy->policyUri = pc->innerPolicy->policyUri;
    policy->securityLevel = pc->innerPolicy->securityLevel;
    policy->localCertificate = pc->innerPolicy->localCertificate;
    policy->certificateGroupId = pc->innerPolicy->certificateGroupId;
    policy->certificateTypeId = pc->innerPolicy->certificateTypeId;
    policy->logger = pc->innerPolicy->logger;

    UA_SecurityPolicyAsymmetricModule *const asymmetricModule = &policy->asymmetricModule;
    memcpy(asymmetricModule, &pc->innerPolicy->asymmetricModule, sizeof(UA_SecurityPolicyAsymmetricModule));
    asymmetricModule->makeCertificateThumbprint = asym_makeThumbprint_sp_filestore;
    asymmetricModule->compareCertificateThumbprint = asym_compareCertificateThumbprint_sp_filestore;

    UA_SecurityPolicySymmetricModule *const symmetricModule = &policy->symmetricModule;
    memcpy(symmetricModule, &pc->innerPolicy->symmetricModule, sizeof(UA_SecurityPolicySymmetricModule));
    symmetricModule->generateKey = sym_generateKey_sp_filestore;
    symmetricModule->generateNonce = sym_generateNonce_sp_filestore;

    UA_SecurityPolicyChannelModule *const channelModule = &policy->channelModule;
    memcpy(channelModule, &pc->innerPolicy->channelModule, sizeof(UA_SecurityPolicyChannelModule));
    channelModule->newContext = channelContext_newContext_sp_filestore;

    policy->certificateSigningAlgorithm = pc->innerPolicy->certificateSigningAlgorithm;

    policy->createSigningRequest = createSigningRequest_sp_filestore;
    policy->updateCertificateAndPrivateKey = updateCertificateAndPrivateKey_sp_filestore;
    policy->clear = clear_sp_filestore;
}

static UA_StatusCode
policyContext_newContext_sp_filestore(UA_SecurityPolicy *policy, const UA_String storePath) {
    if(!policy)
        return UA_STATUSCODE_BADINTERNALERROR;

    SecurityPolicy_FilestoreContext *pc = (SecurityPolicy_FilestoreContext *)
        UA_calloc(1, sizeof(SecurityPolicy_FilestoreContext));
    if(!pc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_String_copy(&storePath, &pc->storePath);
    policy->policyContext = (void *)pc;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecurityPolicy_Filestore(UA_SecurityPolicy *policy,
                            UA_SecurityPolicy *innerPolicy,
                            const UA_String storePath) {
    if(!policy || !innerPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset(policy, 0, sizeof(UA_SecurityPolicy));

    retval = policyContext_newContext_sp_filestore(policy, storePath);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    SecurityPolicy_FilestoreContext *pc =
            (SecurityPolicy_FilestoreContext *) policy->policyContext;
    pc->innerPolicy = innerPolicy;

    init_sp_filestore(policy);

    return retval;
}

#endif

#endif
