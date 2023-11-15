#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "server/ua_server_internal.h"

#include <jni.h>
#include <signal.h>
#include <stdlib.h>

#include "jni_pubsub_PubSubMain.h"
#include "jni_pubsub_PubSubConnect.h"

UA_Server *server = NULL;

#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

#define UA_AES256CTR_SIGNING_KEY_LENGTH 32
#define UA_AES256CTR_KEY_LENGTH 32
#define UA_AES256CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

UA_Byte signingKey256[UA_AES256CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey256[UA_AES256CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce256[UA_AES256CTR_KEYNONCE_LENGTH] = {0};

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;
static int nodeId = 50000;

typedef struct {
    char *name;
    char *publisherId;
    int datasetwriterId;
    int writergroupId;
    int typeFlag;
} DataSetReaderConfigSubscribe;

int connections = -1;
typedef struct {
    int number;
    UA_NodeId identifier;
} ConnectionID;

ConnectionID connectionIDs[100];

static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData, jobjectArray metadata,
                        JNIEnv *env);

/* Add new connection to the server */
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) || (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING((char *)"UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();

    retval |=
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All
 * ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the
 * connection
 * is removed. All network message related filters are only available in
 * the
 * DataSetReader. */
/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server, char *securityMode, char *securityPolicy) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING((char *)"ReaderGroup1");

    if(strcmp(securityMode, "SignAndEncrypt") == 0) {
        /* Encryption settings */
        UA_ServerConfig *config = UA_Server_getConfig(server);
        readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Message Security Mode: SignAndEncrypt");
    } else if(strcmp(securityMode, "Sign") == 0) {
        UA_ServerConfig *config = UA_Server_getConfig(server);
        readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGN;
        readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Message Security Mode: Sign");
    }

    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);

    /* Add the encryption key informaton */

    if(strcmp(securityPolicy, "Aes128Ctr") == 0) {
        UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
        UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
        UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};
        // TODO security token not necessary for readergroup (extracted from
        // security-header)
        UA_Server_setReaderGroupEncryptionKeys(server, readerGroupIdentifier, 1, sk, ek,
                                               kn);
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "AES128 Security Policy configured for PubSub");

    } else {
        UA_ByteString sk = {UA_AES256CTR_SIGNING_KEY_LENGTH, signingKey256};
        UA_ByteString ek = {UA_AES256CTR_KEY_LENGTH, encryptingKey256};
        UA_ByteString kn = {UA_AES256CTR_KEYNONCE_LENGTH, keyNonce256};
        // TODO security token not necessary for readergroup (extracted from
        // security-header)
        UA_StatusCode rvencrypt = UA_Server_setReaderGroupEncryptionKeys(
            server, readerGroupIdentifier, 1, sk, ek, kn);
        if(rvencrypt == UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "AES256 Security Policy configured for PubSub");
    }

    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);
    return retval;
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the
 * DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the
 * configuration necessary to receive and process DataSetMessages
 * on the Subscriber
 * side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within
 * a ReaderGroup. */
