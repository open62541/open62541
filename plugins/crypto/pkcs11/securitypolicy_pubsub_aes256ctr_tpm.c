/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Kalycito Infotech Private Limited
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>
#include <open62541/types.h>

#ifdef UA_ENABLE_TPM2_PKCS11
#include <pkcs11.h>

#define MAX_ENCRYPTION_SIZE 16
/* Signing key rsa1024 is used */
#define SIGNATURE_LENGTH    128

#define UA_AES256CTR_MESSAGENONCE_LENGTH 8

char *encryptionKeyLabel_g;
char *signingKeyLabel_g;
unsigned long slotId_g;
char *userpin_g;
CK_AES_CTR_PARAMS params = {
    .ulCounterBits = sizeof(params.cb) * 8,
    /* initialize the counter to something other than 0 */
    .cb = {
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15,
    }
};
CK_MECHANISM mech = {
    CKM_AES_CTR, &params, sizeof(params)
};

typedef struct {
    UA_PubSubSecurityPolicy *securityPolicy;
    unsigned long session;
} PUBSUB_AES256CTR_PolicyContext;

typedef struct {
    PUBSUB_AES256CTR_PolicyContext *policyContext;
    unsigned long signingKey;
    unsigned long encryptingKey;
    unsigned long keyNonce;
    unsigned long messageNonce;
} PUBSUB_AES256CTR_ChannelContext;

static
CK_BBOOL pkcs11_find_object_by_label(UA_PubSubSecurityPolicy *policy, CK_SESSION_HANDLE hSession, char *label,
                                     CK_OBJECT_HANDLE *object_handle) {

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_Boolean returnValue = UA_FALSE;
    unsigned long int foundCount = 0;
    do {
        CK_OBJECT_HANDLE hObject = 0;
        /* Continues a search for token and session objects that match a template, obtaining additional object handles */
        rv = (UA_StatusCode)C_FindObjects(hSession, &hObject, 1, &foundCount);
        if (rv == UA_STATUSCODE_GOOD) {
            CK_ATTRIBUTE attrTemplate[] = {
                {CKA_LABEL, NULL, 0}
            };
            /* Obtains the value of one or more attributes of an object*/
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0)
                attrTemplate[0].pValue = (char *)UA_malloc(attrTemplate[0].ulValueLen);
            /* Obtains the value of one or more attributes of an object*/
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0) {
                char * val = (char *)UA_malloc(attrTemplate[0].ulValueLen + 1);
                strncpy(val, (const char*)attrTemplate[0].pValue, attrTemplate[0].ulValueLen);
                val[attrTemplate[0].ulValueLen] = '\0';
                if (strncmp(val, label, attrTemplate[0].ulValueLen) == 0) {
                    returnValue = UA_TRUE;
                }

                UA_free(val);
                *object_handle = hObject;
            }

            if (attrTemplate[0].pValue)
                UA_free(attrTemplate[0].pValue);

        } else {
            returnValue = UA_FALSE;
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Find objects failed 0x%.8lX", (long unsigned int)returnValue);
        }

    } while(rv == UA_STATUSCODE_GOOD && foundCount > 0);

    return returnValue;
}

