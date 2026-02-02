/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/log.h>
#include <open62541/server_config_file_based.h>
#include <open62541/client_config_file_based.h>
#include "cj5.h"
#include "open62541/server_config_default.h"
#include "open62541/client_config_default.h"
#ifdef UA_ENABLE_ENCRYPTION
#include "open62541/plugin/certificategroup_default.h"
#endif

#define MAX_TOKENS 1024

typedef struct {
    const char *json;
    const cj5_token *tokens;
    size_t tokensSize;
    size_t index;
    UA_Byte depth;
    cj5_result result;

    UA_Logger *logging;
} ParsingCtx;

static UA_ByteString
getJsonPart(cj5_token tok, const char *json) {
    UA_ByteString bs;
    UA_ByteString_init(&bs);
    if(tok.type == CJ5_TOKEN_STRING) {
        bs.data = (UA_Byte*)(uintptr_t)(json + tok.start - 1);
        bs.length = (tok.end - tok.start) + 3;
        return bs;
    } else {
        bs.data = (UA_Byte*)(uintptr_t)(json + tok.start);
        bs.length = (tok.end - tok.start) + 1;
        return bs;
    }
}

/* Forward declarations*/
#define PARSE_JSON(TYPE) static UA_StatusCode                   \
    TYPE##_parseJson(ParsingCtx *ctx, void *configField, size_t *configFieldSize)

typedef UA_StatusCode
(*parseJsonSignature)(ParsingCtx *ctx, void *configField, size_t *configFieldSize);

#ifdef UA_ENABLE_ENCRYPTION
static UA_ByteString
loadCertificateFile(const char *const path);
#endif

