/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014, 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

/* If UA_ENABLE_INLINABLE_EXPORT is enabled, then this file is the compilation
 * unit for the generated code from UA_INLINABLE definitions. */
#define UA_INLINABLE_IMPL 1

#include <open62541/types_generated_handling.h>
#include <open62541/server.h>
#include <open62541/util.h>
#include <open62541/common.h>

#include "ua_util_internal.h"
#include "pcg_basic.h"
#include "base64.h"
#include "itoa.h"

const char * attributeIdNames[28] = {
    "Invalid", "NodeId", "NodeClass", "BrowseName", "DisplayName", "Description",
    "WriteMask", "UserWriteMask", "IsAbstract", "Symmetric", "InverseName",
    "ContainsNoLoops", "EventNotifier", "Value", "DataType", "ValueRank",
    "ArrayDimensions", "AccessLevel", "UserAccessLevel", "MinimumSamplingInterval",
    "Historizing", "Executable", "UserExecutable", "DataTypeDefinition",
    "RolePermissions", "UserRolePermissions", "AccessRestrictions", "AccessLevelEx"
};

const char *
UA_AttributeId_name(UA_AttributeId attrId) {
    if(attrId < 0 || attrId > UA_ATTRIBUTEID_ACCESSLEVELEX)
        return attributeIdNames[0];
    return attributeIdNames[attrId];
}

/* OR-ing 32 goes from upper-case to lower-case */
UA_AttributeId
UA_AttributeId_fromName(const UA_String name) {
    for(size_t i = 0; i < 28; i++) {
        if(strlen(attributeIdNames[i]) != name.length)
            continue;
        for(size_t j = 0; j < name.length; j++) {
            if((attributeIdNames[i][j] | 32) != (name.data[j] | 32))
                goto next;
        }
        return (UA_AttributeId)i;
    next:
        continue;
    }
    return UA_ATTRIBUTEID_INVALID;
}

size_t
UA_readNumberWithBase(const UA_Byte *buf, size_t buflen, UA_UInt32 *number, UA_Byte base) {
    UA_assert(buf);
    UA_assert(number);
    u32 n = 0;
    size_t progress = 0;
    /* read numbers until the end or a non-number character appears */
    while(progress < buflen) {
        u8 c = buf[progress];
        if(c >= '0' && c <= '9' && c <= '0' + (base-1))
           n = (n * base) + c - '0';
        else if(base > 9 && c >= 'a' && c <= 'z' && c <= 'a' + (base-11))
           n = (n * base) + c-'a' + 10;
        else if(base > 9 && c >= 'A' && c <= 'Z' && c <= 'A' + (base-11))
           n = (n * base) + c-'A' + 10;
        else
           break;
        ++progress;
    }
    *number = n;
    return progress;
}

size_t
UA_readNumber(const UA_Byte *buf, size_t buflen, UA_UInt32 *number) {
    return UA_readNumberWithBase(buf, buflen, number, 10);
}

struct urlSchema {
    const char *schema;
};

static const struct urlSchema schemas[] = {
    {"opc.tcp://"},
    {"opc.udp://"},
    {"opc.eth://"},
    {"opc.mqtt://"}
};

static const unsigned scNumSchemas = sizeof(schemas) / sizeof(schemas[0]);
static const unsigned scEthSchemaIdx = 2;

