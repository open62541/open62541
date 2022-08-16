/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright (c) 2021 Kalycito Infotech Private Limited
 */

 /* Note: Have to enable UA_ENABLE_PUBSUB_ENCRYPTION and UA_ENABLE_ENCRYPTION_TPM2=KEYSTORE
          to run the application */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>

#include <open62541/plugin/securitypolicy_default.h>

#include "ua_pubsub.h"
#include "common.h"

#if defined (UA_ENABLE_PUBSUB_ETH_UADP)
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include <openssl/evp.h>
#include <pkcs11.h>

#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};


UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData);

/* Add new connection to the server */
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &connectionIdentifier);
    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the DataSetReader. */
/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");

    /* Encryption settings */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);

    /* Add the encryption key informaton */
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

    // TODO security token not necessary for readergroup (extracted from security-header)
    UA_Server_setReaderGroupEncryptionKeys(server, readerGroupIdentifier, 1, sk, ek, kn);

    // TODO setOperational MUST be after setting keys
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);

    return retval;
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */
/* Add DataSetReader to the ReaderGroup */
static UA_StatusCode
addDataSetReader(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    /* The following parameters are used to show that the data published by
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the information model */
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);

    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    return retval;
}

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data type.
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

/**
 * **TargetVariables**
 *
 * The SubscribedDataSet option TargetVariables defines a list of Variable mappings between
 * received DataSet fields and target Variables in the Subscriber AddressSpace.
 * The values subscribed from the Publisher are updated in the value field of these variables */
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                           folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields[i].name.data),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newNode);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    retval = UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                           readerConfig.dataSetMetaData.fieldsSize, targetVars);
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    UA_free(targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

/**
 * **DataSetMetaData**
 *
 * The DataSetMetaData describes the content of a DataSet. It provides the information necessary to decode
 * DataSetMessages on the Subscriber side. DataSetMessages received from the Publisher are decoded into
 * DataSet and each field is updated in the Subscriber based on datatype match of TargetVariable fields of Subscriber
 * and PublishedDataSetFields of Publisher */
/* Define MetaData for TargetVariables */
static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name = UA_STRING ("DataSet 1");

    /* Static definition of number of fields size to 4 to create four different
     * targetVariables of distinct datatype
     * Currently the publisher sends only DateTime data type */
    pMetaData->fieldsSize = 4;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* Int32 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                   &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[0].name =  UA_STRING ("Int32");
    pMetaData->fields[0].valueRank = -1; /* scalar */

    /* DateTime DataType */
    UA_FieldMetaData_init (&pMetaData->fields[1]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                    &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[1].name =  UA_STRING ("DateTime");
    pMetaData->fields[1].valueRank = -1; /* scalar */

    /* Int64 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[2]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId,
                   &pMetaData->fields[2].dataType);
    pMetaData->fields[2].builtInType = UA_NS0ID_INT64;
    pMetaData->fields[2].name =  UA_STRING ("Int64");
    pMetaData->fields[2].valueRank = -1; /* scalar */

    /* Boolean DataType */
    UA_FieldMetaData_init (&pMetaData->fields[3]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                    &pMetaData->fields[3].dataType);
    pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[3].name =  UA_STRING ("BoolToggle");
    pMetaData->fields[3].valueRank = -1; /* scalar */
}

/*
 * TODO: add something similary ass addDataSetMetadata for security configuration
 */

/**
 * Followed by the main server code, making use of the above definitions */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4801, NULL);

    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies,
                                      &config->logger);

    /* Add the PubSub network layer implementation to the server config.
     * The TransportLayer is acting as factory to create new connections
     * on runtime. Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#else
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
#endif

    /* API calls */
    /* Add PubSubConnection */
    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add SubscribedVariables to the created DataSetReader */
    retval |= addSubscribedVariables(server, readerIdentifier);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void get_MAC(const uint8_t *message, size_t message_len, unsigned char **message_digest,
                    unsigned int *message_digest_len)
{
    EVP_MD_CTX *md_ctx;
    if((md_ctx = EVP_MD_CTX_new()) == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_MD_CTX_new");
        return;
    }
    if(1 != EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_DigestInit_ex");
        return;
    }
    if(1 != EVP_DigestUpdate(md_ctx, message, message_len)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_DigestUpdate");
        return;
    }
    if((*message_digest = (unsigned char *)OPENSSL_malloc((size_t)EVP_MD_size(EVP_sha256()))) == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while OPENSSL_malloc");
        return;
    }
    if(1 != EVP_DigestFinal_ex(md_ctx, *message_digest, message_digest_len)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error while EVP_DigestFinal_ex");
        return;
    }
    EVP_MD_CTX_free(md_ctx);
}

