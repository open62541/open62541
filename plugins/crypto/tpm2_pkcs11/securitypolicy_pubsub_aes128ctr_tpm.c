/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Kalycito Infotech Private Limited
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>
#include "pkcs11.h"

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

static UA_StatusCode
encryptTPM(UA_PubSubSecurityPolicyTPM *policy, unsigned long session, unsigned long hKey,
           UA_ByteString *data) {

    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    unsigned long int ciphertext_length = 0;

    rv = (UA_StatusCode)C_EncryptInit(session, &mech, hKey);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Encrypt initialization failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    // Determine how much memory will be required to hold the ciphertext
    rv = (UA_StatusCode)C_Encrypt(session, data->data, data->length, NULL, &ciphertext_length);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Encryption failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    // Allocate the required memory.
    unsigned char *ciphertext = (unsigned char *)UA_malloc(ciphertext_length);
    if (ciphertext ==  NULL) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Memory allocation failed");
        rv = CKR_HOST_MEMORY;
        return rv;
    }
    memset(ciphertext, 0, ciphertext_length);

    // Encrypt the data
    rv = (UA_StatusCode)C_Encrypt(session, data->data, data->length, ciphertext, &ciphertext_length);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Encryption failed %s", UA_StatusCode_name(rv));
        return rv;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
decryptTPM(UA_PubSubSecurityPolicyTPM *policy, unsigned long session, unsigned long hKey,
           UA_ByteString *data) {

    CK_BYTE_PTR decrypted_ciphertext = NULL;
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    rv = (UA_StatusCode)C_DecryptInit(session, &mech, hKey);
    if (rv != UA_STATUSCODE_GOOD) {
       UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Decryption initialization failed 0x%.8lX", (long unsigned int)rv);
       return rv;
    }

    unsigned long int decrypted_ciphertext_length = 0;
    rv = (UA_StatusCode)C_Decrypt(session, data->data, data->length, NULL, &decrypted_ciphertext_length);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Decryption failed 0x%.8lX", (long unsigned int)rv);
       return rv;
    }

    // Allocate memory for decrypted ciphertext.
    decrypted_ciphertext = (CK_BYTE_PTR)UA_malloc(decrypted_ciphertext_length + 1); //We want to null terminate the raw chars later
    if (NULL == decrypted_ciphertext) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Could not allocate memory");
        rv = CKR_HOST_MEMORY;
        return rv;
    }

    // Decrypt the ciphertext.
    rv = (UA_StatusCode)C_Decrypt(session, data->data, data->length, decrypted_ciphertext, &decrypted_ciphertext_length);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Decryption failed 0x%.8lX", (long unsigned int)rv);
        return rv;
    }

    decrypted_ciphertext[decrypted_ciphertext_length] = 0; // Turn the chars into a C-String via null termination

    // printf("Decrypted ciphertext: %s\n", decrypted_ciphertext);
    // printf("Decrypted ciphertext length: %lu\n", decrypted_ciphertext_length);
    return rv;
}

static UA_StatusCode
clear(UA_PubSubSecurityPolicyTPM *policy, unsigned long session) {
    C_Logout(session);
    C_CloseSession(session);
    C_Finalize(NULL);
    return UA_STATUSCODE_GOOD;
}

