/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Kalycito Infotech Private Limited
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>
#include <open62541/types.h>

#ifdef UA_ENABLE_TPM2_SECURITY
#include <pkcs11.h>

#if UA_MULTITHREADING >= 100
#include <pthread.h>
static pthread_mutex_t initLock128_g = PTHREAD_MUTEX_INITIALIZER;
#endif

#define MAX_ENCRYPTION_SIZE 16
#define SIGNATURE_LENGTH 32
#define UA_AES128CTR_MESSAGENONCE_LENGTH 8
#define UA_AES128CTR_KEYNONCE_LENGTH 4
// counter block=keynonce(4Byte)+Messagenonce(8Byte)+counter(4Byte) see Part14 7.2.2.2.3.2
// for details
#define UA_AES128CTR_COUNTERBLOCK_SIZE 16
#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16

char *encryptionKeyLabel128_g;
char *signingKeyLabel128_g;
char *userPin128_g;
unsigned long slotId128_g;

CK_MECHANISM smech_128 = {CKM_SHA256_HMAC, NULL_PTR, 0};

typedef struct {
    unsigned long sessionHandle;
} PUBSUB_AES128CTR_PolicyContext;

typedef struct {
    unsigned long signingKeyHandle;
    unsigned long encryptingKeyHandle;
    unsigned long keyNonceHandle;
    unsigned long messageNonceHandle;
} PUBSUB_AES128CTR_GroupContext;

static CK_BBOOL
pkcs11_find_object_by_label(UA_PubSubSecurityPolicy *policy,
                            CK_SESSION_HANDLE hSession, char *label,
                            CK_OBJECT_HANDLE *object_handle) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_Boolean returnValue = false;
    unsigned long int foundCount = 0;
    do {
        CK_OBJECT_HANDLE hObject = 0;
        /* Continues a search for token and session objects that match a
         * template, obtaining additional object handles */
        rv = (UA_StatusCode)C_FindObjects(hSession, &hObject, 1, &foundCount);
        if(rv == UA_STATUSCODE_GOOD) {
            CK_ATTRIBUTE attrTemplate[] = {{CKA_LABEL, NULL, 0}};

            /* Obtains the value of one or more attributes of an object*/
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if(attrTemplate[0].ulValueLen > 0)
                attrTemplate[0].pValue = (char *)UA_malloc(attrTemplate[0].ulValueLen);
            /* Obtains the value of one or more attributes of an object*/
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if(attrTemplate[0].ulValueLen > 0) {
                char * val = (char *)UA_malloc(attrTemplate[0].ulValueLen + 1);
                strncpy(val, (const char *)attrTemplate[0].pValue,
                        attrTemplate[0].ulValueLen);
                val[attrTemplate[0].ulValueLen] = '\0';
                if(strncmp(val, label, attrTemplate[0].ulValueLen) == 0)
                    returnValue = true;

                UA_free(val);
                *object_handle = hObject;
            }

            if(attrTemplate[0].pValue)
                UA_free(attrTemplate[0].pValue);

        } else {
            returnValue = false;
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Find objects failed 0x%.8lX",
                         (long unsigned int)returnValue);
        }

    } while(rv == UA_STATUSCODE_GOOD && foundCount > 0);

    return returnValue;
}