/* If object is found the object_handle is set */
static UA_Boolean
find_object_by_label(CK_SESSION_HANDLE hSession, char *label, unsigned long *object_handle) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_Boolean rtnval = UA_FALSE;
    unsigned long foundCount = 0;
    do
    {
        CK_OBJECT_HANDLE hObject = 0;
        rv = (UA_StatusCode)C_FindObjects(hSession, &hObject, 1, &foundCount );
        if (rv == UA_STATUSCODE_GOOD) {
            /* This will show the labels and values */
            CK_ATTRIBUTE attrTemplate[] = {
                {CKA_LABEL, NULL_PTR, 0}
            };
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0)
                attrTemplate[0].pValue = (char *)UA_malloc(attrTemplate[0].ulValueLen);
            rv = (UA_StatusCode)C_GetAttributeValue(hSession, hObject, attrTemplate, 1);
            if (attrTemplate[0].ulValueLen > 0) {
                char * val = (char *)UA_malloc(attrTemplate[0].ulValueLen + 1);
                strncpy(val, (const char*)attrTemplate[0].pValue, attrTemplate[0].ulValueLen);
                val[attrTemplate[0].ulValueLen] = '\0';
                if (strcasecmp(val, (char *)label) == 0) rtnval = true;
                UA_free(val);
                *object_handle = hObject;
            }
            if (attrTemplate[0].pValue)
                UA_free(attrTemplate[0].pValue);
        } else {
            rtnval = UA_FALSE;
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_FindObjects failed = 0x%.8lx", (long unsigned int)rv);
        }
    } while( rv == UA_STATUSCODE_GOOD && foundCount > 0 && !rtnval);
    return rtnval;
}