static UA_StatusCode getSessionHandle(unsigned long *session,
                                      PUBSUB_AES256CTR_PolicyContext *policy) {
    CK_FLAGS flags;
    unsigned long *pSlotList = NULL;
    unsigned long availableSlotId;
    unsigned long int slotCount=0;

    /* Initializes the Cryptoki library */
    UA_StatusCode rv = (UA_StatusCode)C_Initialize(NULL_PTR);
    if (rv != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Failed to initialize 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    /* To obtain the address of the list of slots in the system */
    rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, NULL, &slotCount);
    if ((rv == UA_STATUSCODE_GOOD) && (slotCount > 0)) {
        pSlotList = (unsigned long*)UA_malloc(slotCount * sizeof (unsigned long));
        if (pSlotList == NULL) {
             UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                          "Unable to allocate memory");
            return EXIT_FAILURE;
        }

        /* To obtain a list of slots in the system */
        rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, pSlotList, &slotCount);
        if (rv != UA_STATUSCODE_GOOD) {
             UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                          "Unable to get slot count 0x%.8lX", (long unsigned int)rv);
            return EXIT_FAILURE;
        }

    } else {
         UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Unable to get slot list");
        return EXIT_FAILURE;
    }

    for (unsigned long int i = 0; i < slotCount; i++) {
        availableSlotId = pSlotList[i];
        if (availableSlotId == slotId_g) {
            CK_TOKEN_INFO tokenInfo;
            /* Obtains information about a particular token in the system */
            rv = (UA_StatusCode)C_GetTokenInfo(availableSlotId, &tokenInfo);
            if (rv != UA_STATUSCODE_GOOD) {
                 UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                              "Failed to fetch token info 0x%.8lX", (long unsigned int)rv);
            }

            break;
        }

    }

    flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    /* Opens a session between an application and a token in a particular slot */
    rv = (UA_StatusCode)C_OpenSession(availableSlotId, flags, (CK_VOID_PTR) NULL, NULL, session);
    if (rv != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Failed to open session 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    /* Logs a user into a token */
    rv = (UA_StatusCode)C_Login(*session, CKU_USER, (unsigned char *)userpin_g, (unsigned long int) strlen(userpin_g));
    if (rv != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(policy->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Failed to login session using userpin 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    return rv;
}

static UA_StatusCode getSecurityKeys(PUBSUB_AES256CTR_ChannelContext *cc) {
    UA_Boolean keyObjectFound = UA_FALSE;
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_AES;

    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &keyClass, sizeof(keyClass)},
        {CKA_KEY_TYPE, &keyType, sizeof(keyType)},
        {CKA_LABEL, (void *)encryptionKeyLabel_g, strlen(encryptionKeyLabel_g)}
    };

    /* Initializes a search for token and session objects that match a template */
    UA_StatusCode rv = (UA_StatusCode)C_FindObjectsInit(cc->policyContext->session, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == UA_STATUSCODE_GOOD) {
        keyObjectFound = pkcs11_find_object_by_label(cc->policyContext->securityPolicy,
                                                     cc->policyContext->session, encryptionKeyLabel_g,
                                                     &cc->encryptingKey);
        if (keyObjectFound == UA_FALSE)
            UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed");

        /* Terminates a search for token and session objects */
        rv = (UA_StatusCode)C_FindObjectsFinal(cc->policyContext->session);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed 0x%.8lX", (long unsigned int)rv);
            return rv;
        }

    } else {
        UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Find object initialization failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    CK_BBOOL signingKeyObjectFound = false;
    CK_OBJECT_CLASS signingKeyClass = CKO_PRIVATE_KEY;
    CK_KEY_TYPE signingKeyType = CKK_RSA;
    CK_ATTRIBUTE attrTemplateSigningKey[] = {
        {CKA_CLASS, &signingKeyClass, sizeof(signingKeyClass)},
        {CKA_KEY_TYPE, &signingKeyType, sizeof(signingKeyType)},
        {CKA_LABEL, (void *)signingKeyLabel_g, strlen(signingKeyLabel_g)}
    };

    rv = (UA_StatusCode)C_FindObjectsInit(cc->policyContext->session, attrTemplateSigningKey, sizeof(attrTemplateSigningKey)/sizeof (CK_ATTRIBUTE));
    if (rv == UA_STATUSCODE_GOOD) {
        signingKeyObjectFound = pkcs11_find_object_by_label(cc->policyContext->securityPolicy,
                                                            cc->policyContext->session, signingKeyLabel_g,
                                                            &cc->signingKey);
        if (signingKeyObjectFound == false){
            UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object signing key failed");
            return EXIT_FAILURE;
        }

        rv = (UA_StatusCode)C_FindObjectsFinal(cc->policyContext->session);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "FindObjectsFinal failed for signing key 0x%.8lX", (long unsigned int)rv);
        }
    } else {
        UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Finding object signing key init failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    return rv;
}