static UA_StatusCode
getSessionHandle(UA_PubSubSecurityPolicy *policy,
                 unsigned long *session) {
    CK_FLAGS flags;
    CK_SESSION_INFO sessionInfo;
    CK_C_INITIALIZE_ARGS initArgs;
    unsigned long *pSlotList = NULL;
    unsigned long availableSlotId=0;
    unsigned long int slotCount=0;
    static bool cryptokiInitialized = false;
    UA_StatusCode rv = UA_STATUSCODE_GOOD;

    /* Set locking flag */
    memset(&initArgs, 0, sizeof(initArgs));
    initArgs.flags = CKF_OS_LOCKING_OK;

    if(!cryptokiInitialized) {
        /* Initializes the Cryptoki library */
        rv = (UA_StatusCode)C_Initialize(&initArgs);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Failed to initialize 0x%.8lX",
                         (long unsigned int)rv);
            return EXIT_FAILURE;
        }
        cryptokiInitialized = true;
    }

    /* To obtain the address of the list of slots in the system */
    rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, NULL, &slotCount);
    if((rv == UA_STATUSCODE_GOOD) && (slotCount > 0)) {
        pSlotList = (unsigned long*)UA_malloc(slotCount * sizeof (unsigned long));
        if(pSlotList == NULL) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Unable to allocate memory");
            return EXIT_FAILURE;
        }

        /* To obtain a list of slots in the system */
        rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, pSlotList, &slotCount);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Unable to get slot count 0x%.8lX",
                         (long unsigned int)rv);
            return EXIT_FAILURE;
        }

    } else {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                      "Unable to get slot list");
        return EXIT_FAILURE;
    }

    for(unsigned long int i = 0; i < slotCount; i++) {
        availableSlotId = pSlotList[i];
        if(availableSlotId == slotId128_g) {
            CK_TOKEN_INFO tokenInfo;
            /* Obtains information about a particular token in the system */
            rv = (UA_StatusCode)C_GetTokenInfo(availableSlotId, &tokenInfo);
            if(rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Failed to fetch token info 0x%.8lX",
                             (long unsigned int)rv);
            }
            UA_free(pSlotList);
            break;
        }

    }

    flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    /* Opens a session between an application and a token in a particular slot */
    rv = (UA_StatusCode)C_OpenSession(availableSlotId, flags, (CK_VOID_PTR)NULL,
                                      NULL, session);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to open session 0x%.8lX",
                     (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    rv = (UA_StatusCode)C_GetSessionInfo(*session, &sessionInfo);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to get session info 0x%.8lX",
                     (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    if(sessionInfo.state != CKS_RW_USER_FUNCTIONS) {
        /* Logs a user into a token */
        rv = (UA_StatusCode)C_Login(*session, CKU_USER, (unsigned char *)userPin128_g,
                                    (unsigned long int) strlen(userPin128_g));
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Failed to login session using userpin 0x%.8lX",
                         (long unsigned int)rv);
            return EXIT_FAILURE;
        }
    }

    return rv;
}

static UA_StatusCode
getSecurityKeys(UA_PubSubSecurityPolicy *sp,
                PUBSUB_AES128CTR_GroupContext *gc) {
    UA_Boolean keyObjectFound = false;
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_AES;

    PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)
        sp->policyContext;

    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &keyClass, sizeof(keyClass)},
        {CKA_KEY_TYPE, &keyType, sizeof(keyType)},
        {CKA_LABEL, (void *)encryptionKeyLabel128_g, strlen(encryptionKeyLabel128_g)}
    };

    /* Initializes a search for token and session objects that match a template */
    UA_StatusCode rv = (UA_StatusCode)
        C_FindObjectsInit(pc->sessionHandle, attrTemplate,
                          sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if(rv == UA_STATUSCODE_GOOD) {
        keyObjectFound = pkcs11_find_object_by_label(sp, pc->sessionHandle,
                                                     encryptionKeyLabel128_g,
                                                     &gc->encryptingKeyHandle);
        if(keyObjectFound == false)
            UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed");

        /* Terminates a search for token and session objects */
        rv = (UA_StatusCode)C_FindObjectsFinal(pc->sessionHandle);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed 0x%.8lX",
                         (long unsigned int)rv);
            return rv;
        }

    } else {
        UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Find object initialization failed 0x%.8lX",
                     (long unsigned int)rv);
        return rv;
    }

    CK_BBOOL signingKeyObjectFound = false;
    CK_OBJECT_CLASS signingKeyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE signingKeyType = CKK_GENERIC_SECRET;
    CK_ATTRIBUTE attrTemplateSigningKey[] = {
        {CKA_CLASS, &signingKeyClass, sizeof(signingKeyClass)},
        {CKA_KEY_TYPE, &signingKeyType, sizeof(signingKeyType)},
        {CKA_LABEL, (void *)signingKeyLabel128_g, strlen(signingKeyLabel128_g)}
    };

    rv = (UA_StatusCode)
        C_FindObjectsInit(pc->sessionHandle, attrTemplateSigningKey,
                          sizeof(attrTemplateSigningKey)/sizeof (CK_ATTRIBUTE));
    if(rv == UA_STATUSCODE_GOOD) {
        signingKeyObjectFound =
            pkcs11_find_object_by_label(sp, pc->sessionHandle,
                                        signingKeyLabel128_g, &gc->signingKeyHandle);
        if(signingKeyObjectFound == false){
            UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object signing key failed");
            return EXIT_FAILURE;
        }

        rv = (UA_StatusCode)C_FindObjectsFinal(pc->sessionHandle);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "FindObjectsFinal failed for signing key 0x%.8lX",
                         (long unsigned int)rv);
        }
    } else {
        UA_LOG_ERROR(sp->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Finding object signing key init failed 0x%.8lX",
                     (long unsigned int)rv);
        return rv;
    }

    return rv;
}