/* Add DataSetReader to the ReaderGroup */
static UA_StatusCode
addDataSetReader(UA_Server *server, DataSetReaderConfigSubscribe dsrc,
                 jobjectArray metadata, JNIEnv *env) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING(dsrc.name);
    /* Parameters to filter which DataSetMessage has to be processed
     * by the
     * DataSetReader */
    /* The following parameters are used to show that the data published by
     *
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the
     * information model */
    if(dsrc.typeFlag == 1) {
        UA_UInt16 publisherIdentifier;
        sscanf(dsrc.publisherId, "%d", &publisherIdentifier);
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
        readerConfig.publisherId.data = &publisherIdentifier;
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "PUBLISHER ID: %d",
                       *((UA_UInt16 *)readerConfig.publisherId.data));
    } else {
        UA_String publisherIdentifier = UA_STRING(dsrc.publisherId);
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_STRING];
        readerConfig.publisherId.data = &publisherIdentifier;
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "PUBLISHER ID: %s",
                       ((UA_String *)readerConfig.publisherId.data)->data);
    }
    readerConfig.writerGroupId = dsrc.writergroupId;
    readerConfig.dataSetWriterId = dsrc.datasetwriterId;

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData, metadata, env);
    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    return retval;
}

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data
 * type.
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables(UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING((char *)"en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName =
            UA_LOCALIZEDTEXT((char *)"en-US", (char *)"Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME(1, (char *)"Subscribed Variables");
    }

    UA_Server_addObjectNode(
        server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), folderBrowseName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

    /**
     * **TargetVariables**
     *
     * The SubscribedDataSet option
     * TargetVariables defines a list of Variable mappings
     * between received DataSet
     * fields and target Variables in the Subscriber
     * AddressSpace. The values
     * subscribed from the Publisher are updated in the value
     * field of these
     * variables */
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)UA_calloc(
        readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING((char *)"en-US");
        vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retval |= UA_Server_addVariableNode(
            server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + nodeId), folderId,
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields[i].name.data),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newNode);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }
    nodeId = nodeId + readerConfig.dataSetMetaData.fieldsSize;
    retval = UA_Server_DataSetReader_createTargetVariables(
        server, dataSetReaderId, readerConfig.dataSetMetaData.fieldsSize, targetVars);
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    UA_free(targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

/**
 * **DataSetMetaData**
 *
 * The DataSetMetaData describes the content of a DataSet.
 * It provides the information
 * necessary to decode DataSetMessages on the Subscriber
 * side. DataSetMessages received
 * from the Publisher are decoded into DataSet and each
 * field is updated in the Subscriber
 * based on datatype match of TargetVariable fields
 * of Subscriber and
 * PublishedDataSetFields of Publisher */
/* Define MetaData for TargetVariables */
static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData, jobjectArray metadata,
                        JNIEnv *env) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING((char *)"DataSet 1");

    /* Static definition of number of fields size to create different
    /*
     * targetVariables of distinct datatype*/

    pMetaData->fieldsSize = (*env)->GetArrayLength(env, metadata);
    pMetaData->fields = (UA_FieldMetaData *)UA_Array_new(
        pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    for(int i = 0; i < pMetaData->fieldsSize; i++) {
        char *receivedType = (char *)(*env)->GetStringUTFChars(
            env, (jstring)(*env)->GetObjectArrayElement(env, metadata, i), NULL);
        if(strcmp(receivedType, "String") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_STRING].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_STRING;
            pMetaData->fields[i].name = UA_STRING((char *)"String");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Int32") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_INT32;
            pMetaData->fields[i].name = UA_STRING((char *)"Int32");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Float") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_FLOAT].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_FLOAT;
            pMetaData->fields[i].name = UA_STRING((char *)"Float");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Double") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_DOUBLE].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_DOUBLE;
            pMetaData->fields[i].name = UA_STRING((char *)"Double");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Int16") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT16].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_INT16;
            pMetaData->fields[i].name = UA_STRING((char *)"Int16");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Boolean") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_BOOLEAN;
            pMetaData->fields[i].name = UA_STRING((char *)"Boolean");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Int64") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_INT64;
            pMetaData->fields[i].name = UA_STRING((char *)"Int64");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Integer") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_INT32;
            pMetaData->fields[i].name = UA_STRING((char *)"Integer");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Datetime") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_DATETIME;
            pMetaData->fields[i].name = UA_STRING((char *)"Datetime");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Byte") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_BYTE].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_BYTE;
            pMetaData->fields[i].name = UA_STRING((char *)"Byte");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "SByte") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_SBYTE].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_SBYTE;
            pMetaData->fields[i].name = UA_STRING((char *)"SByte");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "ByteString") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_BYTESTRING].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_BYTESTRING;
            pMetaData->fields[i].name = UA_STRING((char *)"ByteString");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "GUID") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_GUID].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_GUID;
            pMetaData->fields[i].name = UA_STRING((char *)"GUID");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "UInt16") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT16].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_UINT16;
            pMetaData->fields[i].name = UA_STRING((char *)"UInt16");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "UInt32") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_UINT32;
            pMetaData->fields[i].name = UA_STRING((char *)"UInt32");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "UInt64") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_UINT64;
            pMetaData->fields[i].name = UA_STRING((char *)"UInt64");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "LocalizedText") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_LOCALIZEDTEXT;
            pMetaData->fields[i].name = UA_STRING((char *)"Localized Text");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "Duration") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_DURATION].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_DURATION;
            pMetaData->fields[i].name = UA_STRING((char *)"Duration");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "EnumValue") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_ENUMVALUETYPE].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_ENUMVALUETYPE;
            pMetaData->fields[i].name = UA_STRING((char *)"EnumValue");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "LocaleId") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_LOCALEID].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_LOCALEID;
            pMetaData->fields[i].name = UA_STRING((char *)"Locale Id");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "NodeId") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_NODEID].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_NODEID;
            pMetaData->fields[i].name = UA_STRING((char *)"Node Id");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "QualifiedName") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_QUALIFIEDNAME].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_QUALIFIEDNAME;
            pMetaData->fields[i].name = UA_STRING((char *)"Qualified Name");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "XMLElement") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_XMLELEMENT].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_XMLELEMENT;
            pMetaData->fields[i].name = UA_STRING((char *)"XML Element");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else if(strcmp(receivedType, "UtcTime") == 0) {
            UA_FieldMetaData_init(&pMetaData->fields[i]);
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_UTCTIME].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_UTCTIME;
            pMetaData->fields[i].name = UA_STRING((char *)"Utc Time");
            pMetaData->fields[i].valueRank = -1; /* scalar */
        } else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Type: %s, is not supported!!", receivedType);
            _Exit(0);
        }
    }
}