static UA_StatusCode
decrypt_data(unsigned long slotNum, unsigned char *pin, char *label, UA_ByteString *in_data,
                   UA_ByteString **decrypted_data, UA_ByteString * iv_data, uint64_t orginal_data_length) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_ByteString *out_data = *decrypted_data;
    unsigned long *pSlotList = NULL;
    unsigned long slotID = 0;
    unsigned long int ulSlotCount = 0;
    unsigned long hSession;
    CK_SESSION_INFO sessionInfo;
    unsigned long hObject = 0;

    unsigned char *ptr_encrypted_data;
    unsigned long encrypted_data_length = 0;
    unsigned char *data_clear;
    unsigned long clear_data_length = 0;
    unsigned long declen = 16;
    unsigned char iv[iv_data->length];

    UA_Boolean key_object_found = UA_FALSE;
    CK_OBJECT_CLASS key_class = CKO_SECRET_KEY;
    CK_KEY_TYPE key_type = CKK_AES;
    CK_ATTRIBUTE attrTemplate[] = {
        {CKA_CLASS, &key_class, sizeof(key_class)},
        {CKA_KEY_TYPE, &key_type, sizeof(key_type)},
        {CKA_LABEL, (void *)label, strlen(label)}
    };

    if (iv_data && iv_data->length > 0 && iv_data->data) {
            rv = (UA_StatusCode)C_Initialize(NULL_PTR);
            if (rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                             "C_Initialize failed = 0x%.8lX", (long unsigned int)rv);
                goto cleanup;
            }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "The initializtion vector is not valid");
        goto cleanup;
    }

    rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, NULL, &ulSlotCount);
    if ((rv == UA_STATUSCODE_GOOD) && (ulSlotCount > 0)) {
        pSlotList = (unsigned long*)UA_malloc(ulSlotCount * sizeof (unsigned long));
        if (pSlotList == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "System error: Unable to allocate memory");
            goto cleanup;
        }

        rv = (UA_StatusCode)C_GetSlotList(CK_TRUE, pSlotList, &ulSlotCount);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "GetSlotList failed: Unable to get slot list = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "GetSlotList failed: Unable to get slot count = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    for (unsigned long int i = 0; i < ulSlotCount; i++) {
        slotID = pSlotList[i];
        if (slotID == slotNum) {
            CK_TOKEN_INFO token_info;
            rv = (UA_StatusCode)C_GetTokenInfo(slotID, &token_info);
            if (rv != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                             "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                             "C_GetTokenInfo failed = 0x%.8lX", (long unsigned int)rv);
                goto cleanup;
            }
            break;
        }
    }

    rv = (UA_StatusCode)C_OpenSession(slotID, CKF_SERIAL_SESSION | CKF_RW_SESSION, (CK_VOID_PTR) NULL, NULL, &hSession);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_OpenSession failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    rv = (UA_StatusCode)C_GetSessionInfo(hSession, &sessionInfo);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_GetSessionInfo failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    if(sessionInfo.state != CKS_RW_USER_FUNCTIONS) {
        /* Logs a user into a token */
        rv = (UA_StatusCode)C_Login(hSession, CKU_USER, pin, strlen((const char *)pin));
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_Login failed = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
    }

    rv = (UA_StatusCode)C_FindObjectsInit(hSession, attrTemplate, sizeof(attrTemplate)/sizeof (CK_ATTRIBUTE));
    if (rv == UA_STATUSCODE_GOOD) {
        key_object_found = find_object_by_label(hSession, label, &hObject);
        if (key_object_found == UA_FALSE){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Error: key object not found");
            goto cleanup;
        }

        rv = (UA_StatusCode)C_FindObjectsFinal(hSession);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_FindObjectsFinal failed = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_FindObjectsInit failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    for (size_t i=0; i < iv_data->length; i++) {
        iv[i] = *((CK_BYTE *)(iv_data->data + i));
    }

    CK_MECHANISM mechanism = {CKM_AES_CBC, iv, sizeof(iv)};
    encrypted_data_length = in_data->length;
    clear_data_length = encrypted_data_length;
    ptr_encrypted_data = (CK_BYTE *)(UA_malloc(encrypted_data_length * sizeof(CK_BYTE)));
    memset(ptr_encrypted_data, 0, encrypted_data_length);
    memcpy(ptr_encrypted_data, (CK_BYTE *)(in_data->data), in_data->length);

    rv = (UA_StatusCode)C_DecryptInit(hSession, &mechanism, hObject);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_DecryptInit failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    data_clear = (CK_BYTE *)UA_malloc(clear_data_length * sizeof(CK_BYTE));
    memset(data_clear, 0, clear_data_length);
    unsigned long part_number = 0;
    while (rv == UA_STATUSCODE_GOOD && part_number * 16 < clear_data_length - 16) {
        rv = (UA_StatusCode)C_DecryptUpdate(hSession, ptr_encrypted_data + part_number * 16,
                                            16, &data_clear[part_number*16], &declen);
        if (rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                         "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                         "C_Decryptupdate failed = 0x%.8lX", (long unsigned int)rv);
            goto cleanup;
        }
        part_number++;
    }

    rv = (UA_StatusCode)C_DecryptFinal(hSession, &data_clear[part_number *16], &declen);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Error from tpm2_pkcs11 - refer https://github.com/tpm2-software/tpm2-pkcs11/blob/master/src/pkcs11.h "
                     "C_DecryptFinal failed = 0x%.8lX", (long unsigned int)rv);
        goto cleanup;
    }

    if (orginal_data_length > clear_data_length)
        orginal_data_length = 0;
    if (out_data->data) {
        UA_free(out_data->data);
        out_data->length = 0;
    }
    out_data->data = (UA_Byte *)UA_malloc(orginal_data_length);
    memcpy(out_data->data, data_clear, orginal_data_length);
    out_data->length = orginal_data_length;

    C_Logout(hSession);
    C_CloseSession(hSession);
    C_Finalize(NULL);
cleanup:
    if (data_clear)
        UA_free(data_clear);
    if (pSlotList)
        UA_free(pSlotList);
    if (ptr_encrypted_data)
        UA_free(ptr_encrypted_data);
    return rv;
}