static UA_StatusCode
newContext_pubsub_aes128ctr_tpm(UA_PubSubSecurityPolicy *policy,
                                const UA_ByteString *signingKey,
                                const UA_ByteString *encryptingKey,
                                const UA_ByteString *keyNonce,
                                void **gContext) {
    /* Allocate the channel context */
    PUBSUB_AES128CTR_GroupContext *gc = (PUBSUB_AES128CTR_GroupContext *)
        UA_calloc(1, sizeof(PUBSUB_AES128CTR_GroupContext));
    if(gc == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#if UA_MULTITHREADING >= 100
    pthread_mutex_lock(&initLock128_g);
#endif

    unsigned long session = 0;
    UA_StatusCode rv = getSessionHandle(policy, &session);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Initialize session failed 0x%.8lX",
                     (long unsigned int)rv);
        return rv;
    }

    /* Initialize the channel context
     * TODO: Allow more than one session per SecurityPolicy */
    if(session) {
        PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)
            policy->policyContext;
        pc->sessionHandle = session;
    }

    if(signingKey->length == 0 && encryptingKey->length == 0) {
        rv = getSecurityKeys(policy, gc);
        if(rv != UA_STATUSCODE_GOOD){
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "getSecurityKeys failed 0x%.8lX",
                         (long unsigned int)rv);
            return rv;
        }
    } else {
        memcpy(&gc->encryptingKeyHandle, encryptingKey->data, sizeof(encryptingKey));
        memcpy(&gc->signingKeyHandle, signingKey->data, sizeof(signingKey));
    }

    memcpy(&gc->keyNonceHandle, keyNonce->data, keyNonce->length);

    *gContext = gc;

#if UA_MULTITHREADING >= 100
    pthread_mutex_unlock(&initLock128_g);
#endif

    return UA_STATUSCODE_GOOD;
}

static void
deleteContext_pubsub_aes128ctr_tpm(UA_PubSubSecurityPolicy *policy,
                                   void *gContext) {
    PUBSUB_AES128CTR_PolicyContext *pc =
        (PUBSUB_AES128CTR_PolicyContext *)policy->policyContext;
    C_Logout(pc->sessionHandle); /* Logs a user out from a token */
    C_CloseSession(pc->sessionHandle); /* Closes session between application and token */
    C_Finalize(NULL); /* Clean up miscellaneous Cryptoki-associated resources */
    UA_free(gContext);
}

/* This nonce does not need to be a cryptographically random number, it can be
 * pseudo-random */
static UA_StatusCode
generateNonce_pubsub_aes128ctr_tpm(UA_PubSubSecurityPolicy *policy,
                                   void *gContext, UA_ByteString *out) {
    if(policy == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    PUBSUB_AES128CTR_PolicyContext *pc =
        (PUBSUB_AES128CTR_PolicyContext *)policy->policyContext;
    CK_RV rv = C_GenerateRandom(pc->sessionHandle, out->data, out->length);
    return (UA_StatusCode)rv;
}

static UA_StatusCode
sign_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy,
                      void *gContext, const UA_ByteString *message,
                      UA_ByteString *signature) {
    PUBSUB_AES128CTR_PolicyContext *pc =
        (PUBSUB_AES128CTR_PolicyContext *)policy->policyContext;
    PUBSUB_AES128CTR_GroupContext *gc =
        (PUBSUB_AES128CTR_GroupContext *)gContext;

    /* Initializes a signature operation */
    UA_StatusCode rv = (UA_StatusCode)
        C_SignInit(pc->sessionHandle, &smech_128, gc->signingKeyHandle);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signing initialization failed 0x%.8lX",
                     (long unsigned int)rv);
    }

    /* Signs data in a single part, where the signature is an appendix to the data */
    rv = (UA_StatusCode)C_Sign(pc->sessionHandle, message->data, message->length,
                              (CK_BYTE_PTR)signature->data, &signature->length);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signing failed 0x%.8lX", (long unsigned int)rv);
    }

    return rv;
}

