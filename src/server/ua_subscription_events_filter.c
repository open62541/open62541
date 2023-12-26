/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

/* Ternary Logic
 * -------------
 * Similar to SQL, OPC UA Queries use the K3 - Strong Kleene Logic that
 * considers ternary values true/false/null (unknown). Most operators resolve to
 * a ternary value. Some operators can resolve to a literal value (e.g. CAST). */

typedef enum {
    UA_TERNARY_FALSE = -1,
    UA_TERNARY_NULL = 0,
    UA_TERNARY_TRUE = 1
} UA_Ternary;

static UA_Ternary
UA_Ternary_and(UA_Ternary first, UA_Ternary second) {
    if(first == UA_TERNARY_FALSE || second == UA_TERNARY_FALSE)
        return UA_TERNARY_FALSE;
    if(first == UA_TERNARY_TRUE && second == UA_TERNARY_TRUE)
        return UA_TERNARY_TRUE;
    return UA_TERNARY_NULL;
}

static UA_Ternary
UA_Ternary_or(UA_Ternary first, UA_Ternary second) {
    if(first == UA_TERNARY_TRUE || second == UA_TERNARY_TRUE)
        return UA_TERNARY_TRUE;
    if(first == UA_TERNARY_NULL || second == UA_TERNARY_NULL)
        return UA_TERNARY_TRUE;
    return UA_TERNARY_FALSE;
}

/* Part 4: The NOT operator always evaluates to NULL if applied to a NULL
 * operand. We simply swap true/false. */
static UA_Ternary
UA_Ternary_not(UA_Ternary v) {
    return (UA_Ternary)((int)v * -1);
}

static UA_Ternary v2t(const UA_Variant *v) {
    if(UA_Variant_isEmpty(v) || !UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_BOOLEAN]))
        return UA_TERNARY_NULL;
    UA_Boolean b = *(UA_Boolean*)v->data;
    return (b) ? UA_TERNARY_TRUE : UA_TERNARY_FALSE;
}

static const UA_Boolean bFalse = false;
static const UA_Boolean bTrue  = true;