static UA_StatusCode
channelContext_newContext_sp_pubsub_aes256ctr_tpm(void *policyContext,
                                                  const UA_ByteString *signingKey,
                                                  const UA_ByteString *encryptingKey,
                                                  const UA_ByteString *keyNonce,
                                                  void **wgContext) {

    /* Allocate the channel context */
    PUBSUB_AES256CTR_ChannelContext *cc = (PUBSUB_AES256CTR_ChannelContext *)
        UA_calloc(1, sizeof(PUBSUB_AES256CTR_ChannelContext));
    if(cc == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    cc->policyContext = (PUBSUB_AES256CTR_PolicyContext *)policyContext;

    unsigned long session;
    UA_StatusCode rv = getSessionHandle(&session, cc->policyContext);
    if (rv != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Initialize session failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    /* Initialize the channel context */
    if(session)
        memcpy(&cc->policyContext->session, &session, sizeof(session));

    if (signingKey->length == 0 && encryptingKey->length == 0) {
        rv = getSecurityKeys(cc);
        if (rv != UA_STATUSCODE_GOOD){
            UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "getSecurityKeys failed 0x%.8lX", (long unsigned int)rv);
            return rv;
        }
    } else {
        memcpy(&cc->encryptingKey, encryptingKey->data, sizeof(encryptingKey));
        memcpy(&cc->signingKey, signingKey->data, sizeof(signingKey));
    }

    *wgContext = cc;
    return UA_STATUSCODE_GOOD;
}

static void
channelContext_deleteContext_sp_pubsub_aes256ctr_tpm(PUBSUB_AES256CTR_ChannelContext *cc) {
    /* Logs a user out from a token */
    C_Logout(cc->policyContext->session);
    /* Closes a session between an application and a token */
    C_CloseSession(cc->policyContext->session);
    /* Clean up miscellaneous Cryptoki-associated resources */
    C_Finalize(NULL);

    UA_free(cc->policyContext);
    UA_free(cc);
}

/* This nonce does not need to be a cryptographically random number, it can be
 * pseudo-random */
static UA_StatusCode
generateNonce_sp_pubsub_aes256ctr(void *policyContext, UA_ByteString *out) {
    if(policyContext == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_PolicyContext *pc = (PUBSUB_AES256CTR_PolicyContext *)policyContext;

    CK_RV rv = C_GenerateRandom(pc->session, out->data, out->length);
    return (UA_StatusCode)rv;
}

static UA_StatusCode
sign_sp_pubsub_aes256ctr_tpm(PUBSUB_AES256CTR_ChannelContext *cc,
                         const UA_ByteString *data, UA_ByteString *signature) {

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    CK_MECHANISM smech = {CKM_RSA_PKCS, NULL_PTR, 0};

    /* Initializes a signature operation */
    rv = (UA_StatusCode)C_SignInit(cc->policyContext->session, &smech, cc->signingKey);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signing initialization failed 0x%.8lX", (long unsigned int)rv);
    }

    /* Signs data in a single part, where the signature is an appendix to the data */
    rv = (UA_StatusCode)C_Sign(cc->policyContext->session, data->data, data->length,
                              (CK_BYTE_PTR)signature->data, &signature->length);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signing failed 0x%.8lX", (long unsigned int)rv);
    }

    return rv;
}

