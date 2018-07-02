/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_LOG_H_
#define UA_PLUGIN_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "ua_config.h"

#include "ua_types.h"
#include "ua_types_generated_handling.h"

/**
 * Logging Plugin API
 * ==================
 *
 * Servers and clients must define a logger in their configuration. The logger
 * is just a function pointer. Every log-message consists of a log-level, a
 * log-category and a string message content. The timestamp of the log-message
 * is created within the logger. */

typedef enum {
    UA_LOGLEVEL_TRACE,
    UA_LOGLEVEL_DEBUG,
    UA_LOGLEVEL_INFO,
    UA_LOGLEVEL_WARNING,
    UA_LOGLEVEL_ERROR,
    UA_LOGLEVEL_FATAL
} UA_LogLevel;

typedef enum {
    UA_LOGCATEGORY_NETWORK,
    UA_LOGCATEGORY_SECURECHANNEL,
    UA_LOGCATEGORY_SESSION,
    UA_LOGCATEGORY_SERVER,
    UA_LOGCATEGORY_CLIENT,
    UA_LOGCATEGORY_USERLAND,
    UA_LOGCATEGORY_SECURITYPOLICY
} UA_LogCategory;

/**
 * The message string and following varargs are formatted according to the rules
 * of the printf command. Do not call the logger directly. Instead, make use of
 * the convenience macros that take the minimum log-level defined in ua_config.h
 * into account. */

typedef void (*UA_Logger)(UA_LogLevel level, UA_LogCategory category,
                          const char *msg, va_list args);

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_TRACE(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 100
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_TRACE, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_DEBUG(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 200
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_DEBUG, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_INFO(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 300
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_INFO, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_WARNING(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 400
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_WARNING, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_ERROR(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 500
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_ERROR, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_FATAL(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 600
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_FATAL, category, msg, args);
    va_end(args);
#endif
}

/**
 * Convenience macros for complex types
 * ------------------------------------ */
#define UA_PRINTF_GUID_FORMAT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define UA_PRINTF_GUID_DATA(GUID) (GUID).data1, (GUID).data2, (GUID).data3, \
        (GUID).data4[0], (GUID).data4[1], (GUID).data4[2], (GUID).data4[3], \
        (GUID).data4[4], (GUID).data4[5], (GUID).data4[6], (GUID).data4[7]

#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
#define UA_PRINTF_STRING_DATA(STRING) (int)(STRING).length, (STRING).data

//TODO remove when we merge architectures pull request
#ifndef UA_snprintf
# include <stdio.h>
# if defined(_WIN32)
#  define UA_snprintf(source, size, string, ...) _snprintf_s(source, size, _TRUNCATE, string, __VA_ARGS__)
# else
#  define UA_snprintf snprintf
# endif
#endif

static UA_INLINE UA_StatusCode
UA_ByteString_toString(const UA_ByteString *byteString, UA_String *str) {
    if (str->length != 0) {
        UA_free(str->data);
        str->data = NULL;
        str->length = 0;
    }
    if (byteString == NULL || byteString->data == NULL)
        return UA_STATUSCODE_GOOD;
    if (byteString == str)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    str->length = byteString->length*2;
    str->data = (UA_Byte*)UA_malloc(str->length+1);
    if (str->data == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    for (size_t i=0; i<byteString->length; i++)
        UA_snprintf((char*)&str->data[i*2], 2+1, "%02x", byteString->data[i]);
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE UA_StatusCode
UA_NodeId_toString(const UA_NodeId *nodeId, UA_String *nodeIdStr) {
    if (nodeIdStr->length != 0) {
        UA_free(nodeIdStr->data);
        nodeIdStr->data = NULL;
        nodeIdStr->length = 0;
    }
    if (nodeId == NULL)
        return UA_STATUSCODE_GOOD;


    UA_ByteString byteStr = UA_BYTESTRING_NULL;
    switch (nodeId->identifierType) {
        /* for all the lengths below we add the constant for: */
        /* strlen("ns=XXXXXX;i=")=11 */
        case UA_NODEIDTYPE_NUMERIC:
            /* ns (2 byte, 65535) = 5 chars, numeric (4 byte, 4294967295) = 10 chars, delim = 1 , nullbyte = 1-> 17 chars */
            nodeIdStr->length = 11 + 10 + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "ns=%d;i=%lu",
                        nodeId->namespaceIndex, (unsigned long )nodeId->identifier.numeric);
            break;
        case UA_NODEIDTYPE_STRING:
            /* ns (16bit) = 5 chars, strlen + nullbyte */
            nodeIdStr->length = 11 + nodeId->identifier.string.length + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "ns=%d;i=%.*s", nodeId->namespaceIndex,
                        (int)nodeId->identifier.string.length, nodeId->identifier.string.data);
            break;
        case UA_NODEIDTYPE_GUID:
            /* ns (16bit) = 5 chars + strlen(A123456C-0ABC-1A2B-815F-687212AAEE1B)=36 + nullbyte */
            nodeIdStr->length = 11 + 36 + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "ns=%d;i=" UA_PRINTF_GUID_FORMAT,
                        nodeId->namespaceIndex, UA_PRINTF_GUID_DATA(nodeId->identifier.guid));
            break;
        case UA_NODEIDTYPE_BYTESTRING:
            UA_ByteString_toString(&nodeId->identifier.byteString, &byteStr);
            /* ns (16bit) = 5 chars + LEN + nullbyte */
            nodeIdStr->length = 11 + byteStr.length + 1;
            nodeIdStr->data = (UA_Byte*)UA_malloc(nodeIdStr->length);
            if (nodeIdStr->data == NULL)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            UA_snprintf((char*)nodeIdStr->data, nodeIdStr->length, "ns=%d;i=%.*s", nodeId->namespaceIndex, (int)byteStr.length, byteStr.data);
            UA_String_deleteMembers(&byteStr);
            break;
    }
    return UA_STATUSCODE_GOOD;
}


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PLUGIN_LOG_H_ */
