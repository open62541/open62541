/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Kalycito Infotech Private Limited
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>
#include <open62541/types.h>
#include <pkcs11.h>

#define MAX_ENCRYPTION_SIZE 16

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
    const UA_PubSubSecurityPolicy *securityPolicy;
} PUBSUB_AES128CTR_PolicyContext;

typedef struct {
    PUBSUB_AES128CTR_PolicyContext *policyContext;
    unsigned long session;
    unsigned long key;
} PUBSUB_AES128CTR_ChannelContext;

static void
deleteContext_TPM(PUBSUB_AES128CTR_ChannelContext *cc) {
    UA_free(cc);
}

static UA_StatusCode
newContext_aes128ctr(void *policyContext, unsigned long session, unsigned long key, void **wgContext) {

    /* Allocate the channel context */
    PUBSUB_AES128CTR_ChannelContext *cc = (PUBSUB_AES128CTR_ChannelContext *)
        UA_calloc(1, sizeof(PUBSUB_AES128CTR_ChannelContext));
    if(cc == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Initialize the channel context */
    cc->policyContext = (PUBSUB_AES128CTR_PolicyContext *)policyContext;
    if(session)
        memcpy(&cc->session, &session, sizeof(session));
    if(key)
        memcpy(&cc->key, &key, sizeof(key));
    *wgContext = cc;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
encryptTPM(const PUBSUB_AES128CTR_ChannelContext *cc, UA_ByteString *data) {

    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    CK_BYTE sizeToEncrypt;
    int partNumber     = 0;
    CK_ULONG decLen    = 16;
    CK_BYTE final      = 0;
    CK_ULONG finalLen  = 0;
    UA_StatusCode rv   = UA_STATUSCODE_GOOD;

    /* Initializes an encryption operation */
    rv = (UA_StatusCode)C_EncryptInit(cc->session, &mech, cc->key);
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
        rv = (UA_StatusCode)C_EncryptUpdate(cc->session,
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
    rv = (UA_StatusCode)C_EncryptFinal(cc->session, &final, &finalLen);
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

static void
deleteSession(UA_PubSubSecurityPolicy *policy) {
    /* Logs a user out from a token */
    C_Logout(policy->session);
    /* Closes a session between an application and a token */
    C_CloseSession(policy->session);
    /* Clean up miscellaneous Cryptoki-associated resources */
    C_Finalize(NULL);
}

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

UA_StatusCode
UA_PubSubSecurityPolicy_Aes128CtrTPM (UA_PubSubSecurityPolicy *policy, char *userpin, unsigned long slotId,
                                      char *label, const UA_Logger *logger) {

    UA_StatusCode rv;
    UA_Boolean keyObjectFound = UA_FALSE;
    unsigned long *pSlotList = NULL;
    unsigned long availableSlotId;
    unsigned long int slotCount=0;

    CK_FLAGS flags;
    CK_BBOOL ckFalse = CK_FALSE;
    CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
    CK_KEY_TYPE keyType = CKK_AES;

    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &keyClass, sizeof(keyClass)},
        {CKA_KEY_TYPE, &keyType, sizeof(keyType)},
        {CKA_ALWAYS_AUTHENTICATE, &ckFalse, sizeof(ckFalse)}
    };

    memset(policy, 0, sizeof(UA_PubSubSecurityPolicy));
    policy->logger = logger;
    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR");
    policy->newContextTPM = newContext_aes128ctr;
    policy->deleteContext = (void (*)(void *))deleteContext_TPM;
    policy->clear = deleteSession;

    UA_SecurityPolicySymmetricModule *symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyEncryptionAlgorithm *encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    encryptionAlgorithm->uri =
        UA_STRING("https://tools.ietf.org/html/rfc3686"); /* Temp solution */
    encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))encryptTPM;

    /* Initializes the Cryptoki library */
    rv = (UA_StatusCode)C_Initialize(NULL_PTR);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to initialize 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    /* To obtain the address of the list of slots in the system */
    rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, NULL, &slotCount);
    if ((rv == UA_STATUSCODE_GOOD) && (slotCount > 0)) {
        pSlotList = (unsigned long*)UA_malloc(slotCount * sizeof (unsigned long));
        if (pSlotList == NULL) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Unable to allocate memory");
            return EXIT_FAILURE;
        }

        /* To obtain a list of slots in the system */
        rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, pSlotList, &slotCount);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Unable to get slot count 0x%.8lX", (long unsigned int)rv);
            return EXIT_FAILURE;
        }

    } else {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Unable to get slot list");
        return EXIT_FAILURE;
    }

    for (unsigned long int i = 0; i < slotCount; i++) {
        availableSlotId = pSlotList[i];
        if (availableSlotId == slotId) {
            CK_TOKEN_INFO tokenInfo;
            /* Obtains information about a particular token in the system */
            rv = (UA_StatusCode)C_GetTokenInfo(availableSlotId, &tokenInfo);
            if (rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Failed to fetch token info 0x%.8lX", (long unsigned int)rv);
            }

            break;
        }

    }

    flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    /*Opens a session between an application and a token in a particular slot */
    rv = (UA_StatusCode)C_OpenSession(availableSlotId, flags, (CK_VOID_PTR) NULL, NULL, &policy->session);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to open session 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    /* Logs a user into a token */
    rv = (UA_StatusCode)C_Login(policy->session, CKU_USER, (unsigned char *)userpin, (unsigned long int) strlen(userpin));
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to login session using userpin 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }

    /* Initializes a search for token and session objects that match a template */
    rv = (UA_StatusCode)C_FindObjectsInit(policy->session, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == UA_STATUSCODE_GOOD) {
        keyObjectFound = pkcs11_find_object_by_label(policy, policy->session, label, &policy->key);
        if (keyObjectFound == UA_FALSE)
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed");

        /* Terminates a search for token and session objects */
        rv = (UA_StatusCode)C_FindObjectsFinal(policy->session);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed 0x%.8lX", (long unsigned int)rv);
            return rv;
        }

    } else {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Find object initialization failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    return UA_STATUSCODE_GOOD;
}