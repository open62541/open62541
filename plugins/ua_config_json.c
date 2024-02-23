/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/server_config_file_based.h>
#include <open62541/plugin/log_stdout.h>
#include "cj5.h"
#include "open62541/server_config_default.h"
#ifdef UA_ENABLE_ENCRYPTION
#include "open62541/plugin/certificategroup_default.h"
#endif

#define MAX_TOKENS 256

typedef struct {
    const char *json;
    const cj5_token *tokens;
    cj5_result result;
    unsigned int tokensSize;
    size_t index;
    UA_Byte depth;
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

/* The DataType "kind" is an internal type classification. It is used to
 * dispatch handling to the correct routines. */
#define UA_SERVERCONFIGFIELDKINDS 25
typedef enum {
    /* Basic Types */
    UA_SERVERCONFIGFIELD_INT64 = 0,
    UA_SERVERCONFIGFIELD_UINT16,
    UA_SERVERCONFIGFIELD_UINT32,
    UA_SERVERCONFIGFIELD_UINT64,
    UA_SERVERCONFIGFIELD_STRING,
    UA_SERVERCONFIGFIELD_LOCALIZEDTEXT,
    UA_SERVERCONFIGFIELD_DOUBLE,
    UA_SERVERCONFIGFIELD_BOOLEAN,
    UA_SERVERCONFIGFIELD_DURATION,
    UA_SERVERCONFIGFIELD_DURATIONRANGE,
    UA_SERVERCONFIGFIELD_UINT32RANGE,

    /* Advanced Types */
    UA_SERVERCONFIGFIELD_BUILDINFO,
    UA_SERVERCONFIGFIELD_APPLICATIONDESCRIPTION,
    UA_SERVERCONFIGFIELD_STRINGARRAY,
    UA_SERVERCONFIGFIELD_UINT32ARRAY,
    UA_SERVERCONFIGFIELD_DATETIME,
    UA_SERVERCONFIGFIELD_SUBSCRIPTIONCONFIGURATION,
    UA_SERVERCONFIGFIELD_TCPCONFIGURATION,
    UA_SERVERCONFIGFIELD_PUBSUBCONFIGURATION,
    UA_SERVERCONFIGFIELD_HISTORIZINGCONFIGURATION,
    UA_SERVERCONFIGFIELD_MDNSCONFIGURATION,
    UA_SERVERCONFIGFIELD_SECURITYPOLICIES,
    UA_SERVERCONFIGFIELD_SECURITYPKI,

    /* Enumerations */
    UA_SERVERCONFIGFIELD_APPLICATIONTYPE,
    UA_SERVERCONFIGFIELD_RULEHANDLING
} UA_ServerConfigFieldKind;

extern const parseJsonSignature parseJsonJumpTable[UA_SERVERCONFIGFIELDKINDS];

/*----------------------Basic Types------------------------*/
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
    UA_String locale;
    UA_String text;
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
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
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
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Value of type bool expected.");
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_DURATION](ctx, &field->min, NULL);
            else if(strcmp(field_str, "max") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_DURATION](ctx, &field->max, NULL);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &field->min, NULL);
            else if(strcmp(field_str, "max") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &field->max, NULL);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
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