static UA_StatusCode
verify_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy,
                            void *gContext, const UA_ByteString *message,
                            const UA_ByteString *signature) {
    PUBSUB_AES128CTR_GroupContext *gc =
        (PUBSUB_AES128CTR_GroupContext *)gContext;
    PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)
        policy->policyContext;

    UA_StatusCode rv = (UA_StatusCode)
        C_VerifyInit(pc->sessionHandle, &smech_128, gc->signingKeyHandle);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signing key verification initialization failed 0x%.8lX",
                     (long unsigned int)rv);
    }

    rv = (UA_StatusCode)C_Verify(pc->sessionHandle, message->data, message->length,
                                 (unsigned char *)signature->data,
                                 (unsigned long)signature->length);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Signing key verification failed 0x%.8lX",
                     (long unsigned int)rv);
    }
    return rv;
}

static UA_StatusCode
encrypt_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy, void *gContext,
                             UA_ByteString *data) {
    if(gContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES128CTR_PolicyContext *pc =
        (PUBSUB_AES128CTR_PolicyContext *)policy->policyContext;
    PUBSUB_AES128CTR_GroupContext *gc =
        (PUBSUB_AES128CTR_GroupContext *)gContext;

    CK_BYTE sizeToEncrypt;
    int partNumber     = 0;
    CK_ULONG decLen    = 16;
    CK_BYTE final      = 0;
    CK_ULONG finalLen  = 0;
    UA_StatusCode rv   = UA_STATUSCODE_GOOD;
    CK_AES_CTR_PARAMS params_encrupt_128;

    /* Prepare the counterBlock required for encryption */
    UA_Byte counterBlockEncrypt[UA_AES128CTR_COUNTERBLOCK_SIZE];
    memcpy(counterBlockEncrypt, &gc->keyNonceHandle, UA_AES128CTR_KEYNONCE_LENGTH);
    memcpy(counterBlockEncrypt + UA_AES128CTR_KEYNONCE_LENGTH,
           &gc->messageNonceHandle, UA_AES128CTR_MESSAGENONCE_LENGTH);
    memset(counterBlockEncrypt + UA_AES128CTR_KEYNONCE_LENGTH +
           UA_AES128CTR_MESSAGENONCE_LENGTH, 0, 4);

    params_encrupt_128.ulCounterBits = sizeof(params_encrupt_128.cb) * 8;
    memcpy(params_encrupt_128.cb, counterBlockEncrypt, sizeof(params_encrupt_128.cb));
    CK_MECHANISM mech_128 = {CKM_AES_CTR, &params_encrupt_128, sizeof(params_encrupt_128)};

    /* Initializes an encryption operation */
    rv = (UA_StatusCode)C_EncryptInit(pc->sessionHandle, &mech_128,
                                      gc->encryptingKeyHandle);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Encrypt initialization failed 0x%.8lX",
                     (long unsigned int)rv);
        return rv;
    }

    if((data->length % MAX_ENCRYPTION_SIZE) != 0)
        sizeToEncrypt = (CK_BYTE)(data->length + (MAX_ENCRYPTION_SIZE - (data->length % MAX_ENCRYPTION_SIZE)));
    else
        sizeToEncrypt = (CK_BYTE)data->length;

    CK_BYTE *cipherText = (CK_BYTE*)UA_malloc(sizeToEncrypt * sizeof(CK_BYTE));
    while(rv == UA_STATUSCODE_GOOD &&
          partNumber * MAX_ENCRYPTION_SIZE <= sizeToEncrypt - MAX_ENCRYPTION_SIZE) {
        /* Continues a multiple-part encryption operation, processing another data part */
        rv = (UA_StatusCode)C_EncryptUpdate(
            pc->sessionHandle, &data->data[partNumber * MAX_ENCRYPTION_SIZE],
            MAX_ENCRYPTION_SIZE, &cipherText[partNumber * MAX_ENCRYPTION_SIZE],
            &decLen);
        if(UA_STATUSCODE_GOOD != rv) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Encrypt update failed 0x%.8lX",
                         (long unsigned int)rv);
            goto cleanup;
        }

        partNumber++;
    }

    /* Finishes a multiple-part encryption operation */
    rv = (UA_StatusCode)C_EncryptFinal(pc->sessionHandle, &final, &finalLen);
    if(UA_STATUSCODE_GOOD != rv) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Encrypt final failed 0x%.8lX",
                     (long unsigned int)rv);
        goto cleanup;
    }

    for(int i=0; i< sizeToEncrypt; i++)
        data->data[i] = cipherText[i];

