/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>

#include "ua_util_internal.h"

/* Printing of NodeIds is always enabled. We need it for logging. */

UA_StatusCode
UA_NodeId_print(const UA_NodeId *id, UA_String *output) {
    UA_String_clear(output);
    if(!id)
        return UA_STATUSCODE_GOOD;

    char *nsStr = NULL;
    long snprintfLen = 0;
    size_t nsLen = 0;
    if(id->namespaceIndex != 0) {
        nsStr = (char*)UA_malloc(9+1); // strlen("ns=XXXXX;") = 9 + Nullbyte
        if(!nsStr)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        snprintfLen = UA_snprintf(nsStr, 10, "ns=%d;", id->namespaceIndex);
        if(snprintfLen < 0 || snprintfLen >= 10) {
            UA_free(nsStr);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        nsLen = (size_t)(snprintfLen);
    }

    UA_ByteString byteStr = UA_BYTESTRING_NULL;
    switch (id->identifierType) {
        case UA_NODEIDTYPE_NUMERIC:
            /* ns (2 byte, 65535) = 5 chars, numeric (4 byte, 4294967295) = 10
             * chars, delim = 1 , nullbyte = 1-> 17 chars */
            output->length = nsLen + 2 + 10 + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length, "%si=%lu",
                                      nsLen > 0 ? nsStr : "",
                                      (unsigned long )id->identifier.numeric);
            break;
        case UA_NODEIDTYPE_STRING:
            /* ns (16bit) = 5 chars, strlen + nullbyte */
            output->length = nsLen + 2 + id->identifier.string.length + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length, "%ss=%.*s",
                                      nsLen > 0 ? nsStr : "", (int)id->identifier.string.length,
                                      id->identifier.string.data);
            break;
        case UA_NODEIDTYPE_GUID:
            /* ns (16bit) = 5 chars + strlen(A123456C-0ABC-1A2B-815F-687212AAEE1B)=36 + nullbyte */
            output->length = nsLen + 2 + 36 + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length,
                                      "%sg=" UA_PRINTF_GUID_FORMAT, nsLen > 0 ? nsStr : "",
                                      UA_PRINTF_GUID_DATA(id->identifier.guid));
            break;
        case UA_NODEIDTYPE_BYTESTRING:
            UA_ByteString_toBase64(&id->identifier.byteString, &byteStr);
            /* ns (16bit) = 5 chars + LEN + nullbyte */
            output->length = nsLen + 2 + byteStr.length + 1;
            output->data = (UA_Byte*)UA_malloc(output->length);
            if(output->data == NULL) {
                output->length = 0;
                UA_String_clear(&byteStr);
                UA_free(nsStr);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            snprintfLen = UA_snprintf((char*)output->data, output->length, "%sb=%.*s",
                                      nsLen > 0 ? nsStr : "",
                                      (int)byteStr.length, byteStr.data);
            UA_String_clear(&byteStr);
            break;
    }
    UA_free(nsStr);

    if(snprintfLen < 0 || snprintfLen >= (long) output->length) {
        UA_free(output->data);
        output->data = NULL;
        output->length = 0;
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    output->length = (size_t)snprintfLen;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ExpandedNodeId_print(const UA_ExpandedNodeId *id, UA_String *output) {
    /* Don't print the namespace-index if a NamespaceUri is set */
    UA_NodeId nid = id->nodeId;
    if(id->namespaceUri.data != NULL)
        nid.namespaceIndex = 0;

    /* Encode the NodeId */
    UA_String outNid = UA_STRING_NULL;
    UA_StatusCode res = UA_NodeId_print(&nid, &outNid);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Encode the ServerIndex */
    char svr[100];
    if(id->serverIndex == 0)
        svr[0] = 0;
    else
        UA_snprintf(svr, 100, "svr=%"PRIu32";", id->serverIndex);
    size_t svrlen = strlen(svr);

    /* Encode the NamespaceUri */
    char nsu[100];
    if(id->namespaceUri.data == NULL)
        nsu[0] = 0;
    else
        UA_snprintf(nsu, 100, "nsu=%.*s;", (int)id->namespaceUri.length, id->namespaceUri.data);
    size_t nsulen = strlen(nsu);

    /* Combine everything */
    res = UA_ByteString_allocBuffer((UA_String*)output, outNid.length + svrlen + nsulen);
    if(res == UA_STATUSCODE_GOOD) {
        memcpy(output->data, svr, svrlen);
        memcpy(&output->data[svrlen], nsu, nsulen);
        memcpy(&output->data[svrlen+nsulen], outNid.data, outNid.length);
    }

    UA_String_clear(&outNid);
    return res;
}

#ifdef UA_ENABLE_JSON_ENCODING

UA_StatusCode
UA_print(const void *p, const UA_DataType *type, UA_String *output) {
    if(!p || !type || !output)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set up the encoding options */
    UA_EncodeJsonOptions options = {0};
    options.useReversible = true;
    options.prettyPrint = true;
    options.unquotedKeys = true;
    options.stringNodeIds = true;

    /* If no buffer is provided, compute the output length and allocate */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(output->length == 0) {
        size_t len = UA_calcSizeJson(p, type, &options);
        if(len == 0)
            return UA_STATUSCODE_BADENCODINGERROR;
        res = UA_ByteString_allocBuffer(output, len);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Encode and return */
    return UA_encodeJson(p, type, output, &options);
}

#endif