/*----------------------Basic Types------------------------*/
#if 0
PARSE_JSON(Int64Field) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_Int64 out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Int64 *field = (UA_Int64*)configField;
    *field = out;
    return retval;
}
#endif
PARSE_JSON(UInt16Field) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_UInt16 out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_UINT16], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_UInt16 *field = (UA_UInt16*)configField;
    *field = out;
    return retval;
}
PARSE_JSON(UInt32Field) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_UInt32 out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_UINT32], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_UInt32 *field = (UA_UInt32*)configField;
    *field = out;
    return retval;
}
PARSE_JSON(UInt64Field) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_UInt64 out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_UINT64], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_UInt64 *field = (UA_UInt64*)configField;
    *field = out;
    return retval;
}
PARSE_JSON(Int32Field) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_Int32 out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_INT32], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Int32 *field = (UA_Int32*)configField;
    *field = out;
    return retval;
}
PARSE_JSON(StringField) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_String out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_STRING], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_String *field = (UA_String*)configField;
    if(field != NULL) {
        UA_String_clear(field);
        *field = out;
    }
    return retval;
}
PARSE_JSON(LocalizedTextField) {
    /*
     applicationName: {
        locale: "de-DE",
        text: "Test text"
    }
     */
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_String locale = {.length = 0, .data = NULL};
    UA_String text = {.length = 0, .data = NULL};
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field, &str_len);

            tok = ctx->tokens[++ctx->index];
            UA_ByteString buf = getJsonPart(tok, ctx->json);
            if(strcmp(field, "locale") == 0)
                retval |= UA_decodeJson(&buf, &locale, &UA_TYPES[UA_TYPES_STRING], NULL);
            else if(strcmp(field, "text") == 0)
                retval |= UA_decodeJson(&buf, &text, &UA_TYPES[UA_TYPES_STRING], NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field);
            }
            UA_free(field);
            break;
        }
        default:
            break;
        }
    }
    UA_LocalizedText out;
    out.locale = locale;
    out.text = text;
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_LocalizedText *field = (UA_LocalizedText*)configField;
    if(field != NULL) {
        UA_LocalizedText_clear(field);
        *field = out;
    }
    return retval;
}
PARSE_JSON(DoubleField) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_Double out;
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Double *field = (UA_Double *)configField;
    *field = out;
    return retval;
}
PARSE_JSON(BooleanField) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_Boolean out;
    if(tok.type != CJ5_TOKEN_BOOL) {
        UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Value of type bool expected.");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    UA_String val = UA_STRING("true");
    if(UA_String_equal(&val, &buf)) {
        out = true;
    }else {
        out = false;
    }
    /* set server config field */
    UA_Boolean *field = (UA_Boolean *)configField;
    *field = out;
    return UA_STATUSCODE_GOOD;
}
#ifdef UA_ENABLE_SUBSCRIPTIONS
PARSE_JSON(DurationField) {
    UA_Double double_value;
    UA_StatusCode retval = DoubleField_parseJson(ctx, &double_value, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_Duration *field = (UA_Duration*)configField;
    *field = (UA_Duration)double_value;
    return retval;
}
PARSE_JSON(DurationRangeField) {
    UA_DurationRange *field = (UA_DurationRange*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "min") == 0)
                DurationField_parseJson(ctx, &field->min, NULL);
            else if(strcmp(field_str, "max") == 0)
                DurationField_parseJson(ctx, &field->max, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
PARSE_JSON(UInt32RangeField) {
    UA_UInt32Range *field = (UA_UInt32Range*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "min") == 0)
                UInt32Field_parseJson(ctx, &field->min, NULL);
            else if(strcmp(field_str, "max") == 0)
                UInt32Field_parseJson(ctx, &field->max, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

/*----------------------Advanced Types------------------------*/
PARSE_JSON(StringArrayField) {
    if(configFieldSize == NULL) {
        UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Pointer to the array size is not set.");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_String *stringArray = (UA_String*)UA_malloc(sizeof(UA_String) * tok.size);
    size_t stringArraySize = 0;
    for(size_t j = tok.size; j > 0; j--) {
        UA_String out = {.length = 0, .data = NULL};
        StringField_parseJson(ctx, &out, NULL);
        UA_String_copy(&out, &stringArray[stringArraySize++]);
        UA_String_clear(&out);
    }
    /* Add to the config */
    UA_String **field = (UA_String**)configField;
    if(*configFieldSize > 0) {
        UA_Array_delete(*field, *configFieldSize,
                        &UA_TYPES[UA_TYPES_STRING]);
        *field = NULL;
        *configFieldSize = 0;
    }
    UA_StatusCode retval =
        UA_Array_copy(stringArray, stringArraySize,
                      (void**)field, &UA_TYPES[UA_TYPES_STRING]);
    *configFieldSize = stringArraySize;

    /* Clean up */
    UA_Array_delete(stringArray, stringArraySize, &UA_TYPES[UA_TYPES_STRING]);
    return retval;
}
PARSE_JSON(DateTimeField) {
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_DateTime *field = (UA_DateTime*)configField;
    *field = out;
    return retval;
}
PARSE_JSON(BuildInfo) {
    UA_BuildInfo *field = (UA_BuildInfo*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "productUri") == 0)
                StringField_parseJson(ctx, &field->productUri, NULL);
            else if(strcmp(field_str, "manufacturerName") == 0)
                StringField_parseJson(ctx, &field->manufacturerName, NULL);
            else if(strcmp(field_str, "productName") == 0)
                StringField_parseJson(ctx, &field->productName, NULL);
            else if(strcmp(field_str, "softwareVersion") == 0)
                StringField_parseJson(ctx, &field->softwareVersion, NULL);
            else if(strcmp(field_str, "buildNumber") == 0)
                StringField_parseJson(ctx, &field->buildNumber, NULL);
            else if(strcmp(field_str, "buildDate") == 0)
                DateTimeField_parseJson(ctx, &field->buildDate, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}

PARSE_JSON(ApplicationTypeField) {
    UA_UInt32 enum_value;
    UA_StatusCode retval = UInt32Field_parseJson(ctx, &enum_value, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_ApplicationType *field = (UA_ApplicationType*)configField;
    *field = (UA_ApplicationType)enum_value;
    return retval;
}
PARSE_JSON(ApplicationDescriptionField) {
    UA_ApplicationDescription *field = (UA_ApplicationDescription*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "applicationUri") == 0)
                StringField_parseJson(ctx, &field->applicationUri, NULL);
            else if(strcmp(field_str, "productUri") == 0)
                StringField_parseJson(ctx, &field->productUri, NULL);
            else if(strcmp(field_str, "applicationName") == 0)
                LocalizedTextField_parseJson(ctx, &field->applicationName, NULL);
            else if(strcmp(field_str, "applicationType") == 0)
                ApplicationTypeField_parseJson(ctx, &field->applicationType, NULL);
            else if(strcmp(field_str, "gatewayServerUri") == 0)
                StringField_parseJson(ctx, &field->gatewayServerUri, NULL);
            else if(strcmp(field_str, "discoveryProfileUri") == 0)
                StringField_parseJson(ctx, &field->discoveryProfileUri, NULL);
            else if(strcmp(field_str, "discoveryUrls") == 0)
                StringArrayField_parseJson(ctx, &field->discoveryUrls, &field->discoveryUrlsSize);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#if defined(UA_ENABLE_DISCOVERY_MULTICAST) && defined(UA_ENABLE_DISCOVERY_MULTICAST_MDNSD) && !defined(UA_HAS_GETIFADDR)
PARSE_JSON(UInt32ArrayField) {
    if(configFieldSize == NULL) {
        UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Pointer to the array size is not set.");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_UInt32 *numberArray = (UA_UInt32*)UA_malloc(sizeof(UA_UInt32) * tok.size);
    size_t numberArraySize = 0;
    for(size_t j = tok.size; j > 0; j--) {
        UA_UInt32 value;
        UA_StatusCode retval = UInt32Field_parseJson(ctx, &value, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            continue;
        numberArray[numberArraySize++] = value;
    }
    /* Add to the config */
    UA_UInt32 **field = (UA_UInt32**)configField;
    if(*configFieldSize > 0) {
        UA_Array_delete(*field, *configFieldSize,
                        &UA_TYPES[UA_TYPES_UINT32]);
        *field = NULL;
        *configFieldSize = 0;
    }
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(numberArraySize > 0) {
        retval = UA_Array_copy(numberArray, numberArraySize,
                          (void **)field, &UA_TYPES[UA_TYPES_UINT32]);
        *configFieldSize = numberArraySize;
    }
    /* Clean up */
    UA_Array_delete(numberArray, numberArraySize, &UA_TYPES[UA_TYPES_UINT32]);
    return retval;
}
#endif
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
PARSE_JSON(MdnsConfigurationField) {
    UA_ServerConfig *config = (UA_ServerConfig*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "mdnsServerName") == 0)
                StringField_parseJson(ctx, &config->mdnsConfig.mdnsServerName, NULL);
            else if(strcmp(field_str, "serverCapabilities") == 0)
                StringArrayField_parseJson(ctx, &config->mdnsConfig.serverCapabilities, &config->mdnsConfig.serverCapabilitiesSize);
#ifdef UA_ENABLE_DISCOVERY_MULTICAST_MDNSD
            else if(strcmp(field_str, "mdnsInterfaceIP") == 0)
                StringField_parseJson(ctx, &config->mdnsInterfaceIP, NULL);
            /* mdnsIpAddressList and mdnsIpAddressListSize are only available if UA_HAS_GETIFADDR is not defined: */
# if !defined(UA_HAS_GETIFADDR)
            else if(strcmp(field_str, "mdnsIpAddressList") == 0)
                UInt32ArrayField_parseJson(ctx, &config->mdnsIpAddressList, &config->mdnsIpAddressListSize);
# endif
#endif
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
        break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
PARSE_JSON(SubscriptionConfigurationField) {
    UA_ServerConfig *config = (UA_ServerConfig*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "maxSubscriptions") == 0)
                UInt32Field_parseJson(ctx, &config->maxSubscriptions, NULL);
            else if(strcmp(field_str, "maxSubscriptionsPerSession") == 0)
                UInt32Field_parseJson(ctx, &config->maxSubscriptionsPerSession, NULL);
            else if(strcmp(field_str, "publishingIntervalLimits") == 0)
                DurationRangeField_parseJson(ctx, &config->publishingIntervalLimits, NULL);
            else if(strcmp(field_str, "lifeTimeCountLimits") == 0)
                UInt32RangeField_parseJson(ctx, &config->lifeTimeCountLimits, NULL);
            else if(strcmp(field_str, "keepAliveCountLimits") == 0)
                UInt32RangeField_parseJson(ctx, &config->keepAliveCountLimits, NULL);
            else if(strcmp(field_str, "maxNotificationsPerPublish") == 0)
                UInt32Field_parseJson(ctx, &config->maxNotificationsPerPublish, NULL);
            else if(strcmp(field_str, "enableRetransmissionQueue") == 0)
                BooleanField_parseJson(ctx, &config->enableRetransmissionQueue, NULL);
            else if(strcmp(field_str, "maxRetransmissionQueueSize") == 0)
                UInt32Field_parseJson(ctx, &config->maxRetransmissionQueueSize, NULL);
# ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
            else if(strcmp(field_str, "maxEventsPerNode") == 0)
                UInt32Field_parseJson(ctx, &config->maxEventsPerNode, NULL);
# endif
            else if(strcmp(field_str, "maxMonitoredItems") == 0)
                UInt32Field_parseJson(ctx, &config->maxMonitoredItems, NULL);
            else if(strcmp(field_str, "maxMonitoredItemsPerSubscription") == 0)
                UInt32Field_parseJson(ctx, &config->maxMonitoredItemsPerSubscription, NULL);
            else if(strcmp(field_str, "samplingIntervalLimits") == 0)
                DurationRangeField_parseJson(ctx, &config->samplingIntervalLimits, NULL);
            else if(strcmp(field_str, "queueSizeLimits") == 0)
                UInt32RangeField_parseJson(ctx, &config->queueSizeLimits, NULL);
            else if(strcmp(field_str, "maxPublishReqPerSession") == 0)
                UInt32Field_parseJson(ctx, &config->maxPublishReqPerSession, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

PARSE_JSON(TcpConfigurationField) {
    UA_ServerConfig *config = (UA_ServerConfig*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "tcpBufSize") == 0)
                UInt32Field_parseJson(ctx, &config->tcpBufSize, NULL);
            else if(strcmp(field_str, "tcpMaxMsgSize") == 0)
                UInt32Field_parseJson(ctx, &config->tcpMaxMsgSize, NULL);
            else if(strcmp(field_str, "tcpMaxChunks") == 0)
                UInt32Field_parseJson(ctx, &config->tcpMaxChunks, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_PUBSUB
PARSE_JSON(PubsubConfigurationField) {
    UA_PubSubConfiguration *field = (UA_PubSubConfiguration*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "enableDeltaFrames") == 0)
                BooleanField_parseJson(ctx, &field->enableDeltaFrames, NULL);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
            else if(strcmp(field_str, "enableInformationModelMethods") == 0)
                BooleanField_parseJson(ctx, &field->enableInformationModelMethods, NULL);
#endif
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

#ifdef UA_ENABLE_HISTORIZING
PARSE_JSON(HistorizingConfigurationField) {
    UA_ServerConfig *config = (UA_ServerConfig*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "accessHistoryDataCapability") == 0)
                BooleanField_parseJson(ctx, &config->accessHistoryDataCapability, NULL);
            else if(strcmp(field_str, "maxReturnDataValues") == 0)
                UInt32Field_parseJson(ctx, &config->maxReturnDataValues, NULL);
            else if(strcmp(field_str, "accessHistoryEventsCapability") == 0)
                BooleanField_parseJson(ctx, &config->accessHistoryEventsCapability, NULL);
            else if(strcmp(field_str, "maxReturnEventValues") == 0)
                UInt32Field_parseJson(ctx, &config->maxReturnEventValues, NULL);
            else if(strcmp(field_str, "insertDataCapability") == 0)
                BooleanField_parseJson(ctx, &config->insertDataCapability, NULL);
            else if(strcmp(field_str, "insertEventCapability") == 0)
                BooleanField_parseJson(ctx, &config->insertEventCapability, NULL);
            else if(strcmp(field_str, "insertAnnotationsCapability") == 0)
                BooleanField_parseJson(ctx, &config->insertAnnotationsCapability, NULL);
            else if(strcmp(field_str, "replaceDataCapability") == 0)
                BooleanField_parseJson(ctx, &config->replaceDataCapability, NULL);
            else if(strcmp(field_str, "replaceEventCapability") == 0)
                BooleanField_parseJson(ctx, &config->replaceEventCapability, NULL);
            else if(strcmp(field_str, "updateDataCapability") == 0)
                BooleanField_parseJson(ctx, &config->updateDataCapability, NULL);
            else if(strcmp(field_str, "updateEventCapability") == 0)
                BooleanField_parseJson(ctx, &config->updateEventCapability, NULL);
            else if(strcmp(field_str, "deleteRawCapability") == 0)
                BooleanField_parseJson(ctx, &config->deleteRawCapability, NULL);
            else if(strcmp(field_str, "deleteEventCapability") == 0)
                BooleanField_parseJson(ctx, &config->deleteEventCapability, NULL);
            else if(strcmp(field_str, "deleteAtTimeDataCapability") == 0)
                BooleanField_parseJson(ctx, &config->deleteAtTimeDataCapability, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

PARSE_JSON(SecurityPolciesField) {
#ifdef UA_ENABLE_ENCRYPTION
    UA_ServerConfig *config = (UA_ServerConfig*)configField;

    UA_String noneuri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String basic128Rsa15uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
    UA_String basic256uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
    UA_String basic256Sha256uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    UA_String aes128sha256rsaoaepuri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
    UA_String aes256sha256rsapssuri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss");

    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size; j > 0; j--) {

        UA_String policy = {.length = 0, .data = NULL};
        UA_ByteString certificate = {.length = 0, .data = NULL};
        UA_ByteString privateKey = {.length = 0, .data = NULL};

        tok = ctx->tokens[++ctx->index];
        for(size_t i = tok.size / 2; i > 0; i--) {
            tok = ctx->tokens[++ctx->index];
            switch(tok.type) {
            case CJ5_TOKEN_STRING: {
                char *field_str = (char *)UA_malloc(tok.size + 1);
                unsigned int str_len = 0;
                cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
                if(strcmp(field_str, "certificate") == 0) {
                    UA_String out = {.length = 0, .data = NULL};
                    StringField_parseJson(ctx, &out, NULL);

                    if(out.length > 0) {
                        char *certfile = (char *)UA_malloc(out.length + 1);
                        memcpy(certfile, out.data, out.length);
                        certfile[out.length] = '\0';
                        certificate = loadCertificateFile(certfile);
                        UA_String_clear(&out);
                        UA_free(certfile);
                    }
                } else if(strcmp(field_str, "privateKey") == 0) {
                    UA_String out = {.length = 0, .data = NULL};
                    StringField_parseJson(ctx, &out, NULL);

                    if(out.length > 0) {
                        char *keyfile = (char *)UA_malloc(out.length + 1);
                        memcpy(keyfile, out.data, out.length);
                        keyfile[out.length] = '\0';
                        privateKey = loadCertificateFile(keyfile);
                        UA_String_clear(&out);
                        UA_free(keyfile);
                    }
                } else if(strcmp(field_str, "policy") == 0) {
                    StringField_parseJson(ctx, &policy, NULL);
                } else {
                    UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
                }
                UA_free(field_str);
                break;
            }
            default:
                break;
            }
        }

        if(certificate.length == 0 || privateKey.length == 0) {
            UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION,
                           "Certificate and PrivateKey must be set for every policy.");
            if(policy.length > 0)
                UA_String_clear(&policy);
            if(certificate.length > 0)
                UA_ByteString_clear(&certificate);
            if(privateKey.length > 0)
                UA_ByteString_clear(&privateKey);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        UA_StatusCode retval = UA_STATUSCODE_GOOD;
        if(UA_String_equal(&policy, &noneuri)) {
            /* Nothing to do! */
        } else if(UA_String_equal(&policy, &basic128Rsa15uri)) {
            retval = UA_ServerConfig_addSecurityPolicyBasic128Rsa15(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION,
                               "Could not add SecurityPolicy#Basic128Rsa15 with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &basic256uri)) {
            retval = UA_ServerConfig_addSecurityPolicyBasic256(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION,
                               "Could not add SecurityPolicy#Basic256 with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &basic256Sha256uri)) {
            retval = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION,
                               "Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &aes128sha256rsaoaepuri)) {
            retval = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION,
                               "Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &aes256sha256rsapssuri)) {
             retval = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(config, &certificate, &privateKey);
             if(retval != UA_STATUSCODE_GOOD) {
                 UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION,
                                "Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else {
            UA_LOG_WARNING(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown Security Policy.");
        }

        /* Add all Endpoints */
        UA_ServerConfig_addAllEndpoints(config);

        if(policy.length > 0)
            UA_String_clear(&policy);
        if(certificate.length > 0)
            UA_ByteString_clear(&certificate);
        if(privateKey.length > 0)
            UA_ByteString_clear(&privateKey);
    }
#endif
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_ENCRYPTION
PARSE_JSON(SecurityPkiField) {
    UA_ServerConfig *config = (UA_ServerConfig*)configField;
    UA_String pkiFolder = {.length = 0, .data = NULL};

    cj5_token tok = ctx->tokens[++ctx->index];
    UA_ByteString buf = getJsonPart(tok, ctx->json);
    UA_StatusCode retval = UA_decodeJson(&buf, &pkiFolder, &UA_TYPES[UA_TYPES_STRING], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    /* Currently not supported! */
    (void)config;
    return UA_STATUSCODE_GOOD;
#else
    /* Set up the parameters */
    UA_KeyValuePair params[2];
    size_t paramsSize = 2;

    params[0].key = UA_QUALIFIEDNAME(0, "max-trust-listsize");
    UA_Variant_setScalar(&params[0].value, &config->maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);
    params[1].key = UA_QUALIFIEDNAME(0, "max-rejected-listsize");
    UA_Variant_setScalar(&params[1].value, &config->maxRejectedListSize, &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = paramsSize;

    /* set server config field */
    UA_NodeId defaultApplicationGroup =
           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    retval = UA_CertificateGroup_Filestore(&config->secureChannelPKI, &defaultApplicationGroup,
                                           pkiFolder, config->logging, &paramsMap);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_String_clear(&pkiFolder);
        return retval;
    }

    UA_NodeId defaultUserTokenGroup =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    retval = UA_CertificateGroup_Filestore(&config->sessionPKI, &defaultUserTokenGroup,
                                            pkiFolder, config->logging, &paramsMap);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_String_clear(&pkiFolder);
        return retval;
    }

    /* Clean up */
    UA_String_clear(&pkiFolder);
#endif
    return UA_STATUSCODE_GOOD;
}
#endif
PARSE_JSON(RuleHandlingField) {
    UA_UInt32 enum_value;
    UA_StatusCode retval = UInt32Field_parseJson(ctx, &enum_value, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_RuleHandling *field = (UA_RuleHandling*)configField;
    *field = (UA_RuleHandling)enum_value;
    return retval;
}

/* Skips unknown item (simple, object or array) in config file. 
* Unknown items may happen if we don't support some features. 
* E.g. if  UA_ENABLE_ENCRYPTION is not defined and config file 
* contains "securityPolicies" entry.
*/
static void
skipUnknownItem(ParsingCtx* ctx) {
    unsigned int end = ctx->tokens[ctx->index].end;
    do {
        ctx->index++;
    } while (ctx->index < ctx->tokensSize &&
        ctx->tokens[ctx->index].start < end);
}

static UA_StatusCode
parseJSONServerConfig(UA_ServerConfig *config, UA_ByteString json_config) {
    // Parsing json config
    const char *json = (const char*)json_config.data;
    cj5_token tokens[MAX_TOKENS];
    cj5_result r = cj5_parse(json, (unsigned int)json_config.length, tokens, MAX_TOKENS, NULL);

    ParsingCtx ctx;
    ctx.json = json;
    ctx.result = r;
    ctx.tokens = r.tokens;
    ctx.tokensSize = r.num_tokens;
    ctx.index = 1; // The first token is ignored because it is known and not needed.

    ctx.logging = config->logging;

    size_t serverConfigSize = 0;
    if(ctx.tokens)
        serverConfigSize = (ctx.tokens[ctx.index-1].size/2);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for (size_t j = serverConfigSize; j > 0; j--) {
        cj5_token tok = ctx.tokens[ctx.index];
        switch (tok.type) {
            case CJ5_TOKEN_STRING: {
                char *field = (char*)UA_malloc(tok.size + 1);
                unsigned int str_len = 0;
                cj5_get_str(&ctx.result, (unsigned int)ctx.index, field, &str_len);
                if(strcmp(field, "buildInfo") == 0)
                    retval = BuildInfo_parseJson(&ctx, &config->buildInfo, NULL);
                else if(strcmp(field, "applicationDescription") == 0)
                    retval = ApplicationDescriptionField_parseJson(&ctx, &config->applicationDescription, NULL);
                else if(strcmp(field, "shutdownDelay") == 0)
                    retval = DoubleField_parseJson(&ctx, &config->shutdownDelay, NULL);
                else if(strcmp(field, "verifyRequestTimestamp") == 0)
                    retval = RuleHandlingField_parseJson(&ctx, &config->verifyRequestTimestamp, NULL);
                else if(strcmp(field, "allowEmptyVariables") == 0)
                    retval = RuleHandlingField_parseJson(&ctx, &config->allowEmptyVariables, NULL);
                else if(strcmp(field, "serverUrls") == 0)
                    retval = StringArrayField_parseJson(&ctx, &config->serverUrls, &config->serverUrlsSize);
                else if(strcmp(field, "tcpEnabled") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->tcpEnabled, NULL);
                else if(strcmp(field, "tcp") == 0)
                    retval = TcpConfigurationField_parseJson(&ctx, config, NULL);
                else if(strcmp(field, "securityPolicyNoneDiscoveryOnly") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->securityPolicyNoneDiscoveryOnly, NULL);
                else if(strcmp(field, "modellingRulesOnInstances") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->modellingRulesOnInstances, NULL);
                else if(strcmp(field, "maxSecureChannels") == 0)
                    retval = UInt16Field_parseJson(&ctx, &config->maxSecureChannels, NULL);
                else if(strcmp(field, "maxSecurityTokenLifetime") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxSecurityTokenLifetime, NULL);
                else if(strcmp(field, "maxSessions") == 0)
                    retval = UInt16Field_parseJson(&ctx, &config->maxSessions, NULL);
                else if(strcmp(field, "maxSessionTimeout") == 0)
                    retval = DoubleField_parseJson(&ctx, &config->maxSessionTimeout, NULL);
                else if(strcmp(field, "maxNodesPerRead") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerRead, NULL);
                else if(strcmp(field, "maxNodesPerWrite") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerWrite, NULL);
                else if(strcmp(field, "maxNodesPerMethodCall") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerMethodCall, NULL);
                else if(strcmp(field, "maxNodesPerBrowse") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerBrowse, NULL);
                else if(strcmp(field, "maxNodesPerRegisterNodes") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerRegisterNodes, NULL);
                else if(strcmp(field, "maxNodesPerTranslateBrowsePathsToNodeIds") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerTranslateBrowsePathsToNodeIds, NULL);
                else if(strcmp(field, "maxNodesPerNodeManagement") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxNodesPerNodeManagement, NULL);
                else if(strcmp(field, "maxMonitoredItemsPerCall") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxMonitoredItemsPerCall, NULL);
                else if(strcmp(field, "maxReferencesPerNode") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxReferencesPerNode, NULL);
                else if(strcmp(field, "reverseReconnectInterval") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->reverseReconnectInterval, NULL);

#if UA_MULTITHREADING >= 100
                else if(strcmp(field, "asyncOperationTimeout") == 0)
                    retval = DoubleField_parseJson(&ctx, &config->asyncOperationTimeout, NULL);
                else if(strcmp(field, "maxAsyncOperationQueueSize") == 0)
                    retval = UInt64Field_parseJson(&ctx, &config->maxAsyncOperationQueueSize, NULL);
#endif

#ifdef UA_ENABLE_DISCOVERY
                else if(strcmp(field, "discoveryCleanupTimeout") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->discoveryCleanupTimeout, NULL);
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
                else if(strcmp(field, "mdnsEnabled") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->mdnsEnabled, NULL);
                else if(strcmp(field, "mdns") == 0)
                    retval = MdnsConfigurationField_parseJson(&ctx, config, NULL);
#endif
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
                else if(strcmp(field, "subscriptionsEnabled") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->subscriptionsEnabled, NULL);
                else if(strcmp(field, "subscriptions") == 0)
                    retval = SubscriptionConfigurationField_parseJson(&ctx, config, NULL);
# endif

#ifdef UA_ENABLE_HISTORIZING
                else if(strcmp(field, "historizingEnabled") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->historizingEnabled, NULL);
                else if(strcmp(field, "historizing") == 0)
                    retval = HistorizingConfigurationField_parseJson(&ctx, config, NULL);
#endif

#ifdef UA_ENABLE_PUBSUB
                else if(strcmp(field, "pubsubEnabled") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->pubsubEnabled, NULL);
                else if(strcmp(field, "pubsub") == 0)
                    retval = PubsubConfigurationField_parseJson(&ctx, &config->pubSubConfig, NULL);
#endif
#ifdef UA_ENABLE_ENCRYPTION
                else if(strcmp(field, "securityPolicies") == 0)
                    retval = SecurityPolciesField_parseJson(&ctx, config, NULL);
                else if(strcmp(field, "pkiFolder") == 0)
                    retval = SecurityPkiField_parseJson(&ctx, config, NULL);
#endif
                else {
                    UA_LOG_WARNING(ctx.logging, UA_LOGCATEGORY_APPLICATION,
                                   "Field name '%s' unknown or misspelled. Maybe the feature is not enabled either.", field);
                    /* skip the name of item */
                    ++ctx.index;
                    /* skip value of unknown item */
                    skipUnknownItem(&ctx);
                    /* after skipUnknownItem() ctx->index points to the name of the following item.
                       We must decrement index in oder following increment will
                       still set index to the right position (name of the following item) */
                    --ctx.index;
                }
                UA_free(field);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_LOG_ERROR(ctx.logging, UA_LOGCATEGORY_APPLICATION, "An error occurred while parsing the configuration file.");
                    return retval;
                }
                break;
            }
            default:
                break;
        }
        ctx.index += 1;
    }
    return retval;
}

UA_Server *
UA_Server_newFromFile(const UA_ByteString json_config) {
    UA_ServerConfig config;
    UA_StatusCode res = UA_ServerConfig_loadFromFile(&config, json_config);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;
    return UA_Server_newWithConfig(&config);
}

UA_StatusCode
UA_ServerConfig_loadFromFile(UA_ServerConfig *config, const UA_ByteString json_config) {
    memset(config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setDefault(config);
    res |= parseJSONServerConfig(config, json_config);
    return res;
}

PARSE_JSON(ConnectionConfig) {
    UA_ConnectionConfig *field = (UA_ConnectionConfig*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "protocolVersion") == 0)
                UInt32Field_parseJson(ctx, &field->protocolVersion, NULL);
            else if(strcmp(field_str, "recvBufferSize") == 0)
                UInt32Field_parseJson(ctx, &field->recvBufferSize, NULL);
            else if(strcmp(field_str, "sendBufferSize") == 0)
                UInt32Field_parseJson(ctx, &field->sendBufferSize, NULL);
            else if(strcmp(field_str, "localMaxMessageSize") == 0)
                UInt32Field_parseJson(ctx, &field->localMaxMessageSize, NULL);
            else if(strcmp(field_str, "remoteMaxMessageSize") == 0)
                UInt32Field_parseJson(ctx, &field->remoteMaxMessageSize, NULL);
            else if(strcmp(field_str, "localMaxChunkCount") == 0)
                UInt32Field_parseJson(ctx, &field->localMaxChunkCount, NULL);
            else if(strcmp(field_str, "remoteMaxChunkCount") == 0)
                UInt32Field_parseJson(ctx, &field->remoteMaxChunkCount, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}

PARSE_JSON(EndpointDescription) {
    UA_EndpointDescription *field = (UA_EndpointDescription*)configField;
    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t j = tok.size/2; j > 0; j--) {
        tok = ctx->tokens[++ctx->index];
        switch (tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "endpointUrl") == 0)
                StringField_parseJson(ctx, &field->endpointUrl, NULL);
            else if(strcmp(field_str, "server") == 0)
                ApplicationDescriptionField_parseJson(ctx, &field->server, NULL);
            // TODO
            //else if(strcmp(field_str, "serverCertificate") == 0)
            //    ByteString_parseJson(ctx, &field->serverCertificate, NULL);
            // TODO
            //else if(strcmp(field_str, "securityMode") == 0)
            //   MessageSecurityMode_parseJson(ctx, &field->securityMode, NULL);
            else if(strcmp(field_str, "securityPolicyUri") == 0)
               StringField_parseJson(ctx, &field->securityPolicyUri, NULL);
            // TODO
            //else if(strcmp(field_str, "userIdentityTokens") == 0)
            //   UserTokenPolicyArray_parseJson(ctx, &field->userIdentityTokens, &field->userIdentityTokensSize);
            else if(strcmp(field_str, "transportProfileUri") == 0)
                StringField_parseJson(ctx, &field->transportProfileUri, NULL);
            // TODO
            //else if(strcmp(field_str, "transportProfileUri") == 0)
            //    ByteField_parseJson(ctx, &field->securityLevel, NULL);
            else {
                UA_LOG_ERROR(ctx->logging, UA_LOGCATEGORY_APPLICATION, "Unknown field name '%s'.", field_str);
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}



static UA_StatusCode
parseJSONClientConfig(UA_ClientConfig *config, UA_ByteString json_config) {
    // Parsing json config
    const char *json = (const char*)json_config.data;
    cj5_token tokens[MAX_TOKENS];
    cj5_result r = cj5_parse(json, (unsigned int)json_config.length, tokens, MAX_TOKENS, NULL);

    ParsingCtx ctx;
    ctx.json = json;
    ctx.result = r;
    ctx.tokens = r.tokens;
    ctx.tokensSize = r.num_tokens;
    ctx.index = 1; // The first token is ignored because it is known and not needed.

    ctx.logging = config->logging;

    size_t clientConfigSize = 0;
    if(ctx.tokens)
        clientConfigSize = (ctx.tokens[ctx.index-1].size/2);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for (size_t j = clientConfigSize; j > 0; j--) {
        cj5_token tok = ctx.tokens[ctx.index];
        switch (tok.type) {
            case CJ5_TOKEN_STRING: {
                char *field = (char*)UA_malloc(tok.size + 1);
                unsigned int str_len = 0;
                cj5_get_str(&ctx.result, (unsigned int)ctx.index, field, &str_len);
                if(strcmp(field, "timeout") == 0)
                    retval = Int32Field_parseJson(&ctx, &config->timeout, NULL);
                else if(strcmp(field, "applicationDescription") == 0)
                    retval = ApplicationDescriptionField_parseJson(&ctx, &config->clientDescription, NULL);
                else if(strcmp(field, "endpointUrl") == 0)
                    retval = StringField_parseJson(&ctx, &config->endpointUrl, NULL);
                // TODO missing userIdentityToken ?
                else if(strcmp(field, "sessionName") == 0)
                    retval = StringField_parseJson(&ctx, &config->sessionName, NULL);
                else if(strcmp(field, "sessionLocaleIds") == 0)
                    // TODO missing constraints for type?
                    retval = StringArrayField_parseJson(&ctx, &config->sessionLocaleIds, &config->sessionLocaleIdsSize);
                else if(strcmp(field, "noSession") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->noSession, NULL);
                else if(strcmp(field, "noReconnect") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->noReconnect, NULL);
                else if(strcmp(field, "noNewSession") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->noNewSession, NULL);
                else if(strcmp(field, "secureChannelLifeTime") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->secureChannelLifeTime, NULL);
                else if(strcmp(field, "requestedSessionTimeout") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->requestedSessionTimeout, NULL);
                else if(strcmp(field, "localConnectionConfig") == 0)
                    retval = ConnectionConfig_parseJson(&ctx, &config->localConnectionConfig, NULL);
                else if(strcmp(field, "connectivityCheckInterval") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->connectivityCheckInterval, NULL);
                else if(strcmp(field, "tcpReuseAddr") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->tcpReuseAddr, NULL);
                else if(strcmp(field, "endpoint") == 0)
                    retval = EndpointDescription_parseJson(&ctx, &config->endpoint, NULL);
                //else if(strcmp(field, "userTokenPolicy") == 0)
                //    retval = UserTokenPolicy_parseJson(&ctx, &config->userTokenPolicy, NULL);
                else if(strcmp(field, "applicationUri") == 0)
                    retval = StringField_parseJson(&ctx, &config->applicationUri, NULL);
                //else if(strcmp(field, "securityMode") == 0)
                //    retval = SecurityMode_parseJson(&ctx, &config->securityMode, NULL);
                else if(strcmp(field, "securityPolicyUri") == 0)
                    retval = StringField_parseJson(&ctx, &config->securityPolicyUri, NULL);
                else if(strcmp(field, "authSecurityPolicyUri") == 0)
                    retval = StringField_parseJson(&ctx, &config->authSecurityPolicyUri, NULL);
                else if(strcmp(field, "securityPolicies") == 0)
                    retval = SecurityPolciesField_parseJson(&ctx, &config->securityPolicies, &config->securityPoliciesSize);
                else if(strcmp(field, "authSecurityPolicies") == 0)
                    retval = SecurityPolciesField_parseJson(&ctx, &config->authSecurityPolicies, &config->authSecurityPoliciesSize);
                //else if(strcmp(field, "certificateVerification") == 0)
                //    retval = CertificateGroup_parseJson(&ctx, &config->certificateVerification, NULL);
                else if(strcmp(field, "allowNonePolicyPassword") == 0)
                    retval = BooleanField_parseJson(&ctx, &config->allowNonePolicyPassword, NULL);
#ifdef UA_ENABLE_ENCRYPTION
                else if(strcmp(field, "maxTrustListSize") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxTrustListSize, NULL);
                else if(strcmp(field, "maxRejectedListSize") == 0)
                    retval = UInt32Field_parseJson(&ctx, &config->maxRejectedListSize, NULL);
#endif
                /* TODO missing customDataTypes */
                else if(strcmp(field, "namespaces") == 0)
                    retval = StringArrayField_parseJson(&ctx, &config->namespaces, &config->namespacesSize);
                else if(strcmp(field, "outStandingPublishRequests") == 0)
                    retval = UInt16Field_parseJson(&ctx, &config->outStandingPublishRequests, NULL);
                else {
                    UA_LOG_WARNING(ctx.logging, UA_LOGCATEGORY_APPLICATION,
                                   "Field name '%s' unknown or misspelled. Maybe the feature is not enabled either.", field);
                    /* skip the name of item */
                    ++ctx.index;
                    /* skip value of unknown item */
                    skipUnknownItem(&ctx);
                    /* after skipUnknownItem() ctx->index points to the name of the following item.
                       We must decrement index in oder following increment will
                       still set index to the right position (name of the following item) */
                    --ctx.index;
                }
                UA_free(field);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_LOG_ERROR(ctx.logging, UA_LOGCATEGORY_APPLICATION, "An error occurred while parsing the configuration file.");
                    return retval;
                }
                break;
            }
            default:
                break;
        }
        ctx.index += 1;
    }
    return retval;
}

UA_Client *
UA_Client_newFromFile(const UA_ByteString json_config)
{
    UA_ClientConfig config;
    UA_StatusCode res = UA_ClientConfig_loadFromFile(&config, json_config);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;
    return UA_Client_newWithConfig(&config);
}

UA_StatusCode
UA_ClientConfig_loadFromFile(UA_ClientConfig *config, const UA_ByteString json_config)
{
    memset(config, 0, sizeof(UA_ClientConfig));
    UA_StatusCode res = UA_ClientConfig_setDefault(config);
    res |= parseJSONClientConfig(config, json_config);
    return res;
}

#ifdef UA_ENABLE_ENCRYPTION
static UA_ByteString
loadCertificateFile(const char *const path) {
    UA_ByteString fileContents = UA_BYTESTRING_NULL;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}
#endif
