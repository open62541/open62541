/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"

#ifdef UA_ENABLE_JSON_ENCODING
# include "ua_types_encoding_json.h"
#endif

const UA_String *
UA_Http_getHeader(const UA_KeyValueMap *params, const char *name,
                  UA_Boolean *duplicate, UA_Boolean *invalid) {
    if(duplicate)
        *duplicate = false;
    if(invalid)
        *invalid = false;
    const UA_Variant *value = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "headers"));
    if(!value ||
       !UA_Variant_hasArrayType(value, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]))
        return NULL;

    UA_String expected = UA_STRING((char *)(uintptr_t)name);
    const UA_KeyValuePair *headers = (const UA_KeyValuePair *)value->data;
    const UA_String *result = NULL;
    size_t occurrences = 0;
    for(size_t i = 0; i < value->arrayLength; i++) {
        if(!UA_String_equal_ignorecase(&headers[i].key.name, &expected))
            continue;
        occurrences++;
        if(duplicate && occurrences > 1)
            *duplicate = true;
        if(!UA_Variant_hasScalarType(&headers[i].value,
                                     &UA_TYPES[UA_TYPES_STRING])) {
            if(invalid)
                *invalid = true;
            continue;
        }
        if(!result)
            result = (const UA_String *)headers[i].value.data;
    }
    return result;
}

UA_Boolean
UA_Http_mediaTypeEquals(const UA_String *header, const UA_String *mediaType) {
    if(!header || !mediaType)
        return false;
    size_t begin = 0;
    while(begin < header->length &&
          (header->data[begin] == ' ' || header->data[begin] == '\t'))
        begin++;
    size_t end = begin;
    while(end < header->length && header->data[end] != ';')
        end++;
    while(end > begin &&
          (header->data[end - 1] == ' ' || header->data[end - 1] == '\t'))
        end--;
    UA_String value = {end - begin, &header->data[begin]};
    return UA_String_equal_ignorecase(&value, mediaType);
}

UA_Boolean
UA_Http_headerValueEquals(const UA_String *header, const char *expected) {
    if(!header)
        return false;
    size_t begin = 0;
    while(begin < header->length &&
          (header->data[begin] == ' ' || header->data[begin] == '\t'))
        begin++;
    size_t end = header->length;
    while(end > begin &&
          (header->data[end - 1] == ' ' || header->data[end - 1] == '\t'))
        end--;
    UA_String value = {end - begin, &header->data[begin]};
    UA_String expectedValue = UA_STRING((char *)(uintptr_t)expected);
    return UA_String_equal_ignorecase(&value, &expectedValue);
}

UA_Boolean
UA_Http_contentTypeMatchesEncoding(const UA_String *contentType,
                                   UA_SecureChannelEncoding encoding) {
#ifdef UA_ENABLE_JSON_ENCODING
    if(encoding == UA_SECURECHANNEL_ENCODING_JSON)
        return UA_Http_mediaTypeEquals(contentType,
                                       &UA_HTTP_CONTENTTYPE_JSON);
#endif
    return encoding == UA_SECURECHANNEL_ENCODING_BINARY &&
        (UA_Http_mediaTypeEquals(contentType, &UA_HTTP_CONTENTTYPE_BINARY) ||
         UA_Http_mediaTypeEquals(contentType,
                                 &UA_HTTP_CONTENTTYPE_BINARY_LEGACY));
}

UA_StatusCode
UA_Http_sendResponse(UA_ConnectionManager *cm, uintptr_t connectionId,
                     UA_UInt16 status, const UA_String *contentType,
                     const UA_String *contentCodingPolicy,
                     UA_ByteString *body) {
    if(!cm || connectionId == 0)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    UA_KeyValuePair params[3] = {0};
    params[0].key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&params[0].value, &status, &UA_TYPES[UA_TYPES_UINT16]);
    size_t paramsSize = 1;
    UA_KeyValuePair header = {0};
    if(contentType) {
        header.key = UA_QUALIFIEDNAME(0, "content-type");
        UA_Variant_setScalar(&header.value, (void *)(uintptr_t)contentType,
                             &UA_TYPES[UA_TYPES_STRING]);
        params[1].key = UA_QUALIFIEDNAME(0, "headers");
        UA_Variant_setArray(&params[1].value, &header, 1,
                            &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
        paramsSize++;
    }
    if(contentCodingPolicy) {
        params[paramsSize].key =
            UA_QUALIFIEDNAME(0, "content-coding-policy");
        UA_Variant_setScalar(&params[paramsSize].value,
                             (void *)(uintptr_t)contentCodingPolicy,
                             &UA_TYPES[UA_TYPES_STRING]);
        paramsSize++;
    }
    UA_KeyValueMap map = {paramsSize, params};
    return cm->sendWithConnection(cm, connectionId, &map, body);
}