cleanup:
    UA_free(cipherText);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decrypt_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy,
                             void *gContext, UA_ByteString *data) {
    if(gContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES128CTR_GroupContext *gc =
        (PUBSUB_AES128CTR_GroupContext *)gContext;
    PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)
        policy->policyContext;

    UA_StatusCode rv          = UA_STATUSCODE_GOOD;
    int decodePartNumber      = 0;
    CK_ULONG decodeDecLen     = 16;
    CK_BYTE decodeFinal       = 0;
    CK_ULONG decodeFinalLen   = 0;
    CK_BYTE sizeToDecrypt;
    CK_AES_CTR_PARAMS params_decrypt_128;

    /* Prepare the counterBlock required for decryption */
    UA_Byte counterBlockDecrypt[UA_AES128CTR_COUNTERBLOCK_SIZE];
    memcpy(counterBlockDecrypt, &gc->keyNonceHandle, UA_AES128CTR_KEYNONCE_LENGTH);
    memcpy(counterBlockDecrypt + UA_AES128CTR_KEYNONCE_LENGTH,
           &gc->messageNonceHandle, UA_AES128CTR_MESSAGENONCE_LENGTH);
    memset(counterBlockDecrypt + UA_AES128CTR_KEYNONCE_LENGTH +
           UA_AES128CTR_MESSAGENONCE_LENGTH, 0, 4);

    params_decrypt_128.ulCounterBits = sizeof(params_decrypt_128.cb) * 8;
    memcpy(params_decrypt_128.cb, counterBlockDecrypt, sizeof(params_decrypt_128.cb));
    CK_MECHANISM mech_128 = {CKM_AES_CTR, &params_decrypt_128, sizeof(params_decrypt_128)};

    rv = (UA_StatusCode)C_DecryptInit(pc->sessionHandle, &mech_128,
                                      gc->encryptingKeyHandle);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Decrypt init failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    if((data->length % MAX_ENCRYPTION_SIZE) != 0)
        sizeToDecrypt = (CK_BYTE)(data->length + (MAX_ENCRYPTION_SIZE - (data->length % MAX_ENCRYPTION_SIZE)));
    else
        sizeToDecrypt = (CK_BYTE)data->length;

    CK_BYTE *decodeCiphertext = (CK_BYTE*)UA_malloc(sizeToDecrypt * sizeof(CK_BYTE));

    while(rv == UA_STATUSCODE_GOOD &&
          decodePartNumber * MAX_ENCRYPTION_SIZE <= sizeToDecrypt - MAX_ENCRYPTION_SIZE) {
        rv = (UA_StatusCode)C_DecryptUpdate(pc->sessionHandle,
                                            &data->data[decodePartNumber*MAX_ENCRYPTION_SIZE], MAX_ENCRYPTION_SIZE,
                                            &decodeCiphertext[decodePartNumber*MAX_ENCRYPTION_SIZE], &decodeDecLen);

        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Decrypt update failed 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }

        decodePartNumber++;
    }

    rv = (UA_StatusCode)C_DecryptFinal(pc->sessionHandle, &decodeFinal,
                                       &decodeFinalLen);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Decrypt final failed 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    for(int i=0; i< sizeToDecrypt; i++)
        data->data[i] = decodeCiphertext[i];