/*----------------------Advanced Types------------------------*/
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->productUri, NULL);
            else if(strcmp(field_str, "manufacturerName") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->manufacturerName, NULL);
            else if(strcmp(field_str, "productName") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->productName, NULL);
            else if(strcmp(field_str, "softwareVersion") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->softwareVersion, NULL);
            else if(strcmp(field_str, "buildNumber") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->buildNumber, NULL);
            else if(strcmp(field_str, "buildDate") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_DATETIME](ctx, &field->buildDate, NULL);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->applicationUri, NULL);
            else if(strcmp(field_str, "productUri") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->productUri, NULL);
            else if(strcmp(field_str, "applicationName") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_LOCALIZEDTEXT](ctx, &field->applicationName, NULL);
            else if(strcmp(field_str, "applicationType") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_APPLICATIONTYPE](ctx, &field->applicationType, NULL);
            else if(strcmp(field_str, "gatewayServerUri") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->gatewayServerUri, NULL);
            else if(strcmp(field_str, "discoveryProfileUri") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &field->discoveryProfileUri, NULL);
            else if(strcmp(field_str, "discoveryUrls") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRINGARRAY](ctx, &field->discoveryUrls, &field->discoveryUrlsSize);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
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
PARSE_JSON(StringArrayField) {
    if(configFieldSize == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pointer to the array size is not set.");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_String *stringArray = (UA_String*)UA_malloc(sizeof(UA_String) * tok.size);
    size_t stringArraySize = 0;
    for(size_t j = tok.size; j > 0; j--) {
        UA_String out = {.length = 0, .data = NULL};;
        parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &out, NULL);
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
PARSE_JSON(UInt32ArrayField) {
    if(configFieldSize == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Pointer to the array size is not set.");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    cj5_token tok = ctx->tokens[++ctx->index];
    UA_UInt32 *numberArray = (UA_UInt32*)UA_malloc(sizeof(UA_UInt32) * tok.size);;
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

PARSE_JSON(MdnsConfigurationField) {
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &config->mdnsConfig.mdnsServerName, NULL);
            else if(strcmp(field_str, "serverCapabilities") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRINGARRAY](ctx, &config->mdnsConfig.serverCapabilities, &config->mdnsConfig.serverCapabilitiesSize);
            else if(strcmp(field_str, "mdnsInterfaceIP") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &config->mdnsInterfaceIP, NULL);
            else if(strcmp(field_str, "mdnsIpAddressList") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32ARRAY](ctx, &config->mdnsIpAddressList, &config->mdnsIpAddressListSize);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
            }
            UA_free(field_str);
            break;
        }
        default:
        break;
        }
    }
#endif
    return UA_STATUSCODE_GOOD;
}

PARSE_JSON(SubscriptionConfigurationField) {
#ifdef UA_ENABLE_SUBSCRIPTIONS
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxSubscriptions, NULL);
            else if(strcmp(field_str, "maxSubscriptionsPerSession") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxSubscriptionsPerSession, NULL);
            else if(strcmp(field_str, "publishingIntervalLimits") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_DURATIONRANGE](ctx, &config->publishingIntervalLimits, NULL);
            else if(strcmp(field_str, "lifeTimeCountLimits") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32RANGE](ctx, &config->lifeTimeCountLimits, NULL);
            else if(strcmp(field_str, "keepAliveCountLimits") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32RANGE](ctx, &config->keepAliveCountLimits, NULL);
            else if(strcmp(field_str, "maxNotificationsPerPublish") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxNotificationsPerPublish, NULL);
            else if(strcmp(field_str, "enableRetransmissionQueue") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->enableRetransmissionQueue, NULL);
            else if(strcmp(field_str, "maxRetransmissionQueueSize") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxRetransmissionQueueSize, NULL);
# ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
            else if(strcmp(field_str, "maxEventsPerNode") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxEventsPerNode, NULL);
# endif
            else if(strcmp(field_str, "maxMonitoredItems") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxMonitoredItems, NULL);
            else if(strcmp(field_str, "maxMonitoredItemsPerSubscription") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxMonitoredItemsPerSubscription, NULL);
            else if(strcmp(field_str, "samplingIntervalLimits") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_DURATIONRANGE](ctx, &config->samplingIntervalLimits, NULL);
            else if(strcmp(field_str, "queueSizeLimits") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32RANGE](ctx, &config->queueSizeLimits, NULL);
            else if(strcmp(field_str, "maxPublishReqPerSession") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxPublishReqPerSession, NULL);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
