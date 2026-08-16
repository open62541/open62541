/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    UA_ExtensionObject extensionObject;
    UA_Variant variant;
    size_t extensionObjectsSize;
    UA_ExtensionObject *extensionObjects;
} JsonInteropEnvelope;

static UA_DataTypeMember envelopeMembers[3] = {
    {UA_TYPENAME("ExtensionObject") &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
     0, false, false},
    {UA_TYPENAME("Variant") &UA_TYPES[UA_TYPES_VARIANT],
     offsetof(JsonInteropEnvelope, variant) -
         offsetof(JsonInteropEnvelope, extensionObject) -
         sizeof(UA_ExtensionObject),
     false, false},
    {UA_TYPENAME("ExtensionObjects") &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
     offsetof(JsonInteropEnvelope, extensionObjectsSize) -
         offsetof(JsonInteropEnvelope, variant) - sizeof(UA_Variant),
     true, false}
};

static const UA_DataType envelopeType = {
    UA_TYPENAME("JsonInteropEnvelope")
    {1, UA_NODEIDTYPE_NUMERIC, {5100}},
    {1, UA_NODEIDTYPE_NUMERIC, {6100}},
    {1, UA_NODEIDTYPE_NUMERIC, {7100}},
    sizeof(JsonInteropEnvelope), UA_DATATYPEKIND_STRUCTURE,
    false, false, 3, envelopeMembers
};

static UA_StatusCode
writeFile(const char *path, const UA_ByteString *content) {
    FILE *file = fopen(path, "wb");
    if(!file)
        return UA_STATUSCODE_BADINTERNALERROR;
    size_t written = fwrite(content->data, 1, content->length, file);
    int closeResult = fclose(file);
    return (written == content->length && closeResult == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
readFile(const char *path, UA_ByteString *content) {
    FILE *file = fopen(path, "rb");
    if(!file)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    long length = ftell(file);
    if(length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode res = UA_ByteString_allocBuffer(content, (size_t)length);
    if(res == UA_STATUSCODE_GOOD &&
       fread(content->data, 1, content->length, file) != content->length)
        res = UA_STATUSCODE_BADINTERNALERROR;
    if(fclose(file) != 0)
        res = UA_STATUSCODE_BADINTERNALERROR;
    if(res != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(content);
    return res;
}

static UA_Boolean
isExpectedReadValue(const UA_ReadValueId *readValue) {
    UA_NodeId expected = UA_NS0ID(SERVER_SERVERSTATUS);
    return readValue &&
        UA_NodeId_equal(&readValue->nodeId, &expected) &&
        readValue->attributeId == UA_ATTRIBUTEID_VALUE;
}

static UA_Boolean
isExpectedReadValueId(const UA_ExtensionObject *value) {
    return UA_ExtensionObject_hasDecodedType(
        value, &UA_TYPES[UA_TYPES_READVALUEID]) &&
        isExpectedReadValue(
            (const UA_ReadValueId*)value->content.decoded.data);
}

static UA_StatusCode
encodeEnvelope(const char *path) {
    UA_ReadValueId readValue;
    UA_ReadValueId_init(&readValue);
    readValue.nodeId = UA_NS0ID(SERVER_SERVERSTATUS);
    readValue.attributeId = UA_ATTRIBUTEID_VALUE;

    JsonInteropEnvelope envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.extensionObject.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    envelope.extensionObject.content.decoded.type =
        &UA_TYPES[UA_TYPES_READVALUEID];
    envelope.extensionObject.content.decoded.data = &readValue;
    UA_Variant_setScalar(&envelope.variant, &envelope.extensionObject,
                         &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    UA_ExtensionObject values[2] = {
        envelope.extensionObject, envelope.extensionObject
    };
    envelope.extensionObjectsSize = 2;
    envelope.extensionObjects = values;

    UA_EncodeJsonOptions options = {0};
    options.useCompactEncoding = true;
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&envelope, &envelopeType,
                                      &encoded, &options);
    if(res == UA_STATUSCODE_GOOD)
        res = writeFile(path, &encoded);
    UA_ByteString_clear(&encoded);
    return res;
}

static UA_StatusCode
decodeEnvelope(const char *path) {
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = readFile(path, &encoded);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    JsonInteropEnvelope envelope;
    memset(&envelope, 0, sizeof(envelope));
    res = UA_decodeJson(&encoded, &envelope, &envelopeType, NULL);
    UA_ByteString_clear(&encoded);
    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Could not decode Compact JSON envelope: 0x%08x\n",
                (unsigned)res);
        return res;
    }

    UA_Boolean valid = isExpectedReadValueId(&envelope.extensionObject) &&
        UA_Variant_isScalar(&envelope.variant) &&
        envelope.variant.type == &UA_TYPES[UA_TYPES_READVALUEID] &&
        isExpectedReadValue((const UA_ReadValueId*)envelope.variant.data) &&
        envelope.extensionObjectsSize == 2 &&
        isExpectedReadValueId(&envelope.extensionObjects[0]) &&
        isExpectedReadValueId(&envelope.extensionObjects[1]);
    UA_clear(&envelope, &envelopeType);
    if(!valid)
        fprintf(stderr, "Decoded Compact JSON envelope has unexpected values\n");
    return valid ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADDECODINGERROR;
}

int
main(int argc, char **argv) {
    if(argc != 3 ||
       (strcmp(argv[1], "encode") != 0 && strcmp(argv[1], "decode") != 0)) {
        fprintf(stderr, "Usage: %s encode|decode <json-file>\n", argv[0]);
        return 2;
    }

    UA_StatusCode res = (strcmp(argv[1], "encode") == 0) ?
        encodeEnvelope(argv[2]) : decodeEnvelope(argv[2]);
    if(res != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Compact JSON interop failed with 0x%08x\n",
                (unsigned)res);
        return 1;
    }
    return 0;
}