cleanup:
    UA_free(decodeCiphertext);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setMessageNonce_pubsub_aes128ctr_tpm(UA_PubSubSecurityPolicy *policy,
                                     void *gContext, const UA_ByteString *nonce) {
    PUBSUB_AES128CTR_GroupContext *gc =
        (PUBSUB_AES128CTR_GroupContext *)gContext;
    if(nonce->length != UA_AES128CTR_MESSAGENONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    memcpy(&gc->messageNonceHandle, nonce->data, nonce->length);
    return UA_STATUSCODE_GOOD;
}

static size_t
getSignatureSize_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy,
                                      const void *gContext) {
    return SIGNATURE_LENGTH;
}

static size_t
getSignatureKeyLength_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy,
                                           const void *gContext) {
    return UA_AES128CTR_SIGNING_KEY_LENGTH;
}

static size_t
getEncryptionKeyLength_pubsub_aes128ctr_tpm(const UA_PubSubSecurityPolicy *policy,
                                            const void *gContext) {
    return UA_AES128CTR_KEY_LENGTH;
}

static void
clear_pubsub_aes128ctr_tpm(UA_PubSubSecurityPolicy *policy) {
    UA_free(policy->policyContext);
    policy->policyContext = NULL;
}

static UA_StatusCode
setKeys_pubsub_aes128ctr_tpm(UA_PubSubSecurityPolicy *policy, void *gContext,
                             const UA_ByteString *signingKey,
                             const UA_ByteString *encryptingKey,
                             const UA_ByteString *keyNonce) {
    PUBSUB_AES128CTR_GroupContext *gc =
        (PUBSUB_AES128CTR_GroupContext *)gContext;
    memcpy(&gc->encryptingKeyHandle, encryptingKey->data, sizeof(encryptingKey));
    memcpy(&gc->signingKeyHandle, signingKey->data, sizeof(signingKey));
    memcpy(&gc->keyNonceHandle, keyNonce->data, sizeof(keyNonce));
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes128CtrTPM(UA_PubSubSecurityPolicy *sp, char *userpin,
                                     unsigned long slotId, char *encryptionKeyLabel,
                                     char* signingKeyLabel, const UA_Logger *logger) {
    /* TODO: Move global variables into the policy context */
    encryptionKeyLabel128_g = encryptionKeyLabel;
    signingKeyLabel128_g = signingKeyLabel;
    slotId128_g = slotId;
    userPin128_g = userpin;

    /* Initialize the SecurityPolicy */
    memset(sp, 0, sizeof(UA_PubSubSecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR");

    /* Set the method pointers */
    sp->newGroupContext = newContext_pubsub_aes128ctr_tpm;
    sp->deleteGroupContext = deleteContext_pubsub_aes128ctr_tpm;
    sp->verify = verify_pubsub_aes128ctr_tpm;
    sp->sign = sign_pubsub_aes128ctr_tpm;
    sp->getSignatureSize = getSignatureSize_pubsub_aes128ctr_tpm;
    sp->getSignatureKeyLength = getSignatureKeyLength_pubsub_aes128ctr_tpm;
    sp->getEncryptionKeyLength = getEncryptionKeyLength_pubsub_aes128ctr_tpm;
    sp->encrypt = encrypt_pubsub_aes128ctr_tpm;
    sp->decrypt = decrypt_pubsub_aes128ctr_tpm;
    sp->setSecurityKeys = setKeys_pubsub_aes128ctr_tpm;
    sp->generateKey = NULL;
    sp->generateNonce = generateNonce_pubsub_aes128ctr_tpm;
    sp->nonceLength = UA_AES128CTR_SIGNING_KEY_LENGTH +
        UA_AES128CTR_KEY_LENGTH + UA_AES128CTR_KEYNONCE_LENGTH;
    sp->setMessageNonce = setMessageNonce_pubsub_aes128ctr_tpm;
    sp->clear = clear_pubsub_aes128ctr_tpm;

    /* Initialize the policyContext */
    PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)
        UA_calloc(1, sizeof(PUBSUB_AES128CTR_PolicyContext));
    sp->policyContext = (void *)pc;
    if(!pc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    return UA_STATUSCODE_GOOD;
}
#endif