static UA_StatusCode
encrypt_sp_pubsub_aes256ctr_tpm(const PUBSUB_AES256CTR_ChannelContext *cc, UA_ByteString *data) {

    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    CK_BYTE sizeToEncrypt;
    int partNumber     = 0;
    CK_ULONG decLen    = 16;
    CK_BYTE final      = 0;
    CK_ULONG finalLen  = 0;
    UA_StatusCode rv   = UA_STATUSCODE_GOOD;
    /* Initializes an encryption operation */
    rv = (UA_StatusCode)C_EncryptInit(cc->policyContext->session, &mech, cc->encryptingKey);
    if (rv != UA_STATUSCODE_GOOD) {
         UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Encrypt initialization failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    if ((data->length % MAX_ENCRYPTION_SIZE) != 0)
        sizeToEncrypt = (CK_BYTE)(data->length + (MAX_ENCRYPTION_SIZE - (data->length % MAX_ENCRYPTION_SIZE)));
    else
        sizeToEncrypt = (CK_BYTE)data->length;

    CK_BYTE *cipherText = (CK_BYTE*)UA_malloc(sizeToEncrypt * sizeof(CK_BYTE));
    while(rv == UA_STATUSCODE_GOOD && partNumber * MAX_ENCRYPTION_SIZE <= sizeToEncrypt - MAX_ENCRYPTION_SIZE) {
        /* Continues a multiple-part encryption operation, processing another data part */
        rv = (UA_StatusCode)C_EncryptUpdate(cc->policyContext->session,
                                            &data->data[partNumber*MAX_ENCRYPTION_SIZE], MAX_ENCRYPTION_SIZE,
                                            &cipherText[partNumber*MAX_ENCRYPTION_SIZE], &decLen);
        if (UA_STATUSCODE_GOOD != rv) {
             UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Encrypt update failed 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }

        partNumber++;
    }

    /* Finishes a multiple-part encryption operation */
    rv = (UA_StatusCode)C_EncryptFinal(cc->policyContext->session, &final, &finalLen);
    if (UA_STATUSCODE_GOOD != rv) {
         UA_LOG_ERROR(cc->policyContext->securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Encrypt final failed 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    for (int i=0; i< sizeToEncrypt; i++)
        data->data[i] = cipherText[i];

cleanup:
    UA_free(cipherText);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setMessageNonce_sp_pubsub_aes256ctr_tpm(PUBSUB_AES256CTR_ChannelContext *cc,
                                                       const UA_ByteString *nonce) {
    if(nonce->length != UA_AES256CTR_MESSAGENONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    memcpy(&cc->messageNonce, nonce->data, nonce->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
getSignatureSize_sp_pubsub_aes256ctr_tpm(const void *channelContext) {
    return SIGNATURE_LENGTH;
}

static void
deleteMembers_sp_pubsub_aes256ctr_tpm(UA_PubSubSecurityPolicy *policy) {
    /* Can be used for future implementation */
}

static UA_StatusCode
channelContext_setKeys_sp_pubsub_aes256ctr_tpm(PUBSUB_AES256CTR_ChannelContext *cc,
                                               const UA_ByteString *signingKey,
                                               const UA_ByteString *encryptingKey,
                                               const UA_ByteString *keyNonce) {

    memcpy(&cc->encryptingKey, encryptingKey->data, sizeof(encryptingKey));
    memcpy(&cc->signingKey, signingKey->data, sizeof(signingKey));
    memcpy(&cc->keyNonce, keyNonce->data, sizeof(keyNonce));
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes256CtrTPM (UA_PubSubSecurityPolicy *policy, char *userpin,
                                      unsigned long slotId, char *encryptionKeyLabel,
                                      char* signingKeyLabel, const UA_Logger *logger) {
    encryptionKeyLabel_g = encryptionKeyLabel;
    signingKeyLabel_g = signingKeyLabel;
    slotId_g = slotId;
    userpin_g = userpin;

    memset(policy, 0, sizeof(UA_PubSubSecurityPolicy));
    policy->logger = logger;
    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    policy->newContext = channelContext_newContext_sp_pubsub_aes256ctr_tpm;
    policy->deleteContext = (void (*)(void *))channelContext_deleteContext_sp_pubsub_aes256ctr_tpm;
    policy->setSecurityKeys = (UA_StatusCode(*)(void *, const UA_ByteString *,
                                                const UA_ByteString *,
                                                const UA_ByteString *))
                              channelContext_setKeys_sp_pubsub_aes256ctr_tpm;
    policy->setMessageNonce = (UA_StatusCode(*)(void *, const UA_ByteString *))
                              channelContext_setMessageNonce_sp_pubsub_aes256ctr_tpm;
    policy->clear = deleteMembers_sp_pubsub_aes256ctr_tpm;

    UA_SecurityPolicySymmetricModule *symmetricModule = &policy->symmetricModule;
    symmetricModule->generateNonce = generateNonce_sp_pubsub_aes256ctr;
    UA_SecurityPolicySignatureAlgorithm *signatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    signatureAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#sha256");
    signatureAlgorithm->sign =
        (UA_StatusCode(*)(void *, const UA_ByteString *, UA_ByteString *))sign_sp_pubsub_aes256ctr_tpm;
    signatureAlgorithm->getLocalSignatureSize = getSignatureSize_sp_pubsub_aes256ctr_tpm;

    UA_SecurityPolicyEncryptionAlgorithm *encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    encryptionAlgorithm->uri =
        UA_STRING("https://tools.ietf.org/html/rfc3686"); /* Temp solution */
    encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))encrypt_sp_pubsub_aes256ctr_tpm;

    PUBSUB_AES256CTR_PolicyContext *pc = (PUBSUB_AES256CTR_PolicyContext *)
        UA_calloc(1, sizeof(PUBSUB_AES256CTR_PolicyContext));
    policy->policyContext = (void *)pc;
    if(!pc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    pc->securityPolicy = policy;

    return UA_STATUSCODE_GOOD;
}
#endif