static
CK_BBOOL pkcs11_find_object_by_label(UA_PubSubSecurityPolicyTPM *policy,CK_SESSION_HANDLE hSession, char *label, CK_OBJECT_HANDLE *object_handle) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_Boolean returnValue = UA_FALSE;
    unsigned long int foundCount = 0;
    do
    {
        CK_OBJECT_HANDLE hObject = 0;   // -> zero is an invalid object handle value
        rv = (UA_StatusCode)C_FindObjects( hSession, &hObject, 1, &foundCount );
        if (rv == UA_STATUSCODE_GOOD) {
            CK_ATTRIBUTE attrTemplate[] = {
                {CKA_LABEL, NULL, 0}
            };
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0)
                attrTemplate[0].pValue = (char *)UA_malloc(attrTemplate[0].ulValueLen);
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0) {
                char * val = (char *)UA_malloc(attrTemplate[0].ulValueLen + 1);
	        	//strncpy_s(val, RSIZE_MAX_STR - 1, attrTemplate[0].pValue, attrTemplate[0].ulValueLen);
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
UA_PubSubSecurityPolicy_Aes128CtrTPM (UA_PubSubSecurityPolicyTPM *policy, char *userpin, unsigned long slotId,
                                      char *label, const UA_Logger *logger) {

    CK_OBJECT_HANDLE hKey = 0;
    UA_Boolean key_object_found = UA_FALSE;

    CK_BBOOL ck_false = CK_FALSE;
    CK_OBJECT_CLASS key_class = CKO_SECRET_KEY;
    CK_KEY_TYPE key_type = CKK_AES;
    CK_FUNCTION_LIST_PTR pFunctionList;
    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &key_class, sizeof(key_class)},
        {CKA_KEY_TYPE, &key_type, sizeof(key_type)},
        {CKA_ALWAYS_AUTHENTICATE, &ck_false, sizeof(ck_false)}
    };

    memset(policy, 0, sizeof(UA_PubSubSecurityPolicyTPM));
    unsigned long session;
    policy->logger = logger;
    policy->session = session;

    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR");
    policy->encryptTPM = encryptTPM;
    policy->decryptTPM = decryptTPM;
    policy->clear = clear;

    UA_StatusCode rv;
    rv = (UA_StatusCode)C_GetFunctionList(&pFunctionList);
    if (rv != UA_STATUSCODE_GOOD) {
        printf("Failed get function list\n");
        return EXIT_FAILURE;
    }
 
    rv = (UA_StatusCode)C_Initialize(NULL_PTR);
    if (rv != UA_STATUSCODE_GOOD) {
        printf("Failed to initialize\n");
        return EXIT_FAILURE;
    }

    unsigned long *pSlotList = NULL;
    unsigned long slot_id;
    unsigned long int slot_count=0;
    rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, NULL, &slot_count);
    if ((rv == UA_STATUSCODE_GOOD) && (slot_count > 0)) {
        pSlotList = (unsigned long*)UA_malloc(slot_count * sizeof (unsigned long));

        if (pSlotList == NULL) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Unable to allocate memory");
            return EXIT_FAILURE;
        }

        rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, pSlotList, &slot_count);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Unable to get slot count");
            return EXIT_FAILURE;
        }

    } else {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Unable to get slot list");
        return EXIT_FAILURE;
    }

    for (unsigned long int i = 0; i < slot_count; i++) {
        slot_id = pSlotList[i];
        if (slot_id == slotId) {
            CK_TOKEN_INFO token_info;
            rv = (UA_StatusCode)C_GetTokenInfo(slot_id, &token_info);
            if (rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Failed to fetch token info");
            }
            break;
        }
    }
    
    CK_FLAGS flags;
    flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    rv = (UA_StatusCode)C_OpenSession(slot_id, flags, (CK_VOID_PTR) NULL, NULL, &session);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to open session");
        return EXIT_FAILURE;
    }
    printf ("before login\n");
 
    rv = (UA_StatusCode)C_Login(session, CKU_USER, (unsigned char*) userpin, (unsigned long int) strlen(userpin));
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Failed to login session using userpin 0x%.8lX", (long unsigned int)rv);
        return EXIT_FAILURE;
    }
    printf ("after login\n");

    rv = (UA_StatusCode)C_FindObjectsInit(session, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == UA_STATUSCODE_GOOD) {
        key_object_found = pkcs11_find_object_by_label(policy, session, label, &hKey);
        if (key_object_found == UA_FALSE)
            UA_LOG_ERROR(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Finding object failed");
        rv = (UA_StatusCode)C_FindObjectsFinal(session);
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

    clear(policy, session);
    return UA_STATUSCODE_GOOD;
}