const UA_String UA_HTTP_CONTENTTYPE_BINARY =
    UA_STRING_STATIC("application/octet-stream");
const UA_String UA_HTTP_CONTENTTYPE_BINARY_LEGACY =
    UA_STRING_STATIC("application/opcua+uabinary");
const UA_String UA_HTTP_PROFILE_HTTPS_BINARY = UA_STRING_STATIC(
    "http://opcfoundation.org/UA-Profile/Transport/https-uabinary");
const UA_String UA_HTTP_PROFILE_HTTP_BINARY = UA_STRING_STATIC(
    "http://open62541.org/UA-Profile/Transport/http-uabinary");
#ifdef UA_ENABLE_JSON_ENCODING
const UA_String UA_HTTP_CONTENTTYPE_JSON =
    UA_STRING_STATIC("application/json");
const UA_String UA_HTTP_PROFILE_HTTPS_JSON = UA_STRING_STATIC(
    "http://opcfoundation.org/UA-Profile/Transport/https-uajson");
const UA_String UA_HTTP_PROFILE_HTTP_JSON = UA_STRING_STATIC(
    "http://open62541.org/UA-Profile/Transport/http-uajson");
#endif

static UA_StatusCode
encodeHttpBinaryBody(UA_SecureChannel *channel, const void *payload,
                     const UA_DataType *payloadType, UA_ByteString *body) {
    UA_EncodeBinaryOptions options;
    memset(&options, 0, sizeof(options));
    options.namespaceMapping = channel->namespaceMapping;
    size_t typeIdSize = UA_calcSizeBinary(
        &payloadType->binaryEncodingId, &UA_TYPES[UA_TYPES_NODEID], &options);
    size_t payloadSize = UA_calcSizeBinary(payload, payloadType, &options);
    if(typeIdSize > SIZE_MAX - payloadSize)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    size_t messageSize = typeIdSize + payloadSize;
    if(messageSize == 0)
        return UA_STATUSCODE_BADENCODINGERROR;
    UA_StatusCode res = UA_ByteString_allocBuffer(body, messageSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_Byte *pos = body->data;
    const UA_Byte *end = &body->data[body->length];
    res = UA_encodeBinaryInternal(&payloadType->binaryEncodingId,
                                  &UA_TYPES[UA_TYPES_NODEID],
                                  &pos, &end, &options, NULL, NULL);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_encodeBinaryInternal(payload, payloadType, &pos, &end,
                                      &options, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_ByteString_clear(body);
    return res;
}

#ifdef UA_ENABLE_JSON_ENCODING
static UA_StatusCode
encodeHttpJsonBody(UA_SecureChannel *channel, const void *payload,
                   const UA_DataType *payloadType, UA_ByteString *body) {
    UA_ExtensionObject envelope;
    UA_ExtensionObject_setValueNoDelete(&envelope,
                                        (void *)(uintptr_t)payload,
                                        payloadType);
    UA_EncodeJsonOptions options;
    memset(&options, 0, sizeof(options));
    options.namespaceMapping = channel->namespaceMapping;
    return UA_encodeJson(&envelope, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], body,
                         &options);
}
#endif

static UA_StatusCode
encodeHttpBody(UA_SecureChannel *channel, const void *payload,
               const UA_DataType *payloadType, UA_ByteString *body,
               const UA_String **contentType) {
#ifdef UA_ENABLE_JSON_ENCODING
    if(channel->encoding == UA_SECURECHANNEL_ENCODING_JSON) {
        *contentType = &UA_HTTP_CONTENTTYPE_JSON;
        return encodeHttpJsonBody(channel, payload, payloadType, body);
    }
#endif
    if(channel->encoding != UA_SECURECHANNEL_ENCODING_BINARY)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    *contentType = &UA_HTTP_CONTENTTYPE_BINARY;
    return encodeHttpBinaryBody(channel, payload, payloadType, body);
}

UA_StatusCode
UA_SecureChannel_sendHttpResponse(UA_SecureChannel *channel,
                                  uintptr_t connectionId, void *payload,
                                  const UA_DataType *payloadType) {
    if(!channel || !payload || !payloadType || connectionId == 0 ||
       (channel->state != UA_SECURECHANNELSTATE_OPEN &&
        channel->state != UA_SECURECHANNELSTATE_CLOSING) ||
       !channel->connectionManager)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    UA_ByteString body = UA_BYTESTRING_NULL;
    const UA_String *contentType = NULL;
    UA_StatusCode res = encodeHttpBody(channel, payload, payloadType, &body,
                                       &contentType);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    static const UA_String identity = UA_STRING_STATIC("identity");
#ifdef UA_ENABLE_JSON_ENCODING
    static const UA_String gzip = UA_STRING_STATIC("gzip");
    const UA_String *codingPolicy =
        channel->encoding == UA_SECURECHANNEL_ENCODING_JSON ? &gzip : &identity;
#else
    const UA_String *codingPolicy = &identity;
#endif
    return UA_Http_sendResponse(channel->connectionManager, connectionId, 200,
                                contentType, codingPolicy, &body);
}

UA_StatusCode
UA_SecureChannel_sendMSGHttp(UA_SecureChannel *channel, UA_UInt32 requestId,
                             void *payload, const UA_DataType *payloadType) {
    if(!channel || !payload || !payloadType ||
       (channel->state != UA_SECURECHANNELSTATE_OPEN &&
        channel->state != UA_SECURECHANNELSTATE_CLOSING) ||
       !channel->connectionManager || channel->connectionId == 0 ||
       requestId == 0)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    UA_ByteString body = UA_BYTESTRING_NULL;
    const UA_String *contentType = NULL;
    UA_StatusCode res = encodeHttpBody(channel, payload, payloadType, &body,
                                       &contentType);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String method = UA_STRING_STATIC("POST");
    UA_KeyValuePair headers[3] = {0};
    headers[0].key = UA_QUALIFIEDNAME(0, "content-type");
    UA_Variant_setScalar(&headers[0].value, (void *)(uintptr_t)contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    headers[1].key = UA_QUALIFIEDNAME(0, "opcua-securitypolicy");
    UA_Variant_setScalar(&headers[1].value,
                         &channel->securityPolicy->policyUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    static const UA_String identity = UA_STRING_STATIC("identity");
#ifdef UA_ENABLE_HTTP_COMPRESSION
    static const UA_String gzip = UA_STRING_STATIC("gzip");
    const UA_String *acceptEncoding =
        channel->encoding == UA_SECURECHANNEL_ENCODING_JSON ? &gzip : &identity;
#else
    const UA_String *acceptEncoding = &identity;
#endif
    headers[2].key = UA_QUALIFIEDNAME(0, "accept-encoding");
    UA_Variant_setScalar(&headers[2].value,
                         (void *)(uintptr_t)acceptEncoding,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_UInt16 timeout = 0;
    const UA_RequestHeader *requestHeader = (const UA_RequestHeader *)payload;
    if(requestHeader->timeoutHint > 0) {
        UA_UInt32 seconds = requestHeader->timeoutHint / 1000u +
            (requestHeader->timeoutHint % 1000u != 0);
        timeout = (UA_UInt16)(seconds > UA_UINT16_MAX ? UA_UINT16_MAX : seconds);
    }
    UA_KeyValuePair params[4] = {0};
    params[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&params[0].value, &method, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&params[1].value, headers, 3,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    params[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setScalar(&params[2].value, &requestId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    size_t paramsSize = 3;
    if(timeout) {
        params[3].key = UA_QUALIFIEDNAME(0, "timeout");
        UA_Variant_setScalar(&params[3].value, &timeout,
                             &UA_TYPES[UA_TYPES_UINT16]);
        paramsSize++;
    }
    UA_KeyValueMap map = {paramsSize, params};
    return channel->connectionManager->sendWithConnection(
        channel->connectionManager, channel->connectionId, &map, &body);
}