#endif
    return UA_STATUSCODE_GOOD;
}

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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->tcpBufSize, NULL);
            else if(strcmp(field_str, "tcpMaxMsgSize") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->tcpMaxMsgSize, NULL);
            else if(strcmp(field_str, "tcpMaxChunks") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->tcpMaxChunks, NULL);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
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

PARSE_JSON(PubsubConfigurationField) {
#ifdef UA_ENABLE_PUBSUB
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &field->enableDeltaFrames, NULL);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
            else if(strcmp(field_str, "enableInformationModelMethods") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &field->enableInformationModelMethods, NULL);
#endif
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
#endif
    return UA_STATUSCODE_GOOD;
}

PARSE_JSON(HistorizingConfigurationField) {
#ifdef UA_ENABLE_HISTORIZING
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
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->accessHistoryDataCapability, NULL);
            else if(strcmp(field_str, "maxReturnDataValues") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxReturnDataValues, NULL);
            else if(strcmp(field_str, "accessHistoryEventsCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->accessHistoryEventsCapability, NULL);
            else if(strcmp(field_str, "maxReturnEventValues") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](ctx, &config->maxReturnEventValues, NULL);
            else if(strcmp(field_str, "insertDataCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->insertDataCapability, NULL);
            else if(strcmp(field_str, "insertEventCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->insertEventCapability, NULL);
            else if(strcmp(field_str, "insertAnnotationsCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->insertAnnotationsCapability, NULL);
            else if(strcmp(field_str, "replaceDataCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->replaceDataCapability, NULL);
            else if(strcmp(field_str, "replaceEventCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->replaceEventCapability, NULL);
            else if(strcmp(field_str, "updateDataCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->updateDataCapability, NULL);
            else if(strcmp(field_str, "updateEventCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->updateEventCapability, NULL);
            else if(strcmp(field_str, "deleteRawCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->deleteRawCapability, NULL);
            else if(strcmp(field_str, "deleteEventCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->deleteEventCapability, NULL);
            else if(strcmp(field_str, "deleteAtTimeDataCapability") == 0)
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](ctx, &config->deleteAtTimeDataCapability, NULL);
            else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
#endif
    return UA_STATUSCODE_GOOD;
}

PARSE_JSON(SecurityPolciesField) {
#ifdef UA_ENABLE_ENCRYPTION
    UA_ServerConfig *config = (UA_ServerConfig*)configField;

    UA_String noneuri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String basic128Rsa15uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15");
    UA_String basic256uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256");
    UA_String basic256Sha256uri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    UA_String aes128sha256rsaoaepuri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

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
                    parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &out, NULL);

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
                    parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &out, NULL);

                    if(out.length > 0) {
                        char *keyfile = (char *)UA_malloc(out.length + 1);
                        memcpy(keyfile, out.data, out.length);
                        keyfile[out.length] = '\0';
                        privateKey = loadCertificateFile(keyfile);
                        UA_String_clear(&out);
                        UA_free(keyfile);
                    }
                } else if(strcmp(field_str, "policy") == 0) {
                    parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &policy, NULL);
                } else {
                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
                }
                UA_free(field_str);
                break;
            }
            default:
                break;
            }
        }

        if(certificate.length == 0 || privateKey.length == 0) {
            UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
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
                UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                               "Could not add SecurityPolicy#Basic128Rsa15 with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &basic256uri)) {
            retval = UA_ServerConfig_addSecurityPolicyBasic256(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                               "Could not add SecurityPolicy#Basic256 with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &basic256Sha256uri)) {
            retval = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                               "Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else if(UA_String_equal(&policy, &aes128sha256rsaoaepuri)) {
            retval = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &certificate, &privateKey);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND,
                               "Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                               UA_StatusCode_name(retval));
            }
        } else {
            UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_USERLAND, "Unknown Security Policy.");
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

PARSE_JSON(SecurityPkiField) {
#ifdef UA_ENABLE_ENCRYPTION
    UA_CertificateGroup *field = (UA_CertificateGroup*)configField;
    UA_String trustListFolder = {.length = 0, .data = NULL};
    UA_String issuerListFolder = {.length = 0, .data = NULL};
    UA_String revocationListFolder = {.length = 0, .data = NULL};

    cj5_token tok = ctx->tokens[++ctx->index];
    for(size_t i = tok.size/2; i > 0; i--) {
        tok = ctx->tokens[++ctx->index];
        switch(tok.type) {
        case CJ5_TOKEN_STRING: {
            char *field_str = (char*)UA_malloc(tok.size + 1);
            unsigned int str_len = 0;
            cj5_get_str(&ctx->result, (unsigned int)ctx->index, field_str, &str_len);
            if(strcmp(field_str, "trustListFolder") == 0) {
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &trustListFolder, NULL);
            } else if(strcmp(field_str, "issuerListFolder") == 0) {
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &issuerListFolder, NULL);
            } else if(strcmp(field_str, "revocationListFolder") == 0) {
                parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRING](ctx, &revocationListFolder, NULL);
            } else {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Unknown field name.");
            }
            UA_free(field_str);
            break;
        }
        default:
            break;
        }
    }
#ifndef __linux__
    /* Currently not supported! */
    (void)field;
    return UA_STATUSCODE_GOOD;
#else
    /* set server config field */
    char *sTrustListFolder = NULL;
    char *sIssuerListFolder = NULL;
    char *sRevocationListFolder = NULL;
    if(trustListFolder.length > 0) {
        sTrustListFolder = (char*)UA_malloc(trustListFolder.length+1);
        memcpy(sTrustListFolder, trustListFolder.data, trustListFolder.length);
        sTrustListFolder[trustListFolder.length] = '\0';
    }
    if(issuerListFolder.length > 0) {
        sIssuerListFolder = (char*)UA_malloc(issuerListFolder.length+1);
        memcpy(sIssuerListFolder, issuerListFolder.data, issuerListFolder.length);
        sIssuerListFolder[issuerListFolder.length] = '\0';
    }
    if(revocationListFolder.length > 0) {
        sRevocationListFolder = (char*)UA_malloc(revocationListFolder.length+1);
        memcpy(sRevocationListFolder, revocationListFolder.data, revocationListFolder.length);
        sRevocationListFolder[revocationListFolder.length] = '\0';
    }
    if(field && field->clear)
        field->clear(field);
#ifdef UA_ENABLE_CERT_REJECTED_DIR
    UA_StatusCode retval = UA_CertificateVerification_CertFolders(field, sTrustListFolder,
                                                                  sIssuerListFolder, sRevocationListFolder, NULL);
#else
    UA_StatusCode retval = UA_CertificateVerification_CertFolders(field, sTrustListFolder,
                                                                  sIssuerListFolder, sRevocationListFolder);
#endif
    /* Clean up */
    if(sTrustListFolder)
        UA_free(sTrustListFolder);
    if(sIssuerListFolder)
        UA_free(sIssuerListFolder);
    if(sRevocationListFolder)
        UA_free(sRevocationListFolder);
    UA_String_clear(&trustListFolder);
    UA_String_clear(&issuerListFolder);
    UA_String_clear(&revocationListFolder);

    return retval;
#endif
#endif
    return UA_STATUSCODE_GOOD;
}

/*----------------------Enumerations------------------------*/
PARSE_JSON(ApplicationTypeField) {
    UA_UInt32 enum_value;
    UA_StatusCode retval = UInt32Field_parseJson(ctx, &enum_value, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_ApplicationType *field = (UA_ApplicationType*)configField;
    *field = (UA_ApplicationType)enum_value;
    return retval;
}
PARSE_JSON(RuleHandlingField) {
    UA_UInt32 enum_value;
    UA_StatusCode retval = UInt32Field_parseJson(ctx, &enum_value, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    UA_RuleHandling *field = (UA_RuleHandling*)configField;
    *field = (UA_RuleHandling)enum_value;
    return retval;
}

const parseJsonSignature parseJsonJumpTable[UA_SERVERCONFIGFIELDKINDS] = {
    /* Basic Types */
    (parseJsonSignature)Int64Field_parseJson,
    (parseJsonSignature)UInt16Field_parseJson,
    (parseJsonSignature)UInt32Field_parseJson,
    (parseJsonSignature)UInt64Field_parseJson,
    (parseJsonSignature)StringField_parseJson,
    (parseJsonSignature)LocalizedTextField_parseJson,
    (parseJsonSignature)DoubleField_parseJson,
    (parseJsonSignature)BooleanField_parseJson,
    (parseJsonSignature)DurationField_parseJson,
    (parseJsonSignature)DurationRangeField_parseJson,
    (parseJsonSignature)UInt32RangeField_parseJson,

    /* Advanced Types */
    (parseJsonSignature)BuildInfo_parseJson,
    (parseJsonSignature)ApplicationDescriptionField_parseJson,
    (parseJsonSignature)StringArrayField_parseJson,
    (parseJsonSignature)UInt32ArrayField_parseJson,
    (parseJsonSignature)DateTimeField_parseJson,
    (parseJsonSignature)SubscriptionConfigurationField_parseJson,
    (parseJsonSignature)TcpConfigurationField_parseJson,
    (parseJsonSignature)PubsubConfigurationField_parseJson,
    (parseJsonSignature)HistorizingConfigurationField_parseJson,
    (parseJsonSignature)MdnsConfigurationField_parseJson,
    (parseJsonSignature)SecurityPolciesField_parseJson,
    (parseJsonSignature)SecurityPkiField_parseJson,

    /* Enumerations */
    (parseJsonSignature)ApplicationTypeField_parseJson,
    (parseJsonSignature)RuleHandlingField_parseJson,
};

static UA_StatusCode
parseJSONConfig(UA_ServerConfig *config, UA_ByteString json_config) {
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
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BUILDINFO](&ctx, &config->buildInfo, NULL);
                else if(strcmp(field, "applicationDescription") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_APPLICATIONDESCRIPTION](&ctx, &config->applicationDescription, NULL);
                else if(strcmp(field, "shutdownDelay") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_DOUBLE](&ctx, &config->shutdownDelay, NULL);
                else if(strcmp(field, "verifyRequestTimestamp") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_RULEHANDLING](&ctx, &config->verifyRequestTimestamp, NULL);
                else if(strcmp(field, "allowEmptyVariables") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_RULEHANDLING](&ctx, &config->allowEmptyVariables, NULL);
                else if(strcmp(field, "serverUrls") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_STRINGARRAY](&ctx, &config->serverUrls, &config->serverUrlsSize);
                else if(strcmp(field, "tcpEnabled") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->tcpEnabled, NULL);
                else if(strcmp(field, "tcp") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_TCPCONFIGURATION](&ctx, config, NULL);
                else if(strcmp(field, "securityPolicyNoneDiscoveryOnly") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->securityPolicyNoneDiscoveryOnly, NULL);
                else if(strcmp(field, "modellingRulesOnInstances") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->modellingRulesOnInstances, NULL);
                else if(strcmp(field, "maxSecureChannels") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT16](&ctx, &config->maxSecureChannels, NULL);
                else if(strcmp(field, "maxSecurityTokenLifetime") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxSecurityTokenLifetime, NULL);
                else if(strcmp(field, "maxSessions") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT16](&ctx, &config->maxSessions, NULL);
                else if(strcmp(field, "maxSessionTimeout") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_DOUBLE](&ctx, &config->maxSessionTimeout, NULL);
                else if(strcmp(field, "maxNodesPerRead") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerRead, NULL);
                else if(strcmp(field, "maxNodesPerWrite") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerWrite, NULL);
                else if(strcmp(field, "maxNodesPerMethodCall") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerMethodCall, NULL);
                else if(strcmp(field, "maxNodesPerBrowse") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerBrowse, NULL);
                else if(strcmp(field, "maxNodesPerRegisterNodes") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerRegisterNodes, NULL);
                else if(strcmp(field, "maxNodesPerTranslateBrowsePathsToNodeIds") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerTranslateBrowsePathsToNodeIds, NULL);
                else if(strcmp(field, "maxNodesPerNodeManagement") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxNodesPerNodeManagement, NULL);
                else if(strcmp(field, "maxMonitoredItemsPerCall") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxMonitoredItemsPerCall, NULL);
                else if(strcmp(field, "maxReferencesPerNode") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->maxReferencesPerNode, NULL);
                else if(strcmp(field, "reverseReconnectInterval") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->reverseReconnectInterval, NULL);

#if UA_MULTITHREADING >= 100
                else if(strcmp(field, "asyncOperationTimeout") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_DOUBLE](&ctx, &config->asyncOperationTimeout, NULL);
                else if(strcmp(field, "maxAsyncOperationQueueSize") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT64](&ctx, &config->maxAsyncOperationQueueSize, NULL);
#endif

#ifdef UA_ENABLE_DISCOVERY
                else if(strcmp(field, "discoveryCleanupTimeout") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32](&ctx, &config->discoveryCleanupTimeout, NULL);
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
                else if(strcmp(field, "mdnsEnabled") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->mdnsEnabled, NULL);
                else if(strcmp(field, "mdns") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_MDNSCONFIGURATION](&ctx, config, NULL);
#if !defined(UA_HAS_GETIFADDR)
                else if(strcmp(field, "mdnsIpAddressList") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_UINT32ARRAY](&ctx, &config->mdnsIpAddressList, &config->mdnsIpAddressListSize);
#endif
#endif
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
                else if(strcmp(field, "subscriptionsEnabled") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->subscriptionsEnabled, NULL);
                else if(strcmp(field, "subscriptions") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_SUBSCRIPTIONCONFIGURATION](&ctx, config, NULL);
# endif

#ifdef UA_ENABLE_HISTORIZING
                else if(strcmp(field, "historizingEnabled") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->historizingEnabled, NULL);
                else if(strcmp(field, "historizing") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_HISTORIZINGCONFIGURATION](&ctx, config, NULL);
#endif

#ifdef UA_ENABLE_PUBSUB
                else if(strcmp(field, "pubsubEnabled") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_BOOLEAN](&ctx, &config->pubsubEnabled, NULL);
                else if(strcmp(field, "pubsub") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_PUBSUBCONFIGURATION](&ctx, config, NULL);
#endif
#ifdef UA_ENABLE_ENCRYPTION
                else if(strcmp(field, "securityPolicies") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_SECURITYPOLICIES](&ctx, config, NULL);
                else if(strcmp(field, "secureChannelPKI") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_SECURITYPKI](&ctx, &config->secureChannelPKI, NULL);
                else if(strcmp(field, "sessionPKI") == 0)
                    retval = parseJsonJumpTable[UA_SERVERCONFIGFIELD_SECURITYPKI](&ctx, &config->sessionPKI, NULL);
#endif
                else {
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Field name '%s' unknown or misspelled. Maybe the feature is not enabled either.", field);
                }
                UA_free(field);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "An error occurred while parsing the configuration file.");
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
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setDefault(&config);
    res |= parseJSONConfig(&config, json_config);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;
    return UA_Server_newWithConfig(&config);
}

UA_StatusCode
UA_ServerConfig_updateFromFile(UA_ServerConfig *config, const UA_ByteString json_config) {
    UA_StatusCode res = parseJSONConfig(config, json_config);
    return res;
}

#ifdef UA_ENABLE_ENCRYPTION
static UA_ByteString
loadCertificateFile(const char *const path) {
    UA_ByteString fileContents = UA_STRING_NULL;

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