/**
 * Followed by the main server code, making use of the above definitions */
UA_Boolean running = true;
static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
    UA_Server_delete(server);
    _Exit(0);
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl,
    DataSetReaderConfigSubscribe dsrcs[], jobjectArray metadataArray,
    jstring securityMode, jstring policy, JNIEnv *env) {

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    if(strcmp((char *)(*env)->GetStringUTFChars(env, policy, NULL), "Aes128Ctr") == 0) {
        /* Instantiate the PubSub SecurityPolicy */
        config->pubSubConfig.securityPolicies =
            (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
        config->pubSubConfig.securityPoliciesSize = 1;
        UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies,
                                          &config->logger);
    } else if(strcmp((char *)(*env)->GetStringUTFChars(env, policy, NULL), "Aes256Ctr") ==
              0) {
        /* Instantiate the PubSub SecurityPolicy */
        config->pubSubConfig.securityPolicies =
            (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
        config->pubSubConfig.securityPoliciesSize = 1;
        UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                          &config->logger);
    }

    /* Add the PubSub network layer implementation to the server config.
     * The
     * TransportLayer is acting as factory to create new connections
     * on runtime.
     * Details about the PubSubTransportLayer can be found inside the
     *
     * tutorial_pubsub_connection */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

    /* API calls */
    /* Add PubSubConnection */
    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl);
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add ReaderGroup to the created PubSubConnection */
    retval |=
        addReaderGroup(server, (char *)(*env)->GetStringUTFChars(env, securityMode, NULL),
                       (char *)(*env)->GetStringUTFChars(env, policy, NULL));
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add DataSetReader to the created ReaderGroup */
    for(int i = 0; i < (*env)->GetArrayLength(env, metadataArray); i++) {
        jobjectArray metadata =
            (jobjectArray)(*env)->GetObjectArrayElement(env, metadataArray, i);
        DataSetReaderConfigSubscribe dsrc = dsrcs[i];
        retval |= addDataSetReader(server, dsrc, metadata, env);

        if(retval != UA_STATUSCODE_GOOD)
            return EXIT_FAILURE;
    }

    /* Add SubscribedVariables to the created DataSetReader */
    retval |= addSubscribedVariables(server, readerIdentifier);
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

//User Can use their package name in place of "jni_pubsub" and according they have to change the header file included above
JNIEXPORT void JNICALL
Java_jni_pubsub_PubSubConnect_createServer(
    JNIEnv *env, jobject javaobj, jstring serverSecurityPolicy) {
    char *serverSecurityPolicy1 =
        (char *)(*env)->GetStringUTFChars(env, serverSecurityPolicy, NULL);
    server = UA_Server_new();
    running = true;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;

    int port = (rand() % 501) + 4500;
    if(strcmp(serverSecurityPolicy1, "Secure") == 0) {
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Trying to create a certificate.");
        UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                                UA_STRING_STATIC("O=SampleOrganization"),
                                UA_STRING_STATIC("CN=Open62541Server@localhost")};
        UA_UInt32 lenSubject = 3;
        UA_String subjectAltName[2] = {
            UA_STRING_STATIC("DNS:localhost"),
            UA_STRING_STATIC("URI:urn:open62541.server.application")};
        UA_UInt32 lenSubjectAltName = 2;
        UA_StatusCode statusCertGen = UA_CreateCertificate(
            UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName, 0,
            UA_CERTIFICATEFORMAT_DER, &privateKey, &certificate);

        if(statusCertGen != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Generating Certificate failed: %s",
                        UA_StatusCode_name(statusCertGen));
            return EXIT_FAILURE;
        }

        /* Load the trustlist */
        size_t trustListSize = 0;
        UA_STACKARRAY(UA_ByteString, trustList, trustListSize + 1);

        /* Loading of an issuer list, not used in this application */
        size_t issuerListSize = 0;
        UA_ByteString *issuerList = NULL;

        /* Loading of a revocation list currently unsupported */
        UA_ByteString *revocationList = NULL;
        size_t revocationListSize = 0;

        UA_StatusCode retval1 = UA_ServerConfig_setDefaultWithSecurityPolicies(
            config, port, &certificate, &privateKey, trustList, trustListSize, issuerList,
            issuerListSize, revocationList, revocationListSize);
#else
        return EXIT_FAILURE;