static UA_StatusCode
decrypt(unsigned long slotNum, unsigned char *pin, char *label,
                      UA_ByteString *in_data, UA_ByteString **decrypted_data) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    /* For decrypt
       Calculate the HMAC of the output without the 32 byte (256 bit) md_value
       Check the calcualted md with the md from the input
       if they match continue to decrypt the input */
    unsigned char *md_value;
    unsigned int md_len;
    unsigned int expected_md_len = 32;
    UA_ByteString *enc_data = UA_ByteString_new();
    UA_ByteString *iv_data = UA_ByteString_new();

    /* Get the HMAC */
    get_MAC(in_data->data, in_data->length - expected_md_len, &md_value, &md_len);
    if (memcmp(in_data->data + in_data->length - expected_md_len, md_value, md_len) == 0) {
        /* Get the iv that was appended to the encrypted data
           find the iv. It was packed in the first 16 bytes of the last 56 in the file */
        UA_ByteString_allocBuffer(iv_data, 16);
        uint8_t * ptr_in_data = (uint8_t *)(in_data->data);
        memcpy(iv_data->data, ptr_in_data + in_data->length - sizeof(uint64_t) - expected_md_len - iv_data->length, iv_data->length);

        /* Find the data length in output */
        uint64_t clear_data_length;
        clear_data_length = *((uint64_t *)(ptr_in_data + in_data->length - sizeof(uint64_t) - expected_md_len));

        /* Remove the extra 56 bytes that were added at the end of the encrypted data */
        UA_ByteString_allocBuffer(enc_data, in_data->length - sizeof(uint64_t) - expected_md_len - iv_data->length);
        memcpy(enc_data->data, ptr_in_data, enc_data->length);

        rv = (UA_StatusCode)decrypt_data(slotNum, pin, label, enc_data, decrypted_data, iv_data, clear_data_length);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "decrypt_data failed");
        }
        UA_ByteString_delete(iv_data);
        UA_ByteString_delete(enc_data);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "HMAC does not match");
        return EXIT_FAILURE;
    }
    if(md_value) OPENSSL_free(md_value);
    return rv;
}

static void
usage(char *progname) {
    printf("usage: %s [uri] [device] [encryptionKey] [signingkey] [slotId] [userPin] [keyLabel]\n", progname);
}

int main(int argc, char **argv) {
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    UA_ByteString encrypt_in_data = UA_BYTESTRING_NULL;
    UA_ByteString sign_in_data = UA_BYTESTRING_NULL;
    UA_ByteString *encrypt_out_data = NULL;
    UA_ByteString *sign_out_data = NULL;
    char *encryptionKeyFile = NULL;
    unsigned char *userpin = NULL;
    char *signingKeyFile = NULL;
    char *keyLabel = NULL;
    unsigned long slotId = 0;

    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc != 8) {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                             "Error: Provide uri, interface name, encryptionKey, signingkey, slotId, userpin, and keyLabel");
                return EXIT_FAILURE;
            }
        } else {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Error: unknown URI");
            return EXIT_FAILURE;
        }
        networkAddressUrl.url = UA_STRING(argv[1]);
        networkAddressUrl.networkInterface = UA_STRING(argv[2]);
        encryptionKeyFile = argv[3];
        signingKeyFile = argv[4];
        slotId = (unsigned long)atoi(argv[5]);
        userpin = (unsigned char*)argv[6];
        keyLabel = argv[7];
    }
    else {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    encrypt_in_data = loadFile(encryptionKeyFile);
    if(encrypt_in_data.length == 0) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Unable to load encryption file %s", encryptionKeyFile);
        return EXIT_FAILURE;
    }

    sign_in_data = loadFile(signingKeyFile);
    if(sign_in_data.length == 0) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Unable to load signing file %s", signingKeyFile);
        return EXIT_FAILURE;
    }

    encrypt_out_data = (UA_ByteString *)UA_malloc(sizeof(UA_ByteString));
    encrypt_out_data->data = NULL;
    encrypt_out_data->length = 0;

    sign_out_data = (UA_ByteString *)UA_malloc(sizeof(UA_ByteString));
    sign_out_data->data = NULL;
    sign_out_data->length = 0;

    rv = decrypt(slotId, userpin, keyLabel, &encrypt_in_data, &encrypt_out_data);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Decrypt failed for encryption key");
        return EXIT_FAILURE;
    }
    rv = decrypt(slotId, userpin, keyLabel, &sign_in_data, &sign_out_data);
    if (rv != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SECURITYPOLICY, "Decrypt failed for signing key");
        return EXIT_FAILURE;
    }

    signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = *(UA_Byte*)sign_out_data->data;
    encryptingKey[UA_AES128CTR_KEY_LENGTH] = *(UA_Byte*)encrypt_out_data->data;

    UA_ByteString_clear(&encrypt_in_data);
    UA_ByteString_clear(&sign_in_data);
    if (sign_out_data) {
        if (sign_out_data->data)
            UA_free(sign_out_data->data);
        UA_free(sign_out_data);
    }
    if (encrypt_out_data) {
        if (encrypt_out_data->data)
            UA_free(encrypt_out_data->data);
        UA_free(encrypt_out_data);
    }

    return run(&transportProfile, &networkAddressUrl);
}
