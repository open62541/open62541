/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef OPEN62541_PUBSUB_CONFIG_TEST_HELPERS_H
#define OPEN62541_PUBSUB_CONFIG_TEST_HELPERS_H

#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include <string.h>

/* Content of the PubSubConfiguration file (Part 14 v1.05 9.1.3.7.1): an
 * ExtensionObject with a UABinaryFileDataType that has the configuration as
 * body. The configuration can reference external memory. */
static UA_INLINE UA_StatusCode
UA_PubSubTest_encodeConfigFile(const UA_PubSubConfiguration2DataType *config,
                               UA_ByteString *file) {
    UA_UABinaryFileDataType binFile;
    UA_UABinaryFileDataType_init(&binFile);
    UA_Variant_setScalar(&binFile.body, (void*)(uintptr_t)config,
                         &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE]);
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    eo.content.decoded.type = &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE];
    eo.content.decoded.data = &binFile;
    UA_ByteString_init(file);
    return UA_encodeBinary(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], file, NULL);
}

/* Deep copy of the configuration in the file content. Clean up with
 * UA_PubSubConfiguration2DataType_clear. */
static UA_INLINE UA_StatusCode
UA_PubSubTest_decodeConfigFile(const UA_ByteString *file,
                               UA_PubSubConfiguration2DataType *config) {
    UA_PubSubConfiguration2DataType_init(config);
    UA_ExtensionObject eo;
    UA_StatusCode res =
        UA_decodeBinary(file, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_UABinaryFileDataType *binFile = (UA_UABinaryFileDataType*)eo.content.decoded.data;
    if(eo.encoding != UA_EXTENSIONOBJECT_DECODED ||
       eo.content.decoded.type != &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE] ||
       !UA_Variant_hasScalarType(&binFile->body,
                                 &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE])) {
        UA_ExtensionObject_clear(&eo);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    res = UA_PubSubConfiguration2DataType_copy(
        (const UA_PubSubConfiguration2DataType*)binFile->body.data, config);
    UA_ExtensionObject_clear(&eo);
    return res;
}

/* Deep copy of the running configuration (the file content decoded) */
static UA_INLINE UA_StatusCode
UA_PubSubTest_readConfig(UA_Server *server,
                         UA_PubSubConfiguration2DataType *config) {
    UA_ByteString file;
    UA_StatusCode res = UA_Server_readPubSubConfiguration(server, &file);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubConfiguration2DataType_init(config);
        return res;
    }
    res = UA_PubSubTest_decodeConfigFile(&file, config);
    UA_ByteString_clear(&file);
    return res;
}

/* Encode the configuration and apply it with the given references */
static UA_INLINE UA_StatusCode
UA_PubSubTest_updateConfig(UA_Server *server,
                           const UA_PubSubConfiguration2DataType *config,
                           size_t referencesSize,
                           const UA_PubSubConfigurationRefDataType *references,
                           UA_Boolean requireCompleteUpdate,
                           UA_PubSubConfigurationUpdateResult *result) {
    UA_ByteString file;
    UA_StatusCode res = UA_PubSubTest_encodeConfigFile(config, &file);
    if(res != UA_STATUSCODE_GOOD) {
        memset(result, 0, sizeof(UA_PubSubConfigurationUpdateResult));
        return res;
    }
    res = UA_Server_updatePubSubConfiguration(server, &file, referencesSize,
                                              references, requireCompleteUpdate,
                                              result);
    UA_ByteString_clear(&file);
    return res;
}

/* Apply all elements of a file in one complete update: the references for
 * Add load the file, with replace the elements are removed and added again.
 * Returns the first failing code. */
static UA_INLINE UA_StatusCode
UA_PubSubTest_applyConfigFile(UA_Server *server, const UA_ByteString *file,
                              UA_Boolean replace) {
    size_t removeSize = 0, addSize = 0;
    UA_PubSubConfigurationRefDataType *removeRefs = NULL, *addRefs = NULL;
    UA_StatusCode res = UA_PubSubConfiguration_createReferences(
        file, UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD, &addSize, &addRefs);
    if(res == UA_STATUSCODE_GOOD && replace)
        res = UA_PubSubConfiguration_createReferences(
            file, UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE, &removeSize, &removeRefs);
    size_t refsSize = removeSize + addSize;
    UA_PubSubConfigurationRefDataType *refs = NULL;
    if(res == UA_STATUSCODE_GOOD && refsSize > 0) {
        refs = (UA_PubSubConfigurationRefDataType*)
            UA_Array_new(refsSize, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);
        if(!refs) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
        } else {
            if(removeSize > 0)
                memcpy(refs, removeRefs,
                       removeSize * sizeof(UA_PubSubConfigurationRefDataType));
            memcpy(&refs[removeSize], addRefs,
                   addSize * sizeof(UA_PubSubConfigurationRefDataType));
        }
    }
    if(res == UA_STATUSCODE_GOOD) {
        UA_PubSubConfigurationUpdateResult result;
        res = UA_Server_updatePubSubConfiguration(server, file, refsSize, refs,
                                                  true, &result);
        for(size_t i = 0; res == UA_STATUSCODE_GOOD &&
                i < result.referencesResultsSize; i++)
            res = result.referencesResults[i];
        UA_PubSubConfigurationUpdateResult_clear(&result);
    }
    UA_Array_delete(refs, refsSize, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);
    UA_Array_delete(removeRefs, removeSize,
                    &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);
    UA_Array_delete(addRefs, addSize,
                    &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);
    return res;
}

#endif /* OPEN62541_PUBSUB_CONFIG_TEST_HELPERS_H */