UA_StatusCode
UA_parseEndpointUrl(const UA_String *endpointUrl, UA_String *outHostname,
                    UA_UInt16 *outPort, UA_String *outPath) {
    /* Url must begin with "opc.tcp://" or opc.udp:// (if pubsub enabled) */
    if(endpointUrl->length < 11) {
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    }

    /* Which type of schema is this? */
    unsigned schemaType = 0;
    for(; schemaType < scNumSchemas; schemaType++) {
        if(strncmp((char*)endpointUrl->data,
                   schemas[schemaType].schema,
                   strlen(schemas[schemaType].schema)) == 0)
            break;
    }
    if(schemaType == scNumSchemas)
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;

    /* Forward the current position until the first colon or slash */
    size_t start = strlen(schemas[schemaType].schema);
    size_t curr = start;
    UA_Boolean ipv6 = false;
    if(endpointUrl->length > curr && endpointUrl->data[curr] == '[') {
        /* IPv6: opc.tcp://[2001:0db8:85a3::8a2e:0370:7334]:1234/path */
        for(; curr < endpointUrl->length; ++curr) {
            if(endpointUrl->data[curr] == ']')
                break;
        }
        if(curr == endpointUrl->length)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        curr++;
        ipv6 = true;
    } else {
        /* IPv4 or hostname: opc.tcp://something.something:1234/path */
        for(; curr < endpointUrl->length; ++curr) {
            if(endpointUrl->data[curr] == ':' || endpointUrl->data[curr] == '/')
                break;
        }
    }

    /* Set the hostname */
    if(ipv6) {
        /* Skip the ipv6 '[]' container for getaddrinfo() later */
        outHostname->data = &endpointUrl->data[start+1];
        outHostname->length = curr - (start+2);
    } else {
        outHostname->data = &endpointUrl->data[start];
        outHostname->length = curr - start;
    }

    /* Empty string? */
    if(outHostname->length == 0)
        outHostname->data = NULL;

    /* Already at the end */
    if(curr == endpointUrl->length)
        return UA_STATUSCODE_GOOD;

    /* Set the port - and for ETH set the VID.PCP postfix in the outpath string.
     * We have to parse that externally. */
    if(endpointUrl->data[curr] == ':') {
        if(++curr == endpointUrl->length)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;

        /* ETH schema */
        if(schemaType == scEthSchemaIdx) {
            if(outPath != NULL) {
                outPath->data = &endpointUrl->data[curr];
                outPath->length = endpointUrl->length - curr;
            }
            return UA_STATUSCODE_GOOD;
        }

        u32 largeNum;
        size_t progress = UA_readNumber(&endpointUrl->data[curr],
                                        endpointUrl->length - curr, &largeNum);
        if(progress == 0 || largeNum > 65535)
            return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
        /* Test if the end of a valid port was reached */
        curr += progress;
        if(curr == endpointUrl->length || endpointUrl->data[curr] == '/')
            *outPort = (u16)largeNum;
        if(curr == endpointUrl->length)
            return UA_STATUSCODE_GOOD;
    }

    /* Set the path */
    UA_assert(curr < endpointUrl->length);
    if(endpointUrl->data[curr] != '/')
        return UA_STATUSCODE_BADTCPENDPOINTURLINVALID;
    if(++curr == endpointUrl->length)
        return UA_STATUSCODE_GOOD;
    if(outPath != NULL) {
        outPath->data = &endpointUrl->data[curr];
        outPath->length = endpointUrl->length - curr;

        /* Remove trailing slash from the path */
        if(endpointUrl->data[endpointUrl->length - 1] == '/')
            outPath->length--;

        /* Empty string? */
        if(outPath->length == 0)
            outPath->data = NULL;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_parseEndpointUrlEthernet(const UA_String *endpointUrl, UA_String *target,
                            UA_UInt16 *vid, UA_Byte *pcp) {
    /* Url must begin with "opc.eth://" */
    if(endpointUrl->length < 11) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(strncmp((char*) endpointUrl->data, "opc.eth://", 10) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Where does the host address end? */
    size_t curr = 10;
    for(; curr < endpointUrl->length; ++curr) {
        if(endpointUrl->data[curr] == ':') {
           break;
        }
    }

    /* set host address */
    target->data = &endpointUrl->data[10];
    target->length = curr - 10;
    if(curr == endpointUrl->length) {
        return UA_STATUSCODE_GOOD;
    }

    /* Set VLAN */
    u32 value = 0;
    curr++;  /* skip ':' */
    size_t progress = UA_readNumber(&endpointUrl->data[curr],
                                    endpointUrl->length - curr, &value);
    if(progress == 0 || value > 4096) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    curr += progress;
    if(curr == endpointUrl->length || endpointUrl->data[curr] == '.') {
        *vid = (UA_UInt16) value;
    }
    if(curr == endpointUrl->length) {
        return UA_STATUSCODE_GOOD;
    }

    /* Set priority */
    if(endpointUrl->data[curr] != '.') {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    curr++;  /* skip '.' */
    progress = UA_readNumber(&endpointUrl->data[curr],
                             endpointUrl->length - curr, &value);
    if(progress == 0 || value > 7) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    curr += progress;
    if(curr != endpointUrl->length) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    *pcp = (UA_Byte) value;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ByteString_toBase64(const UA_ByteString *byteString,
                       UA_String *str) {
    UA_String_init(str);
    if(!byteString || !byteString->data)
        return UA_STATUSCODE_GOOD;

    str->data = (UA_Byte*)
        UA_base64(byteString->data, byteString->length, &str->length);
    if(!str->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ByteString_fromBase64(UA_ByteString *bs,
                         const UA_String *input) {
    UA_ByteString_init(bs);
    if(input->length == 0)
        return UA_STATUSCODE_GOOD;
    bs->data = UA_unbase64((const unsigned char*)input->data,
                           input->length, &bs->length);
    /* TODO: Differentiate between encoding and memory errors */
    if(!bs->data)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/* Key Value Map */

const UA_KeyValueMap UA_KEYVALUEMAP_NULL = {0, NULL};

UA_KeyValueMap *
UA_KeyValueMap_new(void) {
    return (UA_KeyValueMap*)UA_calloc(1, sizeof(UA_KeyValueMap));
}

UA_StatusCode
UA_KeyValueMap_set(UA_KeyValueMap *map,
                   const UA_QualifiedName key,
                   const UA_Variant *value) {
    if(map == NULL || value == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Key exists already */
    const UA_Variant *v = UA_KeyValueMap_get(map, key);
    if(v) {
        UA_Variant copyV;
        UA_StatusCode res = UA_Variant_copy(value, &copyV);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        UA_Variant *target = (UA_Variant*)(uintptr_t)v;
        UA_Variant_clear(target);
        *target = copyV;
        return UA_STATUSCODE_GOOD;
    }

    /* Append to the array */
    UA_KeyValuePair pair;
    pair.key = key;
    pair.value = *value;
    return UA_Array_appendCopy((void**)&map->map, &map->mapSize, &pair,
                               &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
}

UA_StatusCode
UA_KeyValueMap_setScalar(UA_KeyValueMap *map,
                         const UA_QualifiedName key,
                         void * UA_RESTRICT p,
                         const UA_DataType *type) {
    if(p == NULL || type == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Variant v;
    UA_Variant_init(&v);
    v.type = type;
    v.arrayLength = 0;
    v.data = p;
    return UA_KeyValueMap_set(map, key, &v);
}

const UA_Variant *
UA_KeyValueMap_get(const UA_KeyValueMap *map,
                   const UA_QualifiedName key) {
    if(!map)
        return NULL;
    for(size_t i = 0; i < map->mapSize; i++) {
        if(map->map[i].key.namespaceIndex == key.namespaceIndex &&
           UA_String_equal(&map->map[i].key.name, &key.name))
            return &map->map[i].value;

    }
    return NULL;
}

UA_Boolean
UA_KeyValueMap_isEmpty(const UA_KeyValueMap *map) {
    if(!map)
        return true;
    return map->mapSize == 0;
}

const void *
UA_KeyValueMap_getScalar(const UA_KeyValueMap *map,
                         const UA_QualifiedName key,
                         const UA_DataType *type) {
    const UA_Variant *v = UA_KeyValueMap_get(map, key);
    if(!v || !UA_Variant_hasScalarType(v, type))
        return NULL;
    return v->data;
}

void
UA_KeyValueMap_clear(UA_KeyValueMap *map) {
    if(!map)
        return;
    if(map->mapSize > 0) {
        UA_Array_delete(map->map, map->mapSize, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
        map->mapSize = 0;
    }
}

void
UA_KeyValueMap_delete(UA_KeyValueMap *map) {
    UA_KeyValueMap_clear(map);
    UA_free(map);
}

UA_StatusCode
UA_KeyValueMap_remove(UA_KeyValueMap *map,
                      const UA_QualifiedName key) {
    if(!map)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_KeyValuePair *m = map->map;
    size_t s = map->mapSize;
    size_t i = 0;
    for(; i < s; i++) {
        if(m[i].key.namespaceIndex == key.namespaceIndex &&
           UA_String_equal(&m[i].key.name, &key.name))
            break;
    }
    if(i == s)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Clean the slot and move the last entry to fill the slot */
    UA_KeyValuePair_clear(&m[i]);
    if(s > 1 && i < s - 1) {
        m[i] = m[s-1];
        UA_KeyValuePair_init(&m[s-1]);
    }
    
    /* Ignore the result. In case resize fails, keep the longer original array
     * around. Resize never fails when reducing the size to zero. Reduce the
     * size integer in any case. */
    UA_StatusCode res =
        UA_Array_resize((void**)&map->map, &map->mapSize, map->mapSize - 1,
                          &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    (void)res;
    map->mapSize--;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_KeyValueMap_copy(const UA_KeyValueMap *src, UA_KeyValueMap *dst) {
    if(!dst)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!src) {
        dst->map = NULL;
        dst->mapSize = 0;
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode res = UA_Array_copy(src->map, src->mapSize, (void**)&dst->map,
                                      &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res == UA_STATUSCODE_GOOD)
        dst->mapSize = src->mapSize;
    return res;
}

UA_Boolean
UA_KeyValueMap_contains(const UA_KeyValueMap *map, const UA_QualifiedName key) {
    if(!map)
        return false;
    for(size_t i = 0; i < map->mapSize; ++i) {
        if(UA_QualifiedName_equal(&map->map[i].key, &key))
            return true;
    }
    return false;
}

UA_StatusCode
UA_KeyValueMap_merge(UA_KeyValueMap *lhs, const UA_KeyValueMap *rhs) {
    if(!lhs)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!rhs)
        return UA_STATUSCODE_GOOD;

    UA_KeyValueMap merge;
    UA_StatusCode res = UA_KeyValueMap_copy(lhs, &merge);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    for(size_t i = 0; i < rhs->mapSize; ++i) {
        res = UA_KeyValueMap_set(&merge, rhs->map[i].key, &rhs->map[i].value);
        if(res != UA_STATUSCODE_GOOD) {
            UA_KeyValueMap_clear(&merge);
            return res;
        }
    }

    UA_KeyValueMap_clear(lhs);
    *lhs = merge;
    return UA_STATUSCODE_GOOD;
}

/***************************/
/* Random Number Generator */
/***************************/

/* TODO is this safe for multithreading? */
static pcg32_random_t UA_rng = PCG32_INITIALIZER;

void
UA_random_seed(u64 seed) {
    pcg32_srandom_r(&UA_rng, seed, (u64)UA_DateTime_now());
}

u32
UA_UInt32_random(void) {
    return (u32)pcg32_random_r(&UA_rng);
}

UA_Guid
UA_Guid_random(void) {
    UA_Guid result;
    result.data1 = (u32)pcg32_random_r(&UA_rng);
    u32 r = (u32)pcg32_random_r(&UA_rng);
    result.data2 = (u16) r;
    result.data3 = (u16) (r >> 16);
    r = (u32)pcg32_random_r(&UA_rng);
    result.data4[0] = (u8)r;
    result.data4[1] = (u8)(r >> 4);
    result.data4[2] = (u8)(r >> 8);
    result.data4[3] = (u8)(r >> 12);
    r = (u32)pcg32_random_r(&UA_rng);
    result.data4[4] = (u8)r;
    result.data4[5] = (u8)(r >> 4);
    result.data4[6] = (u8)(r >> 8);
    result.data4[7] = (u8)(r >> 12);
    return result;
}

/********************/
/* Malloc Singleton */
/********************/

#ifdef UA_ENABLE_MALLOC_SINGLETON
# include <stdlib.h>
UA_EXPORT UA_THREAD_LOCAL void * (*UA_mallocSingleton)(size_t size) = malloc;
UA_EXPORT UA_THREAD_LOCAL void (*UA_freeSingleton)(void *ptr) = free;
UA_EXPORT UA_THREAD_LOCAL void * (*UA_callocSingleton)(size_t nelem, size_t elsize) = calloc;
UA_EXPORT UA_THREAD_LOCAL void * (*UA_reallocSingleton)(void *ptr, size_t size) = realloc;
#endif

/************************/
/* ReferenceType Lookup */
/************************/

typedef struct {
    UA_String browseName;
    UA_UInt32 identifier;
} RefTypeName;

#define KNOWNREFTYPES 17
static const RefTypeName knownRefTypes[KNOWNREFTYPES] = {
    {UA_STRING_STATIC("References"), UA_NS0ID_REFERENCES},
    {UA_STRING_STATIC("HierachicalReferences"), UA_NS0ID_HIERARCHICALREFERENCES},
    {UA_STRING_STATIC("NonHierachicalReferences"), UA_NS0ID_NONHIERARCHICALREFERENCES},
    {UA_STRING_STATIC("HasChild"), UA_NS0ID_HASCHILD},
    {UA_STRING_STATIC("Aggregates"), UA_NS0ID_AGGREGATES},
    {UA_STRING_STATIC("HasComponent"), UA_NS0ID_HASCOMPONENT},
    {UA_STRING_STATIC("HasProperty"), UA_NS0ID_HASPROPERTY},
    {UA_STRING_STATIC("HasOrderedComponent"), UA_NS0ID_HASORDEREDCOMPONENT},
    {UA_STRING_STATIC("HasSubtype"), UA_NS0ID_HASSUBTYPE},
    {UA_STRING_STATIC("Organizes"), UA_NS0ID_ORGANIZES},
    {UA_STRING_STATIC("HasModellingRule"), UA_NS0ID_HASMODELLINGRULE},
    {UA_STRING_STATIC("HasTypeDefinition"), UA_NS0ID_HASTYPEDEFINITION},
    {UA_STRING_STATIC("HasEncoding"), UA_NS0ID_HASENCODING},
    {UA_STRING_STATIC("GeneratesEvent"), UA_NS0ID_GENERATESEVENT},
    {UA_STRING_STATIC("AlwaysGeneratesEvent"), UA_NS0ID_ALWAYSGENERATESEVENT},
    {UA_STRING_STATIC("HasEventSource"), UA_NS0ID_HASEVENTSOURCE},
    {UA_STRING_STATIC("HasNotifier"), UA_NS0ID_HASNOTIFIER}
};

UA_StatusCode
lookupRefType(UA_Server *server, UA_QualifiedName *qn, UA_NodeId *outRefTypeId) {
    /* Check well-known ReferenceTypes first */
    if(qn->namespaceIndex == 0) {
        for(size_t i = 0; i < KNOWNREFTYPES; i++) {
            if(UA_String_equal(&qn->name, &knownRefTypes[i].browseName)) {
                *outRefTypeId = UA_NODEID_NUMERIC(0, knownRefTypes[i].identifier);
                return UA_STATUSCODE_GOOD;
            }
        }
    }

    /* Browse the information model. Return the first results if the browse name
     * in the hierarchy is ambiguous. */
    if(server) {
        UA_BrowseDescription bd;
        UA_BrowseDescription_init(&bd);
        bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES);
        bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
        bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        bd.nodeClassMask = UA_NODECLASS_REFERENCETYPE;

        size_t resultsSize = 0;
        UA_ExpandedNodeId *results = NULL;
        UA_StatusCode res =
            UA_Server_browseRecursive(server, &bd, &resultsSize, &results);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        for(size_t i = 0; i < resultsSize; i++) {
            UA_QualifiedName bn;
            UA_Server_readBrowseName(server, results[i].nodeId, &bn);
            if(UA_QualifiedName_equal(qn, &bn)) {
                UA_QualifiedName_clear(&bn);
                *outRefTypeId = results[i].nodeId;
                UA_NodeId_clear(&results[i].nodeId);
                UA_Array_delete(results, resultsSize, &UA_TYPES[UA_TYPES_NODEID]);
                return UA_STATUSCODE_GOOD;
            }
            UA_QualifiedName_clear(&bn);
        }

        UA_Array_delete(results, resultsSize, &UA_TYPES[UA_TYPES_NODEID]);
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
getRefTypeBrowseName(const UA_NodeId *refTypeId, UA_String *outBN) {
    /* Canonical name known? */
    if(refTypeId->namespaceIndex == 0 &&
       refTypeId->identifierType == UA_NODEIDTYPE_NUMERIC) {
        for(size_t i = 0; i < KNOWNREFTYPES; i++) {
            if(refTypeId->identifier.numeric != knownRefTypes[i].identifier)
                continue;
            memcpy(outBN->data, knownRefTypes[i].browseName.data, knownRefTypes[i].browseName.length);
            outBN->length = knownRefTypes[i].browseName.length;
            return UA_STATUSCODE_GOOD;
        }
    }

    /* Print the NodeId */
    return UA_NodeId_print(refTypeId, outBN);
}

/************************/
/* Printing and Parsing */
/************************/

static UA_INLINE UA_Boolean
isReserved(char c) {
    return (c == '/' || c == '.' || c == '<' || c == '>' ||
            c == ':' || c == '#' || c == '!' || c == '&');
}

static UA_INLINE UA_Boolean
isReservedExtended(char c) {
    return (isReserved(c) || c == ',' || c == '(' || c == ')' ||
            c == '[' || c == ']' || c == ' ' || c == '\t' ||
            c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

char *
find_unescaped(char *pos, char *end, UA_Boolean extended) {
    while(pos < end) {
        if(*pos == '&') {
            pos += 2;
            continue;
        }
        UA_Boolean reserved = (extended) ? isReservedExtended(*pos) : isReserved(*pos);
        if(reserved)
            return pos;
        pos++;
    }
    return end;
}

void
UA_String_unescape(UA_String *s, UA_Boolean extended) {
    UA_Byte *writepos = s->data;
    UA_Byte *end = &s->data[s->length];
    for(UA_Byte *pos = s->data; pos < end; pos++,writepos++) {
        UA_Boolean skip = (extended) ? isReservedExtended(*pos) : isReserved(*pos);
        if(skip && ++pos >= end)
            break;
        *writepos = *pos;
    }
    s->length = (size_t)(writepos - s->data);
}

UA_StatusCode
UA_String_append(UA_String *s, const UA_String s2) {
    if(s2.length == 0)
        return UA_STATUSCODE_GOOD;
    UA_Byte *buf = (UA_Byte*)UA_realloc(s->data, s->length + s2.length);
    if(!buf)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(buf + s->length, s2.data, s2.length);
    s->data = buf;
    s->length += s2.length;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_String_escapeAppend(UA_String *s, const UA_String s2, UA_Boolean extended) {
    /* Allocate memory for the qn name.
     * We allocate enough space to escape every character. */
    UA_Byte *buf = (UA_Byte*)UA_realloc(s->data, s->length + (s2.length*2));
    if(!buf)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    s->data = buf;

    /* Copy + escape s2 */
    for(size_t j = 0; j < s2.length; j++) {
        UA_Boolean reserved = (extended) ?
            isReservedExtended(s2.data[j]) : isReserved(s2.data[j]);
        if(reserved)
            s->data[s->length++] = '&';
        s->data[s->length++] = s2.data[j];
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_PARSING

static UA_StatusCode
moveTmpToOut(UA_String *tmp, UA_String *out) {
    /* Output has zero length */
    if(tmp->length == 0) {
        UA_assert(tmp->data == NULL);
        if(out->data == NULL)
            out->data = (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL;
        out->length = 0;
        return UA_STATUSCODE_GOOD;
    }

    /* No output buffer provided, return the tmp buffer */
    if(out->length == 0) {
        *out = *tmp;
        return UA_STATUSCODE_GOOD;
    }

    /* The provided buffer is too short */
    if(out->length < tmp->length) {
        UA_String_clear(tmp);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }

    /* Copy output to the provided buffer */
    memcpy(out->data, tmp->data, tmp->length);
    out->length = tmp->length;
    UA_String_clear(tmp);
    return UA_STATUSCODE_GOOD;
}

static const UA_NodeId hierarchicalRefs =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HIERARCHICALREFERENCES}};
static const UA_NodeId aggregatesRefs =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_AGGREGATES}};

UA_StatusCode
UA_RelativePath_print(const UA_RelativePath *rp, UA_String *out) {
    UA_String tmp = UA_STRING_NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < rp->elementsSize && res == UA_STATUSCODE_GOOD; i++) {
        /* Print the reference type */
        UA_RelativePathElement *elm = &rp->elements[i];
        if(UA_NodeId_equal(&hierarchicalRefs, &elm->referenceTypeId) &&
           !elm->isInverse && elm->includeSubtypes) {
            res |= UA_String_append(&tmp, UA_STRING("/"));
        } else if(UA_NodeId_equal(&aggregatesRefs, &elm->referenceTypeId) &&
                  !elm->isInverse && elm->includeSubtypes) {
            res |= UA_String_append(&tmp, UA_STRING("."));
        } else {
            res |= UA_String_append(&tmp, UA_STRING("<"));
            if(!elm->includeSubtypes)
                res |= UA_String_append(&tmp, UA_STRING("#"));
            if(elm->isInverse)
                res |= UA_String_append(&tmp, UA_STRING("!"));
            UA_Byte bnBuf[512];
            UA_String bnBufStr = {512, bnBuf};
            res |= getRefTypeBrowseName(&elm->referenceTypeId, &bnBufStr);
            if(res != UA_STATUSCODE_GOOD)
                break;
            res |= UA_String_escapeAppend(&tmp, bnBufStr, false);
            res |= UA_String_append(&tmp, UA_STRING(">"));
        }

        /* Print the qualified name namespace */
        UA_QualifiedName *qn = &elm->targetName;
        if(qn->namespaceIndex > 0) {
            char nsStr[8]; /* Enough for a uint16 */
            itoaUnsigned(qn->namespaceIndex, nsStr, 10);
            res |= UA_String_append(&tmp, UA_STRING(nsStr));
            res |= UA_String_append(&tmp, UA_STRING(":"));
        }
        res |= UA_String_escapeAppend(&tmp, qn->name, false);
    }

    /* Encoding failed, clean up */
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&tmp);
        return res;
    }

    return moveTmpToOut(&tmp, out);
}

static UA_NodeId baseEventTypeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEEVENTTYPE}};

UA_StatusCode
UA_SimpleAttributeOperand_print(const UA_SimpleAttributeOperand *sao,
                                UA_String *out) {
    UA_String tmp = UA_STRING_NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* Print the TypeDefinitionId */
    if(!UA_NodeId_equal(&baseEventTypeId, &sao->typeDefinitionId)) {
        UA_Byte nodeIdBuf[512];
        UA_String nodeIdBufStr = {512, nodeIdBuf};
        res = UA_NodeId_print(&sao->typeDefinitionId, &nodeIdBufStr);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        res = UA_String_escapeAppend(&tmp, nodeIdBufStr, true);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Print the BrowsePath */
    for(size_t i = 0; i < sao->browsePathSize; i++) {
        res |= UA_String_append(&tmp, UA_STRING("/"));
        UA_QualifiedName *qn = &sao->browsePath[i];
        if(qn->namespaceIndex > 0) {
            char nsStr[8]; /* Enough for a uint16 */
            itoaUnsigned(qn->namespaceIndex, nsStr, 10);
            res |= UA_String_append(&tmp, UA_STRING(nsStr));
            res |= UA_String_append(&tmp, UA_STRING(":"));
        }
        res |= UA_String_escapeAppend(&tmp, qn->name, true);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Print the attribute name */
    if(sao->attributeId != UA_ATTRIBUTEID_VALUE) {
        res |= UA_String_append(&tmp, UA_STRING("#"));
        const char *attrName= UA_AttributeId_name((UA_AttributeId)sao->attributeId);
        res |= UA_String_append(&tmp, UA_STRING((char*)(uintptr_t)attrName));
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Print the IndexRange
     * TODO: Validate the indexRange string */
    if(sao->indexRange.length > 0) {
        res |= UA_String_append(&tmp, UA_STRING("["));
        res |= UA_String_append(&tmp, sao->indexRange);
        res |= UA_String_append(&tmp, UA_STRING("]"));
    }

 cleanup:
    /* Encoding failed, clean up */
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&tmp);
        return res;
    }

    return moveTmpToOut(&tmp, out);
}

#endif

/************************/
/* Cryptography Helpers */
/************************/

UA_ByteString
getLeafCertificate(UA_ByteString chain) {
    /* Detect DER encoded X.509 v3 certificate. If the DER detection fails,
     * return the entire chain.
     *
     * The OPC UA standard requires this to be DER. But we also allow other
     * formats like PEM. Afterwards it depends on the crypto backend to parse
     * it. mbedTLS and OpenSSL detect the format automatically. */
    if(chain.length < 4 || chain.data[0] != 0x30 || chain.data[1] != 0x82)
        return chain;

    /* The certificate length is encoded in the next 2 bytes. */
    size_t leafLen = 4; /* Magic numbers + length bytes */
    leafLen += (size_t)(((uint16_t)chain.data[2]) << 8);
    leafLen += chain.data[3];

    /* Consistency check */
    if(leafLen > chain.length)
        return UA_BYTESTRING_NULL;

    /* Adjust the length and return */
    chain.length = leafLen;
    return chain;
}

UA_Boolean
UA_constantTimeEqual(const void *ptr1, const void *ptr2, size_t length) {
    volatile const UA_Byte *a = (volatile const UA_Byte *)ptr1;
    volatile const UA_Byte *b = (volatile const UA_Byte *)ptr2;
    volatile UA_Byte c = 0;
    for(size_t i = 0; i < length; ++i) {
        UA_Byte x = a[i], y = b[i];
        c = c | (x ^ y);
    }
    return !c;
}

void
UA_ByteString_memZero(UA_ByteString *bs) {
#if defined(__STDC_LIB_EXT1__)
   memset_s(bs->data, bs->length, 0, bs->length);
#elif defined(_WIN32)
   SecureZeroMemory(bs->data, bs->length);
#else
   volatile unsigned char *volatile ptr =
       (volatile unsigned char *)bs->data;
   size_t i = 0;
   size_t maxLen = bs->length;
   while(i < maxLen) {
       ptr[i++] = 0;
   }
#endif
}