#endif

    }
    ////////////////////////////////////////////////////////////
    else
        UA_ServerConfig_setMinimal(config, port, NULL);
}
// User Can use their package name in place of "jni_pubsub" and according they have to change the header file included above
JNIEXPORT void JNICALL
Java_jni_pubsub_PubSubConnect_runServer(
    JNIEnv *env, jobject javaobj) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = UA_Server_run(server, &running);
}
// User Can use their package name in place of "jni_pubsub" and according they have to change the header file included above
JNIEXPORT jint JNICALL
Java_jni_pubsub_PubSubConnect_connect(
    JNIEnv *env, jobject javaobj, jstring pubIP, jobjectArray jdsrc,
    jobjectArray metadata, jstring protocol, jstring securityMode, jstring policy) {
    jobjectArray writerGroupIds =
        (jobjectArray)(*env)->GetObjectArrayElement(env, jdsrc, 1);
    jobjectArray datasetWriterIds =
        (jobjectArray)(*env)->GetObjectArrayElement(env, jdsrc, 2);
    // DataSetReaderConfigSubscribe *dsrcs = new
    // DataSetReaderConfigSubscribe[env->GetArrayLength(publisherIds)];          //FOR
    // WINDOWS(if defined in .cpp file)
    DataSetReaderConfigSubscribe dsrcs[100];  // FOR LINUX

    char *ID = (char *)(*env)->GetStringUTFChars(
        env, (jstring)(*env)->GetObjectArrayElement(env, jdsrc, 0), NULL);

    int typeFlag = 0;  // String Type
    if(atoi(ID) != 0)
        typeFlag = 1;  // Integer type

    for(int i = 0; i < (*env)->GetArrayLength(env, datasetWriterIds); i++) {
        DataSetReaderConfigSubscribe dsrc;
        char name[18] = "Dataset Reader";
        char tempStore[4];
        sprintf(tempStore, "%d", i + 1);
        strncat(name, tempStore, strlen(tempStore));
        dsrc.name = (char *)malloc(strlen(name) + 1);
        strncpy(dsrc.name, name, strlen(name) + 1);

        dsrc.typeFlag = typeFlag;
        dsrc.publisherId = (char *)malloc(strlen(ID) + 1);
        strncpy(dsrc.publisherId, ID, strlen(ID) + 1);

        sscanf((char *)(*env)->GetStringUTFChars(
                   env, (jstring)(*env)->GetObjectArrayElement(env, writerGroupIds, i),
                   NULL),
               "%d", &dsrc.writergroupId);
        sscanf((char *)(*env)->GetStringUTFChars(
                   env, (jstring)(*env)->GetObjectArrayElement(env, datasetWriterIds, i),
                   NULL),
               "%d", &dsrc.datasetwriterId);
        dsrcs[i] = dsrc;
        // dsrc.publisherId = jobjToInt(env, publisherIds, i);
        // dsrc.writergroupId = jobjToInt(env, writerGroupIds, i);
        // dsrc.datasetwriterId = jobjToInt(env, datasetWriterIds, i);
    }

    char *publisherIP = (char *)(*env)->GetStringUTFChars(env, pubIP, NULL);
    char *protocolUsed = (char *)(*env)->GetStringUTFChars(env, protocol, NULL);
    char UADP[30] = "opc.udp://";  // FOR UBUNTU
    // char UADP = "opc.udp://";               //FOR WINDOWS(.cpp file)
    if(strcmp(protocolUsed, (char *)"UADP") == 0)
        strncat(UADP, publisherIP, strlen(publisherIP));

    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "***** Using Publisher Url: % s", UADP);
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(UADP)};
    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "***** Using transportProfileUri: "
                   "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    UA_String transportProfile = UA_STRING(
        (char *)"http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    int retVal = run(&transportProfile, &networkAddressUrl, dsrcs, metadata, securityMode,
                     policy, env);
    if(!retVal) {
        connections++;
        connectionIDs[connections].number = connections;
        connectionIDs[connections].identifier = connectionIdentifier;
        return connections;
    }
    return -1;
}
// User Can use their package name in place of "jni_pubsub" and according they have to change the header file included above
JNIEXPORT void JNICALL
Java_jni_pubsub_PubSubConnect_teardownConnection(
    JNIEnv *env, jobject javaobj, jint connectionNumber) {
    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "***** Tearing down Connection");
    UA_Server_removePubSubConnection(server, connectionIDs[connectionNumber].identifier);
    return;
}
// User Can use their package name in place of "jni_pubsub" and according they have to change the header file included above
JNIEXPORT void JNICALL
Java_jni_pubsub_PubSubConnect_stopServer(
    JNIEnv *env, jobject javaobj) {

    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "***** Tearing down Server");
    running = false;

    /*transferFlag = 1;
    processFlag = 1;
    messagesFlag = 0;
    messagesCaptured = 0;*/

    return;
}
#endif /* UA_ENABLE_PUBSUB */