static UA_Variant t2v(UA_Ternary t) {
    UA_Variant v;
    UA_Variant_init(&v);
    switch(t) {
    case UA_TERNARY_FALSE:
        UA_Variant_setScalar(&v, (void*)(uintptr_t)&bFalse, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    case UA_TERNARY_TRUE:
        UA_Variant_setScalar(&v, (void*)(uintptr_t)&bTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        break;
    default:
        return v;
    }
    v.storageType = UA_VARIANT_DATA_NODELETE;
    return v;
}

/* Type Casting Rules
 * ------------------
 * For comparison operations values from different datatypes can be implicitly
 * or explicitly cast. The standard defines rules to selected the target type
 * for casting and when implicit casts are possible. */

/* Defined in Part 4 Table 123 "Data Precedence Rules". Implicit casting is
 * always to the type of lower precedence value. */
static UA_Byte typePrecedence[UA_DATATYPEKINDS] = {
    12, 10, 11, 8, 9, 5, 6, 3, 4, 2, 1, 14, 255, 13, 255, 255, 16,
    15, 7, 18, 17, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

/* Type conversion table from the standard.
 * 0 -> Same Type, 1 -> Implicit Cast, 2 -> Only explicit Cast, -1 -> cast invalid */
static UA_SByte convertLookup[21][21] = {
    { 0, 1,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 1,-1, 2,-1,-1, 1, 1, 1,-1},
    { 2, 0,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 1,-1, 2,-1,-1, 1, 1, 1,-1},
    {-1,-1, 0,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 0,-1, 2,-1, 2, 2, 2,-1, 2,-1, 2,-1,-1, 2, 2, 2,-1},
    {-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1, 2,-1,-1, 1,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 0,-1, 2, 2, 2,-1, 2,-1, 2,-1,-1, 2, 2, 2,-1},
    {-1,-1, 2,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 0, 1, 1,-1, 2,-1, 2,-1,-1, 2, 1, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 0, 1,-1, 2, 2, 2,-1,-1, 2, 2, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 2, 0,-1, 2, 2, 2,-1,-1, 2, 2, 2,-1},
    {-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 0,-1, 2,-1,-1, 1, 1, 1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1, 0,-1,-1,-1, 2, 1, 1,-1},
    { 1, 1,-1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,-1, 0, 2, 2, 1, 1, 1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 0,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 0,-1,-1,-1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 1, 1, 1,-1, 2, 1, 2,-1,-1, 0, 1, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 1, 1,-1, 2, 2, 2,-1,-1, 2, 0, 1,-1},
    { 2, 2,-1,-1, 1,-1, 1,-1, 2, 2, 1,-1, 2, 2, 2,-1,-1, 2, 2, 0,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0}
};

/* This array maps the index of the DataType-Kind to the index of the type
 * conversion matrix (255 -> not contained in the matrix) */
static UA_Byte convertLookupIndex[UA_DATATYPEKINDS] = {
    0,  12,  1,  8, 17,  9, 18, 10, 19,  6, 4,  14,  3,  7,  2,
    20, 11,  5, 13, 16, 15,255,255,255,255,255,255,255,255,255,255
};

/* Returns the target type for implicit cassting or NULL if no implicit casting
 * is possible */
static const UA_DataType *
implicitCastTargetType(const UA_DataType *t1, const UA_DataType *t2) {
    if(!t1 || t1 == t2)
        return t1;

    /* Get the type precedence. Return if no implicit casting is possible. */
    UA_Byte p1 = typePrecedence[t1->typeKind];
    UA_Byte p2 = typePrecedence[t2->typeKind];
    if(p1 == UA_BYTE_MAX || p2 == UA_BYTE_MAX)
        return NULL;

    /* Which is the target type based on the precedence rules? */
    const UA_DataType *targetType = (p1 < p2) ? t1 : t2;
    const UA_DataType *sourceType = (p1 < p2) ? t2 : t1;

    /* Lookup the casting rule */
    UA_Byte sourceIndex = convertLookupIndex[sourceType->typeKind];
    UA_Byte targetIndex = convertLookupIndex[targetType->typeKind];
    if(sourceIndex == UA_BYTE_MAX || targetIndex == UA_BYTE_MAX)
        return NULL;
    UA_SByte castingRule = convertLookup[sourceIndex][targetIndex];

    /* Is implicit casting allowed? */
    if(castingRule != 0 && castingRule != 1)
        return NULL;
    return targetType;
}

/* Cast Numerical
 * --------------
 * To reduce the number of cases we first "normalize" to either of
 * Int64/UInt64/Double. Then cast from there to the target type. This works for
 * all numericals (not only the implicit conversion).
 *
 * Numerical conversion rules from the standard:
 *
 * - If the conversion fails the result is a null value.
 * - Floating point values are rounded by adding 0.5 and truncating when they
 *   are converted to integer values.
 * - Converting a value that is outside the range of the target type causes a
 *   conversion error. */

#define UA_CAST_SIGNED(t, T)                                         \
    if(i < T##_MIN || (i > 0 && (t)i > T##_MAX))                     \
        return;                                                      \
    *(t*)data = (t)i;                                                \
    do { } while(0)

#define UA_CAST_UNSIGNED(t, T)                                       \
    if(u > T##_MAX)                                                  \
        return;                                                      \
    *(t*)data = (t)u;                                                \
    do { } while(0)

#define UA_CAST_FLOAT(t, T)                                          \
    if(f + 0.5 < (UA_Double)T##_MIN || f + 0.5 > (UA_Double)T##_MAX) \
        return;                                                      \
    *(t*)data = (t)(f + 0.5);                                        \
    do { } while(0)

/* We can cast between any numerical type. So this can be reused for explicit casting. */
static void
castNumerical(const UA_Variant *in, const UA_DataType *type, UA_Variant *out) {
    UA_assert(UA_Variant_isScalar(in));
    UA_Variant_init(out); /* Set to null value */

    UA_Int64  i = 0;
    UA_UInt64 u = 0;
    UA_Double f = 0.0;

    const UA_DataTypeKind ink = (UA_DataTypeKind)in->type->typeKind;
    switch(ink) {
    case UA_DATATYPEKIND_SBYTE:  i = *(UA_SByte*)in->data; break;
    case UA_DATATYPEKIND_INT16:  i = *(UA_Int16*)in->data; break;
    case UA_DATATYPEKIND_INT32:  i = *(UA_Int32*)in->data; break;
    case UA_DATATYPEKIND_INT64:  i = *(UA_Int64*)in->data; break;
    case UA_DATATYPEKIND_BYTE: /* or */
    case UA_DATATYPEKIND_BOOLEAN: u = *(UA_Byte*)in->data; break;
    case UA_DATATYPEKIND_UINT16:  u = *(UA_UInt16*)in->data; break;
    case UA_DATATYPEKIND_UINT32: /* or */
    case UA_DATATYPEKIND_STATUSCODE: u = *(UA_UInt32*)in->data; break;
    case UA_DATATYPEKIND_UINT64: u = *(UA_UInt64*)in->data; break;
    case UA_DATATYPEKIND_FLOAT:  f = *(UA_Float*)in->data; break;
    case UA_DATATYPEKIND_DOUBLE: f = *(UA_Double*)in->data; break;
    default: return;
    }

    void *data = UA_new(type);
    if(!data)
        return;

    if(ink == UA_DATATYPEKIND_SBYTE || ink == UA_DATATYPEKIND_INT16 ||
       ink == UA_DATATYPEKIND_INT32 || ink == UA_DATATYPEKIND_INT64) {
        /* Cast from signed */
        switch(type->typeKind) {
        case UA_DATATYPEKIND_SBYTE:  UA_CAST_SIGNED(UA_SByte, UA_SBYTE); break;
        case UA_DATATYPEKIND_INT16:  UA_CAST_SIGNED(UA_Int16, UA_INT16); break;
        case UA_DATATYPEKIND_INT32:  UA_CAST_SIGNED(UA_Int32, UA_INT32); break;
        case UA_DATATYPEKIND_INT64:  *(UA_Int64*)data = i; break;
        case UA_DATATYPEKIND_BYTE:   UA_CAST_SIGNED(UA_Byte, UA_BYTE); break;
        case UA_DATATYPEKIND_UINT16: UA_CAST_SIGNED(UA_UInt16, UA_UINT16); break;
        case UA_DATATYPEKIND_UINT32: UA_CAST_SIGNED(UA_UInt32, UA_UINT32); break;
        case UA_DATATYPEKIND_UINT64: UA_CAST_SIGNED(UA_UInt64, UA_UINT64); break;
        case UA_DATATYPEKIND_FLOAT:  *(UA_Float*)data = (UA_Float)i; break;
        case UA_DATATYPEKIND_DOUBLE: *(UA_Double*)data = (UA_Double)i; break;
        default:
            UA_free(data);
            return;
        }
    } else if(ink == UA_DATATYPEKIND_BYTE   || ink == UA_DATATYPEKIND_UINT16 ||
              ink == UA_DATATYPEKIND_UINT32 || ink == UA_DATATYPEKIND_UINT64) {
        /* Cast from unsigned */
        switch(type->typeKind) {
        case UA_DATATYPEKIND_SBYTE:  UA_CAST_UNSIGNED(UA_SByte, UA_SBYTE); break;
        case UA_DATATYPEKIND_INT16:  UA_CAST_UNSIGNED(UA_Int16, UA_INT16); break;
        case UA_DATATYPEKIND_INT32:  UA_CAST_UNSIGNED(UA_Int32, UA_INT32); break;
        case UA_DATATYPEKIND_INT64:  UA_CAST_UNSIGNED(UA_Int64, UA_INT64); break;
        case UA_DATATYPEKIND_BYTE:   UA_CAST_UNSIGNED(UA_Byte, UA_BYTE); break;
        case UA_DATATYPEKIND_UINT16: UA_CAST_UNSIGNED(UA_UInt16, UA_UINT16); break;
        case UA_DATATYPEKIND_UINT32: UA_CAST_UNSIGNED(UA_UInt32, UA_UINT32); break;
        case UA_DATATYPEKIND_UINT64: *(UA_UInt64*)data = u; break;
        case UA_DATATYPEKIND_FLOAT:  *(UA_Float*)data = (UA_Float)u; break;
        case UA_DATATYPEKIND_DOUBLE: *(UA_Double*)data = (UA_Double)u; break;
        default:
            UA_free(data);
            return;
        }
    } else {
        /* Cast from float */
        if(f != f) {
            /* NaN cannot be cast */
            UA_free(data);
            return;
        }
        switch(type->typeKind) {
        case UA_DATATYPEKIND_SBYTE:  UA_CAST_FLOAT(UA_SByte, UA_SBYTE); break;
        case UA_DATATYPEKIND_INT16:  UA_CAST_FLOAT(UA_Int16, UA_INT16); break;
        case UA_DATATYPEKIND_INT32:  UA_CAST_FLOAT(UA_Int32, UA_INT32); break;
        case UA_DATATYPEKIND_INT64:  UA_CAST_FLOAT(UA_Int64, UA_INT64); break;
        case UA_DATATYPEKIND_BYTE:   UA_CAST_FLOAT(UA_Byte, UA_BYTE); break;
        case UA_DATATYPEKIND_UINT16: UA_CAST_FLOAT(UA_UInt16, UA_UINT16); break;
        case UA_DATATYPEKIND_UINT32: UA_CAST_FLOAT(UA_UInt32, UA_UINT32); break;
        case UA_DATATYPEKIND_UINT64: UA_CAST_FLOAT(UA_UInt64, UA_UINT64); break;
        case UA_DATATYPEKIND_FLOAT:  *(UA_Float*)data = (UA_Float)f; break;
        case UA_DATATYPEKIND_DOUBLE: *(UA_Double*)data = (UA_Double)f; break;
        default:
            UA_free(data);
            return;
        }
    }

    UA_Variant_setScalar(out, data, type);
}

/* Implicit Casting
 * ---------------- */

static UA_INLINE UA_Byte uppercase(UA_Byte in) { return in | 32; }

static UA_StatusCode
castImplicitFromString(const UA_Variant *in, const UA_DataType *outType, UA_Variant *out) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(outType == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        /* String -> Boolean
         *
         * Part 4 says: String values containing "true", "false", "1" or "0"
         * can be converted to Boolean values. Other string values cause a
         * conversion error. In this case Strings are case-insensitive. */
        UA_Boolean b;
        const UA_String *inStr = (const UA_String*)in->data;
        if(inStr->length == 1 && inStr->data[0] == '0') {
            b = false;
        } else if(inStr->length == 1 && inStr->data[0] == '1') {
            b = true;
        } else if(inStr->length == 4 &&
                  uppercase(inStr->data[0])== 'T' && uppercase(inStr->data[1])== 'R' &&
                  uppercase(inStr->data[2])== 'U' && uppercase(inStr->data[3])== 'E') {
            b = true;
        } else if(inStr->length == 5              && uppercase(inStr->data[0])== 'F' &&
                  uppercase(inStr->data[1])== 'A' && uppercase(inStr->data[2])== 'L' &&
                  uppercase(inStr->data[3])== 'S' && uppercase(inStr->data[4])== 'E') {
            b = false;
        } else {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
        return UA_Variant_setScalarCopy(out, &b, outType);
    }

#ifdef UA_ENABLE_PARSING
    else if(outType == &UA_TYPES[UA_TYPES_GUID]) {
        /* String -> Guid */
        UA_Guid guid;
        res = UA_Guid_parse(&guid, *(UA_String*)in->data);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        return UA_Variant_setScalarCopy(out, &guid, outType);
    }
#endif

#ifdef UA_ENABLE_JSON_ENCODING
    /* String -> Numerical, uses the JSON decoding */
    else if(UA_DataType_isNumeric(outType)) {
        void *outData = UA_new(outType);
        if(!outData)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = UA_decodeJson((const UA_ByteString*)in->data, outData, outType, NULL);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free(outData);
            return res;
        }
        UA_Variant_setScalar(out, outData, outType);
        return UA_STATUSCODE_GOOD;
    }
#endif

    /* No implicit casting possible */
    return UA_STATUSCODE_BADTYPEMISMATCH;
}

static UA_StatusCode
castImplicit(const UA_Variant *in, const UA_DataType *outType, UA_Variant *out) {
    /* Of the input is empty, casting results in a NULL value */
    if(UA_Variant_isEmpty(in)) {
        UA_Variant_init(out);
        return UA_STATUSCODE_GOOD;
    }

    /* TODO: We only support scalar values for now */
    if(!UA_Variant_isScalar(in))
        return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;

    /* No casting necessary */
    if(in->type == outType) {
        *out = *in;
        out->storageType = UA_VARIANT_DATA_NODELETE;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(in->type->typeKind) {
    case UA_DATATYPEKIND_EXPANDEDNODEID: {
        /* ExpandedNodeId -> String */
        if(outType != &UA_TYPES[UA_TYPES_STRING])
            break;
        UA_String *outStr = UA_String_new();
        if(!outStr)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = UA_ExpandedNodeId_print((const UA_ExpandedNodeId*)in->data, outStr);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free(outStr);
            break;
        }
        UA_Variant_setScalar(out, outStr, outType);
        break;
    }

    case UA_DATATYPEKIND_NODEID: {
        if(outType == &UA_TYPES[UA_TYPES_STRING]) {
            /* NodeId -> String */
            UA_String *outStr = UA_String_new();
            if(!outStr)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            res = UA_NodeId_print((const UA_NodeId*)in->data, outStr);
            if(res != UA_STATUSCODE_GOOD) {
                UA_free(outStr);
                break;
            }
            UA_Variant_setScalar(out, outStr, outType);
        } else if(outType == &UA_TYPES[UA_TYPES_EXPANDEDNODEID]) {
            /* NodeId -> ExpandedNodeId */
            UA_ExpandedNodeId *eid = UA_ExpandedNodeId_new();
            if(!eid)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            res = UA_NodeId_copy((const UA_NodeId*)in->data, &eid->nodeId);
            if(res != UA_STATUSCODE_GOOD) {
                UA_free(eid);
                break;
            }
            UA_Variant_setScalar(out, eid, outType);
        }
        break;
    }

    case UA_DATATYPEKIND_STRING:
        res = castImplicitFromString(in, outType, out);
        break;

    case UA_DATATYPEKIND_LOCALIZEDTEXT: {
        if(outType != &UA_TYPES[UA_TYPES_STRING])
            break;
        /* LocalizedText -> String */
        UA_LocalizedText *inLT = (UA_LocalizedText*)in->data;
        res = UA_Variant_setScalarCopy(out, &inLT->text, outType);
        break;
    }

    case UA_DATATYPEKIND_QUALIFIEDNAME: {
        UA_QualifiedName *inQN = (UA_QualifiedName*)in->data;
        if(outType == &UA_TYPES[UA_TYPES_STRING]) {
            /* QualifiedName -> String */
            res = UA_Variant_setScalarCopy(out, &inQN->name, outType);
        } else if(outType == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]) {
            /* QualifiedName -> LocalizedText */
            UA_LocalizedText lt;
            lt.text = inQN->name;
            lt.locale = UA_STRING_NULL;
            res = UA_Variant_setScalarCopy(out, &lt, outType);
        }
        break;
    }

    default:
        /* Try casting between numericals (also works for Boolean and StatusCode
         * input). The conversion can fail if the limits of the output type are
         * exceeded and then results in a NULL value. */
        castNumerical(in, outType, out);
    }

    return res;
}

/* Filter Evaluation
 * ----------------- */

typedef struct {
    UA_Server *server;
    UA_Session *session;
    const UA_NodeId *eventNode;
    const UA_ContentFilter *filter;
    UA_ContentFilterResult *filterResult;
    UA_Variant results[UA_EVENTFILTER_MAXELEMENTS];

    /* The stack contains temporary variants. Cleaned up after the evaluation of
     * each operator. */
    size_t top;
    UA_Variant stack[UA_EVENTFILTER_MAXOPERANDS];
} UA_FilterEvalContext;

/* Operand Resolving
 * ~~~~~~~~~~~~~~~~~
 * Methods that all resolve an operator operand to a Variant. */

/* Part 4, 7.4.4.5 SimpleAttributeOperand: The clause can point to any attribute
 * of nodes. Either a child of the event node and also the event type. */
static UA_StatusCode
resolveSimpleAttributeOperand(UA_Server *server, UA_Session *session,
                              const UA_NodeId *origin,
                              const UA_SimpleAttributeOperand *sao,
                              UA_Variant *value) {
    /* Prepare the ReadValueId */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.indexRange = sao->indexRange;
    rvi.attributeId = sao->attributeId;

    /* Read the value */
    UA_DataValue v;
    if(sao->browsePathSize == 0) {
        /* If this list (browsePath) is empty, the Node is the instance of the
         * TypeDefinition. (Part 4, 7.4.4.5) */
        rvi.nodeId = *origin;

        /* A Condition is an indirection. Look up the target node. */
        /* TODO: check for Branches! One Condition could have multiple Branches */
        UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
        if(UA_NodeId_equal(&sao->typeDefinitionId, &conditionTypeId)) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
            UA_StatusCode res = UA_getConditionId(server, origin, &rvi.nodeId);
            UA_CHECK_STATUS(res, return res);
#else
            return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
        }

        v = readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    } else {
        /* Resolve the browse path, starting from the event-source (and not the
         * typeDefinitionId). */
        UA_BrowsePathResult bpr =
            browseSimplifiedBrowsePath(server, *origin,
                                       sao->browsePathSize, sao->browsePath);
        if(bpr.targetsSize == 0 && bpr.statusCode == UA_STATUSCODE_GOOD)
            bpr.statusCode = UA_STATUSCODE_BADNOTFOUND;
        if(bpr.statusCode != UA_STATUSCODE_GOOD) {
            UA_StatusCode res = bpr.statusCode;
            UA_BrowsePathResult_clear(&bpr);
            return res;
        }

        /* Use the first match */
        rvi.nodeId = bpr.targets[0].targetId.nodeId;
        v = readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Validate the result */
    if(v.status != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&v.value);
        return v.status;
    }
    if(!v.hasValue)
        return UA_STATUSCODE_BADNODATAAVAILABLE;

    /* Move the result to the output */
    *value = v.value;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
resolveOperand(UA_FilterEvalContext *ctx, UA_ExtensionObject *op, UA_Variant *out) {
    if(op->encoding != UA_EXTENSIONOBJECT_DECODED &&
       op->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE)
        return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;

    /* Result of an operator that was evaluated prior */
    if(op->content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
        UA_ElementOperand *eo = (UA_ElementOperand*)op->content.decoded.data;
        *out = ctx->results[eo->index];
        out->storageType = UA_VARIANT_DATA_NODELETE;
        return UA_STATUSCODE_GOOD;
    }

    /* Literal value */
    if(op->content.decoded.type == &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
        UA_LiteralOperand *lo = (UA_LiteralOperand*)op->content.decoded.data;
        *out = lo->value;
        out->storageType = UA_VARIANT_DATA_NODELETE;
        return UA_STATUSCODE_GOOD;
    }

    /* SimpleAttributeOperand with a BrowsePath */
    if(op->content.decoded.type == &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]) {
        UA_SimpleAttributeOperand *sao =
            (UA_SimpleAttributeOperand*)op->content.decoded.data;
        return resolveSimpleAttributeOperand(ctx->server, ctx->session,
                                             ctx->eventNode, sao, out);
    }

    return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
}

/* The operandIndex is within the operator arguments, not the operand index for
 * the overall stack */
static UA_StatusCode
setOperandError(UA_FilterEvalContext *ctx, size_t elementIndex,
                size_t operandIndex, UA_StatusCode statusCode) {
    UA_ContentFilterElementResult *res = &ctx->filterResult->elementResults[elementIndex];
    res->operandStatusCodes[operandIndex] = statusCode;
    /* The operator status is set globally in a single location upwards the call chain
     * res->statusCode = statusCode; */
    return statusCode;
}

/* Filter Operators
 * ~~~~~~~~~~~~~~~~ */

static UA_StatusCode
ofTypeOperator(UA_FilterEvalContext *ctx, size_t index) {
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    UA_assert(elm->filterOperandsSize == 1);

    /* Get the operand. Must be a literal NodeId */
    UA_Variant *op0 = &ctx->stack[ctx->top++];
    UA_StatusCode res = resolveOperand(ctx, &elm->filterOperands[0], op0);
    if(res != UA_STATUSCODE_GOOD || !UA_Variant_hasScalarType(op0, &UA_TYPES[UA_TYPES_NODEID]))
        return setOperandError(ctx, index, 0, UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED);

    /* Read the event type */
    UA_Variant eventTypeVar;
    UA_Variant_init(&eventTypeVar);
    const UA_NodeId *operandTypeId = (const UA_NodeId *)op0->data;
    res = readObjectProperty(ctx->server, *ctx->eventNode,
                             UA_QUALIFIEDNAME(0, "EventType"), &eventTypeVar);
    UA_CHECK_STATUS(res, return res);

    if(!UA_Variant_hasScalarType(&eventTypeVar, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_LOG_WARNING(ctx->server->config.logging, UA_LOGCATEGORY_SERVER,
                       "EventType has an invalid type.");
        UA_Variant_clear(&eventTypeVar);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check if the eventtype is equal to the operand or a subtype of it */
    const UA_NodeId *eventTypeId = (UA_NodeId*)eventTypeVar.data;
    UA_Boolean ofType = isNodeInTree_singleRef(ctx->server, eventTypeId, operandTypeId,
                                               UA_REFERENCETYPEINDEX_HASSUBTYPE);
    ctx->results[index] = t2v(ofType ? UA_TERNARY_TRUE : UA_TERNARY_FALSE);
    UA_Variant_clear(&eventTypeVar);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
andOperator(UA_FilterEvalContext *ctx, size_t index) {
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    UA_assert(elm->filterOperandsSize == 2);
    UA_Variant *op0 = &ctx->stack[ctx->top++];
    UA_StatusCode res = resolveOperand(ctx, &elm->filterOperands[0], op0);
    UA_CHECK_STATUS(res, return res);
    UA_Variant *op1 = &ctx->stack[ctx->top++];
    res = resolveOperand(ctx, &elm->filterOperands[1], op1);
    UA_CHECK_STATUS(res, return res);
    ctx->results[index] = t2v(UA_Ternary_and(v2t(op0), v2t(op1)));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
orOperator(UA_FilterEvalContext *ctx, size_t index) {
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    UA_assert(elm->filterOperandsSize == 2);
    UA_Variant *op0 = &ctx->stack[ctx->top++];
    UA_StatusCode res = resolveOperand(ctx, &elm->filterOperands[0], op0);
    UA_CHECK_STATUS(res, return res);
    UA_Variant *op1 = &ctx->stack[ctx->top++];
    res = resolveOperand(ctx, &elm->filterOperands[1], op1);
    UA_CHECK_STATUS(res, return res);
    ctx->results[index] = t2v(UA_Ternary_or(v2t(op0), v2t(op1)));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
notOperator(UA_FilterEvalContext *ctx, size_t index) {
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    UA_assert(elm->filterOperandsSize == 1);
    UA_Variant *op0 = &ctx->stack[ctx->top++];
    UA_StatusCode res = resolveOperand(ctx, &elm->filterOperands[0], op0);
    UA_CHECK_STATUS(res, return res);
    ctx->results[index] = t2v(UA_Ternary_not(v2t(op0)));
    return UA_STATUSCODE_GOOD;
}

/* Resolves the operands and casts them implicitly to the same type.
 * The result is set at &ctx->stack[ctx->top] (for the initial value of top). */
static UA_StatusCode
castResolveOperands(UA_FilterEvalContext *ctx, size_t index, UA_Boolean setError) {
    /* Enough space on the stack left? */
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    if(ctx->top + elm->filterOperandsSize > UA_EVENTFILTER_MAXOPERANDS)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Resolve all operands */
    UA_assert(ctx->top == 0); /* Assume the stack is empty */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < elm->filterOperandsSize; i++) {
        res = resolveOperand(ctx, &elm->filterOperands[i], &ctx->stack[ctx->top++]);
        UA_CHECK_STATUS(res, return res);
    }
    UA_assert(ctx->top > 0); /* Assume the stack is no longer empty */

    /* Get the datatype for casting */
    const UA_DataType *targetType = ctx->stack[0].type;
    for(size_t pos = 1; pos < ctx->top; pos++) {
        if(targetType)
            targetType = implicitCastTargetType(targetType, ctx->stack[pos].type);
        if(!targetType)
            return (setError) ? setOperandError(ctx, index, pos, res) : res;
    }

    /* Cast the operands. Put the result in the same location on the stack. */
    for(size_t pos = 0; pos < ctx->top; pos++) {
        UA_Variant orig = ctx->stack[pos];
        res = castImplicit(&orig, targetType, &ctx->stack[pos]);
        if(res != UA_STATUSCODE_GOOD)
            return (setError) ? setOperandError(ctx, index, pos, res) : res;
        if(ctx->stack[pos].data == orig.data) {
            /* Reuse the storage type of the original data if the variant is
             * identical or only the type has changed */
            ctx->stack[pos].storageType = orig.storageType;
        } else {
            UA_Variant_clear(&orig); /* Fresh allocation of the cast variant. Clean up. */
        }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
compareOperator(UA_FilterEvalContext *ctx, size_t index, UA_FilterOperator op) {
    UA_assert(ctx->filter->elements[index].filterOperandsSize == 2);

    /* Resolve and cast the operands. A failed casting results in FALSE. Note
     * that operands could cast to NULL. */
    UA_assert(ctx->top == 0); /* Assume the stack is empty */
    UA_StatusCode res = castResolveOperands(ctx, index, false);
    if(res != UA_STATUSCODE_GOOD || !ctx->stack[0].type ||
       ctx->stack[0].type != ctx->stack[1].type) {
        ctx->results[index] = t2v(UA_TERNARY_FALSE);
        return UA_STATUSCODE_GOOD;
    }
    UA_assert(ctx->top == 2); /* Assume the stack is no longer empty */

    /* The equals operator is always possible. For the other comparisons it has
     * to be an ordered type: Numerical, Boolean, StatusCode or DateTime. */
    const UA_DataType *type = ctx->stack[0].type;
    if(op != UA_FILTEROPERATOR_EQUALS && !UA_DataType_isNumeric(type) &&
       type->typeKind != UA_DATATYPEKIND_BOOLEAN &&
       type->typeKind != UA_DATATYPEKIND_STATUSCODE &&
       type->typeKind != UA_DATATYPEKIND_DATETIME)
        return setOperandError(ctx, index, 0, UA_STATUSCODE_BADFILTEROPERANDINVALID);

    /* Compute the order */
    UA_Order eq = UA_order(ctx->stack[0].data, ctx->stack[1].data, type);
    UA_Ternary operatorResult = UA_TERNARY_FALSE;
    switch(op) {
    case UA_FILTEROPERATOR_EQUALS:
    default:
        if(eq == UA_ORDER_EQ)
            operatorResult = UA_TERNARY_TRUE;
        break;
    case UA_FILTEROPERATOR_GREATERTHAN:
        if(eq == UA_ORDER_MORE)
            operatorResult = UA_TERNARY_TRUE;
        break;
    case UA_FILTEROPERATOR_LESSTHAN:
        if(eq == UA_ORDER_LESS)
            operatorResult = UA_TERNARY_TRUE;
        break;
    case UA_FILTEROPERATOR_GREATERTHANOREQUAL:
        if(eq == UA_ORDER_MORE || eq == UA_ORDER_EQ)
            operatorResult = UA_TERNARY_TRUE;
        break;
    case UA_FILTEROPERATOR_LESSTHANOREQUAL:
        if(eq == UA_ORDER_LESS || eq == UA_ORDER_EQ)
            operatorResult = UA_TERNARY_TRUE;
        break;
    }

    /* Set result as a literal value */
    ctx->results[index] = t2v(operatorResult);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
equalsOperator(UA_FilterEvalContext *ctx, size_t index) {
    return compareOperator(ctx, index, UA_FILTEROPERATOR_EQUALS);
}

static UA_StatusCode
gtOperator(UA_FilterEvalContext *ctx, size_t index) {
    return compareOperator(ctx, index, UA_FILTEROPERATOR_GREATERTHAN);
}

static UA_StatusCode
ltOperator(UA_FilterEvalContext *ctx, size_t index) {
    return compareOperator(ctx, index, UA_FILTEROPERATOR_LESSTHAN);
}

static UA_StatusCode
gteOperator(UA_FilterEvalContext *ctx, size_t index) {
    return compareOperator(ctx, index, UA_FILTEROPERATOR_GREATERTHANOREQUAL);
}

static UA_StatusCode
lteOperator(UA_FilterEvalContext *ctx, size_t index) {
    return compareOperator(ctx, index, UA_FILTEROPERATOR_LESSTHANOREQUAL);
}

static UA_StatusCode
bitwiseOperator(UA_FilterEvalContext *ctx, size_t index, UA_FilterOperator op) {
    UA_assert(ctx->filter->elements[index].filterOperandsSize == 2);

    /* Resolve and cast the operands. Note that operands could cast to NULL. */
    UA_assert(ctx->top == 0); /* Assume the stack is empty */
    UA_StatusCode res = castResolveOperands(ctx, index, true);
    UA_CHECK_STATUS(res, return res);
    UA_assert(ctx->top == 2); /* Assume we have two elements */

    /* Operands can cast to NULL */
    const UA_DataType *type = ctx->stack[0].type;
    if(!type || !UA_DataType_isNumeric(type) ||
       ctx->stack[0].type != ctx->stack[1].type)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Copy the casted literal to the result */
    res = UA_Variant_copy(&ctx->stack[0], &ctx->results[index]);
    UA_CHECK_STATUS(res, return res);

    /* Do the bitwise operation on the result data */
    UA_Byte *bytesOut = (UA_Byte*)ctx->results[index].data;
    const UA_Byte *bytes2 = (const UA_Byte*)ctx->stack[1].data;
    for(size_t i = 0; i < type->memSize; i++) {
        if(op == UA_FILTEROPERATOR_BITWISEAND)
            bytesOut[i] = bytesOut[i] & bytes2[i];
        else
            bytesOut[i] = bytesOut[i] | bytes2[i];
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
bitwiseAndOperator(UA_FilterEvalContext *ctx, size_t index) {
    return bitwiseOperator(ctx, index, UA_FILTEROPERATOR_BITWISEAND);
}

static UA_StatusCode
bitwiseOrOperator(UA_FilterEvalContext *ctx, size_t index) {
    return bitwiseOperator(ctx, index, UA_FILTEROPERATOR_BITWISEOR);
}

static UA_StatusCode
betweenOperator(UA_FilterEvalContext *ctx, size_t index) {
    UA_assert(ctx->filter->elements[index].filterOperandsSize == 3);

    /* If no implicit conversion is available and the operands are of different
     * types, the particular result is FALSE. */
    UA_assert(ctx->top == 0); /* Assume the stack is empty */
    UA_StatusCode res = castResolveOperands(ctx, index, false);
    if(res != UA_STATUSCODE_GOOD) {
        ctx->results[index] = t2v(UA_TERNARY_FALSE);
        return UA_STATUSCODE_GOOD;
    }
    UA_assert(ctx->top == 3); /* Assume we have three elements */

    /* The casting can result in NULL values or a non-numerical type */
    const UA_DataType *type = ctx->stack[0].type;
    if(!type || !UA_DataType_isNumeric(type) ||
       type != ctx->stack[1].type || type != ctx->stack[2].type)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_Order o1 = UA_order(ctx->stack[0].data, ctx->stack[1].data, type);
    UA_Order o2 = UA_order(ctx->stack[0].data, ctx->stack[2].data, type);
    UA_Ternary comp = ((o1 == UA_ORDER_MORE || o1 == UA_ORDER_EQ) &&
                       (o2 == UA_ORDER_LESS || o2 == UA_ORDER_EQ)) ?
        UA_TERNARY_TRUE : UA_TERNARY_FALSE;

    ctx->results[index] = t2v(comp);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
inListOperator(UA_FilterEvalContext *ctx, size_t index) {
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    UA_assert(elm->filterOperandsSize >= 2);
    UA_Boolean found = false;
    UA_Variant *op0 = &ctx->stack[ctx->top++];
    UA_Variant *op1 = &ctx->stack[ctx->top++];
    UA_StatusCode res = resolveOperand(ctx, &elm->filterOperands[0], op0);
    UA_CHECK_STATUS(res, return res);
    for(size_t i = 1; i < elm->filterOperandsSize && !found; i++) {
        res = resolveOperand(ctx, &elm->filterOperands[i], op1);
        if(res != UA_STATUSCODE_GOOD)
            continue;
        if(op0->type == op1->type && UA_equal(op0->data, op1->data, op0->type))
            found = true;
        UA_Variant_clear(op1);
    }
    ctx->results[index] = t2v((found) ? UA_TERNARY_TRUE: UA_TERNARY_FALSE);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
isNullOperator(UA_FilterEvalContext *ctx, size_t index) {
    const UA_ContentFilterElement *elm = &ctx->filter->elements[index];
    UA_assert(elm->filterOperandsSize == 1);
    UA_Variant *op0 = &ctx->stack[ctx->top++];
    UA_StatusCode res = resolveOperand(ctx, &elm->filterOperands[0], op0);
    UA_CHECK_STATUS(res, return res);
    ctx->results[index] = t2v(UA_Variant_isEmpty(op0) ? UA_TERNARY_TRUE : UA_TERNARY_FALSE);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
notImplementedOperator(UA_FilterEvalContext *ctx, size_t index) {
    return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
}

/* Filter Evaluation
 * ~~~~~~~~~~~~~~~~~ */

typedef struct {
    UA_StatusCode (*operatorMethod)(UA_FilterEvalContext *ctx, size_t index);
    UA_Byte minOperatorCount;
    UA_Byte maxOperatorCount;
} UA_FilterOperatorJumptableElement;

static const UA_FilterOperatorJumptableElement operatorJumptable[18] = {
    {equalsOperator, 2, 2},
    {isNullOperator, 1, 1},
    {gtOperator, 2, 2},
    {ltOperator, 2, 2},
    {gteOperator, 2, 2},
    {lteOperator, 2, 2},
    {notImplementedOperator, 0, UA_EVENTFILTER_MAXOPERANDS}, /* like */
    {notOperator, 1, 1},
    {betweenOperator, 3, 3},
    {inListOperator, 2, UA_EVENTFILTER_MAXOPERANDS},
    {andOperator, 2, 2},
    {orOperator, 2, 2},
    {notImplementedOperator, 0, UA_EVENTFILTER_MAXOPERANDS}, /* cast */
    {notImplementedOperator, 0, UA_EVENTFILTER_MAXOPERANDS}, /* in view */
    {ofTypeOperator, 1, 1},
    {notImplementedOperator, 0, UA_EVENTFILTER_MAXOPERANDS}, /* related to */
    {bitwiseAndOperator, 2, 2},
    {bitwiseOrOperator, 2, 2}
};

UA_StatusCode
evaluateWhereClause(UA_Server *server, UA_Session *session, const UA_NodeId *eventNode,
                    const UA_ContentFilter *contentFilter,
                    UA_ContentFilterResult *contentFilterResult) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* An empty filter always succeeds */
    if(contentFilter->elementsSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Prepare the context */
    UA_FilterEvalContext ctx;
    ctx.filterResult = contentFilterResult;
    ctx.filter = contentFilter;
    ctx.server = server;
    ctx.session = session;
    ctx.eventNode = eventNode;
    ctx.top = 0;

    /* Pacify some compilers by initializing the first result */
    UA_Variant_init(&ctx.results[0]);

    /* Evaluate the filter. Iterate backwards over the filter elements and
     * resolve each. This ensures that all element-index operands point to an
     * evaluated element. */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    int i = (int)contentFilter->elementsSize - 1;
    for(; i >= 0; i--) {
        UA_ContentFilterElement *cfe = &contentFilter->elements[i];
        res = operatorJumptable[cfe->filterOperator].operatorMethod(&ctx, (size_t)i);
        for(size_t j = 0; j < ctx.top; j++)
            UA_Variant_clear(&ctx.stack[j]); /* clean up the stack */
        ctx.top = 0;
        if(res != UA_STATUSCODE_GOOD)
            break;
    }

    /* The filter matches if the operator at the first position evaluates to TRUE */
    if(res == UA_STATUSCODE_GOOD && v2t(&ctx.results[0]) != UA_TERNARY_TRUE)
        res = UA_STATUSCODE_BADNOMATCH;

    /* Clean up the element result variants */
    for(int j = (int)contentFilter->elementsSize - 1; j > i; j--)
        UA_Variant_clear(&ctx.results[j]);
    return res;
}

static UA_Boolean
isValidEvent(UA_Server *server, const UA_NodeId *validEventParent,
             const UA_NodeId *eventId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Find the eventType variableNode */
    UA_QualifiedName findName = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, *eventId, 1, &findName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_clear(&bpr);
        return false;
    }

    /* Get the EventType Property Node */
    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);
    /* Read the Value of EventType Property Node (the Value should be a NodeId) */
    UA_StatusCode retval = readWithReadValue(server, &bpr.targets[0].targetId.nodeId,
                                             UA_ATTRIBUTEID_VALUE, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_BrowsePathResult_clear(&bpr);
        return false;
    }

    const UA_NodeId *tEventType = (UA_NodeId*)tOutVariant.data;

    /* Check whether the EventType is a Subtype of CondtionType (Part 9 first
     * implementation) */
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(validEventParent, &conditionTypeId) &&
       isNodeInTree_singleRef(server, tEventType, &conditionTypeId,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_BrowsePathResult_clear(&bpr);
        UA_Variant_clear(&tOutVariant);
        return true;
    }

    /* EventType is not a Subtype of CondtionType (ConditionId Clause won't be
     * present in Events, which are not Conditions) */
    /* Check whether Valid Event other than Conditions */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    UA_Boolean isSubtypeOfBaseEvent =
        isNodeInTree_singleRef(server, tEventType, &baseEventTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE);

    UA_BrowsePathResult_clear(&bpr);
    UA_Variant_clear(&tOutVariant);
    return isSubtypeOfBaseEvent;
}

UA_StatusCode
filterEvent(UA_Server *server, UA_Session *session,
            const UA_NodeId *eventNode, UA_EventFilter *filter,
            UA_EventFieldList *efl, UA_EventFilterResult *result) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(filter->selectClausesSize == 0)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    UA_EventFieldList_init(efl);
    efl->eventFields = (UA_Variant *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!efl->eventFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    efl->eventFieldsSize = filter->selectClausesSize;

    /* Empty event filter result */
    UA_EventFilterResult_init(result);
    result->selectClauseResultsSize = filter->selectClausesSize;
    result->selectClauseResults = (UA_StatusCode *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!result->selectClauseResults) {
        UA_EventFieldList_clear(efl);
        UA_EventFilterResult_clear(result);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Prepare content filter result structure */
    if(filter->whereClause.elementsSize > 0) {
        result->whereClauseResult.elementResultsSize = filter->whereClause.elementsSize;
        result->whereClauseResult.elementResults = (UA_ContentFilterElementResult *)
            UA_Array_new(filter->whereClause.elementsSize,
                         &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT]);
        if(!result->whereClauseResult.elementResults) {
            UA_EventFieldList_clear(efl);
            UA_EventFilterResult_clear(result);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        for(size_t i = 0; i < filter->whereClause.elementsSize; ++i) {
            UA_ContentFilterElementResult *er =
                &result->whereClauseResult.elementResults[i];
            UA_ContentFilterElement *cf = &filter->whereClause.elements[i];
            er->operandStatusCodesSize = cf->filterOperandsSize;
            er->operandStatusCodes = (UA_StatusCode *)
                UA_Array_new(er->operandStatusCodesSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
            if(!er->operandStatusCodes) {
                UA_EventFieldList_clear(efl);
                UA_EventFilterResult_clear(result);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
        }
    }

    /* Evaluate the where filter. Do we event need to consider the event? */
    UA_StatusCode res = evaluateWhereClause(server, session, eventNode,
                                            &filter->whereClause,
                                            &result->whereClauseResult);
    if(res != UA_STATUSCODE_GOOD){
        UA_EventFieldList_clear(efl);
        UA_EventFilterResult_clear(result);
        return res;
    }

    /* Apply the select filter */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    for(size_t i = 0; i < filter->selectClausesSize; i++) {
        UA_SimpleAttributeOperand *sc = &filter->selectClauses[i];
        /* Check if the browsePath is BaseEventType, in which case nothing more
         * needs to be checked */
        if(!UA_NodeId_equal(&sc->typeDefinitionId, &baseEventTypeId) &&
           !isValidEvent(server, &sc->typeDefinitionId, eventNode)) {
            UA_Variant_init(&efl->eventFields[i]);
            /* EventFilterResult currently isn't being used
               notification->result.selectClauseResults[i] =
                   UA_STATUSCODE_BADTYPEDEFINITIONINVALID; */
            continue;
        }

        /* Lookup the field. The overall filter can succeed even if a single
         * select-field cannot be resolved. */
        result->selectClauseResults[i] =
            resolveSimpleAttributeOperand(server, session, eventNode,
                                          sc, &efl->eventFields[i]);
    }

    return UA_STATUSCODE_GOOD;
}

/*****************************************/
/* Validation of Filters during Creation */
/*****************************************/

/* Initial select clause validation. The following checks are currently performed:
 * - Check if typedefenitionid or browsepath of any clause is NULL
 * - Check if the eventType is a subtype of BaseEventType
 * - Check if attributeId is valid
 * - Check if browsePath contains null
 * - Check if indexRange is defined and if it is parsable
 * - Check if attributeId is value */
UA_StatusCode
UA_SimpleAttributeOperandValidation(UA_Server *server,
                                    const UA_SimpleAttributeOperand *sao) {
    /* TypeDefinition is not NULL? */
    if(UA_NodeId_isNull(&sao->typeDefinitionId))
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* EventType is a subtype of BaseEventType? */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    if(!isNodeInTree_singleRef(server, &sao->typeDefinitionId,
                               &baseEventTypeId, UA_REFERENCETYPEINDEX_HASSUBTYPE))
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* AttributeId is valid ? */
    if(sao->attributeId == 0 || sao->attributeId >= 28)
        return UA_STATUSCODE_BADATTRIBUTEIDINVALID;

    /* If the BrowsePath is empty, the Node is the instance of the
     * TypeDefinition. (Part 4, 7.4.4.5) */
    if(sao->browsePathSize == 0)
        return UA_STATUSCODE_GOOD;

    /* BrowsePath contains empty BrowseNames? */
    for(size_t j = 0; j < sao->browsePathSize; ++j) {
        if(UA_QualifiedName_isNull(&sao->browsePath[j]))
            return UA_STATUSCODE_BADBROWSENAMEINVALID;
    }

    /* Get the list of subtypes from event type (including the event type itself) */
    UA_ReferenceTypeSet reftypes_interface =
        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_ExpandedNodeId *childTypeNodes = NULL;
    size_t childTypeNodesSize = 0;
    UA_StatusCode res = browseRecursive(server, 1, &sao->typeDefinitionId,
                                        UA_BROWSEDIRECTION_FORWARD, &reftypes_interface,
                                        UA_NODECLASS_OBJECTTYPE, true, &childTypeNodesSize,
                                        &childTypeNodes);
    if(res != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADATTRIBUTEIDINVALID;

    /* Is the browse path valid for one of them? */
    UA_Boolean subTypeContainField = false;
    for(size_t j = 0; j < childTypeNodesSize && !subTypeContainField; j++) {
        UA_BrowsePathResult bpr =
            browseSimplifiedBrowsePath(server, childTypeNodes[j].nodeId,
                                       sao->browsePathSize, sao->browsePath);

        if(bpr.statusCode == UA_STATUSCODE_GOOD && bpr.targetsSize > 0)
            subTypeContainField = true;
        UA_BrowsePathResult_clear(&bpr);
    }

    UA_Array_delete(childTypeNodes, childTypeNodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);

    if(!subTypeContainField)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* IndexRange is defined ? */
    if(!UA_String_isEmpty(&sao->indexRange)) {
        UA_NumericRange numericRange = UA_NUMERICRANGE("");
        if(UA_NumericRange_parse(&numericRange, sao->indexRange) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINDEXRANGEINVALID;
        UA_free(numericRange.dimensions);

        /* AttributeId has to be a value if an IndexRange is defined */
        if(sao->attributeId != UA_ATTRIBUTEID_VALUE)
            return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    return UA_STATUSCODE_GOOD;
}

/* Initial content filter (where clause) check. Current checks:
 * - Number of operands for each (supported) operator
 * - ElementOperands point forward only */
UA_ContentFilterElementResult
UA_ContentFilterElementValidation(UA_Server *server, size_t operatorIndex,
                                  size_t operatorsCount,
                                  const UA_ContentFilterElement *ef) {
    /* Initialize the result structure */
    UA_ContentFilterElementResult er;
    UA_ContentFilterElementResult_init(&er);
    er.operandStatusCodes = (UA_StatusCode *)
        UA_Array_new(ef->filterOperandsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!er.operandStatusCodes) {
        er.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return er;
    }
    er.operandStatusCodesSize = ef->filterOperandsSize;

    /* Is the operator in the defined range? Test this before the following
     * jumptable lookup. */
    if(ef->filterOperator < 0 || ef->filterOperator > UA_FILTEROPERATOR_BITWISEOR) {
        er.statusCode = UA_STATUSCODE_BADEVENTFILTERINVALID;
        return er;
    }

    /* Number of operands supported for the operator? */
    if(ef->filterOperandsSize < operatorJumptable[ef->filterOperator].minOperatorCount ||
       ef->filterOperandsSize > operatorJumptable[ef->filterOperator].maxOperatorCount) {
        er.statusCode = UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
        return er;
    }

    /* Generic validation of the operands */
    for(size_t i = 0; i < ef->filterOperandsSize; i++) {
        /* Must be a decoded ExtensionObject */
        UA_ExtensionObject *op = &ef->filterOperands[i];
        if(op->encoding != UA_EXTENSIONOBJECT_DECODED &&
           op->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) {
            er.operandStatusCodes[i] = UA_STATUSCODE_BADFILTEROPERANDINVALID;
            er.statusCode = UA_STATUSCODE_BADFILTEROPERANDINVALID;
            return er;
        }

        /* Supported type and conforming to the rules? */
        if(op->content.decoded.type == &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
            /* Part 4, 7.4.4.2 defines conditions for Element Operands: An index
             * is considered valid if its value is greater than the element
             * index it is part of and it does not Reference a non-existent
             * element. Clients shall construct filters in this way to avoid
             * circular and invalid References. */
            UA_ElementOperand *eo = (UA_ElementOperand *)op->content.decoded.data;
            if(eo->index <= operatorIndex || eo->index >= operatorsCount) {
                er.operandStatusCodes[i] = UA_STATUSCODE_BADINDEXRANGEINVALID;
                er.statusCode = UA_STATUSCODE_BADINDEXRANGEINVALID;
                return er;
            }
        } else if(op->content.decoded.type == &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]) {
            er.operandStatusCodes[i] = UA_STATUSCODE_BADNOTSUPPORTED;
            er.statusCode = UA_STATUSCODE_BADFILTERNOTALLOWED;
            return er;
        } else if(op->content.decoded.type != &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND] &&
                  op->content.decoded.type != &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
            er.operandStatusCodes[i] = UA_STATUSCODE_BADFILTEROPERANDINVALID;
            er.statusCode = UA_STATUSCODE_BADFILTEROPERANDINVALID;
            return er;
        }
    }

    /* Specific validations for different operators */
    switch(ef->filterOperator) {
        case UA_FILTEROPERATOR_INVIEW:
        case UA_FILTEROPERATOR_RELATEDTO:
            /* Not allowed for event WhereClause according to 7.17.3 in Part 4 */
            er.statusCode = UA_STATUSCODE_BADEVENTFILTERINVALID;
            break;

        case UA_FILTEROPERATOR_EQUALS:
        case UA_FILTEROPERATOR_GREATERTHAN:
        case UA_FILTEROPERATOR_LESSTHAN:
        case UA_FILTEROPERATOR_GREATERTHANOREQUAL:
        case UA_FILTEROPERATOR_LESSTHANOREQUAL:
        case UA_FILTEROPERATOR_BITWISEAND:
        case UA_FILTEROPERATOR_BITWISEOR:
        case UA_FILTEROPERATOR_BETWEEN:
        case UA_FILTEROPERATOR_INLIST:
        case UA_FILTEROPERATOR_AND:
        case UA_FILTEROPERATOR_OR:
        case UA_FILTEROPERATOR_ISNULL:
        case UA_FILTEROPERATOR_NOT:
            break;

        case UA_FILTEROPERATOR_OFTYPE: {
            /* Make sure the operand is a NodeId literal */
            UA_ExtensionObject *o = &ef->filterOperands[0];
            UA_LiteralOperand *lo = (UA_LiteralOperand *)o->content.decoded.data;
            if(o->content.decoded.type != &UA_TYPES[UA_TYPES_LITERALOPERAND] ||
               !UA_Variant_hasScalarType(&lo->value, &UA_TYPES[UA_TYPES_NODEID])) {
                er.operandStatusCodes[0] = UA_STATUSCODE_BADFILTEROPERANDINVALID;
                er.statusCode = UA_STATUSCODE_BADFILTEROPERANDINVALID;
                break;
            }

            /* Make sure the operand is a subtype of BaseEventType */
            UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
            if(!isNodeInTree_singleRef(server, (UA_NodeId *)lo->value.data, &baseEventTypeId,
                                       UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
                er.operandStatusCodes[0] = UA_STATUSCODE_BADFILTEROPERANDINVALID;
                er.statusCode = UA_STATUSCODE_BADFILTEROPERANDINVALID;
                break;
            }
            break;
        }

        case UA_FILTEROPERATOR_LIKE:
        case UA_FILTEROPERATOR_CAST:
            er.statusCode = UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
            break;

        default:
            er.statusCode = UA_STATUSCODE_BADFILTEROPERATORINVALID;
            break;
    }
    return er;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
